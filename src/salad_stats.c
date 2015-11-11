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

#include "main.h"

#include <salad/salad.h>
#include <salad/io.h>
#include <util/log.h>

const int _salad_stats_(const config_t* const c)
{
	FILE* const f_filter = fopen(c->bloom, "rb");
	if (f_filter == NULL)
	{
		error("Unable to open bloom filter.");
		return EXIT_FAILURE;
	}

	SALAD_T(s);
	int ret = salad_from_file_ex(f_filter, &s);
	fclose(f_filter);
	if (ret != EXIT_SUCCESS)
	{
		error("Corrupt bloom filter file.");
		return EXIT_FAILURE;
	}

	BLOOM* bloom = GET_BLOOMFILTER(s.model);
	const size_t set = bloom_count(bloom);
	status("Saturation: %.3f%%", (((double)set)/ ((double)bloom->bitsize))*100);

	salad_destroy(&s);
	return EXIT_SUCCESS;
}
