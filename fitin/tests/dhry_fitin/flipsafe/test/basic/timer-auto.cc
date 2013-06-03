// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include "time_redundancy.hh"

int basic(int input)
{
  return sihft::time_redundancy([=](){
    int x = input;
    if (likely(x <= 2))
      x += 1;
    return x;
  });
}
