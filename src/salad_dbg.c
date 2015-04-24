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

#include "main.h"

#ifdef TEST_SALAD

#define CTEST_MAIN
#include <ctest.h>

extern const char* salad_filename;

const int _salad_dbg_(const test_config_t* const c)
{
	assert(c != NULL);

	const char* argv[] = { salad_filename, NULL, NULL };

	if (c->test_suite != NULL)
	{
		argv[1] = c->test_suite;
	}

	if (c->disable_memcheck)
	{
		argv[2] = "valgrind";
	}

	return (ctest_main(3, argv) == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

#endif
