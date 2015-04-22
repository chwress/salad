/*
 * MurmurHash2, 64-bit versions, by Austin Appleby
 * --
 * The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
 * and endian-ness issues if used across multiple platforms.
 */

#ifndef UTIL_MURMUR_H
#define UTIL_MURMUR_H

#include <stdint.h>

uint32_t MurmurHash2(const void *key, int32_t len, uint32_t seed);
uint64_t MurmurHash64B(const void *key, int32_t len, uint32_t seed);

#endif /* UTIL_MURMUR_H */
