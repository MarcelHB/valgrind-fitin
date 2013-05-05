#ifndef __FI_REG_H_
#define __FI_REG_H_

#include "valgrind.h"

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"

#include "fitin.h"

#define LOAD_STATE_INVALID_INDEX -1

typedef struct {
    Bool relevant;
    Addr location;
    SizeT size;
    SizeT full_size;
} LoadState;

typedef struct {
    IRTemp dest_temp;
    IRType ty;
    IRExpr *addr; 
    IRTemp state_list_index;
} LoadData;

typedef struct {
    IRTemp old_temp;
    IRTemp new_temp;
} ReplaceData;

void fi_reg_add_temp_load(XArray *list, LoadData* data);

void fi_reg_add_load_on_get(toolData *tool_data,
                            XArray *loads,
                            IRTemp new_temp,
                            IRExpr *expr);

/* Method to be called on IRDirty to check the need for reg-read helpers and
   to insert helpers if appllicable. */
void fi_reg_add_pre_dirty_modifiers(toolData *tool_data,
                                    IRDirty *di,
                                    IRSB *sb);

void fi_reg_set_occupancy(toolData *tool_data,
                          XArray *loads,
                          Int offset,
                          IRExpr *expr,
                          IRSB *sb);

Int fi_reg_compare_loads(void *l1, void *l2);

Int fi_reg_compare_replacements(void *l1, void *l2);

/* To be called before reading from a register if there is no state list to use.
   Instead, data from the register shadow fields is used. 

   The caller must know the correct `offset` and do duplication checks in
   advance (i.e. not calling 4 times this method on the same 32bit register). */

UWord fi_reg_flip_or_leave_no_state_list(toolData *tool_data, 
                                         UWord data,
                                         Int offset);

void fi_reg_flip_or_leave_mem(toolData *toolData, Addr a, SizeT size);

void fi_reg_instrument_access(toolData *tool_data,
                              XArray *loads,
                              XArray *replacements,
                              IRExpr **expr,
                              IRSB *sb,
                              Bool replace_only);

/* Method to be called on a Store instruction. Needs all the lists, `expr`
   as the pointer to the expression to store, `address` as destination. */
Bool fi_reg_instrument_store(toolData *tool_data,
                             XArray *loads,
                             XArray *replacements,
                             IRExpr **expr,
                             IRExpr *address,
                             IRSB *sb);

#endif
