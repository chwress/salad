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


#include <ctest.h>

#include <salad/salad.h>
#include <salad/util.h>

#include <util/getline.h>
#include <util/colors.h>
#include <util/util.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <limits.h>
#include <unistd.h>
#include <strings.h>

#include "common.h"

#define CMD_LENGTH 4096

#define TEST_DATA "The quick brown fox jumps over the lazy dog"
#define TEST_DATA_LENGTH strlen(TEST_DATA)

#define TEST_INPUT "test.in"
#define TEST_OUT "test.out"
#define TEST_LOG "test.log"

#ifdef USE_ARCHIVES
static const char* const EX1_INPUT1 = TEST_SRC "res/testing/ex1-train.zip";
static const char* const EX1_INPUT2 = TEST_SRC "res/testing/ex1-test.zip";
static const char* const EX1_OUTPUT = TEST_SRC "res/testing/ref/ex1/n=%"ZU".model";
static const char* const EX1_SCORES = TEST_SRC "res/testing/ref/ex1/n=%"ZU".scores";

static const char* const EX2_INPUT1 = TEST_SRC "res/testing/ex2-train.zip";
static const char* const EX2_OUTPUT = TEST_SRC "res/testing/ref/ex2/n=%"ZU".model";
static const char* const EX2_STATS  = TEST_SRC "res/testing/ref/ex2/n=%"ZU".stats";
static const char* const EX2_INSPECT= TEST_SRC "res/testing/ref/ex2/n=%"ZU".inspect";
#endif


CTEST_DATA(main)
{
	char cmd[CMD_LENGTH];
	size_t initial_length;

	const char* log;
	const char* out;
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

	data->log = TEST_LOG;
	data->out = TEST_OUT;
}

CTEST_TEARDOWN(main)
{
	remove(data->log);
	remove(data->out);
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

static char* const read_log_ex(const struct main_data* const d, char* const prefix)
{
	assert(d != NULL);
	char* const log = getlines_ex(d->log, prefix);
	assert(log != NULL);
	return log;
}

static char* const read_log(const struct main_data* const d)
{
	return read_log_ex(d, NULL);
}

static const int EXEC_EX(const struct main_data* const d)
{
	assert(d != NULL);
	char cmd[CMD_LENGTH +100];
	snprintf(cmd, CMD_LENGTH +100, " %s > %s 2>&1", d->cmd, d->log);

	const int ret = system(cmd);
#ifdef WEXITSTATUS
	return WEXITSTATUS(ret);
#else
	return ret;
#endif
}

static const int EXEC(const int expected_return_value, const struct main_data* const d)
{
	const int ret = EXEC_EX(d);
	if (ret != expected_return_value)
	{
		FILE* f = fopen(d->log, "r");
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
			free(line);
			fclose(f);
		}
		remove(d->log);
		remove(d->out);
	}
	ASSERT_EQUAL(expected_return_value, ret);
	return ret;
}

static int FIND_IN_LOG_EX(const struct main_data* const d, const char* const needle)
{
	char* const log = read_log(d);
	if (log != NULL)
	{
		char* const x = strstr(log, needle);
		free(log);

		return (x != NULL);
	}
	return FALSE;
}

static void FIND_IN_LOG(const struct main_data* const d, const char* const needle)
{
	if (!FIND_IN_LOG_EX(d, needle))
	{
		remove((d)->log);
		CTEST_LOG("Not found: %s", needle);
		ASSERT_FAIL();
	}
}

void CMP_FILES(const char* const a, const char* const b)
{
	FILE* const f1 = fopen(a, "rb");
	ASSERT_NOT_NULL(f1);

	FILE* const f2 = fopen(b, "rb");
	if (f2 == NULL)
	{
		fclose(f1);
		ASSERT_NOT_NULL(f2);
	}

	int ch1, ch2;
	do
	{
		ch1 = getc(f1);
		ch2 = getc(f2);
	} while ((ch1 != EOF) && (ch2 != EOF) && (ch1 == ch2));

	fclose(f1); fclose(f2);
	ASSERT_EQUAL(ch1, ch2);
}

void CMP_FILECONTENT(const char* const filename, const char* const data, const size_t n)
{
	FILE* const f = fopen(filename, "rb");
	ASSERT_NOT_NULL(f);

	const unsigned char* x = (unsigned char*) data;
	const unsigned char* const end = (unsigned char*) data +n;
	int ch1, ch2;
	do
	{
		ch1 = getc(f);
		ch2 = *x++;
	} while ((ch1 != EOF) && (x != end) && (ch1 == ch2));

	fclose(f);
	ASSERT_EQUAL(ch1, ch2);
}

void CMP_LINE(const char* const a, const char* const b, const char* const needle)
{
	char* const A = getlines(a);
	ASSERT_NOT_NULL(A);

	char* const B = getlines(b);
	if (B == NULL)
	{
		free(A);
		ASSERT_NOT_NULL(B);
	}

	char* const x = strstr(A, needle);
	char* const y = strstr(B, needle);

	if (x == NULL || y == NULL)
	{
		free(A); free(B);
		CTEST_LOG("Does not contain '%s'", needle);
		ASSERT_FAIL();
	}

	for (size_t i = 0, j = 0; x[i] != '\n' && x[i] != 0x00; i++, j++)
	{
		if (x[i] != y[j])
		{
			free(A); free(B);
			ASSERT_STR(x, y);
		}
	}

	free(A); free(B);
}

void CMP_MODELS(const char* const a, const char* const b)
{
	SALAD_T(s1);
	if (salad_from_file(a, &s1) != EXIT_SUCCESS)
	{
		salad_destroy(&s1);
		ASSERT_FAIL();
	}

	SALAD_T(s2);
	if (salad_from_file(b, &s2) != EXIT_SUCCESS)
	{
		salad_destroy(&s1);
		salad_destroy(&s2);
		return; // Output file doesn't exist?
	}

	ASSERT_FALSE(salad_spec_diff(&s1, &s2));

	BLOOM* const b1 = GET_BLOOMFILTER(s1.model);
	BLOOM* const b2 = GET_BLOOMFILTER(s2.model);

	ASSERT_EQUAL(0, bloom_compare(b1, b2));

	salad_destroy(&s1);	salad_destroy(&s2);
}

static const char* SALAD_MODES[] = {
		"train", "predict", "inspect", "stats", "test", NULL
};


CTEST2(main, modes)
{
	SET_MODE(data, "mlsec");
	EXEC(1, data);
	FIND_IN_LOG(data, "Unknown mode 'mlsec'");

	SET_MODE(data, "train");
	EXEC(1, data);
	FIND_IN_LOG(data, "No input file");
}

// Test salad's modes (train, predict, inspect, ...)
CTEST2(main, help)
{
	char* const possible_modes = join_ex("<mode> may be one of ", ", ", SALAD_MODES, "'%s'");

	ADD_PARAM(data, "--help", "");
	EXEC(0, data);
	char* const log = read_log(data);
	ASSERT_NOT_NULL(strstr(log, "Usage: salad [<mode>] [options]"));
	ASSERT_NOT_NULL(strstr(log, possible_modes));
	free(log);
	free(possible_modes);

	for (const char** x = SALAD_MODES; *x != NULL; x++)
	{
		char str[100];
		snprintf(str, 100, "Usage: salad %s [options]", *x);

		SET_MODE(data, *x);
		ADD_PARAM(data, "--help", "");
		EXEC(0, data);
		FIND_IN_LOG(data, str);
	}
}

CTEST2(main, train)
{
	// No output specified
	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	EXEC(1, data);
	FIND_IN_LOG(data, "No output file");

	// Illegal input specification
	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT "-4KmR19beV");
	ADD_PARAM(data, "-o", data->out);
	EXEC(1, data);
	FIND_IN_LOG(data, "Unable to open input");

	// Default to lines mode
	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "-o", data->out);
	EXEC(0, data);
	FIND_IN_LOG(data, "(lines mode)");


	// Input modes
	const char* input_modes[] = {
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

	for (const char** x = input_modes; *x != NULL; x++)
	{
		char str[100];
		snprintf(str, 100, "(%s mode", *x);

		SET_MODE(data, "train");
		ADD_PARAM(data, "-i", TEST_INPUT "-4KmR19beV");
		ADD_PARAM(data, "-f", *x);
		ADD_PARAM(data, "-o", data->out);
		EXEC(1, data);
		FIND_IN_LOG(data, str);
	}

	// Input mode 'files' is not yet implemented
	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "-f", "files");
	ADD_PARAM(data, "-o", data->out);
	EXEC(1, data);
	FIND_IN_LOG(data, "'files' is not yet implemented");

	// Echoing program arguments
	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "-o", data->out);
	ADD_PARAM(data, "-e", "");
	EXEC(0, data);

	char* const log = read_log_ex(data, "[I] ");
	ASSERT_EQUAL_U(1 +3, count_char(log, '\n'));
	ASSERT_NOT_NULL(strstr(log, "n-Gram length: 3"));
	ASSERT_NOT_NULL(strstr(log, "Filter size: 24"));
	ASSERT_NOT_NULL(strstr(log, "Hash set: simple"));
	free(log);

	ADD_PARAM(data, "--help", "");
	EXEC(0, data);
	FIND_IN_LOG(data, "Usage: salad train [options]");

	// Output format txt
	typedef struct {
		const char* const fmt;
		const char* const prefix;
	} testpair_t;

	const testpair_t output_fmts[] = {
			{"txt", "Salad Configuration"},
#ifdef USE_ARCHIVES
			{"archive", "\x50\x4B"},
#endif
			{NULL, NULL}
	};


	for (const testpair_t* x = output_fmts; x->fmt != NULL; x++)
	{
		SET_MODE(data, "train");
		ADD_PARAM(data, "-i", TEST_INPUT);
		ADD_PARAM(data, "-o", data->out);
		ADD_PARAM(data, "-F", x->fmt);
		EXEC(0, data);

		CMP_FILECONTENT(data->out, x->prefix, strlen(x->prefix));
	}
}

CTEST2(main, train_bgrams)
{
	char* const exp =
		"\x84\x00\x00\x00\x84\x00\x00\x00"
		"\x84\x00\x00\x00\x84\x00\x00\x00"
		"\x84\x00\x00\x00\x84\x00\x00\x00"
		"\x84\x00\x00\x00\x84\x00\x00\x00";

	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "--ngram-len", "3");
	ADD_PARAM(data, "--binary", "");
	ADD_PARAM(data, "--filter-size", "8");
	ADD_PARAM(data, "--hash-set", "simple2");
	ADD_PARAM(data, "-o", data->out);
	EXEC(0, data);

	SALAD_T(s);
	salad_from_file(data->out, &s);

	BLOOM* const b = GET_BLOOMFILTER(s.model);
	ASSERT_DATA((unsigned char*) exp, 32, b->a, b->size);
}

CTEST2(main, train_ngrams)
{
	char* const exp =
		"\x12\x8f\x9c\x68\x04\xf7\x42\xbc"
		"\xa0\x02\x64\xa2\xd8\x00\x51\x94"
		"\x23\x9a\x61\xd8\x73\x55\xe8\x64"
		"\xbe\x18\x30\x4f\x07\x00\xa0\x0a";

	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "--ngram-len", "3");
	ADD_PARAM(data, "--filter-size", "8");
	ADD_PARAM(data, "--hash-set", "simple2");
	ADD_PARAM(data, "-o", data->out);
	EXEC(0, data);

	SALAD_T(s);
	salad_from_file(data->out, &s);

	BLOOM* const b = GET_BLOOMFILTER(s.model);
	ASSERT_DATA((unsigned char*) exp, 32, b->a, b->size);
}

CTEST2(main, train_wgrams)
{
	char* const exp =
		"\x00\x00\x08\x02\x20\x20\x04\x08"
		"\x10\x00\x10\x00\x00\x01\x00\x20"
		"\x80\x10\x21\x00\x80\x00\x00\x00"
		"\x00\x00\x00\x01\x10\x00\x28\x00";

	SET_MODE(data, "train");
	ADD_PARAM(data, "-i", TEST_INPUT);
	ADD_PARAM(data, "--ngram-len", "3");
	ADD_PARAM(data, "--ngram-delim", "' '");
	ADD_PARAM(data, "--filter-size", "8");
	ADD_PARAM(data, "--hash-set", "simple2");
	ADD_PARAM(data, "-o", data->out);
	EXEC(0, data);

	SALAD_T(s);
	salad_from_file(data->out, &s);

	BLOOM* const b = GET_BLOOMFILTER(s.model);
	ASSERT_DATA((unsigned char*) exp, 32, b->a, b->size);
}

#ifdef USE_ARCHIVES
CTEST2(main, ex1)
{
	for (size_t n = 1; n <= 3; n++)
	for (size_t batch_size = 0; batch_size < 10; batch_size++)
	{
		// training
		SET_MODE(data, "train");
		ADD_PARAM(data, "-i", EX1_INPUT1);
		ADD_PARAM(data, "-f", "archive");
		if (batch_size > 0)
		{
			ADD_PARAM(data, "--batch-size", "%"ZU, (SIZE_T) batch_size);
		}
		ADD_PARAM(data, "-n", "%"ZU, (SIZE_T) n);
		ADD_PARAM(data, "-d", "%s", "\"%0a%0d%20%21%22.,:;?\"");
		ADD_PARAM(data, "--hash-set", "simple");
		ADD_PARAM(data, "-o", data->out);
		int ret = EXEC_EX(data);
		if (ret != 0)
		{
			CTEST_LOG("'%s' is not available", EX1_INPUT1);
			return; // Input file doesn't exist?
		}

		FIND_IN_LOG(data, "[*] Train salad on");

		char buf[0x1000];
		snprintf(buf, 0x1000, EX1_OUTPUT, (SIZE_T) n);

		CMP_MODELS(data->out, buf);


		// testing
		char b[0x1000];
		snprintf(b, 0x1000, "%s~tmp", data->out);
		ASSERT_NOT_EQUAL(-1, rename(data->out, b));

		SET_MODE(data, "predict");
		ADD_PARAM(data, "-i", EX1_INPUT2);
		ADD_PARAM(data, "-f", "archive");
		if (batch_size > 0)
		{
			ADD_PARAM(data, "--batch-size", "%"ZU, (SIZE_T) batch_size);
		}
		ADD_PARAM(data, "-b", b);
		ADD_PARAM(data, "-o", data->out);
		ret = EXEC_EX(data);
		remove(b);

		if (ret != 0)
		{
			CTEST_LOG("'%s' is not available", EX1_INPUT2);
			return; // Input file doesn't exist?
		}

		snprintf(buf, 0x1000, EX1_SCORES, n);
		CMP_FILES(data->out, buf);
	}
}

CTEST2(main, ex2)
{
	for (size_t n = 1; n <= 3; n++)
	for (size_t batch_size = 0; batch_size < 10; batch_size++)
	{
		// training
		SET_MODE(data, "train");
		ADD_PARAM(data, "-i", EX2_INPUT1);
		ADD_PARAM(data, "-f", "archive");
		if (batch_size > 0)
		{
			ADD_PARAM(data, "--batch-size", "%"ZU, (SIZE_T) batch_size);
		}
		ADD_PARAM(data, "-n", "%"ZU, (SIZE_T) n);
		ADD_PARAM(data, "-d", "''");
		ADD_PARAM(data, "--hash-set", "simple");
		ADD_PARAM(data, "-o", data->out);
		int ret = EXEC_EX(data);
		if (ret != 0)
		{
			CTEST_LOG("'%s' is not available", EX2_INPUT1);
			return; // Input file doesn't exist?
		}

		FIND_IN_LOG(data, "[*] Train salad on");

		char buf[0x1000];
		snprintf(buf, 0x1000, EX2_OUTPUT, (SIZE_T) n);

		CMP_MODELS(data->out, buf);


		// stats
		char b[0x1000];
		snprintf(b, 0x1000, "%s~tmp", data->out);
		ASSERT_NOT_EQUAL(-1, rename(data->out, b));

		SET_MODE(data, "stats");
		ADD_PARAM(data, "--bloom", b);
		ret = EXEC_EX(data);
		remove(b);

		snprintf(buf, 0x1000, EX2_STATS, n);
		CMP_LINE(data->log, buf, "Saturation: ");

		// inspect
		SET_MODE(data, "inspect");
		ADD_PARAM(data, "-i", EX2_INPUT1);
		ADD_PARAM(data, "-f", "archive");
		if (batch_size > 0)
		{
			ADD_PARAM(data, "--batch-size", "%"ZU, (SIZE_T) batch_size);
		}
		ADD_PARAM(data, "-n", "%"ZU, (SIZE_T) n);
		ADD_PARAM(data, "-d", "''");
		ADD_PARAM(data, "-o", data->out);
		ret = EXEC_EX(data);
		if (ret != 0)
		{
			CTEST_LOG("'%s' is not available", EX2_INPUT1);
			return; // Input file doesn't exist?
		}

		snprintf(buf, 0x1000, EX2_INSPECT, (SIZE_T) n);
		CMP_FILES(data->out, buf);
	}
}
#endif

CTEST(main, cleanup)
{
	remove(TEST_INPUT);
}

CTEST(valgrind, memcheck)
{
	struct main_data d;
	d.log = TEST_LOG "-memcheck";
	d.out = TEST_OUT "-memcheck";

	snprintf(d.cmd, CMD_LENGTH, "valgrind --tool=memcheck");
	if (EXEC_EX(&d) == 127)
	{
		CTEST_LOG("valgrind not installed... ");
	}
	else
	{
		snprintf(d.cmd , CMD_LENGTH, "valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all %s test -m", my_name);
		EXEC_EX(&d);
		remove(d.out);

		FIND_IN_LOG(&d, "ERROR SUMMARY: 0 errors from 0 contexts");
		if (!FIND_IN_LOG_EX(&d, "All heap blocks were freed -- no leaks are possible"))
		{
			CTEST_LOG("A third-party library seems to cause problems. Salad appears to be ok, though.");
		}
		remove(d.log);
	}
}
