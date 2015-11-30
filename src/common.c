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

#include "common.h"

#include <salad/util.h>
#include <util/log.h>

const char* const saladmode_to_string(saladmode_t m)
{
	switch (m)
	{
	case TRAINING: return "training";
	case PREDICT:  return "predict";
	case INSPECT:  return "inspect";
	case STATS:    return "stats";
	case TEST:      return "test";
	default: break;
	}
	return "undefined";
}

const saladmode_t to_saladmode(const char* const str)
{
	switch (cmp(str, "train", "predict", "inspect", "stats", "test", NULL))
	{
	case 0: return TRAINING;
	case 1: return PREDICT;
	case 2: return INSPECT;
	case 3: return STATS;
	case 4: return TEST;
	}
	return UNDEFINED;
}



const int salad_heart(const config_t* const c, FN_SALAD fct)
{
	assert(c != NULL);
	const data_processor_t* const dp = to_dataprocessor(c->input_type);

#ifdef USE_NETWORK
	net_param_t p = {
			(c->input_type == IOMODE_NETWORK),
			 c->pcap_filter,
			 c->net_clientcomm,
			 c->net_servercomm,
			 NULL };
#else
	io_param_t p = { NULL };
#endif

	file_t f_in;
	int ret = dp->open(&f_in, c->input, FILE_IO_READ, &p);
	if (ret != EXIT_SUCCESS)
	{
		error("Unable to open input data.");
		if (p.error_msg != NULL)
		{
			error("%s", p.error_msg);
		}
		return EXIT_FAILURE;
	}

	ret = dp->filter(&f_in, c->input_filter);
	if (ret != EXIT_SUCCESS)
	{
		error("Unable to compile regular expression for input filtering.");
		return EXIT_FAILURE;
	}

	ret = dp->meta(&f_in, c->group_input);
	if (ret != EXIT_SUCCESS)
	{
		error("Unable to read meta data.");
		return EXIT_FAILURE;
	}

	const char* const opentype = (c->update_model ? "rb+" : "wb+");

	FILE* const f_out = fopen(c->output, opentype);
	if (f_out == NULL)
	{
		error("Unable to open/ create output file.");
		if (f_in.fd != NULL)
		{
			fclose(f_in.fd);
		}
		return EXIT_FAILURE;
	}

	const int result = fct(c, dp, &f_in, f_out);

	dp->close(&f_in);
	fclose(f_out);

	return result;
}


void salad_header(const char* const msg, const metadata_t* const meta, const config_t* c)
{
	print("");
	if (meta->num_items <= 0)
	{
		status("%s network data", msg);
	}
	else
	{
		status("%s %"ZU" strings in chunks of %"ZU, (SIZE_T) msg, meta->num_items, (SIZE_T) c->batch_size);
	}

	if (c->bloom != NULL)
	{
		status("Based on %s as content model", c->bloom);
	}
	if (c->bbloom != NULL)
	{
		status("and %s as bad content model", c->bloom);
	}
}

void echo_options(config_t* const config)
{
	info("Options:");
	info(" # n-Gram length: %u", (unsigned int) config->ngram_length);

	if (config->binary_ngrams)
	{
		info("  # Use binary n-Grams");
	}

	if (config->delimiter != NULL)
	{
		info(" # Delimiter: '%s'", config->delimiter);
	}

	info(" # Filter size: %u", config->filter_size);
	info(" # Hash set: %s", hashset_to_string(config->hash_set));
}


const int salad_from_config(salad_t* const s, const config_t* const c)
{
	assert(s != NULL && c != NULL);
	salad_init(s);

	// TODO: right now there only are bloom filters!
	assert(c->filter_size <= USHRT_MAX);
	salad_set_bloomfilter_ex(s, bloom_init((unsigned short) c->filter_size, c->hash_set));
	salad_use_binary_ngrams(s, c->binary_ngrams);
	salad_set_delimiter(s, c->delimiter);
	salad_set_ngramlength(s, c->ngram_length);

	return EXIT_SUCCESS;
}

const int salad_from_file_v(const char* const id, const char* const filename, salad_t* const out)
{
	const int ret = salad_from_file(filename, out);

	switch (ret)
	{
	case EXIT_SUCCESS:
		break;

	case EXIT_FAILURE +1:
		error("Unable to open %s filter.", id);
		break;

	case EXIT_FAILURE +2:
		error("Corrupt %s filter.", id);
		break;

	case EXIT_FAILURE:
	default:
		error("Unable to read %s filter from file.", id);
		break;
	}

	return ret;
}
