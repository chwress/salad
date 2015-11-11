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

#include "util.h"

#include <container/container.h>

#include <assert.h>


void salad_create_container(salad_t* const s)
{
	assert(s != NULL);

	if (s->model.x == NULL)
	{
		s->model.x = container_create();
	}
	s->model.type = SALAD_MODEL_NOTSPECIFIED;
}

const saladmodel_type_t to_saladmodeltype(const container_type_t t)
{
	switch (t)
	{
	case CONTAINER_BLOOMFILTER: return SALAD_MODEL_BLOOMFILTER;
	default:                    return SALAD_MODEL_NOTSPECIFIED;
	}
}

void salad_set_container(salad_t* const s, container_t* const c)
{
	assert(c != NULL);
	salad_create_container(s);

	if (container_set(s->model.x, c))
	{
		s->model.type = to_saladmodeltype(c->type);
	}
	// else leave as is
}

void salad_set_bloomfilter_ex(salad_t* const s, BLOOM* const b)
{
	assert(s != NULL);
	salad_create_container(s);

	container_set_bloomfilter(s->model.x, b);
	s->model.type = SALAD_MODEL_BLOOMFILTER;
}
