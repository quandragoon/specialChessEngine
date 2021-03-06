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
#include <stdlib.h>
#include <stdio.h>

#include "eval.h"

// -----------------------------------------------------------------------------
// Evaluation
// -----------------------------------------------------------------------------

typedef int32_t ev_score_t;  // Static evaluator uses "hi res" values

static char neighbor_map[ARR_SIZE] = {
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
};

int RANDOMIZE;

int HATTACK;
int PBETWEEN;
int KFACE;
int KAGGRESSIVE;
int MOBILITY;

// Heuristics for static evaluation

// returns true if c lies on or between a and b, which are not ordered
static inline bool between(int c, int a, int b) {
  return ((c >= a) && (c <= b)) || ((c <= a) && (c >= b));
}

// PBETWEEN heuristic: Bonus for Pawn at (f, r) in rectangle defined by Kings at the corners
static inline ev_score_t pbetween(position_t *p, fil_t f, rnk_t r) {
  bool is_between =
      between(f, fil_of(p->kloc[WHITE]), fil_of(p->kloc[BLACK])) &&
      between(r, rnk_of(p->kloc[WHITE]), rnk_of(p->kloc[BLACK]));
  return is_between ? PBETWEEN : 0;
}


// KFACE and KAGGRESSIVE heuristic
static inline ev_score_t kface_kaggressive (position_t *p, square_t sq, rnk_t r, fil_t f, color_t c) {
  // rnk_t r = rnk_of(sq);
  // fil_t f = fil_of(sq);
  // color_t c = color_of(p->board[sq]);
  square_t opp_sq = p->kloc[opp_color(c)];
  int delta_fil = fil_of(opp_sq) - f;
  int delta_rnk = rnk_of(opp_sq) - r;
  int bonus1, bonus2;

  switch (ori_of(p->board[sq])) {
   case NN:
    // bonus1 = delta_rnk;
    bonus1 = delta_rnk - abs(delta_fil);
    break;

   case EE:
    // bonus1 = delta_fil;
    bonus1 = delta_fil - abs(delta_rnk);
    break;

   case SS:
    // bonus1 = -delta_rnk;
    bonus1 = -delta_rnk - abs(delta_fil);
    break;

   case WW:
    // bonus1 = -delta_fil;
    bonus1 = -delta_fil - abs(delta_rnk);
    break;

   default:
    bonus1 = 0;
    printf("Illegal King orientation\n");
    assert(false);
  }
  
  bonus2 = 0;

  if (delta_fil >= 0 && delta_rnk >= 0) {
    bonus2 = (f + 1) * (r + 1);
  } else if (delta_fil <= 0 && delta_rnk >= 0) {
    bonus2 = (BOARD_WIDTH - f) * (r + 1);
  } else if (delta_fil <= 0 && delta_rnk <= 0) {
    bonus2 = (BOARD_WIDTH - f) * (BOARD_WIDTH - r);
  } else if (delta_fil >= 0 && delta_rnk <= 0) {
    bonus2 = (f + 1) * (BOARD_WIDTH - r);
  }

  /*
  if (delta_fil >= 0)
    if(delta_rnk >= 0)
      bonus2 = (f + 1) * (r + 1);
    else
      bonus2 = (f + 1) * (BOARD_WIDTH - r);
  else
    if(delta_rnk >= 0)
      bonus2 = (BOARD_WIDTH - f) * (r + 1);
    else
      bonus2 = (BOARD_WIDTH - f) * (BOARD_WIDTH - r);
  */

  return (bonus1 * KFACE) / (abs(delta_rnk) + abs(delta_fil)) + (KAGGRESSIVE * bonus2) / (BOARD_WIDTH * BOARD_WIDTH);
}


// KFACE heuristic: bonus (or penalty) for King facing toward the other King
ev_score_t kface(position_t *p, square_t sq) {
  // square_t sq = square_of(f, r);
  piece_t x = p->board[sq];
  color_t c = color_of(x);
  rnk_t r = rnk_of(sq);
  fil_t f = fil_of(sq);
  square_t opp_sq = p->kloc[opp_color(c)];
  int delta_fil = fil_of(opp_sq) - f;
  int delta_rnk = rnk_of(opp_sq) - r;
  int bonus;

  switch (ori_of(p->board[sq])) {
   case NN:
    bonus = delta_rnk;
    break;

   case EE:
    bonus = delta_fil;
    break;

   case SS:
    bonus = -delta_rnk;
    break;

   case WW:
    bonus = -delta_fil;
    break;

   default:
    bonus = 0;
    printf("Illegal King orientation\n");
    assert(false);
  }

  return (bonus * KFACE) / (abs(delta_rnk) + abs(delta_fil));
}

// KAGGRESSIVE heuristic: bonus for King with more space to back
static inline ev_score_t kaggressive(position_t *p, square_t sq) {
  // square_t sq = square_of(f, r);
  piece_t x = p->board[sq];
  color_t c = color_of(x);
  // assert(ptype_of(x) == KING);
  rnk_t r = rnk_of(sq);
  fil_t f = fil_of(sq);

  square_t opp_sq = p->kloc[opp_color(c)];
  fil_t of = fil_of(opp_sq);
  rnk_t _or = (rnk_t) rnk_of(opp_sq);

  int delta_fil = of - f;
  int delta_rnk = _or - r;

  int bonus = 0;

  if (delta_fil >= 0 && delta_rnk >= 0) {
    bonus = (f + 1) * (r + 1);
  } else if (delta_fil <= 0 && delta_rnk >= 0) {
    bonus = (BOARD_WIDTH - f) * (r + 1);
  } else if (delta_fil <= 0 && delta_rnk <= 0) {
    bonus = (BOARD_WIDTH - f) * (BOARD_WIDTH - r);
  } else if (delta_fil >= 0 && delta_rnk <= 0) {
    bonus = (f + 1) * (BOARD_WIDTH - r);
  }

  return (KAGGRESSIVE * bonus) / (BOARD_WIDTH * BOARD_WIDTH);
}

void reset_neighbors(position_t *p, color_t color) {
  square_t king_sq = p->kloc[color];
  neighbor_map[king_sq] = 0;
  for (int d = 0; d < 8; ++d) {
    square_t neighbor = king_sq + dir_of(d);
    char mask = -1;
    mask <<= 1;
    neighbor_map[neighbor] &= mask;
  }
}

// Array for harmonic distances (maps inputs square_t a and square_t b to h_dist)
static float distances[] = {
  0.25, 0.268, 0.292, 0.325, 0.375, 0.458, 0.625, 1.125, 0.625, 0.458, 0.375, 0.325, 0.292, 0.268, 0.25,
  0.268, 0.286, 0.31, 0.343, 0.393, 0.476, 0.643, 1.143, 0.643, 0.476, 0.393, 0.343, 0.31, 0.286, 0.268,
  0.292, 0.31, 0.333, 0.367, 0.417, 0.5, 0.667, 1.167, 0.667, 0.5, 0.417, 0.367, 0.333, 0.31, 0.292,
  0.325, 0.343, 0.367, 0.4, 0.45, 0.533, 0.7, 1.2, 0.7, 0.533, 0.45, 0.4, 0.367, 0.343, 0.325,
  0.375, 0.393, 0.417, 0.45, 0.5, 0.583, 0.75, 1.25, 0.75, 0.583, 0.5, 0.45, 0.417, 0.393, 0.375,
  0.458, 0.476, 0.5, 0.533, 0.583, 0.667, 0.833, 1.333, 0.833, 0.667, 0.583, 0.533, 0.5, 0.476, 0.458,
  0.625, 0.643, 0.667, 0.7, 0.75, 0.833, 1.0, 1.5, 1.0, 0.833, 0.75, 0.7, 0.667, 0.643, 0.625,
  1.125, 1.143, 1.167, 1.2, 1.25, 1.333, 1.5, 2.0, 1.5, 1.333, 1.25, 1.2, 1.167, 1.143, 1.125,
  0.625, 0.643, 0.667, 0.7, 0.75, 0.833, 1.0, 1.5, 1.0, 0.833, 0.75, 0.7, 0.667, 0.643, 0.625,
  0.458, 0.476, 0.5, 0.533, 0.583, 0.667, 0.833, 1.333, 0.833, 0.667, 0.583, 0.533, 0.5, 0.476, 0.458,
  0.375, 0.393, 0.417, 0.45, 0.5, 0.583, 0.75, 1.25, 0.75, 0.583, 0.5, 0.45, 0.417, 0.393, 0.375,
  0.325, 0.343, 0.367, 0.4, 0.45, 0.533, 0.7, 1.2, 0.7, 0.533, 0.45, 0.4, 0.367, 0.343, 0.325,
  0.292, 0.31, 0.333, 0.367, 0.417, 0.5, 0.667, 1.167, 0.667, 0.5, 0.417, 0.367, 0.333, 0.31, 0.292,
  0.268, 0.286, 0.31, 0.343, 0.393, 0.476, 0.643, 1.143, 0.643, 0.476, 0.393, 0.343, 0.31, 0.286, 0.268,
  0.25, 0.268, 0.292, 0.325, 0.375, 0.458, 0.625, 1.125, 0.625, 0.458, 0.375, 0.325, 0.292, 0.268, 0.25,
};

// Harmonic-ish distance: 1/(|dx|+1) + 1/(|dy|+1)
float h_dist(square_t a, square_t b) {
  int delta_fil = fil_of(a) - fil_of(b) + 7;
  int delta_rnk = rnk_of(a) - rnk_of(b) + 7;
  float x = distances[(15 * delta_fil) + (delta_rnk)];
  return x;
}


void mobility(position_t *p, color_t color, int* mob, int* h_att) {
  color_t c = opp_color(color);

  // mobility = # safe squares around enemy king

  square_t king_sq = p->kloc[color];
  assert(ptype_of(p->board[king_sq]) == KING);
  assert(color_of(p->board[king_sq]) == color);

  int mobility = 1;
  neighbor_map[king_sq] = 1;
  char mask = 1; 
  for (int d = 0; d < 8; ++d) {
    square_t neighbor = king_sq + dir_of(d);
    neighbor_map[neighbor] |= mask;
    mobility += (neighbor_map[neighbor] == 1);
  }

  // Fire laser and check if we hit any of the
  // neighboring positions.
  // Mimic the functionality of mark_laser_path
  // except we eliminated some extra work of
  // saving vals and initializing the map
  // every time.
  square_t sq = p->kloc[c];
  int bdir = ori_of(p->board[sq]);

  float h_attackable = 0; 
  assert(ptype_of(p->board[sq]) == KING);
  assert(color_of(p->board[sq]) != color);
  h_attackable += h_dist(king_sq, sq);

  while (true) {
    sq += beam_of(bdir);
    assert(sq < ARR_SIZE && sq >= 0);
    mobility -= (neighbor_map[sq] == 1);
    neighbor_map[sq] &= (-2);
    assert(mobility >= 0);
    switch (ptype_of(p->board[sq])) {
     case EMPTY:  // empty square
      h_attackable += h_dist(king_sq, sq);
      break;
     case PAWN:  // Pawn
      bdir = reflect_of(bdir, ori_of(p->board[sq]));
      h_attackable += h_dist(king_sq, sq);
      if (bdir < 0) {  // Hit back of Pawn
        // Resets opposing king's neighbor spaces to 0 in
        // global map.
        reset_neighbors(p, color);
        *mob = mobility;
        *h_att = h_attackable;
        return;
      }
      break;
     case KING:  // King
      reset_neighbors(p, color);
      h_attackable += h_dist(king_sq, sq);
      *mob = mobility;
      *h_att = h_attackable;
      return;  // sorry, game over my friend!
     case INVALID:  // Ran off edge of board
      reset_neighbors(p, color);
      *mob = mobility;
      *h_att = h_attackable;
      return;
      break;
     default:  // Shouldna happen, man!
      assert(false);
      break;
    }
  }
}


static void mark_laser_path(position_t *p, char *laser_map, color_t c,
                     char mark_mask) {
  position_t np = *p;

  // Fire laser, recording in laser_map
  square_t sq = np.kloc[c];
  int bdir = ori_of(np.board[sq]);

  assert(ptype_of(np.board[sq]) == KING);
  laser_map[sq] |= mark_mask;

  while (true) {
    sq += beam_of(bdir);
    laser_map[sq] |= mark_mask;
    assert(sq < ARR_SIZE && sq >= 0);

    switch (ptype_of(p->board[sq])) {
     case EMPTY:  // empty square
      break;
     case PAWN:  // Pawn
      bdir = reflect_of(bdir, ori_of(p->board[sq]));
      if (bdir < 0) {  // Hit back of Pawn
        return;
      }
      break;
     case KING:  // King
      return;  // sorry, game over my friend!
      break;
     case INVALID:  // Ran off edge of board
      return;
      break;
     default:  // Shouldna happen, man!
      assert(false);
      break;
    }
  }
}

static int old_mobility(position_t *p, color_t color) {
  color_t c = opp_color(color);
  char laser_map[ARR_SIZE];

  for (int i = 0; i < ARR_SIZE; ++i) {
    laser_map[i] = 4;   // Invalid square
  }

  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
      laser_map[square_of(f, r)] = 0;
    }
  }

  mark_laser_path(p, laser_map, c, 1);  // 1 = path of laser with no moves

  // mobility = # safe squares around enemy king

  square_t king_sq = p->kloc[color];
  assert(ptype_of(p->board[king_sq]) == KING);
  assert(color_of(p->board[king_sq]) == color);

  int mobility = 0;
  if (laser_map[king_sq] == 0) {
    mobility++;
  }
  for (int d = 0; d < 8; ++d) {
    square_t sq = king_sq + dir_of(d);
    if (laser_map[sq] == 0) {
      mobility++;
    }
  }
  return mobility;
}

static int old_h_squares_attackable(position_t *p, color_t c) {
  char laser_map[ARR_SIZE];

  for (int i = 0; i < ARR_SIZE; ++i) {
    laser_map[i] = 4;   // Invalid square
  }

  for (fil_t f = 0; f < BOARD_WIDTH; ++f) {
    for (rnk_t r = 0; r < BOARD_WIDTH; ++r) {
      laser_map[square_of(f, r)] = 0;
    }
  }

  mark_laser_path(p, laser_map, c, 1);  // 1 = path of laser with no moves

  square_t o_king_sq = p->kloc[opp_color(c)];
  assert(ptype_of(p->board[o_king_sq]) == KING);
  assert(color_of(p->board[o_king_sq]) != c);

  float h_attackable = 0;
  for (fil_t f = 0; f < BOARD_WIDTH; f++) {
    for (rnk_t r = 0; r < BOARD_WIDTH; r++) {
      square_t sq = square_of(f, r);
      if (laser_map[sq] != 0) {
        h_attackable += h_dist(sq, o_king_sq);
      }
    }
  }
  return h_attackable;
}

// Static evaluation.  Returns score
score_t eval(position_t *p, bool verbose) {
  // verbose = true: print out components of score
  ev_score_t score[2] = { 0, 0 };
  //  int corner[2][2] = { {INF, INF}, {INF, INF} };
  ev_score_t bonus;
  char buf[MAX_CHARS_IN_MOVE];

  for (color_t c = 0; c < 2; c++) {
    int index = (c * 6);
    for (int i = 0; i < 6; ++i) {
      square_t sq = p->pawns[index + i];
      if (sq < 0) continue;
      fil_t f = fil_of(sq);
      rnk_t r = rnk_of(sq);

      if (verbose) {
        square_to_str(sq, buf);
      }
      // MATERIAL heuristic: Bonus for each Pawn
      bonus = PAWN_EV_VALUE;
      if (verbose) {
        printf("ACTUAL- MATERIAL bonus %d for %s Pawn on %s\n", bonus, color_to_str(c), buf);
      }
      score[c] += bonus;

      // PBETWEEN heuristic
      bonus = pbetween(p, f, r);
      if (verbose) {
        printf("ACTUAL- PBETWEEN bonus %d for %s Pawn on %s\n", bonus, color_to_str(c), buf);
      }
      score[c] += bonus;
    }
    square_t king_sq = p->kloc[c];
    fil_t king_f = fil_of(king_sq);
    rnk_t king_r = rnk_of(king_sq);

    /*
    // KFACE heuristic
    bonus = kface(p, king_sq);
    if (verbose) {
      printf("KFACE bonus %d for %s King on %s\n", bonus,
             color_to_str(c), buf);
    }
    score[c] += bonus;

    // KAGGRESSIVE heuristic
    bonus = kaggressive(p, king_sq);
    if (verbose) {
      printf("KAGGRESSIVE bonus %d for %s King on %s\n", bonus, color_to_str(c), buf);
    }
    score[c] += bonus;
    */

    score[c] += kface_kaggressive(p, king_sq, king_r, king_f, c);
  }
    
  int whsa, bhsa;
  int wm, bm;

  mobility(p, WHITE, &wm, &bhsa);
  mobility(p, BLACK, &bm, &whsa);
  assert(whsa == old_h_squares_attackable(p, WHITE));
  assert(bhsa == old_h_squares_attackable(p, BLACK));
  assert(wm == old_mobility(p, WHITE));
  assert(bm == old_mobility(p, BLACK));


  ev_score_t w_hattackable = HATTACK * whsa;
  score[WHITE] += w_hattackable;
  if (verbose) {
    printf("HATTACK bonus %d for White\n", w_hattackable);
  }
  ev_score_t b_hattackable = HATTACK * bhsa;
  score[BLACK] += b_hattackable;
  if (verbose) {
    printf("HATTACK bonus %d for Black\n", b_hattackable);
  }

  int w_mobility = MOBILITY * wm;
  score[WHITE] += w_mobility;
  if (verbose) {
    printf("MOBILITY bonus %d for White\n", w_mobility);
  }
  int b_mobility = MOBILITY * bm;
  score[BLACK] += b_mobility;
  if (verbose) {
    printf("MOBILITY bonus %d for Black\n", b_mobility);
  }

  // score from WHITE point of view
  ev_score_t tot = score[WHITE] - score[BLACK];

  if (RANDOMIZE) {
    ev_score_t  z = rand() % (RANDOMIZE*2+1);
    tot = tot + z - RANDOMIZE;
  }

  if (color_to_move_of(p) == BLACK) {
    tot = -tot;
  }
  return tot / EV_SCORE_RATIO;
}
