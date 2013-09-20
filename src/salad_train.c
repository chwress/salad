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

#include <inttypes.h>


const int salad_train_stub(const config_t* const c, const data_processor_t* const dp, file_t fIn, FILE* const fOut)
{
	BLOOM* const bloom = bloom_init(c->filter_size, c->hash_set);
	uint8_t delim[256] = {0};
	const char* const delimiter = to_delim(delim, c->delimiter);


	static const size_t BATCH_SIZE = 1000;
	data_t data[BATCH_SIZE];

	size_t numRead = 0;
	do
	{
		numRead = dp->read(&fIn, data, BATCH_SIZE);
		if (delimiter == NULL)
		{
			for (size_t i = 0; i < numRead; i++)
			{
				bloomize_ex(bloom, data[i].buf, data[i].len, c->ngramLength);
				free(data[i].buf);
			}
		}
		else
		{
			for (size_t i = 0; i < numRead; i++)
			{
				bloomizew_ex(bloom, data[i].buf, data[i].len, c->ngramLength, delim);
				free(data[i].buf);
			}
		}

		fprintf(stdout, ".");
		fflush(stdout);
	} while (numRead >= BATCH_SIZE);

	fprintf(stdout, "\n");
	const int n = fwrite_model(fOut, bloom, c->ngramLength, delimiter);
	return (n >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

const int salad_train(const config_t* const c)
{
	return salad_heart(c, salad_train_stub);
}

