// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define COMPARE BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_DUP_COMPARES)
template <typename T>
inline bool operator COMPARE(const dup<T> & x, const T & y)
{
  if (x.original COMPARE y) {
      // Note, there is no point in checking that the backup value is identical
      // to the original if we are only interested in the value of the comparison.
      // In that case, we can safely ignore a bit-error that has no impact on
      // the comparison result.
      // In theory, it should be possible to optimize away this check in those
      // cases where a complete consistency check is ultimately required. Sadly,
      // none of the major C++ compilers seem to implement this optimization and
      // thus one might therefore choose to implement a complete check here.
      // As an alternative, one could parameterize the class with a comparison
      // check policy, that would check either partial, full, or no consistency.
      if (unlikely(!(x.backup COMPARE y)))
          fault_detected();
      return true;
  } else {
      if (unlikely(x.backup COMPARE y))
          fault_detected();
      return false;
  }
}
template <typename T>
inline bool operator COMPARE(const T & x, const dup<T> & y)
{
  if (x COMPARE y.original) {
      if (unlikely(!(x COMPARE y.backup)))
          fault_detected();
      return true;
  } else {
      if (unlikely(x COMPARE y.backup))
          fault_detected();
      return false;
  }
}
template <typename T>
inline bool operator COMPARE(const dup<T> & x, const dup<T> & y)
{
  if (x.original COMPARE y.original) {
      if (unlikely(!(x.backup COMPARE y.backup)))
          fault_detected();
      return true;
  } else {
      if (unlikely(x.backup COMPARE y.backup))
          fault_detected();
      return false;
  }
}
