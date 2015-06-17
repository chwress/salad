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
#include "util/vec.h"
#include "util/util.h"
#include "util/murmur.h"

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

	// TODO: Make the the number of bits configurable
	const int num_bits = 24;
	dim_t mask = ((long long unsigned) 2 << (num_bits - 1)) - 1;

	// TODO: 0x123457678 is the magic number used in Sally
	return MurmurHash64B(s, len, 0x12345678) & mask;
}


const int random_level()
{
	static int first = 1;
	int lvl = 0;

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

vec_elem_t* make_node(const int level, const dim_t dim, const float value)
{
	vec_elem_t* sn = (vec_elem_t*) malloc(sizeof(vec_elem_t));
	sn->forward = (vec_elem_t**) calloc(level + 1, sizeof(vec_elem_t *));

	sn->dim = dim;
	sn->value = value;
	return sn;
}

vec_t* const vec_create(const int len)
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
		fprintf(stdout, "%" PRIu64 ": %f", x->dim, x->value);
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

const float vec_get(const vec_t* const vec, const dim_t dim)
{
	assert(vec != NULL);

	if (vec->length >= 0 && dim >= vec->length)
	{
		fprintf(stderr, "Dimension out of range");
		abort();
	}

	int i;
	vec_elem_t* x = vec->header;
	for (i = vec->level; i >= 0; i--)
	{
		while (x->forward[i] != NULL && x->forward[i]->dim < dim)
		{
			x = x->forward[i];
		}
	}
	x = x->forward[0];
	if (x != NULL && x->dim == dim)
		return x->value;

	return 0.0;
}

void delete(vec_t* const vec, const dim_t dim);

void vec_set(vec_t* const vec, const dim_t dim, const float value)
{
	assert(vec != NULL);

	if (value == 0.0)
	{
		delete(vec, dim);
		return;
	}

	int i;
	vec_elem_t* x = vec->header;
	vec_elem_t* update[MAX_LEVEL + 1];
	memset(update, 0, MAX_LEVEL + 1);

	for (i = vec->level; i >= 0; i--)
	{
		while (x->forward[i] != NULL && x->forward[i]->dim < dim)
		{
			x = x->forward[i];
		}
		update[i] = x;
	}
	x = x->forward[0];

	if (x == NULL || x->dim != dim)
	{
		int lvl = random_level();

		if (lvl > vec->level)
		{
			for (i = vec->level + 1; i <= lvl; i++)
			{
				update[i] = vec->header;
			}
			vec->level = lvl;
		}
		x = make_node(lvl, dim, value);
		for (i = 0; i <= lvl; i++)
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

	int i;
	vec_elem_t* x = vec->header;
	vec_elem_t* update[MAX_LEVEL + 1];
	memset(update, 0, MAX_LEVEL + 1);

	for (i = vec->level; i >= 0; i--)
	{
		while (x->forward[i] != NULL && x->forward[i]->dim < dim)
		{
			x = x->forward[i];
		}
		update[i] = x;
	}
	x = x->forward[0];

	if (x != NULL && x->dim == dim)
	{
		for (i = 0; i <= vec->level; i++)
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

	vec_t* const vec = vec_create(-1);
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
			vec->length = atoi(line + 11);
		}
	}

	// Empty feature vector
	if (vec->length == 0)
	{
		return vec;
	}

	size_t read = getline(&line, &len, f);
	for (unsigned int i = 0; i < vec->length && read != -1; i++)
	{
		float w = atof(line);
		if (w > 0.0)
		{
			vec_set(vec, i, w);
		}
		read = getline(&line, &len, f);
	}
	free(line);
	return vec;
}

