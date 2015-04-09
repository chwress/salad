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

#include "io.h"

#include <stdlib.h>
#include <assert.h>

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
