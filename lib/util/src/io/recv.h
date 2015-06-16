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

#ifndef INTERNAL_UTIL_IO_RECV_H_
#define INTERNAL_UTIL_IO_RECV_H_

#include <stdlib.h>

#include "util/io.h"
#include "util/log.h"

inline const size_t recv_stub_ex(file_t* const f, FN_READ read, FN_DATA data, const size_t initial_batch, const size_t batch_size, const size_t progress_max, void* const usr)
{
	assert(f != NULL);
	assert(read != NULL);
	assert(data != NULL);

	dataset_t buf;
	buf.capacity = initial_batch;
	buf.data = (data_t*) calloc(buf.capacity, sizeof(data_t));
	buf.n = 0;

	size_t n = 0, N = 0;
	while ((n = read(f, &buf, batch_size)) > 0)
	{
#ifndef NDEBUG
		const int ret = data(buf.data, buf.n, usr);
		assert(ret == EXIT_SUCCESS);
#else
		data(buf.data, buf.n, usr);
#endif
		// Do not use dataset_free(.) since we want to reuse the
		// memory consumed by dataset_t#data
		for (int i = 0; i < buf.n; i++)
		{
			data_free(&buf.data[i]);
		}
		buf.n = 0;
		N += n;
		progress(N, progress_max);
	};

	free(buf.data); // cf. comment above
	return N;
}

inline const size_t recv_stub(file_t* const f, FN_READ read, FN_DATA data, const size_t batch_size, void* const usr)
{
	return recv_stub_ex(f, read, data, batch_size, batch_size, f->meta.num_items, usr);
}

inline const size_t recv2_stub(file_t* const f, FN_READ read, FN_DATA data, const size_t batch_size, void* const usr)
{
	const size_t INITIAL_BUFFER_SIZE = 128;
	return recv_stub_ex(f, read, data, INITIAL_BUFFER_SIZE, batch_size, f->meta.total_size, usr);
}


#endif /* INTERNAL_UTIL_IO_RECV_H_ */
