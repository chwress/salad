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

#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "bloom.h"

#define SETBIT(a, n)  ((a)[(n)/CHAR_BIT] |= (0x80>>((n)%CHAR_BIT)))
#define GETBIT(a, n)  ((a)[(n)/CHAR_BIT] & (0x80>>((n)%CHAR_BIT)))

BLOOM* const bloom_create_ex(const size_t bitsize, hashfunc_t* const funcs, const uint8_t nfuncs)
{
	BLOOM* const bloom = malloc(sizeof(BLOOM));
	if (bloom == NULL)
	{
		return NULL;
	}

	const size_t size = (bitsize +CHAR_BIT -1)/CHAR_BIT;
	const size_t intsize = (size +sizeof(unsigned int) -1)/ sizeof(unsigned int);

	// We use blocks of integers in order to ease the bit counting, cf. bloom_count(.)
	bloom->a = (unsigned char*) calloc(intsize, sizeof(unsigned int));
	if (bloom->a == NULL)
	{
		free(bloom);
		return NULL;
	}

	bloom->funcs= (hashfunc_t*) malloc(nfuncs *sizeof(hashfunc_t));
	if (bloom->funcs == NULL)
	{
		free(bloom->a);
		free(bloom);
		return NULL;
	}

	for(int i = 0; i < (int) nfuncs; ++i)
	{
		bloom->funcs[i] = funcs[i];
	}

	bloom->nfuncs = nfuncs;
	bloom->bitsize = bitsize;
	bloom->size = size;

	return bloom;
}

BLOOM* const vbloom_create(const size_t size, const uint8_t nfuncs, va_list funcs)
{
	hashfunc_t* buf = (hashfunc_t*) calloc(nfuncs, sizeof(hashfunc_t));
	if (buf == NULL) return NULL;

	for (int i = 0; i < nfuncs; i++)
	{
		buf[i] = va_arg(funcs, hashfunc_t);
	}

	BLOOM* const bloom = bloom_create_ex(size, buf, nfuncs);
	free(buf);
	return bloom;
}

BLOOM* const bloom_create(const size_t size, const uint8_t nfuncs, ...)
{
	va_list args;
	va_start(args, nfuncs);

	BLOOM* const bloom = vbloom_create(size, nfuncs, args);

	va_end(args);
	return bloom;
}

BLOOM* const vbloom_create_from_file(FILE* const f, const uint8_t nfuncs, va_list funcs)
{
	hashfunc_t* buf = (hashfunc_t*) calloc(nfuncs, sizeof(hashfunc_t));
	if (buf == NULL) return NULL;

	for (int i = 0; i < nfuncs; i++)
	{
		buf[i] = va_arg(funcs, hashfunc_t);
	}

	BLOOM* const bloom = bloom_create_from_file_ex(f, buf, nfuncs);
	free(buf);
	return bloom;
}

BLOOM* const bloom_create_from_file(FILE* const f, const uint8_t nfuncs, ...)
{
	va_list args;
	va_start(args, nfuncs);

	BLOOM* const bloom = vbloom_create_from_file(f, nfuncs, args);

	va_end(args);
	return bloom;
}

BLOOM* const bloom_create_from_file_ex(FILE* const f, hashfunc_t* const funcs, const uint8_t nfuncs)
{
	assert(f != NULL);

	size_t asize;
	if (fread(&asize, sizeof(size_t), 1, f) != 1)
	{
		return NULL;
	}

	BLOOM* const bloom = bloom_create_ex(asize, funcs, nfuncs);

	if (bloom == NULL)
	{
		return NULL;
	}

	const size_t numRead = fread(bloom->a, sizeof(char), bloom->size, f);

	if (numRead != bloom->size)
	{
		bloom_destroy(bloom);
		return NULL;
	}
	return bloom;
}

void bloom_clear(BLOOM* const bloom)
{
	memset(bloom->a, 0x00, bloom->size);

	// depending on the bloom filters size, reallocating the
	// filter's memory with calloc might be faster.
}

void bloom_destroy(BLOOM* const bloom)
{
	assert(bloom != NULL);

	free(bloom->a);
	free(bloom->funcs);
	free(bloom);
}

void bloom_add(BLOOM* const bloom, const char* s, const size_t len)
{
	assert(bloom != NULL);

	for(size_t n = 0; n < bloom->nfuncs; ++n)
	{
		unsigned int h = bloom->funcs[n](s, len) % bloom->bitsize;
		SETBIT(bloom->a, h);
	}
}

const int bloom_check(BLOOM* const bloom, const char* s, const size_t len)
{
	assert(bloom != NULL);

	for(size_t n = 0; n < bloom->nfuncs; ++n)
	{
		hash_t h = bloom->funcs[n](s, len) % bloom->bitsize;
		if (!GETBIT(bloom->a, h))
		{
			return 0;
		}
	}

	return 1;
}

/**
 * GCC: int __builtin_popcount (unsigned int x);
 * VC:  unsigned int __popcnt(unsigned int value);
 *
 */
const size_t bloom_count(BLOOM* const bloom)
{
	assert(bloom != NULL);
	size_t count = 0;

	for (size_t i = 0; i < bloom->size; i += sizeof(unsigned int))
	{
		unsigned int* foo = (unsigned int*) (bloom->a +i);
		count += __builtin_popcount(*foo);
	}
	return count;
}

const int bloom_compare(BLOOM* const a, BLOOM* const b)
{
	if (a == b)
	{
		return 0;
	}

	if (a == NULL || b == NULL)
	{
		return a -b;
	}

	if (a->bitsize != b->bitsize)
	{
		return a->bitsize -b->bitsize;
	}

	if (a->size != b->size)
	{
		return a->size -b->size;
	}

	return memcmp(a->a, b->a, a->size);
}

const int bloom_to_file(BLOOM* const bloom, FILE* const f)
{
	assert(f != NULL);

	if (fwrite(&bloom->bitsize, sizeof(size_t), 1, f) != 1) return -1;
	if (fwrite(bloom->a, sizeof(char), bloom->size, f) != bloom->size) return -1;

	fflush(f);
	return bloom->size *sizeof(char) +sizeof(size_t);
}

const int bloom_to_file_2(BLOOM* const bloom, FILE* const f)
{
	assert(f != NULL);

	int ok = (fwrite(&bloom->bitsize, sizeof(size_t), 1, f) == 1);
	ok = (ok && fwrite(bloom->a, sizeof(char), bloom->size, f) == bloom->size);
	fflush(f);

	return (ok ? bloom->size *sizeof(char) +sizeof(size_t) : -1);
}
