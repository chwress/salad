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
#include <util/getline.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const char* const CONFIG_HEADER = "Salad Configuration";

const int fwrite_model(FILE* const f, BLOOM* const bloom, const size_t ngram_length, const char* const delimiter, const int as_binary)
{
	const int nheader = fprintf(f, "%s\n\n", CONFIG_HEADER);
	if (nheader < 0) return -1;

	const int nbinary = fprintf(f, " binary = %s\n", (as_binary ? "True" : "False"));
	if (nbinary < 0) return -1;

	const int ndelim = fprintf(f, "delimiter =  %s\n", (delimiter == NULL ? "" : delimiter));
	if (ndelim < 0) return -1;

	const int n = fprintf(f, "n = %"Z"\n", ngram_length);
	if (n < 0) return -1;

	size_t data_size = 0;

	int nbloom = fprintf(f, "bloom_filter = %"Z"\n", data_size);
	if (nbloom < 0) return -1;

	int m, l;
	if ((m = fwrite_hashspec(f, bloom)) < 0) return -1;
	if ((l = bloom_to_file(bloom, f)) < 0) return -1;

	return nheader + nbinary + ndelim + n + m + l;
}

char* const readline(char** buf, size_t* len, FILE* f)
{
	while (1)
	{
		const ssize_t read = getline(buf, len, f);
		if (read < 0)
		{
			return NULL;
		}

		char* x = ltrim(*buf);
		if (*x == '#') continue; // ignore comments

		rtrim(x);
		if (*x == 0x00) continue; // ignore empty lines

		return x;
	}
}


const int fread_model_txt(FILE* const f, BLOOM** const bloom, size_t* const ngram_length, delimiter_array_t delim, int* const use_wgrams, int* const as_binary)
{
	assert(f != NULL && bloom != NULL);
	long int pos = ftell(f);

	char* line = NULL;
	size_t len = 0;

	char* x = readline(&line, &len, f);
	if (x == NULL || strcasecmp(x, CONFIG_HEADER) != 0)
	{
		free(line);
		fseek(f, pos, SEEK_SET);
		return 0;
	}

	// Set default values
	if (as_binary != NULL) *as_binary = FALSE;
	if (use_wgrams != NULL) *use_wgrams = FALSE;
	if (delim != NULL) to_delimiter_array("", delim);

	// The bloom filter and the n-gram length are mandatory, though
	int ngramlen_specified = FALSE;
	int bloomfilter_specified = FALSE;

	while (1)
	{
		char* x = readline(&line, &len, f);
		if (x == NULL)
		{
			break;
		}

		char* y = strchr(x, '=');
		if (y == NULL)
		{
			free(line);
			return -1;
		}

		y[0] = 0x00;
		char* value = ltrim(y +1);
		rtrim(x);

		char* tail;
		switch (cmp(x, "binary", "delimiter", "n", "bloom_filter", NULL))
		{
		case 0:
		{
			long int b = strtol(value, &tail, 10);
			if (tail == value)
			{
				b = (strcasecmp(value, "True") == 0);
			}
			if (as_binary != NULL) *as_binary = b;
			break;
		}
		case 1:
		{
			if (value[0] != 0x00)
			{
				if (use_wgrams != NULL) *use_wgrams = TRUE;
				if (delim != NULL) to_delimiter_array(value, delim);
			}
			break;
		}
		case 2:
		{
			unsigned long int n = strtoul(value, &tail, 10);
			if (ngram_length != NULL) *ngram_length = n;
			ngramlen_specified = TRUE;
			break;
		}
		case 3:
		{
			unsigned long int len = strtoul(value, &tail, 10);
			UNUSED(len); // we simply ignore the size for now
			*bloom = bloom_init_from_file(f);
			bloomfilter_specified = TRUE;
			break;
		}
		default:
			// Unkown identifier
			free(line);
			return -2;
		}
	}
	free(line);

	if (!ngramlen_specified) return -3;
	if (!bloomfilter_specified) return -4;

	return ftell(f) -pos;
}


const int fread_model_032(FILE* const f, BLOOM** const bloom, size_t* const ngram_length, delimiter_array_t delim, int* const use_wgrams)
{
	assert(f != NULL && bloom != NULL);
	size_t pos = ftell(f);

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
	size_t n = fread(_ngram_length, sizeof(size_t), 1, f);

	// The actual bloom filter values
	*bloom = bloom_init_from_file(f);

	return (n <= 0 || *_ngram_length <= 0 || *bloom == NULL ? -1 : (ftell(f) -pos));
}


const int fread_model(FILE* const f, BLOOM** const bloom, size_t* const ngram_length, delimiter_array_t delim, int* const use_wgrams, int* const as_binary)
{
	int ret = fread_model_txt(f, bloom, ngram_length, delim, use_wgrams, as_binary);
	if (ret == 0) // seems to be in old format
	{
		if (as_binary != NULL) *as_binary = FALSE;
		ret = fread_model_032(f, bloom, ngram_length, delim, use_wgrams);
	}
	return ret;
}
