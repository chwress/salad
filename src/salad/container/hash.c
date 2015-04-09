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

#include "hash.h"

uint32_t sax_hash(const char* const key)
{
	uint32_t h = 0;
	const unsigned char* x = (const unsigned char*) key;

	while(*x)
	{
		h^=(h<<5)+(h>>2)+*x++;
	}
	return h;
}

uint32_t sax_hash_n(const char* const key, const size_t len)
{
	uint32_t h = 0;
	const unsigned char* x = (const unsigned char*) key;

	for(size_t i = 0; i < len; i++)
	{
		h^=(h<<5)+(h>>2)+*x++;
	}
	return h;
}

uint32_t sdbm_hash(const char* const key)
{
	uint32_t h = 0;
	const unsigned char* x = (const unsigned char*) key;

	while(*x)
	{
		h=*x++ + (h<<6) + (h<<16) - h;
	}
	return h;
}

uint32_t sdbm_hash_n(const char* const key, const size_t len)
{
	uint32_t h = 0;
	const unsigned char* x = (const unsigned char*) key;

	for(size_t i = 0; i < len; i++)
	{
		h=*x++ + (h<<6) + (h<<16) - h;
	}
	return h;
}


uint32_t bernstein_hash(const char* const key)
{
	uint32_t h = 0;
	const unsigned char* x = (const unsigned char*) key;

	while(*x)
	{
		h=33*h + *x++;
	}
	return h;
}


uint32_t bernstein_hash_n(const char* const key, const size_t len)
{
	uint32_t h = 0;
	const unsigned char* x = (const unsigned char*) key;

	for(size_t i = 0; i < len; i++)
	{
		h=33*h + *x++;
	}
	return h;
}


#include <util/murmur.h>
#include <string.h>

uint32_t murmur_hash0(const char* const key)
{
	return MurmurHash2(key, strlen(key), 0x428a2f98); // SHA-256 k[0]
}

uint32_t murmur_hash0_n(const char* const key, const size_t len)
{
	return MurmurHash2(key, len, 0x428a2f98);
}

uint32_t murmur_hash1(const char* const key)
{
	return MurmurHash2(key, strlen(key), 0x71374491); // SHA-256 k[1]
}

uint32_t murmur_hash1_n(const char* const key, const size_t len)
{
	return MurmurHash2(key, len, 0x71374491);
}

uint32_t murmur_hash2(const char* const key)
{
	return MurmurHash2(key, strlen(key), 0xb5c0fbcf); // SHA-256 k[2]
}

uint32_t murmur_hash2_n(const char* const key, const size_t len)
{
	return MurmurHash2(key, len, 0xb5c0fbcf);
}


