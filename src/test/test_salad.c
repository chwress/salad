/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2014, Christian Wressnegger
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
#include <salad/salad.h>
#include <salad/anagram.h>

#include <string.h>
#include <limits.h>

#include <config.h>
#include <salad/util.h>


CTEST_DATA(salad)
{
	salad_t x;
};

CTEST_SETUP(salad)
{
	salad_set_bloomfilter_ex(&data->x, bloom_init(8, HASHES_SIMPLE));
	salad_set_delimiter(&data->x, DELIMITER);
	salad_set_ngramlength(&data->x, NGRAM_LENGTH);
}

CTEST_TEARDOWN(salad)
{
	salad_destroy(&data->x);
}

CTEST2(salad, bloom_init)
{
	ASSERT_NOT_NULL(data->x.model.x);
	ASSERT_EQUAL(NGRAM_LENGTH, data->x.ngramLength);
	ASSERT_EQUAL(data->x.useWGrams, (strlen(DELIMITER) != 0));
	DELIM(delim);
	to_delimiter_array(DELIMITER, delim);
	ASSERT_DATA(delim, DELIM_SIZE, data->x.delimiter.d, DELIM_SIZE);
}

CTEST(salad, bloom_diff)
{
	SALAD_T(x);
	SALAD_T(y);

	ASSERT_FALSE(salad_spec_diff(&x, &y));

	x.useWGrams = TRUE;
	ASSERT_TRUE(salad_spec_diff(&x, &y));
	y.useWGrams = TRUE;
	ASSERT_FALSE(salad_spec_diff(&x, &y));

	x.delimiter.d[0] = 1;
	ASSERT_TRUE(salad_spec_diff(&x, &y));
	y.delimiter.d[0] = 1;
	ASSERT_FALSE(salad_spec_diff(&x, &y));

	x.ngramLength = 3;
	ASSERT_TRUE(salad_spec_diff(&x, &y));
	y.ngramLength = 3;
	ASSERT_FALSE(salad_spec_diff(&x, &y));
}

CTEST2(salad, bloomize_ex)
{
	static const char* bf =
		"\x32\x0b\x9c\x28\x46\x77\x6a\xbd"
		"\xe0\x02\x60\x82\xd9\x0c\x59\x9c"
		"\x26\x96\x81\x18\x73\x54\xfc\xc4"
		"\xbf\x20\x19\x47\x05\x00\xa8\x0a";

	BLOOM* bloom = GET_BLOOMFILTER(data->x.model);

	ASSERT_EQUAL(0, bloom_count(bloom));
	bloomize_ex(bloom, TEST_STR, strlen(TEST_STR), data->x.ngramLength);
	ASSERT_DATA((unsigned char*) bf, 32, bloom->a, bloom->size);
}

CTEST2(salad, bloomizew_ex)
{
	static const char* bf =
		"\x00\x20\x02\x80\x88\x01\x00\x00"
		"\x60\x00\x00\x02\x80\x80\x00\x00"
		"\x40\x20\x00\x01\x00\x04\x02\x00"
		"\x20\x00\x20\x80\x00\x00\x00\x00";

	BLOOM* bloom = GET_BLOOMFILTER(data->x.model);

	DELIM(zero) = {0};
	ASSERT_DATA(zero, DELIM_SIZE, data->x.delimiter.d, DELIM_SIZE);
	ASSERT_EQUAL(0, bloom_count(bloom));

	to_delimiter_array(TOKEN_DELIMITER, data->x.delimiter.d);
	bloomizew_ex(bloom, TEST_STR, strlen(TEST_STR), data->x.ngramLength, data->x.delimiter.d);
	ASSERT_DATA((unsigned char*) bf, 32, bloom->a, bloom->size);
}

CTEST(salad, fileformat_consistency)
{
	char* TEST_FILE = "test.ana";

	SALAD_T(x);
	salad_set_bloomfilter_ex(&x, bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE));
	salad_set_delimiter(&x, DELIMITER);
	salad_set_ngramlength(&x, 3);

	BLOOM* const xbloom = GET_BLOOMFILTER(x.model);
	bloomize_ex(xbloom, TEST_STR, strlen(TEST_STR), x.ngramLength);

	FILE* const fOut = fopen(TEST_FILE, "wb+");
	if (fOut == NULL)
	{
		salad_destroy(&x);
		CTEST_ERR("Unable to open test file '%s'.", TEST_FILE);
		ASSERT_NOT_NULL(fOut);
	}

	const int n = fwrite_model(fOut, xbloom, x.ngramLength, DELIMITER, 0);
	fclose(fOut);

	if (n < 0)
	{
		salad_destroy(&x);
		CTEST_ERR("Failed while writing the model.");
		ASSERT_TRUE(n >= 0);
	}

	FILE* const fIn = fopen(TEST_FILE, "rb");
	if (fIn == NULL)
	{
		salad_destroy(&x);
		CTEST_ERR("Failed to open file.");
		ASSERT_NOT_NULL(fIn);
	}

	SALAD_T(y);
	const int ret = salad_from_file_ex(fIn, &y);
	fclose(fIn);

	BLOOM* const ybloom = GET_BLOOMFILTER(y.model);

	if (ret != 0)
	{
		salad_destroy(&x);
		CTEST_ERR("Failed while reading the model.");
		ASSERT_EQUAL(0, ret);
	}

	ASSERT_TRUE(!salad_spec_diff(&x, &y));
	ASSERT_TRUE(bloom_compare(xbloom, ybloom) == 0);

	salad_destroy(&x);
	salad_destroy(&y);
}

// Test salad's modes (train, predict, inspect, ...)
