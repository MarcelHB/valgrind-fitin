// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
template <unsigned R = 1, typename T = unsigned int>
class rmbool {
public:
  static_assert( (T)~0 > 0, "Unsigned type required" );
  static_assert( (8*sizeof(T)) >= R, "Shift must not exceed data type size" );

  T i;
  inline rmbool() {}
  inline rmbool(bool value) : i(-value) {}
  inline explicit rmbool(T value) : i(value) {}

  operator bool() const {
    return (i & ((i >> R) | (i << (8*sizeof(T)) - R))) != 0; // i & rotate(i,R)
  }
};

template <unsigned R, typename T>
rmbool<R,T> operator !(const rmbool<R,T>& b)
{
  return rmbool<R,T>(~b.i);
}

