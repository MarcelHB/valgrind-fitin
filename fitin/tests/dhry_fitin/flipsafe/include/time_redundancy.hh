// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#pragma once
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include <functional>
#include <utility>
#include "handler.hh"
#include "protected_clone.hh"
namespace sihft {

template <typename Result, typename Function, typename... Arguments>
Result prevent_inline(Function f, Arguments... args) __attribute__((noinline));

template <typename Result, typename Function, typename... Arguments>
Result prevent_inline(Function f, Arguments... args)
{
  return f(args...);
}

template <typename Function, typename... Arguments>
inline auto
time_redundancy(Function f, Arguments... args) -> decltype(f(args...))
{
  
  auto result1 = prevent_inline<decltype(f(args...))>(protected_clone(f), args...);
  auto result2 = prevent_inline<decltype(f(args...))>(protected_clone(f), args...);

  if (unlikely(result1 != result2))
    fault_detected();
  return result1;
}

}

