// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include "handler.hh"
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

int basic(int input)
{
  int x = input, y = input, z = input;
  asm ("" : "+g" (y), "+g" (z));
  // Hardcode first prediction to factor out branch prediction
  if (likely(x <= 2) /*&& likely(y <= 2) && likely(z <= 2)*/ )
  {
    x += 1; y += 1; z += 1;
  }
  /* else ... (see mandup.hh) */
  if ( unlikely(x != y) || unlikely(x != z) )
  {
    if (y == z)
      return y;
    else if (likely(x == y || x== z))
      return x;
    else
      sihft::fault_detected();
  }
  return x;
}

