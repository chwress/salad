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

#include <container/io.h>
#include <container/io/bloom.h>
#include <container/io/common.h>

#include <util/util.h>
#include <util/simple_conf.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>


#ifdef USE_ARCHIVES
#include <time.h>
#include <util/archive.h>
#include <archive_entry.h>

void gen_tmpname(char* const buf, const size_t n)
{
	if (n < 1) return;
	buf[n] = 0x00;

	if (n < 2) return;
	buf[0] = '.';

	rand_s(buf+1, n-2);
}
#endif

static const char* const CONFIG_HEADER = "Salad Configuration";

const BOOL fwrite_model(FILE* const f, const salad_t* const s)
{
#ifdef USE_ARCHIVES
	return fwrite_model_zip(f, s);
#else
	return fwrite_model_txt(f, s);
#endif
}

const BOOL fwrite_modelconfig(const container_outputspec_t* const out, const salad_t* const s)
{
	assert(out != NULL);
	return (CONTAINER_TXT(out) ? fwrite_modelconfig_ex(out->config, s) : FALSE);
}

const BOOL fwrite_modelconfig_ex(FILE* const f, const salad_t* const s)
{
	int n = fprintf(f, "%s\n\n", CONFIG_HEADER);
	if (n <= 0) return FALSE;

	n = fprintf(f, "binary = %s\n", (s->as_binary ? "True" : "False"));
	if (n <= 0) return FALSE;

	n = fprintf(f, "delimiter = %s\n", _(s)->delimiter.str);
	if (n <= 0) return FALSE;

	n = fprintf(f, "n = %"ZU"\n", (SIZE_T) s->ngram_length);
	if (n <= 0) return FALSE;

	const container_t* const c = (container_t*) s->model.x;
	return fwrite_containerconfig_ex(f, c);
}

const BOOL fwrite_modeldata(const container_outputspec_t* const out, const salad_t* const s, container_outputstate_t* const state)
{
	const container_t* const c = (container_t*) s->model.x;
	return fwrite_containerdata(out, c, state);
}


const BOOL fwrite_model_txt(FILE* const f, const salad_t* const s)
{
	if (!fwrite_modelconfig_ex(f, s)) return FALSE;

	CONTAINER_OUTPUTSTATE_T(state);
	container_outputspec_t spec = {f, NULL, CONTAINER_OUTPUTFMT_MIXED};
	container_t* c = (container_t*) s->model.x;

	return fwrite_containerdata(&spec, c, &state);
}

const BOOL fwrite_model_zip(FILE* const f, const salad_t* const s)
{
#ifndef USE_ARCHIVES
	return FALSE;
#else
	// Initialize output file
	struct archive* a = archive_write_easyopen(f, ZIP);
	if (a == NULL) return -1;

	// Generate temporary output filename
	char tmpconfig[16], tmpdata[16];
	gen_tmpname(tmpconfig, 16);
	gen_tmpname(tmpdata, 16);

	// Configuration header
	FILE* config = fopen(tmpconfig, "w+");
	if (config == NULL) return -1;

	const BOOL config_ok = fwrite_modelconfig_ex(config, s);

	BOOL data_ok = FALSE;
	CONTAINER_OUTPUTSTATE_T(state);
	do
	{
		// Container data
		FILE* data = fopen(tmpdata, "w+");
		if (tmpdata == NULL)
		{
			remove(tmpconfig);
			return FALSE;
		}

		container_outputspec_t spec = {config, data, CONTAINER_OUTPUTFMT_SEPARATED};
		data_ok = fwrite_modeldata(&spec, s, &state);

		fclose(data);
		if (!data_ok) break;

		archive_write_file(a, tmpdata, state.filename);
		remove(tmpdata);

	} while (!state.done);

	fclose(config);
	if (!config_ok || !config_ok)
	{
		remove(tmpconfig);
		remove(tmpdata);
		return FALSE;
	}

	archive_write_file(a, tmpconfig, "config");
	remove(tmpconfig);

	archive_write_easyclose(a);
	return TRUE;
#endif
}


const BOOL fread_model(FILE* const f, salad_t* const s)
{
	if (fread_model_zip(f, s)) return TRUE;

	// not stored as archive?
	if (fread_model_txt(f, s)) return TRUE;

	// seems to be in old format then
	s->as_binary = FALSE;
	return fread_model_032(f, s);
}


typedef struct
{
	salad_t* s;
	int ngramlen_specified;

	int has_container;
	container_t container;
} modelconf_spec_t;


#define EMPTY_MODELCONF_SPEC_INITIALIZER { \
		.s = NULL, \
		.ngramlen_specified = 0, \
		.has_container = 0, \
		.container = EMPTY_CONTAINER \
}

#define MODELCONF_SPEC_T(spec) modelconf_spec_t spec = EMPTY_MODELCONF_SPEC_INITIALIZER


const BOOL fread_modelconfig(FILE* const f, const char* const key, const char* const value, void* const usr)
{
	assert(usr != NULL);
	container_iodata_t* const x = (container_iodata_t*) usr;
	modelconf_spec_t* const conf = (modelconf_spec_t*) x->data;

	char* tail;
	switch (cmp(key, "binary", "delimiter", "n", NULL))
	{
	case 0:
	{
		int b = (strtol(value, &tail, 10) >= 1);
		if (tail == value)
		{
			b = (stricmp(value, "True") == 0);
		}
		salad_use_binary_ngrams(conf->s, b);
		break;
	}
	case 1:
	{
		salad_set_delimiter(conf->s, value);
		break;
	}
	case 2:
	{
		const size_t n = (size_t) strtoul(value, &tail, 10);
		salad_set_ngramlength(conf->s, n);
		conf->ngramlen_specified = TRUE;
		break;
	}
	default:
	{
		// Unknown identifier
		container_iodata_t state = {&conf->container, x->request_file, x->host};
		if (fread_containerconfig(f, key, value, &state))
		{
			salad_set_container(conf->s, &conf->container);
			return TRUE;
		}
		return FALSE;
	}}
	return TRUE;
}

const BOOL fread_model_txt(FILE* const f, salad_t* const s)
{
	assert(f != NULL && s != NULL);

	// Set default values
	salad_use_binary_ngrams(s, FALSE);
	salad_set_delimiter(s, "");

	MODELCONF_SPEC_T(conf);
	conf.s = s;

	container_iodata_t state = {&conf, NULL, NULL};
	const size_t n = fread_config(f, CONFIG_HEADER, fread_modelconfig, &state);
	if (n <= 0) return FALSE;

	// The bloom filter and the n-gram length are mandatory, though
	if (!conf.ngramlen_specified) return FALSE;

	return TRUE;
}

#ifdef USE_ARCHIVES
typedef struct {
	FILE* archive;
	size_t nread;
} requested_input_t;

FILE* const request_input(const char* const filename, void* const host)
{
	requested_input_t* const x = (requested_input_t*) host;

	char tmpname[16];
	gen_tmpname(tmpname, 16);

	int ret = archive_read_dumpfile2(x->archive, filename, tmpname);
	if (ret != ARCHIVE_OK)
	{
		return NULL;
	}

	FILE* const f = fopen(tmpname, "r+");

	// Record number of bytes read
	fseek_s(f, 0, SEEK_END);
	x->nread += ftell_s(f);
	fseek_s(f, 0, SEEK_SET);

	// Early delete.
	// TODO: I hope this works on Windows as well ò_O
	remove(tmpname);

	return f;
}
#endif

const BOOL fread_model_zip(FILE* const f, salad_t* const s)
{
#ifndef USE_ARCHIVES
	return FALSE;
#else
	const size_t pos = ftell_s(f);

	uint16_t magic;
	const size_t n = fread(&magic, sizeof(uint16_t), 1, f);
	fseek_s(f, pos, SEEK_SET);
	if (n != 1) return FALSE;

	// Is ZIP archive?
	if (magic != 0x4B50) // PK
	{
		return FALSE;
	}

	// Generate temporary output filename
	char tmpname[16];
	gen_tmpname(tmpname, 16);

	// Initialize output file
	if (archive_read_dumpfile2(f, "config", tmpname) != ARCHIVE_OK)
	{
		return FALSE;
	}
	fseek_s(f, 0, SEEK_SET);

	FILE* config = fopen(tmpname, "r");
	if (config == NULL)
	{
		remove(tmpname);
		return FALSE;
	}


	// Set default values
	salad_use_binary_ngrams(s, FALSE);
	salad_set_delimiter(s, "");

	MODELCONF_SPEC_T(conf);
	conf.s = s;

	requested_input_t x = {f, 0};
	container_iodata_t state = {&conf, request_input, &x};
	const size_t m = fread_config(config, CONFIG_HEADER, fread_modelconfig, &state);

	fclose(config); remove(tmpname);
	if (m <= 0) return FALSE;

	// The bloom filter and the n-gram length are mandatory, though
	if (!conf.ngramlen_specified) return FALSE;

	return TRUE;
#endif
}


const BOOL fread_model_032(FILE* const f, salad_t* const s)
{
	assert(f != NULL && s != NULL);

	// Set default values
	salad_use_binary_ngrams(s, FALSE);
	salad_set_delimiter(s, "");

	char* const delimiter = fread_str(f);
	if (delimiter != NULL)
	{
		salad_set_delimiter(s, delimiter);
		free(delimiter);
	}

	const size_t n = fread(&(s->ngram_length), sizeof(size_t), 1, f);

	// The actual bloom filter values
	BLOOM* b; fread_bloom(f, &b);
	salad_set_bloomfilter_ex(s, b);

	return (n > 0 && s->ngram_length > 0 && s->model.x != NULL);
}
