#ifndef __FI_REG_H_
#define __FI_REG_H_

#include "valgrind.h"

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"

#include "fitin.h"

#if __x86_64__
#define SIZE_SUFFIX(n) n ## 64
#else
#define SIZE_SUFFIX(n) n ## 32
#endif

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
                            IRExpr *expr);

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

Bool fi_reg_instrument_store(toolData *tool_data,
                             XArray *loads,
                             XArray *replacements,
                             IRExpr **expr,
                             IRExpr *address,
                             IRSB *sb);

#endif
