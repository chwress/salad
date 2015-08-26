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

#include "salad.h"

#include "common.h"

#include "analyze.h"
#include "classify.h"
#include "util.h"
#include "io.h"

#include <util/util.h>
#include <container/bloom.h>

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void salad_init(salad_t* const s)
{
	assert(s != NULL);
	*s = EMPTY_SALAD_OBJECT;

	s->data = (saladcontext_t*) calloc(1, sizeof(saladcontext_t));
	delimiter_init(&_(s)->delimiter);
	_(s)->use_tokens = FALSE;
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

void salad_use_binary_ngrams(salad_t* const s, const int b)
{
	assert(s != NULL);

	s->as_binary = (b <= 0 ? FALSE : TRUE);
	salad_set_delimiter(s, _(s)->delimiter.str);
}

void salad_set_delimiter(salad_t* const s, const char* const d)
{
	assert(s != NULL);

	// TODO: We somehow need to prevent the memory leak in case this
	// function is called twice, as for instance when this function
	// is used in combination with salad_use_binary_ngrams(.,.)
	to_delimiter(d, &_(s)->delimiter);
	_(s)->use_tokens = (_(s)->delimiter.str != NULL && _(s)->delimiter.str[0] != 0x00);
}

void salad_set_ngramlength(salad_t* const s, const size_t n)
{
	assert(s != NULL);

	s->ngram_length = n;
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

	if (s->model.x != NULL)
	{
		switch (s->model.type)
		{
		case SALAD_MODEL_BLOOMFILTER:
			bloom_destroy(s->model.x);
			break;

		default: break;
		}
	}
	SET_NOTSPECIFIED(s->model);

	saladcontext_t* ctx = _(s);
	if (ctx->delimiter.str != NULL)
	{
		free(ctx->delimiter.str);
		ctx->delimiter.str = NULL;
	}
	free(ctx);
}


const int salad_train(salad_t* const s, const saladdata_t* const data, const size_t n)
{
	assert(s != NULL && data != NULL);
	BLOOM* const bloom = GET_BLOOMFILTER(s->model);

	// TODO: Let's check whether we can optimize away the function calls
	switch (to_model_type(s->as_binary, _(s)->use_tokens))
	{
	case BIT_NGRAM:
		for (size_t i = 0; i < n; i++)
		{
			bloomizeb_ex(bloom, data[i].buf, data[i].len, s->ngram_length);
		}
		break;

	case BYTE_NGRAM:
		for (size_t i = 0; i < n; i++)
		{
			bloomize_ex(bloom, data[i].buf, data[i].len, s->ngram_length);
		}
		break;

	case TOKEN_NGRAM:
		for (size_t i = 0; i < n; i++)
		{
			bloomizew_ex(bloom, data[i].buf, data[i].len, s->ngram_length, _(s)->delimiter.d);
		}
		break;

	default:
		return EXIT_FAILURE;
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

	// TODO: Let's check whether we can optimize away the function calls
	switch (to_model_type(s->as_binary, _(s)->use_tokens))
	{
	case BIT_NGRAM:
		for (size_t i = 0; i < n; i++)
		{
			out[i] = classify_1class_b_ex(bloom, data[i].buf, data[i].len, s->ngram_length);
		}
		break;

	case BYTE_NGRAM:
		for (size_t i = 0; i < n; i++)
		{
			out[i] = classify_1class_ex(bloom, data[i].buf, data[i].len, s->ngram_length);
		}
		break;

	case TOKEN_NGRAM:
		for (size_t i = 0; i < n; i++)
		{
			out[i] = classify_1class_w_ex(bloom, data[i].buf, data[i].len, s->ngram_length, _(s)->delimiter.d);
		}
		break;

	default:
		return EXIT_FAILURE;
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
			|| strcmp(_(a)->delimiter.str, _(b)->delimiter.str) != 0
			|| memcmp(_(a)->delimiter.d, _(b)->delimiter.d, 256) != 0
			|| a->as_binary != b->as_binary
			|| a->ngram_length != b->ngram_length
			|| _(a)->use_tokens != _(b)->use_tokens);
}

const int bloom_from_file_ex(FILE* const f, salad_t* const out)
{
	delimiter_destroy(&_(out)->delimiter);

	BLOOM* bloom = NULL;
	const int ret = fread_model(f, &bloom, &out->ngram_length, _(out)->delimiter.d, &_(out)->use_tokens, &out->as_binary);
	delimiter_array_to_string(_(out)->delimiter.d, &_(out)->delimiter.str);

	if (ret > 0)
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
	salad_init(out);

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
	salad_init(out);

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

	const int n = fwrite_model(f, b, s->ngram_length, _(s)->delimiter.str, s->as_binary);
	return (n >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
