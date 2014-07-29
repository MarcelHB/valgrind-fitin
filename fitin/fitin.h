/*--------------------------------------------------------------------*/
/*--- FITIn: The fault injection tool                      fitin.h ---*/
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

#ifndef __FITIN_H
#define __FITIN_H

#include "valgrind.h"
#include "pub_tool_basics.h"
#include "pub_tool_xarray.h"

#include "fi_client.h"

#include <lua.h>

/* Macro that is needed for determining allocation sizes of shadow fields. */
#define GUEST_STATE_SIZE sizeof(VexGuestArchState)

#define CALLBACK_START    1
#define CALLBACK_END      2
#define CALLBACK_NEXT_SB  4
#define CALLBACK_TREAT_SB 8
#define CALLBACK_ADDRESS  16
#define CALLBACK_FLIP     32
#define CALLBACK_FIELD    64
#define CALLBACK_BP       128

// This is a data structure to store tool specific data.
typedef struct ToolData {
    // Counter for memory loads.
    ULong loads;
    // Executed instructions counter
    ULong instCnt;
    // Faults injected
    UChar injections;
    // Monitorables
    XArray *monitorables;
    // Counter for overall monitored loads
    ULong monLoadCnt;
    /* Execution-time list of load information. */
    XArray *load_states;
    /* Flag indicating whether shadow registers have been allocated. */
    Bool register_lists_loaded;
    /* Instrumentation shadow field: register -> temp. */
    IRTemp *reg_temp_occupancies;
    /* Execution shadow field: register -> original address. */
    Addr *reg_origins;
    /* Execution shadow field: register -> (load size, original size) */
    SizeT *reg_load_sizes;
    /* Guest word size. */
    IRType gWordTy;
    /* Special option ignoring monitorables. */
    Bool ignore_monitorables;
    /* Continue executing runtime functions? */
    Bool runtime_active;
    /* Path to a lua file to load. */
    HChar *lua_script;
    /* Lua state. */
    lua_State *lua;
    /* Available callbacks. */
    /* 0..x: START | END | NEXT | SB? | ADDRESS? | FLIP? | FIELD */
    ULong available_callbacks;
    /* Has read debug symbols? */
    Bool with_debug_symbols;
} ToolData;

typedef struct QueuedLoad {
    IRTemp t;
    UInt i;
} QueuedLoad;

#endif
