/*
 * libutil - Yet Another Utility Library
 * Copyright (c) 2012-2015, Christian Wressnegger
 * --
 * This file is part of the library libutil.
 *
 * libutil is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libutil is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "util/format.h"
#include "util/util.h"

#include <string.h>
#include <ctype.h>

const char* SYMBOLS_CUSTOMARY[] = {"B", "K", "M", "G", "T", "P", "E", "Z", "Y", NULL};
const char* SYMBOLS_CUSTOMARY_EXT[] = {"byte", "kilo", "mega", "giga", "tera", "peta", "exa", "zetta", "iotta", NULL};
const char* SYMBOLS_IEC[] = {"Bi", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi", NULL};
const char* SYMBOLS_IEC_EX[] = {"byte", "kibi", "mebi", "gibi", "tebi", "pebi", "exbi", "zebi", "yobi", NULL};

const char* const* SYMBOLS[] = {SYMBOLS_CUSTOMARY, SYMBOLS_CUSTOMARY_EXT, SYMBOLS_IEC, SYMBOLS_IEC_EX, NULL};


const char* const bytes2human(char* const buf, const size_t n, const size_t x)
{
	return bytes2human_ex(buf, n, x, SYM_CUSTOMARY);
}

const char* const bytes2human_ex(char* const buf, const size_t n, const size_t x, const symbol_variants_t variant)
{
	assert(buf != NULL);

	int m = 0;
	double d = x;
	for (; d > 1024; m++) d /= 1024;

	// Round to up to one decimal place
	const size_t s = (size_t) ceil(d *10);

	if (((double) s) == d)
	{
		snprintf(buf, n, "%"Z"%s", (SIZE_T) d, SYMBOLS[(int) variant][m]);
	}
	else
	{
		snprintf(buf, n, "%"Z".%d%s", (SIZE_T) (s /10), (int) (s %10), SYMBOLS[(int) variant][m]);
	}

	return buf;
}

const size_t human2bytes(const char* const x)
{
	assert(x != NULL);

	char* next = NULL;
	const double n = strtod(x, &next);

	while (*next != 0x00 && isblank(*next)) next++;

	int m = -1;
	for (size_t i = 0; SYMBOLS[i] != NULL && m < 0; i++)
	{
		m = cmp2(next, SYMBOLS[i]);
	}

	if (m >= 6 && n > 16) // 16 Exabyte
	{
		return 0;
	}
	return (m < 0 ? n : n * POW(1024, m));
}
