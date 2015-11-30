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
 * @file
 */

#ifndef SALAD_ANALYZE_EX_H_
#define SALAD_ANALYZE_EX_H_

/**
 * A generic macro for the call of the bloomize<X>_ex<Y> functions.
 * The checks for X and Y are completely static and can be resolved
 * at compilation time. Hence, they will be optimized away and do
 * not influence runtime badly.
 */
#define BLOOMIZE_EX_(Y, X, bloom1, bloom2, str, len, n, delim, weights, stats)          \
{	                                                                                    \
	BLOOM* const BEX_bloom1 = bloom1;                                                   \
	BLOOM* const BEX_bloom2 = bloom2;                                                   \
	const char* const BEX_str = str;                                                    \
	const size_t BEX_len = len;                                                         \
	const size_t BEX_n = n;                                                             \
	const delimiter_array_t BEX_delim = delim;                                          \
	const vec_t* const BEX_weights = weights;                                           \
	bloomize_stats_t* const BEX_stats = stats;                                          \
	                                                                                    \
	switch (X)                                                                          \
	{                                                                                   \
	case 'b':                                                                           \
		switch (Y)                                                                      \
		{	                                                                            \
		case 2:                                                                         \
			bloomizeb_ex2(BEX_bloom1, BEX_str, BEX_len, BEX_n, BEX_weights);            \
			break;                                                                      \
		case 3:                                                                         \
			bloomizeb_ex3(BEX_bloom1, BEX_bloom2, BEX_str, BEX_len, BEX_n, BEX_stats);  \
			break;                                                                      \
		case 4:                                                                         \
			bloomizeb_ex4(BEX_bloom1, BEX_bloom2, BEX_str, BEX_len, BEX_n, BEX_stats);  \
			break;                                                                      \
		default:                                                                        \
			bloomizeb_ex(BEX_bloom1, BEX_str, BEX_len, BEX_n);                          \
			break;                                                                      \
		}	                                                                            \
		break;                                                                          \
	case 'w':                                                                           \
		switch (Y)                                                                      \
		{	                                                                            \
		case 2:                                                                         \
			bloomizew_ex2(BEX_bloom1, BEX_str, BEX_len, BEX_n, BEX_delim, BEX_weights); \
			break;                                                                      \
		case 3:                                                                         \
			bloomizew_ex3(BEX_bloom1, BEX_bloom2, BEX_str, BEX_len, BEX_n, BEX_delim, BEX_stats); \
			break;                                                                      \
		case 4:                                                                         \
			bloomizew_ex4(BEX_bloom1, BEX_bloom2, BEX_str, BEX_len, BEX_n, BEX_delim, BEX_stats); \
			break;                                                                      \
		default:                                                                        \
			bloomizew_ex(BEX_bloom1, BEX_str, BEX_len, BEX_n, BEX_delim);               \
			break;                                                                      \
		}	                                                                            \
		break;                                                                          \
	default:                                                                            \
		switch (Y)                                                                      \
		{	                                                                            \
		case 2:                                                                         \
			bloomize_ex2(BEX_bloom1, BEX_str, BEX_len, BEX_n, BEX_weights);             \
		case 3:                                                                         \
			bloomize_ex3(BEX_bloom1, BEX_bloom2, BEX_str, BEX_len, BEX_n, BEX_stats);   \
			break;                                                                      \
		case 4:                                                                         \
			bloomize_ex4(BEX_bloom1, BEX_bloom2, BEX_str, BEX_len, BEX_n, BEX_stats);   \
			break;                                                                      \
		default:                                                                        \
			bloomize_ex(BEX_bloom1, BEX_str, BEX_len, BEX_n);                           \
			break;                                                                      \
		}	                                                                            \
		break;                                                                          \
	}                                                                                   \
}

/**
 * A generic macro for the call of the bloomize<X>_ex functions.
 */
#define BLOOMIZE_EX(X, bloom, str, len, n, delim)                                       \
{	                                                                                    \
	BLOOMIZE_EX_(1, (X), (bloom), (NULL), (str), (len), (n), (delim), NULL, NULL);      \
}

/**
 * A generic macro for the call of the bloomize<X>_ex2 functions.
 */
#define BLOOMIZE_EX2(X, bloom, str, len, n, delim, weights)                             \
{	                                                                                    \
	BLOOMIZE_EX_(2, (X), (bloom), (NULL), (str), (len), (n), (delim), (weights), NULL); \
}

/**
 * A generic macro for the call of the bloomize<X>_ex3 functions.
 */
#define BLOOMIZE_EX3(X, bloom, bloom2, str, len, n, delim, stats)                       \
{	                                                                                    \
	BLOOMIZE_EX_(3, (X), (bloom), (bloom2), (str), (len), (n), (delim), NULL, (stats)); \
}

/**
 * A generic macro for the call of the bloomize<X>_ex4 functions.
 */
#define BLOOMIZE_EX4(X, bloom, bloom2, str, len, n, delim, stats)                       \
{	                                                                                    \
	BLOOMIZE_EX_(4, (X), (bloom), (bloom2), (str), (len), (n), (delim), NULL, (stats)); \
}

#endif /* SALAD_ANALYZE_EX_H_ */
