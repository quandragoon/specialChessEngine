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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "fen.h"
#include "move_gen.h"
#include "move_gen_reference.h"
#include "search.h"
#include "util.h"

#define MAX(x, y)  ((x) > (y) ? (x) : (y))
#define MIN(x, y)  ((x) < (y) ? (x) : (y))

int USE_KO;  // Respect the Ko rule

static uint64_t   zob[ARR_SIZE][1<<PIECE_SIZE];
static uint64_t   zob_color;

ptype_t ptype_mv_of_reference(move_t mv) {
  return (ptype_t) ((mv >> PTYPE_MV_SHIFT) & PTYPE_MV_MASK);
}

square_t from_square_reference(move_t mv) {
  return (mv >> FROM_SHIFT) & FROM_MASK;
}

square_t to_square_reference(move_t mv) {
  return (mv >> TO_SHIFT) & TO_MASK;
}

rot_t rot_of_reference(move_t mv) {
  return (rot_t) ((mv >> ROT_SHIFT) & ROT_MASK);
}

move_t move_of_reference(ptype_t typ, rot_t rot, square_t from_sq, square_t to_sq) {
  return ((typ & PTYPE_MV_MASK) << PTYPE_MV_SHIFT) |
         ((rot & ROT_MASK) << ROT_SHIFT) |
         ((from_sq & FROM_MASK) << FROM_SHIFT) |
         ((to_sq & TO_MASK) << TO_SHIFT);
}

// Generate all moves from position p.  Returns number of moves.
// strict currently ignored
int generate_all_reference(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict) {
  color_t ctm = color_to_move_of(p);
  int move_count = 0;

  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      square_t  sq = square_of(f, r);
      piece_t x = p->board[sq];

      ptype_t typ = ptype_of(x);
      color_t color = color_of(x);

      switch (typ) {
       case EMPTY:
        break;
       case PAWN:
       case KING:
        if (color != ctm) {  // Wrong color
          break;
        }
        // directions
        for (int d = 0; d < 8; d++) {
          int dest = sq + dir_of(d);
          if (ptype_of(p->board[dest]) == INVALID) {
            continue;    // illegal square
          }

          WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
          WHEN_DEBUG_VERBOSE({
            move_to_str(move_of(typ, (rot_t) 0, sq, dest), buf);
            DEBUG_LOG(1, "Before: %s ", buf);
          })
          assert(move_count < MAX_NUM_MOVES);
          sortable_move_list[move_count++] = move_of(typ, (rot_t) 0, sq, dest);

          WHEN_DEBUG_VERBOSE({
            move_to_str(get_move(sortable_move_list[move_count-1]), buf);
            DEBUG_LOG(1, "After: %s\n", buf);
          })
        }

        // rotations - three directions possible
        for (int rot = 1; rot < 4; ++rot) {
          assert(move_count < MAX_NUM_MOVES);
          sortable_move_list[move_count++] = move_of(typ, (rot_t) rot, sq, sq);
        }
        if (typ == KING) {  // Also generate null move
          assert(move_count < MAX_NUM_MOVES);
          sortable_move_list[move_count++] = move_of(typ, (rot_t) 0, sq, sq);
        }
        break;
       case INVALID:
       default:
        assert(false);  // Couldn't BE more bogus!
      }
    }
  }

  WHEN_DEBUG_VERBOSE({
    DEBUG_LOG(1, "\nGenerated moves: ");
    for (int i = 0; i < move_count; ++i) {
      char buf[MAX_CHARS_IN_MOVE];
      move_to_str(get_move(sortable_move_list[i]), buf);
      DEBUG_LOG(1, "%s ", buf);
    }
    DEBUG_LOG(1, "\n");
  })

  return move_count;
}

void low_level_make_move_reference(position_t *old, position_t *p, move_t mv) {
  assert(mv != 0);

  WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
  WHEN_DEBUG_VERBOSE({
    move_to_str(mv, buf);
    DEBUG_LOG(1, "low_level_make_move: %s\n", buf);
  })

  assert(old->key == compute_zob_key(old));

  WHEN_DEBUG_VERBOSE({
    fprintf(stderr, "Before:\n");
    display(old);
  })

  square_t from_sq = from_square_reference(mv);
  square_t to_sq = to_square_reference(mv);
  rot_t rot = rot_of_reference(mv);

  WHEN_DEBUG_VERBOSE({
    DEBUG_LOG(1, "low_level_make_move 2:\n");
    square_to_str(from_sq, buf);
    DEBUG_LOG(1, "from_sq: %s\n", buf);
    square_to_str(to_sq, buf);
    DEBUG_LOG(1, "to_sq: %s\n", buf);
    switch(rot) {
      case NONE:
        DEBUG_LOG(1, "rot: none\n");
        break;
      case RIGHT:
        DEBUG_LOG(1, "rot: R\n");
        break;
      case UTURN:
        DEBUG_LOG(1, "rot: U\n");
        break;
      case LEFT:
        DEBUG_LOG(1, "rot: L\n");
        break;
      default:
        assert(false);  // Bad, bad, bad
        break;
    }
  })

  *p = *old;

  p->history = old;
  p->last_move = mv;

  assert(from_sq < ARR_SIZE && from_sq > 0);
  assert(p->board[from_sq] < (1 << PIECE_SIZE) &&
         p->board[from_sq] >= 0);
  assert(to_sq < ARR_SIZE && to_sq > 0);
  assert(p->board[to_sq] < (1 << PIECE_SIZE) &&
         p->board[to_sq] >= 0);

  p->key ^= zob_color;   // swap color to move

  piece_t from_piece = p->board[from_sq];
  piece_t to_piece = p->board[to_sq];

  if (to_sq != from_sq) {  // move, not rotation
    p->board[to_sq] = from_piece;  // swap from_piece and to_piece on board
    p->board[from_sq] = to_piece;

    // Hash key updates
    p->key ^= zob[from_sq][from_piece];  // remove from_piece from from_sq
    p->key ^= zob[to_sq][to_piece];  // remove to_piece from to_sq
    p->key ^= zob[to_sq][from_piece];  // place from_piece in to_sq
    p->key ^= zob[from_sq][to_piece];  // place to_piece in from_sq

    // Update King locations if necessary
    if (ptype_of(from_piece) == KING) {
      p->kloc[color_of(from_piece)] = to_sq;
    }
    if (ptype_of(to_piece) == KING) {
      p->kloc[color_of(to_piece)] = from_sq;
    }
  } else {  // rotation

    // remove from_piece from from_sq in hash
    p->key ^= zob[from_sq][from_piece];
    set_ori(&from_piece, rot + ori_of(from_piece));  // rotate from_piece
    p->board[from_sq] = from_piece;  // place rotated piece on board
    p->key ^= zob[from_sq][from_piece];              // ... and in hash
  }

  // Increment ply
  p->ply++;

  assert(p->key == compute_zob_key(p));

  WHEN_DEBUG_VERBOSE({
    fprintf(stderr, "After:\n");
    display(p);
  })
}


// returns square of piece to be removed from board or 0
square_t fire_reference(position_t *p) {
  color_t fctm = (color_to_move_of(p) == WHITE) ? BLACK : WHITE;
  square_t sq = p->kloc[fctm];
  int bdir = ori_of(p->board[sq]);

  assert(ptype_of(p->board[ p->kloc[fctm] ]) == KING);

  while (true) {
    sq += beam_of(bdir);
    assert(sq < ARR_SIZE && sq >= 0);

    switch (ptype_of(p->board[sq])) {
     case EMPTY:  // empty square
      break;
     case PAWN:  // Pawn
      bdir = reflect_of(bdir, ori_of(p->board[sq]));
      if (bdir < 0) {  // Hit back of Pawn
        return sq;
      }
      break;
     case KING:  // King
      return sq;  // sorry, game over my friend!
      break;
     case INVALID:  // Ran off edge of board
      return 0;
      break;
     default:  // Shouldna happen, man!
      assert(false);
      break;
    }
  }
}


// return 0 or victim piece or KO (== -1)
piece_t make_move_reference(position_t *old, position_t *p, move_t mv) {
  assert(mv != 0);

  // move phase 1
  low_level_make_move_reference(old, p, mv);

  // move phase 2
  square_t victim_sq = fire_reference(p);

  WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
  WHEN_DEBUG_VERBOSE({
    if (victim_sq != 0) {
      square_to_str(victim_sq, buf);
      DEBUG_LOG(1, "Zapping piece on %s\n", buf);
    }
  })

  if (victim_sq == 0) {
    p->victim = 0;

    if (USE_KO &&  // Ko rule
        (p->key == (old->key ^ zob_color) || p->key == old->history->key))
      return KO;

  } else {  // we definitely hit something with laser
    p->victim = p->board[victim_sq];
    p->key ^= zob[victim_sq][p->victim];   // remove from board
    p->board[victim_sq] = 0;
    p->key ^= zob[victim_sq][0];

    assert(p->key == compute_zob_key(p));

    WHEN_DEBUG_VERBOSE({
      square_to_str(victim_sq, buf);
      DEBUG_LOG(1, "Zapped piece on %s\n", buf);
    })
  }

  return p->victim;
}

// helper function for do_perft
// ply starting with 0
static uint64_t perft_search_reference(position_t *p, int depth, int ply) {
  uint64_t node_count = 0;
  position_t np;
  sortable_move_t lst[MAX_NUM_MOVES];
  int num_moves;
  int i;

  if (depth == 0) {
    return 1;
  }

  num_moves = generate_all_reference(p, lst, true);

  if (depth == 1) {
    return num_moves;
  }

  for (i = 0; i < num_moves; i++) {
    move_t mv = get_move(lst[i]);

    low_level_make_move_reference(p, &np, mv);  // make the move baby!
    square_t victim_sq = fire_reference(&np);  // the guy to disappear

    if (victim_sq != 0) {            // hit a piece
      ptype_t typ = ptype_of(np.board[victim_sq]);
      assert((typ != EMPTY) && (typ != INVALID));
      if (typ == KING) {  // do not expand further: hit a King
        node_count++;
        continue;
      }
      np.victim = np.board[victim_sq];
      np.key ^= zob[victim_sq][np.victim];   // remove from board
      np.board[victim_sq] = 0;
      np.key ^= zob[victim_sq][0];
    }

    uint64_t partialcount = perft_search_reference(&np, depth-1, ply+1);
    node_count += partialcount;
  }

  return node_count;
}

// help to verify the move generator
void do_perft_reference(position_t *gme, int depth, int ply) {
  fen_to_pos(gme, "");

  for (int d = 1; d <= depth; d++) {
    printf("perft %2d ", d);
    uint64_t j = perft_search_reference(gme, d, 0);
    printf("%" PRIu64 "\n", j);
  }
}

