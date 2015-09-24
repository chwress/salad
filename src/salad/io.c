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
#include <util/util.h>
#include <util/simple_conf.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

#ifdef USE_ARCHIVES
#include <time.h>
#include <util/archive.h>
#include <archive_entry.h>
#endif


void gen_tmpname(char* const buf, const size_t n)
{
	if (n < 1) return;
	buf[n] = 0x00;

	if (n < 2) return;
	buf[0] = '.';

	srand(time(NULL));
	rand_s(buf+1, n-2);
}

static const char* const CONFIG_HEADER = "Salad Configuration";

const int fwrite_model(FILE* const f, const salad_t* const s)
{
#ifdef USE_ARCHIVES
	return fwrite_model_zip(f, s);
#else
	return fwrite_model_txt(f, s);
#endif
}


const int fwrite_header(FILE* const f, const salad_t* const s)
{
	const int nheader = fprintf(f, "%s\n\n", CONFIG_HEADER);
	if (nheader < 0) return -1;

	const int nbinary = fprintf(f, "binary = %s\n", (s->as_binary ? "True" : "False"));
	if (nbinary < 0) return -1;

	const int ndelim = fprintf(f, "delimiter = %s\n", _(s)->delimiter.str);
	if (ndelim < 0) return -1;

	const int n = fprintf(f, "n = %"Z"\n", s->ngram_length);
	if (n < 0) return -1;

	return nheader + nbinary + ndelim + n;
}

const int fwrite_bloom(FILE* const f, BLOOM* const b)
{
	int n, m;
	if ((n = fwrite_hashspec(f, b)) < 0) return -1;
	if ((m = bloom_to_file(b, f)) < 0) return -1;
	return n +m +1;
}

const int fwrite_bloom_inline(FILE* const f, BLOOM* const b)
{
	// Since we do not want to determine the size of the data based on some
	// internals, we do something ugly instead ;)
	char buf[0x100];
	snprintf(buf, 0x100, "%d", INT_MAX);
	for (char* x = buf; *x != 0x00; x++) *x = ' ';

	size_t pos = ftell_s(f) +15;
	int nbloom = fprintf(f, "bloom_filter = %s\n", buf);
	if (nbloom < 0) return -1;

	int n = fwrite_bloom(f, b);
	if (n < 0) return -1;

	if (fprintf(f, "\n") < 0) return -1;


	fseek_s(f, pos, SEEK_SET);
	fprintf(f, "%"Z, (size_t) n);
	fseek_s(f, 0, SEEK_END);

	return n +1;
}

const int fwrite_model_txt(FILE* const f, const salad_t* const s)
{
	int n = fwrite_header(f, s);
	if (n < 0) return -1;

	int m = 0;
	// TODO: right now there only are bloom filters!
	switch (s->model.type)
	{
	case SALAD_MODEL_BLOOMFILTER:
	{
		BLOOM* b = GET_BLOOMFILTER(s->model);
		m = fwrite_bloom_inline(f, b);
		if (m < 0) return -1;
		break;
	}
	default:
		return -1;
	}

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
	char tmpname[16];
	gen_tmpname(tmpname, 16);

	// Configuration header
	static const char* const BLOOM_FNAME = "bloom.data";

	FILE* tmp = fopen(tmpname, "w+");
	if (tmp == NULL) return -1;

	int n = fwrite_header(tmp, s);
	int m = fprintf(tmp, "bloom_filter = %s\n", BLOOM_FNAME);
	fclose(tmp);
	if (n < 0 || m < 0)
	{
		remove(tmpname);
		return -1;
	}

	archive_write_file(a, tmpname, "config");
	remove(tmpname);

	// Container
	tmp = fopen(tmpname, "w+");
	if (tmp == NULL) return -1;

	// TODO: right now there only are bloom filters!
	switch (s->model.type)
	{
	case SALAD_MODEL_BLOOMFILTER:
	{
		BLOOM* b = GET_BLOOMFILTER(s->model);
		n = fwrite_bloom(tmp, b);
		break;
	}
	default:
		n = -1;
		break;
	}

	fclose(tmp);
	if (n < 0)
	{
		remove(tmpname);
		return -1;
	}

	archive_write_file(a, tmpname, BLOOM_FNAME);
	remove(tmpname);

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
	int inline_bloomfilter;
	char* bloomfilter;

} modelconf_spec_t;

#define EMPTY_MODELCONF_SPEC_INITIALIZER { \
		.s = NULL, \
		.ngramlen_specified = FALSE, \
		.inline_bloomfilter = FALSE, \
		.bloomfilter = NULL \
}

/**
 * The preferred way of initializing the salad_t object/ struct.
 */
#define MODELCONF_SPEC_T(spec) modelconf_spec_t spec = EMPTY_MODELCONF_SPEC_INITIALIZER

const int fread_header(FILE* const f, const char* const key, const char* const value, void* const usr)
{
	assert(usr != NULL);
	modelconf_spec_t* const x = (modelconf_spec_t*) usr;

	char* tail;
	switch (cmp(key, "binary", "delimiter", "n", "bloom_filter", NULL))
	{
	case 0:
	{
		long int b = strtol(value, &tail, 10);
		if (tail == value)
		{
			b = (strcasecmp(value, "True") == 0);
		}
		salad_use_binary_ngrams(x->s, b);
		break;
	}
	case 1:
	{
		salad_set_delimiter(x->s, value);
		break;
	}
	case 2:
	{
		const size_t n = (size_t) strtoul(value, &tail, 10);
		salad_set_ngramlength(x->s, n);
		x->ngramlen_specified = TRUE;
		break;
	}
	case 3:
	{
		unsigned long int len = strtoul(value, &tail, 10);
		if (value != tail)
		{
			// Let's assume it is an inline bloom specification
			// with the specified size.

			UNUSED(len); // we simply ignore the size for now
			BLOOM* b = bloom_init_from_file(f);

			x->inline_bloomfilter = (b != NULL);
			salad_set_bloomfilter_ex(x->s, b);
		}
		else
		{
			STRDUP(value, x->bloomfilter);
		}
		break;
	}
	default:
		// Unkown identifier
		return FALSE;
	}
	return TRUE;
}

const int fread_model_txt(FILE* const f, salad_t* const s)
{
	assert(f != NULL && s != NULL);

	// Set default values
	salad_use_binary_ngrams(s, FALSE);
	salad_set_delimiter(s, "");

	MODELCONF_SPEC_T(spec);
	spec.s = s;

	int n = fread_config(f, CONFIG_HEADER, fread_header, &spec);
	if (n <= 0) return n;

	// The bloom filter and the n-gram length are mandatory, though
	if (!spec.ngramlen_specified) return -3;
	if (!spec.inline_bloomfilter) return -4;

	return n;
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

	MODELCONF_SPEC_T(spec);
	spec.s = s;

	int m = fread_config(config, CONFIG_HEADER, fread_header, &spec);
	fclose(config);
	remove(tmpname);
	if (m <= 0) return m;

	// The bloom filter and the n-gram length are mandatory, though
	if (!spec.ngramlen_specified) return -3;
	if (!spec.inline_bloomfilter)
	{
		int ret = archive_read_dumpfile2(f, spec.bloomfilter, tmpname);
		free(spec.bloomfilter);
		if (ret != ARCHIVE_OK)
		{
			return -4;
		}

		FILE* bf = fopen(tmpname, "r+");
		if (bf == NULL)
		{
			remove(tmpname);
			return -5;
		}

		BLOOM* b = bloom_init_from_file(bf);
		if (b == NULL)
		{
			remove(tmpname);
			return -6;
		}

		salad_set_bloomfilter_ex(s, b);
		fclose(bf);
		remove(tmpname);
	}
	else
	{
		free(spec.bloomfilter);
	}
	return UNSIGNED_SUBSTRACTION(ftell_s(f), pos);
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
	salad_set_bloomfilter_ex(s, bloom_init_from_file(f));

	return (n <= 0 || s->ngram_length <= 0 || s->model.x == NULL ? -1 : UNSIGNED_SUBSTRACTION(ftell_s(f), pos));
}
