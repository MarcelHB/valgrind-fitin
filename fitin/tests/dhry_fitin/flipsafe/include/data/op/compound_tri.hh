// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define COMPOP BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_TRI_COMPOPS)
template <typename T, typename U>
inline tri<T>& operator COMPOP(tri<T>& lhs, const U& rhs) {
  lhs.original COMPOP rhs;
  lhs.backup1 COMPOP rhs;
  lhs.backup2 COMPOP rhs;
  return lhs;
}
template <typename T, typename U>
inline tri<T>& operator COMPOP(tri<T>& lhs, const tri<U>& rhs) {
  lhs.original COMPOP rhs.original;
  lhs.backup1 COMPOP rhs.backup1;
  lhs.backup2 COMPOP rhs.backup2;
  return lhs;
}

