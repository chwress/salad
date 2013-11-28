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

#ifndef UTIL_IO_H_
#define UTIL_IO_H_

#include <stdlib.h>
#include <stdio.h>

#include "../config.h"


#ifdef USE_ARCHIVES
#ifdef USE_NETWORK
typedef enum { LINES, FILES, ARCHIVE, NETWORK, NETWORK_DUMP } io_mode_t;
#else
typedef enum { LINES, FILES, ARCHIVE } io_mode_t;
#endif
#else
#ifdef USE_NETWORK
typedef enum { LINES, FILES, NETWORK, NETWORK_DUMP } io_mode_t;
#else
typedef enum { LINES, FILES } io_mode_t;
#endif
#endif

const char* const to_string(const io_mode_t m);
const io_mode_t to_iomode(const char* const str);
const int is_valid_iomode(const char* const str);

#ifdef USE_ARCHIVES
#ifdef USE_NETWORK
	#define VALID_IOMODES "'lines', 'files', 'archive', 'network' or 'network-dump'"
#else
	#define VALID_IOMODES "'lines', 'files' or 'archive'"
#endif
#else
#ifdef USE_NETWORK
	#define VALID_IOMODES "'lines', 'files', 'network' or 'network-dump'"
#else
	#define VALID_IOMODES "'lines' or 'files'"
#endif
#endif

typedef struct
{
	char* buf;
	size_t len;
} data_t;

void data_free(data_t* const d);
void data_destroy(data_t* const d);

typedef struct
{
	FILE* fd;
	void* data;
} file_t;

typedef const int(*FN_OPEN)(file_t* const f, const char* const filename, void *const p);
typedef const size_t (*FN_READ)(file_t* const f, data_t* data, const size_t numLines);
typedef const int (*FN_DATA)(data_t* data, const size_t n, void* const usr);
typedef const size_t (*FN_RECV)(file_t* const f, FN_DATA callback, void* const usr);
typedef const int(*FN_CLOSE)(file_t* const f);


typedef struct
{
	const char* error_msg;
} io_param_t;

#ifdef USE_NETWORK
typedef struct
{
	const int is_device;
	const char* const pcap_filter;
	const char* error_msg;
} net_param_t;
#endif


#define BATCH_SIZE 1000

typedef struct
{
	FN_OPEN open;
	FN_READ read;
	FN_RECV recv;
	FN_CLOSE close;
} data_processor_t;

const data_processor_t* const to_dataprocssor(const io_mode_t m);


#endif /* UTIL_IO_H_ */
