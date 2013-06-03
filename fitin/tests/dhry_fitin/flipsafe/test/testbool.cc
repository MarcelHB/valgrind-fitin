// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include "data/epbool.hh"
#include "data/bcbool.hh"
#include "data/shbool.hh"
#include "data/rmbool.hh"
#include "data/csbool.hh"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestData
#include <boost/test/unit_test.hpp>

template <typename T>
void check() {
  static_assert( sizeof(epbool<8,T>::l) >= sizeof(T), "Size lsb" );
  static_assert( sizeof(epbool<8,T>::m) >= sizeof(T), "Size msb" );
  static_assert( epbool<8,T>::l == (T)0x0101010101010101ull, "Lsb" );
  static_assert( epbool<8,T>::m == (T)0x8080808080808080ull, "Msb" );
}

BOOST_AUTO_TEST_CASE(epbool_test) {
  check<unsigned char>();
  check<unsigned short>();
  check<unsigned int>();
  check<unsigned long>();
  check<unsigned long long>();
}

BOOST_AUTO_TEST_CASE(other_bool) {
  bcbool<3, unsigned int> a;
  bcbool<1, unsigned int> b;
  shbool<16, unsigned int> c;
  rmbool<1, unsigned int> d;
  csbool<unsigned char> e;
}


