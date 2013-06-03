// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#pragma once
// NOTE: Linerangess are referenced from thesis, take care...
namespace sihft
{
template <typename T>
inline T protected_clone(const T & x)
{
#ifdef PROTECTION_DISABLED
  return x;
#else
#ifdef __GNUC__
  T y = x;
  // The asm declaration specifies that the value of y
  // is written by the asm-block, and as gcc doesn't look
  // into the (empty) asm block, it no longer is aware that
  // y is a duplicate of x.
  // The constraints ensure that this will remain inbetween
  // the above definition and any use of y, which is exactly
  // what we want.
  asm ("" : "+g" (y));
#else
  // Declaring the variable volatile ensures that the
  // compiler doesn't remember its value (good) but
  // unfortunately also means that the variable will
  // always be put on the stack instead of in a register.
  volatile T y = x;
#endif
  return y;
#endif
}

}

