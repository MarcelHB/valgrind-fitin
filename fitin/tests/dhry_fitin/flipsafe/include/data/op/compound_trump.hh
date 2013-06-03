// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define COMPOP BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_TRUMP_COMPOPS)
#define QUOT '
#define COMPOPC BOOST_PP_CAT(BOOST_PP_CAT(QUOT,COMPOP),QUOT)
// These operations can have considerable overhead compared to their native
// counterparts, and it is unlikely that the compiler can infer the class
// invariant as to be able to do the optimizations itself.
template <typename T, int A, typename U>
inline trump<T, A>& operator COMPOP(trump<T,A>& lhs, const U& rhs) {
  lhs.original COMPOP rhs;
#if (COMPOPC=='*=') || (COMPOPC=='<<=')
  lhs.backup COMPOP rhs;
#elif (COMPOPC=='+=') || (COMPOPC=='-=') || (COMPOPC=='%=')
  // This optimization requires that lhs.backup is a multiple of A.
  // It is highly unlikely that the compiler keeps track of this in
  // general, and with the previous constructor implementation it
  // wouldn't even be possible (as the multiplication took place
  // before the tricks to ensure the compiler doesn't know the
  // backup value.
  lhs.backup COMPOP (A * rhs);
#else
  lhs.backup /= A;
  lhs.backup COMPOP rhs;
  lhs.backup *= A;
#endif
  return lhs;
}
template <typename T, int A, typename U, int B>
inline trump<T,A>& operator COMPOP(trump<T,A>& lhs, const trump<U,B>& rhs) {
  lhs.original COMPOP rhs.original;
#if (COMPOPC=='*=') || (COMPOPC=='<<=')
  lhs.backup COMPOP (rhs.backup / B);
#elif (COMPOPC=='+=') || (COMPOPC=='-=') || (COMPOPC=='%=')
  // As above, only legal when lhs.backup is a multiple of A (and rhs.backup
  // is a multiple of B)
  lhs.backup COMPOP (rhs.backup * A / B);
#else
  // TODO: Provide both COMPOP and OP, so that we can write this using a
  // single expression instead of updates, so we don't have to bother with
  // aliasing problems at all.
  const U val = rhs.backup / B; // First to allow aliasing of rhs/lhs
  lhs.backup /= A;
  lhs.backup COMPOP val;
  lhs.backup *= A;
#endif
  return lhs;
}
