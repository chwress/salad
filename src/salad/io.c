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

const int fwrite_model(FILE* const f, BLOOM* const bloom, const size_t ngram_length, const char* const delimiter, const int as_binary)
{
	const char* const magic = "SALAD-F2";
	if (fwrite(magic, sizeof(char), strlen(magic), f) != strlen(magic)) return -1;

	const uint8_t b = (as_binary ? 1 : 0);
	if (fwrite(&b, sizeof(uint8_t), 1, f) != 1) return -1;

	const char* const x = (delimiter == NULL ? "" : delimiter);

	size_t n = strlen(x);
	int m, l;
	if (fwrite(x, sizeof(char), n +1, f) != n +1) return -1;

	if (fwrite(&ngram_length, sizeof(size_t), 1, f) != 1) return -1;

	if ((m = fwrite_hashspec(f, bloom)) < 0) return -1;

	if ((l = bloom_to_file(bloom, f)) < 0) return -1;

	return sizeof(uint8_t) + (n+1)*sizeof(char) +sizeof(size_t) +m +l;
}

const int fread_model(FILE* const f, BLOOM** const bloom, size_t* const ngram_length, delimiter_array_t delim, int* const use_wgrams, int* const as_binary)
{
	assert(f != NULL && bloom != NULL);
	long int pos = ftell(f);

	static const size_t len_magic = 8;
	char magic[len_magic +1];

	size_t n = fread(magic, sizeof(char), len_magic, f);
	magic[len_magic] = 0x00;
	UNUSED(n);

	size_t n1 = 1;
	if (starts_with(magic, "SALAD-F"))
	{
		char* tail;
		const unsigned long version = strtoul(magic +7, &tail, 10);
		UNUSED(version);

		uint8_t cdummy;
		n1 = fread(&cdummy, sizeof(uint8_t), 1, f);
		if (as_binary != NULL) *as_binary = cdummy;
	}
	else
	{
		fseek(f, pos, SEEK_SET);
	}

	if (use_wgrams != NULL) *use_wgrams = FALSE;

	char* const delimiter = fread_str(f);
	if (delimiter != NULL)
	{
		if (delimiter[0] != 0x00)
		{
			if (use_wgrams != NULL) *use_wgrams = TRUE;
			if (delim != NULL) to_delimiter_array(delimiter, delim);
		}
		free(delimiter);
	}

	// n-gram length
	size_t dummy;
	size_t* const _ngram_length = (ngram_length == NULL ? &dummy : ngram_length);

	*_ngram_length = 0;
	size_t n2 = fread(_ngram_length, sizeof(size_t), 1, f);

	// The actual bloom filter values
	*bloom = bloom_init_from_file(f);

	return (n1 <= 0 || n2 <= 0 || *_ngram_length <= 0 || *bloom == NULL ? -1 : 0);
}
