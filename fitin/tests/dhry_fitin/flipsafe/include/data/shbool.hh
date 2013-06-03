// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
template <unsigned S = 4*sizeof(unsigned int), typename T = unsigned int>
class shbool {
public:
  static_assert( (T)~0 > 0, "Unsigned type required" );
  static_assert( (8*sizeof(T)) >= S, "Shift must not exceed data type size" );

  T i;
  inline shbool() {}
  inline shbool(bool value) : i(-value) {}
  inline explicit shbool(T value) : i(value) {}

  operator bool() const {
    return (i & (i >> S)) != 0;
  }
};

template <unsigned S, typename T>
shbool<S,T> operator !(const shbool<S,T>& b)
{
  return shbool<S,T>(~b.i);
}

