
/*--------------------------------------------------------------------*/
/*--- FITIn: The fault inejction tool                    fi_main.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of FITIn, a small fault injection tool.

   Copyright (C) 2012 Clemens Terasa clemens.terasa@tu-harburg.de

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

static const unsigned int MAX_STR_SIZE = 512;
static enum exitValues{
	EXIT_SUCCESS,
	EXIT_FAIL,
	EXIT_STOPPED,
};

static void fi_fini(Int exitcode);

/* Monitorables are basically memory locations of which the load operations are counted.
   These ma be extendend du to the needs of the tool.
*/
typedef struct _Monitorable{
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

/* Compare two Monitorables. Needed for an ordered list */
static Int cmpMonitorable (void* v1, void* v2) {
	Monitorable m1 = *(Monitorable*)v1;
	Monitorable m2 = *(Monitorable*)v2;
	if (m1.monAddr < m2.monAddr) return -1;
	if (m1.monAddr > m2.monAddr) return 1;
	return 0;
}

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

// This is a data structure to store tool specific data.
typedef struct _toolData {
	// This is the current intruction Address during the 'static' analysis.
	Addr instAddr;
	// States, whether the current instruction is monitored ('static' analysis).
	Bool monitoredInst;
	// Counter for memory loads.
	ULong loads;
	// A filtertype for the monitorable instructions.
	filterType filter;
	// If filtertype is MT_FILTFUNC this is the function name to be filtered by.
	Char *filtstr;
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
} toolData;

static toolData tData;

static void initTData()
{
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

	tData.monitorables = VG_(newXA)(VG_(malloc), "tData.init", VG_(free), sizeof(Monitorable));
	VG_(setCmpFnXA)(tData.monitorables, cmpMonitorable);
	VG_(sortXA)(tData.monitorables);
}

/* Check whether the instruction at 'instAddr' is in the function with the name 
   in 'fnc' */
static inline Bool instInFunc(Addr instAddr, Char *fnc)
{
	Char fnname[MAX_STR_SIZE];
	return (VG_(get_fnname)(instAddr, fnname, sizeof(fnname))
			&& !VG_(strcmp)(fnname, fnc));
}

/* If the stack shrinks the Monitorables should be inbalidated */
static __inline__
void fi_stop_using_mem_stack(const Addr a, const SizeT len)
{
	Word size = 0, i = 0;
	XArray *mons = tData.monitorables;

	size = VG_(sizeXA)(mons);
	for (i = 0; i < size; i++){
		Monitorable *mon = VG_(indexXA)(mons, i);
		tl_assert(mon != NULL);
		
		if(a <= mon->monAddr && mon->monAddr < (a + len))
			mon->monValid = False;
	}
}
/* Checks whether an instruction with the address 'instAddr' comes from the include path 'incl'.
   We need debug information in the executable to do this.
   */
static inline Bool instInInclude(Addr instAddr, char *incl)
{

	Char filename[MAX_STR_SIZE];
	Char dirname[MAX_STR_SIZE];
	Bool diravail;
	UInt linenum;
	Bool retval;

	if(!VG_(get_filename_linenum)(instAddr,
			filename, MAX_STR_SIZE,
			dirname, MAX_STR_SIZE, &diravail,
			&linenum)){
		return False;
	}
	if(!diravail)
		return False;

	//check whether the dirname begins like the content of incl
	//TODO: Is there a better way than comparing strings?
	retval =  !VG_(strncmp)(dirname, incl, VG_(strlen)(incl));
	//if(retval)
	//	VG_(printf)("%s/%s\n", dirname, filename);
	return retval;

}
/* Check whether the instruction is a possible valid target.
 */
static inline Bool monitorInst(Addr instAddr)
{
	if(VG_(get_fnname_kind_from_IP)(instAddr) == Vg_FnNameBelowMain)
		return False;
	switch (tData.filter){
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
static inline void incrInst()
{
	tData.instCnt++;
	if(tData.instLmt && tData.instCnt >= tData.instLmt){
		fi_fini(EXIT_STOPPED);
		//tl_assert(0);
		VG_(exit)(0);
	}

}

/* FITIn uses several command line options.
   Valgrind helps to parse them.
 */
static Bool fi_process_cmd_line_option(Char* arg)
{

	if VG_STR_CLO(arg, "--fnname", tData.filtstr) {tData.filter = MT_FILTFUNC;}
	else if VG_STR_CLO(arg, "--include", tData.filtstr) {tData.filter = MT_FILTINCL;}
	else if VG_INT_CLO(arg, "--mod-load-time", tData.modMemLoadTime) {}
	else if VG_INT_CLO(arg, "--mod-bit", tData.modBit) {}
	else if VG_INT_CLO(arg, "--inst-limit", tData.instLmt) {}
	else if VG_BOOL_CLO(arg, "--golden-run", tData.goldenRun) {}
	else
		return False;

	tl_assert(tData.filtstr);
	tl_assert(tData.filtstr[0]);

	return True;
}

static void fi_print_usage(void)
{  
	VG_(printf)(
			"    --fnname=<name>           monitor instructions in functon <name> \n"
			"                              [main]\n"
			"    --include=<dir>           monitor instructions whci have debug   \n"
			"                              information from this directory        \n"
			"    --mod-load-time=<number>  modify at a given load time [0]        \n"
			"    --mod_bit=<number>        modify the given bit of the target     \n"
			"    --inst_limit=<number>     the maximum numbers of instructions to \n"
			"                              be executed. To prevent endless loops. \n"
			"    --golden-run=[yes|no]     States whether this is the golden run. \n"
			"                              The golden run just monitors, no modify\n"
		   );
}

static void fi_print_debug_usage(void)
{  
	VG_(printf)(
			"    (none)\n"
		   );
}

static void fi_post_clo_init(void)
{
}

/*
   preLoadHelper
   This dirty helper function is inserted before every Load expression.
   It counts the overall Load expressions, the load operations of the monitorables, 
   and might eventually modify the associated memory.
*/
static
void VEX_REGPARM(3) preLoadHelper(toolData *td, Addr dataAddr, SizeT memSize)
{
	tl_assert(td != NULL && memSize > 0);
	//Always increment all overall load operations
	td->loads++;

	Monitorable key;
	Word first, last;
	key.monAddr = dataAddr;
	if(VG_(clo_verbosity) > 1)
		VG_(printf)("Load: 0x%08x; size: 0x%08x; data: 0x%08x\n", dataAddr, memSize, *((unsigned long*) dataAddr));

	tl_assert(td->monitorables != NULL);
	// iterate over monitorables list
	if(VG_(lookupXA)(td->monitorables, &key, &first, &last)){
		Word i;
		Monitorable *mon;
		
		for(i = first; i <= last; i++){
			mon = (Monitorable *)VG_(indexXA)(td->monitorables, i);
			tl_assert(mon != NULL);
			
			if(!mon->monValid)
				continue;
			
			// if the monitorable is valid 
			mon->monLoads++;
			tData.monLoadCnt++;

//			VG_(printf)("TestModded! load: %d;bit: %d, it:%d\n", mon->monLoads, mon->modBit, mon->modIteration);
			if(!td->goldenRun 
//					&& (mon->monLoads == mon->modIteration || tData.modMemLoadTime == tData.loads)
					&&  tData.modMemLoadTime == tData.monLoadCnt
					&& td->injections == 0)
			{
				// Bring the mod bit to the valid bounds
				UChar numOfBits = memSize << 3; // memSize * 8
				UChar realModBit = mon->modBit % numOfBits;
				UChar realModByte = realModBit >> 3; // realModBit / 8
				UChar* modPtr = (UChar*) ( mon->monAddr  + realModByte);
				*modPtr ^= (1 << (realModBit % 8));
				td->injections++;
				if(VG_(clo_verbosity) > 1){
					VG_(printf)("Modded! monAddr:    0x%016lX\n", mon->monAddr);
					VG_(printf)("        monSize:    %d\n", mon->monSize);
					VG_(printf)("        monLoads:   %d\n", mon->monLoads);
					VG_(printf)("        modPtr:     0x%016lX\n", modPtr);
					VG_(printf)("        realModBit: %d\n", realModBit);
				}
			}
		}
	}
}

/* Instrumentation of every LoadExpression.
   The traversion of the statement exprssion tree probably not necessary in this detail
   */
static void instrumentLoadExpr(toolData *td, IRExpr *expr, IRSB *sbOut)
{
	if(td == NULL || expr == NULL || sbOut == NULL){
		VG_(printf)("Couldn't instrument LD: td: 0x%08x, expr: 0x%08x, sbOut: 0x%08x\n", td,  expr, sbOut);
		return;
	}

	tl_assert(td != NULL && expr != NULL && sbOut != NULL);

	if (expr->tag == Iex_Load) {

		IRDirty *di;
		IRExpr** args;
		IRStmt* st;	

		Int memSize = sizeofIRType(expr->Iex.Load.ty);
		args = mkIRExprVec_3(mkIRExpr_HWord(td),
				expr->Iex.Load.addr,
				mkIRExpr_HWord(memSize));
		di = unsafeIRDirty_0_N ( 3, "preLoadHelper", VG_(fnptr_to_fnentry)(&preLoadHelper), args);
		// The dirty call might modify the memory (which is the intention of the whole practice)
		di->mAddr = expr->Iex.Load.addr;
		di->mSize = memSize;
		di->mFx = Ifx_Modify;

		st = IRStmt_Dirty(di);

		addStmtToIRSB(sbOut, st);		
		if(VG_(clo_verbosity) > 1){
			ppIRStmt(st);
			VG_(printf)("\n");
			VG_(printf)("Load instrumented: instAddr: 0x%08x\n", td->instAddr);
		}
	}
}

	static
IRSB* fi_instrument ( VgCallbackClosure* closure,
		IRSB* sbIn,
		VexGuestLayout* layout, 
		VexGuestExtents* vge,
		IRType gWordTy, IRType hWordTy )
{
	IRSB*      sbOut;
	IRStmt*    st;
	IRExpr** argv;
	IRDirty *di;
	int i;
	if (gWordTy != hWordTy) {
		/* We don't currently support this case. */
		VG_(tool_panic)("host/guest word size mismatch");
	}

	/* Set up SB */
	sbOut = deepCopyIRSBExceptStmts(sbIn);

	// Copy verbatim any IR preamble preceding the first IMark
	i = 0;
	while (i < sbIn->stmts_used && sbIn->stmts[i]->tag != Ist_IMark) {
		addStmtToIRSB( sbOut, sbIn->stmts[i] );
		i++;
	}

	for (/*use current i*/; i < sbIn->stmts_used; i++) {
		st = sbIn->stmts[i];

		if (!st || st->tag == Ist_NoOp) continue;

		if(st->tag == Ist_IMark){
			tData.instAddr = st->Ist.IMark.addr;
			tData.monitoredInst = monitorInst(tData.instAddr);
			
			argv = mkIRExprVec_0();
			di = unsafeIRDirty_0_N ( 0, "incrInst", VG_(fnptr_to_fnentry)(&incrInst), argv);
			addStmtToIRSB(sbOut, IRStmt_Dirty(di));

		}

		// Instrument IRExpressions. Look for loads in the hierarchy and instrument them.
		if(tData.monitoredInst){
			switch (st->tag) {
				case Ist_IMark: // already handled before
				case Ist_NoOp:
				case Ist_AbiHint:
				case Ist_MBE:
					break;
				case Ist_Put:
					instrumentLoadExpr(&tData, st->Ist.Put.data, sbOut);	
					break;
				case Ist_PutI:
					instrumentLoadExpr(&tData, st->Ist.PutI.details->ix, sbOut);	
					instrumentLoadExpr(&tData, st->Ist.PutI.details->data, sbOut);
					break;
				case Ist_WrTmp:
					instrumentLoadExpr(&tData, st->Ist.WrTmp.data, sbOut);
					break;
				case Ist_Store: 
					instrumentLoadExpr(&tData, st->Ist.Store.addr, sbOut);
					instrumentLoadExpr(&tData, st->Ist.Store.data, sbOut);
					break;
				case Ist_Dirty:
					//FIXME: handle IRDirty Statements
					break;
				case Ist_CAS:
					instrumentLoadExpr(&tData, st->Ist.CAS.details->addr, sbOut);
					instrumentLoadExpr(&tData, st->Ist.CAS.details->expdLo, sbOut);
					instrumentLoadExpr(&tData, st->Ist.CAS.details->expdHi, sbOut);
					instrumentLoadExpr(&tData, st->Ist.CAS.details->dataLo, sbOut);
					instrumentLoadExpr(&tData, st->Ist.CAS.details->dataHi, sbOut);
					break;
				case Ist_LLSC:
					instrumentLoadExpr(&tData, st->Ist.LLSC.addr, sbOut);
					if(st->Ist.LLSC.storedata != NULL)
						instrumentLoadExpr(&tData, st->Ist.LLSC.storedata, sbOut);
					break;
				case Ist_Exit:
					instrumentLoadExpr(&tData, st->Ist.Exit.guard, sbOut);
					break;
				default:
					tl_assert(0);
			}

			//print monitored instructions
			if(VG_(clo_verbosity) > 1){
				ppIRStmt(st);
				VG_(printf)("\n");
			}
		}
		// Always add the instruction. We don't want to modify the original code.
		addStmtToIRSB( sbOut, st );
	}
	return sbOut;
}

static void fi_fini(Int exitcode)
{
	switch(exitcode){
	case EXIT_FAIL:
		VG_(printf)("Exited with unknown cause!\n");
		break;
	case EXIT_STOPPED:
		VG_(printf)("Exited: Instruction limit reached!\n");
		break;
	case EXIT_SUCCESS:
	default:
		break;
	}

	XArray *mons = tData.monitorables;

	Word arrSize = VG_(sizeXA)(mons);
	Word i = 0;
	ULong monLoadCnt = 0;
	ULong monBytesLoad = 0;
	for (i = 0; i < arrSize; i++){
		Monitorable *mon = (Monitorable *)VG_(indexXA)(mons, i);
		tl_assert(mon != NULL);
		//TODO: New repersentation of valid: valid up to modify
		//if (!mon->monValid) continue;
		
		if(VG_(clo_verbosity) > 1){
			VG_(printf)("Monitorable memory address: 0x%016llX\n", mon->monAddr);
			VG_(printf)("                   size: %d\n", mon->monSize);
			VG_(printf)("                   loads: %d\n", mon->monLoads);
		}
		monLoadCnt += mon->monLoads;
		monBytesLoad += (mon->monSize * mon->monLoads);
	}
	
	VG_(printf)("Totals:\n");
	VG_(printf)("Overall memory loads: %d\n", tData.loads);
	VG_(printf)("Monitored memory loads: %d\n", tData.monLoadCnt);
	VG_(printf)("Monitored memory bytes load: %d\n", monBytesLoad);
	VG_(printf)("Instructions executed: %d\n", tData.instCnt);
}

static
Bool fi_handle_client_request(ThreadId tid, UWord *args, UWord *ret)
{
	if (!VG_IS_TOOL_USERREQ('F','I',args[0])
			&& VG_USERREQ__GDB_MONITOR_COMMAND   != args[0])
		return False;

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

			if(VG_(lookupXA)(tData.monitorables, &mon, &first, &last)){
				Word i;
				Monitorable *moni;
				for(i = first; i <= last; i++){
					moni = (Monitorable *)VG_(indexXA)(tData.monitorables, i);
					tl_assert(moni != NULL);
					if(moni->monValid)
						continue;
				}
				if(!moni->monValid)
					VG_(addToXA)(tData.monitorables, &mon);
			}else {
					VG_(addToXA)(tData.monitorables, &mon);
			}

//			if( !VG_(lookupXA)(tData.monitorables, &mon, NULL, NULL))
//				VG_(addToXA)(tData.monitorables, &mon);

			VG_(sortXA)(tData.monitorables);

			break;
		}
		case VG_USERREQ__UMON_VAR:
		case VG_USERREQ__UMON_MEM: {
			Word size = 0, i = 0;
			XArray *mons = tData.monitorables;

			size = VG_(sizeXA)(mons);
			for (i = 0; i < size; i++){
				Monitorable *mon = VG_(indexXA)(mons, i);
				tl_assert(mon != NULL);
				
				if( args[1] == mon->monAddr && 
					args[2] == mon->monSize &&
					mon->monValid == True)
					mon->monValid = False;
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

static void fi_pre_clo_init(void)
{
	VG_(details_name)            ("FITIn");
	VG_(details_version)         (NULL);
	VG_(details_description)     ("A simple fault injection tool");
	VG_(details_copyright_author)(
			"Copyright (C) 2012, and GNU GPL'd, by Clemens Terasa.");
	VG_(details_bug_reports_to)  ("BLmadman@gmx.de");

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
}

VG_DETERMINE_INTERFACE_VERSION(fi_pre_clo_init)

	/*--------------------------------------------------------------------*/
	/*--- end                                                          ---*/
	/*--------------------------------------------------------------------*/
