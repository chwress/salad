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

#include "util/log.h"
#include "util/util.h"
#include "util/colors.h"

#include <stdio.h>
#include <string.h>

int log_level = STATUS;

#define LOGBUF_LEN 0x1000
static char buf[LOGBUF_LEN];

int force_stderr = FALSE;
#define  STATUS_PREFIX "[*]"
#define    INFO_PREFIX "[I]"
#define WARNING_PREFIX "[W]"
#define   ERROR_PREFIX "[!]"
#define  FATERR_PREFIX "[!!]"

void vprint_ex(const log_t l, const char* const color, const char* const msg, va_list args)
{
	if (log_level > l)
	{
		return;
	}

	FILE* const f = (force_stderr || l >= WARNING ? stderr : stdout);

	if (color != NULL)
	{
		fprintf(f, "%s", color);
	}

	switch (l)
	{
	case STATUS:
		fprintf(f, STATUS_PREFIX " ");
		break;

	case INFO:
		fprintf(f, INFO_PREFIX " ");
		break;

	case WARNING:
		fprintf(f, WARNING_PREFIX " ");
		break;

	case ERROR:
		fprintf(f, ERROR_PREFIX " ");
		break;

	case FATAL_ERROR:
		fprintf(f, FATERR_PREFIX " ");
		break;
	}
	vsnprintf(buf, LOGBUF_LEN, msg, args);
	fprintf(f, "%s\n", buf);

	if (color != NULL)
	{
		fprintf(f, "%s", COLOR_X);
	}
}

void print_ex(const log_t l, const char* const color, const char* const msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprint_ex(l, color, msg, args);
	va_end(args);
}

void print(const char* const msg, ...)
{
	if (log_level <= INFO)
	{
		va_list args;
		va_start(args, msg);
		vsnprintf(buf, LOGBUF_LEN, msg, args);
		fprintf(stdout, "%s\n", buf);
		va_end(args);
	}
}

void status(const char* const msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprint_ex(STATUS, NULL, msg, args);
	va_end(args);
}

void info(const char* const msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprint_ex(INFO, NULL, msg, args);
	va_end(args);
}

void warn(const char* const msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprint_ex(WARNING, NULL, msg, args);
	va_end(args);
}

void error(const char* const msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprint_ex(ERROR, NULL, msg, args);
	va_end(args);
}

void fatal(const char* const msg, ...)
{
	va_list args;
	va_start(args, msg);
	vprint_ex(FATAL_ERROR, NULL, msg, args);
	va_end(args);
}


static struct {
	char* const prefix;
	char* const suffix;

	char empty;
	char done;
	char head;
	char end;

	size_t length;
} BAR = {STATUS_PREFIX " ", "", '-', '=', '>', '=', 70};

void progress(const size_t cur, const size_t total)
{
	if (log_level > INFO || total <= 0)
	{
		return;
	}

	assert(cur <= total);

	char bar[BAR.length +1];
	memset(bar, BAR.empty, BAR.length);
	bar[BAR.length] = 0x00;

	char* const start = bar +strlen(BAR.prefix);
	char* const end   = bar +BAR.length -strlen(BAR.suffix);
	const size_t len = end -start;

	memcpy(bar, BAR.prefix, strlen(BAR.prefix));
	memcpy(end, BAR.suffix, strlen(BAR.suffix));

	const int done = (int) (len *((double) cur) /total);

	if (done > 0)
	{
		memset(start, BAR.done, done -1);
		start[done -1] = (cur < total ? BAR.head : BAR.end);
	}

	static int LABEL_SIZE = 4;

	char perc[LABEL_SIZE +1];
	snprintf(perc, LABEL_SIZE +1, "%u%%", (unsigned int) (100*((double)cur)/total));
	memcpy(start +LABEL_SIZE +1 -strlen(perc), perc, strlen(perc));

	if (cur < total)
	{
		fprintf(stderr, "\r%s", bar);
	}
	else
	{
		fprintf(stderr, "\r%s", bar); fflush(stderr);
		fprintf(stderr, "\r%*c\r", (int) BAR.length +1, '\0');
	}
	// fprintf(stderr, "\r" "%s%c", bar, (cur < total ? '\0' : '\n'));
}


static struct {
	const char* const prefix;
	const char* const suffix;

	const char* const chars;
	const uint8_t num_chars;
} HOURGLASS = {STATUS_PREFIX " Processing ", "", "-\\|/", 4};

void hourglass(uint8_t* const state, const size_t c)
{
	static size_t BATCH_SIZE = 1000;

	if (c % BATCH_SIZE == 0)
	{
		hourglass_ex(state);
	}
}

void hourglass_ex(uint8_t* const state)
{
	if (log_level > INFO)
	{
		return;
	}

	// Are wever gonna use
	static int reset = FALSE;

	if (state != NULL)
	{
		fprintf(stderr, "\r%s%c%s",
				HOURGLASS.prefix,
				HOURGLASS.chars[++(*state) % HOURGLASS.num_chars],
				HOURGLASS.suffix);
		reset = TRUE;
	}
	else if (reset)
	{
		const size_t n = strlen(HOURGLASS.prefix) + strlen(HOURGLASS.suffix) +1;
		fprintf(stderr, "\r%*c\r", (int) n +1, '\0');
		reset = FALSE;
	}
}

void hourglass_stop()
{
	hourglass_ex(NULL);
}

const int bye_ex(const int ec, const char* const msg)
{
	static const char* const s = "Bye!";
	if (ec == EXIT_SUCCESS)
	{
		print_ex(STATUS, COLOR_G, "%s", msg == NULL ? s : msg);
	}
	else
	{
		print_ex(ERROR, COLOR_R, "%s", s);
	}
	return ec;
}

const int bye(const int ec)
{
	return bye_ex(ec, NULL);
}
