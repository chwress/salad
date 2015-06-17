/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2014, Christian Wressnegger
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

/**
 * @file
 */

#ifndef SALAD_COMMON_H_
#define SALAD_COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define DELIM_SIZE  	256
#define DELIM(delim)	uint8_t delim[DELIM_SIZE]
typedef uint8_t* const delimiter_array_t;
#define NO_DELIMITER	NULL

typedef size_t bitgram_t;
extern const int BITGRAM_SIZE;
extern const int BITGRAM_BITSIZE;

typedef bitgram_t ngram_mask_t;
extern const int MASK_BITSIZE;

/**
 * A generic representation of a set of delimiters that contains
 * a byte array "mask" as well as the string representation.
 */
typedef struct {
	char* str; //!< The string representation of the delimiters.
	DELIM(d); //!< The byte array "mask" that translates delimiter characters.
} delimiter_t;


#define EMPTY_DELIMITER_INITIALIZER { \
		.str = NULL, \
		.d = {0} \
}

/**
 * The preferred way of initializing the delimiter_t object/ struct.
 */
#define DELIMITER_T(d) delimiter_t d = EMPTY_DELIMITER_INITIALIZER
static const DELIMITER_T(EMPTY_DELIMITER);

void delimiter_init(delimiter_t* const d);
void delimiter_destroy(delimiter_t* const d);


typedef struct
{
	int use_tokens; //!< Whether or not to use word/ token n-grams rather than byte/ bit n-grams.
	delimiter_t delimiter; //!< The delimiter of the tokens in case token n-grams are used.
} saladcontext_t;


#define EMPTY_SALAD_CONTEXT_INITIALIZER { \
		.use_tokens= 0, \
		.delimiter = EMPTY_DELIMITER_INITIALIZER \
}

#define _(s)  ((saladcontext_t*) (s)->data)
#define __(s) (*(saladcontext_t*) (s).data)

/**
 * The preferred way of initializing the saladcontext_t object/ struct.
 */
#define SALADCONTEXT_T(m) saladcontext_t m = EMPTY_SALAD_CONTEXT_INITIALIZER
static const SALADCONTEXT_T(EMPTY_SALAD_CONTEXT);


/**
 * Converts the string representation of a set of delimiters to the
 * generic representation of delimiters as used by salad.
 *
 * @param[in] str The string representation of a set of delimiters
 * @param[out] out The generic representation of delimiters as used by salad.
 *
 * @return The input parameter of the string representation of the
 *         set of delimiters.
 */
const char* const to_delimiter(const char* const str, delimiter_t* const out);
/**
 * Converts the string representation of a set of delimiters to
 * corresponding byte array "mask".
 *
 * @param[in] s The string representation of a set of delimiters.
 * @param[out] out The byte array "mask" to transfer the string
 *                 representation to.
 */
void to_delimiter_array(const char* const s, delimiter_array_t out);
/**
 * Converts delimiters represented as byte array "mask" to the corresponding
 * human readable string representation.
 *
 * @param[in] delim The set of delimiters as byte array "mask".
 * @param[out] out The human readable string representation of the
 *                       set of delimiters.
 *
 * @return The human readable string representation of the set of delimiters.
 *         This only echos the output parameter \p out.
 */
const char* const delimiter_array_to_string(const delimiter_array_t delim, char** out);
/**
 * Converts the string representation of a binary n-gram delimiter
 * to the corresponding bit mask.
 *
 * @param[in] s The string representation of a binary n-gram delimiter.
 * @param[out] out the bit mask to transfer the string representation to.
 *
 * @returns A boolean indicator for whether or not the conversion succeeded.
 */
int to_ngram_mask(const char* const s, ngram_mask_t* const out);
/**
 * Converts a bit mask for binary n-grams to the corresponding human
 * readable string representation.
 *
 * @param[in] m The binary n-gram mask to be converted.
 * @param[out] out The human readable string representation of the binary
 *                 n-gram mask. This only echos the output parameter \p out.
 */
const char* const ngram_mask_to_string(const ngram_mask_t m, char** out);

#endif /* SALAD_COMMON_H_ */
