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

#include <util/io.h>
#include <util/util.h>
#include <util/log.h>

#include "io/common.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>



const char* const iomode_to_string(const iomode_t m)
{
	switch (m)
	{
	case IOMODE_LINES:        return "lines";
	case IOMODE_FILES:        return "files";
#ifdef USE_ARCHIVES
	case IOMODE_ARCHIVE:      return "archive";
#endif
#ifdef USE_NETWORK
	case IOMODE_NETWORK:      return "network";
	case IOMODE_NETWORK_DUMP: return "network-dump";
#endif
	default:           return "unknown";
	}
}

const iomode_t to_iomode(const char* const str)
{
	switch (cmp(str, "lines", "files", "archive", "network", "network-dump", NULL))
	{
	case 0:  return IOMODE_LINES;
	case 1:  return IOMODE_FILES;
#ifdef USE_ARCHIVES
	case 2:  return IOMODE_ARCHIVE;
#endif
#ifdef USE_NETWORK
	case 3:  return IOMODE_NETWORK;
	case 4:  return IOMODE_NETWORK_DUMP;
#endif
	// Okay that actually cannot happen ;)
	default: return IOMODE_FILES;
	}
}

const int is_valid_iomode(const char* const str)
{
	switch (cmp(str, "lines", "files", "archive", "network", "network-dump", NULL))
	{
#ifdef USE_NETWORK
	case 4:
	case 3:
#endif
#ifdef USE_ARCHIVES
	case 2:
#endif
	case 1:
	case 0:  return TRUE;
	default: return FALSE;
	}
}

void metadata_destroy(metadata_t* const meta)
{
	assert(meta != NULL);

#ifdef EXTENDED_METADATA
	if (meta->filenames != NULL)
	{
		for (size_t i = 0; i < meta->num_items; i++)
		{
			if (meta->filenames[i] != NULL)
			{
				free(meta->filenames[i]);
			}
		}
		free(meta->filenames);
	}
#endif

#ifdef GROUPED_INPUT
	if (meta->groups != NULL)
	{
		for (size_t i = 0; i < meta->num_groups; i++)
		{
			free(meta->groups[i].name);
		}
		free(meta->groups);
	}
#endif
}

void metadata_free(metadata_t* const meta)
{
	assert(meta != NULL);
	metadata_destroy(meta);
	free(meta);
}


extern void slices_resize(slices_t* const slices, const size_t new_size);
extern void slices_grow(slices_t* const slices);

void destroy_slices(slices_t* const c)
{
	free(c->x);
	c->x = NULL;
	c->capacity = 0;
	c->n = 0;
}

void free_slices(slices_t* s)
{
	destroy_slices(s);
	free(s);
}

void data_free(data_t* const d)
{
	assert(d != NULL);
	free(d->buf);

	destroy_slices(&d->slices);
}

void data_destroy(data_t* const d)
{
	assert(d != NULL);
	data_free(d);
	free(d);
}

void dataset_free(dataset_t* const ds)
{
	assert(ds != NULL);

	assert((ds->n == 0 && ds->capacity == 0 && ds->data == NULL)
		|| (ds->n  > 0 && ds->capacity >= ds->n && ds->data != NULL));

	if (ds->capacity > 0)
	{
		for (size_t i = 0; i < ds->n; i++)
		{
			data_free(&ds->data[i]);
		}
		free(ds->data);
	}
}

void dataset_destroy(dataset_t* const ds)
{
	assert(ds != NULL);
	dataset_free(ds);
	free(ds);
}

const file_iomode_t to_fileiomode(const char* const s)
{
	switch (cmp(s, FILE_IO_READ, FILE_IO_WRITE, NULL))
	{
	case 0:  return FILE_IOMODE_READ;
	case 1:  return FILE_IOMODE_WRITE;
	default: return 0;
	}
}

const char* const fileiomode_tostring(file_iomode_t mode)
{
	switch (mode)
	{
	case FILE_IOMODE_READ:  return FILE_IO_READ;
	case FILE_IOMODE_WRITE: return FILE_IO_WRITE;
	default:                return NULL;
	}
}

static struct {
	const int b;
} _REOPEN = {1};

void* const REOPEN = &_REOPEN;


data_processor_t dp_lines   = {
		.open = file_open,
		.meta = file_meta,
		.filter = all_filter,
		.read = file_read, .read2 = file_read2,
		.recv = file_recv, .recv2 = file_recv2,
		.write = file_write,
		.close = file_close };


data_processor_t dp_files   = {
		.open = NULL,
		.meta = NULL,
		.filter = all_filter,
		.read = NULL, .read2 = NULL,
		.recv = NULL, .recv2 = NULL,
		.write = NULL,
		.close = NULL };


#ifdef USE_ARCHIVES
data_processor_t dp_archive = {
		.open = archive_open,
		.meta = archive_meta,
		.filter = all_filter,
		.read = archive_read, .read2 = archive_read2,
		.recv = archive_recv, .recv2 = archive_recv2,
		.write = archive_write,
		.close = archive_close };
#endif

#ifdef USE_NETWORK
data_processor_t dp_network = {
		.open = net_open,
		.meta = net_meta,
		.filter = net_filter,
		.read = NULL, .read2 = NULL,
		.recv = net_recv, .recv2 = NULL,
		.write = NULL,
		.close = net_close };
#endif

const data_processor_t* const to_dataprocessor(const iomode_t m)
{
	switch (m)
	{
	case IOMODE_LINES:   return &dp_lines;
	case IOMODE_FILES:   return &dp_files;
#ifdef USE_ARCHIVES
	case IOMODE_ARCHIVE: return &dp_archive;
#endif
#ifdef USE_NETWORK
	case IOMODE_NETWORK_DUMP:
	case IOMODE_NETWORK: return &dp_network;
#endif
	default:      return NULL;
	}
}
