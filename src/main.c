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


#include <salad/salad.h>
#include <salad/anagram.h>

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include "main.h"

#include <salad/util.h>
#include <util/vec.h>
#include <util/io.h>
#include <util/log.h>


#define MAIN_OPTION_STR "i:f:b:w:o:t:n:d:r:vh"

static struct option main_longopts[] = {
	// Generic options
	{ "version",        no_argument, NULL, 'v' },
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


#define TRAIN_OPTION_STR "i:f:p:uo:n:d:s:eh"
#define OPTION_INPUTFILTER 1000
#define OPTION_BATCHSIZE   1001
#define OPTION_HASHSET     1002
#define OPTION_BINARY      1003

static struct option train_longopts[] = {
	// I/O options
	{ "input",          required_argument, NULL, 'i' },
	{ "input-format",   required_argument, NULL, 'f' },
	{ "input-filter",   required_argument, NULL, OPTION_INPUTFILTER },
	{ "batch-size",     required_argument, NULL, OPTION_BATCHSIZE },
	{ "pcap-filter",    required_argument, NULL, 'p' },
	{ "update-model",   no_argument,       NULL, 'u' },
	{ "output",         required_argument, NULL, 'o' },

	// Feature options
	{ "ngram-length",   required_argument, NULL, 'n' },
	{ "ngram-delim",    required_argument, NULL, 'd' },
	{ "binary",         no_argument,       NULL, OPTION_BINARY },
	{ "filter-size",    required_argument, NULL, 's' },
	{ "hash-set",       required_argument, NULL, OPTION_HASHSET},

	// Generic options
	{ "echo-params",    no_argument, NULL, 'e' },
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


#define PREDICT_OPTION_STR "i:f:gp:o:b:r:eh"
#define OPTION_BBLOOM    1003

static struct option predict_longopts[] = {
	// I/O options
	{ "input",          required_argument, NULL, 'i' },
	{ "input-format",   required_argument, NULL, 'f' },
	{ "input-filter",   required_argument, NULL, OPTION_INPUTFILTER },
	{ "batch-size",     required_argument, NULL, OPTION_BATCHSIZE },
	{ "group-input",    required_argument, NULL, 'g' },
	{ "pcap-filter",    required_argument, NULL, 'p' },
	{ "output",         required_argument, NULL, 'o' },

	{ "bloom",          required_argument, NULL, 'b' },
	{ "bad-bloom",      required_argument, NULL, OPTION_BBLOOM },
	{ "nan-str",        required_argument, NULL, 'r' },

	// Generic options
	{ "echo-params",    no_argument, NULL, 'e' },
	{ "help",           no_argument, NULL, 'h' },
	{ NULL,             0, NULL, 0 }
};


#define INSPECT_OPTION_STR "i:f:p:b:o:n:d:s:eh"

static struct option inspect_longopts[] = {
	// I/O options
	{ "input",          required_argument, NULL, 'i' },
	{ "input-format",   required_argument, NULL, 'f' },
	{ "input-filter",   required_argument, NULL, OPTION_INPUTFILTER },
	{ "batch-size",     required_argument, NULL, OPTION_BATCHSIZE },
	{ "pcap-filter",    required_argument, NULL, 'p' },
	{ "bloom",          required_argument, NULL, 'b' },
	{ "output",         required_argument, NULL, 'o' },

	// Feature options
	{ "ngram-length",   required_argument, NULL, 'n' },
	{ "ngram-delim",    required_argument, NULL, 'd' },
	{ "binary",         no_argument,       NULL, OPTION_BINARY },
	{ "filter-size",    required_argument, NULL, 's' },
	{ "hash-set",       required_argument, NULL, OPTION_HASHSET},

	// Generic options
	{ "echo-params",    no_argument, NULL, 'e' },
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
#ifdef USE_REGEX_FILTER
	"       --input-filter <regex> The regular expression for filtering input lines\n"
	"                              or filenames respectively.\n"
#endif
	"       --batch-size <num>     Set the size of batches that are read and \n"
	"                              processed in one go (Default: %"Z").\n"
#ifdef USE_NETWORK
	"  -p,  --pcap-filter <str>    Filter expression for the PCAP library in case\n"
	"                              network data is processed (Default: %s).\n"
#endif
	"  -u,  --update-model         In case the specified output file exists and\n"
	"                              contains a valid model this flag indicates\n"
	"                              that that model should be update rather than\n"
	"                              recreated from scratch.\n"
	"  -o,  --output <file>        The output filename.\n"
	"\n"
	"Feature options:\n"
	"  -n,  --ngram-len <num>      Set length of n-grams (Default: %"Z").\n"
	"  -d,  --ngram-delim <delim>  Set delimiters for the use of word/ token n-grams.\n"
	"                              If omitted or empty byte n-grams are used.\n"
	"       --binary               Indicates to use bit n-grams rather than byte\n"
	"                              or token n-grams and consequently, disables the\n"
	"                              --ngram-delim option.\n"
	"  -s,  --filter-size <num>    Set the size of the bloom filter as bits of\n"
	"                              the index (Default: %u).\n"
	"       --hash-set <hashes>    Set the hash set to be used: 'simple' or 'murmur'\n"
	"                              (Default: 'simple').\n"
	"\n"
	"Generic options:\n"
	"  -e,  --echo-params          Echo used parameters and settings.\n"
	"  -h,  --help                 Print this help screen.\n",
	/* --batch-size  */ DEFAULT_CONFIG.batch_size,
#ifdef USE_NETWORK
	/* --pcap-filter */ DEFAULT_CONFIG.pcap_filter,
#endif
	/* --ngram-len   */ DEFAULT_CONFIG.ngram_length,
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
#ifdef USE_REGEX_FILTER
	"       --input-filter <regex> The regular expression for filtering input lines\n"
	"                              or filenames respectively.\n"
#endif
	"       --batch-size <num>     Set the size of batches that are read and \n"
	"                              processed in one go (Default: %"Z").\n"
#ifdef GROUPED_INPUT
	"  -g,  --grouped-input        Indicates that predictions for inputs in the "
	"                              same \"group\" should be grouped as well. \n"
#endif
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
	"  -e,  --echo-params          Echo used parameters and settings.\n"
	"  -h,  --help                 Print this help screen.\n",
	/* --batch-size  */  DEFAULT_CONFIG.batch_size
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
#ifdef USE_REGEX_FILTER
	"       --input-filter <regex> The regular expression for filtering input lines\n"
	"                              or filenames respectively.\n"
#endif
	"       --batch-size <num>     Set the size of batches that are read and \n"
	"                              processed in one go (Default: %"Z").\n"
#ifdef USE_NETWORK
	"  -p,  --pcap-filter <str>    Filter expression for the PCAP library in case\n"
	"                              network data is processed (Default: %s).\n"
#endif
	"  -b,  --bloom <file>         The bloom filter to be used.\n"
	"  -o,  --output <file>        The output filename.\n"
	"\n"
	"Feature options:\n"
	"  -n,  --ngram-len <num>      Set length of n-grams (Default: %"Z").\n"
	"  -d,  --ngram-delim <delim>  Set delimiters for the use of word/ token n-grams.\n"
	"                              If omitted or empty byte n-grams are used.\n"
	"       --binary               Indicates to use bit n-grams rather than byte\n"
	"                              or token n-grams and consequently, disables the\n"
	"                              --ngram-delim option.\n"
	"  -s,  --filter-size <num>    Set the size of the bloom filter as bits of\n"
	"                              the index (Default: %u).\n"
	"       --hash-set <hashes>    Set the hash set to be used: 'simple' or 'murmur'\n"
	"                              (Default: 'simple').\n"
	"\n"
	"Generic options:\n"
	"  -e,  --echo-params          Echo used parameters and settings.\n"
	"  -h,  --help                 Print this help screen.\n",
	/* --batch-size  */ DEFAULT_CONFIG.batch_size,
#ifdef USE_NETWORK
	/* --pcap-filter */ DEFAULT_CONFIG.pcap_filter,
#endif
	/* --ngram-len   */ DEFAULT_CONFIG.ngram_length,
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
	    "Copyright (c) 2012-2015 Christian Wressnegger (christian@mlsec.org)\n",
	    VERSION_STR);

	return EXIT_SUCCESS;
}

const int check_input(config_t* const config, const int filesOnly, const int check_batchsize)
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
			if (check_batchsize && config->batch_size != 1)
			{
				warn("For processing network data we default the batch size to 1");
			}
			config->batch_size = 1;

			const int empty = (config->pcap_filter == NULL || config->pcap_filter[0] == '\0');
			const char* const filter = (empty ? "unfiltered" : config->pcap_filter);

			status("Input: %s (%s mode: %s)",
			       config->input, iomode_to_string(config->input_type), filter);
		}
		else
#endif
#ifdef GROUPED_INPUT
		if (config->input_type == ARCHIVE && config->group_input)
		{
			status("Input: %s (%s mode - grouped)",
					config->input, iomode_to_string(config->input_type));
		}
		else
#endif
		{
			status("Input: %s (%s mode)",
			       config->input, iomode_to_string(config->input_type));
		}
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

const iomode_t as_iomode(const char* const x)
{
	iomode_t y = to_iomode(x);

	if (!is_valid_iomode(x))
	{
		warn("Illegal input type '%s', using '%s' instead",
		     x, iomode_to_string(y));
	}
	return y;
}

const saladstate_t parse_traininglike_options_ex(int argc, char* argv[], config_t* const config,
		const char *shortopts, const struct option *longopts)
{
	assert(argv != NULL);
	assert(config != NULL);

	int option, bs = FALSE, fo = FALSE;
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

		case OPTION_INPUTFILTER:
			config->input_filter = optarg;
			break;

		case OPTION_BATCHSIZE:
		{
			int batch_size = atoi(optarg); // TODO: strtol
			if (batch_size <= 0)
			{
				warn("Illegal batch size specified.\n");
				// This is not true in case of network data as input. Therefore,
				// we simply suppress this output at this point.
				// warn("Defaulting to: %u\n", (unsigned int) config->batch_size);
			}
			else
			{
				bs = TRUE;
				config->batch_size = batch_size;
			}
			break;
		}
#ifdef USE_NETWORK
		case 'p':
			config->pcap_filter = optarg;
			break;
#endif
		case 'b':
			config->bloom = optarg;
			break;

		case 'u':
			config->update_model = TRUE;
			break;

		case 'o':
			config->output = optarg;
			break;

		case 'n':
		{
			fo = TRUE;
			int ngram_length = atoi(optarg); // TODO: strtol
			if (ngram_length <= 0)
			{
				warn("Illegal n-gram length specified.");
				warn("Defaulting to: %u\n", (unsigned int) config->ngram_length);
			}
			else config->ngram_length = ngram_length;
			break;
		}
		case 'd':
			fo = TRUE;
			config->delimiter = optarg;
			break;

		case OPTION_BINARY:
			config->binary_ngrams = TRUE;
			break;

		case 's':
		{
			fo = TRUE;
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
			fo = TRUE;
			hashset_t hashset = to_hashset(optarg);
			if (hashset == HASHES_UNDEFINED)
			{
				warn("Illegal hash set specified.");
				warn("Defaulting to: %s\n", hashset_to_string(config->hash_set));
			}
			else config->hash_set = hashset;
			break;
		}
		case 'e':
			config->echo_params = TRUE;
			break;

		case '?':
		case 'h':
			return SALAD_HELP_TRAIN;

		default:
			// In order to catch program argument that correspond to
			// features that were excluded at compile time.
			fprintf(stderr, "invalid option -- '%c'\n", option);
			return SALAD_HELP_TRAIN;
		}
	}

	config->transfer_spec = !fo;

	if (config->binary_ngrams && config->ngram_length > MASK_BITSIZE)
	{
		error("When using binary n-grams currently only a maximal");
		error("length of %u bits is supported.", MASK_BITSIZE);
		return SALAD_EXIT;
	}

	if (check_input(config, TRUE, bs) == EXIT_FAILURE) return SALAD_EXIT;
	if (check_output(config) == EXIT_FAILURE) return SALAD_EXIT;

	if (config->echo_params)
	{
		if (config->update_model && config->transfer_spec) {
			// cf. salad_train_stub
		} else {
			echo_options(config);
		}
	}
	return SALAD_RUN;
}


const saladstate_t parse_training_options(int argc, char* argv[], config_t* const config)
{
	return parse_traininglike_options_ex(argc, argv, config, TRAIN_OPTION_STR, train_longopts);
}


const saladstate_t parse_predict_options(int argc, char* argv[], config_t* const config)
{
	assert(argv != NULL);
	assert(config != NULL);

	int option, bs = FALSE;
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

		case OPTION_INPUTFILTER:
			config->input_filter = optarg;
			break;

		case OPTION_BATCHSIZE:
		{
			int batch_size = atoi(optarg); // TODO: strtol
			if (batch_size <= 0)
			{
				warn("Illegal batch size specified.");
				// This is not true in case of network data as input. Therefore,
				// we simply suppress this output at this point.
				// warn("Defaulting to: %u\n", (unsigned int) config->batch_size);
			}
			else
			{
				bs = TRUE;
				config->batch_size = batch_size;
			}
			break;
		}
#ifdef GROUPED_INPUT
		case 'g':
			config->group_input = TRUE;
			break;
#endif
#ifdef USE_NETWORK
		case 'p':
			config->pcap_filter = optarg;
			break;
#endif
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

		case 'e':
			config->echo_params = TRUE;
			break;

		case '?':
		case 'h':
			return SALAD_HELP_PREDICT;

		default:
			// In order to catch program argument that correspond to
			// features that were excluded at compile time.
			fprintf(stderr, "invalid option -- '%c'\n", option);
			return SALAD_HELP_PREDICT;
		}
	}

	if (check_input(config, FALSE, bs) == EXIT_FAILURE) return SALAD_EXIT;
	if (check_output(config) == EXIT_FAILURE) return SALAD_EXIT;

	if (config->echo_params)
	{
		// cf. salad_predict_stub
	}
	return SALAD_RUN;
}


const saladstate_t parse_inspect_options(int argc, char* argv[], config_t* const config)
{
	const saladstate_t m = parse_traininglike_options_ex(argc, argv, config, INSPECT_OPTION_STR, inspect_longopts);
	return (m == SALAD_HELP_TRAIN ? SALAD_HELP_INSPECT : m);
}


const saladstate_t parse_stats_options(int argc, char* argv[], config_t* const config)
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

		default:
			// In order to catch program argument that correspond to
			// features that were excluded at compile time.
			fprintf(stderr, "invalid option -- '%c'\n", option);
			return SALAD_HELP_STATS;
		}
	}

	if (check_input(config, TRUE, FALSE) == EXIT_FAILURE)
	{
		return SALAD_EXIT;
	}
	config->bloom = config->input;
	return SALAD_RUN;
}


const saladstate_t parse_options(int argc, char* argv[], config_t* const config)
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
		config->mode = to_saladmode(argv[1]);
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

	if (config.input_type == FILES)
	{
		error("Input mode 'files' is not yet implemented.");
		return bye(EXIT_FAILURE);
	}

	int ret = EXIT_FAILURE, is_metaop = FALSE;
	switch (config.mode)
	{
	case TRAINING:
		ret = _salad_train_(&config);
		break;
	case PREDICT:
		ret = _salad_predict_(&config);
		break;
	case INSPECT:
		ret = _salad_inspect_(&config);
		break;
	default:
		is_metaop = TRUE;
		break;
	}
	switch (config.mode)
	{
	case STATS:
		ret = _salad_stats_(&config);
		break;
#ifdef TEST_SALAD
	case DBG:
		ret = _salad_dbg_(&config);
		break;
#endif

	default: break;
	}
	return bye_ex(ret, is_metaop ? NULL : "Done!");
}
