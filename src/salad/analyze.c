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

#include "analyze.h"

#include "ngrams.h"

#include <util/util.h>

#include <limits.h>
#include <string.h>


typedef struct
{
	BLOOM* bloom;
	const vec_t* weights;

} bloomize_t;

typedef struct
{
	BLOOM* bloom1;
	BLOOM* bloom2;
	size_t new, uniq, total;
} bloomize_stats_ex_t;



BLOOM* const bloomize(const char* str, const size_t len, const size_t n)
{
	BLOOM* const bloom = bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE);
	if (bloom != NULL)
	{
		bloomize_ex(bloom, str, len, n);
	}
	return bloom;
}

BLOOM* const bloomizew(const char* str, const size_t len, const size_t n, const delimiter_array_t delim)
{
	BLOOM* const bloom = bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE);
	if (bloom != NULL)
	{
		bloomizew_ex(bloom, str, len, n, delim);
	}
	return bloom;
}


// generic callback implementations
static inline void simple_add(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	bloomize_t* const d = (bloomize_t*) data;

	bloom_add_str(d->bloom, ngram, len);
}

static inline void checked_add(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	bloomize_t* const d = (bloomize_t*) data;

    const dim_t dim = hash(ngram, len);
    if (vec_get(d->weights, dim) > 0.0)
    {
		bloom_add_str(d->bloom, ngram, len);
    }
}

static inline void counted_add_ex(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	bloomize_stats_ex_t* const d = (bloomize_stats_ex_t*) data;

	if (!bloom_check_str(d->bloom1, ngram, len))
	{
		d->new++;
		bloom_add_str(d->bloom1, ngram, len);
	}
	if (!bloom_check_str(d->bloom2, ngram, len))
	{
		d->uniq++;
		bloom_add_str(d->bloom2, ngram, len);
	}
}

static inline void counted_add(const char* const ngram, const size_t len, void* const data)
{
	counted_add_ex(ngram, len, data);
	((bloomize_stats_ex_t*) data)->total++;
}

static inline void count_ex(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	bloomize_stats_ex_t* const d = (bloomize_stats_ex_t*) data;

	if (!bloom_check_str(d->bloom1, ngram, len))
	{
		d->new++;
	}
	if (!bloom_check_str(d->bloom2, ngram, len))
	{
		d->uniq++;
		bloom_add_str(d->bloom2, ngram, len);
	}
}

static inline void count(const char* const ngram, const size_t len, void* const data)
{
	count_ex(ngram, len, data);
	((bloomize_stats_ex_t*) data)->total++;
}


#define BLOOMIZE_DUAL(X, bloom1, _bloom2, str, len, n, delim, out, fct)         \
{                                                                               \
	BLOOM* const _bloom1_ = bloom1;                                             \
	BLOOM* const _bloom2_ = bloom2;                                             \
	const char* const _str_ = str;                                              \
	const size_t _len_ = len;                                                   \
	const size_t _n_ = n;                                                       \
	const delimiter_array_t _delim_ = delim;                                    \
	bloomize_stats_t* const _out_ = out;                                        \
	                                                                            \
	if (out == NULL)                                                            \
	{                                                                           \
		BLOOMIZE_EX(#X[0], _bloom1_, _str_, _len_, _n_, _delim_);               \
		return;                                                                 \
	}                                                                           \
	                                                                            \
	bloomize_stats_ex_t data;                                                   \
	data.bloom1 = _bloom1_;                                                     \
	data.bloom2 = _bloom2_;                                                     \
	data.new = data.uniq = data.total = 0;                                      \
	                                                                            \
	bloom_clear(_bloom2_);                                                      \
	extract_##X##grams(_str_, _len_, _n_, _delim_, fct, &data);                 \
	                                                                            \
	_out_->new = data.new;                                                      \
	_out_->uniq = data.uniq;                                                    \
	_out_->total = data.total;                                                  \
}

// bit n-grams
void bloomizeb_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = NULL;

	extract_bitgrams(str, len, n, simple_add, &data);
}

void bloomizeb_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const vec_t* const weights)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = weights;

	extract_bitgrams(str, len, n, checked_add, &data);
}

void bloomizeb_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	BLOOMIZE_DUAL(b, bloom1, bloom2, str, len, n, NO_DELIMITER, out, counted_add);
}

void bloomizeb_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	BLOOMIZE_DUAL(b, bloom1, bloom2, str, len, n, NO_DELIMITER, out, count);
}


// byte or character n-grams
void bloomize_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = NULL;

    extract_bytegrams(str, len, n, simple_add, &data);
}

void bloomize_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const vec_t* const weights)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = weights;

	extract_bytegrams(str, len, n, checked_add, &data);
}

void bloomize_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	BLOOMIZE_DUAL(n, bloom1, bloom2, str, len, n, NO_DELIMITER, out, counted_add_ex);
	out->total = (len >= n ? len -n +1 : 0);
}

void bloomize_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	BLOOMIZE_DUAL(n, bloom1, bloom2, str, len, n, NO_DELIMITER, out, count_ex);
	out->total = (len >= n ? len -n +1 : 0);
}


// token or word n-grams
void bloomizew_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = NULL;

	extract_wgrams(str, len, n, delim, simple_add, &data);
}

void bloomizew_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, const vec_t* const weights)
{
	bloomize_t data;
	data.bloom = bloom;
	data.weights = weights;

	extract_wgrams(str, len, n, delim, checked_add, &data);
}

void bloomizew_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, bloomize_stats_t* const out)
{
	BLOOMIZE_DUAL(w, bloom1, bloom2, str, len, n, delim, out, counted_add);
}

void bloomizew_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, bloomize_stats_t* const out)
{
	BLOOMIZE_DUAL(w, bloom1, bloom2, str, len, n, delim, out, count);
}
