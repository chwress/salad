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


#include "bloom.h"
#include "io.h"

#include <util/util.h>

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
