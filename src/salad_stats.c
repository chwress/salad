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


const int salad_stats(const config_t* const c)
{
	FILE* const fFilter = fopen(c->bloom, "r");
	if (fFilter == NULL)
	{
		fprintf(stderr, "[!] Unable to open bloom filter\n");
		return EXIT_FAILURE;
	}

	uint8_t delim[256] = {0};
	int useWGrams = 0;

	size_t ngramLength = 0;
	BLOOM* bloom = NULL;

	int ret = fread_model(fFilter, &bloom, &ngramLength, delim, &useWGrams);
	fclose(fFilter);
	if (ret != 0)
	{
		fprintf(stderr, "[!] Corrupt bloom filter file.\n");
		return EXIT_FAILURE;
	}

	const size_t set = bloom_count(bloom);
	fprintf(stdout, "[*] Saturation: %.3f%%\n", (((double)set)/ ((double)bloom->bitsize))*100);
	return EXIT_SUCCESS;
}
