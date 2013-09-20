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

#include "salad.h"

#include "anagram.h"

#include <string.h>
#include <assert.h>


const int salad_dbg(const config_t* const c)
{
	fprintf(stdout, "[*] Test the binary format's consistency...\n");

	char* TEST_FILE = "test.ana";

	char* TEST_STR = "test";
	size_t NGRAM_LENGTH = 3;
	char* DELIMITER = "";

	BLOOM* const bloom = bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE);
	if (bloom == NULL)
	{
		fprintf(stderr, "[!] Unable to initialize bloom filter.\n");
		return EXIT_FAILURE;
	}
	bloomize_ex(bloom, TEST_STR, strlen(TEST_STR), NGRAM_LENGTH);

	FILE* const fOut = fopen(TEST_FILE, "w+");
	if (fOut == NULL)
	{
		fprintf(stderr, "[!] Unable to option test file '%s'.\n", TEST_FILE);
		return EXIT_FAILURE;
	}

	const int n = fwrite_model(fOut, bloom, NGRAM_LENGTH, DELIMITER);
	fclose(fOut);

	if (n < 0)
	{
		fprintf(stderr, "[!] Failed while writing the model.\n");
		return EXIT_FAILURE;
	}

	FILE* const fIn = fopen(TEST_FILE, "r");
	if (fIn == NULL)
	{
		return EXIT_FAILURE;
	}

	BLOOM* bloomFromFile = NULL;

	uint8_t delim[256] = {0};
	int useWGrams = 0;
	size_t ngramLength = 0;

	const int ret = fread_model(fIn, &bloomFromFile, &ngramLength, delim, &useWGrams);
	fclose(fIn);

	if (ret != 0)
	{
		fprintf(stderr, "[!] Failed while reading the model.\n");
		return EXIT_FAILURE;
	}

	assert(useWGrams == (strlen(DELIMITER) != 0));
	assert(ngramLength == NGRAM_LENGTH);
	assert(bloom_compare(bloom, bloomFromFile) == 0);

	fprintf(stdout, "[*] Everything's fine :)\n");
	return EXIT_SUCCESS;
}
