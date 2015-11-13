/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "util/getline.h"

#include <util/util.h>
#include <stdlib.h>

#if (_XOPEN_SOURCE -0) < 700
#include <errno.h>
#include <string.h>

ssize_t getdelim(char** buf, size_t* bufsiz, int delimiter, FILE* fp)
{
	char *ptr, *eptr;

	if (*buf == NULL || *bufsiz == 0)
	{
		*bufsiz = BUFSIZ;
		if ((*buf = malloc(*bufsiz)) == NULL)
		{
			return -1;
		}
	}

	for (ptr = *buf, eptr = *buf + *bufsiz;;)
	{
		int c = fgetc(fp);
		if (c < 0)
		{
			if (feof(fp)) break;
			return -1;
		}
		*ptr++ = (char) c;
		if (c == delimiter)
		{
			*ptr = '\0';
			return ptr - *buf;
		}
		if (ptr + 2 >= eptr)
		{
			char *nbuf;
			size_t nbufsiz = *bufsiz * 2;
			ssize_t d = ptr - *buf;
			if ((nbuf = realloc(*buf, nbufsiz)) == NULL)
			{
				return -1;
			}
			*buf = nbuf;
			*bufsiz = nbufsiz;
			eptr = nbuf + nbufsiz;
			ptr = nbuf + d;
		}
	}
	return ptr == *buf ? -1 : ptr - *buf;
}

ssize_t getline(char **buf, size_t *bufsiz, FILE *fp)
{
	return getdelim(buf, bufsiz, '\n', fp);
}

#endif


char* const getlines(const char* const fname)
{
	return getlines_ex(fname, NULL);
}

char* const getlines_ex(const char* const fname, const char* const prefix)
{
	FILE* f = fopen(fname, "r");
	if (f == NULL)
	{
		return NULL;
	}

	fseek_s(f, 0, SEEK_END);
	size_t size = ftell_s(f);
	fseek_s(f, 0, SEEK_SET);

	char* log = malloc(sizeof(char) * (size +1));
	if (log == NULL || size == 0)
	{
		return NULL;
	}
	log[0] = 0x00;

	char* const x = log;
	char* line = NULL;
	size_t len = 0;

	while (size > 0)
	{
		const ssize_t read = getline(&line, &len, f);
		if (read < 0)
		{
			break;
		}

		const size_t s = MIN(size, (size_t) read);
		if (s > 0 && (prefix == NULL || starts_with(line, prefix)))
		{
			memcpy(log, line, s +1);

			// Fix potential NULL bytes
			for (char* y = log; y -log < s; y++)
			{
				if (*y == 0x00) *y = ' ';
			}

			log += s;
			size -= s;
		}
	}
	free(line);

	fclose(f);
	return x;
}
