/**
 * Copyright (c) 2012,2013 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include "./abort.h"
#include <stdio.h>
#include <assert.h>

#define INF_ABORT 32700

void abort_constructor(Abort * self) {
	self->parent = NULL; 
  self->pollGranularity = 0;
  self->count = 0;
  self->index_among_siblings = -1;
  self->min_aborted_child_index = INF_ABORT;
}

void abort_constructor_parent(Abort * self, Abort *p, int ias) {
	self->parent = p;
	self->pollGranularity = 0;
	self->count = 0;
	self->index_among_siblings = ias;
	self->min_aborted_child_index = INF_ABORT;
}

void setGranularity(Abort * self, int newGranularity) {
	self->pollGranularity = newGranularity;
}

int stopSpawning(Abort * self) {
	assert(self->min_aborted_child_index <= INF_ABORT);
	assert(self->index_among_siblings <= INF_ABORT);

	return self->min_aborted_child_index != INF_ABORT;
}

int isAborted(Abort * self) {
	assert(self->min_aborted_child_index <= INF_ABORT);
	assert(self->index_among_siblings <= INF_ABORT);

	int aborted = (self->parent && self->parent->min_aborted_child_index != INF_ABORT);
	int ret = aborted || (self->parent && isAborted(self->parent));
	if (ret) {
		self->min_aborted_child_index = -1;
	}
	return ret;
}

void do_abort(Abort * self, int child_index) {
  if (child_index < self->min_aborted_child_index) {
    self->min_aborted_child_index = child_index;
  }
}
