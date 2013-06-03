// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include "bitcount.hh"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestData
#include <boost/test/unit_test.hpp>
#include <stdlib.h>
#include <time.h>

BOOST_AUTO_TEST_CASE(bitcount_test) {
  srand(time(NULL));
  for (int i = 0; i < 100; i++)
  {
    unsigned int x = rand();
    BOOST_CHECK_EQUAL(bitcount(x), __builtin_popcount(x));
  }
}


