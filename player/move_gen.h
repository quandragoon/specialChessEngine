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

#ifndef MOVE_GEN_H
#define MOVE_GEN_H

#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include "util.h" 

#define MAX_NUM_MOVES 100      // real number = 7 x (8 + 3) + 1 x (8 + 4) = 89
#define MAX_PLY_IN_SEARCH 100  // up to 100 ply
#define MAX_PLY_IN_GAME 4096   // long game!  ;^)

// Used for debugging and display
#define MAX_CHARS_IN_MOVE 16  // Could be less
#define MAX_CHARS_IN_TOKEN 64

// the board (which is 8x8 or 10x10) is centered in a 16x16 array
#define ARR_WIDTH 10
#define ARR_SIZE (ARR_WIDTH * ARR_WIDTH)

// board is 8 x 8 or 10 x 10
#define BOARD_WIDTH 8

typedef int square_t;
typedef int rnk_t;
typedef int fil_t;

#define FIL_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)
#define RNK_ORIGIN ((ARR_WIDTH - BOARD_WIDTH) / 2)

#define FIL_SHIFT 4
#define FIL_MASK 15
#define RNK_SHIFT 0
#define RNK_MASK 15

// -----------------------------------------------------------------------------
// pieces
// -----------------------------------------------------------------------------

#define PIECE_SIZE 5  // Number of bits in (ptype, color, orientation)
#define PIECE_MASK 31

typedef uint8_t piece_t;

// -----------------------------------------------------------------------------
// piece types
// -----------------------------------------------------------------------------

#define PTYPE_SHIFT 2
#define PTYPE_MASK 3

typedef enum {
    EMPTY,
    PAWN,
    KING,
    INVALID
} ptype_t;

// -----------------------------------------------------------------------------
// colors
// -----------------------------------------------------------------------------

#define COLOR_SHIFT 4
#define COLOR_MASK 1

typedef enum {
    WHITE = 0,
    BLACK
} color_t;

// -----------------------------------------------------------------------------
// Orientations
// -----------------------------------------------------------------------------

#define NUM_ORI 4
#define ORI_SHIFT 0
#define ORI_MASK (NUM_ORI - 1)

typedef enum {
  NN,
  EE,
  SS,
  WW
} king_ori_t;

typedef enum {
  NW,
  NE,
  SE,
  SW
} pawn_ori_t;

// -----------------------------------------------------------------------------
// Index information
// -----------------------------------------------------------------------------

#define INDEX_SHIFT 5
#define INDEX_MASK 7

// -----------------------------------------------------------------------------
// moves
// -----------------------------------------------------------------------------

// invalid piece_t value returned by make move in illegal ko situation
#define KO (INDEX_MASK << INDEX_SHIFT)  

// MOVE_MASK is 20 bits
#define MOVE_MASK 0xfffff

#define PTYPE_MV_SHIFT 18
#define PTYPE_MV_MASK 3
#define FROM_SHIFT 8
#define FROM_MASK 0xFF
#define TO_SHIFT 0
#define TO_MASK 0xFF
#define ROT_SHIFT 16
#define ROT_MASK 3

typedef uint32_t move_t;
typedef uint64_t sortable_move_t;

// Rotations
typedef enum {
  NONE,
  RIGHT,
  UTURN,
  LEFT
} rot_t;

// -----------------------------------------------------------------------------
// position
// -----------------------------------------------------------------------------

typedef struct position {
  piece_t      board[ARR_SIZE];
  struct position  *history;     // history of position
  uint64_t     key;              // hash key
  int          ply;              // Even ply are White, odd are Black
  move_t       last_move;        // move that led to this position
  piece_t      victim;           // piece destroyed by shooter
  square_t     kloc[2];          // location of kings
  square_t     pawns[12];        // location of pawns
  uint8_t      rnk_count[8];
  uint8_t      fil_count[8];
} position_t;

// -----------------------------------------------------------------------------
// Function prototypes
// -----------------------------------------------------------------------------

char * color_to_str(color_t c);
color_t color_to_move_of(position_t *p);
void set_color(piece_t *x, color_t c);
void set_ptype(piece_t *x, ptype_t pt);
void set_ind(piece_t *x, int index);
int ori_of(piece_t x);
void set_ori(piece_t *x, int ori);
void init_zob();
int square_to_str(square_t sq, char *buf);
void move_to_str(move_t mv, char *buf);
int generate_all(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict);
void do_perft(position_t *gme, int depth, int ply);
void low_level_make_move(position_t *old, position_t *p, move_t mv);
piece_t make_move(position_t *old, position_t *p, move_t mv);
void display(position_t *p);
uint64_t compute_zob_key(position_t *p);

// Inline functions

static inline square_t square_of(fil_t f, rnk_t r) {
  square_t s = ARR_WIDTH * (FIL_ORIGIN + f) + RNK_ORIGIN + r;
  DEBUG_LOG(1, "Square of (file %d, rank %d) is %d\n", f, r, s);
  assert((s >= 0) && (s < ARR_SIZE));
  return s;
}

static fil_t fil_arr[] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

static rnk_t rnk_arr[] = {
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8
};

static inline fil_t fil_of(square_t sq) {
  fil_t f = fil_arr[sq];
  DEBUG_LOG(1, "File of square %d is %d\n", sq, f);
  return f;
}

static inline rnk_t rnk_of(square_t sq) {
  rnk_t r = rnk_arr[sq];
  DEBUG_LOG(1, "Rank of square %d is %d\n", sq, r);
  return r;
}

static inline color_t color_of(piece_t x) {
  return (color_t) ((x >> COLOR_SHIFT) & COLOR_MASK);
}

static inline ptype_t ptype_of(piece_t x) {
  return (ptype_t) ((x >> PTYPE_SHIFT) & PTYPE_MASK);
}

static inline int index_of(piece_t x) {
  int index =  (int) ((x >> INDEX_SHIFT));
  return index;
}

static inline color_t opp_color(color_t c){
    return !c;
}

static int beam[NUM_ORI] = {1, ARR_WIDTH, -1, -ARR_WIDTH};

static inline int beam_of(int direction) {
  assert(direction >= 0 && direction < NUM_ORI);
  return beam[direction];
}

static int dir[8] = { -ARR_WIDTH - 1, -ARR_WIDTH, -ARR_WIDTH + 1, -1, 1,
                              ARR_WIDTH - 1, ARR_WIDTH, ARR_WIDTH + 1 };

static inline int dir_of(int i) {
  assert(i >= 0 && i < 8);
  return dir[i];
}

// reflect[beam_dir][pawn_orientation]
// -1 indicates back of Pawn
static int reflect[NUM_ORI][NUM_ORI] = {
//  NW  NE  SE  SW
  { -1, -1, EE, WW},   // NN
  { NN, -1, -1, SS},   // EE
  { WW, EE, -1, -1 },  // SS
  { -1, NN, SS, -1 }   // WW
};

static inline int reflect_of(int beam_dir, int pawn_ori) {
  assert(beam_dir >= 0 && beam_dir < NUM_ORI);
  assert(pawn_ori >= 0 && pawn_ori < NUM_ORI);
  return reflect[beam_dir][pawn_ori];
}
static inline ptype_t ptype_mv_of(move_t mv) {
  return (ptype_t) ((mv >> PTYPE_MV_SHIFT) & PTYPE_MV_MASK);
}

static inline square_t from_square(move_t mv) {
  return (mv >> FROM_SHIFT) & FROM_MASK;
}

static inline square_t to_square(move_t mv) {
  return (mv >> TO_SHIFT) & TO_MASK;
}

static inline rot_t rot_of(move_t mv) {
  return (rot_t) ((mv >> ROT_SHIFT) & ROT_MASK);
}

static inline move_t move_of(ptype_t typ, rot_t rot, square_t from_sq, square_t to_sq) {
  return ((typ & PTYPE_MV_MASK) << PTYPE_MV_SHIFT) |
         ((rot & ROT_MASK) << ROT_SHIFT) |
         ((from_sq & FROM_MASK) << FROM_SHIFT) |
         ((to_sq & TO_MASK) << TO_SHIFT);
}

#endif  // MOVE_GEN_H
