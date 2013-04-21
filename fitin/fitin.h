#ifndef __FITIN_H
#define __FITIN_H

#include "valgrind.h"
#include "pub_tool_basics.h"
#include "pub_tool_xarray.h"

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

#if __x86_64__
#define GENERAL_PURPOSE_REGISTERS 16
#else
#define GENERAL_PURPOSE_REGISTERS 8
#endif

typedef struct {
    IRTemp temp;
} OccupancyData;

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
    // FITIn-reg: option to allow writing back a flipped value
    Bool write_back_flip;
    // FITIn-reg: runtime list allowing lookup of original address
    XArray *load_states;
    // FITIn-reg: occupancy list
    OccupancyData occupancies[GENERAL_PURPOSE_REGISTERS];
} toolData;

/*
   Monitor the given variable for FITIn handling

   var: Variable to be monitored
 */
#define FITIN_MONITOR_VARIABLE(var)                                   \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__MON_VAR,                 \
                                  (&(var)), sizeof(var), 0, 0, 0)

/*
   Monitor the array content

   array: array address of the array to be monitored
   size: number of elements in the array
   */
#define FITIN_MONITOR_ARRAY(array, size)                              \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__MON_ARR,                 \
                                  (array), sizeof(*array), (size), 0, 0)

/*
   Monitor the given memory area

   mem: start address of the memory area to be monitored
   size: size of the memory area to be monitored
 */
#define FITIN_MONITOR_MEMORY(mem, size)                               \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__MON_MEM,                 \
                                  (mem), (size), 0, 0, 0)

/*
   Unregister the given variable for FITIn handling

   var: Variable to be monitored
 */
#define FITIN_UNMONITOR_VARIABLE(var)                                   \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__UMON_VAR,                 \
                                  (&(var)), sizeof(var), 0, 0, 0)

/*
   Unregister the monitoring of the array content

   array: array address of the array to be monitored
   size: number of elements in the array
   */
#define FITIN_UNMONITOR_ARRAY(array, size)                              \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__UMON_ARR,                 \
                                  (array), sizeof(*array), (size), 0, 0)

/*
   Unregister the monitoring of given memory area

   mem: start adress of the memory area to be monitored
   size: size of the memory area to be monitored
 */
#define FITIN_UNMONITOR_MEMORY(mem, size)                               \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__UMON_MEM,                 \
                                  (mem), (size), 0, 0, 0)

/*
   Inject a bit flip at a given position at a given iteration

   var: variable to be modified
   size: bit to be changed. Bound to variable boundaries
   iteration: bit flip occurs at this iteration of this code
 */
#define FITIN_INJECT_BIT_VARIABLE(var, bit, iteration)                       \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__INJ_B_VAR,                  \
		  (&(var)), sizeof(var), (bit), (iteration), 0)

/*
   Inject a bit flip at a given position at a given iteration

   var: variable to be modified
   size: bit to be changed. Bound to variable boundaries
   iteration: bitflip occurs at this iteration of this code
 */
#define FITIN_INJECT_BIT_MEMORY(mem, size, bit, iteration)                       \
  VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__INJ_B_MEM,                  \
		  (mem), (size), (bit), (iteration), 0)

/*
   Inject a bit flip at a given position at a given condition

   var: variable to be modified
   size: bit to be changed. Bound to variable boundaries
   iteration: bitflip occurs at this iteration of this code
 */
#define FITIN_COND_INJECT_BIT_MEMORY(mem, size, bit, cond)                       \
  if(cond) VALGRIND_DO_CLIENT_REQUEST_STMT(VG_USERREQ__INJ_B_MEM,                  \
		  (mem), (size), (bit), 0, 0)


#endif /* __FITIN_H */
