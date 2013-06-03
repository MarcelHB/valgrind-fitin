// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include "handler.hh"
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

int basic(int input)
{
  int x = input;
  int y = 3 * input;
  asm ("" : "+g" (y));
  // Hardcode branch prediction to factor it out of the performance tests.
  // Also, we can comment out the second condition, as this is automatically
  // checked by the consistency check in the end.
  if (likely(x <= 2) /*&& likely(y <= 6)*/ )
  {
    x += 1; y += 3;
  }
  /* else ... (see manual_under_test_dup_int) */
  if ( !likely(3 * x == y) )
    if (y % 3 == 0)
      x = y / 3;

  return x;
}

