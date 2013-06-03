// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define UNOP BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_TRI_UNOPS)
template <typename T>
inline tri<T>& operator UNOP(tri<T>& x) {
  tri<T> result;
  result.original = UNOP x.original;
  result.backup1 = UNOP x.backup1;
  result.backup2 = UNOP x.backup2;
  return result;
}
