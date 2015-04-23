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

/**
 * @file salad.h
 */

#ifndef SALAD_SALAD_H_
#define SALAD_SALAD_H_

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * The type identifier for the model used by salad.
 */
typedef enum
{
	/*!
	 * Bloom filter as described by Burton H. Bloom in "Space/Time
	 * Trade-offs in Hash Coding with Allowable Errors" (1970),
	 * Communications of the ACM 13 (7): 422–426
	 */
	SALAD_MODEL_BLOOMFILTER,
	SALAD_MODEL_NOTSPECIFIED //!< An unspecified/ not initialized model.
} saladmodel_type_t;

/**
 * The model to be used by salad.
 */
typedef struct
{
	void* x; //!< The domain-specific data structure of the model.
	saladmodel_type_t type; //!< The type identifier of the model.
} saladmodel_t;

/**
 * The common object defining models and its general parameterization/
 * specification as used by salad.
 */
typedef struct
{
	saladmodel_t model; //!< The model to be used by salad.

	size_t ngram_length; //!< The n-gram length.
	int use_tokens; //!< Whether or not to use world/ token n-grams rather than byte n-grams.
	int as_binary; //!> Whether or not to evaluate n-grams on a binary level.
	delimiter_t delimiter; //!< The delimiter of the tokens in case token n-grams are used.
} salad_t;

/**
 * A generic representation of input data as used by salad.
 */
typedef struct
{
	char* buf; //!< The raw data as array of characters.
	size_t len; //!< The length of the raw data.
} saladdata_t;


#define EMPTY_SALAD_OBJECT_INITIALIZER { \
		.model = {NULL, SALAD_MODEL_NOTSPECIFIED}, \
		.ngram_length = 0, \
		.use_tokens= 0, \
		.as_binary = 0, \
		.delimiter = EMPTY_DELIMITER_INITIALIZER \
}

/**
 * The preferred way of initializing the salad_t object/ struct.
 */
#define SALAD_T(m) salad_t m = EMPTY_SALAD_OBJECT_INITIALIZER
static const SALAD_T(EMPTY_SALAD_OBJECT);


/**
 * Initialize the given salad object.
 *
 * @param[inout] s The salad object to be initialized.
 */
PUBLIC void salad_init(salad_t* const s);
/**
 * Allocates the specified number of bytes and write the corresponding
 * pointer to the given salad data struct.
 *
 * @param[out] d The salad data struct to write the pointer of the newly
 *               allocated memory region to.
 * @param[in] n The number of byte to allocate.
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_allocate(saladdata_t* const d, const size_t n);
/**
 * Set the model of the given salad object to a bloom filter
 * with the given parameters.
 *
 * @param[inout] s The salad object to be modified.
 * @param[in] filter_size The size of the bloom filter.
 * @param[in] hashset The hash set to be used for the bloom filter.
 *                    Possible values are: "simple" & "murmur".
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_set_bloomfilter(salad_t* const s, const unsigned int filter_size, const char* const hashset);
/**
 * Use binary n-grams rather than n-grams based on character/bytes.
 *
 * ATTENTION! This also changes how the n-gram delimiter interpreted.
 * If set it is handled as binary string!
 *
 * @param[inout] s The salad object to be modified.
 * @param[in] b A boolean indicator for whether or not to use binary n-grams.
 */
PUBLIC void salad_use_binary_ngrams(salad_t* const s, const int b);
/**
 * Set the n-gram delimiter that should be used for the
 * model. If no delimiter is set byte n-grams are used otherwise
 * salad extract tokens that are split at the given delimiters.
 *
 * If additionally the binary "flag" is set (salad_use_binary_ngrams(.,.))
 * salad extracts n-grams based on bits. This holds true for "standard"
 * n-grams as well as those based on tokens. In this case the delimiter
 * is interpreted as binary string and therefore, may only contain the
 * characters '0' & '1'.
 *
 * @param[inout] s The salad object to be modified.
 * @param[in] d The delimiter that should be used for extracting n-grams.
 */
PUBLIC void salad_set_delimiter(salad_t* const s, const char* const d);
/**
 * Set the n-gram length that should be used for the model.
 *
 * @param[inout] s The salad object to be modified.
 * @param[in] n The n-gram length.
 */
PUBLIC void salad_set_ngramlength(salad_t* const s, const size_t n);
/**
 * Destroys a salad data object and frees all allocated memory.
 *
 * @param[inout] d The salad data object to be destroyed.
 */
PUBLIC void salad_destroy_data(saladdata_t* const d);
/**
 * Destroys a salad object and frees all the resources (e.g. memory)
 * it was using.
 *
 * @param[inout] s The salad object to be destroyed.
 */
PUBLIC void salad_destroy(salad_t* const s);

/**
 * Trains a model based on n-grams extracted from the provided data.
 *
 * @param[inout] s The salad object to be modified.
 * @param[in] data The input data to processed.
 * @param[in] n The number of data elements in the input as defined
 *              by parameter \p data.
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_train(salad_t* const s, const saladdata_t* const data, const size_t n);
/**
 * Predicts the anomaly score or the classification value respectively
 * of the provided data.
 *
 * @param[inout] s The salad object to be modified.
 * @param[in] data The input data to processed.
 * @param[in] n The number of data elements in the input as defined
 *              by parameter \p data.
 * @param[out] out An array of size \p n to write the resulting scores
 *                 anomaly scores to.
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_predict_ex(salad_t* const s, const saladdata_t* const data, const size_t n, double* const out);
/**
 * Predicts the anomaly score or the classification value respectively
 * of the provided data.
 *
 * @param[inout] s The salad object to be modified.
 * @param[in] data The input data to processed.
 * @param[in] n The number of data elements in the input as defined
 *              by parameter \p data.
 *
 * @return An array of size \p n to holding the resulting scores
 *         anomaly scores to or NULL in case of an error. ATTENTION! The
 *         user is responsible for freeing the allocated array of doubles.
 */
PUBLIC const double* const salad_predict(salad_t* const s, const saladdata_t* const data, const size_t n);

/**
 * Checks whether the specification of the given salad models
 * differentiates or not.
 *
 * @param[in] a The first salad object to be compared.
 * @param[in] b The second salad object to be compared.
 *
 * @return A boolean indicator for whether the given models differ or not.
 */
PUBLIC const int salad_spec_diff(const salad_t* const a, const salad_t* const b);
/**
 * Loads a salad model and its specification from the file with the
 * given name.
 *
 * @param[in] filename The name of the file that contains the salad spec.
 * @param[out] out The salad object to write the model specification to.
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_from_file(const char* const filename, salad_t* const out);
/**
 * Loads a salad model and its specification from the file with the
 * given name.
 *
 * @param[in] f The FILE object of the file that contains the salad spec.
 * @param[out] out The salad object to write the model specification to.
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_from_file_ex(FILE* const f, salad_t* const out);
/**
 * Write a salad model and its specification to the file with the given name.
 *
 * @param[in] s The salad object to written to the file.
 * @param[in] filename The name of the file to write the salad object to.
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_to_file(const salad_t* const s, const char* const filename);
/**
 * Write a salad model and its specification to the file with the given name.
 *
 * @param[in] s The salad object to written to the file.
 * @param[in] f The FILE object of the file to write the salad object to.
 *
 * @return An error indicator for whether the operation was
 *         successful or not. Zero means that that the operation
 *         was successful anything else indicates a particular error.
 */
PUBLIC const int salad_to_file_ex(const salad_t* const s, FILE* const f);

#ifdef __cplusplus
}
#endif

#endif /* SALAD_SALAD_H_ */
