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

typedef struct {
    IRTemp dest_temp;
    IRExpr *addr; 
    IRTemp state_list_index;
} LoadData;

typedef struct {
    IRTemp old_temp;
    IRTemp new_temp;
} ReplaceData;

typedef struct {
    IRTemp temp;
    Int offset;
    Bool invalid;
} OccupancyData;

void fi_reg_add_temp_load(XArray *list, LoadData* data);

void fi_reg_add_load_on_get(XArray *loads,
                            XArray *occupancies,
                            IRExpr *expr);

void fi_reg_add_occupancy(XArray *occupancies, Int offset, IRExpr *expr);

Int fi_reg_compare_loads(void *l1, void *l2);

Int fi_reg_compare_replacements(void *l1, void *l2);

Int fi_reg_compare_occupancies(void *l1, void *l2);

void fi_reg_instrument_access(toolData *tool_data,
                              XArray *loads,
                              XArray *replacements,
                              IRExpr *expr,
                              IRSB *sb);

#endif
