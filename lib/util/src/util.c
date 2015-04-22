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

#include "util/util.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>


#ifndef IOTYPE_FILES
#include <sys/stat.h>
#include <sys/types.h>
#endif

const int cmp(const char* const s, ...)
{
	va_list args;
	va_start(args, s);

	const char* needle = va_arg(args, char*);
	for (int i = 0; needle != NULL; i++)
	{
		if (strcmp(s, needle) == 0)
		{
			va_end(args);
			return i;
		}
		needle = va_arg(args, char*);
	}

	va_end(args);
	return -1;
}

const size_t inline_decode(char* s, const size_t len)
{
	char buf[5] = "0x00";
	unsigned int ch;

	unsigned int j = 0;
	for (unsigned int i = 0; i < len; i++, j++)
	{
		if (s[i] != '%')
		{
			s[j] = s[i];
			continue;
		}

		// write out truncated truncated sequence
		const size_t x = len -i;
		if (x <= 2)
		{
			for (unsigned int k = 0; k < x; k++, i++, j++)
			{
				s[j] = s[i];
			}
			break;
		}

		buf[2] = s[++i];
		buf[3] = s[++i];

		// is valid encoding?
		if (isxdigit(buf[2]) && isxdigit(buf[3]))
		{
			sscanf(buf, "%x", (unsigned int*) &ch);
			s[j] = ch;
		}
		else
		{
			s[j++] = '%';
			s[j++] = buf[2];
			s[j]   = buf[3];
		}
	}
	s[j] = 0x00;
	return j;
}

const int starts_with(const char* const s, const char* const prefix)
{
	assert(s != NULL && prefix != NULL);
	return (strstr(s, prefix) == s);
}

const size_t count_char(const char* const s, const char ch)
{
	assert(s != NULL);

	const char* x = s -1;
	size_t c = 0;

	while (*(++x) != 0x00)
	{
		if (*x == ch)
		{
			c += 1;
		}
	}
	return c;
}

char* const join_ex(const char* const prefix, const char* const sep, const char** strs, const char* const fmt)
{
	assert(sep != NULL && strs != NULL);

	static const size_t BUF_SIZE = 100;
	char buf[BUF_SIZE];

	size_t fmt_size = 0;
	char* format_string = "%s%s";

	if (fmt != NULL)
	{
		snprintf(buf, BUF_SIZE, fmt, "");
		fmt_size = strlen(buf);

		snprintf(buf, BUF_SIZE, "%%s%s", fmt);
		format_string = buf;
	}


	size_t size = (prefix != NULL ? strlen(prefix) : 0);
	size_t num_strings = 0;
	for (const char** x = strs; *x != NULL; x++)
	{
		size += strlen(*x) +fmt_size;
		num_strings++;
	}

	if (num_strings > 0)
	{
		size += (num_strings -1) * strlen(sep);
	}

	char* const out = (char*) malloc(sizeof(char) * (size +1));
	if (out == NULL)
	{
		return NULL;
	}

	if (size <= 0)
	{
		out[size] = 0x00;
		return out;
	}

	if (num_strings <= 0)
	{
		strcpy(out, prefix);
		return out;
	}

	sprintf(out, format_string, (prefix != NULL ? prefix : ""), *strs);
	char* y = strchr(out, 0x00);

	for (const char** x = strs +1; *x != NULL; x++)
	{
		sprintf(y, format_string, sep, *x);
		y = strchr(y, 0x00);
	}
	return out;
}

char* const join(const char* const sep, const char** strs)
{
	return join_ex(NULL, sep, strs, NULL);
}

const int memcmp_bytes(const void* const a, const void* const b, const size_t n)
{
	const uint8_t* A = ((const uint8_t*) a) -1;
	const uint8_t* B = ((const uint8_t*) b) -1;

	const uint8_t* const END = ((const uint8_t*) a) +n;

	while (A < END -1 && *++A == *++B);
	return *A - *B;
}

char* const fread_str(FILE* const f)
{
	size_t n = 2;
	char* out = (char*) malloc((n +1) *sizeof(char));
	char* x = out;

	int c = fgetc(f);
	while (c != EOF && c != 0x00)
	{
		if (x >= out +n)
		{
			out = realloc(out, (n*2 +1) *sizeof(char));
			x = out +n; n *= 2;
		}
		*(x++) = (char) c;
		c = fgetc(f);
	}
	*x = 0x00;
	return realloc(out, (STRNLEN(out, n) +1) *sizeof(char));
}

const float frand()
{
	return (float) rand() / RAND_MAX;
}


#ifdef IOTYPE_FILES
FILE* const open_file_ex(char* path, char* name, struct stat* st)
{
	assert(path != NULL);
	char file[512];

	if (name != NULL)
	{
		snprintf(file, 512, "%s/%s", path, name);
	}
	else
	{
		snprintf(file, 512, "%s", path);
	}

	// Open file
	FILE* f = fopen(file, "rb");
	if (f == NULL)
	{
		// warning("Could not open file '%s'", file);
		return NULL;
	}

	if (st != NULL)
	{
		stat(file, st);
	}
	return f;
}

char* load_file(char* path, char* name, unsigned int* size)
{
	// Open file
	struct stat st;

	FILE* f = open_file_ex(path, name, &st);
	if (f == NULL)
	{
		return NULL;
	}

	// Allocate memory
	if (size != NULL)
	{
		*size = st.st_size;
	}
	char* x = malloc((st.st_size + 1) * sizeof(char));
	if (x == NULL)
	{
		return NULL;
	}

	// Read data
	long read = fread(x, sizeof(char), st.st_size, f);
	fclose(f);

	if (st.st_size != read)
	{
		// weird things are happening
	}
	return x;
}
#endif


