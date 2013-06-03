// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
template <typename T = unsigned int>
class csbool {
public:
  T i;
  inline csbool() {}
  inline csbool(bool value) : i(-value) {}
  inline explicit csbool(T value) : i(value) {}

  operator bool() const {
    return i != 0;
  }
};

template <typename T>
csbool<T> operator !(const csbool<T>& b)
{
  return csbool<T>(~b.i);
}

