// Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
#pragma once
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#include "handler.hh"
#include "protected_clone.hh"

namespace sihft
{
/**
 * Implementation of local control flow checking using signature checks.
 * Uses RAII to attach a block id (signature) to a code sequence. The
 * optional second parameter to the construct specifies the valid id of
 * the predecessor block. The code checks before entering the scope that
 * the global block id matches the desired predecessor (if one is specifed).
 * Once the specified scope has ended, the code checks that the global
 * block id matches the current scope, and then resets the global id back to
 * the predecessor. This allows for simple syntax-based implementation of
 * these checks, rather than having to perform control flow analysis and
 * figure out all possible predecessor blocks of a certain block.
 *
 * If a block block mismatch is detected, execution is terminated through
 * the fault_detected() function.
 */
struct cflow_check
{
  static int reference_block;
  int block, wrapping_block;

  cflow_check(int block, int wrapping_block)
    : block(block), wrapping_block(wrapping_block)
  {
    if (unlikely(reference_block != wrapping_block))
      fault_detected();
    reference_block = protected_clone(block);
  }

  cflow_check(int block)
    : block(block), wrapping_block(reference_block)
  {
    reference_block = protected_clone(block);
  }

  ~cflow_check() {
    if (unlikely(reference_block != block))
      fault_detected();
    reference_block = protected_clone(wrapping_block);
  }
};

}
