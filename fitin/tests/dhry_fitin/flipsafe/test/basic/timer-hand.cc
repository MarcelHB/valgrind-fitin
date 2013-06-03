// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include "handler.hh"
#include "protected_clone.hh"

int basic(int input)
{
  int x[2] = {input, sihft::protected_clone(input)};

  if (likely(x[0] <= 2))
    x[0] += 1;
  if (likely(x[1] <= 2))
    x[1] += 1;

  if (x[0] != x[1])
    sihft::fault_detected();

  return x[0];
}
