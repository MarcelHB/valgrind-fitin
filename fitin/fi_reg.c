#include "fi_reg.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_xarray.h"
#include "pub_tool_machine.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_debuginfo.h"

static void add_replacement(XArray *list, IRTemp old, IRTemp new);

static void replace_temps(XArray *replacements, IRExpr *expr);

static IRTemp instrument_access_tmp(toolData *tool_data,
                                           XArray *loads,
                                           IRTemp tmp,
                                           IRSB *sb);

// ----------------------------------------------------------------------------
inline void fi_reg_add_temp_load(XArray *list, LoadData *data) {
    VG_(addToXA)(list, data);
    VG_(sortXA)(list);
}

// ----------------------------------------------------------------------------
Int fi_reg_compare_loads(void *l1, void *l2) {
    IRTemp t1 = ((LoadData*)l1)->dest_temp;
    IRTemp t2 = ((LoadData*)l2)->dest_temp;

    return (t1 == t2) ? 0 : ((t1 < t2) ? -1 : 1);
}

// ----------------------------------------------------------------------------
Int fi_reg_compare_replacements(void *r1, void *r2) {
    IRTemp t1 = ((ReplaceData*)r1)->old_temp;
    IRTemp t2 = ((ReplaceData*)r2)->old_temp;

    return (t1 == t2) ? 0 : ((t1 < t2) ? -1 : 1);
}

// ----------------------------------------------------------------------------
static void VEX_REGPARM(3) fi_reg_set_occupancy_origin(toolData *tool_data, 
                                                       Int index,
                                                       Word state_list_index) {
    tool_data->occupancies[index].state_list_index = state_list_index;
}

// ----------------------------------------------------------------------------
static void VEX_REGPARM(2) fi_reg_set_occupancy_origin_invalid(toolData *tool_data, 
                                                               Int index) {
    tool_data->occupancies[index].state_list_index = LOAD_STATE_INVALID_INDEX;
}

// ----------------------------------------------------------------------------
inline void fi_reg_set_occupancy(toolData *tool_data,
                                 XArray *loads,
                                 Int offset,
                                 IRExpr *expr,
                                 IRSB *sb) {
    Int index = OFFSET_TO_INDEX(offset);

    if(index < GENERAL_PURPOSE_REGISTERS) {
        Bool valid_origin = False;
        IRStmt *st;
        IRExpr **args;
        IRDirty *dirty;

        if(expr->tag == Iex_RdTmp) {
            Word first, last;
            LoadData key = (LoadData) { expr->Iex.RdTmp.tmp, NULL, 0 };

            if(VG_(lookupXA)(loads, &key, &first, &last)) {
                LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);

                args = mkIRExprVec_3(mkIRExpr_HWord(tool_data),
                                       mkIRExpr_HWord(index),
                                       IRExpr_RdTmp(load_data->state_list_index));
                dirty = unsafeIRDirty_0_N(3,
                                           "fi_reg_set_occupancy_origin",
                                           VG_(fnptr_to_fnentry)(&fi_reg_set_occupancy_origin),
                                           args);

                st = IRStmt_Dirty(dirty);
                addStmtToIRSB(sb, st);

                tool_data->occupancies[index].temp = expr->Iex.RdTmp.tmp;
                valid_origin = True;
            }
        }

        // We need to do it this way, IRTemp_INVALID cannot be passed.
        if(!valid_origin) {
            tool_data->occupancies[index].temp = IRTemp_INVALID;

            args = mkIRExprVec_2(mkIRExpr_HWord(tool_data),
                                 mkIRExpr_HWord(index));
            dirty = unsafeIRDirty_0_N(2,
                                      "fi_reg_set_occupancy_origin_invalid",
                                      VG_(fnptr_to_fnentry)(&fi_reg_set_occupancy_origin_invalid),
                                      args);
            st = IRStmt_Dirty(dirty);
            addStmtToIRSB(sb, st);
        }
    }
}

// ----------------------------------------------------------------------------
inline void fi_reg_add_load_on_get(toolData *tool_data,
                                   XArray *loads,
                                   IRExpr *expr) {
    if(expr->tag == Iex_Get) {
        Int index = OFFSET_TO_INDEX(expr->Iex.Get.offset);

        if(index < GENERAL_PURPOSE_REGISTERS) {
            IRTemp temp = tool_data->occupancies[index].temp;
            LoadData load_key = (LoadData) { temp, NULL, 0 };
            Word first, last;

            if(temp == IRTemp_INVALID) {
                return;
            }
            
            if(VG_(lookupXA)(loads, &load_key, &first, &last)) {
                LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);
                LoadData new_load_data;

                VG_(memcpy)(&new_load_data, load_data, sizeof(LoadData));
                new_load_data.dest_temp = temp;

                VG_(addToXA)(loads, &new_load_data);
                VG_(sortXA)(loads);
            }
        }
    }
}

// ----------------------------------------------------------------------------
static UWord VEX_REGPARM(3) fi_reg_flip_or_leave_wrap(toolData *tool_data, UWord data, Word state_list_index) {
   return fi_reg_flip_or_leave(tool_data, data, state_list_index);
}

// ----------------------------------------------------------------------------
inline UWord fi_reg_flip_or_leave(toolData *tool_data, UWord data, Word state_list_index) {
    tool_data->loads++;

    LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

    if(state->relevant && (tool_data->injections == 0)) {
        tool_data->monLoadCnt++;

        if(!tool_data->goldenRun &&
            tool_data->modMemLoadTime == tool_data->monLoadCnt) {
            tool_data->injections++;
            data ^= (1 << tool_data->modBit);

            if(tool_data->write_back_flip) {
                *((UWord*)state->location) = data;
            }
        }
    }

    return data;
}

// ----------------------------------------------------------------------------
inline void fi_reg_flip_or_leave_mem(toolData *tool_data, Addr a) {
    tool_data->loads++;
    tool_data->monLoadCnt++;

    if(!tool_data->goldenRun &&
        tool_data->modMemLoadTime == tool_data->monLoadCnt) {
        UWord data = *((UWord*) a);
        data ^= (1 << tool_data->modBit);
        *((Word*) a) = data;

        tool_data->injections++;
    }
}

// ----------------------------------------------------------------------------
static inline IRTemp instrument_access_tmp(toolData *tool_data,
                                           XArray *loads,
                                           IRTemp tmp,
                                           IRSB *sb) {

    LoadData key = (LoadData) { tmp, 0, 0 };
    Word first, last;

    if(VG_(lookupXA)(loads, &key, &first, &last)) {
        IRStmt *st;
        IRTemp new_temp = newIRTemp(sb->tyenv, SIZE_SUFFIX(Ity_I));
        LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);
        IRExpr **args = mkIRExprVec_3(mkIRExpr_HWord(tool_data),
                                      IRExpr_RdTmp(load_data->dest_temp),
                                      IRExpr_RdTmp(load_data->state_list_index));
        IRDirty *dirty = unsafeIRDirty_0_N(3,
                                           "fi_reg_flip_or_leave_wrap",
                                           VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave),
                                           args);
        dirty->mAddr = load_data->addr;
        dirty->mSize = sizeof(UWord);
        dirty->mFx = Ifx_Modify;
        dirty->tmp = new_temp;

        st = IRStmt_Dirty(dirty);
        addStmtToIRSB(sb, st);

        return new_temp;
    }

    return IRTemp_INVALID;
}

#define INSTRUMENT_NESTED_ACCESS(expr) fi_reg_instrument_access(tool_data,\
                                                                loads, \
                                                                replacements, \
                                                                (expr), \
                                                                sb);

// ----------------------------------------------------------------------------
inline void  fi_reg_instrument_access(toolData *tool_data,
                                      XArray *loads,
                                      XArray *replacements,
                                      IRExpr *expr,
                                      IRSB *sb) {
    switch(expr->tag) {
        case Iex_GetI:
            INSTRUMENT_NESTED_ACCESS(expr->Iex.GetI.ix);
            break;
        case Iex_RdTmp:
            add_replacement(replacements,
                            expr->Iex.RdTmp.tmp,
                            instrument_access_tmp(
                                tool_data,
                                loads,
                                expr->Iex.RdTmp.tmp,
                                sb));
            replace_temps(replacements, expr);
            break;
        case Iex_Qop:
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Qop.details->arg1);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Qop.details->arg2);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Qop.details->arg3);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Qop.details->arg4);
            break;
        case Iex_Triop:
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Triop.details->arg1);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Triop.details->arg2);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Triop.details->arg3);
            break;
        case Iex_Binop:
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Binop.arg1);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Binop.arg2);
            break;
        case Iex_Unop:
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Unop.arg);
            break;
        case Iex_Mux0X:
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Mux0X.cond);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Mux0X.expr0);
            INSTRUMENT_NESTED_ACCESS(expr->Iex.Mux0X.exprX);
            break;
        case Iex_CCall: {
            IRExpr **expr_ptr = expr->Iex.CCall.args;
            while(*expr_ptr != NULL) {
                INSTRUMENT_NESTED_ACCESS(*expr_ptr);
                expr_ptr++;
            }
            break;
        default:
            break;
        }
    }
}

// ----------------------------------------------------------------------------
static inline void add_replacement(XArray *list, IRTemp old_temp, IRTemp new_temp) {
    if(new_temp != IRTemp_INVALID) {
        ReplaceData data = (ReplaceData) { old_temp, new_temp };
        VG_(addToXA)(list, &data);
        VG_(sortXA)(list);
    }
}

// ----------------------------------------------------------------------------
static inline void replace_temps(XArray *replacements, IRExpr *expr) {
    ReplaceData key = (ReplaceData) { expr->Iex.RdTmp.tmp, 0 };
    Word first, last;

    if(VG_(lookupXA)(replacements, &key, &first, &last)) {
        ReplaceData *replace_data = (ReplaceData*) VG_(indexXA)(replacements, first);
        expr->Iex.RdTmp.tmp = replace_data->new_temp;
    } 
}
