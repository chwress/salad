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

#include "recv.h"

extern const size_t recv_stub_ex(file_t* const f, FN_READ read, FN_DATA data, const size_t initial_batch, const size_t batch_size, const size_t progress_max, void* const usr);
extern const size_t recv_stub(file_t* const f, FN_READ read, FN_DATA data, const size_t batch_size, void* const usr);
extern const size_t recv2_stub(file_t* const f, FN_READ read, FN_DATA data, const size_t batch_size, void* const usr);

