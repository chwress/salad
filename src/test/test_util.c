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
#include <string.h>


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
