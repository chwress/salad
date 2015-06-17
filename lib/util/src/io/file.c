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

#include "util/getline.h"

#include "recv.h"
#include "../regex.h"

// OPEN
typedef struct
{
	char* line;
	size_t pos;
	size_t size;
} lines_iterator_state_t;

#define LINE_ITERATOR_STATE_INITIALIZER { \
		.line = NULL, \
		.pos = 0, \
		.size = 0 \
}

typedef struct
{
	FILE* f;
	iterator_context_t context;
	char strip[256];

	lines_iterator_state_t state;
} line_iterator_t;

static inline line_iterator_t* const create_line_iterator(file_t* const f)
{
	assert(f != NULL);
	line_iterator_t* const it = (line_iterator_t*) calloc(1, sizeof(line_iterator_t));

	it->f = (FILE*) f->fd;
	it->state.line = NULL;
	it->state.size = it->state.pos = 0;

	init_iterator_context(&it->context, f);

	memset(&it->strip, 0, 256);
	it->strip['\r'] = TRUE;
	it->strip['\n'] = TRUE;

	return it;
};

static inline void reset_line_iterator_state(line_iterator_t* const it)
{
	assert(it != NULL);

	if (it->state.line != NULL)
	{
		free(it->state.line);
		it->state.line = NULL;
	}
	it->state.pos = 0;
	it->state.size = 0;
}

static inline void destroy_line_iterator(file_t* const f)
{
	assert(f != NULL);

	if (f->data != NULL)
	{
		line_iterator_t* it = (line_iterator_t*) f->data;
		reset_line_iterator_state(it);
		free(it);

		f->data = NULL;
	}
};

const int file_open(file_t* const f, const char* const filename, const char* mode, void* const p)
{
	assert(f != NULL);
	f->mode = to_fileiomode(mode);
	assert(mode != NULL && fileiomode_tostring(f->mode) != NULL);

	f->fd = fopen(filename, (f->mode == FILE_IOMODE_READ ? "rb" : "wb"));
	if (f->fd == NULL)
	{
		return EXIT_FAILURE;
	}
	f->data = create_line_iterator(f);

	// XXX: This doesn't work on Microsoft Windows OSs
	f->is_device = (strncmp(filename, "/dev/", 5) == 0);

	if (p != REOPEN)
	{
		memset(&f->meta, 0x00, sizeof(metadata_t));
		f->meta.filename = filename;
		all_filter_ex(f, "");
	}
	return EXIT_SUCCESS;
}


// META
const int file_meta(file_t* const f, const int group_input)
{
	assert(f != NULL);
	assert(group_input == FALSE); // NOT SUPPORTED

	f->meta.num_items = 0;
	f->meta.total_size = 0; // TODO: some up lengths -- strlen() -3*num_% -num_\n
	size_t num_perc = 0;

	// TODO: Apply the input-filter in order to correctly determine the
	//       correct number of input strings.

	int last = 0x00;
	for (int ch = fgetc(f->fd); ch != EOF; ch = fgetc(f->fd))
	{
		switch (ch)
		{
		case '\n': f->meta.num_items++;  break;
		case '%':  num_perc++;           break;
		default:   f->meta.total_size++; break;
		}
		last = ch;
	}

	if (last != '\n')
	{
		f->meta.num_items++;
	}

	int is_percentencoded = FALSE;
	if (is_percentencoded)
	{
		// This expects the input to be well-formatted of course an therefore
		// might only be an approximation.
		f->meta.total_size -= 2 *num_perc;
	}
	else
	{
		f->meta.total_size += num_perc;
	}

#ifdef EXTENDED_METADATA
	f->meta.filenames = NULL;
#endif
#ifdef GROUPED_INPUT
	f->meta.groups = NULL;
	f->meta.num_groups = 0;
#endif

	// reopen file
	file_close_ex(f, TRUE);
	return file_open(f, f->meta.filename, fileiomode_tostring(f->mode), REOPEN);
}

// READ
enum { LINE_ITERATOR_EOF, LINE_ITERATOR_EOL, LINE_ITERATOR_OK };

static inline const int file_read_next(file_t* const f, data_t* const out, const size_t chunk_size)
{
	assert(f != NULL);
	assert(out != NULL);

	line_iterator_t* const it = (line_iterator_t*) f->data;

	if (it->state.pos >= it->state.size)
	{
		char** line = &it->state.line;

#ifdef USE_REGEX_FILTER
		do
		{
#endif
			// In order not to break with the standard getline/ getdelim functions
			// we read one complete line and check for the size afterwards
			ssize_t read = getline(line, &it->state.size, it->f);
			if (read <= -1)
			{
				reset_line_iterator_state(it);
				return LINE_ITERATOR_EOF;
			}

			int j;
			for (j = read -1; j >= 0; j--)
			{
				if (!it->strip[(int) (*line)[j]]) break;
			}
			(*line)[j +1] = 0x00;

#ifdef USE_REGEX_FILTER
		} while (regexec(&f->filter, *line, 1, it->context.m, 0) != 0); /* match found */
#endif
		it->state.pos = 0;
		it->state.size = inline_decode(*line, strlen(*line));
	}

	out->len = MIN(chunk_size, it->state.size -it->state.pos);
	STRNDUP(out->len, it->state.line +it->state.pos, out->buf);
	it->state.pos += out->len;

#ifdef EXTENDED_METADATA
	out->meta.filename = f->meta.filename;
#endif

	if (it->state.pos >= it->state.size)
	{
		reset_line_iterator_state(it);
		return LINE_ITERATOR_EOL;
	}
	return LINE_ITERATOR_OK;
}


READ_STUB(file, LINE_ITERATOR_EOF, LINE_ITERATOR_EOL, LINE_ITERATOR_OK)
READ2_STUB(file, LINE_ITERATOR_EOF, LINE_ITERATOR_EOL, LINE_ITERATOR_OK)

// RECV
const size_t file_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr)
{
	return recv_stub(f, file_read, callback, batch_size, usr);
}

const size_t file_recv2(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr)
{
	return recv2_stub(f, file_read2, callback, batch_size, usr);
}


// WRITE
#define FILE_WRITESLICE(f, buf, _start_, end) {               \
	const size_t start = _start_;                             \
	const size_t slen = (end) -start;                         \
	const size_t x = encode(&line, &len, buf +start, slen);   \
	                                                          \
	const size_t nwrote = fwrite(line, sizeof(char), x, f);   \
	N += nwrote;                                              \
	if (nwrote != x)                                          \
	{	                                                      \
		break;                                                \
	}	                                                      \
}

const size_t file_write(file_t* const f, const dataset_t* const ds, void* const usr)
{
	assert(f != NULL);
	assert(ds != NULL);

	if (ds->n <= 0 || ds->capacity <= 0 || ds->data == NULL)
	{
		return 0;
	}

	char* line = NULL;
	size_t N = 0, len = 0;

	for (size_t i = 0; i < ds->n; i++)
	{
		const data_t* const d = &ds->data[i];
		if (d->slices.n > 0)
		{
			for (size_t j = 0; j < d->slices.n; j++)
			{
				const slice_t* const s = &d->slices.x[j];
				FILE_WRITESLICE(f->fd, d->buf, s->start, s->end);
			}
		}
		else
		{
			FILE_WRITESLICE(f->fd, d->buf, 0, d->len);
		}

		const size_t nwrote = fwrite("\n", sizeof(char), 1, f->fd);
		N += nwrote;
		if (nwrote != 1) break;
	}

	free(line);
	return N;
}


// CLOSE
const int file_close_itr(file_t* const f)
{
	destroy_line_iterator(f);
	return EXIT_SUCCESS;
}

const int file_close_ex(file_t* const f, int keep_metadata)
{
	assert(f != NULL);

	if (fclose(f->fd) != 0)
	{
		return EXIT_FAILURE;
	}
	f->fd = NULL;

	file_close_itr(f);

	if (!keep_metadata)
	{
		metadata_free(&f->meta);
		all_filter_close(f);
	}
	return EXIT_SUCCESS;
}

const int file_close(file_t* const f)
{
	return file_close_ex(f, FALSE);
}

