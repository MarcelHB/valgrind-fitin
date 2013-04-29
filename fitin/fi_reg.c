#include "fi_reg.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_xarray.h"
#include "pub_tool_machine.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_debuginfo.h"

static void add_replacement(XArray *list, IRTemp old, IRTemp new);

static void add_modifier_for_register(toolData *tool_data, Int index, IRSB *sb);

static void analyze_dirty_and_add_modifiers(toolData *tool_data,
                                            IRDirty *di,
                                            Int nFx,
                                            IRSB *sb);

static void replace_temps(XArray *replacements, IRExpr **expr);

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
    if(tool_data->injections == 0) {
        LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);
        tool_data->occupancies[index].location = state->location;
        tool_data->occupancies[index].relevant = True;
    } else {
        tool_data->occupancies[index].relevant = False;
        tool_data->occupancies[index].location = NULL;
    }
}

// ----------------------------------------------------------------------------
static void VEX_REGPARM(2) fi_reg_set_occupancy_origin_invalid(toolData *tool_data, 
                                                               Int index) {
    tool_data->occupancies[index].relevant = False;
    tool_data->occupancies[index].location = NULL;
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
            LoadData key = (LoadData) { expr->Iex.RdTmp.tmp, Ity_INVALID, NULL, 0 };

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
            LoadData load_key;
            load_key.dest_temp = temp;
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
inline UWord VEX_REGPARM(3) fi_reg_flip_or_leave(toolData *tool_data,
                                                 UWord data,
                                                 Word state_list_index) {
    tool_data->loads++;

    if(tool_data->injections == 0) {
        LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

        if(state->relevant) {
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
    }

    return data;
}

// ----------------------------------------------------------------------------
inline UWord fi_reg_flip_or_leave_no_state_list(toolData *tool_data, 
                                                UWord data,
                                                Addr a) {
    tool_data->loads++;
    tool_data->monLoadCnt++;

    if(!tool_data->goldenRun &&
        tool_data->modMemLoadTime == tool_data->monLoadCnt) {
        tool_data->injections++;

        data ^= (1 << tool_data->modBit);

        if(tool_data->write_back_flip) {
            *((UWord*) a) = data;
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
        LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);
        IRTemp new_temp = newIRTemp(sb->tyenv, SIZE_SUFFIX(Ity_I));
        IRExpr **args = mkIRExprVec_3(mkIRExpr_HWord(tool_data),
                                      IRExpr_RdTmp(load_data->dest_temp),
                                      IRExpr_RdTmp(load_data->state_list_index));
        IRDirty *dirty = unsafeIRDirty_0_N(3,
                                           "fi_reg_flip_or_leave",
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
                                                                &(expr), \
                                                                sb, \
                                                                replace_only);

// ----------------------------------------------------------------------------
inline void  fi_reg_instrument_access(toolData *tool_data,
                                      XArray *loads,
                                      XArray *replacements,
                                      IRExpr **expr,
                                      IRSB *sb,
                                      Bool replace_only) {
    switch((*expr)->tag) {
        case Iex_GetI:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.GetI.ix);
            break;
        case Iex_RdTmp:
            if(!replace_only) {
                add_replacement(replacements,
                                (*expr)->Iex.RdTmp.tmp,
                                instrument_access_tmp(
                                    tool_data,
                                    loads,
                                    (*expr)->Iex.RdTmp.tmp,
                                    sb));
            }
            replace_temps(replacements, expr);
            break;
        case Iex_Qop:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Qop.details->arg1);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Qop.details->arg2);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Qop.details->arg3);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Qop.details->arg4);
            break;
        case Iex_Triop:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Triop.details->arg1);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Triop.details->arg2);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Triop.details->arg3);
            break;
        case Iex_Binop:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Binop.arg1);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Binop.arg2);
            break;
        case Iex_Unop:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Unop.arg);
            break;
        case Iex_Mux0X:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Mux0X.cond);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Mux0X.expr0);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.Mux0X.exprX);
            break;
        case Iex_CCall: {
            IRExpr **expr_ptr = (*expr)->Iex.CCall.args;
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
static inline void replace_temps(XArray *replacements, IRExpr **expr) {
    ReplaceData key = (ReplaceData) { (*expr)->Iex.RdTmp.tmp, 0 };
    Word first, last;

    if(VG_(lookupXA)(replacements, &key, &first, &last)) {
        // unless copied, all uses of an IRTemp share the same IRExpr,
        // so otherwise this would change previous uses as well
        *expr = deepCopyIRExpr(*expr);
        ReplaceData *replace_data = (ReplaceData*) VG_(indexXA)(replacements, first);
        (*expr)->Iex.RdTmp.tmp = replace_data->new_temp;
    } 
}

// ----------------------------------------------------------------------------
inline void fi_reg_add_pre_dirty_modifiers(toolData *tool_data, IRDirty *st, IRSB *sb) {
    if(st->needsBBP) {
        Int i = 0;
        for(; i < st->nFxState; ++i) {
            if(st->fxState[i].fx == Ifx_Read ||
               st->fxState[i].fx == Ifx_Modify) {
                analyze_dirty_and_add_modifiers(tool_data, st, i, sb);
            }
        }
    }
}

// ----------------------------------------------------------------------------
static inline void analyze_dirty_and_add_modifiers(toolData *tool_data,
                                                   IRDirty *st,
                                                   Int nFx,
                                                   IRSB *sb) {
    Bool relevant[GENERAL_PURPOSE_REGISTERS] = { False };

    // just some variables to deal with anonymous type st->fxState[i]
    UShort offset = st->fxState[nFx].offset, 
           size = st->fxState[nFx].offset;
    UChar nRepeats = st->fxState[nFx].nRepeats, 
          repeatLen = st->fxState[nFx].repeatLen;
    Int i = 0;
    
    // Read indices from offsets in range
    for(; i <= nRepeats; ++i) {
        Int start_offset = offset + i * repeatLen; 
        Int j = 0;

        for(; j < size; ++j) {
            Int index = OFFSET_TO_INDEX(start_offset + j);

            if(index < GENERAL_PURPOSE_REGISTERS) {
                relevant[index] = True;
            }
        }
    }

    for(i = 0; i < GENERAL_PURPOSE_REGISTERS; ++i) {
        if(relevant[i]) {
            add_modifier_for_register(tool_data, i, sb);
        }
    }
}

// ----------------------------------------------------------------------------
static void VEX_REGPARM(2) fi_reg_flip_or_leave_no_state_list_wrap(void *bp,
                                                                   toolData *tool_data,
                                                                   Int index) {
    if(tool_data->occupancies[index].relevant) {
        Int offset = INDEX_TO_OFFSET(index);
        UWord data = *(UWord*)(((UChar*) bp) + offset);

        data = fi_reg_flip_or_leave_no_state_list(tool_data,
                                                  data,
                                                  tool_data->occupancies[index].location);
        *(UWord*)(((UChar*) bp) + offset) = data;
    }
}

// ----------------------------------------------------------------------------
static inline void add_modifier_for_register(toolData *tool_data, Int index, IRSB *sb) {
    IRStmt *st;
    IRExpr **args = mkIRExprVec_2(mkIRExpr_HWord(tool_data),
                                  mkIRExpr_HWord(index));

    IRDirty *di = unsafeIRDirty_0_N(2,
                                   "fi_reg_flip_or_leave_no_state_list_wrap",
                                    VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_no_state_list_wrap),
                                    args);
    di->nFxState = 1;
    di->fxState[0].fx = Ifx_Modify;
    di->fxState[0].offset = INDEX_TO_OFFSET(index);
    di->fxState[0].size = SIZE_SUFFIX() / 8;
    di->fxState[0].nRepeats = 0;
    di->fxState[0].repeatLen = 0;
    di->needsBBP = True;

    st = IRStmt_Dirty(di);
    addStmtToIRSB(sb, st);
}
