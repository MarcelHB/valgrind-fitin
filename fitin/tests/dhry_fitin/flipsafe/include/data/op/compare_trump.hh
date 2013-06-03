// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define COMPARE BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_TRUMP_COMPARES)
template <typename T, unsigned A>
inline bool operator COMPARE(const trump<T,A> & x, const T & y)
{
  if (x.original COMPARE y) {
      if (unlikely(!(x.backup COMPARE (A*y))))
          fault_detected();
      return true;
  } else {
      if (unlikely(x.backup COMPARE (A*y)))
          fault_detected();
      return false;
  }
}
template <typename T, unsigned A>
inline bool operator COMPARE(const T & x, const trump<T,A> & y)
{
  if (x COMPARE y.original) {
      if (unlikely(!((A*x) COMPARE y.backup)))
          fault_detected();
      return true;
  } else {
      if (unlikely((A*x) COMPARE y.backup))
          fault_detected();
      return false;
  }
}
template <typename T, unsigned A, unsigned B>
inline bool operator COMPARE(const trump<T, A> & x, const trump<T, B> & y)
{
  if (x.original COMPARE y.original) {
      if (unlikely(!(B*x.backup COMPARE A*y.backup)))
          fault_detected();
      return true;
  } else {
      if (unlikely(B*x.backup COMPARE A*y.backup))
          fault_detected();
      return false;
  }
}
