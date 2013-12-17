/**
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2013, Christian Wressnegger
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
#include "util/log.h"

#include <inttypes.h>
#include <string.h>
#include <math.h>


void bloomize_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomize_ex3(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomizew_ex3_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizew_ex3(p->bloom1, p->bloom2, str, len, p->n, p->delim, out);
}

void bloomize_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomize_ex4(p->bloom1, p->bloom2, str, len, p->n, out);
}

void bloomizew_ex4_wrapper(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out)
{
	bloomizew_ex4(p->bloom1, p->bloom2, str, len, p->n, p->delim, out);
}


typedef struct {
	FN_BLOOMIZE fct;
	bloom_param_t param;

	char buf[0x100];
	bloomize_stats_t stats[BATCH_SIZE];
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
	}

	(n <= 1 ? progress() : progress_step());
	return EXIT_SUCCESS;
}


const int salad_inspect_stub(const config_t* const c, const data_processor_t* const dp, file_t fIn, FILE* const fOut)
{
	bloom_t training = { NULL, FALSE, {0}, 0 };

	if (c->bloom != NULL && !bloom_from_file("training", c->bloom, &training))
	{
		return EXIT_FAILURE;
	}

	const int newBloomFilter = (training.bloom == NULL);
	if (newBloomFilter)
	{
		training.bloom = bloom_init(c->filter_size, c->hash_set);
	}

	bloom_t cur;
	cur.ngramLength = c->ngramLength;
	cur.bloom = bloom_init(c->filter_size, c->hash_set);

	const char* const delimiter = to_delim(cur.delim, c->delimiter);

	FN_BLOOMIZE bloomize = (newBloomFilter ?
			(delimiter == NULL ? bloomize_ex3_wrapper : bloomizew_ex3_wrapper) :
			(delimiter == NULL ? bloomize_ex4_wrapper : bloomizew_ex4_wrapper));


	inspect_t context = {
		bloomize,
		{training.bloom, cur.bloom, cur.ngramLength, cur.delim},
		{0}, {{0, 0, 0}}, fOut
	};

	dp->recv(&fIn, salad_inspect_callback, &context);
	print("");

	const size_t n = bloom_count(cur.bloom);
	error("Saturation: %.3f%%", (((double)n)/ ((double)cur.bloom->bitsize))*100);

	bloom_destroy(training.bloom);
	bloom_destroy(cur.bloom);
	return EXIT_SUCCESS;
}


const int salad_inspect(const config_t* const c)
{
	return salad_heart(c, salad_inspect_stub);
}
