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

#ifndef SALAD_ANAGRAM_EX_H_
#define SALAD_ANAGRAM_EX_H_

/**
 * A generic macro for the call of the bloomize<X>_ex<Y> functions.
 * The checks for X and Y are completely static and can be resolved
 * at compilation time. Hence, they will be optimized away and do
 * not influence runtime badly.
 */
#define BLOOMIZE_EX_(Y, X, bloom1, bloom2, str, len, n, delim, weights, stats)        \
{	                                                                                  \
	BLOOM* const _bloom1_ = bloom1;                                                   \
	BLOOM* const _bloom2_ = bloom2;                                                   \
	const char* const _str_ = str;                                                    \
	const size_t _len_ = len;                                                         \
	const size_t _n_ = n;                                                             \
	const delimiter_array_t _delim_ = delim;                                          \
	const vec_t* const _weights_ = weights;                                           \
	bloomize_stats_t* const _stats_ = stats;                                          \
	                                                                                  \
	switch (X)                                                                        \
	{                                                                                 \
	case 'b':                                                                         \
		switch (Y)                                                                    \
		{	                                                                          \
		case 2:                                                                       \
			bloomizeb_ex2(_bloom1_, _str_, _len_, _n_, _weights_);                    \
			break;                                                                    \
		case 3:                                                                       \
			bloomizeb_ex3(_bloom1_, _bloom2_, _str_, _len_, _n_, _stats_);            \
			break;                                                                    \
		case 4:                                                                       \
			bloomizeb_ex4(_bloom1_, _bloom2_, _str_, _len_, _n_, _stats_);            \
			break;                                                                    \
		default:                                                                      \
			bloomizeb_ex(_bloom1_, _str_, _len_, _n_);                                \
			break;                                                                    \
		}	                                                                          \
		break;                                                                        \
	case 'w':                                                                         \
		switch (Y)                                                                    \
		{	                                                                          \
		case 2:                                                                       \
			bloomizew_ex2(_bloom1_, _str_, _len_, _n_, _delim_, _weights_);           \
			break;                                                                    \
		case 3:                                                                       \
			bloomizew_ex3(_bloom1_, _bloom2_, _str_, _len_, _n_, _delim_, _stats_);   \
			break;                                                                    \
		case 4:                                                                       \
			bloomizew_ex4(_bloom1_, _bloom2_, _str_, _len_, _n_, _delim_, _stats_);   \
			break;                                                                    \
		default:                                                                      \
			bloomizew_ex(_bloom1_, _str_, _len_, _n_, _delim_);                       \
			break;                                                                    \
		}	                                                                          \
		break;                                                                        \
	default:                                                                          \
		switch (Y)                                                                    \
		{	                                                                          \
		case 2:                                                                       \
			bloomize_ex2(_bloom1_, _str_, _len_, _n_, _weights_);                     \
		case 3:                                                                       \
			bloomize_ex3(_bloom1_, _bloom2_, _str_, _len_, _n_, _stats_);             \
			break;                                                                    \
		case 4:                                                                       \
			bloomize_ex4(_bloom1_, _bloom2_, _str_, _len_, _n_, _stats_);             \
			break;                                                                    \
		default:                                                                      \
			bloomize_ex(_bloom1_, _str_, _len_, _n_);                                 \
			break;                                                                    \
		}	                                                                          \
		break;                                                                        \
	}                                                                                 \
}

/**
 * A generic macro for the call of the bloomize<X>_ex functions.
 */
#define BLOOMIZE_EX(X, bloom, str, len, n, delim)                                     \
{	                                                                                  \
	BLOOMIZE_EX_(1, X, bloom, NULL, str, len, n, delim, NULL, NULL);                  \
}

/**
 * A generic macro for the call of the bloomize<X>_ex2 functions.
 */
#define BLOOMIZE_EX2(X, bloom, str, len, n, delim, weights)                           \
{	                                                                                  \
	BLOOMIZE_EX_(2, X, bloom, NULL, str, len, n, delim, weights, NULL);               \
}

/**
 * A generic macro for the call of the bloomize<X>_ex3 functions.
 */
#define BLOOMIZE_EX3(X, bloom, bloom2, str, len, n, delim, stats)                     \
{	                                                                                  \
	BLOOMIZE_EX_(3, X, bloom, bloom2, str, len, n, delim, NULL, stats);               \
}

/**
 * A generic macro for the call of the bloomize<X>_ex4 functions.
 */
#define BLOOMIZE_EX4(X, bloom, bloom2, str, len, n, delim, stats)                     \
{	                                                                                  \
	BLOOMIZE_EX_(4, X, bloom, bloom2, str, len, n, delim, NULL, stats);               \
}

#endif /* SALAD_ANAGRAM_EX_H_ */
