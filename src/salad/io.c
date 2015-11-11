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

const int fwrite_model(FILE* const f, const salad_t* const s)
{
#ifdef USE_ARCHIVES
	return fwrite_model_zip(f, s);
#else
	return fwrite_model_txt(f, s);
#endif
}

const int fwrite_modelconfig(const container_outputspec_t* const out, const salad_t* const s)
{
	assert(out != NULL);
	return (fwrite_modelconfig_ex(out->config, s) ? CONTAINER_TXT(out) : -1);
}

const int fwrite_modelconfig_ex(FILE* const f, const salad_t* const s)
{
	const int nheader = fprintf(f, "%s\n\n", CONFIG_HEADER);
	if (nheader < 0) return -1;

	const int nbinary = fprintf(f, "binary = %s\n", (s->as_binary ? "True" : "False"));
	if (nbinary < 0) return -1;

	const int ndelim = fprintf(f, "delimiter = %s\n", _(s)->delimiter.str);
	if (ndelim < 0) return -1;

	const int n = fprintf(f, "n = %"Z"\n", s->ngram_length);
	if (n < 0) return -1;

	const container_t* const c = (container_t*) s->model.x;
	const int m = fwrite_containerconfig_ex(f, c);
	if (m < 0) return -1;

	return nheader + nbinary + ndelim + n + m;
}

const int fwrite_modeldata(const container_outputspec_t* const out, const salad_t* const s, container_outputstate_t* const state)
{
	const container_t* const c = (container_t*) s->model.x;
	return fwrite_containerdata(out, c, state);
}


const int fwrite_model_txt(FILE* const f, const salad_t* const s)
{
	int n = fwrite_modelconfig_ex(f, s);
	if (n < 0) return -1;

	CONTAINER_OUTPUTSTATE_T(state);
	container_outputspec_t spec = {f, NULL, CONTAINER_OUTPUTFMT_MIXED};
	container_t* c = (container_t*) s->model.x;

	const int m = fwrite_containerdata(&spec, c, &state);
	if (m < 0) return -1;

	return n +m;
}

const int fwrite_model_zip(FILE* const f, const salad_t* const s)
{
#ifdef USE_ARCHIVES
	const size_t pos = ftell_s(f);

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

	const int n = fwrite_modelconfig_ex(config, s);

	int m = -1, M = 0;
	CONTAINER_OUTPUTSTATE_T(state);
	do
	{
		// Container data
		FILE* data = fopen(tmpdata, "w+");
		if (tmpdata == NULL)
		{
			remove(tmpconfig);
			return -1;
		}

		container_outputspec_t spec = {config, data, CONTAINER_OUTPUTFMT_SEPARATED};
		m = fwrite_modeldata(&spec, s, &state);
		M += m;

		fclose(data);
		if (m < 0) break;

		archive_write_file(a, tmpdata, state.filename);
		remove(tmpdata);

	} while (!state.done);

	fclose(config);
	if (n < 0 || m < 0)
	{
		remove(tmpconfig);
		remove(tmpdata);
		return -1;
	}

	archive_write_file(a, tmpconfig, "config");
	remove(tmpconfig);

	const size_t size = UNSIGNED_SUBSTRACTION(ftell_s(f), pos);
	archive_write_easyclose(a);
	return size;
#else
	return 0;
#endif
}


const int fread_model(FILE* const f, salad_t* const s)
{
	int ret = fread_model_zip(f, s);
	if (ret == 0) // not stored as archive
	{
		ret = fread_model_txt(f, s);
	}
	if (ret == 0) // seems to be in old format then
	{
		s->as_binary = FALSE;
		ret = fread_model_032(f, s);
	}
	return ret;
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


const int fread_modelconfig(FILE* const f, const char* const key, const char* const value, void* const usr)
{
	assert(usr != NULL);
	container_iodata_t* const x = (container_iodata_t*) usr;
	modelconf_spec_t* const conf = (modelconf_spec_t*) x->data;

	char* tail;
	switch (cmp(key, "binary", "delimiter", "n", NULL))
	{
	case 0:
	{
		long int b = strtol(value, &tail, 10);
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

const int fread_model_txt(FILE* const f, salad_t* const s)
{
	assert(f != NULL && s != NULL);

	// Set default values
	salad_use_binary_ngrams(s, FALSE);
	salad_set_delimiter(s, "");

	MODELCONF_SPEC_T(conf);
	conf.s = s;

	container_iodata_t state = {&conf, NULL, NULL};
	const int n = fread_config(f, CONFIG_HEADER, fread_modelconfig, &state);
	if (n <= 0) return n;

	// The bloom filter and the n-gram length are mandatory, though
	if (!conf.ngramlen_specified) return -3;

	return n;
}

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

const int fread_model_zip(FILE* const f, salad_t* const s)
{
#ifdef USE_ARCHIVES
	const size_t pos = ftell_s(f);

	uint16_t magic;
	size_t n = fread(&magic, sizeof(uint16_t), 1, f);
	fseek_s(f, pos, SEEK_SET);
	if (n <= 0) return -1;

	// Is ZIP archive?
	if (magic != 0x4B50) // PK
	{
		return 0;
	}

	// Generate temporary output filename
	char tmpname[16];
	gen_tmpname(tmpname, 16);

	// Initialize output file
	if (archive_read_dumpfile2(f, "config", tmpname) != ARCHIVE_OK)
	{
		return -1;
	}
	fseek_s(f, 0, SEEK_SET);

	FILE* config = fopen(tmpname, "r");
	if (config == NULL)
	{
		remove(tmpname);
		return -2;
	}


	// Set default values
	salad_use_binary_ngrams(s, FALSE);
	salad_set_delimiter(s, "");

	MODELCONF_SPEC_T(conf);
	conf.s = s;

	requested_input_t x = {f, 0};
	container_iodata_t state = {&conf, request_input, &x};
	const int m = fread_config(config, CONFIG_HEADER, fread_modelconfig, &state);

	fclose(config); remove(tmpname);
	if (m <= 0) return m;

	// The bloom filter and the n-gram length are mandatory, though
	if (!conf.ngramlen_specified) return -3;

	return x.nread + m;
#else
	return 0;
#endif
}


const int fread_model_032(FILE* const f, salad_t* const s)
{
	assert(f != NULL && s != NULL);
	const size_t pos = ftell(f);

	// Set default values
	salad_use_binary_ngrams(s, FALSE);
	salad_set_delimiter(s, "");

	char* const delimiter = fread_str(f);
	if (delimiter != NULL)
	{
		salad_set_delimiter(s, delimiter);
		free(delimiter);
	}

	size_t n = fread(&(s->ngram_length), sizeof(size_t), 1, f);

	// The actual bloom filter values
	BLOOM* b; fread_bloom(f, &b);
	salad_set_bloomfilter_ex(s, b);

	return (n <= 0 || s->ngram_length <= 0 || s->model.x == NULL ? -1 : UNSIGNED_SUBSTRACTION(ftell_s(f), pos));
}
