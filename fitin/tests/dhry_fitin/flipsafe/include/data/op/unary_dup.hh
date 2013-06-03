// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define UNOP BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_DUP_UNOPS)
template <typename T>
inline dup<T>& operator UNOP(dup<T>& x) {
  dup<T> result;
  result.original = UNOP x.original;
  result.backup = UNOP x.backup;
  return result;
}
