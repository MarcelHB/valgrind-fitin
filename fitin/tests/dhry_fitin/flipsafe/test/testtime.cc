// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include <setjmp.h>
#include "time_redundancy.hh"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestTimeRedundancy
#include <boost/test/unit_test.hpp>

// The main processing
int stateless(int a, int b)
{
  return a + b;
}

int stateful(int a, int b)
{
  static int c = 0;
  return c += a + b;
}

jmp_buf retbuf;
void throw_on_detected()
{
  longjmp(retbuf, 1);
}

// The main application, where redundancy is plugged in using higher-order function
BOOST_AUTO_TEST_CASE(timered)
{
  BOOST_CHECK_EQUAL(stateless(1,2), sihft::time_redundancy(stateless, 1, 2));

  sihft::set_fault_detected(throw_on_detected);
  // This will obviously not be true for any stateful function, as the results
  // might be the same, given that the error_collector just falls through.
  if (!setjmp(retbuf)) 
  {
    sihft::time_redundancy(stateful, 1, 2);
    BOOST_ERROR( "Error not detected" );
  }
}
