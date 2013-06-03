// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include "handler.hh"
#include "protected_clone.hh"

int basic(int input)
{
  static unsigned int block = 0;
  int x = input;
  if (likely(x <= 2))
  {
    if (unlikely(sihft::protected_clone(block) != 0))
      sihft::fault_detected();
    block = 1;
    x += 1;
  }
  if (unlikely(sihft::protected_clone(block) > 1))
    sihft::fault_detected();
  block = 0;
  return x;
}
