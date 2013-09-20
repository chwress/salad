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
#include <string.h>
#include <math.h>


typedef void (*FN_BLOOMIZE)(BLOOM* const, BLOOM* const, const char* const, const size_t, const size_t, const uint8_t* const, bloomize_stats_t* const);

void bloomize_ex3_wrapper(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const uint8_t* const delim, bloomize_stats_t* const out)
{
	bloomize_ex3(bloom1, bloom2, str, len, n, out);
}

void bloomize_ex4_wrapper(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const uint8_t* const delim, bloomize_stats_t* const out)
{
	bloomize_ex4(bloom1, bloom2, str, len, n, out);
}


const int salad_inspect_stub(const config_t* const c, const data_processor_t* const dp, file_t fIn, FILE* const fOut)
{
	BLOOM* bloom = (c->bloom != NULL ? bloom_from_file("training", c->bloom, NULL, NULL, NULL) : NULL);

	const int newBloomFilter = (bloom == NULL);
	if (newBloomFilter)
	{
		bloom = bloom_init(c->filter_size, c->hash_set);
	}

	BLOOM* const cur = bloom_init(c->filter_size, c->hash_set);

	uint8_t delim[256] = {0};
	const char* const delimiter = to_delim(delim, c->delimiter);

	FN_BLOOMIZE bloomize = (newBloomFilter ?
			(delimiter == NULL ? bloomize_ex3_wrapper : bloomizew_ex3) :
			(delimiter == NULL ? bloomize_ex4_wrapper : bloomizew_ex4));

	char buf[0x1000];

	static const size_t BATCH_SIZE = 1000;
	data_t data[BATCH_SIZE];
	bloomize_stats_t stats[BATCH_SIZE];

	size_t numRead = 0;
	do
	{
		numRead = dp->read(&fIn, data, BATCH_SIZE);
		for (size_t i = 0; i < numRead; i++)
		{
			bloomize(bloom, cur, data[i].buf, data[i].len, c->ngramLength, delim, &stats[i]);
			free(data[i].buf);
		}

		for (size_t i = 0; i < numRead; i++)
		{
#ifdef Z
			snprintf(buf, 0x1000, "%10zu\t%10zu\t%10zu%10zu\n", stats[i].new, stats[i].uniq, stats[i].total, data[i].len);
#else
			snprintf(buf, 0x1000, "%10lu\t%10lu\t%10lu\t%10lu\n",
					(unsigned long) stats[i].new,
					(unsigned long) stats[i].uniq,
					(unsigned long) stats[i].total,
					(unsigned long) data[i].len);
#endif
			fwrite(buf, sizeof(char), strlen(buf), fOut);
		}

		fprintf(stdout, ".");
		fflush(stdout);
	} while (numRead >= BATCH_SIZE);

	fprintf(stdout, "\n");

	const size_t set = bloom_count(bloom);
	fprintf(stdout, "[*] Saturation: %.3f%%\n", (((double)set)/ ((double)bloom->bitsize))*100);

	return EXIT_SUCCESS;
}


const int salad_inspect(const config_t* const c)
{
	return salad_heart(c, salad_inspect_stub);
}
