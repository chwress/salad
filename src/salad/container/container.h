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

#ifndef SALAD_CONTAINER_CONTAINER_H_
#define SALAD_CONTAINER_CONTAINER_H_

#include <stdlib.h>
#include <stdio.h>

#include <util/util.h>

typedef enum { CONTAINER_BLOOMFILTER, CONTAINER_UNKNOWN, CONTAINER_ERROR } container_type_t;

const char* const container_to_string(container_type_t t);


typedef struct
{
	container_type_t type;
	void* data;
} container_t;


#define EMPTY_CONTAINER_INITIALIZER { \
		.type = CONTAINER_UNKNOWN, \
		.data = NULL \
}

#define ERROR_CONTAINER_INITIALIZER { \
		.type = CONTAINER_ERROR, \
		.data = NULL \
}

/**
 * The preferred way of initializing the salad_t object/ struct.
 */
#define CONTAINER_T(c) container_t c = EMPTY_CONTAINER_INITIALIZER
static const CONTAINER_T(EMPTY_CONTAINER);
static const container_t ERROR_CONTAINER = ERROR_CONTAINER_INITIALIZER;

container_t* const container_create();

const int container_init_bloomfilter(container_t* const c, const unsigned int filter_size, const char* const hashset);
const int container_set(container_t* const c, container_t* const other);
const int container_set_bloomfilter(container_t* const c, void* const b);

void container_destroy(container_t* const c);
void container_free(container_t* const c);


typedef struct
{
	const char* filename;
	unsigned int count;
	int done;
} container_outputstate_t;


#define EMPTY_CONTAINER_OUTPUTSTATE_INITIALIZER { \
		.filename = NULL, \
		.count = 0, \
		.done = FALSE \
}

/**
 * The preferred way of initializing the salad_t object/ struct.
 */
#define CONTAINER_OUTPUTSTATE_T(state) container_outputstate_t state = EMPTY_CONTAINER_OUTPUTSTATE_INITIALIZER
static const CONTAINER_OUTPUTSTATE_T(EMPTY_CONTAINER_OUTPUTSTATE);

#endif /* SALAD_CONTAINER_CONTAINER_H_ */
