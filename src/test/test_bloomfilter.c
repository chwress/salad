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
#include <salad/anagram.h>
#include <util/util.h>

#include <string.h>
#include <limits.h>


CTEST_DATA(bloom)
{
	BLOOM* b1;
	BLOOM* b2;

	BLOOM* x1;
	BLOOM* x2;

	size_t BITSIZE;
};

CTEST_SETUP(bloom)
{
	data->BITSIZE = POW(2, DEFAULT_BFSIZE);

	data->b1 = bloom_create(data->BITSIZE, NUM_FUNCS, FUNCS);
	data->b2 = bloom_create(data->BITSIZE, NUM_FUNCS, FUNCS);

	data->x1 = bloom_init(DEFAULT_BFSIZE, HASHES_SIMPLE);
	data->x2 = bloom_init(DEFAULT_BFSIZE, HASHES_MURMUR);
}

CTEST_TEARDOWN(bloom)
{
	bloom_destroy(data->b1);
	bloom_destroy(data->b2);

	bloom_destroy(data->x1);
	bloom_destroy(data->x2);
}

CTEST2(bloom, create)
{
	int INTSIZE = (data->BITSIZE +CHAR_BIT -1)/CHAR_BIT;

	ASSERT_NOT_NULL(data->b1);
	ASSERT_EQUAL(NUM_FUNCS, data->b1->nfuncs);
	ASSERT_EQUAL(data->BITSIZE, data->b1->bitsize);
	ASSERT_EQUAL(INTSIZE, data->b1->size);

	int x = 0;
	for (int i = 0; i < data->b1->size; ++i)
	{
		x |= data->b1->a[i];
	}
	ASSERT_EQUAL(0, x);
}

CTEST2(bloom, compare)
{
	ASSERT_EQUAL(0, bloom_compare(data->b1, data->b2));

	data->b1->bitsize--;
	ASSERT_NOT_EQUAL(0, bloom_compare(data->b1, data->b2));
	data->b2->bitsize--;
	ASSERT_EQUAL(0, bloom_compare(data->b1, data->b2));
	data->b1->bitsize++;
	data->b2->bitsize++;

	data->b1->size--;
	ASSERT_NOT_EQUAL(0, bloom_compare(data->b1, data->b2));
	data->b2->size--;
	ASSERT_EQUAL(0, bloom_compare(data->b1, data->b2));
	data->b1->size++;
	data->b2->size++;

	data->b1->a[0]++;
	ASSERT_NOT_EQUAL(0, bloom_compare(data->b1, data->b2));
	data->b2->a[0]++;
	ASSERT_EQUAL(0, bloom_compare(data->b1, data->b2));
	data->b1->a[0]--;
	data->b2->a[0]--;
}

CTEST2(bloom, add)
{
	bloom_add_str(data->b1, "abc", 3);
	ASSERT_NOT_EQUAL(0, bloom_compare(data->b1, data->b2));
}

CTEST2(bloom, count)
{
	data->b1->a[0] = 0x03;
	data->b1->a[data->b1->size -1] = 0x80;
	ASSERT_EQUAL(3, bloom_count(data->b1));
}

CTEST2(bloom, hash_collisions)
{
	bloom_add_str(data->x1, "abc", 3);
	ASSERT_EQUAL(3, bloom_count(data->x1));

	bloom_add_str(data->x2, "abc", 3);
	ASSERT_EQUAL(3, bloom_count(data->x2));
}

