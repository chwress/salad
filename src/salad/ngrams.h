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

/**
 * @file
 */

#ifndef SALAD_NGRAMS_H_
#define SALAD_NGRAMS_H_

#include "common.h"

#include <stdlib.h>
#include <limits.h>

#include <util/util.h>

typedef void(*FN_PROCESS_NGRAM)(const char* const ngram, const size_t len, void* const data);

#ifdef IS_BIGENDIAN
#define TO_BITGRAM(x) (*(bitgram_t*) x)
#define TO_CHARPTR(x, m) ((char*) &(x))
#else
#define TO_BITGRAM(x) REVERSE_REG((*(bitgram_t*) x))
#define TO_CHARPTR(x, m) ((char*) &(x) +(BITGRAM_SIZE -(m)))
#endif


// bit n-grams
inline void extract_bitgrams(const char* const str, const size_t len, const size_t n, FN_PROCESS_NGRAM fct, void* const data)
{
	const ngram_mask_t mask = ((ngram_mask_t) -1) << (int) MAX(BITGRAM_BITSIZE -n, 0);
	const size_t m = (size_t) ceil(((double) n)/8); // BITGRAM_SIZE

	const char* x = str;
	for (; x < str +len -BITGRAM_SIZE; x++)
	{
		bitgram_t u = TO_BITGRAM(x);
		for (int i = 0; i < CHAR_BIT; i++)
		{
			bitgram_t cur = (u & mask);
			fct(TO_CHARPTR(cur, m), m, data);
			u <<= 1;
		}
	}

	const ptrdiff_t d = x -str;
	assert(d > 0);
	const size_t remainder = (len -((size_t) d)) *8;

	bitgram_t u = TO_BITGRAM(x);
	for (size_t i = 0; i < remainder -n +1; i++)
	{
		bitgram_t cur = (u & mask);
		fct(TO_CHARPTR(cur, m), m, data);
		u <<= 1;
	}
}

inline void extract_bgrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM fct, void* const data)
{
	extract_bitgrams(str, len, n, fct, data);
}


// byte or character n-grams
inline void extract_bytegrams(const char* const str, const size_t len, const size_t n, FN_PROCESS_NGRAM const fct, void* const data)
{
	const char* x = str;
    for (; x <= str + len -n; x++) // num_ngrams = strlen(.) -n +1
    {
    	fct(x, n, data);
    }
}

inline void extract_ngrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM const fct, void* const data)
{
	extract_bytegrams(str, len, n, fct, data);
}


// token or word n-grams
inline const char pick_delimiterchar(const delimiter_array_t delim)
{
	for (size_t i = 0; i < DELIM_SIZE; i++)
	{
		if (delim[i])
		{
			return (char) i;
		}
	}
	return -1;
}

inline char* const uniquify(const char** const str, size_t* const len, const delimiter_array_t delim, const char ch)
{
	assert(str != NULL && *str != NULL && len != NULL);

	size_t slen = 0;
	char* s = (char*) malloc(*len +2);

	char prev = ch;
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

inline void extract_wgrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM fct, void* const data)
{
	char ch = pick_delimiterchar(delim);

	const char* s = str;
	size_t slen = len;
	// ATTENTION! "uniquify" allocates new memory stored in &s
	uniquify(&s, &slen, delim, ch);

    const char** wgrams = (const char**) malloc(sizeof(char*) *(n +1));
    wgrams[0] = s;

    // Initialize sliding window
	const char* x = s;
    for (size_t i = 1; x < s +slen && i < n; x++)
    {
    	if (*x == ch)
    	{
    		wgrams[i++] = x +1;
    	}
    }

    size_t i = n -1, a = n +1;
    for (; x < s +slen; x++)
    {
    	if (*x == ch)
    	{
    		i = (i+1) % a;
    		wgrams[i] = x +1;

    		const size_t y = (i +1) % a;
    		const char* const next = wgrams[y];

    		const ptrdiff_t d = wgrams[i] -next;
    		assert(d > 0); // and also > 1 due to construction of wgrams array
    		const size_t wlen = ((size_t) d) -1;

    		fct(next, wlen, data);
    	}
    }

    free(wgrams);
    free((void*) s);
}

#endif /* SALAD_NGRAMS_H_ */
