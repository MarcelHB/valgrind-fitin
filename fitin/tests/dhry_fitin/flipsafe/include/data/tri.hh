// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#pragma once
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include "handler.hh"
#include "assert.hh"
#include "protected_clone.hh"
#include <boost/operators.hpp>
#include <boost/preprocessor.hpp>
namespace sihft {

template <typename T>
class tri
  : boost::euclidean_ring_operators< tri<T> >
  , boost::euclidean_ring_operators< tri<T>, T >
  , boost::bitwise< tri<T> >
  , boost::bitwise< tri<T>, T >
  , boost::shiftable< tri<T> >
  , boost::shiftable< tri<T>, T >
  , boost::unit_steppable< tri<T> >
{
public:
  T original, backup1, backup2;

  inline tri() { }
  inline tri(const T & x)
    : original(x),
      backup1(protected_clone(x)),
      backup2(protected_clone(backup1))
  {
  }

  inline ~tri() {
    assert_valid();
  }

  inline void assert_valid() const {
    assert( (original == backup1) && (original == backup2) );
  }

  inline operator const T() const {
    assert_valid();
    return original;
  }
  inline operator T() {
    // Repair the original value from the backup if necessary
    if ( unlikely(original != backup1) || unlikely(original != backup2) )
    {
      if ( unlikely(backup1 == backup2) )
        original = backup1;
      else if (original == backup1)
        backup2 = original;
      else if (original == backup2)
        backup1 = original;
      else /* if (all different) */
        fault_detected(); 
    }

    return original;
  }

  inline tri& operator=(const T & x)
  {
    // Assign separately, to ensure compiler doesn't know any relations between the variables...
    backup2 =
      protected_clone(backup1 =
          protected_clone(original = x));
    return *this;
  }

  inline tri& operator--()
  {
    return *this -= static_cast<T>(1);
  }

  inline tri& operator++()
  {
    return *this += static_cast<T>(1);
  }

};

#define SIHFT_TRI_UNOPS (4, (!, ~, +, -))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_TRI_UNOPS)-1)
#define BOOST_PP_FILENAME_1 "data/op/unary_tri.hh"
#include BOOST_PP_ITERATE()

#define SIHFT_TRI_COMPOPS (10, (+=, -=, *=, /=, %=, |=, &=, ^=, <<=, >>=))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_TRI_COMPOPS)-1)
#define BOOST_PP_FILENAME_1 "data/op/compound_tri.hh"
#include BOOST_PP_ITERATE()

#define SIHFT_TRI_COMPARES  (6, (<, <=, ==, !=, >=, >))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_TRI_COMPARES)-1)
#define BOOST_PP_FILENAME_1 "data/op/compare_tri.hh"
#include BOOST_PP_ITERATE()

}
