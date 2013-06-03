// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include "handler.hh"
#include "cflow.hh"
#include <setjmp.h>
#include <vector>


#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestCflow
#include <boost/test/unit_test.hpp>

#define MAX_JUMPS 1024

std::vector<bool> jmpset(MAX_JUMPS, false);
std::vector< std::vector<bool> > jmpdetected(MAX_JUMPS, std::vector<bool>(MAX_JUMPS, false));
jmp_buf jmpbuf[MAX_JUMPS];
unsigned tests = 0;
unsigned counter = 0;
jmp_buf retbuf;

// Unfortunately, we must do this as a macro, to ensure inlining.
// If we aren't inlined, then when we return, the return address
// will be read from either stack or registers that might not have
// been saved by setjmp and thus we cannot detect any errors.
#define single_jump(Id) \
{ \
  /* Use counter to determine which jumps to take, and */ \
  /* use test to avoid repeating the same test over and over. */ \
  if (counter/MAX_JUMPS == Id && tests++ == 0) \
  { \
    assert(jmpset[counter % MAX_JUMPS]); \
    longjmp(jmpbuf[counter % MAX_JUMPS], 1); \
  } \
  \
  /* Loop so that we can retry jumping here later, without */ \
  /* rerunning the baseline */ \
  while (setjmp(jmpbuf[Id])) {}; \
  \
  /* Record that we actually reached this jump, to make sure */ \
  /* that we do not crash on unreachable code. */ \
  jmpset[Id] = true; \
}

void detect_return()
{
  longjmp(retbuf, 1);
}

void run_jump_test(void (*under_test)())
{
  std::fill(jmpset.begin(), jmpset.end(), false);
  std::fill(jmpdetected.begin(), jmpdetected.end(), jmpset);
  memset(jmpbuf, 0, sizeof(jmpbuf));

  tests = 1;
  under_test();

  sihft::set_fault_detected(detect_return);

  for (counter = 0; counter < MAX_JUMPS * MAX_JUMPS; counter++)
  {

    if (setjmp(retbuf)) {
      assert(tests != 0);
      // We get here if an error was detected (which is good)
      jmpdetected[counter / MAX_JUMPS][counter % MAX_JUMPS] = true;
      continue;
    }

    // These are not executed, so tests will obviously fail
    if (!jmpset[counter / MAX_JUMPS] || !jmpset[counter % MAX_JUMPS])
      continue;

    // These are self-jumps, no point in testing.
    if (counter / MAX_JUMPS == counter % MAX_JUMPS)
      continue;

    tests = 0;
    sihft::cflow_check::reference_block = -1;
    under_test();
    assert(tests != 0);
  }

}

enum sites {
  before1, start1, in2, end1, after1, in3, count
};

void test_case()
{
  single_jump(before1);
  {
    sihft::cflow_check c1(1, -1);
    single_jump(start1);

  if (true) {
    sihft::cflow_check c2(2, 1);
    single_jump(in2);
  }
  else
  {
    sihft::cflow_check c3(3, 1);
    single_jump(in3);
  }

  single_jump(end1);
  }
  single_jump(after1);
}

BOOST_AUTO_TEST_CASE(cflow_test_case)
{
  run_jump_test(test_case);

  // We should have jumps for all reachables
  for (int i = 0; i < count; i++)
    BOOST_CHECK(jmpset[i] == (i != in3));

  // No jumps to after1 should be detected
  for (int i = 0; i < count; i++)
    BOOST_CHECK(!jmpdetected[i][after1]);

  for (int to = 0; to < after1; to++)
    if (to != before1)
      BOOST_CHECK(jmpdetected[before1][to]);

  BOOST_CHECK(jmpdetected[start1][in2]);
  BOOST_CHECK(jmpdetected[start1][before1]);
  BOOST_CHECK(!jmpdetected[start1][end1]);

  for (int to = 0; to < after1; to++)
    if (to != in2)
      BOOST_CHECK(jmpdetected[in2][to]);
  
  BOOST_CHECK(jmpdetected[end1][in2]);
  BOOST_CHECK(jmpdetected[end1][before1]);
  BOOST_CHECK(!jmpdetected[end1][start1]);
}

