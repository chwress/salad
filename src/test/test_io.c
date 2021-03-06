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
#include <util/io.h>
#include <util/util.h>

#include "common.h"

const char* const input_lines = TEST_SRC"res/testing/http.txt";
const char* const input_files = TEST_SRC"res/testing/http/";
const char* const input_archive = TEST_SRC"res/testing/http.tar.gz";
const char* const input_networkdump = TEST_SRC"res/testing/http.cap";

const char* const INPUT_REF[] = {
	TEST_SRC"res/testing/ref/http.0",
	TEST_SRC"res/testing/ref/http.1",
	NULL
};

typedef struct
{
	data_t* expected;
	size_t n;
} testdata_t;

void testdata_free(testdata_t* const td)
{
	assert(td != NULL);

	for (size_t i = 0; i < td->n; i++)
	{
		data_free(&td->expected[i]);
	}
	free(td->expected);
}

CTEST_DATA(io)
{
	const data_processor_t* lines;
	const data_processor_t* files;
#ifdef USE_ARCHIVES
	const data_processor_t* archive;
#endif
#ifdef USE_NETWORK
	const data_processor_t* network;
#endif

	testdata_t ref;
};

CTEST_SETUP(io)
{
	data->lines = to_dataprocessor(IOMODE_LINES);
	data->files = to_dataprocessor(IOMODE_FILES);
#ifdef USE_ARCHIVES
	data->archive = to_dataprocessor(IOMODE_ARCHIVE);
#endif
#ifdef USE_NETWORK
	data->network = to_dataprocessor(IOMODE_NETWORK);
#endif

	size_t num_refs = 0;
	for (size_t i = 0; INPUT_REF[i] != NULL; i++) num_refs++;
	data->ref.expected = (data_t*) calloc(num_refs, sizeof(data_t));
	data->ref.n = 0;

	for (size_t i = 0; i < num_refs; i++)
	{
		FILE* const f = fopen(INPUT_REF[i], "rb");
		if (f == NULL)
		{
			testdata_free(&data->ref);
			CTEST_ERR("Cannot read reference file '%s'", INPUT_REF[i]);
			ASSERT_FAIL();
		}
		fseek_s(f, 0, SEEK_END);
		size_t n = ftell_s(f);
		data->ref.expected[i].len = n;
		data->ref.expected[i].buf = (char*) calloc(n, sizeof(data_t));
		data->ref.n++;

		fseek_s(f, 0, SEEK_SET);
		n = fread(data->ref.expected[i].buf, 1, data->ref.expected[i].len, f);
		fclose(f);

		if (data->ref.expected[i].len != n)
		{
			testdata_free(&data->ref);
			ASSERT_EQUAL_U(data->ref.expected[i].len, n);
		}
	}
}

CTEST_TEARDOWN(io)
{
	testdata_free(&data->ref);
}

const int test_read_stub(const char* const filename, const data_processor_t* const dp, void* const p, testdata_t* const ref)
{
	assert(filename != NULL);
	assert(dp != NULL);
	assert(dp->open != NULL && dp->read != NULL && dp->close != NULL);
	assert(ref != NULL);

	file_t input;
	if (dp->open(&input, filename, FILE_IO_READ, p) != EXIT_SUCCESS)
	{
		CTEST_ERR("Unable to open test file '%s'.", filename);
		ASSERT_FAIL();
	}

	static const size_t N = 1;

	dataset_t ds;
	ds.capacity = N;
	ds.data = (data_t*) calloc(ds.capacity, sizeof(data_t));
	ds.n = 0;

	data_t* const data = ds.data;
	size_t num_read = 0, j = 0;
	do
	{
		num_read = dp->read(&input, &ds, N);
		ASSERT_TRUE(j +num_read <= ref->n);
		// Do not use dataset_free(.) since we want to reuse the
		// memory consumed by dataset_t#data
		for (size_t i = 0; i < ds.n; i++)
		{
			ASSERT_DATA(
				(unsigned char*) ref->expected[j].buf, ref->expected[j].len,
				(unsigned char*) data[i].buf, data[i].len
			);
			j++;
			data_free(&data[i]);
		}
	} while (num_read >= N);

	free(ds.data); // cf. comment above
	dp->close(&input);

	ASSERT_TRUE(j == ref->n);
	return EXIT_SUCCESS;
}

typedef struct
{
	size_t j;
	testdata_t* const ref;
} test_recv_t;

const int test_recv_callback(data_t* data, const size_t n, void* usr)
{
	assert(data != NULL);
	assert(usr  != NULL);

	test_recv_t* const x = (test_recv_t*) usr;
	ASSERT_TRUE(x->j +n <= x->ref->n);

	for (size_t i = 0; i < n; i++)
	{
		ASSERT_DATA(
			(unsigned char*) x->ref->expected[x->j].buf, x->ref->expected[x->j].len,
			(unsigned char*) data[i].buf, data[i].len
		);
		x->j++;
	}
	return EXIT_SUCCESS;
}

const int test_recv_stub(const char* const filename, const data_processor_t* const dp, void* const p, testdata_t* const ref)
{
	assert(filename != NULL);
	assert(dp != NULL);
	assert(dp->open != NULL && dp->recv != NULL && dp->close != NULL);
	assert(ref != NULL);

	file_t input;
	if (dp->open(&input, filename, FILE_IO_READ, p) != EXIT_SUCCESS)
	{
		CTEST_ERR("Unable to open test file '%s'.", filename);
		ASSERT_FAIL();
	}

	test_recv_t usr = { 0, ref };
	const size_t ret = dp->recv(&input, test_recv_callback, 1, &usr);
	dp->close(&input);

	ASSERT_TRUE(usr.j == ref->n); // That basically is the same check as the following
	return (ret == ref->n ? EXIT_SUCCESS : EXIT_FAILURE);
}

CTEST2(io, read_lines)
{
	ASSERT_EQUAL(
		EXIT_SUCCESS,
		test_read_stub(input_lines, data->lines, NULL, &data->ref)
	);
}

CTEST2(io, recv_lines)
{
	ASSERT_EQUAL(
		EXIT_SUCCESS,
		test_recv_stub(input_lines, data->lines, NULL, &data->ref)
	);
}

// Not yet supported
CTEST2_SKIP(io, read_files)
{
	ASSERT_EQUAL(
		EXIT_SUCCESS,
		test_read_stub(input_files, data->files, NULL, &data->ref)
	);
}

CTEST2_SKIP(io, recv_files)
{
	ASSERT_EQUAL(
		EXIT_SUCCESS,
		test_recv_stub(input_files, data->files, NULL, &data->ref)
	);
}

#ifdef USE_ARCHIVES
CTEST2(io, read_archive)
{
	ASSERT_EQUAL(
		EXIT_SUCCESS,
		test_read_stub(input_archive, data->archive, NULL, &data->ref)
	);
}

CTEST2(io, recv_archive)
{
	ASSERT_EQUAL(
		EXIT_SUCCESS,
		test_recv_stub(input_archive, data->archive, NULL, &data->ref)
	);
}
#endif


#ifdef USE_NETWORK
CTEST2_SKIP(io, read_network)
{
	ASSERT_EQUAL(EXIT_SUCCESS, EXIT_SUCCESS);
}

CTEST2(io, recv_network)
{
	ASSERT_EQUAL(
		EXIT_SUCCESS,
		test_recv_stub(input_networkdump, data->network, NULL, &data->ref)
	);
}
#endif
