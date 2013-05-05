#ifndef __FITIN_H
#define __FITIN_H

#include "valgrind.h"
#include "pub_tool_basics.h"
#include "pub_tool_xarray.h"

#include "fi_client.h"

typedef
	enum {
		VG_USERREQ__MON_VAR = VG_USERREQ_TOOL_BASE('F','I'),
		VG_USERREQ__MON_ARR,
		VG_USERREQ__MON_MEM,
		VG_USERREQ__UMON_VAR,
		VG_USERREQ__UMON_ARR,
		VG_USERREQ__UMON_MEM,
		VG_USERREQ__INJ_B_VAR,
		VG_USERREQ__INJ_B_MEM,
		VG_USERREQ__CINJ_B_VAR,
		
	} Vg_FITInClientRequest;

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
} toolData;

#endif /* __FITIN_H */
