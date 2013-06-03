// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <boost/preprocessor.hpp>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "data.hh"
#include "cflow.hh"

const unsigned long long iterations = 200000000;
static int start;

template <typename F>
static int run_test(int start, F f, const char * label) __attribute__((noinline));
template <typename F>
static int run_test(int start, F f, const char * label) {
  start_timer();
  int r = start;
  for (unsigned long long i = 0; i < iterations; ++i)
    r = f(++start % 4);
  record_result( timer(), label );
  return r;
}

#include "basic/all"

int main()
{
  srand(time(NULL));
  start = rand() % 4;

  all_tests();
}
