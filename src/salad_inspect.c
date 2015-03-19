/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2014, Christian Wressnegger
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
#include <util/log.h>

#include <inttypes.h>
#include <string.h>
#include <math.h>

void bloomizeb_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizeb_ex3(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomize_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomize_ex3(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomizew_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizew_ex3(p->bloom1, p->bloom2, str, len, p->n, p->delim, out);
}

void bloomizeb_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizeb_ex4(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomize_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomize_ex4(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomizew_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizew_ex4(p->bloom1, p->bloom2, str, len, p->n, p->delim, out);
}


FN_BLOOMIZE pick_wrapper(const model_type_t t, const int use_new)
{
	switch (t)
	{
	case BIT_NGRAM:
		return (use_new ? bloomizeb_ex3_wrapper : bloomizeb_ex4_wrapper);

	case BYTE_NGRAM:
		return (use_new ? bloomize_ex3_wrapper  : bloomize_ex4_wrapper );

	case TOKEN_NGRAM:
		return (use_new ? bloomizew_ex3_wrapper : bloomizew_ex4_wrapper);
	}
	return NULL;
}

typedef struct {
	FN_BLOOMIZE fct;
	bloom_param_t param;

	char buf[0x100];
	bloomize_stats_t* stats;
	size_t num_uniq;
	FILE* const fOut;
} inspect_t;

const int salad_inspect_callback(data_t* data, const size_t n, void* const usr)
{
	inspect_t* const x = (inspect_t*) usr;

	for (size_t i = 0; i < n; i++)
	{
		x->fct(&x->param, data[i].buf, data[i].len, &x->stats[i]);
	}

	for (size_t i = 0; i < n; i++)
	{
		snprintf(x->buf, 0x100, "%10"Z"\t%10"Z"\t%10"Z"%10"Z"\n",
					(SIZE_T) x->stats[i].new,
					(SIZE_T) x->stats[i].uniq,
					(SIZE_T) x->stats[i].total,
					(SIZE_T) data[i].len);

		fwrite(x->buf, sizeof(char), strlen(x->buf), x->fOut);
		x->num_uniq += x->stats[i].new;
	}
	return EXIT_SUCCESS;
}


const int salad_inspect_stub(const config_t* const c, const data_processor_t* const dp, file_t* const fIn, FILE* const fOut)
{
	salad_header("Inspect", &fIn->meta, c);
	SALAD_T(training);

	if (c->bloom != NULL && salad_from_file_v("training", c->bloom, &training) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// TODO: right now only bloom filter are possible
	const int newBloomFilter = (training.model.x == NULL);
	if (newBloomFilter)
	{
		salad_from_config(&training, c);
	}

	SALAD_T(cur);
	salad_from_config(&cur, c);

	const model_type_t t = to_model_type(cur.asBinary, cur.useWGrams);

	inspect_t context = {
			.fct = pick_wrapper(t, newBloomFilter),
			.param = {training.model.x, cur.model.x, cur.ngramLength, cur.delimiter.d},
			.buf = {0},
			.stats = (bloomize_stats_t*) calloc(c->batch_size, sizeof(bloomize_stats_t)),
			.num_uniq = 0,
			.fOut = fOut
	};

	dp->recv(fIn, salad_inspect_callback, c->batch_size, &context);

	BLOOM* const cur_model = (BLOOM*) cur.model.x;

	const size_t N = bloom_count(cur_model);
	info("Saturation: %.3f%%", (((double)N)/ ((double)cur_model->bitsize))*100);

	const uint8_t k = cur_model->nfuncs;
	const size_t n = context.num_uniq;
	const size_t m = cur_model->bitsize;
	info("Expected error: %.3f%%", pow(1- exp(((double) -k*n)/ ((double) m)), k) *100);

	salad_destroy(&training);
	salad_destroy(&cur);
	return EXIT_SUCCESS;
}


const int _salad_inspect_(const config_t* const c)
{
	return salad_heart(c, salad_inspect_stub);
}
