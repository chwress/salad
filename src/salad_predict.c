/**
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

#include "salad.h"
#include "anagram.h"

#include <sys/time.h>
#include <math.h>

#include "util/io.h"
#include "util/log.h"

#define TO_SEC(t) ((t).tv_sec+((t).tv_usec/1000000.0))


const double anacheck_ex_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheck_ex(p->bloom1, input, len, p->n);
}

const double anacheck_ex2_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheck_ex2(p->bloom1, p->bloom2, input, len, p->n);
}

const double anacheckw_ex_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheckw_ex(p->bloom1, input, len, p->n, p->delim);
}

const double anacheckw_ex2_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheckw_ex2(p->bloom1, p->bloom2, input, len, p->n, p->delim);
}


typedef struct {
	FN_ANACHECK fct;
	bloom_param_t param;

	char nan[0x100];
	float scores[BATCH_SIZE];
	FILE* const fOut;
	double totalTime;
} predict_t;

const int salad_predict_callback1(data_t* data, const size_t n, void* const usr)
{
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
	x->totalTime += diff;

	// Write scores
	char buf[0x100];
	for (size_t j = 0; j < n;  j++)
	{
		if (isnan(x->scores[j]))
		{
			fputs(x->nan, x->fOut);
		}
		else
		{
			snprintf(buf, 0x100, "%f\n", 1.0 -x->scores[j]);
			fputs(buf, x->fOut);
		}
	}

	(n <= 1 ? progress() : progress_step());
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
	for (size_t j = 0; j < n;  j++)
	{
		if (isnan(x->scores[j]))
		{
			fputs(x->nan, x->fOut);
		}
		else
		{
			snprintf(buf, 0x100, "%f\n", 1.0 -x->scores[j]);
			fputs(buf, x->fOut);
		}
	}
	return EXIT_SUCCESS;
}
#endif

const int salad_predict_stub(const config_t* const c, const data_processor_t* const dp, file_t fIn, FILE* const fOut)
{
	bloom_t good = { NULL, FALSE, {0}, 0 };
	bloom_t bad = { NULL, FALSE, {0}, 0 };

	if (!bloom_from_file("training", c->bloom, &good))
	{
		return EXIT_FAILURE;
	}

	if (c->bbloom)
	{
		if (!bloom_from_file("bad content", c->bbloom, &bad))
		{
			bloom_destroy(good.bloom);
			return EXIT_FAILURE;
		}

		if (bloom_t_diff(&good, &bad))
		{
			bloom_destroy(good.bloom);
			bloom_destroy(bad.bloom);
			status("The normal and the bad content filter were not generated with the same parameters.");
			return EXIT_FAILURE;
		}
	}

	FN_ANACHECK anacheck = (bad.bloom == NULL ?
			(good.useWGrams ? anacheckw_ex_wrapper  : anacheck_ex_wrapper) :
			(good.useWGrams ? anacheckw_ex2_wrapper : anacheck_ex2_wrapper));

	predict_t context = {
		anacheck,
		{good.bloom, bad.bloom, good.ngramLength, good.delim},
		{0}, {0.0}, fOut, 0.0
	};

	snprintf(context.nan, 0x100, "%s\n", c->nan);
#ifdef USE_NETWORK
	if (c->input_type == NETWORK)
	{
		dp->recv(&fIn, salad_predict_callback2, &context);
	}
#endif
	{
		dp->recv(&fIn, salad_predict_callback1, &context);
		print("");
		info("Net Calculation Time: %.4f seconds", context.totalTime);
	}

	if (bad.bloom != NULL)
	{
		bloom_destroy(bad.bloom);
	}
	bloom_destroy(good.bloom);
	return EXIT_SUCCESS;
}

const int salad_predict(const config_t* const c)
{
	return salad_heart(c, salad_predict_stub);
}
