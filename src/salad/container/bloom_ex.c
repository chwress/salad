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

#include "bloom_ex.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <util/util.h>

static const size_t CHAR_HIGHBIT = (((char) 1) << (sizeof(unsigned char) * 8 -1));

#define SETBIT(a, n)  ((a)[(n)/CHAR_BIT] |= ((unsigned char) (CHAR_HIGHBIT>>((n)%CHAR_BIT))))
#define GETBIT(a, n)  ((a)[(n)/CHAR_BIT] &  ((unsigned char) (CHAR_HIGHBIT>>((n)%CHAR_BIT))))

BLOOM* const bloom_create(const size_t bitsize)
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

	bloom->funcs = (hashfunc_t*) calloc(1, sizeof(hashfunc_t));
	bloom->nfuncs = 0;
	bloom->bitsize = bitsize;
	bloom->size = size;

	return bloom;
}

const int bloom_set_hashfuncs(BLOOM* const bloom, const uint8_t nfuncs, ...)
{
	va_list args;
	va_start(args, nfuncs);

	const int ret = vbloom_set_hashfuncs(bloom, nfuncs, args);

	va_end(args);
	return ret;
}

const int vbloom_set_hashfuncs(BLOOM* const bloom, const uint8_t nfuncs, va_list funcs)
{
	hashfunc_t* buf = (hashfunc_t*) calloc(nfuncs, sizeof(hashfunc_t));
	if (buf == NULL) return EXIT_FAILURE;

	for (uint8_t i = 0; i < nfuncs; i++)
	{
		buf[i] = va_arg(funcs, hashfunc_t);
	}

	const int ret = bloom_set_hashfuncs_ex(bloom, buf, nfuncs);
	free(buf);
	return ret;
}

const int bloom_set_hashfuncs_ex(BLOOM* const bloom, hashfunc_t* const funcs, const uint8_t nfuncs)
{
	bloom->funcs = (hashfunc_t*) realloc(bloom->funcs, nfuncs *sizeof(hashfunc_t));
	if (bloom->funcs == NULL)
	{
		return EXIT_FAILURE;
	}

	bloom->nfuncs = nfuncs;

	for(int i = 0; i < (int) nfuncs; ++i)
	{
		bloom->funcs[i] = funcs[i];
	}
	return EXIT_SUCCESS;
}

typedef const int (*FN_COPYBYTES)(BLOOM* const bloom, const size_t n, void* usr);
const int __bloom_set(BLOOM* const bloom, FN_COPYBYTES cpy, const size_t bitsize, void* usr)
{
	assert(bloom != NULL);

	const size_t size = (bitsize +CHAR_BIT -1)/CHAR_BIT;
	const size_t intsize = (size +sizeof(unsigned int) -1)/ sizeof(unsigned int);

	// We use blocks of integers in order to ease the bit counting, cf. bloom_count(.)
	unsigned char* b = (unsigned char*) calloc(intsize, sizeof(unsigned int));
	if (b == NULL)
	{
		return FALSE;
	}

	free(bloom->a);
	bloom->a = b;

	if (!cpy(bloom, size, usr))
	{
		return FALSE;
	}
	bloom->bitsize = bitsize;
	bloom->size = size;
	return TRUE;
}

static inline const int __memcpy(BLOOM* const bloom, const size_t size, void* buf)
{
	memcpy(bloom->a, buf, size);
	return TRUE;
}

typedef struct {
	FN_READBYTE fct;
	void* usr;
} __fctcpy_t;

static inline const int __fctcpy(BLOOM* const bloom, const size_t size, void* usr)
{
	__fctcpy_t* const x = (__fctcpy_t*) usr;

	for (size_t i = 0; i < size; i++)
	{
		const int ch = x->fct(x->usr);
		if (ch < 0) return FALSE;
		bloom->a[i] = (unsigned char) ch;
	}
	return TRUE;
}

const int bloom_set(BLOOM* const bloom, const uint8_t* const buf, const size_t size)
{
	return __bloom_set(bloom, __memcpy, size, (void*) buf);
}

const int bloom_set_ex(BLOOM* const bloom, FN_READBYTE fct, const size_t size, void* usr)
{
	__fctcpy_t x = {fct, usr};
	return __bloom_set(bloom, __fctcpy, size, &x);
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

void bloom_add_str(BLOOM* const bloom, const char* s, const size_t len)
{
	assert(bloom != NULL);

	for(size_t n = 0; n < bloom->nfuncs; ++n)
	{
		const size_t h = bloom->funcs[n](s, len) % bloom->bitsize;
		SETBIT(bloom->a, h);
	}
}

void bloom_add_num(BLOOM* const bloom, const size_t num)
{
	assert(bloom != NULL);

	for(size_t n = 0; n < bloom->nfuncs; ++n)
	{
		const size_t h = bloom->funcs[n]((char*) &num, sizeof(size_t)) % bloom->bitsize;
		SETBIT(bloom->a, h);
	}
}

static inline int bloom_check(BLOOM* const bloom, const char* s, const size_t len)
{
	assert(bloom != NULL);

	for(size_t n = 0; n < (bloom)->nfuncs; ++n)
	{
		const size_t h = (bloom)->funcs[n](s, len) % (bloom)->bitsize;
		if (!GETBIT((bloom)->a, h))
		{
			return FALSE;
		}
	}
	return TRUE;
}

const int bloom_check_str(BLOOM* const bloom, const char* s, const size_t len)
{
	return bloom_check(bloom, s, len);
}

const int bloom_check_num(BLOOM* const bloom, const size_t num)
{
	return bloom_check(bloom, (const char*) &num, sizeof(size_t));
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
		// __builtin_popcount: Returns the number of 1-bits in x
		count += (size_t) __builtin_popcount(*foo);
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
		return (a < b ? -1 : 1);
	}

	if (a->bitsize != b->bitsize)
	{
		return (a->bitsize < b->bitsize ? -1 : 1);
	}

	if (a->size != b->size)
	{
		return (a->size < b->size ? -1 : 1);
	}

#if 1 // DEBUGGING
	unsigned char *A = a->a, *B = b->a;
	for (size_t i = 0; i < a->size; i++)
	{
		if (*A != *B)
		{
			break;
		}
		A++; B++;
	}
#endif
	return memcmp(a->a, b->a, a->size);
}

void bloom_print(BLOOM* const bloom)
{
	bloom_print_ex(stdout, bloom);
}

void bloom_print_ex(FILE* const f, BLOOM* const bloom)
{
	assert(f != NULL && bloom != NULL);

	fprintf(f, "Size:    \t %"ZU"\n", (SIZE_T) bloom->size);
	fprintf(f, "Bit-Size:\t %"ZU"\n", (SIZE_T) bloom->bitsize);
	fprintf(f, "Data:\n");

	int i = 0, j = 0;
	for (; i < bloom->size; i++)
	{
		if (j++ >= 16)
		{
			fprintf(f, "\n");
			j = 0;
		}
		fprintf(f, "%02X ", bloom->a[i]);
	}

	fprintf(f, "\n");
}

const int bloom_to_file(const BLOOM* const bloom, FILE* const f)
{
	assert(f != NULL);

	if (fwrite(&bloom->bitsize, sizeof(size_t), 1, f) != 1) return -1;
	if (fwrite(bloom->a, sizeof(char), bloom->size, f) != bloom->size) return -1;

	fflush(f);
	return (int) MIN(INT_MAX, bloom->size *sizeof(char) +sizeof(size_t));
}

const int bloom_to_file_2(BLOOM* const bloom, FILE* const f)
{
	assert(f != NULL);

	int ok = (fwrite(&bloom->bitsize, sizeof(size_t), 1, f) == 1);
	ok = (ok && fwrite(bloom->a, sizeof(char), bloom->size, f) == bloom->size);
	fflush(f);

	return (ok ? (int) MIN(INT_MAX, bloom->size *sizeof(char) +sizeof(size_t)) : -1);
}
