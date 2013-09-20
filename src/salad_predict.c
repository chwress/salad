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

#include <sys/time.h>
#include <math.h>
#include <string.h>


#define TO_SEC(t) ((t).tv_sec+((t).tv_usec/1000000.0))


typedef const double (*FN_ANACHECK)(BLOOM* const, BLOOM* const, const char* const, const size_t, const size_t, const uint8_t* const);

const double anacheck_ex_wrapper(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const uint8_t* const delim)
{
	return anacheck_ex(bloom, input, len, n);
}

const double anacheck_ex2_wrapper(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const uint8_t* const delim)
{
	return anacheck_ex2(bloom, bbloom, input, len, n);
}

const double anacheckw_ex_wrapper(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const uint8_t* const delim)
{
	return anacheckw_ex(bloom, input, len, n, delim);
}


const int salad_predict_stub(const config_t* const c, const data_processor_t* const dp, file_t fIn, FILE* const fOut)
{
	uint8_t delim[256] = {0}, delim_b[256] = {0};
	int useWGrams = 0, useWGrams_b;
	size_t ngramLength = 0, ngramLength_b;

	BLOOM* bloom = bloom_from_file("training", c->bloom, delim, &useWGrams, &ngramLength);
	if (bloom == NULL) return EXIT_FAILURE;

	BLOOM* bbloom = NULL;
	if (c->bbloom)
	{
		bbloom = bloom_from_file("bad content", c->bbloom, delim_b, &useWGrams_b, &ngramLength_b);
		if (bbloom == NULL) return EXIT_FAILURE;

		if (memcmp(delim, delim_b, 256) != 0 || useWGrams != useWGrams_b || ngramLength != ngramLength_b)
		{
			fprintf(stderr, "[!] The normal and the bad content filter do were not generated with the same parameters.\n");
			return EXIT_FAILURE;
		}
	}


	char nan[0x100];
	snprintf(nan, 0x100, "%s\n", c->nan);

	FN_ANACHECK anacheck = (bbloom == NULL ?
			(useWGrams ? anacheckw_ex_wrapper : anacheck_ex_wrapper) :
			(useWGrams ? anacheckw_ex2        : anacheck_ex2_wrapper));


	static const size_t BATCH_SIZE = 1000;
	data_t data[1000];
	float scores[1000];

	double totalTime = 0;
	size_t numRead = 0;
	do
	{
		numRead = dp->read(&fIn, data, BATCH_SIZE);
		struct timeval start, end;
		gettimeofday(&start, NULL);

		for (size_t i = 0; i < numRead; i++)
		{
			scores[i] = anacheck(bloom, bbloom, data[i].buf, data[i].len, ngramLength, delim);
			free(data[i].buf);
		}

		// Clock the calculation procedure
		gettimeofday(&end, NULL);
		double diff = TO_SEC(end) -TO_SEC(start);
		fprintf(stdout, ".");
		fflush(stdout);
		totalTime += diff;

		// Write scores
		char buf[0x100];
		for (size_t j = 0; j < numRead;  j++)
		{
			if (isnan(scores[j]))
			{
				fputs(nan, fOut);
			}
			else
			{
				snprintf(buf, 0x100, "%f\n", 1.0 -scores[j]);
				fputs(buf, fOut);
			}
		}
	} while (numRead >= BATCH_SIZE);

	bloom_destroy(bloom);

	fprintf(stdout, "\n");
	fprintf(stderr, "[I] Net Calculation Time: %.4f seconds\n", totalTime);
	return EXIT_SUCCESS;
}

const int salad_predict(const config_t* const c)
{
	return salad_heart(c, salad_predict_stub);
}
