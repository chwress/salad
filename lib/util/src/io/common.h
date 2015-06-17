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

#ifndef INTERNAL_UTIL_IO_COMMON_H_
#define INTERNAL_UTIL_IO_COMMON_H_

#include <util/config.h>
#include <util/io.h>

const int all_filter(file_t* const f, const char* const pattern);
const int all_filter_ex(file_t* const f, const char* const pattern);
const int all_filter_close(file_t* const f);

const int    file_open(file_t* const f, const char* const filename, const char* mode, void *const p);
const int    file_meta(file_t* const f, const int group_input);
const size_t file_read(file_t* const f, dataset_t* const ds, const size_t num_lines);
const size_t file_read2(file_t* const f, dataset_t* const ds, const size_t chunk_size);
const size_t file_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t file_recv2(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t file_write(file_t* const f, const dataset_t* const ds, void* const usr);
const int    file_close(file_t* const f);
const int    file_close_ex(file_t* const f, int keep_metadata);
const int    file_close_itr(file_t* const f);

#ifdef USE_ARCHIVES
const int    archive_open(file_t* const f, const char* const filename, const char* mode, void *const p);
const int    archive_meta(file_t* const f, const int group_input);
const size_t archive_read(file_t* const f, dataset_t* const ds, const size_t num_files);
const size_t archive_read2(file_t* const f, dataset_t* const ds, const size_t chunk_size);
const size_t archive_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t archive_recv2(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t archive_write(file_t* const f, const dataset_t* const ds, void* const usr);
const int    archive_close(file_t* const f);
const int    archive_close_ex(file_t* const f, int keep_metadata);
#endif


#ifdef USE_NETWORK
const int    net_open(file_t* const f, const char* const filename, const char* mode, void *const p);
const int    net_meta(file_t* const f, const int group_input);
const int    net_filter(file_t* const f, const char* const pattern);
const size_t net_read(file_t* const f, dataset_t* const ds, const size_t num_streams);
const size_t net_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t net_write(file_t* const f, const dataset_t* const ds, void* const usr);
const int    net_close(file_t* const f);
#endif


#define READ_STUB(type, ITERATOR_END, ITERATOR_UNIT, ITERATOR_OK)                      \
const size_t type##_read(file_t* const f, dataset_t* const ds, const size_t num_files) \
{                                                                                      \
	assert(f != NULL);                                                                 \
	assert(ds != NULL);                                                                \
	assert(num_files <= ds->capacity);                                                 \
                                                                                       \
	if (ds->capacity <= 0 || ds->data == NULL)                                         \
	{                                                                                  \
		return 0;                                                                      \
	}                                                                                  \
                                                                                       \
	ds->n = 0; /* Overwrite everything! */                                             \
	for (size_t i = 0; i < MIN(num_files, ds->capacity); i++)                          \
	{                                                                                  \
		data_t* const data = &ds->data[ds->n];                                         \
		const int ret = type##_read_next(f, data, ITERATOR_READALL);                   \
		/* assert(ret == ITERATOR_END || ret == ITERATOR_UNIT); */                     \
                                                                                       \
		if (ret == ITERATOR_END)                                                       \
		{                                                                              \
			break;                                                                     \
		}                                                                              \
		ds->n++;                                                                       \
	}                                                                                  \
	return ds->n;                                                                      \
}

#define READ2_STUB(type, ITERATOR_END, ITERATOR_UNIT, ITERATOR_OK)                                     \
const size_t type##_read2(file_t* const f, dataset_t* const ds, const size_t chunk_size)               \
{	                                                                                                   \
	assert(f != NULL);                                                                                 \
	assert(ds != NULL);                                                                                \
	                                                                                                   \
	size_t num_units = 0, size = 0;                                                                    \
	                                                                                                   \
	int ret = ITERATOR_OK;                                                                             \
	while (size < chunk_size)                                                                          \
	{                                                                                                  \
		if (ds->n >= ds->capacity)                                                                     \
		{                                                                                              \
			ds->capacity *= 2;                                                                         \
			ds->data = (data_t*) realloc(ds->data, ds->capacity * sizeof(data_t*));                    \
		}                                                                                              \
		                                                                                               \
		ret = type##_read_next(f, &ds->data[ds->n], chunk_size -size);                                 \
		if (ret == ITERATOR_END) break;                                                                \
		if (ret == ITERATOR_UNIT)                                                                      \
		{                                                                                              \
			num_units++;                                                                               \
		}                                                                                              \
		size += ds->data[ds->n].len;                                                                   \
		ds->n++;                                                                                       \
	}                                                                                                  \
	                                                                                                   \
	return size;                                                                                       \
}


#endif /* INTERNAL_UTIL_IO_COMMON_H_ */
