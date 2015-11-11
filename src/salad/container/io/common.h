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

/**
 * @file
 */

#ifndef SALAD_CONTAINER_IO_COMMON_H_
#define SALAD_CONTAINER_IO_COMMON_H_

#include <util/io.h>

typedef enum {
	CONTAINER_OUTPUTFMT_BINARY,
	CONTAINER_OUTPUTFMT_TXT,
	CONTAINER_OUTPUTFMT_MIXED,
	CONTAINER_OUTPUTFMT_SEPARATED,
	CONTAINER_OUTPUTFMT_NOT_SPECIFIED
} container_outputformat_t;

typedef struct
{
	FILE* config;
	FILE* data;
	container_outputformat_t type;
} container_outputspec_t;


#define EMPTY_CONTAINER_OUTPUTSPEC_INITIALIZER { \
		.config = NULL, \
		.data = NULL, \
		.type = CONTAINER_OUTPUTFMT_NOT_SPECIFIED \
}

#define CONTAINER_OUTPUTSPEC_T(spec) container_outputspec_t spec = EMPTY_CONTAINER_OUTPUTSPEC_INITIALIZER

#define CONTAINER_TXT(spec) (\
		(spec)->type == CONTAINER_OUTPUTFMT_TXT || \
		(spec)->type == CONTAINER_OUTPUTFMT_MIXED || \
		(spec)->type == CONTAINER_OUTPUTFMT_SEPARATED)


typedef struct
{
	void* data;

	FN_REQUESTFILE request_file;
	void* host;
} container_iodata_t;


#define EMPTY_CONTAINER_IODATA_INITIALIZER { \
		.data = NULL, \
		.request_file = NULL \
}

#define CONTAINER_IODATA_T(state) container_iostate_t state = EMPTY_CONTAINER_IODATA_INITIALIZER

#endif /* SALAD_CONTAINER_IO_COMMON_H_ */
