/*--------------------------------------------------------------------*/
/*--- FITIn: The fault injection tool                    fi_main.c ---*/
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

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_options.h"
#include "pub_tool_machine.h"
#include "pub_tool_clreq.h"

#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_replacemalloc.h"
#include "pub_tool_xarray.h"
#include "pub_tool_aspacemgr.h"
#include "libvex_guest_amd64.h"

#include "fitin.h"
#include "fi_reg.h"

static const unsigned int MAX_STR_SIZE = 512;
typedef enum exitValues {
    EXIT_SUCCESS,
    EXIT_FAIL,
    EXIT_STOPPED,
} ExitValues;

static void fi_fini(Int exitcode);

/* Monitorables are basically memory locations of which the load operations are counted.
   These ma be extendend du to the needs of the tool.
*/
typedef struct _Monitorable {
    // Monitorable memory address; TODO: Find out how to reset it. (after function return?)
    Addr monAddr;
    // The size of one monitorable element
    UWord monSize;
    // The number of monitorable elements (i.e. array elements)
    UWord monLen;
    // # of loads
    ULong monLoads;
    // # of stores
    ULong monWrites;
    // valid address (may be invalidated due to function return)
    Bool monValid;
    // Bit to pos modified (only valid if modIteration != 0)
    UChar modBit;
    // # of loads until modification. (0 means never, 1 means on first load and so on)
    ULong modIteration;
    // Last write instruction
    ULong monLstWriteIsn;
} Monitorable;

static toolData tData;

/* Compare two Monitorables. Needed for an ordered list */
/* --------------------------------------------------------------------------*/
static Int cmpMonitorable (const void *v1, const void *v2) {
    Monitorable m1 = *(Monitorable *)v1;
    Monitorable m2 = *(Monitorable *)v2;
    if (m1.monAddr < m2.monAddr) {
        return -1;
    }
    if (m1.monAddr > m2.monAddr) {
        return 1;
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
static void initTData(void) {
    tData.instAddr = (Addr)NULL;
    tData.monitoredInst = False;
    tData.loads = 0;
    tData.filter = MT_FILTFUNC;
    tData.filtstr = "main";
    tData.instCnt = 0;
    tData.instLmt = 0;
    tData.modMemLoadTime = 1;
    tData.modBit = 0;
    tData.monLoadCnt = 0;
    tData.goldenRun = False;
    tData.reg_temp_occupancies = NULL;
    tData.reg_origins = NULL;
    tData.reg_load_sizes = NULL;

    tData.monitorables = VG_(newXA)(VG_(malloc), "tData.init", VG_(free), sizeof(Monitorable));
    VG_(setCmpFnXA)(tData.monitorables, cmpMonitorable);
    VG_(sortXA)(tData.monitorables);

    tData.load_states = VG_(newXA)(VG_(malloc),
                                   "tData.loadStates.init",
                                   VG_(free),
                                   sizeof(LoadState));
    tData.register_lists_loaded = False;
}

/* Check whether the instruction at 'instAddr' is in the function with the name
   in 'fnc' */
/* --------------------------------------------------------------------------*/
static inline Bool instInFunc(Addr instAddr, const HChar *fnc) {
    HChar fnname[MAX_STR_SIZE];
    return (VG_(get_fnname)(instAddr, fnname, sizeof(fnname))
            && !VG_(strcmp)(fnname, fnc));
}

/* If the stack shrinks the Monitorables should be inbalidated */
/* --------------------------------------------------------------------------*/
static __inline__
void fi_stop_using_mem_stack(const Addr a, const SizeT len) {
    Word size = 0, i = 0;
    XArray *mons = tData.monitorables;

    size = VG_(sizeXA)(mons);
    for (i = 0; i < size; i++) {
        Monitorable *mon = VG_(indexXA)(mons, i);
        tl_assert(mon != NULL);

        if(a <= mon->monAddr && mon->monAddr < (a + len)) {
            mon->monValid = False;
        }
    }
}

/* Checks whether an instruction with the address 'instAddr' comes from the include path 'incl'.
   We need debug information in the executable to do this.
   */
/* --------------------------------------------------------------------------*/
static inline Bool instInInclude(Addr instAddr, char *incl) {

    HChar filename[MAX_STR_SIZE];
    HChar dirname[MAX_STR_SIZE];
    Bool diravail;
    UInt linenum;
    Bool retval;

    if(!VG_(get_filename_linenum)(instAddr,
                                  filename, MAX_STR_SIZE,
                                  dirname, MAX_STR_SIZE, &diravail,
                                  &linenum)) {
        return False;
    }
    if(!diravail) {
        return False;
    }

    //check whether the dirname begins like the content of incl
    //TODO: Is there a better way than comparing strings?
    retval =  !VG_(strncmp)(dirname, incl, VG_(strlen)(incl));

    return retval;

}

/* Check whether the instruction is a possible valid target.
 */
/* --------------------------------------------------------------------------*/
static inline Bool monitorInst(Addr instAddr) {
    if(VG_(get_fnname_kind_from_IP)(instAddr) == Vg_FnNameBelowMain) {
        return False;
    }
    switch (tData.filter) {
        case MT_FILTFUNC:
            return instInFunc(instAddr, tData.filtstr);
        case MT_FILTINCL:
            return instInInclude(instAddr, tData.filtstr);
        default:
            tl_assert(0);
            break;
    }
}

/* A simple instruction counter. */
/* --------------------------------------------------------------------------*/
static inline void incrInst(void) {
    tData.instCnt++;
    if(tData.instLmt && tData.instCnt >= tData.instLmt) {
        fi_fini(EXIT_STOPPED);
        VG_(exit)(0);
    }

}

/* FITIn uses several command line options.
   Valgrind helps to parse them.
 */
/* --------------------------------------------------------------------------*/
static Bool fi_process_cmd_line_option(const HChar *arg) {

    if VG_STR_CLO(arg, "--fnname", tData.filtstr) {
        tData.filter = MT_FILTFUNC;
    } else if VG_STR_CLO(arg, "--include", tData.filtstr) {
        tData.filter = MT_FILTINCL;
    } else if VG_INT_CLO(arg, "--mod-load-time", tData.modMemLoadTime) {}
    else if VG_INT_CLO(arg, "--mod-bit", tData.modBit) {}
    else if VG_INT_CLO(arg, "--inst-limit", tData.instLmt) {}
    else if VG_BOOL_CLO(arg, "--golden-run", tData.goldenRun) {}
    else if VG_BOOL_CLO(arg, "--persist-flip", tData.write_back_flip) {}
    else if VG_BOOL_CLO(arg, "--all-addresses", tData.ignore_monitorables) {}
    else {
        return False;
    }

    tl_assert(tData.filtstr);
    tl_assert(tData.filtstr[0]);

    return True;
}

/* --------------------------------------------------------------------------*/
static void fi_print_usage(void) {
    VG_(printf)(
        "    --fnname=<name>           monitor instructions in functon <name> \n"
        "                              [main]\n"
        "    --include=<dir>           monitor instructions whci have debug   \n"
        "                              information from this directory        \n"
        "    --mod-load-time=<number>  modify at a given load time [0]        \n"
        "    --mod-bit=<number>        modify the given bit of the target     \n"
        "    --inst-limit=<number>     the maximum numbers of instructions to \n"
        "                              be executed. To prevent endless loops. \n"
        "    --golden-run=[yes|no]     States whether this is the golden run. \n"
        "                              The golden run just monitors, no modify\n"
        "    --persist-flip=[yes|no]   writes flipped data back to its memory \n"
        "                              origin\n"
        "    --all-addresses=[yes|no]  Considers every load address as rele-  \n"
        "                              vant: No need for macros.\n"
    );
}

/* --------------------------------------------------------------------------*/
static void fi_print_debug_usage(void) {
    VG_(printf)(
        "    (none)\n"
    );
}

/* --------------------------------------------------------------------------*/
static void fi_post_clo_init(void) {
}

/**
 *  preLoadHelper
 *  This dirty helper function is inserted before every Load expression.
 *  It counts the overall Load expressions, the load operations of the monitorables,
 *  and might eventually modify the associated memory.
 *
 *  The returned value indicates whether the loaded address is revelant for variable
 *  tracing.
 */
/* --------------------------------------------------------------------------*/
static Word VEX_REGPARM(3) preLoadHelper(toolData *td, 
                                         Addr dataAddr,
                                         IRType ty) {
    LoadState state = (LoadState) { False, dataAddr, 0, 0, NULL, 0 };
    Monitorable key;
    Word first, last, state_list_size = VG_(sizeXA)(td->load_states);
    Int size = sizeofIRType(ty);

    tl_assert(state_list_size != LOAD_STATE_INVALID_INDEX);

    key.monAddr = dataAddr;

    if(VG_(clo_verbosity) > 1) {
        VG_(printf)("[FITIn] Load: %p (size: %lu)\n", (void*) dataAddr, (unsigned long) size);
    }

    state.size = state.original_size = size;

    // iterate over monitorables list
    if(td->ignore_monitorables) {
        state.relevant = True;
        state.full_size = size;
    } else {
        if(VG_(lookupXA)(td->monitorables, &key, &first, &last)) {
            Monitorable *mon = (Monitorable *)VG_(indexXA)(td->monitorables, first);

            if(mon->monValid) {
                state.relevant = True;
                state.full_size = mon->monSize;
            }
        }
    }

    VG_(addToXA)(td->load_states, &state);

    return state_list_size;
}

/** 
 * Instrumentation of every LoadExpression.
 * The traversion of the statement exprssion tree probably not necessary in this detail
 *
 * The returned value is a pair of a load marker and an index to the load state list 
 * where relevancy and address can be found.
 */
/* --------------------------------------------------------------------------*/
static LoadData* instrument_load(toolData *td, IRExpr *expr, IRSB *sbOut) {
    if (expr->tag == Iex_Load) {
        IRDirty *di;
        IRExpr **args;
        IRStmt *st;
        LoadData *load_data = VG_(malloc)("fi.reg.load_data.intermediate", sizeof(LoadData));
  
        args = mkIRExprVec_3(mkIRExpr_HWord((HWord) td),
                             expr->Iex.Load.addr,
                             mkIRExpr_HWord(expr->Iex.Load.ty));
        di = unsafeIRDirty_0_N(3,
                               "preLoadHelper",
                               VG_(fnptr_to_fnentry)(&preLoadHelper),
                               args);
        di->tmp = newIRTemp(sbOut->tyenv, td->gWordTy);

        st = IRStmt_Dirty(di);
        addStmtToIRSB(sbOut, st);

        load_data->ty = expr->Iex.Load.ty;
        load_data->addr = expr->Iex.Load.addr;
        load_data->end = expr->Iex.Load.end;
        load_data->state_list_index = di->tmp;

        return load_data;
    }

    return NULL;
}

/* Allocates shadow register fields, with `size` being the guest state size. */
/* --------------------------------------------------------------------------*/
static inline void initialize_register_lists(Int size) {
    tData.reg_origins = VG_(malloc)("fi.init.reg_origins", sizeof(Addr) * size);
    VG_(memset)(tData.reg_origins, 0, sizeof(Addr) * size);

    /* Initialize with 0xFF is equivalent to IRTemp_INVALID. */
    tData.reg_temp_occupancies =
        VG_(malloc)("fi.init.reg_temp_occupancies", sizeof(IRTemp) * size);
    VG_(memset)(tData.reg_temp_occupancies, 0xFF, sizeof(IRTemp) * size);

    tData.reg_load_sizes =
        VG_(malloc)("fi.init.reg_load_sizes", sizeof(SizeT) * size * 2);
    VG_(memset)(tData.reg_load_sizes, 0, sizeof(SizeT) * size * 2);
}

#define INSTRUMENT_ACCESS(expr) fi_reg_instrument_access(&tData, \
                                                         loads,\
                                                         replacements,\
                                                         &(expr), \
                                                         sbOut, \
                                                         False)

#define JUST_REPLACE_ACCESS(expr) fi_reg_instrument_access(&tData, \
                                                           loads,\
                                                           replacements,\
                                                           &(expr), \
                                                           sbOut, \
                                                           True)

/* --------------------------------------------------------------------------*/
static IRSB *fi_instrument(VgCallbackClosure *closure,
                           IRSB *sbIn,
                           VexGuestLayout *layout,
                           VexGuestExtents *vge,
													 VexArchInfo *archInfo,
                           IRType gWordTy, IRType hWordTy ) {
    IRSB      *sbOut;
    IRStmt    *st;
    IRExpr **argv;
    IRDirty *di;
    int i;
    XArray *loads = NULL, *replacements = NULL;

    /* We don't currently support this case. - Really? */
    if (gWordTy != hWordTy) {
        VG_(tool_panic)("host/guest word size mismatch");
    }
    
    /* Conditional because we need that information only once. */
    if(!tData.register_lists_loaded) {
        initialize_register_lists(layout->total_sizeB);
        tData.register_lists_loaded = True;
        tData.gWordTy = gWordTy;
    }

    loads = VG_(newXA)(VG_(malloc), 
                           "fi.reg.load.list",
                           VG_(free),
                           sizeof(LoadData)); 
    VG_(setCmpFnXA)(loads, fi_reg_compare_loads);
    VG_(sortXA)(loads);

    replacements = VG_(newXA)(VG_(malloc),
                              "fi.reg.replace.list",
                              VG_(free),
                              sizeof(ReplaceData)); 
    VG_(setCmpFnXA)(replacements, fi_reg_compare_replacements);
    VG_(sortXA)(replacements);

    /* Set up SB-out, copy of SB-in. */
    sbOut = deepCopyIRSBExceptStmts(sbIn);
    i = 0;
    while (i < sbIn->stmts_used && sbIn->stmts[i]->tag != Ist_IMark) {
        addStmtToIRSB( sbOut, sbIn->stmts[i] );
        i++;
    }

    for (; i < sbIn->stmts_used; i++) {
        st = sbIn->stmts[i];

        if (!st || st->tag == Ist_NoOp) {
            continue;
        }

        if(st->tag == Ist_IMark) {
            tData.instAddr = st->Ist.IMark.addr;
            tData.monitoredInst = monitorInst(tData.instAddr);

            /* Add the counter to every single IMark, no matter where. */
            argv = mkIRExprVec_0();
            di = unsafeIRDirty_0_N ( 0, "incrInst", VG_(fnptr_to_fnentry)(&incrInst), argv);
            addStmtToIRSB(sbOut, IRStmt_Dirty(di));
        }

        /* Check for correct function. */
        if(tData.monitoredInst) {
            switch (st->tag) {
                case Ist_Put:
                    /* PUTting something should not count as access. */
                    JUST_REPLACE_ACCESS(st->Ist.Put.data);

                    fi_reg_set_occupancy(&tData,
                                         loads,
                                         st->Ist.Put.offset,
                                         st->Ist.Put.data,
                                         sbOut);
                    break;
                case Ist_PutI:
                    /* The impact of those operation to the shadow registers
                       is currently not supported! */
                    INSTRUMENT_ACCESS(st->Ist.PutI.details->ix);
                    INSTRUMENT_ACCESS(st->Ist.PutI.details->data);
                    break;
                case Ist_WrTmp: {
                    /* IMPORTANT: please see fi_reg_add_load_on_resize.*/
                    if(tData.gWordTy == Ity_I64 && 
                            fi_reg_add_load_on_resize(&tData,
                                                      loads,
                                                      replacements,
                                                      &(st->Ist.WrTmp.data),
                                                      st->Ist.WrTmp.tmp,
                                                      sbOut)) {
                        break;
                    }

                    /* This will instrument the assigned data. */
                    INSTRUMENT_ACCESS(st->Ist.WrTmp.data);
                    LoadData *load_data = instrument_load(&tData, st->Ist.WrTmp.data, sbOut);

                    if(load_data != NULL) {
                        load_data->dest_temp = st->Ist.WrTmp.tmp;
                        fi_reg_add_temp_load(loads, load_data);
                        VG_(free)(load_data);
                    } else {
                        fi_reg_add_load_on_get(&tData, 
                                               loads,
                                               st->Ist.WrTmp.tmp,
                                               typeOfIRTemp(sbIn->tyenv, st->Ist.WrTmp.tmp),
                                               st->Ist.WrTmp.data,
                                               sbOut);
                    }
                    break;
                }
                case Ist_Store:
                    INSTRUMENT_ACCESS(st->Ist.Store.addr);
                    if(!fi_reg_instrument_store(&tData,
                                                loads,
                                                replacements,
                                                &(st->Ist.Store.data),
                                                st->Ist.Store.addr,
                                                sbOut)) {
                        /* It was something else than a RdTmp, check it
                           for temps. */
                        INSTRUMENT_ACCESS(st->Ist.Store.data);
                    }
                    break;
                case Ist_Dirty:
                    INSTRUMENT_ACCESS(st->Ist.Dirty.details->guard);
                    IRExpr **arg_ptr = st->Ist.Dirty.details->args;

                    /* Check arguments. */
                    while(*arg_ptr != NULL) {
                        INSTRUMENT_ACCESS(*arg_ptr);
                        arg_ptr++;
                    }

                    /* Check destination address in case of memFx. */
                    IREffect fx = st->Ist.Dirty.details->mFx;
                    if(fx != Ifx_None) {
                        INSTRUMENT_ACCESS(st->Ist.Dirty.details->mAddr);

                        if(fx == Ifx_Read || fx == Ifx_Modify) {
                            fi_reg_add_pre_dirty_modifiers_mem(&tData,
                                                               st->Ist.Dirty.details->mAddr,
                                                               st->Ist.Dirty.details->mSize,
                                                               sbOut);
                        }
                    }

                    /* Check for reads from registers. */
                    fi_reg_add_pre_dirty_modifiers(&tData, st->Ist.Dirty.details, sbOut);
                    break;
                case Ist_CAS:                    
                    INSTRUMENT_ACCESS(st->Ist.CAS.details->addr);
                    INSTRUMENT_ACCESS(st->Ist.CAS.details->expdLo);
                    INSTRUMENT_ACCESS(st->Ist.CAS.details->expdHi);
                    INSTRUMENT_ACCESS(st->Ist.CAS.details->dataLo);
                    INSTRUMENT_ACCESS(st->Ist.CAS.details->dataHi);
                    break;
                case Ist_LLSC:                
                    INSTRUMENT_ACCESS(st->Ist.LLSC.addr);

                    if(st->Ist.LLSC.storedata != NULL) {
                        INSTRUMENT_ACCESS(st->Ist.LLSC.storedata);
                    }
                    break;
                case Ist_Exit:
                    INSTRUMENT_ACCESS(st->Ist.Exit.guard);
                    break;
                default:
                    break;
            }
        }

        addStmtToIRSB(sbOut, st);
    }

    VG_(deleteXA)(replacements);
    VG_(deleteXA)(loads);

    return sbOut;
}

/* --------------------------------------------------------------------------*/
static void fi_fini(Int exitcode) {
    switch(exitcode) {
        case EXIT_FAIL:
            VG_(printf)("[FITIn] Exited with unknown cause!\n");
            break;
        case EXIT_STOPPED:
            VG_(printf)("[FITIn] Exited: Instruction limit reached!\n");
            break;
        case EXIT_SUCCESS:
        default:
            break;
    }

    XArray *mons = tData.monitorables;
    Word arrSize = VG_(sizeXA)(mons);
    Word i = 0;

    for (i = 0; i < arrSize; i++) {
        Monitorable *mon = (Monitorable *)VG_(indexXA)(mons, i);

        if(VG_(clo_verbosity) > 1) {
            VG_(printf)("[FITIn] Monitorable memory address: %p\n", (void*) mon->monAddr);
            VG_(printf)("[FITIn]                    size: %lu\n", (unsigned long) mon->monSize);
        }
    }

    VG_(deleteXA)(tData.monitorables);
    VG_(deleteXA)(tData.load_states);

    if(tData.register_lists_loaded) {
        VG_(free)(tData.reg_origins);
        VG_(free)(tData.reg_temp_occupancies);
        VG_(free)(tData.reg_load_sizes);
    }

    VG_(printf)("[FITIn] Totals (of monitored code blocks)\n");
    VG_(printf)("[FITIn]   Overall variable accesses: %lu\n", (unsigned long) tData.loads);
    VG_(printf)("[FITIn]   Monitored variable accesses: %lu\n", (unsigned long) tData.monLoadCnt);
    VG_(printf)("[FITIn]   Instructions executed: %lu\n", (unsigned long) tData.instCnt);
}

/* --------------------------------------------------------------------------*/
static Bool fi_handle_client_request(ThreadId tid, UWord *args, UWord *ret) {
    if (!VG_IS_TOOL_USERREQ('F', 'I', args[0])
            && VG_USERREQ__GDB_MONITOR_COMMAND != args[0]) {
        return False;
    }

    switch(args[0]) {
        case VG_USERREQ__MON_VAR:
        case VG_USERREQ__MON_MEM: {
            //Initialize and add monitorable to list
            Monitorable mon;
            mon.monAddr = args[1];
            mon.monSize = args[2];
            mon.monLen  = 1; // Length is constant 1 because it's no array
            mon.monLoads = 0;
            mon.monWrites = 0;
            mon.monValid = True;
            mon.modBit = tData.modBit;
            mon.modIteration = tData.modMemLoadTime;
            mon.monLstWriteIsn = 0;

            Word first = 0, last = 0;

            if(VG_(lookupXA)(tData.monitorables, &mon, &first, &last)) {
                Word i = first;
                Monitorable *other_mon = (Monitorable*) VG_(indexXA)(tData.monitorables, i);
                
                /* Let's reactivate them and save some space. */
                other_mon->monValid = True;

                if(args[2] > other_mon->monSize) {
                    other_mon->monSize = args[2];
                }
            } else {
                VG_(addToXA)(tData.monitorables, &mon);
                VG_(sortXA)(tData.monitorables);
            }

            break;
        }
        case VG_USERREQ__UMON_VAR:
        case VG_USERREQ__UMON_MEM: {
            Word size = 0, i = 0;
            XArray *mons = tData.monitorables;

            size = VG_(sizeXA)(mons);
            for (i = 0; i < size; i++) {
                Monitorable *mon = VG_(indexXA)(mons, i);
                tl_assert(mon != NULL);

                if( args[1] == mon->monAddr &&
                        args[2] == mon->monSize &&
                        mon->monValid == True) {
                    mon->monValid = False;
                }
            }
            break;
        }
        case VG_USERREQ__MON_ARR:
        case VG_USERREQ__UMON_ARR:
        case VG_USERREQ__INJ_B_VAR:
        case VG_USERREQ__INJ_B_MEM:
        case VG_USERREQ__CINJ_B_VAR:
        default:
            return False;
    }
    return True;
}

/* The load states can be dropped after leaving client code to keep it
   small. */
/* --------------------------------------------------------------------------*/
static void fi_reg_on_client_code_stop(ThreadId tid, ULong dispatched_blocks) {
    Int size = VG_(sizeXA)(tData.load_states), i = 0;

    for(; i < size; ++i) {
        LoadState *state = VG_(indexXA)(tData.load_states, i);
        if(state->data != NULL) {
            VG_(free)(state->data);
        }
    }

    VG_(deleteXA)(tData.load_states);
    tData.load_states = VG_(newXA)(VG_(malloc),
                                   "fi.reg.loadStates.renew",
                                   VG_(free),
                                   sizeof(LoadState));
}

/* This method will check for every `size` bytes, beginning at `a` whether there
   exists a monitorable. Otherwise, we can't handle bytes that are located away
   from `a`, such as arrays or strings. */
/* --------------------------------------------------------------------------*/
static void fi_reg_on_mem_read(CorePart part, ThreadId tid, const HChar *s,
                               Addr a, SizeT size) {

    if(tData.injections == 0 && part == Vg_CoreSysCall) {
        Word first, last;
        Int mem_offset = 0;

        for(; mem_offset <= size; ++mem_offset) {
            Monitorable key;
            key.monAddr = a + mem_offset;

            if(VG_(lookupXA)(tData.monitorables, &key, &first, &last)) {
                Monitorable *mon = (Monitorable*) VG_(indexXA)(tData.monitorables, first);

                if(mon->monValid) {
                    fi_reg_flip_or_leave_mem(&tData, a, mon->monSize);
                }
            }
        }
    }
}

/* Callback for mem on ascii data. Passes strlen + 1 to fi_reg_on_mem_read. */
/* --------------------------------------------------------------------------*/
static void fi_reg_on_mem_read_str(CorePart part, ThreadId tid, const HChar *s,
                                   Addr a) {
    SizeT strlen = VG_(strlen)((const HChar*) a) + 1;
    fi_reg_on_mem_read(part, tid, s, a, strlen);
}

/* Callback for register syscalls. */
/* --------------------------------------------------------------------------*/
static void fi_reg_on_reg_read(CorePart part, ThreadId tid, const HChar *s,
                               PtrdiffT offset, SizeT size) {
    if(part == Vg_CoreSysCall) {
        UChar *buf = VG_(malloc)("fi.reg.syscall.reg", size);
        VG_(get_shadow_regs_area)(tid, buf, 0, offset, size);
        fi_reg_flip_or_leave_registers(&tData, buf, offset, size);
        VG_(set_shadow_regs_area)(tid, 0, offset, size, buf);
        VG_(free)(buf);
    }
}

/* --------------------------------------------------------------------------*/
static void fi_pre_clo_init(void) {
    VG_(details_name)            ("FITIn");
    VG_(details_version)         (NULL);
    VG_(details_description)     ("A simple fault injection tool");
    VG_(details_copyright_author)(
        "Copyright (C) 2013, and GNU GPL'd, by Clemens Terasa, Marcel Heing-Becker");
    VG_(details_bug_reports_to)  ("https://github.com/MarcelHB/valgrind-fitin/issues");

    VG_(details_avg_translation_sizeB) ( 275 );

    VG_(basic_tool_funcs)        (fi_post_clo_init,
                                  fi_instrument,
                                  fi_fini);
    VG_(needs_command_line_options)(fi_process_cmd_line_option,
                                    fi_print_usage,
                                    fi_print_debug_usage);
    VG_(needs_client_requests)(fi_handle_client_request);
    initTData();
    VG_(track_die_mem_stack)(fi_stop_using_mem_stack);

    VG_(track_stop_client_code)(fi_reg_on_client_code_stop);
    VG_(track_pre_reg_read)(fi_reg_on_reg_read);
    VG_(track_pre_mem_read)(fi_reg_on_mem_read);
    VG_(track_pre_mem_read_asciiz)(fi_reg_on_mem_read_str);
}

VG_DETERMINE_INTERFACE_VERSION(fi_pre_clo_init)

/*--------------------------------------------------------------------*/
/*--- end                                                          ---*/
/*--------------------------------------------------------------------*/
