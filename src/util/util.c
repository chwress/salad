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

#include "util.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>


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

char* const fread_str(FILE* const f)
{
	size_t n = 2;
	char* out = (char*) malloc(n +1);
	char* x = out;

	int c = fgetc(f);
	while (c != EOF && c != 0x00)
	{
		if (x >= out +n)
		{
			out = realloc(out, (n *= 2) +1);
		}
		*(x++) = c;
		c = fgetc(f);
	}
	*x = 0x00;
	return realloc(out, strlen(out) +1);
}

const float frand()
{
	return (float) rand() / RAND_MAX;
}


void progress_step()
{
	fprintf(stdout, ".");
	fflush(stdout);
}

void progress()
{
	static size_t c = 0; c++;
	if (c % BATCH_SIZE == 0)
	{
		progress_step();
	}
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
	FILE* f = fopen(file, "r");
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


