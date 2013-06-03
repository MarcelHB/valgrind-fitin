// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#pragma once
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include "handler.hh"
#include "assert.hh"
#include "protected_clone.hh"
#include <boost/operators.hpp>
#include <boost/preprocessor.hpp>
#include <iostream>
namespace sihft {

//.The maximum value (for signed/unsigned integers), for unsigned values we set all bits by
// complementing zero, and for signed values we set all but the most significant (sizeof(T)*8-1).
// We check for unsigned types by checking that -1 is bigger than zero.
template <typename T>
struct int_max
{
  enum { value = (T)~((T)-1 > 0 ? 0ull : 1ull << (sizeof(T)*8 - 1)) };
};

template <typename T, int A>
struct max_value
{
  enum { value = int_max<T>::value / A };
};



template <int A, typename T>
inline static T scale_check(const T & x) {
#ifndef PROTECTION_DISABLED
  // Multiplication might overflow, and we provide this function to provide
  // an early check to ensure that this doesn't happen. Ultimately, one would
  // probably want to specify a scale-check policy for the trump annotation
  // rather than hardcoding the decision here.
  assert ( likely((x <= max_value<T,A>::value)) );
#endif
  return A * x;
}

template <typename T, int A = 3>
class trump
  : boost::euclidean_ring_operators< trump<T, A> >
  , boost::euclidean_ring_operators< trump<T, A>, T >
  , boost::bitwise< trump<T, A> >
  , boost::bitwise< trump<T, A>, T >
  , boost::shiftable< trump<T, A> >
  , boost::shiftable< trump<T, A>, T >
  , boost::unit_steppable< trump<T, A> >
{
public:
  T original, backup;

  inline trump() { }
  inline trump(const T & x)
    : original(x), backup(scale_check<A>(protected_clone(x)))
  {
  }

  inline ~trump() {
    assert_valid();
  }

  inline void assert_valid() const {
    assert( A*original == backup );
  }

  inline operator const T() const {
    assert_valid();
    return original;
  }
  inline operator T() {
#ifdef SIHFT_IN_TEST
    assert_valid();
#else
    if ( unlikely(A*original != backup) )
    {
      if (backup % A == 0)
        original = backup / A;
      else
        backup = A * original;
    }
#endif
    return original;
  }

  inline trump& operator=(const T & x)
  {
    backup = scale_check<A>(protected_clone(original = x));
    return *this;
  }

  inline trump& operator--()
  {
    return *this -= static_cast<T>(1);
  }

  inline trump& operator++()
  {
    return *this += static_cast<T>(1);
  }

};

#define SIHFT_TRUMP_UNOPS (4, (!, ~, +, -))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_TRUMP_UNOPS)-1)
#define BOOST_PP_FILENAME_1 "data/op/unary_trump.hh"
#include BOOST_PP_ITERATE()

#define SIHFT_TRUMP_COMPOPS (10, (+=, -=, *=, /=, %=, |=, &=, ^=, <<=, >>=))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_TRUMP_COMPOPS)-1)
#define BOOST_PP_FILENAME_1 "data/op/compound_trump.hh"
#include BOOST_PP_ITERATE()

#define SIHFT_TRUMP_COMPARES  (6, (<, <=, ==, !=, >=, >))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_TRUMP_COMPARES)-1)
#define BOOST_PP_FILENAME_1 "data/op/compare_trump.hh"
#include BOOST_PP_ITERATE()

}
