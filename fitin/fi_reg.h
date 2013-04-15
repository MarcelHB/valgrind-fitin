#ifndef __FI_REG_H_
#define __FI_REG_H_

#include "valgrind.h"

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_xarray.h"
#include "pub_tool_machine.h"

#include "fitin.h"

#if __x86_64__
#define SIZE_SUFFIX(n) n ## 64
#else
#define SIZE_SUFFIX(n) n ## 32
#endif

typedef struct {
    IRTemp dest_temp;
    IRTemp state_temp;
} LoadData;

typedef struct {
    IRTemp old_temp;
    IRTemp new_temp;
} ReplaceData;

void fi_reg_add_temp_load(XArray *list, IRTemp dest, IRTemp state);

Int fi_reg_compare_loads(void *l1, void *l2);

Int fi_reg_compare_replacements(void *l1, void *l2);

void fi_reg_instrument_access(toolData *tool_data,
                              XArray *loads,
                              XArray *replacements,
                              IRExpr *expr,
                              IRSB *sb);

#endif
