// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define UNOP BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_TRUMP_UNOPS)
#define QUOT '
#define UNOPC BOOST_PP_CAT(BOOST_PP_CAT(QUOT,UNOP),QUOT)
// These operations can have considerable overhead compared to their native
// counterparts, and it is unlikely that the compiler can infer the class
// invariant as to be able to do the optimizations itself.
template <typename T, int A, typename U>
inline trump<T, A>& operator UNOP(trump<T,A>& x) {
  trump<T, A> result;
  result.original = UNOP x.original;
#if (UNOPC=='+') || (UNOPC=='-')
  result.backup = UNOP x.backup;
#elif (UNOPC=='!')
  result.backup = A * !x.backup;
#elif (UNOPC=='~')
  result.backup = -x.backup - A;
#else
  result.backup = A * (UNOP (x.backup / A));
#endif
  return result;
}
