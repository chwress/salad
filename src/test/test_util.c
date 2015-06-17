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

#include <ctest.h>

#include <stdlib.h>
#include <string.h>

#include "common.h"

CTEST(util, cmp)
{
	#define STRS "ABCD", "¼ pounder", "what's that?", "\"§ate%37nh"
	char* strs[] = { STRS, NULL, "missing" };

	char** x = strs;
	for (int i = 0; *x != NULL; x++, i++)
	{
		ASSERT_EQUAL(i, cmp(*x, STRS, NULL));
	}
	ASSERT_EQUAL(-1, cmp(*++x, STRS, NULL));
}

CTEST(util, inline_decode)
{
	char str0[] = "%41%2542%43%20";
	char str1[] = "%41%2542%43%20%";
	char str2[] = "%41%2542%43%20%0";
	char str3[] = "%41%2542%43%20%00";
	char str4[] = "%41%2542%43%20%0x";
	char str5[] = "%41%2542%43%20%x0";
	char str6[] = "%41%2542%43%20%x0%44";
	char str7[] = "¼ pounder with cheese";

	char* strs[] = {
		str0, "A%42C ",
		str1, "A%42C %",
		str2, "A%42C %0",
		str3, "A%42C ",
		str4, "A%42C %0x",
		str5, "A%42C %x0",
		str6, "A%42C %x0D",
		str7, "¼ pounder with cheese", NULL
	};

	for (int i = 0; strs[i] != NULL; i += 2)
	{
		inline_decode(strs[i], strlen(strs[i]));
		ASSERT_STR(strs[i +1], strs[i]);
	}
}

CTEST(util, starts_with)
{
	#define STR "¼ pounder with cheese"

	char str0[] = STR;
	char str1[] = STR "!";

	ASSERT_FALSE(starts_with(str0, str1));
	ASSERT_TRUE (starts_with(str0, str0));

	str1[strlen(str0) -7] = 0x00;
	ASSERT_TRUE (starts_with(str0, str1));

	str1[1] = 0x00;
	ASSERT_TRUE (starts_with(str0, str1));

	ASSERT_TRUE (starts_with(str0, ""));
	ASSERT_TRUE (starts_with(STR, str0));

	ASSERT_TRUE (starts_with("", ""));
	ASSERT_FALSE(starts_with("", "a"));
}

CTEST(util, count_char)
{
	char str[] = "1/4 pounder with cheese";
	ASSERT_EQUAL(1, count_char(str, '1'));
	ASSERT_EQUAL(3, count_char(str, ' '));
	ASSERT_EQUAL(4, count_char(str, 'e'));
	ASSERT_EQUAL(1, count_char(str, 'o'));
	ASSERT_EQUAL(0, count_char(str, 'a'));
	ASSERT_EQUAL(0, count_char(str, 0x00));
}

#define TEST_JOIN_EX(exp, prefix, sep, strs, fmt)       \
{                                                       \
	char* const out = join_ex(prefix, sep, strs, fmt);  \
	ASSERT_EQUAL(0, strcmp(out, exp));                  \
	free(out);                                          \
}

CTEST(util, join_ex)
{
	const char* strs[] = {
		"The", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog", NULL
	};

	TEST_JOIN_EX("Thequickbrownfoxjumpsoverthelazydog", NULL, "", strs, NULL);
	TEST_JOIN_EX("The quick brown fox jumps over the lazy dog", NULL, " ", strs, NULL);
	TEST_JOIN_EX("The  quick  brown  fox  jumps  over  the  lazy  dog", NULL, "  ", strs, NULL);

	TEST_JOIN_EX(" The  quick  brown  fox  jumps  over  the  lazy  dog ", NULL, "", strs, " %s ");
	TEST_JOIN_EX("x x x x x x x x x", NULL, " ", strs, "x");

	TEST_JOIN_EX("", NULL, "", strs, "");

	strs[1] = NULL;
	TEST_JOIN_EX("The", NULL, "-", strs, NULL);
	TEST_JOIN_EX("The", "", "-", strs, NULL);
	TEST_JOIN_EX("The", "\x00TEST ", "-", strs, NULL);
	TEST_JOIN_EX("TEST The", "TEST ", "-", strs, NULL);

	strs[0] = NULL;
	TEST_JOIN_EX("", NULL, "-", strs, NULL);
	TEST_JOIN_EX("", NULL, "-", strs, "asdf");
	TEST_JOIN_EX("TEST ", "TEST ", "-", strs, NULL);
}

CTEST(util, memcmp_bytes)
{
	static const size_t size = 100;
	uint8_t a[size], b[size], c[size];

	memset(&a, 0x00, sizeof(uint8_t) *size);
	memset(&b, 0x00, sizeof(uint8_t) *size);
	memset(&c, 0xFF, sizeof(uint8_t) *size);


	ASSERT_EQUAL(0, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));
	ASSERT_NOT_EQUAL(0, memcmp_bytes(&a, &c, sizeof(uint8_t) *size));


	b[size -1] = 1;
	ASSERT_EQUAL(-1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));

	a[size -1] = 2;
	ASSERT_EQUAL(1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));


	a[42] = 0xFF;
	ASSERT_EQUAL(0xFF, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));

	b[23] = 0xFF;
	ASSERT_EQUAL(-0xFF, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));


	a[0] = 1;
	ASSERT_EQUAL(1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));

	b[0] = 2;
	ASSERT_EQUAL(-1, memcmp_bytes(&a, &b, sizeof(uint8_t) *size));


	memset(&a, 0xFF, sizeof(uint8_t) *(size -2));
	ASSERT_EQUAL(-0xFF, memcmp_bytes(&a, &c, sizeof(uint8_t) *(size -1)));

	a[size -2] = 0xFF;
	ASSERT_EQUAL(0, memcmp_bytes(&a, &c, sizeof(uint8_t) *(size -1)));

	a[size -1] = 0xFF;
	ASSERT_EQUAL(0, memcmp_bytes(&a, &c, sizeof(uint8_t) *size));
}
