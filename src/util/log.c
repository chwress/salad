/**
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

#include "log.h"
#include "util.h"

#include <stdio.h>

static char buf[0x1000];
int force_stderr = FALSE;

void print_ex(const log_t l, const char* const msg, va_list args)
{
	FILE* const fout = (force_stderr || l >= WARNING ? stderr : stdout);

	switch (l)
	{
	case STATUS:
		fprintf(fout, "[*] ");
		break;

	case INFO:
		fprintf(fout, "[I] ");
		break;

	case WARNING:
		fprintf(fout, "[W] ");
		break;

	case ERROR:
		fprintf(fout, "[!] ");
		break;

	case FATAL_ERROR:
		fprintf(fout, "[!!] ");
		break;
	}
	vsnprintf(buf, 0x1000, msg, args);
	fprintf(fout, "%s\n", buf);
}

void print(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
	vsnprintf(buf, 0x1000, msg, args);
	fprintf(stdout, "%s\n", buf);
    va_end(args);
}

void status(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
	print_ex(STATUS, msg, args);
    va_end(args);
}

void info(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
	print_ex(INFO, msg, args);
    va_end(args);
}

void warn(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
	print_ex(WARNING, msg, args);
    va_end(args);
}

void error(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
	print_ex(ERROR, msg, args);
    va_end(args);
}

void fatal(const char* const msg, ...)
{
    va_list args;
    va_start(args, msg);
	print_ex(FATAL_ERROR, msg, args);
    va_end(args);
}

