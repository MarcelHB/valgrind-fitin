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

#ifdef FITIN_WITH_LUA
#include <lua.h>
#endif

/* In order to monitor only a subset of the called functions we need filter
   the correct instructions. This can be done in various ways. I.e. the
   functions may be filtered by function name.
 */
typedef enum {
    //Filter single function
    MT_FILTFUNC,
    //Filter by given include path
    MT_FILTINCL
    //FIXME: Not used yet
    //Filter a given function and all functions called from it
    //MT_FILTFUNCABOVE
} filterType;

/* Macro that is needed for determining allocation sizes of shadow fields. */
#define GUEST_STATE_SIZE sizeof(VexGuestArchState)

// This is a data structure to store tool specific data.
typedef struct _toolData {
    // Counter for memory loads.
    ULong loads;
    // A filtertype for the monitorable instructions.
    filterType filter;
    // If filtertype is MT_FILTFUNC this is the function name to be filtered by.
    HChar *filtstr;
    // Executed instructions counter
    ULong instCnt;
    // Instruction limit
    ULong instLmt;
    // The count when the memory will be modified. TODO: check the upper bound of the golden run?
    ULong modMemLoadTime;
    // Bit which will be modified.
    UChar modBit;
    // Faults injected
    UChar injections;
    // States if this is the golden run. No memory modifies should be made during the golden run.
    Bool goldenRun;
    // Monitorables
    XArray *monitorables;
    // Counter for overall monitored loads
    ULong monLoadCnt;
    /* Option that enables persisting of flips by writing them to memory. */
    Bool write_back_flip;
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
#ifdef FITIN_WITH_LUA
		/* Path to a lua file to load. */
		HChar *lua_script;
		/* Lua state. */
		lua_State *lua;
		/* Available callbacks. */
		/* 0..x: START | END | NEXT | SB? | ADDRESS? | FLIP? */
		ULong available_callbacks;
#endif
} toolData;

typedef struct QueuedLoad {
    IRTemp t;
    UInt i;
} QueuedLoad;

#endif /* __FITIN_H */
