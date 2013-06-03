// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#pragma once
#include "handler.hh"
namespace sihft {

inline void assert(bool condition)
{
  if (__builtin_expect(!condition, 0)) fault_detected();
}

}

