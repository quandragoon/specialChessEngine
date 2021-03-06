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
#include "search.h"
#include "util.h"

#define MAX(x, y)  ((x) > (y) ? (x) : (y))
#define MIN(x, y)  ((x) < (y) ? (x) : (y))

int USE_KO;  // Respect the Ko rule

static char *color_strs[2] = {"White", "Black"};

char * color_to_str(color_t c) {
  return color_strs[c];
}

// colors accessors
color_t color_to_move_of(position_t *p) {
  if ((p->ply & 1) == 0) {
    return WHITE;
  } else {
    return BLACK;
  }
}

/*
color_t color_of(piece_t x) {
  return (color_t) ((x >> COLOR_SHIFT) & COLOR_MASK);
}
*/

/*

color_t opp_color(color_t c) {
  if (c == WHITE) {
    return BLACK;
  } else {
    return WHITE;
  }
}
*/

void set_color(piece_t *x, color_t c) {
  assert((c >= 0) & (c <= COLOR_MASK));
  *x = ((c & COLOR_MASK) << COLOR_SHIFT) |
        (*x & ~(COLOR_MASK << COLOR_SHIFT));
}

void set_ptype(piece_t *x, ptype_t pt) {
  *x = ((pt & PTYPE_MASK) << PTYPE_SHIFT) |
        (*x & ~(PTYPE_MASK << PTYPE_SHIFT));
}

void set_ind(piece_t *x, int index) {
  *x = ((index & INDEX_MASK) << INDEX_SHIFT) | 
        (*x & ~(INDEX_MASK << INDEX_SHIFT));
}

int ori_of(piece_t x) {
  return (x >> ORI_SHIFT) & ORI_MASK;
}

void set_ori(piece_t *x, int ori) {
  *x = ((ori & ORI_MASK) << ORI_SHIFT) |
        (*x & ~(ORI_MASK << ORI_SHIFT));
}

// King orientations
char *king_ori_to_rep[2][NUM_ORI] = { { "NN", "EE", "SS", "WW" },
                                      { "nn", "ee", "ss", "ww" } };

// Pawn orientations
char *pawn_ori_to_rep[2][NUM_ORI] = { { "NW", "NE", "SE", "SW" },
                                      { "nw", "ne", "se", "sw" } };

char *nesw_to_str[NUM_ORI] = {"north", "east", "south", "west"};

// -----------------------------------------------------------------------------
// Board, squares
// -----------------------------------------------------------------------------

static uint64_t   zob[ARR_SIZE][1<<PIECE_SIZE];
static uint64_t   zob_color;

// Zobrist
uint64_t compute_zob_key(position_t *p) {
  uint64_t key = 0;
  square_t sq = ARR_WIDTH*(FIL_ORIGIN-1) + RNK_ORIGIN;
  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    sq += ARR_WIDTH;
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      // square_t sq = square_of(f, r);
      key ^= zob[sq + r][p->board[sq + r] & PIECE_MASK];
    }
  }
  if (color_to_move_of(p) == BLACK)
    key ^= zob_color;

  return key;
}

uint64_t myrand();

void init_zob() {
  for (int i = 0; i < ARR_SIZE; i++) {
    for (int j = 0; j < (1 << PIECE_SIZE); j++) {
      zob[i][j] = myrand();
    }
  }
  zob_color = myrand();
}

// converts a square to string notation, returns number of characters printed
int square_to_str(square_t sq, char *buf) {
  fil_t f = fil_of(sq);
  rnk_t r = rnk_of(sq);
  if (f >= 0) {
    return sprintf(buf, "%c%d", 'a'+ f, r);
  } else  {
    return sprintf(buf, "%c%d", 'z' + f + 1, r);
  }
}

// converts a move to string notation for FEN
void move_to_str(move_t mv, char *buf) {
  square_t f = from_square(mv);  // from-square
  square_t t = to_square(mv);    // to-square
  rot_t r = rot_of(mv);          // rotation

  buf += square_to_str(f, buf);
  if (f != t) {
    buf += square_to_str(t, buf);
  } else {
    switch (r) {
     case NONE:
      buf += square_to_str(t, buf);
      break;
     case RIGHT:
      buf += sprintf(buf, "R");
      break;
     case UTURN:
      buf += sprintf(buf, "U");
      break;
     case LEFT:
      buf += sprintf(buf, "L");
      break;
     default:
      assert(false);  // Bad, bad, bad
      break;
    }
  }
}

// Generate all moves from position p.  Returns number of moves.
// strict currently ignored
int generate_all(position_t *p, sortable_move_t *sortable_move_list,
                 bool strict) {
  color_t ctm = color_to_move_of(p);
  int move_count = 0;

  int ind = 6*ctm;
  for (int i = 0; i < 6; ++i) {
    square_t sq = p->pawns[ind + i];
    if (sq < 0)
      continue;

    assert(ptype_of(p->board[sq]) == PAWN);
    assert(color_of(p->board[sq]) == ctm);

    // directions
    for (int d = 0; d < 8; d++) {
      int dest = sq + dir_of(d);
      if (ptype_of(p->board[dest]) == INVALID) {
        continue;    // illegal square
      }

      WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
      WHEN_DEBUG_VERBOSE({
        move_to_str(move_of(PAWN, (rot_t) 0, sq, dest), buf);
        DEBUG_LOG(1, "Before: %s ", buf);
      })
      assert(move_count < MAX_NUM_MOVES);
      sortable_move_list[move_count++] = move_of(PAWN, (rot_t) 0, sq, dest);

      WHEN_DEBUG_VERBOSE({
        move_to_str(get_move(sortable_move_list[move_count-1]), buf);
        DEBUG_LOG(1, "After: %s\n", buf);
      })
    }

    // rotations - three directions possible
    for (int rot = 1; rot < 4; ++rot) {
      assert(move_count < MAX_NUM_MOVES);
      sortable_move_list[move_count++] = move_of(PAWN, (rot_t) rot, sq, sq);
    }
  }

  square_t sq = p->kloc[ctm];

  assert(ptype_of(p->board[sq]) == KING);
  assert(color_of(p->board[sq]) == ctm);
  // directions
        
  for (int d = 0; d < 8; d++) {
    int dest = sq + dir_of(d);
    if (ptype_of(p->board[dest]) == INVALID) {
      continue;    // illegal square
    }

    WHEN_DEBUG_VERBOSE( char buf[MAX_CHARS_IN_MOVE]; )
    WHEN_DEBUG_VERBOSE({
      move_to_str(move_of(KING, (rot_t) 0, sq, dest), buf);
      DEBUG_LOG(1, "Before: %s ", buf);
    })
    assert(move_count < MAX_NUM_MOVES);
    sortable_move_list[move_count++] = move_of(KING, (rot_t) 0, sq, dest);

    WHEN_DEBUG_VERBOSE({
      move_to_str(get_move(sortable_move_list[move_count-1]), buf);
      DEBUG_LOG(1, "After: %s\n", buf);
    })
  }

  // rotations - three directions possible
  for (int rot = 1; rot < 4; ++rot) {
    assert(move_count < MAX_NUM_MOVES);
    sortable_move_list[move_count++] = move_of(KING, (rot_t) rot, sq, sq);
  }
  assert(move_count < MAX_NUM_MOVES);
  sortable_move_list[move_count++] = move_of(KING, (rot_t) 0, sq, sq);

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

void low_level_make_move(position_t *old, position_t *p, move_t mv) {
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

  square_t from_sq = from_square(mv);
  square_t to_sq = to_square(mv);
  rot_t rot = rot_of(mv);

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
  assert(p->board[from_sq] < (1 << (PIECE_SIZE + 3)) &&
         p->board[from_sq] >= 0);
  assert(to_sq < ARR_SIZE && to_sq > 0);
  assert(p->board[to_sq] < (1 << (PIECE_SIZE + 3)) &&
         p->board[to_sq] >= 0);

  p->key ^= zob_color;   // swap color to move

  piece_t from_piece = p->board[from_sq];
  piece_t to_piece = p->board[to_sq];

  if (to_sq != from_sq) {  // move, not rotation
    p->board[to_sq] = from_piece;  // swap from_piece and to_piece on board
    p->board[from_sq] = to_piece;

    // Hash key updates
    p->key ^= zob[from_sq][from_piece & PIECE_MASK];  // remove from_piece from from_sq
    p->key ^= zob[to_sq][to_piece & PIECE_MASK];  // remove to_piece from to_sq
    p->key ^= zob[to_sq][from_piece & PIECE_MASK];  // place from_piece in to_sq
    p->key ^= zob[from_sq][to_piece & PIECE_MASK];  // place to_piece in from_sq

    // Update locations in pawn/king lookup arrays if necessary
    if (ptype_of(from_piece) == KING) {
      p->kloc[color_of(from_piece)] = to_sq;
    } else if (ptype_of(from_piece) == PAWN) {
      int start_index = 6 * color_of(from_piece);
      assert(from_sq == p->pawns[start_index + index_of(from_piece)]);
      p->pawns[start_index + index_of(from_piece)] = to_sq;
    }

    if (ptype_of(to_piece) == KING) {
      p->kloc[color_of(to_piece)] = from_sq;
    } else if (ptype_of(to_piece) == PAWN) {
      int start_index = 6 * color_of(to_piece);
      assert(to_sq == p->pawns[start_index + index_of(to_piece)]);
      p->pawns[start_index + index_of(to_piece)] = from_sq;
    } else {
      // This suggests that the square is empty.
      p->fil_count[fil_of(from_sq)]--;
      p->rnk_count[rnk_of(from_sq)]--;
      p->fil_count[fil_of(to_sq)]++;
      p->rnk_count[rnk_of(to_sq)]++;
    }

  } else {  // rotation
    // remove from_piece from from_sq in hash
    p->key ^= zob[from_sq][from_piece & PIECE_MASK];
    set_ori(&from_piece, rot + ori_of(from_piece));  // rotate from_piece
    p->board[from_sq] = from_piece;  // place rotated piece on board
    p->key ^= zob[from_sq][from_piece & PIECE_MASK];              // ... and in hash
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
square_t fire(position_t *p) {
  color_t fctm = (color_to_move_of(p) == WHITE) ? BLACK : WHITE;
  square_t sq = p->kloc[fctm];
  int bdir = ori_of(p->board[sq]);

  if (bdir%2==1) {
    if (p->rnk_count[rnk_of(sq)] < 2) return 0;
  } else {
    if (p->fil_count[fil_of(sq)] < 2) return 0;
  }
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

      // If there is only one piece in the beam path,
      // we can short circuit fire here
      if (bdir%2==1) {
        if (p->rnk_count[rnk_of(sq)] < 2) return 0;
      } else {
        if (p->fil_count[fil_of(sq)] < 2) return 0;
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
piece_t make_move(position_t *old, position_t *p, move_t mv) {
  assert(mv != 0);

  // move phase 1
  low_level_make_move(old, p, mv);

  // move phase 2
  square_t victim_sq = fire(p);

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
    p->key ^= zob[victim_sq][p->victim & PIECE_MASK];   // remove from board
    p->board[victim_sq] = 0;
    p->key ^= zob[victim_sq][0];

    if (ptype_of(p->victim) == PAWN) {
      color_t col = color_of(p->victim);
      int start_index = 6 * col;
      assert(victim_sq == p->pawns[start_index + index_of(p->victim)]);
      p->pawns[start_index + index_of(p->victim)] = -1;
    }

    // Victim impacts counts of row and column
    p->fil_count[fil_of(victim_sq)]--;
    p->rnk_count[rnk_of(victim_sq)]--;

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
static uint64_t perft_search(position_t *p, int depth, int ply) {
  uint64_t node_count = 0;
  position_t np;
  sortable_move_t lst[MAX_NUM_MOVES];
  int num_moves;
  int i;

  if (depth == 0) {
    return 1;
  }

  num_moves = generate_all(p, lst, true);

  if (depth == 1) {
    return num_moves;
  }

  for (i = 0; i < num_moves; i++) {
    move_t mv = get_move(lst[i]);

    low_level_make_move(p, &np, mv);  // make the move baby!
    square_t victim_sq = fire(&np);  // the guy to disappear

    if (victim_sq != 0) {            // hit a piece
      ptype_t typ = ptype_of(np.board[victim_sq]);
      assert((typ != EMPTY) && (typ != INVALID));
      if (typ == PAWN) {
        color_t col = color_of(np.board[victim_sq]);
        int start_index = col * 6;
        assert(victim_sq == np.pawns[start_index + index_of(np.board[victim_sq])]);
        np.pawns[start_index + index_of(np.board[victim_sq])] = -1;
      } else if (typ == KING) {  // do not expand further: hit a King
        node_count++;
        continue;
      }
      np.victim = np.board[victim_sq];
      np.key ^= zob[victim_sq][np.victim & PIECE_MASK];   // remove from board
      np.board[victim_sq] = 0;
      np.key ^= zob[victim_sq][0];
    }

    uint64_t partialcount = perft_search(&np, depth-1, ply+1);
    node_count += partialcount;
  }

  return node_count;
}

// help to verify the move generator
void do_perft(position_t *gme, int depth, int ply) {
  fen_to_pos(gme, "");

  for (int d = 1; d <= depth; d++) {
    printf("perft %2d ", d);
    uint64_t j = perft_search(gme, d, 0);
    printf("%" PRIu64 "\n", j);
  }
}

void display(position_t *p) {
  char buf[MAX_CHARS_IN_MOVE];

  printf("\ninfo Ply: %d\n", p->ply);
  printf("info Color to move: %s\n", color_to_str(color_to_move_of(p)));

  square_to_str(p->kloc[WHITE], buf);
  printf("info White King: %s, ", buf);
  square_to_str(p->kloc[BLACK], buf);
  printf("info Black King: %s\n", buf);

  if (p->last_move != 0) {
    move_to_str(p->last_move, buf);
    printf("info Last move: %s\n", buf);
  } else {
    printf("info Last move: NULL\n");
  }

  for (rnk_t r = BOARD_WIDTH - 1; r >=0 ; --r) {
    printf("\ninfo %1d  ", r);
    for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
      square_t sq = square_of(f, r);

      if (ptype_of(p->board[sq]) == INVALID) {     // invalid square
        assert(false);                             // This is bad!
      }

      if (ptype_of(p->board[sq]) == EMPTY) {       // empty square
        printf(" --");
        continue;
      }

      int ori = ori_of(p->board[sq]);  // orientation
      color_t c = color_of(p->board[sq]);

      if (ptype_of(p->board[sq]) == KING) {
        printf(" %2s", king_ori_to_rep[c][ori]);
        continue;
      }

      if (ptype_of(p->board[sq]) == PAWN) {
        printf(" %2s", pawn_ori_to_rep[c][ori]);
        continue;
      }
    }
  }

  printf("\n\ninfo    ");
  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    printf(" %c ", 'a'+f);
  }
  printf("\n\n");
}

