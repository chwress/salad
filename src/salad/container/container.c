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

#include "container.h"

#include "bloom.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

const char* const container_to_string(container_type_t t)
{
	switch (t)
	{
	case CONTAINER_BLOOMFILTER: return "bloom-filter";
	default:                    return "unknown";
	}
}

container_t* const container_create()
{
	container_t* const c = (container_t*) calloc(1, sizeof(container_t));
	if (c == NULL) NULL;

	*c = EMPTY_CONTAINER;
	return c;
}


const int container_init_bloomfilter(container_t* const c, const unsigned int filter_size, const char* const hashset)
{
	assert(c != NULL);
	*c = EMPTY_CONTAINER;

	assert(filter_size <= USHRT_MAX);
	return container_set_bloomfilter(c, bloom_init((unsigned short) filter_size, to_hashset(hashset)));
}

const int container_isvalid(container_t* const c)
{
	if (c == NULL || c->data == NULL) return FALSE;

	switch (c->type)
	{
	case CONTAINER_BLOOMFILTER:
		if (((BLOOM*) c->data)->size <= 0) return FALSE;
		break;
	default:
		break;
	}
	return TRUE;
}

const int container_set(container_t* const c, container_t* const other)
{
	assert(c != NULL);
	if (!container_isvalid(other))
	{
		*c = ERROR_CONTAINER;
		return FALSE;
	}
	c->data = other->data;
	c->type = other->type;
	return TRUE;
}

const int container_set_bloomfilter(container_t* const c, void* const b)
{
	assert(c != NULL);
	container_destroy(c);

	if (b == NULL)
	{
		*c = ERROR_CONTAINER;
		return FALSE;
	}

	c->data = b;
	c->type = CONTAINER_BLOOMFILTER;
	return TRUE;
}

void container_destroy(container_t* const c)
{
	assert(c != NULL);

	if (c->data != NULL)
	{
		switch (c->type)
		{
		case CONTAINER_BLOOMFILTER:
			bloom_destroy(c->data);
			break;

		default: break;
		}
	}
}

void container_free(container_t* const c)
{
	assert(c != NULL);
	container_destroy(c);
	free(c);
}
