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

#ifndef UTIL_IO_H_
#define UTIL_IO_H_

#include <stdlib.h>
#include <stdio.h>

#include "../config.h"


#ifdef USE_ARCHIVES
typedef enum { LINES, FILES, ARCHIVE } io_mode_t;
#else
typedef enum { LINES, FILES } io_mode_t;
#endif

const char* const to_string(const io_mode_t m);
const io_mode_t to_iomode(const char* const str);
const int is_valid_iomode(const char* const str);

#ifdef USE_ARCHIVES
	#define VALID_IOMODES "'lines', 'files' or 'archive'"
#else
	#define VALID_IOMODES "'lines' or 'files'"
#endif

typedef struct
{
	char* buf;
	size_t len;
} data_t;

typedef struct
{
	FILE* fd;
	void* data;
} file_t;

typedef const int(*FN_OPEN)(file_t* const f, const char* const filename);
typedef const size_t (*FN_READ)(file_t* const f, data_t* data, const size_t numLines);
typedef const int(*FN_CLOSE)(file_t* const f);

typedef struct
{
	FN_OPEN open;
	FN_READ read;
	FN_CLOSE close;
} data_processor_t;

const data_processor_t* const to_dataprocssor(const io_mode_t m);


#endif /* UTIL_IO_H_ */
