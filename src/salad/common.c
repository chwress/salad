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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <util/util.h>

const char* const to_delimiter(const char* const str, delimiter_t* const out)
{
	assert(out != NULL);

	const char* x = str;
	if (x != NULL)
	{
		if (x[0] == 0x00)
		{
			x = NULL;
		}
	}
	to_delimiter_array(x, out->d);
	delimiter_array_to_string(out->d, &out->str);
	return x;
}

void to_delimiter_array(const char* const s, delimiter_array_t out)
{
	assert(out != NULL);
    memset(out, 0, 256);

	if (s == NULL)
	{
		return;
	}

	char* const x = malloc((strlen(s) +1) *sizeof(char));
	strcpy(x, s);
	inline_decode(x, strlen(x));

    for (size_t i = 0; i < strlen(x); i++)
    {
    	out[(unsigned int) x[i]] = 1;
    }
    free(x);
}

const char* const delimiter_array_to_string(const delimiter_array_t delim, char** out)
{
	char buf[] = "%00";
	char x[256*3 +1] = {0};
	for (int i = 0; i < DELIM_SIZE; i++)
	{
		if (delim[i])
		{
			sprintf(buf, (isprint(i) ? "%c" : "%%%02X"), i);
			strcat(x, buf);
		}
	}
	STRNDUP(strlen(x), x, *out);
	return *out;
}
