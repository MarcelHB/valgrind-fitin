// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include "cflow.hh"

int basic(int input)
{
  sihft::cflow_check c0(0);
  int x = input;
  if (likely(x <= 2))
  {
    sihft::cflow_check c1(1, 0);
    x += 1;
  }
  return x;
}
