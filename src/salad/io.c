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

#include <container/io.h>
#include <util/util.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
