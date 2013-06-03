// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define COMPOP BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_DUP_COMPOPS)
template <typename T, typename U>
inline dup<T>& operator COMPOP(dup<T>& lhs, const U& rhs) {
  lhs.original COMPOP rhs;
  lhs.backup COMPOP rhs;
  return lhs;
}
template <typename T, typename U>
inline dup<T>& operator COMPOP(dup<T>& lhs, const dup<U>& rhs) {
  lhs.original COMPOP rhs.original;
  lhs.backup COMPOP rhs.backup;
  return lhs;
}
