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

#include "io.h"
#include "io/bloom.h"
#include "io/common.h"
#include "bloom.h"

#include <stdlib.h>
#include <assert.h>

#include <util/util.h>
#include <util/simple_conf.h>


const int fwrite_container(FILE* const f, const container_t* const c)
{
	return fwrite_container_ex(f, c, CONTAINER_OUTPUTFMT_TXT);
}

const int fwrite_container_ex(FILE* const f, const container_t* const c, const container_outputformat_t fmt)
{
	assert(f != NULL);
	assert(c != NULL);

	container_outputspec_t spec = {f, NULL, fmt};
	if (!fwrite_containerconfig(&spec, c)) return FALSE;

	CONTAINER_OUTPUTSTATE_T(state);
	if (!fwrite_containerdata(&spec, c, &state)) return FALSE;

	return TRUE;
}

const BOOL fwrite_containerconfig(const container_outputspec_t* const out, const container_t* const c)
{
	assert(out != NULL);
	return (CONTAINER_TXT(out) ? fwrite_containerconfig_ex(out->config, c) : FALSE);
}

const BOOL fwrite_containerconfig_ex(FILE* const f, const container_t* const c)
{
	assert(f != NULL);
	assert(c != NULL);

	const int n = fprintf(f, "container = %s\n", container_to_string(c->type));
	if (n <= 0) return FALSE;

	switch (c->type)
	{
	case CONTAINER_BLOOMFILTER:
		return fwrite_bloomconfig_ex(f, c->data);
	default:
		return FALSE;
	}
}

const BOOL fwrite_containerdata(const container_outputspec_t* const out, const container_t* c, container_outputstate_t* const state)
{
	assert(out != NULL);
	assert(c != NULL);

	return fwrite_bloomdata(out, c->data, state);
}



const BOOL fread_containerconfig(FILE* const f, const char* const key, const char* const value, void* const usr)
{
	assert(usr != NULL);
	container_iodata_t* const x = (container_iodata_t*) usr;
	container_t* const container = (container_t*) x->data;

	switch (cmp(key, "container", NULL))
	{
	case 0:
	{
		switch (cmp(value, "bloom-filter", NULL))
		{
		case 0:
			container->type = CONTAINER_BLOOMFILTER;
			break;
		default:
			container->type = CONTAINER_ERROR;
			break;
		}
		break;
	}
	default:
		// Unknown identifier
		if (container->type == CONTAINER_BLOOMFILTER)
		{
			container_iodata_t state = {container, x->request_file, x->host};
			return fread_bloomconfig(f, key, value, &state);
		}
		return FALSE;
	}
	return TRUE;
}


const BOOL fread_container(FILE* const f, container_t* const c)
{
	assert(f != NULL && c != NULL);
	const size_t pos = ftell_s(f);

	container_iodata_t state = {c, NULL, NULL};
	const size_t ret = fread_config(f, NULL, fread_containerconfig, &state);
	if (ret > 0) return TRUE;

	fseek_s(f, pos, SEEK_SET);
	return FALSE;
}
