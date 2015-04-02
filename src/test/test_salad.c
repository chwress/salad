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

#include "common.h"
#include "ctest.h"

#include <salad/salad.h>
#include <salad/anagram.h>

#include <string.h>
#include <limits.h>

#include <config.h>
#include <salad/util.h>


CTEST_DATA(salad)
{
	salad_t x;
	salad_t b1;
	salad_t b2;
};

CTEST_SETUP(salad)
{
	salad_set_bloomfilter_ex(&data->x, bloom_init(8, HASHES_SIMPLE));
	salad_set_delimiter(&data->x, DELIMITER);
	salad_set_ngramlength(&data->x, NGRAM_LENGTH);

	salad_t* y[] = { &data->b1, &data->b2, NULL };
	for (salad_t** x = y; *x != NULL; x++)
	{
		salad_set_bloomfilter_ex(*x, bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE));
		salad_set_delimiter(*x, DELIMITER);
		salad_set_ngramlength(*x, NGRAM_LENGTH);
	}
}

CTEST_TEARDOWN(salad)
{
	salad_destroy(&data->x);
	salad_destroy(&data->b1);
	salad_destroy(&data->b2);
}


static const int bloom_compare_bytes(BLOOM* const a, BLOOM* const b)
{
	return memcmp_bytes(a->a, b->a, a->size);
}


CTEST2(salad, bloom_init)
{
	ASSERT_NOT_NULL(data->x.model.x);
	ASSERT_EQUAL(NGRAM_LENGTH, data->x.ngram_length);
	ASSERT_EQUAL(data->x.use_tokens, (strlen(DELIMITER) != 0));
	DELIM(delim);
	to_delimiter_array(DELIMITER, delim);
	ASSERT_DATA(delim, DELIM_SIZE, data->x.delimiter.d, DELIM_SIZE);
}

CTEST(salad, spec_diff)
{
	SALAD_T(x);
	SALAD_T(y);

	ASSERT_FALSE(salad_spec_diff(&x, &y));

	x.use_tokens = (DELIMITER[0] == 0);
	ASSERT_TRUE(salad_spec_diff(&x, &y));
	y.use_tokens = x.use_tokens;
	ASSERT_FALSE(salad_spec_diff(&x, &y));

	x.as_binary = TRUE;
	ASSERT_TRUE(salad_spec_diff(&x, &y));
	y.as_binary = x.as_binary;
	ASSERT_FALSE(salad_spec_diff(&x, &y));

	x.delimiter.d[0] = 1;
	ASSERT_TRUE(salad_spec_diff(&x, &y));
	y.delimiter.d[0] = 1;
	ASSERT_FALSE(salad_spec_diff(&x, &y));

	x.ngram_length = NGRAM_LENGTH +1;
	ASSERT_TRUE(salad_spec_diff(&x, &y));
	y.ngram_length = x.ngram_length;
	ASSERT_FALSE(salad_spec_diff(&x, &y));
}

#define TEST_BLOOMIZE_EX(X, bf)                                                                      \
{	                                                                                                 \
	BLOOM* const x = GET_BLOOMFILTER(data->x.model);                                                 \
	                                                                                                 \
	ASSERT_EQUAL(0, bloom_count(x));                                                                 \
	BLOOMIZE_EX(#X[0], x, TEST_STR1, strlen(TEST_STR1), data->x.ngram_length, data->x.delimiter.d);  \
	ASSERT_DATA((unsigned char*) (bf), 32, x->a, x->size);                                           \
}

typedef struct {
	size_t uniq;
	size_t total;
} count_exp_t;

void TEST_BLOOMIZE_COUNT_EX(const char X, salad_t* const s1, const count_exp_t exp1, salad_t* const s2, const count_exp_t exp2, const size_t new, const size_t n)
{
	BLOOM* const b1 = GET_BLOOMFILTER(s1->model);
	BLOOM* const b2 = GET_BLOOMFILTER(s2->model);

	bloomize_stats_t stats;
	// Adds TEST_STR1 to b1
	BLOOMIZE_EX(X, b1, TEST_STR1, strlen(TEST_STR1), n, s1->delimiter.d);

	// Clears b2, adds TEST_STR1 to both
	BLOOMIZE_EX3(X, b1, b2, TEST_STR1, strlen(TEST_STR1), n, s1->delimiter.d, &stats);
	ASSERT_EQUAL(0, bloom_compare(b1, b2));
	ASSERT_EQUAL(0, stats.new);
	ASSERT_EQUAL(exp1.uniq, stats.uniq);
	ASSERT_EQUAL(exp1.total, stats.total);

	// Clears b2, adds TEST_STR2 to b2 only
	BLOOMIZE_EX4(X, b1, b2, TEST_STR2, strlen(TEST_STR2), n, s1->delimiter.d, &stats);
	ASSERT_NOT_EQUAL(0, bloom_compare(b1, b2));
	ASSERT_EQUAL(new, stats.new);
	ASSERT_EQUAL(exp2.uniq, stats.uniq);
	ASSERT_EQUAL(exp2.total, stats.total);

	// Clears b2, adds TEST_STR2 to both
	BLOOMIZE_EX3(X, b1, b2, TEST_STR2, strlen(TEST_STR2), n, s1->delimiter.d, &stats);
	ASSERT_EQUAL((X == 'w' ? 4 : 0), bloom_compare_bytes(b1, b2));
	ASSERT_EQUAL(new, stats.new);
	ASSERT_EQUAL(exp2.uniq, stats.uniq);
	ASSERT_EQUAL(exp2.total, stats.total);

	// Clears b2, adds TEST_STR2 to b2 only
	BLOOMIZE_EX4(X, b1, b2, TEST_STR2, strlen(TEST_STR2), n, s1->delimiter.d, &stats);
	ASSERT_EQUAL((X == 'w' ? 4 : 0), bloom_compare_bytes(b1, b2));
	ASSERT_EQUAL(0, stats.new);
	ASSERT_EQUAL(exp2.uniq, stats.uniq);
	ASSERT_EQUAL(exp2.total, stats.total);
}

CTEST2(salad, bloomizeb_ex)
{
	static const char* bf =
		"\x84\x00\x00\x02\x01\x01\x00\x44"
		"\x02\x00\x00\x00\x1C\x00\x00\x22"
		"\x88\x00\x00\x00\x10\x00\x00\x11"
		"\x20\x00\x00\x00\x40\x00\x04\x08";

	TEST_BLOOMIZE_EX(b, bf);


	// larger tests
	ASSERT_TRUE(data->b1.ngram_length == data->b2.ngram_length);
	const size_t n = data->b1.ngram_length *8;

	salad_set_ngramlength(&data->b1, n);
	salad_set_ngramlength(&data->b2, n);

	// TODO: bloomizeb_ex2

	const count_exp_t exp1 = {318, 8* (strlen(TEST_STR1) -NGRAM_LENGTH) +1};
	const count_exp_t exp2 = {326, 8* (strlen(TEST_STR2) -NGRAM_LENGTH) +1};
	TEST_BLOOMIZE_COUNT_EX('b', &data->b1, exp1, &data->b2, exp2, 8, n);
}

CTEST2(salad, bloomize_ex)
{
	static const char* bf =
		"\x32\x0b\x9c\x28\x46\x77\x6a\xbd"
		"\xe0\x02\x60\x82\xd9\x0c\x59\x9c"
		"\x26\x96\x81\x18\x73\x54\xfc\xc4"
		"\xbf\x20\x19\x47\x05\x00\xa8\x0a";

	TEST_BLOOMIZE_EX(, bf);

	// larger tests
	ASSERT_TRUE(data->b1.ngram_length == data->b2.ngram_length);
	const size_t n = data->b1.ngram_length;

	// TODO: bloomize_ex2

	const count_exp_t exp1 = {40, strlen(TEST_STR1) -NGRAM_LENGTH +1};
	const count_exp_t exp2 = {41, strlen(TEST_STR2) -NGRAM_LENGTH +1};
	TEST_BLOOMIZE_COUNT_EX('n', &data->b1, exp1, &data->b2, exp2, 1, n);
}

CTEST2(salad, bloomizew_ex)
{
	static const char* bf =
		"\x00\x20\x02\x80\x88\x01\x00\x00"
		"\x60\x00\x00\x02\x80\x80\x00\x00"
		"\x40\x20\x00\x01\x00\x04\x02\x00"
		"\x20\x00\x20\x80\x00\x00\x00\x00";

	DELIM(zero) = {0};
	ASSERT_DATA(zero, DELIM_SIZE, data->x.delimiter.d, DELIM_SIZE);
	to_delimiter_array(TOKEN_DELIMITER, data->x.delimiter.d);

	TEST_BLOOMIZE_EX(w, bf);

	// larger tests
	ASSERT_TRUE(data->b1.ngram_length == data->b2.ngram_length);
	const size_t n = data->b1.ngram_length;

	to_delimiter_array(TOKEN_DELIMITER, data->b1.delimiter.d);
	to_delimiter_array(TOKEN_DELIMITER, data->b2.delimiter.d);

	// TODO: bloomizew_ex2

	const char* x = TEST_STR1;
	size_t num_tokens = 1;
	for(; x < TEST_STR1 +strlen(TEST_STR1); x++)
	{
			if (*x == ' ') num_tokens++;
	}

	const count_exp_t exp = {num_tokens -NGRAM_LENGTH +1, num_tokens -NGRAM_LENGTH +1};
	TEST_BLOOMIZE_COUNT_EX('w', &data->b1, exp, &data->b2, exp, 1, n);
}

CTEST(salad, fileformat_consistency)
{
	char* TEST_FILE = "test.ana";

	SALAD_T(x);
	salad_set_bloomfilter_ex(&x, bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE));
	salad_set_delimiter(&x, DELIMITER);
	salad_set_ngramlength(&x, NGRAM_LENGTH);

	BLOOM* const xbloom = GET_BLOOMFILTER(x.model);
	bloomize_ex(xbloom, TEST_STR1, strlen(TEST_STR1), x.ngram_length);

	FILE* const f_out = fopen(TEST_FILE, "wb+");
	if (f_out == NULL)
	{
		salad_destroy(&x);
		CTEST_ERR("Unable to open test file '%s'.", TEST_FILE);
		ASSERT_NOT_NULL(f_out);
	}

	const int n = fwrite_model(f_out, xbloom, x.ngram_length, DELIMITER, x.as_binary);
	fclose(f_out);

	if (n < 0)
	{
		salad_destroy(&x);
		CTEST_ERR("Failed while writing the model.");
		ASSERT_TRUE(n >= 0);
	}

	FILE* const f_in = fopen(TEST_FILE, "rb");
	if (f_in == NULL)
	{
		salad_destroy(&x);
		CTEST_ERR("Failed to open file.");
		ASSERT_NOT_NULL(f_in);
	}

	SALAD_T(y);
	const int ret = salad_from_file_ex(f_in, &y);
	fclose(f_in);

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
