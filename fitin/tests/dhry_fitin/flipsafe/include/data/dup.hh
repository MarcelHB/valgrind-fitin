// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#pragma once
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include "assert.hh"
#include "protected_clone.hh"
#include <boost/operators.hpp>
#include <boost/preprocessor.hpp>

namespace sihft {

template <typename T>
class dup
  : boost::euclidean_ring_operators< dup<T> >
  , boost::euclidean_ring_operators< dup<T>, T >
  , boost::bitwise< dup<T> >
  , boost::bitwise< dup<T>, T >
  , boost::shiftable< dup<T> >
  , boost::shiftable< dup<T>, T >
  , boost::unit_steppable< dup<T> >
{
public:
  T original, backup;

  inline dup() { }
  inline dup(const T & x)
    : original(x), backup(protected_clone(x))
  {
  }

  inline ~dup() {
    assert_valid();
  }

  inline void assert_valid() const {
    assert( original == backup );
  }

  inline operator T() const {
    assert_valid();
    return original;
  }

  inline dup& operator=(const T & x)
  {
    original = x;
    backup = protected_clone(x); 
    return *this;
  }

  inline dup& operator--()
  {
    return *this -= static_cast<T>(1);
  }

  inline dup& operator++()
  {
    return *this += static_cast<T>(1);
  }
};

#define SIHFT_DUP_UNOPS (4, (!, ~, +, -))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_DUP_UNOPS)-1)
#define BOOST_PP_FILENAME_1 "data/op/unary_dup.hh"
#include BOOST_PP_ITERATE()


#define SIHFT_DUP_COMPOPS (10, (+=, -=, *=, /=, %=, |=, &=, ^=, <<=, >>=))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_DUP_COMPOPS)-1)
#define BOOST_PP_FILENAME_1 "data/op/compound_dup.hh"
#include BOOST_PP_ITERATE()



#define SIHFT_DUP_COMPARES  (6, (<, <=, ==, !=, >=, >))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(SIHFT_DUP_COMPARES)-1)
#define BOOST_PP_FILENAME_1 "data/op/compare_dup.hh"
#include BOOST_PP_ITERATE()


}
