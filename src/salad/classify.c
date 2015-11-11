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

#include "classify.h"

#include "ngrams.h"

#include <stdlib.h>

const model_type_t to_model_type(const int as_binary, const int use_tokens)
{
	if (as_binary) {
		return BIT_NGRAM;

	} else if (use_tokens) {
		return TOKEN_NGRAM;

	} else {
		return BYTE_NGRAM;
	}
}


typedef struct
{
	BLOOM* bloom;
	unsigned int num_known;
	unsigned int num_ngrams;

} check_t;

static inline void check(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	check_t* const d = (check_t*) data;

	d->num_known += bloom_check_str(d->bloom, ngram, len);
	d->num_ngrams++;
}

#define CLASSIFY_1CLASS(X, bloom, input, len, n, delim)                   \
{	                                                                      \
	BLOOM* const _bloom_ = bloom;                                         \
	const char* const _input_ = input;                                    \
	const size_t _len_ = len;                                             \
	const size_t _n_ = n;                                                 \
	const delimiter_array_t _delim_ = delim;                              \
	                                                                      \
	check_t data;                                                         \
	data.bloom = _bloom_;                                                 \
	data.num_known = 0;                                                   \
	data.num_ngrams = 0;                                                  \
	                                                                      \
	extract_##X##grams(_input_, _len_, _n_, _delim_, check, &data);       \
	return ((double) (data.num_ngrams -data.num_known))/ data.num_ngrams; \
}

#define GOOD 0
#define BAD  1

static inline void check2(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	check_t* const d = (check_t*) data;

	d[GOOD].num_known += bloom_check_str(d[GOOD].bloom, ngram, len);
	d[BAD ].num_known += bloom_check_str(d[BAD ].bloom, ngram, len);
	d[BAD ].num_ngrams++;
}

#define CLASSIFY_2CLASS(X, bloom, bbloom, input, len, n, delim)                         \
{	                                                                                    \
	BLOOM* const _bloom_ = bloom;                                                       \
	BLOOM* const _bbloom_ = bbloom;                                                     \
	const char* const _input_ = input;                                                  \
	const size_t _len_ = len;                                                           \
	const size_t _n_ = n;                                                               \
	const delimiter_array_t _delim_ = delim;                                            \
	                                                                                    \
	check_t data[2];                                                                    \
	data[GOOD].bloom = _bloom_;                                                         \
	data[GOOD].num_known = 0;                                                           \
	data[GOOD].num_ngrams = 0;                                                          \
	                                                                                    \
	data[BAD].bloom = _bbloom_;                                                         \
	data[BAD].num_known = 0;                                                            \
	data[BAD].num_ngrams = 0;                                                           \
	                                                                                    \
	extract_##X##grams(_input_, _len_, _n_, _delim_, check2, &data);                    \
	return (((double)data[BAD].num_known) -data[GOOD].num_known)/ data[BAD].num_ngrams; \
}

// bit n-grams
const double classify_1class_b_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n)
{
	CLASSIFY_1CLASS(b, bloom, input, len, n, NO_DELIMITER);
}

const double classify_1class_b(bloom_param_t* const p, const char* const input, const size_t len)
{
	return classify_1class_b_ex(p->bloom1, input, len, p->n);
}

const double classify_2class_b_ex(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n)
{
	CLASSIFY_2CLASS(b, bloom, bbloom, input, len, n, NO_DELIMITER);
}

const double classify_2class_b(bloom_param_t* const p, const char* const input, const size_t len)
{
	return classify_2class_b_ex(p->bloom1, p->bloom2, input, len, p->n);
}

// byte n-grams
const double classify_1class_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n)
{
	CLASSIFY_1CLASS(n, bloom, input, len, n, NO_DELIMITER);
}

const double classify_1class(bloom_param_t* const p, const char* const input, const size_t len)
{
	return classify_1class_ex(p->bloom1, input, len, p->n);
}

const double classify_2class_ex(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n)
{
	CLASSIFY_2CLASS(n, bloom, bbloom, input, len, n, NO_DELIMITER);
}

const double classify_2class(bloom_param_t* const p, const char* const input, const size_t len)
{
	return classify_2class_ex(p->bloom1, p->bloom2, input, len, p->n);
}

// token/ word n-grams
const double classify_1class_w_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim)
{
	CLASSIFY_1CLASS(w, bloom, input, len, n, delim);
}

const double classify_1class_w(bloom_param_t* const p, const char* const input, const size_t len)
{
	return classify_1class_w_ex(p->bloom1, input, len, p->n, p->delim);
}

const double classify_2class_w_ex(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim)
{
	CLASSIFY_2CLASS(w, bloom, bbloom, input, len, n, delim);
}

const double classify_2class_w(bloom_param_t* const p, const char* const input, const size_t len)
{
	return classify_2class_w_ex(p->bloom1, p->bloom2, input, len, p->n, p->delim);
}



FN_CLASSIFIER pick_classifier(const model_type_t t, const int anomaly_detection)
{
	switch (t)
	{
	case BIT_NGRAM:
		return (anomaly_detection ? classify_1class_b : classify_2class_b);

	case BYTE_NGRAM:
		return (anomaly_detection ? classify_1class  : classify_2class );

	case TOKEN_NGRAM:
		return (anomaly_detection ? classify_1class_w : classify_2class_w);
	}
	return NULL;
}
