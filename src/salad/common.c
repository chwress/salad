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


const int BITGRAM_SIZE = sizeof(bitgram_t);
const int BITGRAM_BITSIZE = sizeof(bitgram_t) * 8;
const int MASK_BITSIZE = sizeof(ngram_mask_t) * 8 -7;


void delimiter_init(delimiter_t* const d)
{
	assert(d != NULL);

	*d = EMPTY_DELIMITER;
	d->str = calloc(1, sizeof(char));
}

void delimiter_destroy(delimiter_t* const d)
{
	assert(d != NULL);

	if (d->str != NULL)
	{
		free(d->str);
	}
	*d = EMPTY_DELIMITER;
}


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

	delimiter_destroy(out);

	to_delimiter_array(x, out->d);
	delimiter_array_to_string(out->d, &out->str);
	return out->str;
}

void to_delimiter_array(const char* const s, delimiter_array_t out)
{
	assert(out != NULL);
    memset(out, 0, sizeof(uint8_t) *DELIM_SIZE);

	if (s == NULL || s[0] == 0x00)
	{
		return;
	}

	char* const x = malloc((strlen(s) +1) *sizeof(char));
	strcpy(x, s);
	const size_t n = inline_decode(x, strlen(x));

    for (size_t i = 0; i < n; i++)
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
			sprintf(buf, (isprint(i) && !isspace(i) ? "%c" : "%%%02X"), i);
			strcat(x, buf);
		}
	}
	STRNDUP(strlen(x), x, *out);
	return *out;
}

#define SETBIT(m, x)	(m) |= (0x80>>(x))
#define GETBIT(m, x)	(m) & (0x80>>(x))

int to_ngram_mask(const char* const s, ngram_mask_t* const out)
{
	assert(out != NULL);

	if (s == NULL)
	{
		*out = 0;
	}
	else
	{
		for (int i = 0; i < strlen(s); i++)
		{
			if (s[i] != '0' && s[i] != '1')
			{
				return FALSE;
			}
			SETBIT(*out, i);
		}
	}
	return TRUE;
}

const char* const ngram_mask_to_string(const ngram_mask_t m, char** out)
{
	*out = (char*) calloc(MASK_BITSIZE +1, sizeof(char));

	for (int i = 0; i < MASK_BITSIZE; i++)
	{
		(*out)[i] = (GETBIT(m, i) ? '1' : '0');
	}
	return *out;
}
