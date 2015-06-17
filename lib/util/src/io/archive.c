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

#include "common.h"
#include "iterator.h"


#ifdef USE_ARCHIVES
#include <time.h>
#include <archive.h>
#include <archive_entry.h>

#include "recv.h"

// OPEN
typedef struct
{
	struct archive_entry* entry;
	size_t n;
	size_t size;

	int length_at_end;
} archive_iterator_state_t;

#define ARCHIVE_ITERATOR_STATE_INITIALIZER { \
		.entry = NULL, \
		.n = 0, \
		.size = 0, \
		.length_at_end = FALSE \
}

#define ARCHIVE_ITERATOR_STATE_T(name) archive_iterator_state_t name = ARCHIVE_ITERATOR_STATE_INITIALIZER
static const ARCHIVE_ITERATOR_STATE_T(EMPTY_ARCHIVE_ITERATOR_STATE);


typedef struct
{
	struct archive* a;
	iterator_context_t context;
	archive_iterator_state_t state;
} archive_iterator_t;

#define RESET_ARCHIVE_ITERATOR_ENTRY(it) \
{ \
	(it).state = EMPTY_ARCHIVE_ITERATOR_STATE; \
}

static inline archive_iterator_t* const create_archive_iterator(file_t* const f, struct archive* a)
{
	assert(f != NULL);
	assert(a != NULL);

	archive_iterator_t* const it = (archive_iterator_t*) calloc(1, sizeof(archive_iterator_t));
	it->a = a;

	RESET_ARCHIVE_ITERATOR_ENTRY(*it);
	init_iterator_context(&it->context, f);
	return it;
};

#define GET_ARCHIVE(f) ((archive_iterator_t*) f->data)->a


const int archive_open(file_t* const f, const char* const filename, const char* mode, void* const p)
{
	int ret = file_open(f, filename, mode, p);
	if (ret != EXIT_SUCCESS)
	{
		return ret;
	}

	struct archive* a = NULL;
	switch (f->mode)
	{
	case FILE_IOMODE_READ:
	{
		a = archive_read_new();
	#ifdef LIBARCHIVE2
		archive_read_support_compression_all(a);
	#else
		archive_read_support_filter_all(a);
	#endif
		archive_read_support_format_all(a);

		ret = archive_read_open_FILE(a, f->fd);
		break;
	}
	case FILE_IOMODE_WRITE:
	{
		a = archive_write_new();
		archive_write_add_filter_gzip(a);
		archive_write_set_format_pax_restricted(a);
		ret = archive_write_open_FILE(a, f->fd);
		break;
	}}


	if (ret != ARCHIVE_OK)
	{
		return EXIT_FAILURE;
	}

	file_close_itr(f);
	f->data = create_archive_iterator(f, a);
	return EXIT_SUCCESS;
}


// META
const int archive_meta(file_t* const f, const int group_input)
{
	assert(f != NULL);

	metadata_t* const meta = &f->meta;
	meta->num_items  = 0;
	meta->total_size = 0;

#ifdef EXTENDED_METADATA
	size_t fnames_capacity = 1;
	meta->filenames = (char**) calloc(fnames_capacity, sizeof(char*));
#endif

#ifdef GROUPED_INPUT
	size_t groups_capacity = 1;
	meta->groups = (group_t*) calloc(groups_capacity, sizeof(group_t));
	meta->num_groups = 0;

	group_t* cur = &meta->groups[0]; cur->name = "";
#endif
#ifdef USE_REGEX_FILTER
    regmatch_t m[1];
#endif

    // XXX: Use the archive_iterator directly
	struct archive* a = GET_ARCHIVE(f);
	struct archive_entry* entry = NULL;

	int r = ARCHIVE_FAILED;
	while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
	{
        if (archive_entry_filetype(entry) == AE_IFREG)
        {
        	// is optimized away ifndef EXTENDED_METADATA, GROUPED_INPUT, USE_REGEX_FILTER
        	const char* const name = archive_entry_pathname(entry); UNUSED(name);

#ifdef GROUPED_INPUT
        	const char* const slash = strrchr(name, '/');

    		if (slash == NULL || strncmp(name, cur->name, slash -name +1) != 0)
    		{
    			// new class
    			if (meta->num_groups >= groups_capacity)
				{
					groups_capacity *= 2;
					const size_t s = groups_capacity *sizeof(group_t);
					meta->groups = (group_t*) realloc(meta->groups, s);
				}
				cur = &meta->groups[meta->num_groups++];
				STRNDUP((slash == NULL ? strlen(name) : slash -name) +1, name, cur->name);
				cur->n = 1;
    		}
    		else
    		{
    			cur->n++;
    		}
#endif
#ifdef USE_REGEX_FILTER
    	    if (regexec(&f->filter, name, 1, m, 0) != 0) continue;
#endif

#ifdef EXTENDED_METADATA
			if (meta->num_items >= fnames_capacity)
			{
				fnames_capacity *= 2;
				const size_t s = fnames_capacity *sizeof(char*);
				meta->filenames = (char**) realloc(meta->filenames, s);
			}
        	STRDUP(name, meta->filenames[meta->num_items]);
#endif
    		meta->num_items++;
            meta->total_size += (size_t) archive_entry_size(entry);
        }
        archive_read_data_skip(a);
	}
#ifdef EXTENDED_METADATA
	meta->filenames = (char**) realloc(meta->filenames, meta->num_items *sizeof(char*));
#endif
#ifdef GROUPED_INPUT
	meta->groups = (group_t*) realloc(meta->groups, meta->num_groups *sizeof(group_t));
#endif

	// reopen archive
	archive_close_ex(f, TRUE);
	archive_open(f, f->meta.filename, fileiomode_tostring(f->mode), REOPEN);

	return (r == ARCHIVE_EOF ? EXIT_SUCCESS : EXIT_FAILURE);
}


// READ
#ifdef GROUPED_INPUT
group_t* const group_next(metadata_t* const meta, int* const gid, int* const fid)
{
	if (*gid >= meta->num_groups)
	{
		return NULL;
	}

	if (*fid >= meta->groups[*gid].n)
	{
		(*gid)++;
		(*fid) = 0;
		return group_next(meta, gid, fid);
	}

	(*fid)++;
	return &meta->groups[*gid];
}
#endif


enum { ARCHIVE_ITERATOR_EOA, ARCHIVE_ITERATOR_EOF, ARCHIVE_ITERATOR_OK };

static inline int archive_read_next(file_t* const f, data_t* const out, const size_t chunk_size)
{
	assert(f != NULL);
	assert(out != NULL);

	archive_iterator_t* const it = (archive_iterator_t*) f->data;

	if (it->state.entry == NULL)
	{
		while (1)
		{
			if (archive_read_next_header(it->a, &it->state.entry) != ARCHIVE_OK)
			{
				RESET_ARCHIVE_ITERATOR_ENTRY(*it);
				return ARCHIVE_ITERATOR_EOA;
			}

        	// is optimized away ifndef EXTENDED_METADATA, GROUPED_INPUT, USE_REGEX_FILTER
			const char* const name = archive_entry_pathname(it->state.entry); UNUSED(name);

			if (archive_entry_filetype(it->state.entry) != AE_IFREG) continue;
#ifdef USE_REGEX_FILTER
			if (regexec(&f->filter, name, 1, it->context.m, 0) != 0) continue;
#endif

#ifdef EXTENDED_METADATA
			out->meta.filename = name;
#endif
#ifdef GROUPED_INPUT
			out->meta.group = group_next(&f->meta, &it->context.gid, &it->context.fid);
#endif
			it->state.n = 0;
			it->state.length_at_end = (archive_entry_size_is_set(it->state.entry) == 0);

			if (!it->state.length_at_end)
			{
				it->state.size = (size_t) archive_entry_size(it->state.entry);
			}
			break;
		}
	}

	if (it->state.length_at_end)
	{
		static const unsigned int BLOCK_SIZE = 102400;
		unsigned long capacity = MIN(BLOCK_SIZE, chunk_size);

		out->buf = (char*) malloc(capacity * sizeof(char));
		out->len = 0;

		size_t read = 0, next_block = 0;
		do
		{
			size_t next_block = MIN(BLOCK_SIZE, chunk_size -out->len);

			if (out->len +next_block > capacity)
			{
				capacity = MIN(capacity *2, chunk_size);
				out->buf = (char*) realloc(out->buf, capacity * sizeof(char));
			}

			char* const x = out->buf +out->len;
			read = archive_read_data(it->a, x, next_block);
			out->len += read;

		} while (read > 0);

		if (next_block > 0)// EOF reached
		{
			it->state.n = it->state.size = out->len;
			out->buf = (char*) realloc(out->buf, out->len * sizeof(char));
		}
	}
	else
	{
		out->len = MIN(chunk_size, it->state.size -it->state.n);
		it->state.n += out->len;

		out->buf = (char*) malloc(out->len * sizeof(char));
		archive_read_data(it->a, out->buf, out->len);
	}

	if (it->state.n >= it->state.size)
	{
		archive_read_data_skip(it->a);
		RESET_ARCHIVE_ITERATOR_ENTRY(*it);
		return ARCHIVE_ITERATOR_EOF;
	}
	return ARCHIVE_ITERATOR_OK;
}

READ_STUB(archive, ARCHIVE_ITERATOR_EOA, ARCHIVE_ITERATOR_EOF, ARCHIVE_ITERATOR_OK)
READ2_STUB(archive, ARCHIVE_ITERATOR_EOA, ARCHIVE_ITERATOR_EOF, ARCHIVE_ITERATOR_OK)


// RECV
const size_t archive_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr)
{
	return recv_stub(f, archive_read, callback, batch_size, usr);
}

const size_t archive_recv2(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr)
{
	return recv2_stub(f, archive_read2, callback, batch_size, usr);
}


// WRITE
#define ARCHIVE_WRITESLICE(a, buf, _start_, end) {            \
	const size_t start = _start_;                             \
	const size_t len = ((end) -start);                        \
	const size_t n = archive_write_data(a, buf +start, len);  \
	if (n != len)                                             \
	{	                                                      \
		return i;                                             \
	}	                                                      \
}

const size_t archive_write(file_t* const f, const dataset_t* const ds, void* const usr)
{
	assert(f != NULL);
	assert(ds != NULL);

	if (ds->n <= 0 || ds->capacity <= 0 || ds->data == NULL)
	{
		return 0;
	}

	struct archive* a = GET_ARCHIVE(f);
	struct archive_entry* entry = archive_entry_new();

	for (size_t i = 0; i < ds->n; i++)
	{
		const data_t* const d = &ds->data[i];
#ifdef EXTENDED_METADATA
		const char* const fname = d->meta.filename;
#else
		const char* const fname = "";
#endif
		size_t len = d->len;

		const int has_slices = (d->slices.n > 0);

		if (has_slices)
		{
			len = 0;
			for (size_t j = 0; j < d->slices.n; j++)
			{
				slice_t* const s = &d->slices.x[j];
				len += (s->end -s->start);
			}
		}

		archive_entry_clear(entry);
		archive_entry_set_pathname(entry, fname);
		archive_entry_set_size(entry, len);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0644);
		archive_write_header(a, entry);

		if (has_slices)
		{
			for (size_t j = 0; j < d->slices.n; j++)
			{
				slice_t* const s = &d->slices.x[j];
				ARCHIVE_WRITESLICE(a, d->buf, s->start, s->end);
			}
		}
		else
		{
			ARCHIVE_WRITESLICE(a, d->buf, 0, d->len);
		}
		archive_entry_free(entry);
	}
	return ds->n;
}


// CLOSE
typedef int (*FN_ARCHIVE)(struct archive *);

const int archive_close_ex(file_t* const f, int keep_metadata)
{
	assert(f != NULL);
	FN_ARCHIVE close_archive = NULL;
	FN_ARCHIVE free_archive = NULL;

	if (f->mode == FILE_IOMODE_READ)
	{
		close_archive = archive_read_close;
#ifdef LIBARCHIVE2
		free_archive = archive_read_finish;
#else
		free_archive = archive_read_free;
#endif
	}
	else if (f->mode == FILE_IOMODE_WRITE)
	{
		close_archive = archive_write_close;
#ifdef LIBARCHIVE2
		free_archive = archive_write_finish;
#else
		free_archive = archive_write_free;
#endif
	}

	assert(close_archive != NULL);
	assert(free_archive != NULL);

	archive_iterator_t* const it = (archive_iterator_t*) f->data;
	if (close_archive(it->a) != ARCHIVE_OK ||
		free_archive(it->a) != ARCHIVE_OK)
	{
		return EXIT_FAILURE;
	}

	free(it);
	f->data = NULL;

	if (!keep_metadata)
	{
		metadata_free(&f->meta);
		all_filter_close(f);
	}
	return file_close_ex(f, TRUE);
}

const int archive_close(file_t* const f)
{
	return archive_close_ex(f, FALSE);
}
#endif
