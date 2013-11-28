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

#include "common.h"
#include "../anagram.h"

#include <string.h>
#include <limits.h>


CTEST_DATA(salad)
{
	bloom_t x;
};

CTEST_SETUP(salad)
{
	data->x.bloom = bloom_init(8, HASHES_SIMPLE);
	data->x.useWGrams = (strlen(DELIMITER) != 0);
	to_delim(data->x.delim, DELIMITER);
	data->x.ngramLength = NGRAM_LENGTH;
}

CTEST_TEARDOWN(salad)
{
	bloom_destroy(data->x.bloom);
}

CTEST2(salad, bloom_init)
{
	ASSERT_NOT_NULL(data->x.bloom);
	ASSERT_EQUAL(NGRAM_LENGTH, data->x.ngramLength);
	ASSERT_EQUAL(data->x.useWGrams, (strlen(DELIMITER) != 0));
	DELIM(delim);
	to_delimiter_array(delim, DELIMITER);
	ASSERT_DATA(delim, DELIM_SIZE, data->x.delim, DELIM_SIZE);
}

CTEST(salad, bloom_diff)
{
	bloom_t x = { NULL, FALSE, {0}, 0 };
	bloom_t y = { NULL, FALSE, {0}, 0 };

	ASSERT_FALSE(bloom_t_diff(&x, &y));

	x.useWGrams = TRUE;
	ASSERT_TRUE(bloom_t_diff(&x, &y));
	y.useWGrams = TRUE;
	ASSERT_FALSE(bloom_t_diff(&x, &y));

	x.delim[0] = 1;
	ASSERT_TRUE(bloom_t_diff(&x, &y));
	y.delim[0] = 1;
	ASSERT_FALSE(bloom_t_diff(&x, &y));

	x.ngramLength = 3;
	ASSERT_TRUE(bloom_t_diff(&x, &y));
	y.ngramLength = 3;
	ASSERT_FALSE(bloom_t_diff(&x, &y));
}

CTEST2(salad, bloomize_ex)
{
	static const char* bf =
		"\x32\x0b\x9c\x28\x46\x77\x6a\xbd"
		"\xe0\x02\x60\x82\xd9\x0c\x59\x9c"
		"\x26\x96\x81\x18\x73\x54\xfc\xc4"
		"\xbf\x20\x19\x47\x05\x00\xa8\x0a";

	ASSERT_EQUAL(0, bloom_count(data->x.bloom));
	bloomize_ex(data->x.bloom, TEST_STR, strlen(TEST_STR), data->x.ngramLength);
	ASSERT_DATA((unsigned char*) bf, 32, data->x.bloom->a, data->x.bloom->size);
}

CTEST2(salad, bloomizew_ex)
{
	static const char* bf =
		"\x00\x20\x02\x80\x88\x01\x00\x00"
		"\x60\x00\x00\x02\x80\x80\x00\x00"
		"\x40\x20\x00\x01\x00\x04\x02\x00"
		"\x20\x00\x20\x80\x00\x00\x00\x00";

	DELIM(zero) = {0};
	ASSERT_DATA(zero, DELIM_SIZE, data->x.delim, DELIM_SIZE);
	ASSERT_EQUAL(0, bloom_count(data->x.bloom));

	to_delimiter_array(data->x.delim, TOKEN_DELIMITER);
	bloomizew_ex(data->x.bloom, TEST_STR, strlen(TEST_STR), data->x.ngramLength, data->x.delim);
	ASSERT_DATA((unsigned char*) bf, 32, data->x.bloom->a, data->x.bloom->size);
}

CTEST(salad, fileformat_consistency)
{
	char* TEST_FILE = "test.ana";

	bloom_t x = {
		bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE),
		(strlen(DELIMITER) != 0), {0}, 3
	};

	bloomize_ex(x.bloom, TEST_STR, strlen(TEST_STR), x.ngramLength);

	FILE* const fOut = fopen(TEST_FILE, "w+");
	if (fOut == NULL)
	{
		bloom_destroy(x.bloom);
		CTEST_ERR("Unable to open test file '%s'.", TEST_FILE);
		ASSERT_NOT_NULL(fOut);
	}

	const int n = fwrite_model(fOut, x.bloom, x.ngramLength, DELIMITER);
	fclose(fOut);

	if (n < 0)
	{
		bloom_destroy(x.bloom);
		CTEST_ERR("Failed while writing the model.");
		ASSERT_TRUE(n >= 0);
	}

	FILE* const fIn = fopen(TEST_FILE, "r");
	if (fIn == NULL)
	{
		bloom_destroy(x.bloom);
		CTEST_ERR("Failed to open file.");
		ASSERT_NOT_NULL(fIn);
	}

	bloom_t y = { NULL, FALSE, {0}, 0 };

	const int ret = fread_model(fIn, &y.bloom, &y.ngramLength, y.delim, &y.useWGrams);
	fclose(fIn);

	if (ret != 0)
	{
		bloom_destroy(x.bloom);
		CTEST_ERR("Failed while reading the model.");
		ASSERT_EQUAL(0, ret);
	}

	ASSERT_TRUE(!bloom_t_diff(&x, &y));
	ASSERT_TRUE(bloom_compare(x.bloom, y.bloom) == 0);

	bloom_destroy(x.bloom);
	bloom_destroy(y.bloom);
}

// Test salad's modes (train, predict, inspect, ...)
