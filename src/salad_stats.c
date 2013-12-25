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
#include "util/log.h"

const int salad_stats(const config_t* const c)
{
	FILE* const fFilter = fopen(c->bloom, "rb");
	if (fFilter == NULL)
	{
		error("Unable to open bloom filter.");
		return EXIT_FAILURE;
	}

	DELIM(delim) = {0};
	int useWGrams = 0;

	size_t ngramLength = 0;
	BLOOM* bloom = NULL;

	int ret = fread_model(fFilter, &bloom, &ngramLength, delim, &useWGrams);
	fclose(fFilter);
	if (ret != 0)
	{
		error("Corrupt bloom filter file.");
		return EXIT_FAILURE;
	}

	const size_t set = bloom_count(bloom);
	status("Saturation: %.3f%%", (((double)set)/ ((double)bloom->bitsize))*100);
	return EXIT_SUCCESS;
}
