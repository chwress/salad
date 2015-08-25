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

#include "main.h"

#include <salad/salad.h>
#include <salad/analyze.h>
#include <salad/classify.h>
#include <salad/util.h>

#include <util/log.h>
#include <util/io.h>

#include <inttypes.h>


typedef struct {
	BLOOM* const bloom;
	delimiter_t* const delim;
	const config_t* const config;
} train_t;


#define TRAINING_CALLBACK(X, _data_, _n_, _usr_)                                                        \
static inline const int salad_train_callback##X(data_t* data, const size_t n, void* usr)                \
{	                                                                                                    \
	salad_t* const s = (salad_t*) usr;                                                                  \
	                                                                                                    \
	for (size_t i = 0; i < n; i++)                                                                      \
	{                                                                                                   \
		switch (#X[0]) /* This is a static check and will be optimized away */                          \
		{                                                                                               \
		case 'b':                                                                                       \
			bloomizeb_ex(s->model.x, data[i].buf, data[i].len, s->ngram_length);                        \
			break;                                                                                      \
		case 'w':                                                                                       \
			bloomizew_ex(s->model.x, data[i].buf, data[i].len, s->ngram_length, _(s)->delimiter.d);     \
			break;                                                                                      \
		default:                                                                                        \
			bloomize_ex (s->model.x, data[i].buf, data[i].len, s->ngram_length);                        \
			break;                                                                                      \
		}                                                                                               \
	}                                                                                                   \
	return EXIT_SUCCESS;                                                                                \
}

#define TRAINING_NET_CALLBACK(X, _data_, _n_, _usr_)                                                    \
static inline const int salad_train_net_callback##X(data_t* data, const size_t n, void* usr)            \
{	                                                                                                    \
	assert(n == 1);                                                                                     \
	salad_t* const s = (salad_t*) usr;                                                                  \
	switch (#X[0]) /* This is a static check and will be optimized away */                              \
	{                                                                                                   \
	case 'b':                                                                                           \
		bloomizeb_ex(s->model.x, data[0].buf, data[0].len, s->ngram_length);                            \
		break;                                                                                          \
	case 'w':                                                                                           \
		bloomizew_ex(s->model.x, data[0].buf, data[0].len, s->ngram_length, _(s)->delimiter.d);         \
		break;                                                                                          \
	default:                                                                                            \
		bloomize_ex (s->model.x, data[0].buf, data[0].len, s->ngram_length);                            \
		break;                                                                                          \
	}                                                                                                   \
	return EXIT_SUCCESS;                                                                                \
}

TRAINING_CALLBACK(b, data, n, usr)
TRAINING_NET_CALLBACK(b, data, n, usr)

TRAINING_CALLBACK(, data, n, usr)
TRAINING_NET_CALLBACK(, data, n, usr)

TRAINING_CALLBACK(w, data, n, usr)
TRAINING_NET_CALLBACK(w, data, n, usr)


FN_DATA pick_callback(const model_type_t t, const int use_network)
{
	switch (t)
	{
	case BIT_NGRAM:
		return (use_network ? salad_train_net_callbackb : salad_train_callbackb);

	case BYTE_NGRAM:
		return (use_network ? salad_train_net_callback  : salad_train_callback );

	case TOKEN_NGRAM:
		return (use_network ? salad_train_net_callbackw : salad_train_callbackw);
	}
	return NULL;
}

const int salad_train_stub(const config_t* const c, const data_processor_t* const dp, file_t* const f_in, FILE* const f_out)
{
	salad_header("Train salad on", &f_in->meta, c);

	SALAD_T(s1);
	salad_from_config(&s1, c);

	if (c->update_model)
	{
		SALAD_T(s2);

		if (salad_from_file_ex(f_out, &s2) == EXIT_FAILURE)
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
		fseek(f_out, 0, SEEK_SET);

		if (c->echo_params)
		{
			// TODO: cf. salad_predict
		}
	}

	const model_type_t t = to_model_type(s1.as_binary, __(s1).use_tokens);

#ifdef USE_NETWORK
	if (c->input_type == NETWORK || c->input_type == NETWORK_DUMP)
	{
		dp->recv(f_in, pick_callback(t, 1), c->batch_size, &s1);
	}
	else
#endif
	{
		dp->recv(f_in, pick_callback(t, 0), c->batch_size, &s1);
	}

	const int ret = salad_to_file_ex(&s1, f_out);
	salad_destroy(&s1);

	return ret;
}

const int _salad_train_(const config_t* const c)
{
	return salad_heart(c, salad_train_stub);
}

