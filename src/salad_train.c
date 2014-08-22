/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2014, Christian Wressnegger
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

#include "main.h"

#include <salad/salad.h>
#include <salad/anagram.h>
#include <salad/util.h>

#include <util/log.h>

#include <inttypes.h>


typedef struct {
	BLOOM* const bloom;
	delimiter_t* const delim;
	const config_t* const config;
} train_t;


const int salad_train_callback1(data_t* data, const size_t n, void* usr)
{
	salad_t* const s = (salad_t*) usr;

	for (size_t i = 0; i < n; i++)
	{
		bloomize_ex(s->model.x, data[i].buf, data[i].len, s->ngramLength);
	}
	return EXIT_SUCCESS;
}

const int salad_train_callbackn1(data_t* data, const size_t n, void* usr)
{
	assert(n == 1);
	salad_t* const s = (salad_t*) usr;
	bloomize_ex(s->model.x, data[0].buf, data[0].len, s->ngramLength);
	return EXIT_SUCCESS;
}

const int salad_train_callback2(data_t* data, const size_t n, void* usr)
{
	salad_t* const s = (salad_t*) usr;

	for (size_t i = 0; i < n; i++)
	{
		bloomizew_ex(s->model.x, data[i].buf, data[i].len, s->ngramLength, s->delimiter.d);
	}
	return EXIT_SUCCESS;
}

const int salad_train_callbackn2(data_t* data, const size_t n, void* usr)
{
	assert(n == 1);
	salad_t* const s = (salad_t*) usr;
	bloomizew_ex(s->model.x, data[0].buf, data[0].len, s->ngramLength, s->delimiter.d);
	return EXIT_SUCCESS;
}

const int salad_train_stub(const config_t* const c, const data_processor_t* const dp, file_t* const fIn, FILE* const fOut)
{
	salad_header("Train salad on", &fIn->meta, c);

	SALAD_T(s1);
	salad_from_config(&s1, c);

	if (c->update_model)
	{
		SALAD_T(s2);

		if (salad_from_file_ex(fOut, &s2) == EXIT_FAILURE)
		{
			error("The provided model cannot be read, hence not updated.");
			return EXIT_FAILURE;
		}
		else
		{
			if (!c->transfer_spec && salad_spec_diff(&s1, &s2))
			{
				error("The specification of the existing model contradicts with the current one.");
				return EXIT_FAILURE;
			}

			salad_destroy(&s1);
			s1 = s2;
		}
		fseek(fOut, 0, SEEK_SET);

		if (c->echo_params)
		{
			// TODO: cf. salad_predict
		}
	}

#ifdef USE_NETWORK
	if (c->input_type == NETWORK || c->input_type == NETWORK_DUMP)
	{
		if (s1.useWGrams) dp->recv(fIn, salad_train_callbackn2, c->batch_size, &s1);
		else              dp->recv(fIn, salad_train_callbackn1, c->batch_size, &s1);
	}
	else
#endif
	{
		if (s1.useWGrams) dp->recv(fIn, salad_train_callback2, c->batch_size, &s1);
		else              dp->recv(fIn, salad_train_callback1, c->batch_size, &s1);
	}

	const int ret = salad_to_file_ex(&s1, fOut);
	salad_destroy(&s1);

	return ret;
}

const int _salad_train_(const config_t* const c)
{
	return salad_heart(c, salad_train_stub);
}

