/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2015, Christian Wressnegger
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
#include "config.h"
#include "util/getline.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>

#include <limits.h>
#include <unistd.h>
#include <strings.h>

#define CMD_LENGTH 4096

#define TEST_DATA "The quick brown fox jumps over the lazy dog"
#define TEST_DATA_LENGTH strlen(TEST_DATA)

#define TEST_INPUT "test.in"
#define TEST_OUT "test.out"
#define TEST_LOG "test.log"


CTEST_DATA(main)
{
	char cmd[CMD_LENGTH];
	size_t initial_length;
};

CTEST_SETUP(main)
{
	snprintf(data->cmd, CMD_LENGTH, "%s", my_name);
	data->initial_length = strlen(data->cmd);

	FILE* f = fopen(TEST_INPUT, "r");
	if (f != NULL)
	{
		fclose(f);
	}
	else
	{
		f = fopen(TEST_INPUT, "w+");
		if (f == NULL)
		{
			ASSERT_FAIL();
		}
		size_t n = fwrite(TEST_STR1, sizeof(uint8_t), TEST_DATA_LENGTH, f);
		if (n != TEST_DATA_LENGTH)
		{
			ASSERT_FAIL();
		}
		fclose(f);
	}
}

CTEST_TEARDOWN(main)
{
	remove(TEST_OUT);
	remove(TEST_LOG);
}

static void RESET(const struct main_data* const d)
{
	assert(d != NULL);

	char* const space = strchr(d->cmd, ' ');
	if (space != NULL)
	{
		space[0] = 0x00;
	}

	assert(strlen(d->cmd) == d->initial_length);
}

static void SET_MODE(struct main_data* const d, const char* const mode)
{
	assert(d != NULL && mode != NULL);

	RESET(d);
	const size_t len = strlen(d->cmd);
	snprintf(d->cmd +len, CMD_LENGTH -len, " %s", mode);
}

static void ADD_PARAM(struct main_data* const d, const char* const parg, const char* const value, ...)
{
	assert(d != NULL && parg != NULL && value != NULL);
	char param[CMD_LENGTH];

	va_list args;
	va_start(args, value);

	const size_t len = strlen(d->cmd);
	snprintf(param, CMD_LENGTH, " %s %s", parg, value);
	vsnprintf(d->cmd +len, CMD_LENGTH -len, param, args);

	va_end(args);
}

static char* const read_log()
{
	FILE* f = fopen(TEST_LOG, "r");
	if (f == NULL)
	{
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* log = malloc(sizeof(char) * size);
	if (log == NULL)
	{
		return NULL;
	}

	char* const x = log;
	char* line = NULL;
	size_t len = 0;

	while (size > 0)
	{
		const ssize_t read = getline(&line, &len, f);
		if (read < 0) break;

		const size_t s = MIN(size, read);
		if (s > 0)
		{
			memcpy(log, line, s);

			// Fix potential NULL bytes
			char* y = strchr(log, 0x00);
			while (y -log < s)
			{
				y[0] = ' ';
				y = strchr(++y, 0x00);
			}

			log += s;
			size -= s;
		}
	}
	fclose(f);
	return x;
}

static const int EXEC_EX(const struct main_data* const d)
{
	assert(d != NULL);
	char cmd[CMD_LENGTH +100];
	snprintf(cmd, CMD_LENGTH +100, " %s > " TEST_LOG " 2>&1", d->cmd);

	return WEXITSTATUS(system(cmd));
}

static const int EXEC(const int expected_return_value, const struct main_data* const d)
{
	const int ret = EXEC_EX(d);
	if (ret != expected_return_value)
	{
		FILE* f = fopen(TEST_LOG, "r");
		if (f != NULL)
		{
			char* line = NULL;
			size_t len = 0;

			while (1)
			{
				const ssize_t read = getline(&line, &len, f);
				if (read < 0) break;

				char* const newline = strrchr(line, '\n');
				if (newline != NULL)
				{
					newline[0] = 0x00;
				}
				CTEST_ERR("%s", line);
			}
			fclose(f);
		}
	}
	ASSERT_EQUAL(expected_return_value, ret);
	return ret;
}

static void FIND_IN_LOG(const char* const needle)
{
	char* const log = read_log();
	ASSERT_NOT_NULL(strstr(log, needle));
	free(log);
}

// Test salad's modes (train, predict, inspect, ...)
CTEST2(main, help)
{
	ADD_PARAM(data, "--help", "");
	EXEC(0, data);

	SET_MODE(data, "train");
	ADD_PARAM(data, "--help", "");
	EXEC(0, data);

	SET_MODE(data, "predict");
	ADD_PARAM(data, "--help", "");
	EXEC(0, data);

	SET_MODE(data, "inspect");
	ADD_PARAM(data, "--help", "");
	EXEC(0, data);

	SET_MODE(data, "dbg");
	ADD_PARAM(data, "--help", "");
	EXEC(1, data);
}

CTEST2(main, train)
{
	SET_MODE(data, "train");
	ADD_PARAM(data, "--help", "");
	EXEC(0, data);


	SET_MODE(data, "train");
	EXEC(1, data);
	FIND_IN_LOG("No input file");


	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	EXEC(1, data);
	FIND_IN_LOG("No output file");


	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT "-4KmR19beV");
	ADD_PARAM(data, "-o", TEST_OUT);
	EXEC(1, data);
	FIND_IN_LOG("Unable to open input");


	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "-o", TEST_OUT);
	EXEC(0, data);
	FIND_IN_LOG("(lines mode)");


	char* input_modes[] = {
			"lines",
			"files",
#ifdef USE_ARCHIVES
			"archive",
#endif
#ifdef USE_NETWORK
#ifdef ALLOW_LIVE_TRAINING
			"network",
#endif
			"network-dump",
#endif
			NULL
	};

	for (char** x = input_modes; *x != NULL; x++)
	{
		SET_MODE(data, "train");
		ADD_PARAM(data, "-i", TEST_INPUT);
		ADD_PARAM(data, "-f", *x);
		ADD_PARAM(data, "-o", TEST_OUT);
		EXEC_EX(data);

		char str[100];
		snprintf(str, 100, "(%s mode)", *x);

		char* const log = read_log();
		ASSERT_NOT_NULL(strstr(log, str));
		free(log);
	}

	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "-f", "files");
	ADD_PARAM(data, "-o", TEST_OUT);
	EXEC(1, data);
	FIND_IN_LOG("'files' is not yet implemented");
}

CTEST2(main, train_bgrams)
{
	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "--binary", "");
	ADD_PARAM(data, "-o", TEST_OUT);
	EXEC(0, data);
}

CTEST2(main, train_ngrams)
{
}

CTEST2(main, train_wgrams)
{
}


CTEST(main, cleanup)
{
	remove(TEST_INPUT);
}
