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

#include "anagram.h"

#include "hash.h"
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


const hashset_t to_hashset(const char* const str)
{
	switch (cmp(str, "simple", "murmur", NULL))
	{
	case 0: return HASHES_SIMPLE;
	case 1: return HASHES_MURMUR;
	default: break;
	}

	return HASHES_UNDEFINED;
}

const char* const hashset_to_string(hashset_t hs)
{
	switch(hs)
	{
	case HASHES_SIMPLE: return "simple";
	case HASHES_MURMUR: return "murmur";
	default: break;
	}
	return "undefined";
}


hashfunc_t HASH_FCTS[NUM_HASHFCTS] =
{
	sax_hash_n,
	sdbm_hash_n,
	bernstein_hash_n,
	murmur_hash0_n,
	murmur_hash1_n,
	murmur_hash2_n
};

const int to_hashid(hashfunc_t h)
{
	for (int j = 0; j < NUM_HASHFCTS; j++)
	{
		if (HASH_FCTS[j] == h)
		{
			return j;
		}
	}
	return -1;
}


BLOOM* const bloom_init(const unsigned short size, const hashset_t hs)
{
	assert(size <= sizeof(void*) *8);

	switch (hs)
	{
	case HASHES_SIMPLE:
		return bloom_create_ex(POW(2, size), HASHSET_SIMPLE);

	case HASHES_MURMUR:
		return bloom_create_ex(POW(2, size), HASHSET_MURMUR);

	default: break;
	}
	return NULL;
}


BLOOM* const bloom_init_from_file(FILE* const f)
{
	uint8_t nfuncs;
	hashfunc_t* hashfuncs;
	if (fread_hashspec(f, &hashfuncs, &nfuncs) < 0) return NULL;

	BLOOM* const b = bloom_create_from_file_ex(f, hashfuncs, nfuncs);
	free(hashfuncs);
	return b;
}

const int bloomfct_equal(BLOOM* const bloom, hashfunc_t* const funcs, const uint8_t nfuncs)
{
	for (uint8_t i = 0; i < nfuncs; i++)
	{
		if (bloom->funcs[i] != funcs[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}

const int bloomfct_cmp(BLOOM* const bloom, ...)
{
	va_list args;
	va_start(args, bloom);

	hashfunc_t* funcs;
	for (int i = 0; (funcs = va_arg(args, hashfunc_t*)) != NULL; i++)
	{
		// 'uint8_t' is promoted to 'int' when passed through '...'
		const uint8_t nfuncs = va_arg(args, int);
		if (nfuncs != bloom->nfuncs) continue;

		if (bloomfct_equal(bloom, funcs, nfuncs))
		{
			va_end(args);
			return i;
		}
	}
	return -1;
}

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

// file i/o

const int fwrite_hashspec(FILE* const f, BLOOM* const bloom)
{
	assert(f != NULL);

	if (fwrite(&bloom->nfuncs, sizeof(uint8_t), 1, f) != 1) return -1;

	for (uint8_t i = 0; i < bloom->nfuncs; i++)
	{
		const int id = to_hashid(bloom->funcs[i]);
		if (id < 0 || id >= 256) return -1;

		const uint8_t hid = id;
		if (fwrite(&hid, sizeof(uint8_t), 1, f) != 1) return -1;
	}

	return (1 +bloom->nfuncs) *sizeof(uint8_t);
}

const int fwrite_model(FILE* const f, BLOOM* const bloom, const size_t ngramLength, const char* const delimiter, const int asBinary)
{
	const uint8_t b = (asBinary ? 1 : 0);
	if (fwrite(&b, sizeof(uint8_t), 1, f) != 1) return -1;

	const char* const x = (delimiter == NULL ? "" : delimiter);

	size_t n = strlen(x);
	int m, l;
	if (fwrite(x, sizeof(char), n +1, f) != n +1) return -1;

	if (fwrite(&ngramLength, sizeof(size_t), 1, f) != 1) return -1;

	if ((m = fwrite_hashspec(f, bloom)) < 0) return -1;

	if ((l = bloom_to_file(bloom, f)) < 0) return -1;

	return sizeof(uint8_t) + (n+1)*sizeof(char) +sizeof(size_t) +m +l;
}

const int fread_hashspec(FILE* const f, hashfunc_t** const hashfuncs, uint8_t* const nfuncs)
{
	*nfuncs = 0;
	uint8_t fctid = 0xFF;

	const size_t nread = fread(nfuncs, sizeof(uint8_t), 1, f);
	if (nread != 1) return -1;

	*hashfuncs = (hashfunc_t*) calloc(*nfuncs, sizeof(hashfunc_t));
	if (*hashfuncs == NULL)
	{
		return -1;
	}
	for (int i = 0; i < (int) *nfuncs; i++)
	{
		if (fread(&fctid, sizeof(uint8_t), 1, f) != 1 || fctid >= NUM_HASHFCTS)
		{
			free(*hashfuncs);
			*hashfuncs = NULL;
			return -1;
		}
		(*hashfuncs)[i] = HASH_FCTS[fctid];
	}
	return nread;
}

const int fread_model(FILE* const f, BLOOM** const bloom, size_t* const ngramLength, delimiter_array_t delim, int* const useWGrams, int* const asBinary)
{
	assert(f != NULL && bloom != NULL);

	uint8_t cdummy;
	size_t n1 = fread(&cdummy, sizeof(uint8_t), 1, f);
	if (asBinary != NULL) *asBinary = cdummy;


	if (useWGrams != NULL) *useWGrams = 0;

	char* const delimiter = fread_str(f);
	if (delimiter != NULL)
	{
		if (delimiter[0] != 0x00)
		{
			if (useWGrams != NULL) *useWGrams = 1;
			if (delim != NULL) to_delimiter_array(delimiter, delim);
		}
		free(delimiter);
	}

	// n-gram length
	size_t dummy;
	size_t* const _ngramLength = (ngramLength == NULL ? &dummy : ngramLength);

	*_ngramLength = 0;
	size_t n2 = fread(_ngramLength, sizeof(size_t), 1, f);

	// The actual bloom filter values
	*bloom = bloom_init_from_file(f);

	return (n1 <= 0 || n2 <= 0 || *_ngramLength <= 0 || *bloom == NULL ? -1 : 0);
}

// n-grams
// generic implementations

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

static inline void counted_add(const char* const ngram, const size_t len, void* const data)
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
	}
	d->total++;

	bloom_add_str(d->bloom1, ngram, len);
	bloom_add_str(d->bloom2, ngram, len);
}

static inline void count(const char* const ngram, const size_t len, void* const data)
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
	}
	d->total++;

	bloom_add_str(d->bloom2, ngram, len);
}

typedef void(*FN_PROCESS_NGRAM)(const char* const ngram, const size_t len, void* const data);


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

static inline void extract_bitgrams(const char* const str, const size_t len, const size_t n, FN_PROCESS_NGRAM fct, void* const data)
{
	const ngram_mask_t mask = ((ngram_mask_t) -1) >> (int) MAX(BITGRAM_BITSIZE -n, 0);

	const char* x = str;
	for (; x < str +len -BITGRAM_SIZE; x++)
	{
		bitgram_t u = *(bitgram_t*) x;
		for (int i = 0; i < CHAR_BIT; i++)
		{
			bitgram_t cur = (u & mask);
			fct((char*) &cur, BITGRAM_SIZE, data);
			u >>= 1;
		}
	}

	bitgram_t u = *(bitgram_t*) x;
	const size_t remainder = (len -(x -str)) *8;

	for (int i = 0; i < remainder -n +1; i++)
	{
		bitgram_t cur = (u & mask);
		fct((char*) &cur, BITGRAM_SIZE, data);
		u >>= 1;
	}
}

static inline void extract_bgrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM fct, void* const data)
{
	extract_bitgrams(str, len, n, fct, data);
}

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

// ATTENTION: Due to the reasonable size of the code for extracting n-grams,
// we accept the code duplicate for the following 3 functions, in order to keep
// the runtime performance of the program high.
void bloomize_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n)
{
	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

        bloom_add_str(bloom, x, n);
    }
}

void bloomize_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const vec_t* const weights)
{
	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

	    const dim_t dim = hash(x, n);
	    if (vec_get(weights, dim) > 0.0)
	    {
	        bloom_add_str(bloom, x, n);
	    }
    }
}

void bloomize_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	if (out == NULL)
	{
		bloomize_ex(bloom1, str, len, n);
		return;
	}

	out->new = out->uniq = 0;
	out->total = (len >= n ? len -n +1 : 0);
	bloom_clear(bloom2);

	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

        if (!bloom_check_str(bloom1, x, n)) out->new++;
        bloom_add_str(bloom1, x, n);

        if (!bloom_check_str(bloom2, x, n)) out->uniq++;
        bloom_add_str(bloom2, x, n);
    }
}

void bloomize_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out)
{
	if (out == NULL)
	{
		bloomize_ex(bloom1, str, len, n);
		return;
	}

	out->new = out->uniq = 0;
	out->total = (len >= n ? len -n +1 : 0);
	bloom_clear(bloom2);

	const char* x = str;
    for (; x < str + len; x++)
    {
        // Check for sequence end
        if (x + n > str + len) break;

        if (!bloom_check_str(bloom1, x, n)) out->new++;
        if (!bloom_check_str(bloom2, x, n)) out->uniq++;
        bloom_add_str(bloom2, x, n);
    }
}


// token or word n-grams

const int pick_delimiterchar(const delimiter_array_t delim)
{
	for (size_t i = 0; i < 256; i++)
	{
		if (delim[i])
		{
			return i;
		}
	}
	return -1;
}

char* const uniquify(const char** const str, size_t* const len, const delimiter_array_t delim, const int ch)
{
	assert(str != NULL && *str != NULL && len != NULL);

	size_t slen = 0;
	char* s = (char*) malloc(*len +2);

	int prev = ch;
    for (const char* x = *str; x < *str +*len; x++)
    {
		if (delim[(unsigned char) *x])
		{
			if (prev != ch)
			{
				prev = s[slen++] = ch;
			}
		}
		else
		{
			prev = s[slen++] = *x;
		}
    }
    // make sure to end with a separator character
    if (prev != ch)
    {
    	s[slen++] = ch;
    }

    *str = s;
    *len = slen;
    return s;
}

static inline void extract_wgrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM fct, void* const data)
{
	int ch = pick_delimiterchar(delim);

	const char* s = str;
	size_t slen = len;
	// ATTENTION! "uniquify" allocates new memory stored in &s
	uniquify(&s, &slen, delim, ch);

    const char** wgrams = (const char**) malloc(sizeof(char*) *(n +1));
    wgrams[0] = s;

    // Initialize sliding window
	const char* x = s;
    for (int i = 1; x < s +slen && i < n; x++)
    {
    	if (*x == ch)
    	{
    		wgrams[i++] = x +1;
    	}
    }

    int i = n -1, a = n +1;
    for (; x < s +slen; x++)
    {
    	if (*x == ch)
    	{
    		i = (i+1) % a;
    		wgrams[i] = x +1;

    		const int y = (i +1) % a;
    		const char* const next = wgrams[y];
    		const size_t wlen =  wgrams[i] -next -1;

    		fct(next, wlen, data);
    	}
    }

    free(wgrams);
    free((void*) s);
}

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


// anagram stuff

typedef struct
{
	BLOOM* bloom;
	unsigned int numKnown;
	unsigned int numNGrams;

} anacheck_t;

void check(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	anacheck_t* const d = (anacheck_t*) data;

	d->numKnown += bloom_check_str(d->bloom, ngram, len);
	d->numNGrams++;
}

#define ANACHECK(X, bloom, input, len, n, delim)                       \
{	                                                                   \
	BLOOM* const _bloom_ = bloom;                                      \
	const char* const _input_ = input;                                 \
	const size_t _len_ = len;                                          \
	const size_t _n_ = n;                                              \
	const delimiter_array_t _delim_ = delim;                           \
	                                                                   \
	anacheck_t data;                                                   \
	data.bloom = _bloom_;                                              \
	data.numKnown = 0;                                                 \
	data.numNGrams = 0;                                                \
	                                                                   \
	extract_##X##grams(_input_, _len_, _n_, _delim_, check, &data);    \
	return ((double) (data.numNGrams -data.numKnown))/ data.numNGrams; \
}

#define GOOD 0
#define BAD  1

void check2(const char* const ngram, const size_t len, void* const data)
{
	assert(ngram != NULL && data != NULL);
	anacheck_t* const d = (anacheck_t*) data;

	d[GOOD].numKnown += bloom_check_str(d[GOOD].bloom, ngram, len);
	d[BAD ].numKnown += bloom_check_str(d[BAD ].bloom, ngram, len);
	d[BAD ].numNGrams++;
}

#define ANACHECK2(X, bloom, bbloom, input, len, n, delim)                            \
{	                                                                                 \
	BLOOM* const _bloom_ = bloom;                                                    \
	BLOOM* const _bbloom_ = bbloom;                                                  \
	const char* const _input_ = input;                                               \
	const size_t _len_ = len;                                                        \
	const size_t _n_ = n;                                                            \
	const delimiter_array_t _delim_ = delim;                                         \
	                                                                                 \
	anacheck_t data[2];                                                              \
	data[GOOD].bloom = _bloom_;                                                      \
	data[GOOD].numKnown = 0;                                                         \
	data[GOOD].numNGrams = 0;                                                        \
	                                                                                 \
	data[BAD].bloom = _bbloom_;                                                      \
	data[BAD].numKnown = 0;                                                          \
	data[BAD].numNGrams = 0;                                                         \
	                                                                                 \
	extract_wgrams(_input_, _len_, _n_, _delim_, check2, &data);                     \
	return (((double)data[BAD].numKnown) -data[GOOD].numKnown)/ data[BAD].numNGrams; \
}

const double anacheckb_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n)
{
	ANACHECK(b, bloom, input, len, n, NO_DELIMITER);
}

const double anacheckb_ex_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheckb_ex(p->bloom1, input, len, p->n);
}

const double anacheckb_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n)
{
	ANACHECK2(w, bloom, bbloom, input, len, n, NO_DELIMITER);
}

const double anacheckb_ex2_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheckb_ex2(p->bloom1, p->bloom2, input, len, p->n);
}

const double anacheck_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n)
{
	const char* x = input;

	unsigned int numKnown = 0;
	unsigned int numNGrams = 0;

	for (; x < input + len; x++)
	{
		// Check for sequence end
		if (x + n > input + len) break;

		numKnown += bloom_check_str(bloom, x, n);
		numNGrams++;
	}
	return ((double) (numNGrams -numKnown))/ numNGrams;
}

const double anacheck_ex_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheck_ex(p->bloom1, input, len, p->n);
}

const double anacheck_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n)
{
	const char* x = input;

	unsigned int numGoodKnown = 0;
	unsigned int numBadKnown = 0;
	unsigned int numNGrams = 0;

	for (; x < input + len; x++)
	{
		// Check for sequence end
		if (x + n > input + len) break;

		numGoodKnown += bloom_check_str(bloom, x, n);
		numBadKnown += bloom_check_str(bbloom, x, n);
		numNGrams++;
	}
	return (((double) (numBadKnown)) -numGoodKnown) /numNGrams;
}

const double anacheck_ex2_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheck_ex2(p->bloom1, p->bloom2, input, len, p->n);
}


const double anacheckw_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim)
{
	ANACHECK(w, bloom, input, len, n, delim);
}

const double anacheckw_ex_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheckw_ex(p->bloom1, input, len, p->n, p->delim);
}

const double anacheckw_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim)
{
	ANACHECK2(w, bloom, bbloom, input, len, n, delim);
}

const double anacheckw_ex2_wrapper(bloom_param_t* const p, const char* const input, const size_t len)
{
	return anacheckw_ex2(p->bloom1, p->bloom2, input, len, p->n, p->delim);
}


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

FN_ANACHECK pick_classifier(const model_type_t t, const int anomaly_detection)
{
	switch (t)
	{
	case BIT_NGRAM:
		return (anomaly_detection ? anacheckb_ex_wrapper : anacheckb_ex2_wrapper);

	case BYTE_NGRAM:
		return (anomaly_detection ? anacheck_ex_wrapper  : anacheck_ex2_wrapper );

	case TOKEN_NGRAM:
		return (anomaly_detection ? anacheckw_ex_wrapper : anacheckw_ex2_wrapper);
	}
	return NULL;
}
