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

/**
 * @file
 */

#ifndef UTIL_VEC_H_
#define UTIL_VEC_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef size_t dim_t;

const dim_t hash(const char* const s, const size_t len);

/**
 * The sparse vector implementation is based on the Skip Set implementation
 * at http://en.literateprograms.org/Skip_list_(C)
 */

struct sn
{
	dim_t dim;
	float value;
	struct sn** forward; // pointer to array of pointers
};

typedef struct sn vec_elem_t;

typedef struct
{
	vec_elem_t* header;
	size_t level;
	size_t length;
} vec_t;

vec_t* const vec_create(const size_t len);
void vec_print(vec_t* const v);
const size_t vec_length(const vec_t* const v);

typedef void (*FN_VEC_FOREACH)(const dim_t dim, const float value, void* data);
void vec_foreach(const vec_t* const vec, FN_VEC_FOREACH op, void* data);

const float vec_get(const vec_t* const v, const dim_t dim);
void vec_set(vec_t* const v, const dim_t dim, const float value);

vec_t* const vec_read_liblinear(FILE* const f);

#endif /* UTIL_VEC_H_ */
