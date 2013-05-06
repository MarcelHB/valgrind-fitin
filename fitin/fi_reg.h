/*--------------------------------------------------------------------*/
/*--- FITIn: The fault injection tool                     fi_reg.h ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of FITIn, a small fault injection tool.

   Copyright (C) 2012 Clemens Terasa clemens.terasa@tu-harburg.de
                      Marcel Heing-Becker <marcel.heing@tu-harburg.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
 */

#ifndef __FI_REG_H_
#define __FI_REG_H_

#include "valgrind.h"

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"

#include "fitin.h"

#define LOAD_STATE_INVALID_INDEX -1

/* LoadState is used at execution time of the instrumented code, it
   contains all the data necessary to do relevancy tests, and size-
   aware flipping. */
typedef struct {
    Bool relevant;
    Addr location;
    SizeT size;
    SizeT full_size;
} LoadState;

/* This struct is used at instrumentation time to keep track of
   load sizes, origin temps and and load state temps. */
typedef struct {
    IRTemp dest_temp;
    IRType ty;
    IRExpr *addr; 
    IRTemp state_list_index;
} LoadData;

/* Just a data tuple of (old temp, new temp) used to replace original
   temps with potentially flipped ones. */
typedef struct {
    IRTemp old_temp;
    IRTemp new_temp;
} ReplaceData;

/* Helper function to properly add a LoadData into the appropriate list. */
void fi_reg_add_temp_load(XArray *list, LoadData* data);

/* On GET, this method will test whether `expr` maps to a relevent occupancy,
   and in this case, it adds a copy of that original temp to the loads for
   `new_temp`. */
void fi_reg_add_load_on_get(toolData *tool_data,
                            XArray *loads,
                            IRTemp new_temp,
                            IRExpr *expr);

/* Method to be called on IRDirty to check the need for reg-read helpers and
   to insert helpers if appllicable. */
void fi_reg_add_pre_dirty_modifiers(toolData *tool_data,
                                    IRDirty *di,
                                    IRSB *sb);

/* Whenever a PUT to an `offset` occurs, this method will analyze the
   expression `expr` for its relevancy and add the runtime helpers. */
void fi_reg_set_occupancy(toolData *tool_data,
                          XArray *loads,
                          Int offset,
                          IRExpr *expr,
                          IRSB *sb);

/* Function to be used by XArray to sort loads by destination IRTemp. */
Int fi_reg_compare_loads(void *l1, void *l2);

/* Function to be used by XArray to sort replacements by replacable IRTemp. */
Int fi_reg_compare_replacements(void *l1, void *l2);

/* To be called before reading from a register if there is no state list to use.
   Instead, data from the register shadow fields is used. 

   The caller must know the correct `offset` and do duplication checks in
   advance (i.e. not calling 4 times this method on the same 32bit register). */
UWord fi_reg_flip_or_leave_no_state_list(toolData *tool_data, 
                                         UWord data,
                                         Int offset);

/* This method must be used if we know that data is definitely read from 
   memory (syscall). It takes the address `a` and the size `size` to check
   for proper modBit. For obvious reasons, persist-flip option is irrelevant. */
void fi_reg_flip_or_leave_mem(toolData *toolData, Addr a, SizeT size);

/* Recursive method that iterates into `expr` to find all sub-expressions and
   to instrument all accesses. `replace_only` will skip instrumentation. */
void fi_reg_instrument_access(toolData *tool_data,
                              XArray *loads,
                              XArray *replacements,
                              IRExpr **expr,
                              IRSB *sb,
                              Bool replace_only);

/* Method to be called on a Store instruction. Needs all the lists, `expr`
   as the pointer to the expression to store, `address` as destination.
   The return value indicates whether the expr was a RdTmp. */
Bool fi_reg_instrument_store(toolData *tool_data,
                             XArray *loads,
                             XArray *replacements,
                             IRExpr **expr,
                             IRExpr *address,
                             IRSB *sb);

#endif
