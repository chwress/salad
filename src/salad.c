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

#include "util/vec.h"
#include "util/io.h"
#include "anagram.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "salad.h"

#include "util/log.h"


#define MAIN_OPTION_STR "i:f:b:w:o:t:n:d:r:vh"

static struct option main_longopts[] = {
	// Generic options
	{ "version",        no_argument, NULL, 'v' },
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


#define TRAIN_OPTION_STR "i:f:p:o:n:d:s:h"
#define OPTION_HASHSET 1000

static struct option train_longopts[] = {
	// I/O options
	{ "input",          required_argument, NULL, 'i' },
	{ "input-format",   required_argument, NULL, 'f' },
	{ "pcap-filter",    required_argument, NULL, 'p' },
	{ "output",         required_argument, NULL, 'o' },

	// Feature options
	{ "ngram-length",   required_argument, NULL, 'n' },
	{ "ngram-delim",    required_argument, NULL, 'd' },
	{ "filter-size",    required_argument, NULL, 's' },
	{ "hash-set",       required_argument, NULL, OPTION_HASHSET},

	// Generic options
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


#define PREDICT_OPTION_STR "i:f:p:o:b:r:h"
#define OPTION_BBLOOM 1000

static struct option predict_longopts[] = {
	// I/O options
	{ "input",          required_argument, NULL, 'i' },
	{ "input-format",   required_argument, NULL, 'f' },
	{ "pcap-filter",    required_argument, NULL, 'p' },
	{ "output",         required_argument, NULL, 'o' },

	{ "bloom",          required_argument, NULL, 'b' },
	{ "bad-bloom",      required_argument, NULL, OPTION_BBLOOM },
	{ "nan-str",        required_argument, NULL, 'r' },

	// Generic options
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


#define INSPECT_OPTION_STR "i:f:p:b:o:n:d:s:h"

static struct option inspect_longopts[] = {
	// I/O options
	{ "input",          required_argument, NULL, 'i' },
	{ "input-format",   required_argument, NULL, 'f' },
	{ "pcap-filter",    required_argument, NULL, 'p' },
	{ "bloom",          required_argument, NULL, 'b' },
	{ "output",         required_argument, NULL, 'o' },

	// Feature options
	{ "ngram-length",   required_argument, NULL, 'n' },
	{ "ngram-delim",    required_argument, NULL, 'd' },
	{ "filter-size",    required_argument, NULL, 's' },
	{ "hash-set",       required_argument, NULL, OPTION_HASHSET},

	// Generic options
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


#define STATS_OPTION_STR "b:h"

static struct option stats_longopts[] = {
	// I/O options
	{ "bloom",          required_argument, NULL, 'b' },

	// Generic options
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


const int usage_main()
{
	print("Usage: salad [<mode>] [options]\n"
	"\n"
#ifdef TEST_SALAD
	"<mode> may be one of 'train', 'predict', 'inspect', 'stats', 'dbg'\n"
#else
	"<mode> may be one of 'train', 'predict', 'inspect', 'stats'\n"
#endif
	"\n"
	"Generic options:\n"
	"  -v,  --version              Print version and copyright.\n"
	"  -h,  --help                 Print this help screen.\n");
	return EXIT_SUCCESS;
}


const int usage_train()
{
	print("Usage: salad train [options]\n"
	"\n"
	"I/O options:\n"
	"  -i,  --input <file>         The input filename.\n"
	"  -f,  --input-format <fmt>   Sets the format of input. This option might be \n"
	"                              one of " VALID_IOMODES ".\n"
#ifdef USE_NETWORK
	"  -p,  --pcap-filter <str>    Filter expression for the PCAP library in case\n"
	"                              network data is processed (Default: %s).\n"
#endif
	"  -o,  --output <file>        The output filename.\n"
	"\n"
	"Feature options:\n"
	"  -n,  --ngram-len <num>      Set length of n-grams (Default: %"Z").\n"
	"  -d,  --ngram-delim <delim>  Set delimiters for the use of words n-grams.\n"
	"                              If omitted or empty byte n-grams are used.\n"
	"  -s,  --filter-size <num>    Set the size of the bloom filter as bits of\n"
	"                              the index (Default: %u).\n"
	"       --hash-set <hashes>    Set the hash set to be used: 'simple' or 'murmur'\n"
	"                              (Default: 'simple').\n"
	"\n"
	"Generic options:\n"
	"  -h,  --help                 Print this help screen.\n",
#ifdef USE_NETWORK
	/* --pcap-filter */ DEFAULT_CONFIG.pcap_filter,
#endif
	/* --ngram-len   */ DEFAULT_CONFIG.ngramLength,
	/* --filter-size */ DEFAULT_CONFIG.filter_size);
	return EXIT_SUCCESS;
}


const int usage_predict()
{
	print("Usage: salad predict [options]\n"
	"\n"
	"I/O options:\n"
	"  -i,  --input <file>         The input filename.\n"
	"  -f,  --input-format <fmt>   Sets the format of input. This option might be \n"
	"                              one of " VALID_IOMODES ".\n"
#ifdef USE_NETWORK
	"  -p,  --pcap-filter <str>    Filter expression for the PCAP library in case\n"
	"                              network data is processed (Default: %s).\n"
#endif
	"  -b,  --bloom <file>         The bloom filter to be used.\n"
	"       --bad-bloom <file>     The bloom filter for the 2nd class (optional).\n"
	"  -o,  --output <file>        The output filename.\n"
	"\n"
	"Feature options:\n"
	"  -r,  --nan-str <str>        Set the string to be shown for NaN values.\n"
	"\n"
	"Generic options:\n"
	"  -h,  --help                 Print this help screen.\n"
#ifdef USE_NETWORK
	/* --pcap-filter */ ,DEFAULT_CONFIG.pcap_filter
#endif
	);
	return EXIT_SUCCESS;
}


const int usage_inspect()
{
	print("Usage: salad inspect [options]\n"
	"\n"
	"I/O options:\n"
	"  -i,  --input <file>         The input filename.\n"
	"  -f,  --input-format <fmt>   Sets the format of input. This option might be \n"
	"                              one of " VALID_IOMODES ".\n"
#ifdef USE_NETWORK
	"  -p,  --pcap-filter <str>    Filter expression for the PCAP library in case\n"
	"                              network data is processed (Default: %s).\n"
#endif
	"  -b,  --bloom <file>         The bloom filter to be used.\n"
	"  -o,  --output <file>        The output filename.\n"
	"\n"
	"Feature options:\n"
	"  -n,  --ngram-len <num>      Set length of n-grams (Default: %"Z").\n"
	"  -d,  --ngram-delim <delim>  Set delimiters for the use of words n-grams.\n"
	"                              If omitted or empty byte n-grams are used.\n"
	"  -s,  --filter-size <num>    Set the size of the bloom filter as bits of\n"
	"                              the index (Default: %u).\n"
	"       --hash-set <hashes>    Set the hash set to be used: 'simple' or 'murmur'\n"
	"                              (Default: 'simple').\n"
	"\n"
	"Generic options:\n"
	"  -h,  --help                 Print this help screen.\n",
#ifdef USE_NETWORK
	/* --pcap-filter */ DEFAULT_CONFIG.pcap_filter,
#endif
	/* --ngram-len   */ DEFAULT_CONFIG.ngramLength,
	/* --filter-size */ DEFAULT_CONFIG.filter_size);
	return EXIT_SUCCESS;
}


const int usage_stats()
{
	print("Usage: salad stats [options]\n"
	"\n"
	"I/O options:\n"
	"  -b,  --bloom <file>         The bloom filter to be analyzed.\n"
	"\n"
	"Generic options:\n"
	"  -h,  --help                 Print this help screen.\n");
	return EXIT_SUCCESS;
}


const int version()
{
	print("Letter Salad %s - A Content Anomaly Detector Based on n-Grams\n"
	    "Copyright (c) 2012-2013 Christian Wressnegger (christian@mlsec.org)\n",
	    VERSION_STR);

	return EXIT_SUCCESS;
}

const int check_input(config_t* const config, const int filesOnly)
{
	if (config->input == NULL || config->input[0] == 0x00)
	{
		error("No input file specified.");
		return EXIT_FAILURE;
	}

	if (config->mode == STATS)
	{
		status("Input: %s", config->input);
	}
	else
	{
#ifdef USE_NETWORK
#ifndef ALLOW_LIVE_TRAINING
		if (config->input_type == NETWORK && filesOnly)
		{
			error("Use a network dump for training");
			return EXIT_FAILURE;
		}
#endif
		if (config->input_type == NETWORK || config->input_type == NETWORK_DUMP)
		{
			const int empty = (config->pcap_filter == NULL || config->pcap_filter[0] == '\0');
			const char* const filter = (empty ? "unfiltered" : config->pcap_filter);

			status("Input: %s (%s mode: %s)",
			       config->input, to_string(config->input_type), filter);
		}
		else
#endif
		{
			status("Input: %s (%s mode)",
			       config->input, to_string(config->input_type));
		}
	}

	if (config->bloom != NULL)
	{
		status("Bloom filter: %s", config->bloom);
	}

	if (config->bbloom != NULL)
	{
		status("Bad content bloom filter: %s", config->bbloom);
	}

	return EXIT_SUCCESS;
}

const int check_output(config_t* const config)
{
	if (config->output == NULL || config->output[0] == 0x00)
	{
		error("No output file specified.");
		return EXIT_FAILURE;
	}

	status("Output: %s", config->output);
	return EXIT_SUCCESS;
}


const hashset_t to_hashset(const char* const str)
{
	switch (cmp(str, "simple", "murmur", NULL))
	{
	case 0: return HASHES_SIMPLE;
	case 1: return HASHES_MURMUR;
	default: break;
	}

	return HASHES_UNDEFINED;
}

const char* const hashset_to_string(hashset_t hs)
{
	switch(hs)
	{
	case HASHES_SIMPLE: return "simple";
	case HASHES_MURMUR: return "murmur";
	default: break;
	}
	return "undefined";
}

const io_mode_t as_iomode(const char* const x)
{
	io_mode_t y = to_iomode(optarg);

	if (!is_valid_iomode(optarg))
	{
		warn("Illegal input type '%s', using '%s' instead",
		     optarg, to_string(y));
	}
	return y;
}

const saladmode_t parse_traininglike_options_ex(int argc, char* argv[], config_t* const config,
		const char *shortopts, const struct option *longopts)
{
	assert(argv != NULL);
	assert(config != NULL);

	int option;
	while ((option = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
	{
		switch (option)
		{
		case 'i':
			config->input = optarg;
			break;

		case 'f':
			config->input_type = as_iomode(optarg);
			break;

		case 'p':
			config->pcap_filter = optarg;
			break;

		case 'b':
			config->bloom = optarg;
			break;

		case 'o':
			config->output = optarg;
			break;

		case 'n':
		{
			int ngramLength = atoi(optarg); // TODO: strtol
			if (ngramLength <= 0)
			{
				warn("Illegal n-gram length specified.");
				warn("Defaulting to: %u\n", (unsigned int) config->ngramLength);
			}
			else config->ngramLength = ngramLength;
			break;
		}
		case 'd':
			config->delimiter = optarg;
			break;

		case 's':
		{
			int filter_size = atoi(optarg); // TODO: strtol
			if (filter_size <= 0)
			{
				warn("Illegal filter size specified.");
				warn("Defaulting to: %u\n", (unsigned int) config->filter_size);
			}
			else config->filter_size = filter_size;
			break;
		}
		case OPTION_HASHSET:
		{
			hashset_t hashset = to_hashset(optarg);
			if (hashset == HASHES_UNDEFINED)
			{
				warn("Illegal hash set specified.");
				warn("Defaulting to: %s\n", hashset_to_string(config->hash_set));
			}
			else config->hash_set = hashset;
			break;
		}
		case '?':
		case 'h':
			return SALAD_HELP_TRAIN;

		default: break;
		}
	}

	status("Mode: %s", anamode_to_string(config->mode));
	if (config->bloom != NULL)
	{
		status("Based on: %s", config->bloom);
	}

	if (check_input(config, TRUE) == EXIT_FAILURE) return SALAD_EXIT;
	if (check_output(config) == EXIT_FAILURE) return SALAD_EXIT;

	status("n-Gram length: %u", (unsigned int) config->ngramLength);

	if (config->delimiter != NULL)
	{
		status("Delimiter: '%s'", config->delimiter);
	}

	status("Filter size: %u", config->filter_size);
	status("Hash set: %s", hashset_to_string(config->hash_set));
	return SALAD_RUN;
}


const saladmode_t parse_training_options(int argc, char* argv[], config_t* const config)
{
	return parse_traininglike_options_ex(argc, argv, config, TRAIN_OPTION_STR, train_longopts);
}


const saladmode_t parse_predict_options(int argc, char* argv[], config_t* const config)
{
	assert(argv != NULL);
	assert(config != NULL);

	int option;
	while ((option = getopt_long(argc, argv, PREDICT_OPTION_STR, predict_longopts, NULL)) != -1)
	{
		switch (option)
		{
		case 'i':
			config->input = optarg;
			break;

		case 'f':
			config->input_type = as_iomode(optarg);
			break;

		case 'p':
			config->pcap_filter = optarg;
			break;

		case 'o':
			config->output = optarg;
			break;

		case 'b':
			config->bloom = optarg;
			break;

		case OPTION_BBLOOM:
			config->bbloom = optarg;
			break;

		case 'r':
			config->nan = optarg;
			break;

		case '?':
		case 'h':
			return SALAD_HELP_PREDICT;

		default: break;
		}
	}

	status("Mode: %s", anamode_to_string(config->mode));

	if (check_input(config, FALSE) == EXIT_FAILURE) return SALAD_EXIT;
	if (check_output(config) == EXIT_FAILURE) return SALAD_EXIT;

	return SALAD_RUN;
}


const saladmode_t parse_inspect_options(int argc, char* argv[], config_t* const config)
{
	const saladmode_t m = parse_traininglike_options_ex(argc, argv, config, INSPECT_OPTION_STR, inspect_longopts);
	return (m == SALAD_HELP_TRAIN ? SALAD_HELP_INSPECT : m);
}


const saladmode_t parse_stats_options(int argc, char* argv[], config_t* const config)
{
	assert(argv != NULL);
	assert(config != NULL);

	int option;
	while ((option = getopt_long(argc, argv, STATS_OPTION_STR, stats_longopts, NULL)) != -1)
	{
		switch (option)
		{
		case 'b':
			config->input = optarg;
			break;

		case '?':
		case 'h':
			return SALAD_HELP_STATS;

		default: break;
		}
	}

	status("Mode: %s", anamode_to_string(config->mode));

	if (check_input(config, TRUE) == EXIT_FAILURE)
	{
		return SALAD_EXIT;
	}
	config->bloom = config->input;
	return SALAD_RUN;
}


const saladmode_t parse_options(int argc, char* argv[], config_t* const config)
{
	assert(argv != NULL);
	assert(config != NULL);

	if (argc <= 1)
	{
		return SALAD_HELP;
	}

	*config = DEFAULT_CONFIG;

	if (*argv[1] != '-')
	{
		config->mode = to_anamode(argv[1]);
		switch (config->mode)
		{
		case TRAINING: return parse_training_options(argc, argv, config);
		case PREDICT:  return parse_predict_options(argc, argv, config);
		case INSPECT:  return parse_inspect_options(argc, argv, config);
		case STATS:    return parse_stats_options(argc, argv, config);
#ifdef TEST_SALAD
		case DBG:      return SALAD_RUN;
#endif
		default:
			error("Unknown mode '%s'.", argv[1]);
			return SALAD_EXIT;
		}
	}

	int option;
	while ((option = getopt_long(argc, argv, MAIN_OPTION_STR, main_longopts, NULL)) != -1)
	{
		switch (option)
		{
		case '?':
		case 'h':
			return SALAD_HELP;

		case 'v':
			return SALAD_VERSION;

		default: break;
		}
	}
	return SALAD_EXIT;
}


const int bye(const int ec)
{
	if (ec == EXIT_SUCCESS)
	{
		status("Bye!");
	}
	else
	{
		error("Bye!");
	}
	return ec;
}


int main(int argc, char* argv[])
{
	config_t config;
	switch (parse_options(argc, argv, &config))
	{
	case SALAD_EXIT:         return bye(EXIT_FAILURE);
	case SALAD_HELP:         return usage_main();
	case SALAD_HELP_TRAIN:   return usage_train();
	case SALAD_HELP_PREDICT: return usage_predict();
	case SALAD_HELP_INSPECT: return usage_inspect();
	case SALAD_HELP_STATS:   return usage_stats();
	case SALAD_VERSION:      return version();
	default: break;
	}

	int ret = 0;
	if (config.input_type == FILES)
	{
		error("Input mode 'files' is not yet implemented.");
		ret = EXIT_FAILURE;
	}
	else
	{
		switch (config.mode)
		{
		case TRAINING:
			ret = salad_train(&config);
			break;
		case PREDICT:
			ret = salad_predict(&config);
			break;
		case INSPECT:
			ret = salad_inspect(&config);
			break;
		case STATS:
			ret = salad_stats(&config);
			break;
#ifdef TEST_SALAD
		case DBG:
			ret = salad_dbg(&config);
			break;
#endif

		default: break;
		}
	}

	return bye(ret);
}


const char* const anamode_to_string(anamode_t m)
{
	switch (m)
	{
	case TRAINING: return "training";
	case PREDICT:  return "predict";
	case INSPECT:  return "inspect";
	case STATS:    return "stats";
	case DBG:      return "dbg";
	default: break;
	}
	return "undefined";
}

const anamode_t to_anamode(const char* const str)
{
	switch (cmp(str, "train", "predict", "inspect", "stats", "dbg", NULL))
	{
	case 0: return TRAINING;
	case 1: return PREDICT;
	case 2: return INSPECT;
	case 3: return STATS;
	case 4: return DBG;
	}
	return UNDEFINED;
}


const int bloom_t_diff(const bloom_t* const a, const bloom_t* const b)
{
	assert(a != NULL);
	assert(b != NULL);

	return (memcmp(a->delim, b->delim, 256) != 0
			|| a->useWGrams != b->useWGrams
			|| a->ngramLength != b->ngramLength);
}

const char* const to_delim(uint8_t* const delim, const char* const delimiter)
{
	const char* x = delimiter;
	if (x != NULL)
	{
		if (x[0] == 0x00)
		{
			x = NULL;
		}
	}
	to_delimiter_array(delim, x);
	return x;
}


BLOOM* const bloom_from_file_ex(const char* const id, const char* const filename, uint8_t* const delim, int* const useWGrams, size_t* const ngramLength)
{
	assert(filename != NULL);

	FILE* const fFilter = fopen(filename, "r");
	if (fFilter == NULL)
	{
		error("Unable to open %s filter.", id);
		return NULL;
	}

	BLOOM* bloom = NULL;
	int ret = fread_model(fFilter, &bloom, ngramLength, delim, useWGrams);
	fclose(fFilter);

	if (ret != 0)
	{
		error("Corrupt %s filter.", id);
		return NULL;
	}
	return bloom;
}


const int bloom_from_file(const char* const id, const char* const filename, bloom_t* const out)
{
	if (out != NULL)
	{
		out->bloom = bloom_from_file_ex(id, filename, out->delim, &out->useWGrams, &out->ngramLength);
		return (out->bloom != NULL);
	}
	return (bloom_from_file_ex(id, filename, NULL, NULL, NULL) != NULL);
}


const int salad_heart(const config_t* const c, FN_SALAD fct)
{
	assert(c != NULL);
	const data_processor_t* const dp = to_dataprocssor(c->input_type);

#ifdef USE_NETWORK
	net_param_t p = { (c->input_type == NETWORK), c->pcap_filter, NULL };
#else
	io_param_t p = { NULL };
#endif

	file_t fIn;
	int ret = dp->open(&fIn, c->input, &p);
	if (ret != EXIT_SUCCESS)
	{
		error("Unable to open input data.");
		if (p.error_msg != NULL)
		{
			error("%s", p.error_msg);
		}
		return EXIT_FAILURE;
	}

	FILE* const fOut = fopen(c->output, "w+");
	if (fOut == NULL)
	{
		error("Unable to create output file.");
		if (fIn.fd != NULL)
		{
			fclose(fIn.fd);
		}
		return EXIT_FAILURE;
	}

	const int result = fct(c, dp, fIn, fOut);

	dp->close(&fIn);
	fclose(fOut);

	return result;
}

