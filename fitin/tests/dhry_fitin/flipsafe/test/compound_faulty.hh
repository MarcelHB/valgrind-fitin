// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define COMPOP BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), COMPOPS)
template <typename T, typename U>
inline faulty<T>& operator COMPOP(faulty<T>& lhs, const U & rhs) {
  lhs.value COMPOP rhs;
  return lhs;
}
template <typename T>
inline faulty<T>& operator COMPOP(faulty<T>& lhs, const faulty<T>& rhs) {
  lhs.value COMPOP rhs.value;
  return lhs;
}
