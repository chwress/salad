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

#include "anagram.h"

#include <util/util.h>

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "salad.h"
#include "util.h"
#include "bloom.h"


void salad_init(salad_t* const s)
{
	assert(s != NULL);
	*s = EMPTY_SALAD_OBJECT;
}

const int salad_allocate(saladdata_t* const d, const size_t n)
{
	assert(d != NULL);

	d->len = 0;
	d->buf = (char*) calloc(n, sizeof(char));

	if (d->buf == NULL)
	{
		return EXIT_FAILURE;
	}

	d->len = n;
	return EXIT_SUCCESS;
}

const int salad_set_bloomfilter(salad_t* const s, const unsigned int filter_size, const char* const hashset)
{
	BLOOM* const b = bloom_init(filter_size, to_hashset(hashset));
	if (b == NULL)
	{
		SET_NOTSPECIFIED(s->model);
		return EXIT_FAILURE;
	}

	salad_set_bloomfilter_ex(s, b);
	return EXIT_SUCCESS;
}

void salad_set_delimiter(salad_t* const s, const char* const d)
{
	assert(s != NULL);

	to_delimiter(d, &s->delimiter);
	s->useWGrams = (s->delimiter.str != NULL && s->delimiter.str[0] != 0x00);
}

void salad_set_ngramlength(salad_t* const s, const size_t n)
{
	assert(s != NULL);

	s->ngramLength = n;
}

void salad_destroy_data(saladdata_t* const d)
{
	free(d->buf);
	d->buf = NULL;
	d->len = 0;
}

void salad_destroy(salad_t* const s)
{
	assert(s != NULL);

	switch (s->model.type)
	{
	case SALAD_MODEL_BLOOMFILTER:
		bloom_destroy(s->model.x);
		break;

	default: break;
	}

	SET_NOTSPECIFIED(s->model);

	free(s->delimiter.str);
	s->delimiter.str = NULL;
}


const int salad_train(salad_t* const s, const saladdata_t* const data, const size_t n)
{
	assert(s != NULL && data != NULL);
	BLOOM* const bloom = GET_BLOOMFILTER(s->model);

	if (s->useWGrams)
	{
		for (size_t i = 0; i < n; i++)
		{
			bloomizew_ex(bloom, data[i].buf, data[i].len, s->ngramLength, s->delimiter.d);
		}
	}
	else
	{
		for (size_t i = 0; i < n; i++)
		{
			bloomize_ex(bloom, data[i].buf, data[i].len, s->ngramLength);
		}
	}
	return EXIT_SUCCESS;
}

const int salad_predict_ex(salad_t* const s, const saladdata_t* const data, const size_t n, double* const out)
{
	assert(s != NULL && data != NULL);
	BLOOM* const bloom = GET_BLOOMFILTER(s->model);

	if (out == NULL)
	{
		return EXIT_FAILURE;
	}

	if (s->useWGrams)
	{
		for (size_t i = 0; i < n; i++)
		{
			out[i] = anacheckw_ex(bloom, data[i].buf, data[i].len, s->ngramLength, s->delimiter.d);
		}
	}
	else
	{
		for (size_t i = 0; i < n; i++)
		{
			out[i] = anacheck_ex(bloom, data[i].buf, data[i].len, s->ngramLength);
		}
	}
	return EXIT_SUCCESS;
}

const double* const salad_predict(salad_t* const s, const saladdata_t* const data, const size_t n)
{
	double* const scores = (double*) calloc(n, sizeof(double));
	if (scores == NULL)
	{
		return NULL;
	}

	if (salad_predict_ex(s, data, n, scores) != EXIT_SUCCESS)
	{
		return NULL;
	}
	return scores;
}


const int salad_spec_diff(const salad_t* const a, const salad_t* const b)
{
	assert(a != NULL);
	assert(b != NULL);

	return (a->model.type != b->model.type
			|| strcmp(a->delimiter.str, b->delimiter.str) != 0
			|| memcmp(a->delimiter.d, b->delimiter.d, 256) != 0
			|| a->useWGrams != b->useWGrams
			|| a->ngramLength != b->ngramLength);
}

const int bloom_from_file_ex(FILE* const f, salad_t* const out)
{
	BLOOM* bloom = NULL;
	const int ret = fread_model(f, &bloom, &out->ngramLength, out->delimiter.d, &out->useWGrams);
	delimiter_array_to_string(out->delimiter.d, &out->delimiter.str);

	if (ret == 0)
	{
		salad_set_bloomfilter_ex(out, bloom);
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

const int bloom_from_file(const char* const filename, salad_t* const out)
{
	assert(filename != NULL && out != NULL);

	FILE* const f = fopen(filename, "rb");
	if (f == NULL)
	{
		return EXIT_FAILURE +1;
	}

	const int ret = bloom_from_file_ex(f, out);
	fclose(f);

	return (ret == EXIT_SUCCESS ? ret : EXIT_FAILURE +2);
}


const int salad_from_file(const char* const filename, salad_t* const out)
{
	// TODO: right now there only are bloom filters!
	out->model.type = SALAD_MODEL_BLOOMFILTER;

	switch (out->model.type)
	{
	case SALAD_MODEL_BLOOMFILTER:
		return bloom_from_file(filename, out);

	default:
		return EXIT_FAILURE;
	}
}

const int salad_from_file_ex(FILE* const f, salad_t* const out)
{
	// TODO: right now there only are bloom filters!
	out->model.type = SALAD_MODEL_BLOOMFILTER;

	switch (out->model.type)
	{
	case SALAD_MODEL_BLOOMFILTER:
		return bloom_from_file_ex(f, out);

	default:
		return EXIT_FAILURE;
	}
}

const int salad_to_file(const salad_t* const s, const char* const filename)
{
	FILE* const f = fopen(filename, "wb+");
	if (f == NULL)
	{
		return EXIT_FAILURE;
	}

	const int ret = salad_to_file_ex(s, f);

	fclose(f);
	return ret;
}

const int salad_to_file_ex(const salad_t* const s, FILE* const f)
{
	if (s == NULL)
	{
		return EXIT_FAILURE;
	}

	// TODO: right now there only are bloom filters!
	BLOOM* const b = GET_BLOOMFILTER(s->model);

	const int n = fwrite_model(f, b, s->ngramLength, s->delimiter.str);
	return (n >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
