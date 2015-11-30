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

#include "util/getline.h"

#include <util/vec.h>
#include <util/util.h>
#include <util/murmur.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>


#define P 0.5
#define MAX_LEVEL 6


const dim_t hash(const char* const s, const size_t len)
{
	assert(s != NULL && len >= 0);
	assert(len <= INT32_MAX); // cf. MurmurHash64B :/

	// TODO: Make the the number of bits configurable
	const int num_bits = 24;
	dim_t mask = ((long long unsigned) 2 << (num_bits - 1)) - 1;

	// TODO: 0x123457678 is the magic number used in Sally
	return MurmurHash64B(s, (int32_t) len, 0x12345678) & mask;
}


const size_t random_level()
{
	static int first = 1;
	size_t lvl = 0;

	if (first)
	{
		srand((unsigned) time(NULL));
		first = 0;
	}

	while (frand() < P && lvl < MAX_LEVEL)
	{
		lvl++;
	}
	return lvl;
}

vec_elem_t* make_node(const size_t level, const dim_t dim, const double value)
{
	vec_elem_t* sn = (vec_elem_t*) malloc(sizeof(vec_elem_t));
	sn->forward = (vec_elem_t**) calloc(level + 1, sizeof(vec_elem_t *));

	sn->dim = dim;
	sn->value = value;
	return sn;
}

vec_t* const vec_create(const size_t len)
{
	vec_t* const v = (vec_t*) malloc(sizeof(vec_t));
	v->header = make_node(MAX_LEVEL, 0, 0.0);
	v->level = 0;
	v->length = len;
	return v;
}

void vec_print(vec_t* const vec)
{
	assert(vec != NULL);

	vec_elem_t* x = vec->header->forward[0];
	fprintf(stdout, "{");
	while (x != NULL)
	{
		fprintf(stdout, "%"ZU": %f", (SIZE_T) x->dim, x->value);
		x = x->forward[0];
		if (x != NULL)
			printf(",");
	}
	fprintf(stdout, "}\n");
}

const size_t vec_length(const vec_t* const vec)
{
	assert(vec != NULL);

	size_t length = 0;

	vec_elem_t* x = vec->header->forward[0];
	while (x != NULL)
	{
		length++;
		x = x->forward[0];
	}
	return length;
}

void vec_foreach(const vec_t* const vec, FN_VEC_FOREACH op, void* data)
{
	assert(vec != NULL);

	if (op != NULL)
	{
		vec_elem_t* x = vec->header->forward[0];
		while (x != NULL)
		{
			op(x->dim, x->value, data);
			x = x->forward[0];
		}
	}
}

const double vec_get(const vec_t* const vec, const dim_t dim)
{
	assert(vec != NULL);

	if (dim >= vec->length)
	{
		fprintf(stderr, "Dimension out of range");
		abort();
	}

	vec_elem_t* x = vec->header;
	for (size_t i = vec->level +1; i > 0; i--)
	{
		const size_t j = i -1;
		while (x->forward[j] != NULL && x->forward[j]->dim < dim)
		{
			x = x->forward[j];
		}
	}
	x = x->forward[0];
	if (x != NULL && x->dim == dim)
		return x->value;

	return 0.0;
}

void delete(vec_t* const vec, const dim_t dim);

void vec_set(vec_t* const vec, const dim_t dim, const double value)
{
	assert(vec != NULL);

	if (value == 0.0)
	{
		delete(vec, dim);
		return;
	}

	vec_elem_t* x = vec->header;
	vec_elem_t* update[MAX_LEVEL + 1];
	memset(update, 0, MAX_LEVEL + 1);

	for (size_t i = vec->level +1; i > 0; i--)
	{
		const size_t j = i -1;
		while (x->forward[j] != NULL && x->forward[j]->dim < dim)
		{
			x = x->forward[j];
		}
		update[j] = x;
	}
	x = x->forward[0];

	if (x == NULL || x->dim != dim)
	{
		const size_t lvl = random_level();

		if (lvl > vec->level)
		{
			for (size_t i = vec->level + 1; i <= lvl; i++)
			{
				update[i] = vec->header;
			}
			vec->level = lvl;
		}
		x = make_node(lvl, dim, value);
		for (size_t i = 0; i <= lvl; i++)
		{
			x->forward[i] = update[i]->forward[i];
			update[i]->forward[i] = x;
		}
	}
	else // update existing element
	{
		x->value = value;
	}
}

void delete(vec_t* const vec, const dim_t dim)
{
	assert(vec != NULL);

	vec_elem_t* x = vec->header;
	vec_elem_t* update[MAX_LEVEL + 1];
	memset(update, 0, MAX_LEVEL + 1);

	for (size_t i = vec->level +1; i > 0; i--)
	{
		const size_t j = i -1;
		while (x->forward[j] != NULL && x->forward[j]->dim < dim)
		{
			x = x->forward[j];
		}
		update[j] = x;
	}
	x = x->forward[0];

	if (x != NULL && x->dim == dim)
	{
		for (size_t i = 0; i <= vec->level; i++)
		{
			if (update[i]->forward[i] != x)
				break;
			update[i]->forward[i] = x->forward[i];
		}
		free(x);
		while (vec->level > 0 && vec->header->forward[vec->level] == NULL)
		{
			vec->level--;
		}
	}
}


/**
 * Reads a liblinear weight vector form a file stream
 * @param f File pointer
 * @return The weights as feature vector
 */
vec_t* const vec_read_liblinear(FILE* const f)
{
	assert(f != NULL);

	vec_t* const vec = vec_create(SIZE_MAX);
	if (vec == NULL)
	{
		fprintf(stderr, "Could not create feature vector");
		return NULL;
	}

	char* line = NULL;
	size_t len = 0;

	while (getline(&line, &len, f) != -1 && strncmp(line, "w", 1) != 0)
	{
		if (strncmp(line, "nr_feature", 10) == 0)
		{
			char* tail;
			long long int ll = strtoll(line + 11, &tail, 10);
			assert(ll >= 0 && ll <= SIZE_MAX);
			vec->length = (size_t) MIN(SIZE_MAX, (unsigned long) MAX(0, ll));
		}
	}

	// Empty feature vector
	if (vec->length == 0)
	{
		return vec;
	}

	ssize_t read = getline(&line, &len, f);
	for (size_t i = 0; i < vec->length && read != -1; i++)
	{
		double w = atof(line);
		if (w > 0.0)
		{
			vec_set(vec, i, w);
		}
		read = getline(&line, &len, f);
	}
	free(line);
	return vec;
}

