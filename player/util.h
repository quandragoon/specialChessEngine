/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
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

#ifndef UTIL_H
#define UTIL_H


#ifndef DEBUG_VERBOSE
#define DEBUG_VERBOSE 0
#endif 

#if DEBUG_VERBOSE
// DEBUG logging thresh level; 
// When calling debug_log, only messages with log_level >= DEBUG_LOG_THRESH get printed
// so "important" debugging messages should get higher level
#define DEBUG_LOG_THRESH 3
#define DEBUG_LOG(arg...) debug_log(arg)
#define WHEN_DEBUG_VERBOSE(ex) ex
#else
#define DEBUG_LOG_THRESH 0
#define DEBUG_LOG(arg...)
#define WHEN_DEBUG_VERBOSE(ex)
#endif // EVAL_DEBUG_VERBOSE

void debug_log(int log_level, const char *str, ...);
double  milliseconds();

// Inlining.


// Public domain code for JLKISS64 RNG - long period KISS RNG producing
// 64-bit results
static inline uint64_t myrand() {
  static int first_time = 0;
  // Seed variables
  static uint64_t x = 123456789123ULL, y = 987654321987ULL;
  static unsigned int z1 = 43219876, c1 = 6543217, z2 = 21987643,
                      c2 = 1732654;  // Seed variables
  static uint64_t t;

  if (first_time) {
    int  i;
    FILE *f = fopen("/dev/urandom", "r");
    for (i = 0; i < 64; i += 8) {
      x = x ^ getc(f) << i;
      y = y ^ getc(f) << i;
    }

    fclose(f);
    first_time = 0;
  }

  x = 1490024343005336237ULL * x + 123456789;

  y ^= y << 21;
  y ^= y >> 17;
  y ^= y << 30;  // Do not set y=0!

  t = 4294584393ULL * z1 + c1;
  c1 = t >> 32;
  z1 = t;

  t = 4246477509ULL * z2 + c2;
  c2 = t >> 32;
  z2 = t;

  return x + y + z1 + ((uint64_t)z2 << 32);  // Return 64-bit result
}


#endif  // UTIL_H
