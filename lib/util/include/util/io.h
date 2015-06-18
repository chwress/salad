/*
 * libutil - Yet Another Utility Library
 * Copyright (c) 2012-2015, Christian Wressnegger
 * --
 * This file is part of the library libutil.
 *
 * libutil is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libutil is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/**
 * @file
 */

#ifndef UTIL_IO_H_
#define UTIL_IO_H_

#include <stdlib.h>
#include <stdio.h>
#include <util/config.h>
#include "util.h"

#ifdef USE_ARCHIVES
#ifdef USE_NETWORK
typedef enum { LINES, FILES, ARCHIVE, NETWORK, NETWORK_DUMP } iomode_t;
#else
typedef enum { LINES, FILES, ARCHIVE } iomode_t;
#endif
#else
#ifdef USE_NETWORK
typedef enum { LINES, FILES, NETWORK, NETWORK_DUMP } iomode_t;
#else
typedef enum { LINES, FILES } iomode_t;
#endif
#endif

const char* const iomode_to_string(const iomode_t m);
const iomode_t to_iomode(const char* const str);
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
	size_t start;
	size_t end;
} slice_t;

#define SLICE_LEN(s) ((s).end -(s).start)

typedef struct {
	slice_t* x;
	size_t n;
	size_t capacity;
} slices_t;


inline void slices_resize(slices_t* const slices, const size_t new_size)
{
	slices->x = (slice_t*) realloc(slices->x, new_size);
	slices->capacity = new_size;
	slices->n = MIN(slices->n, new_size);
}

inline void slices_grow(slices_t* const slices)
{
	if (slices->n == slices->capacity)
	{
		slices_resize(slices, slices->capacity *2);
	}
}

void destroy_slices(slices_t* const c);
void free_slices(slices_t* c);

#ifdef GROUPED_INPUT
typedef struct
{
	char* name;
	size_t n;
} group_t;
#endif

#if defined(EXTENDED_METADATA) && defined(GROUPED_INPUT)
#define MAINTAIN_METADATA
#endif

#ifdef MAINTAIN_METADATA
typedef struct
{
#ifdef EXTENDED_METADATA
		const char* filename;
#endif
#ifdef GROUPED_INPUT
		group_t* group;
#endif
} metaref_t;
#endif

typedef struct
{
		char* buf;
		size_t len;
		slices_t slices;
#ifdef MAINTAIN_METADATA
		metaref_t meta;
#endif
} data_t;

void data_free(data_t* const d);
void data_destroy(data_t* const d);

typedef struct
{
	const char* filename;
	size_t num_items;
	size_t total_size;

#ifdef EXTENDED_METADATA
	char** filenames;
#endif

#ifdef GROUPED_INPUT
	group_t* groups;
	size_t num_groups;
#endif
} metadata_t;

void metadata_destroy(metadata_t* const meta);
void metadata_free(metadata_t* const meta);

typedef struct
{
	data_t* data;
	size_t capacity;
	size_t n;
} dataset_t;

void dataset_free(dataset_t* const ds);
void dataset_destroy(dataset_t* const ds);

#ifdef USE_REGEX_FILTER
#include <regex.h>
#endif

static const char* const FILE_IO_READ = "r";
static const char* const FILE_IO_WRITE = "w";
typedef enum { FILE_IOMODE_READ = 1, FILE_IOMODE_WRITE = 2 } file_iomode_t;

const file_iomode_t to_fileiomode(const char* const s);
const char* const fileiomode_tostring(const file_iomode_t mode);

typedef struct
{
	FILE* fd;
	file_iomode_t mode;
	void* data;
#ifdef USE_REGEX_FILTER
	regex_t filter;
#endif

	int is_device;
	metadata_t meta;
} file_t;

typedef const int(*FN_OPEN)(file_t* const f, const char* const filename, const char* mode, void *const p);
typedef const int (*FN_META)(file_t* const f, const int group_input);
typedef const int (*FN_FILTER)(file_t* const f, const char* const pattern);
typedef const size_t (*FN_READ)(file_t* const f, dataset_t* const ds, const size_t num_files);
typedef const size_t (*FN_READ2)(file_t* const f, dataset_t* const ds, const size_t chunk_size);
typedef const int (*FN_DATA)(data_t* data, const size_t n, void* const usr);
typedef const size_t (*FN_RECV)(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
typedef const size_t (*FN_RECV2)(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
typedef const size_t (*FN_WRITE)(file_t* const f, const dataset_t* const ds, void* const usr);
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

extern void* const REOPEN;

typedef struct
{
	FN_OPEN open;
	FN_META meta;
	FN_FILTER filter;
	FN_READ read; FN_READ2 read2;
	FN_RECV recv; FN_RECV2 recv2;
	FN_WRITE write;
	FN_CLOSE close;
} data_processor_t;

const data_processor_t* const to_dataprocessor(const iomode_t m);


#endif /* UTIL_IO_H_ */
