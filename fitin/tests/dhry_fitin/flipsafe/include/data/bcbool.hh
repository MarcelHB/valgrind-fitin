// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include <bitcount.hh>
template <unsigned C = 8, typename T = unsigned int>
class bcbool {
public:
  static_assert( (T)~0 > 0, "Unsigned type required" );
  static_assert( C > 0 && C < (8*sizeof(T)), "Illegal bit count" );

  T i;
  inline bcbool() {}
  inline bcbool(bool value) : i(-value) {}
  inline explicit bcbool(T value) : i(value) {}

  operator bool() const {
    return bitcount(i) > C;
  }
};

template <typename T>
class bcbool<1, T> {
public:
  static_assert( (T)~0 > 0, "Unsigned type required" );

  T i;
  inline bcbool() {}
  inline bcbool(bool value) : i(-value) {}
  inline explicit bcbool(T value) : i(value) {}

  operator bool() const {
    return (i & (i-1)) != 0;
  }
};


template <unsigned C, typename T>
bcbool<C,T> operator !(const bcbool<C,T>& b)
{
  return bcbool<C,T>(~b.i);
}

