// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#define SIHFT_IN_TEST
#include "data.hh"
// Temporary place for this to ensure it at least compiles...
// TODO: Implement this more nicely.
namespace sihft {
#include "data/op/duptrump.hh"
}

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestData
#include <boost/test/unit_test.hpp>
#include <boost/preprocessor.hpp>
#include <stdlib.h>
#include <time.h>

template <typename Method>
void data_test()
{
  srand(time(NULL));

  typedef typename Method::template bind<int>::type Protected;
  int x1, x2;
  Protected y1, y2;

#define QUICK_TEST \
  for (int i = 0; i < 30; i++) \
    if (y1 = x1 = rand() % ((1 << (sizeof(int)*8-1))/3), true) \
    if (y2 = x2 = rand() % ((1 << (sizeof(int)*8-1))/3), true) \
    if ((std::cerr << '.'), true)

  std::cerr << "\n==";
  QUICK_TEST {
    BOOST_CHECK_EQUAL(x1, y1);
    BOOST_CHECK_EQUAL(x2, y2);
    BOOST_CHECK_NE(x1, y2);
    BOOST_CHECK_NE(x2, y1);
  }

  std::cerr << "\n++";
  QUICK_TEST {
    BOOST_CHECK_EQUAL(x1, y1++);
    BOOST_CHECK_EQUAL(x1+1, y1);
  }
  std::cerr << "\n--";
  QUICK_TEST {
    BOOST_CHECK_EQUAL(x1, y1--);
    BOOST_CHECK_EQUAL(x1-1, y1);
  }

  std::cerr << "\n+";
  QUICK_TEST
    BOOST_CHECK_EQUAL(2*x1, x1 + y1);
  QUICK_TEST
    BOOST_CHECK_EQUAL(2*x1, y1 + y1);
  QUICK_TEST
    BOOST_CHECK_EQUAL(2*x1, y1 + x1);

#define COMPOUND_TEST(r, d, op) \
  std::cerr << '\n' << #op; \
  QUICK_TEST { \
    BOOST_CHECK_EQUAL(x1 op x2, y1 op x2); \
    BOOST_CHECK_EQUAL(x1, y1); \
  } \
  QUICK_TEST { \
    BOOST_CHECK_EQUAL(x1 op x2, y1 op y2); \
    BOOST_CHECK_EQUAL(x1, y1); \
  } \
  QUICK_TEST { \
    BOOST_CHECK_EQUAL(x1 op x1, y1 op y1); \
    BOOST_CHECK_EQUAL(x1, y1); \
  }

  BOOST_PP_LIST_FOR_EACH(COMPOUND_TEST, _, (+=, (-=, (*=, (/=, (%=, (<<=, (>>=, (|=, (^=, (&=, BOOST_PP_NIL)))))))))))

}

namespace traits
{
  struct dup   { template<typename T> struct bind { typedef sihft::dup<T>   type; }; };
  struct tri   { template<typename T> struct bind { typedef sihft::tri<T>   type; }; };
  struct trump { template<typename T> struct bind { typedef sihft::trump<T> type; }; };
}

#define SIHFT_DATA_TEST(what) \
  BOOST_AUTO_TEST_CASE(data_test_ ## what) { \
    data_test<traits::what>(); \
  }

SIHFT_DATA_TEST(dup);
SIHFT_DATA_TEST(tri);
SIHFT_DATA_TEST(trump);

