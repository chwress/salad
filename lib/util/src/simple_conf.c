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

#include <util/simple_conf.h>
#include <util/getline.h>
#include <util/util.h>


char* const readline(char** buf, size_t* len, FILE* f)
{
	while (1)
	{
		const ssize_t read = getline(buf, len, f);
		if (read < 0)
		{
			return NULL;
		}

		char* x = ltrim(*buf);
		if (*x == '#') continue; // ignore comments

		rtrim(x);
		if (*x == 0x00) continue; // ignore empty lines

		return x;
	}
}

const int fread_config(FILE* const f, const char* const header, FN_KEYVALUE handle, void* const usr)
{
	assert(f != NULL);
	const size_t pos = ftell_s(f);

	char* line = NULL;
	size_t len = 0;

	if (header != NULL)
	{
		char* x = readline(&line, &len, f);
		if (x == NULL || strcasecmp(x, header) != 0)
		{
			free(line);
			fseek_s(f, pos, SEEK_SET);
			return 0;
		}
	}

	unsigned int nhandled = 0;
	while (1)
	{
		char* x = readline(&line, &len, f);
		if (x == NULL)
		{
			break;
		}

		char* y = strchr(x, '=');
		if (y == NULL)
		{
			free(line);
			return -1;
		}

		y[0] = 0x00;
		char* value = ltrim(y +1);
		rtrim(x);

		if (handle(f, x, value, usr)) nhandled++;
	}
	free(line);
	return UNSIGNED_SUBSTRACTION(ftell_s(f), pos);
}
