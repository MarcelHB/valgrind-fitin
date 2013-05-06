#ifndef __FITIN_CLIENT_H
#define __FITIN_CLIENT_H

#include "valgrind.h"
		
typedef	enum {
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

#endif
