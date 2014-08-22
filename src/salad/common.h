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

#include <config.h>

#include <stddef.h>
#include <stdint.h>

#if defined _WIN32 || defined __CYGWIN__
  #define PUBLIC __declspec(dllexport)
#else
  #if __GNUC__ >= 4
    #define PUBLIC __attribute__ ((visibility ("default")))
  #else
    #define PUBLIC
  #endif
#endif

#define DELIM_SIZE  	 256
#define DELIM(delim)	 uint8_t delim[DELIM_SIZE]
typedef uint8_t* const delimiter_array_t;

/**
 * A generic representation of a set of delimiters that contains
 * a byte array "mask" as well as the string representation.
 */
typedef struct {
	DELIM(d); //!< The byte array "mask" that translates delimiter characters.
	char* str; //!< The string representation of the delimiters.
} delimiter_t;


#define EMPTY_DELIMITER_INITIALIZER { \
		.d = {0}, \
		.str = "" \
}

/**
 * The preferred way of initializing the delimiter_t object/ struct.
 */
#define DELIMITER_T(d) delimiter_t d = EMPTY_DELIMITER_INITIALIZER
static const DELIMITER_T(EMPTY_DELIMITER);


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
 * Converts string representation of a set of delimiters to corresponding
 * byte array "mask".
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
 *         This only echos the output parameters \p out.
 */
const char* const delimiter_array_to_string(const delimiter_array_t delim, char** out);


#endif /* SALAD_COMMON_H_ */
