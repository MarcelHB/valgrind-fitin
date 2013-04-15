#include "fi_reg.h"

#include "pub_tool_debuginfo.h"

static void add_replacement(XArray *list, IRTemp old, IRTemp new);

static void replace_temps(XArray *replacements, IRExpr *expr);

// ----------------------------------------------------------------------------
inline void fi_reg_add_temp_load(XArray *list, IRTemp dest, IRTemp state) {
    LoadData data = (LoadData) { dest, state };
    VG_(addToXA)(list, &data);
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
static UWord VEX_REGPARM(3) fi_reg_flip_or_leave(toolData *tool_data, UWord data, UWord state) {
    VG_(printf)("%u, %u\n", data, state);
    tool_data->loads++;

    if(state != 0 && tool_data->injections == 0) {
        tool_data->monLoadCnt++;

        if(!tool_data->goldenRun &&
            tool_data->modMemLoadTime == tool_data->monLoadCnt) {
            tool_data->injections++;
            data ^= (1 << tool_data->modBit);
        }
    }
    VG_(printf)("DATA: %u\n", data);
    return data;
}

// ----------------------------------------------------------------------------
static inline IRTemp fi_reg_instrument_access_tmp(toolData *tool_data,
                                                  XArray *loads,
                                                  IRTemp tmp,
                                                  IRSB *sb) {
    LoadData key = (LoadData) { tmp, 0 };
    Word first, last;

    if(VG_(lookupXA)(loads, &key, &first, &last)) {
        IRStmt *st;
        IRTemp new_temp = newIRTemp(sb->tyenv, SIZE_SUFFIX(Ity_I));
        LoadData *load_data = (LoadData*)VG_(indexXA)(loads, first);
        IRExpr **args = mkIRExprVec_3(mkIRExpr_HWord(tool_data),
                                      IRExpr_RdTmp(load_data->dest_temp),
                                      IRExpr_RdTmp(load_data->state_temp));
        IRDirty *dirty = unsafeIRDirty_0_N(3, 
                                           "fi_reg_flip_or_leave",
                                           VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave),
                                           args);
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
                            fi_reg_instrument_access_tmp(
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
            while(expr_ptr != NULL) {
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
        ReplaceData *replace_data = (ReplaceData*)VG_(indexXA)(replacements, first);
        expr->Iex.RdTmp.tmp = replace_data->new_temp;
    } 
}
