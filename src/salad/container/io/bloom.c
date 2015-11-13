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

#include "bloom.h"

#include "common.h"
#include "../bloom.h"

#include <util/util.h>
#include <util/simple_conf.h>

#include <assert.h>
#include <ctype.h>


const BOOL fwrite_bloomconfig(const container_outputspec_t* const out, const BLOOM* const b)
{
	assert(out != NULL);
	return (CONTAINER_TXT(out) ? fwrite_bloomconfig_ex(out->config, b) : FALSE);
}

const BOOL fwrite_bloomconfig_ex(FILE* const f, const BLOOM* const b)
{
	assert(f != NULL);
	assert(b != NULL);

	const int n = fprintf(f, "hashes = %s", (b->nfuncs <= 0 ? "" : to_hashname(b->funcs[0])));
	if (n <= 0) return FALSE;

	for (size_t i = 1; i < b->nfuncs; i++)
	{
		const int n = fprintf(f, ",%s", to_hashname(b->funcs[i]));
		if (n <= 0) return FALSE;
	}

	return (fprintf(f, "\n") == 1);
}


#define INLINE_MARKER "<inline>"

const int process_next(container_outputstate_t* const state, const char* const filename)
{
	// There is one file only
	if (state != NULL)
	{
		if (state->count <= 0)
		{
			state->count = 1;
			state->filename = filename;
			state->done = TRUE;
		}
		else
		{
			state->filename = NULL;
			state->done = TRUE;
			return FALSE;
		}
	}
	return TRUE;
}

const BOOL fwrite_bloomdata_asis(FILE* const f, const BLOOM* const b, container_outputstate_t* const state);
const BOOL fwrite_bloomdata_ext(FILE* const config, FILE* const data, const BLOOM* const b, container_outputstate_t* const state);
const BOOL fwrite_bloomdata_inline(FILE* const f, const BLOOM* const b, container_outputstate_t* const state);
const BOOL fwrite_bloomdata_txt(FILE* const f, const BLOOM* const b, container_outputstate_t* const state);

const BOOL fwrite_bloomdata(const container_outputspec_t* const out, const BLOOM* const b, container_outputstate_t* const state)
{
	assert(out != NULL);
	assert(out->type != CONTAINER_OUTPUTFMT_BINARY); // DONT DO THIS!

	switch (out->type)
	{
	case CONTAINER_OUTPUTFMT_BINARY:
		return fwrite_bloomdata_asis(out->config, b, state);
	case CONTAINER_OUTPUTFMT_TXT:
		return fwrite_bloomdata_txt(out->config, b, state);
	case CONTAINER_OUTPUTFMT_MIXED:
		return fwrite_bloomdata_inline(out->config, b, state);
	case CONTAINER_OUTPUTFMT_SEPARATED:
		return fwrite_bloomdata_ext(out->config, out->data, b, state);
	default:
		return FALSE;
	}
}

const BOOL fwrite_bloomdata_asis(FILE* const f, const BLOOM* const b, container_outputstate_t* const state)
{
	assert(f != NULL);
	assert(b != NULL);

	if (!process_next(state, INLINE_MARKER)) return TRUE;

	const size_t n = fwrite(b->a, sizeof(char), b->size, f);
	return (n == b->size);
}

const BOOL fwrite_bloomdata_ext(FILE* const config, FILE* const data, const BLOOM* const b, container_outputstate_t* const state)
{
	assert(config != NULL);
	assert(data != NULL);
	assert(b != NULL);

	static const char* const BLOOM_FNAME = "bloom.data";
	if (!process_next(state, BLOOM_FNAME)) return 0;

	const int n = fprintf(config, "data = %s/%"Z"\n", BLOOM_FNAME, b->bitsize);
	if (n <= 0) return FALSE;

	return fwrite_bloomdata_asis(data, b, NULL);
}


const BOOL fwrite_bloomdata_inline(FILE* const f, const BLOOM* const b, container_outputstate_t* const state)
{
	assert(f != NULL);
	assert(b != NULL);

	if (!process_next(state, INLINE_MARKER)) return TRUE;

	// Since we do not want to determine the size of the data based on some
	// internals, we do something ugly instead ;)
	const int n = fprintf(f, "data = %"Z"raw\n", b->bitsize);
	if (n <= 0) return FALSE;

	if (!fwrite_bloomdata_asis(f, b, NULL)) return FALSE;

	return (fprintf(f, "\n") == 1);
}


const BOOL fwrite_bloomdata_txt(FILE* const f, const BLOOM* const b, container_outputstate_t* const state)
{
	assert(f != NULL);
	assert(b != NULL);

	if (!process_next(state, INLINE_MARKER)) return 0;

	// Since we do not want to determine the size of the data based on some
	// internals, we do something ugly instead ;)
	char buf[0x100];
	snprintf(buf, 0x100, "%d", INT_MAX);
	for (char* x = buf; *x != 0x00; x++) *x = ' ';

	const int n = fprintf(f, "data = %"Z"\n", b->bitsize);
	if (n <= 0) return FALSE;

	for (size_t i = 0; i < b->size;)
	{
		for (size_t j = 0; j < 16 && i < b->size; j++)
		{
			const int n = fprintf(f, "%02x", b->a[i++]);
			if (n <= 0) return FALSE;
		}
		if (fprintf(f, "\n") != 1) return FALSE;
	}
	return TRUE;
}


const BOOL fwrite_hashspec(FILE* const f, const BLOOM* const b)
{
	assert(f != NULL);

	if (fwrite(&b->nfuncs, sizeof(uint8_t), 1, f) != 1) return FALSE;

	for (uint8_t i = 0; i < b->nfuncs; i++)
	{
		const int id = to_hashid(b->funcs[i]);
		if (id < 0 || id >= 256) return FALSE;

		const uint8_t hid = (uint8_t) id;
		if (fwrite(&hid, sizeof(uint8_t), 1, f) != 1) return FALSE;
	}

	return TRUE;
}


const BOOL fwrite_bloom(FILE* const f, const BLOOM* const b)
{
	return fwrite_bloom_ex(f, b, CONTAINER_OUTPUTFMT_TXT);
}

const BOOL fwrite_bloom_ex(FILE* const f, const BLOOM* const b, const container_outputformat_t fmt)
{
	container_outputspec_t spec = {f, NULL, fmt};
	if (!CONTAINER_TXT(&spec) || !fwrite_bloomconfig_ex(f, b))
	{
		return FALSE;
	}

	CONTAINER_OUTPUTSTATE_T(state);
	return fwrite_bloomdata(&spec, b, &state);
}


const BOOL fwrite_bloom_032(FILE* const f, const BLOOM* const b)
{
	return (fwrite_hashspec(f, b) && bloom_to_file(b, f) < 0);
}


static inline const int read_hexbyte(void* usr)
{
	FILE* f = (FILE*) usr;
	assert(f != NULL);

	int b1 = 0, b2;
	while ((b1 = fgetc(f)) != EOF && !isxdigit(b1));
	while ((b2 = fgetc(f)) != EOF && !isxdigit(b1));

	if (b2 < 0) return EOF;

	char buf[3] = {(char) b1, (char) b2, 0};

	unsigned int ch;
	sscanf(buf, "%x", &ch);

	return (int) ch;
}

static inline const int read_byte(void* usr)
{
	FILE* f = (FILE*) usr;
	assert(f != NULL);

	return fgetc(f);
}

const BOOL fread_bloomconfig(FILE* const f, const char* const key, const char* const value, void* const usr)
{
	assert(usr != NULL);
	container_iodata_t* const x = (container_iodata_t*) usr;
	container_t* const container = (container_t*) x->data;

	if (container->data == NULL)
	{
		BLOOM* const b = bloom_create(0);
		container_set_bloomfilter(container, b);
	}

	char* tail;
	switch (cmp(key, "hashes", "data", NULL))
	{
	case 0:
	{
		char* buf;
		STRDUP(value, buf);

		size_t n = 1;
		char* needle = buf;
		for (; *needle != '\0'; needle++) if (*needle == ',') n++;

		ASSERT(n <= UINT8_MAX);
		hashfunc_t* funcs = (hashfunc_t*) calloc(n, sizeof(hashfunc_t));

		size_t i = 0;
		char* prev = buf;
		while ((needle = strchr(prev, ',')) != NULL)
		{
			*needle = '\0';
			funcs[i++] = to_hashfunc(prev);
			prev = needle +1;
		}
		funcs[i] = to_hashfunc(prev);

		bloom_set_hashfuncs_ex(container->data, funcs, (uint8_t) n);
		free(funcs); free(buf);
		break;
	}
	case 1:
	{
		size_t size = strtoul(value, &tail, 10);
		int has_validsize = (value != tail && (*tail == '\0' || strcmp(tail, "raw") == 0));

		if (has_validsize)
		{
			// We assume it is an inline bloom specification
			// with the specified size.

			BLOOM* const b = (BLOOM*) container->data;
			FN_READBYTE read = (*tail == 'r' ? read_byte : read_hexbyte);
			if (!bloom_set_ex(b, read, size, f)) return FALSE;

			break;
		}

		const char* const filename = tail;
		if (x->request_file == NULL || x->host == NULL)
		{
			// Illegal configuration or programming error
			return FALSE;
		}

		char* const slash = strrchr(tail, '/');
		if (slash != NULL)
		{
			size = strtoul(slash +1, &tail, 10);
			has_validsize = (tail != slash +1 && *tail == '\0');
			if (has_validsize)
			{
				// all read
				*slash = '\0';
			}
		}

		FILE* const f = x->request_file(filename, x->host);
		if (f == NULL) return FALSE;

		if (!has_validsize)
		{
			// No size specification let's use the file size and
			// assume the bit size equals the number of bytes *8
			fseek_s(f, 0, SEEK_END);
			size = ftell_s(f) * 8;
			fseek_s(f, 0, SEEK_SET);

			has_validsize = TRUE;
		}


		BLOOM* const b = (BLOOM*) container->data;
		if (!bloom_set_ex(b, read_byte, size, f)) return FALSE;
		fclose(f);

		break;
	}
	default:
		// Unknown identifier
		return FALSE;
	}
	return TRUE;
}

const BOOL fread_hashspec(FILE* const f, hashfunc_t** const hashfuncs, uint8_t* const nfuncs)
{
	*nfuncs = 0;
	uint8_t fctid = 0xFF;

	const size_t nread = fread(nfuncs, sizeof(uint8_t), 1, f);
	if (nread != 1) return FALSE;

	*hashfuncs = (hashfunc_t*) calloc(*nfuncs, sizeof(hashfunc_t));
	if (*hashfuncs == NULL) return FALSE;

	for (uint8_t i = 0; i < *nfuncs; i++)
	{
		if (fread(&fctid, sizeof(uint8_t), 1, f) != 1 || fctid >= NUM_HASHFCTS)
		{
			free(*hashfuncs);
			*hashfuncs = NULL;
			return FALSE;
		}
		(*hashfuncs)[i] = HASH_FCTS[fctid];
	}
	return TRUE;
}

const BOOL fread_bloom_txt(FILE* const f, BLOOM** b);

const BOOL fread_bloom(FILE* const f, BLOOM** b)
{
	if (fread_bloom_txt(f, b)) return TRUE;
	if (fread_bloom_032(f, b)) return TRUE;

	return FALSE;
}

const BOOL fread_bloom_txt(FILE* const f, BLOOM** b)
{
	assert(f != NULL && b != NULL);
	const size_t pos = ftell_s(f);

	CONTAINER_T(c);
	container_iodata_t state = {&c, NULL, NULL};
	const size_t ret = fread_config(f, NULL, fread_bloomconfig, &state);
	*b = c.data;

	if (ret > 0) return TRUE;

	fseek_s(f, pos, SEEK_SET);
	return FALSE;
}

const BOOL fread_bloom_032(FILE* const f, BLOOM** b)
{
	assert(f != NULL);
	assert(b != NULL);
	*b = NULL;

	uint8_t nfuncs;
	hashfunc_t* hashfuncs;

	if (!fread_hashspec(f, &hashfuncs, &nfuncs)) return FALSE;

	size_t asize;
	if (fread(&asize, sizeof(size_t), 1, f) != 1)
	{
		free(hashfuncs);
		return FALSE;
	}

	*b = bloom_create(asize);
	bloom_set_hashfuncs_ex(*b, hashfuncs, nfuncs);
	free(hashfuncs);

	if (*b == NULL) return FALSE;

	const size_t numRead = fread((*b)->a, sizeof(char), (*b)->size, f);

	if (numRead != (*b)->size)
	{
		bloom_destroy(*b);
		return FALSE;
	}
	return TRUE;
}
