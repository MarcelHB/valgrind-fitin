// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#include <setjmp.h>
#include <climits>
#include <iostream>
#include "data.hh"
#include "handler.hh"

static int global_counter;

template <typename T>
struct faulty
{
  mutable T value;

  inline faulty() {}
  template <typename X>
  inline faulty(const X & x) : value(x) {}

  inline faulty & operator=(const T & x) { value = x; }

  inline void inject() const
  {
    if (global_counter >= 0)
    {
      const int bitcount = CHAR_BIT * sizeof(T);
      if (global_counter < bitcount)
      {
        ((char *)(T*)&value)[global_counter / CHAR_BIT] ^=
          (1 << (global_counter % CHAR_BIT));
      }
      global_counter -= bitcount;
    }
  }

  operator int () const
  {
    inject();
    return value;
  }

};

#define COMPOPS (10, (+=, -=, *=, /=, %=, |=, &=, ^=, <<=, >>=))
#define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_ARRAY_SIZE(COMPOPS)-1)
#define BOOST_PP_FILENAME_1 "compound_faulty.hh"
#include BOOST_PP_ITERATE()

template <typename T>
inline int under_test(int input)
{
  T x = input;
  if (x <= 2)
    x += 1;
  return x;
}

jmp_buf retbuf;
void throw_on_detected()
{
  longjmp(retbuf, 1);
}

static int impact_free, corrupted, crashed;

template <typename T>
void test_case()
{
  global_counter = -1;
  int input = 1;
  int expected = under_test< faulty<T> >(input);

  sihft::set_fault_detected(throw_on_detected);
  int start = 0;
  impact_free = corrupted = crashed = 0;
  while (true) {
    global_counter = start++;

    if (!setjmp(retbuf))
    {
      int result = under_test< faulty<T> >(input);
      if (global_counter >= 0)
        break;

      if (result == expected)
        impact_free++;
      else
      {
        // std::cout << "Experiment " << start << " expected " << expected << " result " << result << std::endl;
        corrupted++;
      }
    }
    else {
      crashed++;
    }
  }
  while (global_counter < 0);

  std::cout <<
    (start - 1) << ';' <<
    crashed << ';' <<
    impact_free << ';' <<
    corrupted << std::endl;

}

#define TEST_CASE(type) do { std::cout << #type ";"; test_case<type>(); } while (0);

int main()
{
  std::cout << "Type;Injected;Detected;Impactless;Corrupted\n";
  TEST_CASE(int);
  TEST_CASE(float);
  TEST_CASE(sihft::dup<int>);
  TEST_CASE(sihft::dup<float>);
  TEST_CASE(sihft::tri<int>);
  TEST_CASE(sihft::tri<float>);
  TEST_CASE(sihft::trump<int>);
}
