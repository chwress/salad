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

#ifndef SALAD_UTIL_H_
#define SALAD_UTIL_H_

#include "common.h"
#include "salad.h"

#include <container/common.h>

void salad_set_bloomfilter_ex(salad_t* const s, BLOOM* const b);


#define GET_BLOOMFILTER(model) \
	(BLOOM*) model.x; \
	assert(model.type == SALAD_MODEL_BLOOMFILTER);

#define TO_BLOOMFILTER(model) (BLOOM*) model.x;


#define SET_NOTSPECIFIED(model) { \
	model.x = NULL; \
	model.type = SALAD_MODEL_NOTSPECIFIED; \
}


typedef struct {
	BLOOM* const bloom1; // e.g. good content filter
	BLOOM* const bloom2; // e.g. bad content filter
	const size_t n;      // n-gram length
	const delimiter_array_t delim;
} bloom_param_t;


#endif /* SALAD_UTIL_H_ */
