/**
 * @class hidden_copyright
 * \n
 *
 * Copyright (c) 2012-2015, Christian Wressnegger\n
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "common.h"
#include "test/common.h"

/**
 * @class hidden_intro
 * \n
 *
 * \b Letter \b Salad or \b Salad for short, is an efficient and flexible
 * implementation of the well-known anomaly detection method Anagram by Wang et
 * al (RAID 2006).
 *
 * \b Salad enables detecting anomalies in large-scale string data. The tool is
 * based on the concepts of n-grams, that is, strings are compared using all
 * substrings of length n. During training, cf. \ref salad-train
 * "salad-train"(1), these n-grams are extracted from a collection of strings
 * and stored in a Bloom filter. This enables the detector to represent a large
 * number of n-grams in very little memory. During anomaly detection, the
 * n-grams of unknown strings are matched against the Bloom filter and strings
 * containing several n-grams not seen during training are flagged as anomalous.
 *
 * \b Salad extends the original method Anagram in different ways: First, the
 * tool does not only operate on n-grams of bytes, but is also capable of
 * comparing n-grams over bits or words and tokens. Second, \b Salad implements
 * a 2-class version of the detector that enables discriminating strings of two
 * types, cf. \ref salad-predict "salad-predict"(1) for more details. Finally,
 * the tool features a build-in inspection and statistic mode that can help to
 * analyze the learned Bloom filter and its predictions, cf. \ref salad-inspect
 * "salad-inspect"(1) and \ref salad-stats "salad-stats"(1) respectively.
 *
 * The tool can be utilized in different fields of application. For example, the
 * concept underlying \b Salad has been prominently used for intrusion detection,
 * but is not limited to this scenario.
 */

/**
 * @mainpage Salad - A Content Anomaly Detector based on n-Grams
 *
 * @section sec_overview Overview
 *
 * \copydoc hidden_intro
 *
 * The individual modes can be accessed using by specifying the mode of
 * operation as command line option for the main executable. Please refer to the
 * corresponding man page for more details on its usage: \ref salad "salad"(1).
 *
 * \copydoc hidden_version
 *
 * @section sec_install Installation
 * @subsection sec_dependencies Dependencies
 *
 * The following libraries are required for building Salad from source code.
 * These libraries are available as packages with many operating system
 * distributions, e.g. Debian and Ubuntu Linux.
 *
 * - CMake as a build system (&ge;2.6)\n
 *   http://www.cmake.org
 * - libarchive (&ge; 2.8.4)\n
 *   http://www.libarchive.org/
 *
 * @subsection sec_compilation Compilation
 * Salad make use of the cross-platform build system cmake. It has
   been successfully compiled on Linux, Mac OS X and Windows
 *
 * \code
 * $ cmake [options] .
 * $ make
 * $ make doc
 * $ make install
 * \endcode
 *
 * @subsection sec_options Configuration Options
 * @par -DCMAKE_INSTALL_PREFIX=PATH
 * By default Salad is installed into /usr/local. If you prefer a different
 * location, use this option to select an installation directory.
 *
 * @par -DUSE_ARCHIVES=ON/OFF
 * If this feature is enabled, Sally can also be applied to read the contents
 * of archives, such as .tgz and .zip. This allows for processing data in
 * compressed form and may drastically save storage space. By default this
 * feature is enabled.
 *
 * @section sec_background Background information
 *
 * The following technical article details the background of Anagram.
 * - Anagram: A Content Anomaly Detector Resistant to Mimicry Attacks\n
 *   Ke Wang, Janak J. Parekh and Salvatore J. Stolfo.\n
 *   Proc. of 9th International Conference on Recent Advances in Intrusion
 *   Detection (RAID), 2006.
 *
 * A detailed description and analysis of the underlying n-gram model and
 * learning strategies used in Salad can be found here:
 * - A Close Look on n-Grams in Intrusion Detection\n
 *   Christian Wressnegger, Guido Schwenk, Daniel Arp and Konrad Rieck.\n
 *   6th ACM CCS Workshop on Artificial Intelligence and Security (AISEC), 2013.
 *
 *
 * @section sec_license License (GPL-3)
 * \copydoc hidden_copyright
 */

#include <stdlib.h>

/**
 * @page salad A Content Anomaly Detector Based on n-Grams
 *
 * @section SYNOPSIS
 *
 * salad [&lt;mode&gt;] [options]
 *
 * @section DESCRIPTION
 * \copydoc hidden_intro
 *
 * @section OPTIONS
 *
 * The options depend on the provided mode of operation. If no mode is
 * specified, the following generic options are available:
 *
 * @par --help
 * Print the help screen.
 * @par --version
 * Print version and copyright.
 *
 * @section SALAD MODES
 *
 * There exist two different means of accessing salad's modes: (1) As command
 * line option to the main executable, or (2) as stand-alone executable prefixed
 * with \b salad-.
 *
 * The following list contains the names of the stand-alone executables, for
 * which individual man pages are available:
 *
 * @subsection sec_salad-train   salad-train(1)
 * Trains the anomaly detector.
 *
 * @subsection sec_salad-predict salad-predict(1)
 * Predicts the anomaly score of the specified data.
 *
 * @subsection sec_salad-stats   salad-stats(1)
 * Provides statistical information of a trained anomaly detector.
 *
 * @subsection sec_salad-inspect salad-inspect(1)
 * Analyzes the specified data with respect to the n-gram model used by the
 * detector.
 *
 * @section sec_copyright COPYRIGHT
 * \copydoc hidden_copyright
 */
int main(int argc, char* argv[]);

/**
 * @page salad-train Training mode of Salad
 *
 * @section SYNOPSIS
 *
 * salad train [options]
 *
 * @section DESCRIPTION
 *
 * Trains a detector based on n-grams extracted from the provided data which
 * might be given in various <em>input format</em>s. The
 * detector is here represented as Bloom filter of a specific <em>size</em>
 * that gets populated by the extracted n-grams. The <em>length</em> of the
 * n-gram is indicated by the variable n. For the type of n-gram it is
 * possible to choose between bit, byte and token n-grams indicated by the
 * <em>n-gram delimiter</em>.
 * Thereby \b Salad implements the general bag-of-words model.
 *
 * Bloom filters basically are associative bit arrays. Therefore, \b Salad is
 * restricted to the binary embedding of n-grams, that is, the representation of
 * the pure Boolean occurrence of such n-grams. The actual mapping of an n-gram
 * to the index within the bit array is achieved by hashing the n-gram value. To
 * do so \b Salad offers two different <em>hash sets</em>: (1) three fundamentally
 * different hash functions and (2) three differently seeded instances of the
 * murmur hash function.
 *
 * @section OPTIONS
 *
 * @subsection sec_ioops I/O Options:
 * @par -i, --input &lt;file&gt;
 * The input filename.
 *
 * @par -f, --input-format &lt;fmt&gt;
 * Sets the format of input. This option might be one of 'lines', 'files',
 * 'archive', 'network' or 'network-dump'. This depends on the configuration
 * Salad was compiled with -- cf. USE_ARCHIVES, USE_NETWORK, ALLOW_LIVE_TRAINING.
 *
 * @par     --input-filter &lt;regex&gt;
 * The regular expression for filtering input lines or filenames respectively
 * depending on the input format/ type used -- cf. USE_REGEX_FILTER.
 * This can be used to operate salad more easily in a 2-class setting and use
 * identical input files as used for embeddings created by sally.
 *
 * @par     --batch-size &lt;num&gt;
 * Sets the size of batches that are read and processed in one go. When
 * processing network streams this is automatically set to 1.
 *
 * @par -p,  --pcap-filter &lt;str&gt;
 * Filter expression for the PCAP library in case network data is processed
 * (Default: tcp). This option is only available if Salad was compiled with
 * network support -- cf. USE_NETWORK.
 *
 * @par -u,  --update-model
 * In case the specified output file exists and contains a valid model this
 * flag indicates that that model should be update rather than recreated from
 * scratch.
 *
 * @par -o,  --output &lt;file&gt;
 * The output filename.
 *
 * @subsection sec_featureops Feature Options:
 * @par -n, --ngram-len &lt;num&gt;
 * Set length of n-grams (Default: 3).
 *
 * @par -d, --ngram-delim &lt;delim&gt;
 * Set delimiters for the use of word/ token n-grams. If omitted or empty
 * byte n-grams are used.
 *
 * @par     --binary
 * Indicates to use bit n-grams rather than byte or token n-grams and
 * consequently, disables the --ngram-delim option.
 *
 * @par -s, --filter-size &lt;num&gt;
 * Set the size of the bloom filter as bits of the index (Default: 24).
 *
 * @par     --hash-set &lt;hashes&gt;
 * Set the hash set to be used: 'simple' or 'murmur' (Default: 'simple').
 *
 * @subsection sec_genericops Generic Options:
 * @par -e, --echo-params
 * Echo used parameters and settings.
 *
 * @par -q, --quiet
 * Suppress all output but warning and errors.
 *
 * @par -h, --help
 * Print the help screen.
 *
 * @section sec_copyright COPYRIGHT
 * \copydoc hidden_copyright
 */
const int _salad_train_(const config_t* const c);

/**
 * @page salad-predict Prediction mode of Salad
 *
 * @section SYNOPSIS
 *
 * salad predict [options]
 *
 * @section DESCRIPTION
 *
 * Predicts the anomaly score or the classification value respectively of the
 * provided data which might be given in various <em>input format</em>s.
 *
 * Whether the detector performs anomaly detection or classification depends on
 * the number of bloom filters given. If only one is specified \b Salad uses
 * that one in order to detect anomalies thereof. For two filters the matching
 * class is determined.
 *
 * Due to structure of the decision function <em>NaN</em> value may occur.
 * Therefore it might be necessary to specify a default value for these cases.
 *
 * @section OPTIONS
 *
 * @subsection sec_ioops I/O Options:
 * @par -i, --input &lt;file&gt;
 * The input filename.
 *
 * @par -f, --input-format &lt;fmt&gt;
 * Sets the format of input. This option might be one of 'lines', 'files',
 * 'archive', 'network' or 'network-dump'. This depends on the configure
 * Salad was compiled with -- cf. USE_ARCHIVES, USE_NETWORK, ALLOW_LIVE_TRAINING.
 *
 * @par     --input-filter &lt;regex&gt;
 * The regular expression for filtering input lines or filenames respectively
 * depending on the input format/ type used -- cf. USE_REGEX_FILTER.
 * This can be used to operate salad more easily in a 2-class setting and use
 * identical input files as used for embeddings created by sally.
 *
 * @par     --batch-size &lt;num&gt;
 * Sets the size of batches that are read and processed in one go. When
 * processing network streams this is automatically set to 1.
 *
 * @par -g, --group-input
 * Indicates that predictions for inputs in the same "group" should be
 * grouped as well.
 *
 * @par -p,  --pcap-filter &lt;str&gt;
 * Filter expression for the PCAP library in case network data is processed
 * (Default: tcp). This option is only available if Salad was compiled with
 * network support -- cf. USE_NETWORK.
 *
 * @par -b,  --bloom &lt;file&gt;
 * The bloom filter to be used.
 *
 * @par      --bad-bloom &lt;file&gt;
 * The bloom filter for the 2nd class (optional).
 *
 * @par -o,  --output &lt;file&gt;
 * The output filename.
 *
 * @subsection sec_featureops Feature Options:
 * @par -r,  --nan-str &lt;str&gt;
 * Set the string to be shown for NaN values.
 *
 * @subsection sec_genericops Generic Options:
 * @par -e, --echo-params
 * Echo used parameters and settings.
 *
 * @par -q, --quiet
 * Suppress all output but warning and errors.
 *
 * @par -h, --help
 * Print the help screen.
 *
 * @section sec_copyright COPYRIGHT
 * \copydoc hidden_copyright
 */
const int _salad_predict_(const config_t* const c);

/**
 * @page salad-stats Statistics mode of Salad
 *
 * @section SYNOPSIS
 *
 * salad stats [options]
 *
 * @section DESCRIPTION
 *
 * Provides statistical information of the specified Bloom filter. In particular
 * this currently is limited to the filter's saturation.
 *
 * @section OPTIONS
 *
 * @subsection sec_ioops I/O Options:
 * @par -b,  --bloom &lt;file&gt;
 * The bloom filter to be analyzed.
 *
 * @subsection sec_genericops Generic Options:
 * @par -h, --help
 * Print the help screen.
 *
 * @section sec_copyright COPYRIGHT
 * \copydoc hidden_copyright
 */
const int _salad_stats_(const config_t* const c);

/**
 * @page salad-inspect Inspection mode of Salad
 *
 * @section SYNOPSIS
 *
 * salad inspect [options]
 *
 * @section DESCRIPTION
 *
 * Performs a detailed inspection of the n-grams that would be extracted in the
 * course of processing the data using salad-train and the model
 * generated thereby. Specifically the following four values are extracted
 * for each sample:
 * - The number of new/ unseen n-grams
 * - The number of unique n-grams
 * - The total number of n-grams
 * - The length of the sample in number of bytes
 *
 * This in turn can be used to make a point how well and if anomaly detection is
 * suitable for this particular data set or classification might be the better
 * choice in this setting.
 *
 * @section OPTIONS
 *
 * @subsection sec_ioops I/O Options:
 * @par -i, --input &lt;file&gt;
 * The input filename.
 *
 * @par -f, --input-format &lt;fmt&gt;
 * Sets the format of input. This option might be one of 'lines', 'files',
 * 'archive', 'network' or 'network-dump'. This depends on the configure
 * Salad was compiled with -- cf. USE_ARCHIVES, USE_NETWORK, ALLOW_LIVE_TRAINING.
 *
 * @par     --input-filter &lt;regex&gt;
 * The regular expression for filtering input lines or filenames respectively
 * depending on the input format/ type used -- cf. USE_REGEX_FILTER.
 * This can be used to operate salad more easily in a 2-class setting and use
 * identical input files as used for embeddings created by sally.
 *
 * @par     --batch-size &lt;num&gt;
 * Sets the size of batches that are read and processed in one go. When
 * processing network streams this is automatically set to 1.
 *
 * @par -p,  --pcap-filter &lt;str&gt;
 * Filter expression for the PCAP library in case network data is processed
 * (Default: tcp). This option is only available if Salad was compiled with
 * network support -- cf. USE_NETWORK.
 *
 * @par -b,  --bloom &lt;file&gt;
 * The bloom filter to be used.
 *
 * @par -o,  --output &lt;file&gt;
 * The output filename.
 *
 * @subsection sec_featureops Feature Options:
 * @par -n, --ngram-len &lt;num&gt;
 * Set length of n-grams (Default: 3).
 *
 * @par -d, --ngram-delim &lt;delim&gt;
 * Set delimiters for the use of word/ token n-grams. If omitted or empty
 * byte n-grams are used.
 *
 * @par     --binary
 * Indicates to use bit n-grams rather than byte or token n-grams and
 * consequently, disables the --ngram-delim option.
 *
 * @par -s, --filter-size &lt;num&gt;
 * Set the size of the bloom filter as bits of the index (Default: 24).
 *
 * @par     --hash-set &lt;hashes&gt;
 * Set the hash set to be used: 'simple' or 'murmur' (Default: 'simple').
 *
 * @subsection sec_genericops Generic Options:
 * @par -e, --echo-params
 * Echo used parameters and settings.
 *
 * @par -h, --help
 * Print the help screen.
 *
 * @section sec_copyright COPYRIGHT
 * \copydoc hidden_copyright
 */
const int _salad_inspect_(const config_t* const c);

#ifdef TEST_SALAD
/**
 * @page salad-dbg (Unit) Testing of the implementation of Salad
 *
 * @section SYNOPSIS
 *
 * salad dbg [options]
 *
 * @section DESCRIPTION
 *
 * Performs a series of in-depth tests of \b Salad's implementation and the
 * libraries used.
 *
 * @section OPTIONS
 *
 * @subsection sec_ioops Test Options:
 * @par -m, --no-memcheck
 * Disable the memcheck (using valgrind).
 *
 * @par -h, --help
 * Print the help screen.
 *
 * @section sec_copyright COPYRIGHT
 * \copydoc hidden_copyright
 */
const int _salad_dbg_(const test_config_t* const c);
#endif

#endif /* MAIN_H_ */
