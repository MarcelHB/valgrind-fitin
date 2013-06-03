// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define COMPARE BOOST_PP_ARRAY_ELEM(BOOST_PP_ITERATION(), SIHFT_TRI_COMPARES)
template <typename T>
inline bool operator COMPARE(const tri<T> & x, const T & y)
{
  if (x.original COMPARE y) {
      if (likely(x.backup1 COMPARE y) && likely(x.backup2 COMPARE y))
          return true;
  } else {
      if (!unlikely(x.backup1 COMPARE y) && !unlikely(x.backup2 COMPARE y))
          return false;
  }
  fault_detected();
}

template <typename T>
inline bool operator COMPARE(const T& x, const tri<T> & y)
{
  if (x COMPARE y.original) {
      if (unlikely(!(x COMPARE y.backup1)) || unlikely(!(x COMPARE y.backup2)))
          fault_detected();
      return true;
  } else {
      if (unlikely(x COMPARE y.backup1) || unlikely(x COMPARE y.backup2))
          fault_detected();
      return false;
  }
}

// To avoid too many comparisons and still make x op x detect an error we simply swap backup1/backup2
template <typename T>
inline bool operator COMPARE(const tri<T> & x, const tri<T> & y)
{
  if (x.original COMPARE y.original) {
      if (unlikely(!(x.backup1 COMPARE y.backup2)) || unlikely(!(x.backup2 COMPARE y.backup1)))
          fault_detected();
      return true;
  } else {
      if (unlikely(x.backup1 COMPARE y.backup2) || unlikely(x.backup2 COMPARE y.backup1))
          fault_detected();
      return false;
  }
}

