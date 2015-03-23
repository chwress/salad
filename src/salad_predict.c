/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2015, Christian Wressnegger
 * --
 * This file is part of Letter Salad or Salad for short.
 *
 * Salad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Salad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "main.h"
#include <salad/salad.h>
#include <salad/anagram.h>
#include <salad/util.h>

#include <sys/time.h>
#include <math.h>

#include <util/io.h>
#include <util/log.h>

#define TO_SEC(t) ((t).tv_sec+((t).tv_usec/1000000.0))


typedef struct {
	FN_ANACHECK fct;
	bloom_param_t param;

	const config_t* const config;
	float* const scores;
	FILE* const out;
	double total_time;
} predict_t;


#define TO_STRING(score) (isnan(score) ? \
			x->config->nan : \
			snprintf(buf, 0x100, "%f", 1.0 -score) > 0 ? buf : "(null)")

const int salad_predict_callback(data_t* data, const size_t n, void* const usr)
{
	assert(data != NULL);
	assert(n > 0);
	assert(usr != NULL);

	predict_t* const x = (predict_t*) usr;

	struct timeval start, end;
	gettimeofday(&start, NULL);

	for (size_t i = 0; i < n; i++)
	{
		x->scores[i] = x->fct(&x->param, data[i].buf, data[i].len);
	}

	// Clock the calculation procedure
	gettimeofday(&end, NULL);
	double diff = TO_SEC(end) -TO_SEC(start);
	x->total_time += diff;

	// Write scores
	char buf[0x100];

#ifdef GROUPED_INPUT
	if (x->config->group_input)
	{
		group_t* prev = data[0].meta;
		fputs(TO_STRING(x->scores[0]), x->out);

		for (size_t j = 1; j < n; j++)
		{
			fputs(prev == data[j].meta ? " " : "\n", x->out);
			fputs(TO_STRING(x->scores[j]), x->out);
			prev = data[j].meta;
		}
	}
	else
#endif
	{
		for (size_t j = 0; j < n; j++)
		{
			fputs(TO_STRING(x->scores[j]), x->out);
			fputs("\n", x->out);
		}
	}
	return EXIT_SUCCESS;
}

#ifdef USE_NETWORK
const int salad_predict_callback2(data_t* data, const size_t n, void* const usr)
{
	predict_t* const x = (predict_t*) usr;

	for (size_t i = 0; i < n; i++)
	{
		x->scores[i] = x->fct(&x->param, data[i].buf, data[i].len);
	}

	// Write scores
	char buf[0x100];

#ifdef GROUPED_INPUT
	if (x->config->group_input)
	{
		group_t* prev = data[0].meta;
		fputs(TO_STRING(x->scores[0]), x->out);

		for (size_t j = 1; j < n; j++)
		{
			fputs(prev == data[j].meta ? " " : "\n", x->out);
			fputs(TO_STRING(x->scores[1]), x->out);
			prev = data[j].meta;
		}
	}
	else
#endif
	{
		for (size_t j = 0; j < n;  j++)
		{
			fputs(TO_STRING(x->scores[1]), x->out);
			fputs("\n", x->out);
		}
	}
	return EXIT_SUCCESS;
}
#endif

const int salad_predict_stub(const config_t* const c, const data_processor_t* const dp, file_t* const f_in, FILE* const f_out)
{
	salad_header("Predict scores of", &f_in->meta, c);
	SALAD_T(good);
	SALAD_T(bad);

	if (salad_from_file_v("training", c->bloom, &good) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}


	if (c->bbloom)
	{
		if (salad_from_file_v("bad content", c->bbloom, &bad) != EXIT_SUCCESS)
		{
			salad_destroy(&good);
			return EXIT_FAILURE;
		}

		if (salad_spec_diff(&good, &bad))
		{
			salad_destroy(&good);
			salad_destroy(&bad);
			status("The normal and the bad content filter were not generated with the same parameters.");
			return EXIT_FAILURE;
		}
		// XXX: Just checking the validity of the model ;)
		// GET_BLOOMFILTER(bad.model);
		assert(bad.model.type == SALAD_MODEL_BLOOMFILTER);
	}

	// TODO: right now there only are bloom filters!
	BLOOM* const good_model = GET_BLOOMFILTER(good.model);
	BLOOM* const bad_model = TO_BLOOMFILTER(bad.model);


	if (c->echo_params)
	{
		config_t cfg;
		cfg.ngram_length = good.ngram_length;
		cfg.filter_size = log2(good_model->bitsize);

		switch (bloomfct_cmp(good_model, HASHSET_SIMPLE, HASHSET_MURMUR))
		{
		case 0:
			cfg.hash_set = HASHES_SIMPLE;
			break;
		case 1:
			cfg.hash_set = HASHES_MURMUR;
			break;
		default:
			cfg.hash_set = HASHES_UNDEFINED;
			break;
		}

		STRDUP(good.delimiter.str, cfg.delimiter);
		echo_options(&cfg);
		free(cfg.delimiter);
	}

	const model_type_t t = to_model_type(good.as_binary, good.use_tokens);
	FN_ANACHECK anacheck = pick_classifier(t, bad_model == NULL);

	predict_t context = {
			.fct = anacheck,
			.param = {good_model, bad_model, good.ngram_length, good.delimiter.d},
			.config = c,
			// TODO: we do not know the batch size of the recv function
			.scores = (float*) calloc(c->batch_size, sizeof(float)),
			.out = f_out,
			.total_time = 0.0
	};

	dp->recv(f_in, salad_predict_callback, c->batch_size, &context);
	free(context.scores);

#ifdef USE_NETWORK
	if (c->input_type != NETWORK)
#endif
	{
		info("Net calculation time: %.4f seconds", context.total_time);

		const double size = ((double)f_in->meta.total_size) /(1024*1024 /8);
		const double throughput = size/ context.total_time;

		if (!isinf(throughput))
		{
			info("Net throughput: %.3f Mb/s", throughput);
		}
	}

	if (bad_model != NULL)
	{
		salad_destroy(&bad);
	}
	salad_destroy(&good);
	return EXIT_SUCCESS;
}

const int _salad_predict_(const config_t* const c)
{
	return salad_heart(c, salad_predict_stub);
}
