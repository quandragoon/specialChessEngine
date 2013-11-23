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

#ifndef MOVE_GEN_REFERENCE_H
#define MOVE_GEN_REFERENCE_H

#include <inttypes.h>
#include <stdbool.h>
#include "./move_gen.h"

// -----------------------------------------------------------------------------
// Function prototypes
// -----------------------------------------------------------------------------

ptype_t ptype_mv_of_reference(move_t mv);
square_t from_square_reference(move_t mv);
square_t to_square_reference(move_t mv);
rot_t rot_of_reference(move_t mv);
move_t move_of_reference(ptype_t typ, rot_t rot, square_t from_sq, square_t to_sq);
int generate_all_reference(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict);
void do_perft_reference(position_t *gme, int depth, int ply);
void low_level_make_move_reference(position_t *old, position_t *p, move_t mv);
piece_t make_move_reference(position_t *old, position_t *p, move_t mv);

#endif  // MOVE_GEN_REFERENCE_H
