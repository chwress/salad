/**
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2013, Christian Wressnegger
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

#include "salad.h"
#include "anagram.h"
#include "util/log.h"

#include <inttypes.h>


typedef struct {
	BLOOM* const bloom;
	DELIM(delim);
	const config_t* const config;
} train_t;


const int salad_train_callback1(data_t* data, const size_t n, void* usr)
{
	train_t x = *((train_t*) usr);

	for (size_t i = 0; i < n; i++)
	{
		bloomize_ex(x.bloom, data[i].buf, data[i].len, x.config->ngramLength);
	}

	progress_step();
	return EXIT_SUCCESS;
}

const int salad_train_callbackn1(data_t* data, const size_t n, void* usr)
{
	assert(n == 1);
	train_t x = *((train_t*) usr);
	bloomize_ex(x.bloom, data[0].buf, data[0].len, x.config->ngramLength);

	progress();
	return EXIT_SUCCESS;
}

const int salad_train_callback2(data_t* data, const size_t n, void* usr)
{
	train_t x = *((train_t*) usr);

	for (size_t i = 0; i < n; i++)
	{
		bloomizew_ex(x.bloom, data[i].buf, data[i].len, x.config->ngramLength, x.delim);
	}

	progress_step();
	return EXIT_SUCCESS;
}

const int salad_train_callbackn2(data_t* data, const size_t n, void* usr)
{
	assert(n != 1);
	train_t x = *((train_t*) usr);
	bloomizew_ex(x.bloom, data[0].buf, data[0].len, x.config->ngramLength, x.delim);

	progress();
	return EXIT_SUCCESS;
}

const int salad_train_stub(const config_t* const c, const data_processor_t* const dp, file_t fIn, FILE* const fOut)
{
	train_t usr = {
		bloom_init(c->filter_size, c->hash_set),
		{0}, c
	};

	const char* const d = to_delim(usr.delim, c->delimiter);
#ifdef USE_NETWORK
	if (c->input_type == NETWORK || c->input_type == NETWORK_DUMP)
	{
		if (d == NULL) dp->recv(&fIn, salad_train_callbackn1, &usr);
		else           dp->recv(&fIn, salad_train_callbackn2, &usr);
	}
	else
#endif
	{
		if (d == NULL) dp->recv(&fIn, salad_train_callback1, &usr);
		else           dp->recv(&fIn, salad_train_callback2, &usr);
	}
	print("");

	const int n = fwrite_model(fOut, usr.bloom, c->ngramLength, d);
	bloom_destroy(usr.bloom);

	return (n >= 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

const int salad_train(const config_t* const c)
{
	return salad_heart(c, salad_train_stub);
}

