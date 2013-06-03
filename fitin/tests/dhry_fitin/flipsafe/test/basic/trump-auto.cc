// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include "data/trump.hh"
int basic(int input)
{
  sihft::trump<int> x = input;
  // Hardcode prediction, so we get consistent results...
  if (likely(x <= 2))
    x += 1;
  return x;
}

