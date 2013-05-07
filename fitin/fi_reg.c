/*--------------------------------------------------------------------*/
/*--- FITIn: The fault injection tool                     fi_reg.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of FITIn, a small fault injection tool.

   Copyright (C) 2012 Clemens Terasa clemens.terasa@tu-harburg.de
                      Marcel Heing-Becker <marcel.heing@tu-harburg.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
 */

#include "fi_reg.h"

#include "pub_tool_mallocfree.h"
#include "pub_tool_xarray.h"
#include "pub_tool_machine.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_options.h"

/* ------- Protoypes, please scroll down for more info  --------*/
static void add_replacement(XArray *list, IRTemp old, IRTemp new);

static void add_modifier_for_register(toolData *tool_data,
                                      Int offset,
                                      SizeT size,
                                      IRSB *sb);

static void add_modifier_for_offset(toolData *tool_data,
                                    Int j,
                                    Int initial_offset,
                                    IRSB *sb);

static void analyze_dirty_and_add_modifiers(toolData *tool_data,
                                            IRDirty *di,
                                            Int nFx,
                                            IRSB *sb);

static UChar flip_byte_at_bit(UChar byte, UChar bit);

static UWord flip_or_leave(toolData *tool_data, UWord data, LoadState *state);

static UChar* get_destination_address(Addr a, SizeT size, UChar bit);

static void optional_memory_writing(toolData *tool_data,
                                    UWord data,
                                    SizeT full_size,
                                    Addr location,
                                    Bool data_flipped);

static void replace_temps(XArray *replacements, IRExpr **expr);

static void update_reg_load_sizes(toolData *tool_data,
                                  Int offset,
                                  SizeT left,
                                  SizeT full_size);

static void update_reg_origins(toolData *tool_data, 
                               Int offset,
                               SizeT size,
                               Addr origin_offset);

static IRTemp instrument_access_tmp(toolData *tool_data,
                                    XArray *loads,
                                    IRTemp tmp,
                                    IRType ty,
                                    IRSB *sb);

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_add_temp_load(XArray *list, LoadData *data) {
    VG_(addToXA)(list, data);
    VG_(sortXA)(list);
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
Int fi_reg_compare_loads(void *l1, void *l2) {
    IRTemp t1 = ((LoadData*)l1)->dest_temp;
    IRTemp t2 = ((LoadData*)l2)->dest_temp;

    return (t1 == t2) ? 0 : ((t1 < t2) ? -1 : 1);
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
Int fi_reg_compare_replacements(void *r1, void *r2) {
    IRTemp t1 = ((ReplaceData*)r1)->old_temp;
    IRTemp t2 = ((ReplaceData*)r2)->old_temp;

    return (t1 == t2) ? 0 : ((t1 < t2) ? -1 : 1);
}

/* Copies data from load state list at `state_list_index` into the register
   shadow fields, beginning at `offset` for `size` bytes. This includes the
   original address, the size of the LD instruction and the monitorable size. */ 
/* ---------------------------------------------------------------------------*/
static void fi_reg_set_occupancy_origin(toolData *tool_data, 
                                        Int offset,
                                        SizeT size,
                                        Word state_list_index) {
    /* After an injection, update any (remaining) information as irrelevant. */
    if(tool_data->injections == 0) {
        LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);
        update_reg_origins(tool_data, offset, size, state->location);
        update_reg_load_sizes(tool_data, offset, size, state->full_size);
    } else {
        update_reg_origins(tool_data, offset, size, (Addr) NULL);
        update_reg_load_sizes(tool_data, offset, size, 0);
    }
}

/* Fills the register shadow fields beginning at `offset` for `size` bytes
   with data indicating irrelevant contents. We need this additional dirty
   as we we cannot pass an IRTemp_INVALID as state_list_index for the
   previous one. */
/* --------------------------------------------------------------------------*/
static void VEX_REGPARM(3) fi_reg_set_occupancy_origin_irrelevant(toolData *tool_data, 
                                                                  Int offset,
                                                                  SizeT size) {
    update_reg_origins(tool_data, offset, size, (Addr) NULL);
    update_reg_load_sizes(tool_data, offset, size, 0);
}

/* Function that is doing the actual writing to `tool_data`. Starting at
   `offset`, it writes the origin for any `size` bytes and adjusts the 
   address.*/
/* --------------------------------------------------------------------------*/
static inline void update_reg_origins(toolData *tool_data, 
                                      Int offset,
                                      SizeT size,
                                      Addr origin_offset) {
    Int i = 0;
    for(; i < size; ++i) {
        /* NULL must not be incremented. */
        Addr offset_i = (origin_offset == 0 ? 0 : origin_offset + i);
        tool_data->reg_origins[offset+i] = offset_i;
    }
}

/* Function that is doing the actual writing to `tool_data`. Starting at
   `offset`, it writes for the loaded size and the original monitoring size
   down to the shadows field. Starting at `offset`, it writes down as many
   bytes as `left` is large, 0 meaning 'last byte' of this load.*/
/* --------------------------------------------------------------------------*/
static inline void update_reg_load_sizes(toolData *tool_data,
                                         Int offset,
                                         SizeT left,
                                         SizeT full_size) {
    Int i = offset * 2;
    SizeT load_list_size = left * 2;
    Int until = offset * 2 + load_list_size;

    for(; i < until; i += 2) {
        tool_data->reg_load_sizes[i] = --left; 
        tool_data->reg_load_sizes[i+1] = full_size;
    }
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_set_occupancy(toolData *tool_data,
                                 XArray *loads,
                                 Int offset,
                                 IRExpr *expr,
                                 IRSB *sb) {
    Bool valid_origin = False;
    IRStmt *st;
    IRExpr **args;
    IRDirty *dirty;
    SizeT size = 0;

    /* The only case for a relevant origin is the expression of an IRTemp to
       be PUT. */
    if(expr->tag == Iex_RdTmp) {
        Word first, last;
        LoadData key = (LoadData) { expr->Iex.RdTmp.tmp, Ity_INVALID, NULL, 0 };

        IRType ty = typeOfIRTemp(sb->tyenv, expr->Iex.RdTmp.tmp);
        size = sizeofIRType(ty);

        if(VG_(lookupXA)(loads, &key, &first, &last)) {
            LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);

            args = mkIRExprVec_4(mkIRExpr_HWord((HWord) tool_data),
                                 mkIRExpr_HWord(offset),
                                 mkIRExpr_HWord(size),
                                 IRExpr_RdTmp(load_data->state_list_index));
            dirty = unsafeIRDirty_0_N(0,
                                      "fi_reg_set_occupancy_origin",
                                      VG_(fnptr_to_fnentry)(&fi_reg_set_occupancy_origin),
                                      args);

            st = IRStmt_Dirty(dirty);
            addStmtToIRSB(sb, st);

            tool_data->reg_temp_occupancies[offset] = expr->Iex.RdTmp.tmp;
            valid_origin = True;
        }
    }

    /* If the previous block did not recognize a valid origin, we must now 
       set up the helpers for invalidating the register shadow fields. */
    if(!valid_origin) {
        /* Do not update INVALID by INVALID, saves lots of helpers and time. */
        if(tool_data->reg_temp_occupancies[offset] == IRTemp_INVALID) {
            return; 
        }

        /* Only if there was no Iex.RdTmp.tmp, otherwise we already have a size. */
        if(size == 0) {
            if(expr->tag == Iex_Const) {
                IRType ty = typeOfIRConst(expr->Iex.Const.con);
                size = sizeofIRType(ty);
            } else {
                /* If this happens, we need a more general way to identify the data
                   written into a PUT. */
                tl_assert(0);
            }
        }

        tool_data->reg_temp_occupancies[offset] = IRTemp_INVALID;

        args = mkIRExprVec_3(mkIRExpr_HWord((HWord) tool_data),
                             mkIRExpr_HWord(offset),
                             mkIRExpr_HWord(size));
        dirty = unsafeIRDirty_0_N(3,
                                  "fi_reg_set_occupancy_origin_irrelevant",
                                  VG_(fnptr_to_fnentry)(&fi_reg_set_occupancy_origin_irrelevant),
                                  args);
        st = IRStmt_Dirty(dirty);
        addStmtToIRSB(sb, st);
    }
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_add_load_on_get(toolData *tool_data,
                                   XArray *loads,
                                   IRTemp new_temp,
                                   IRExpr *expr) {
    if(expr->tag == Iex_Get) {
        Int offset = expr->Iex.Get.offset;
        IRTemp temp = tool_data->reg_temp_occupancies[offset];
        LoadData load_key;
        load_key.dest_temp = temp;
        Word first, last;

        /* Does not contain anything interesting for us. */
        if(temp == IRTemp_INVALID) {
            return;
        }

        if(VG_(lookupXA)(loads, &load_key, &first, &last)) {
            /* Load the load data of the original temp, and insert a copy
               that only contains the new IRTemp, created by WrTmp. */
            LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);
            LoadData new_load_data = *load_data;

            new_load_data.dest_temp = new_temp;

            fi_reg_add_temp_load(loads, &new_load_data);
        }
    }
}

/* This dirty call must be called before any RdTmp access to an IRTemp which
   has been set up for tracking by preLoadHelper. It gets the value by `data`
   and the index to use for looking of loading information.
   
   The return value may be flipped or not but must be used for replacing the
   original acccessed IRTemp in either case. */
/* --------------------------------------------------------------------------*/
/* Prevents warning. */
UWord VEX_REGPARM(3) fi_reg_flip_or_leave(toolData *tool_data,
                                          UWord data,
                                          Word state_list_index);

inline UWord VEX_REGPARM(3) fi_reg_flip_or_leave(toolData *tool_data,
                                                 UWord data,
                                                 Word state_list_index) {
    tool_data->loads++;

    if(tool_data->injections == 0 && state_list_index != LOAD_STATE_INVALID_INDEX) {
        LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

        return flip_or_leave(tool_data, data, state);
    }

    return data;
}

/* This dirty call must be used before ST. It will ensure that flipping is only
   done if the original address of the IRTemp and the destination `address` are
   different. */
/* --------------------------------------------------------------------------*/
/* Prevents warning. */
UWord fi_reg_flip_or_leave_before_store(toolData *tool_data,
                                        UWord data,
                                        Addr address,
                                        Word state_list_index);

inline UWord fi_reg_flip_or_leave_before_store(toolData *tool_data,
                                               UWord data,
                                               Addr address,
                                               Word state_list_index) {
    tool_data->loads++;

    if(tool_data->injections == 0) {
        LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

        if(state->location != address) {
            return flip_or_leave(tool_data, data, state);
        }
    }

    return data;
}

/* The method that is performing the bit-flip if applicable. It takes place 
   inside of `data` and will be returned. */
/* --------------------------------------------------------------------------*/
static inline UWord flip_or_leave(toolData *tool_data,
                                  UWord data,
                                  LoadState *state) {
    if(state->relevant) {
        tool_data->monLoadCnt++;

        if(!tool_data->goldenRun &&
            tool_data->modMemLoadTime == tool_data->monLoadCnt) {
            UChar *addr = get_destination_address((Addr) &data, state->size, tool_data->modBit);

            if(addr != NULL) {
                *addr = flip_byte_at_bit(*addr, tool_data->modBit % 8);

                if(VG_(clo_verbosity) > 1) {
                    VG_(printf)("[FITIn] FLIP! Data from %p\n", (void*) state->location);
                }
            }
  
            /* Test for writing back into memory. */
            optional_memory_writing(tool_data, 
                                    data,
                                    state->full_size,
                                    state->location,
                                    addr != NULL);
            tool_data->injections++;
        }
    }

    return data;
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline UWord fi_reg_flip_or_leave_no_state_list(toolData *tool_data, 
                                                UWord data,
                                                Int offset) {
    tool_data->loads++;
    tool_data->monLoadCnt++;

    if(!tool_data->goldenRun &&
        tool_data->modMemLoadTime == tool_data->monLoadCnt) {
        Int full_size_offset = offset * 2 + 1;
        UChar *addr = get_destination_address((Addr) &data,
                                              tool_data->reg_load_sizes[offset*2],
                                              tool_data->modBit);

        if(addr != NULL) {
            *addr = flip_byte_at_bit(*addr, tool_data->modBit % 8);

            if(VG_(clo_verbosity) > 1) {
                VG_(printf)("[FITIn] FLIP! Data from %p\n", (void*) tool_data->reg_origins[offset]);
            }
        }

        /* Test for writing back into memory. */
        optional_memory_writing(tool_data, 
                                data,
                                tool_data->reg_load_sizes[full_size_offset],
                                tool_data->reg_origins[offset],
                                addr != NULL);

        tool_data->injections++;
    }

    return data;
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_flip_or_leave_mem(toolData *tool_data, Addr a, SizeT size) {
    tool_data->loads++;
    tool_data->monLoadCnt++;

    if(!tool_data->goldenRun &&
        tool_data->modMemLoadTime == tool_data->monLoadCnt) {
        UChar *addr = get_destination_address(a, size, tool_data->modBit);

        if(addr != NULL) {
            *addr = flip_byte_at_bit(*addr, tool_data->modBit % 8);

            if(VG_(clo_verbosity) > 1) {
                VG_(printf)("[FITIn] FLIP! Data at %p\n", (void*) a);
            }
        }

        tool_data->injections++;
    }
}

/* Helper to extract a byte-precise address. This can be used to do the actual
   flip. It tests whether `bit` is located inside of `a` + `size`. If not,
   this will return NULL. */
/* --------------------------------------------------------------------------*/
static inline UChar* get_destination_address(Addr a, SizeT size, UChar bit) {
    UChar bit_size = size * 8; 

    if(bit < bit_size) {
        UChar byte = bit / 8;
        return ((UChar*) a) + byte;
    }

    return NULL;
}

/* Flip peforming method, to flip `bit` in the given `byte`. Reduces code
   duplication. */
/* --------------------------------------------------------------------------*/
static inline UChar flip_byte_at_bit(UChar byte, UChar bit) {
    return byte ^ (1 << bit); 
}

/* This method is to be used by the dirty flipper methods if operating on
   non-original locations. Only working if `--persist-flip` is on.

   The method determines the flipped byte in `data` and the according memory
   byte as offset to `location`. If `data_flipped` is false, e.g. by a too
   small partial load, this method attempts to do the flip on `location` and
   `full_size` on memory again. */
/* --------------------------------------------------------------------------*/
static inline void optional_memory_writing(toolData *tool_data,
                                           UWord data,
                                           SizeT full_size,
                                           Addr location,
                                           Bool data_flipped) {
    if(tool_data->write_back_flip) {
        UChar *data_addr = get_destination_address((Addr) &data,
                                                   full_size,
                                                   tool_data->modBit);

        if(data_addr != NULL) {
            UChar *dest_addr = get_destination_address(location,
                                                       full_size,
                                                       tool_data->modBit);

            if(!data_flipped) {
                *data_addr = flip_byte_at_bit(*data_addr, tool_data->modBit % 8);

                if(VG_(clo_verbosity) > 1) {
                    VG_(printf)("[FITIn] FLIP (secondary)! Data at %p\n", (void*) dest_addr);
                }
            }

            *dest_addr = *data_addr;
        }
    }
}

/* This is an important method to be called before inserting any flip-helper.
   As loads may be < UWord, but arguments are required to have UWord-size,
   this method inserts casts to change the size to UWord, not respecting signs. */
/* --------------------------------------------------------------------------*/
static inline IRTemp insert_size_widener(toolData *tool_data, 
                                         IRTemp tmp,
                                         IRType ty,
                                         IRSB *sb) {
    IROp op = Iop_INVALID;
    Bool is64 = tool_data->gWordTy == Ity_I64;

    switch(ty) {
        case Ity_I8:
            op = is64 ? Iop_8Uto64 : Iop_8Uto32;
            break;
        case Ity_I16:
            op = is64 ? Iop_16Uto64 : Iop_16Uto32;
            break;
        case Ity_I32:
            op = is64 ? Iop_32Uto64 : Iop_INVALID;
            break;
        default:
            break;
    }

    if(op != Iop_INVALID) {
        IRTemp new_temp = newIRTemp(sb->tyenv, tool_data->gWordTy);

        IRStmt *st = IRStmt_WrTmp(
                                    new_temp,
                                    IRExpr_Unop(
                                        op,
                                        IRExpr_RdTmp(
                                            tmp
                                        )
                                    )
                                 );

        addStmtToIRSB(sb, st);

        return new_temp;
    }

    return IRTemp_INVALID;
}

/* Whenever an RdTmp occurs, this function needs to be called to manage 
   all necessary things, and inserting the helper. Needs `loads` for
   doing a relevance check for `tmp`. `ty` is the type of `temp`.*/
/* --------------------------------------------------------------------------*/
static inline IRTemp instrument_access_tmp(toolData *tool_data,
                                           XArray *loads,
                                           IRTemp tmp,
                                           IRType ty,
                                           IRSB *sb) {

    LoadData key = (LoadData) { tmp, 0, 0 };
    Word first, last;

    if(VG_(lookupXA)(loads, &key, &first, &last)) {
        IRStmt *st;
        LoadData *load_data;
        IRTemp new_temp, access_temp = tmp;
        IRExpr **args;
        IRDirty *dirty;

        /* Insert widener if temp is too small for being an argument. */
        if(ty < tool_data->gWordTy) {
            access_temp = insert_size_widener(tool_data, tmp, ty, sb);
            
            if(access_temp == IRTemp_INVALID) {
                return IRTemp_INVALID;
            }
        }
            
        new_temp = newIRTemp(sb->tyenv, ty);
        load_data = (LoadData*) VG_(indexXA)(loads, first);
        args = mkIRExprVec_3(mkIRExpr_HWord((HWord) tool_data),
                             IRExpr_RdTmp(access_temp),
                             IRExpr_RdTmp(load_data->state_list_index));
        dirty = unsafeIRDirty_0_N(3,
                                  "fi_reg_flip_or_leave",
                                  VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave),
                                  args);

        /* Skip registering memory writing if not enabled. */
        if(tool_data->write_back_flip) {
            /* Open issue: Can we predict the exact byte here w.r.t to full_size? */
            dirty->mAddr = load_data->addr;
            dirty->mSize = 1;
            dirty->mFx = Ifx_Write;
        }
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

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
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
            /* Stop recursion if arrived on a RdTmp. And replace
               it if there are replacement entries for this one. */
            if(!replace_only) {
                add_replacement(replacements,
                                (*expr)->Iex.RdTmp.tmp,
                                instrument_access_tmp(
                                    tool_data,
                                    loads,
                                    (*expr)->Iex.RdTmp.tmp,
                                    typeOfIRTemp(sb->tyenv, (*expr)->Iex.RdTmp.tmp),
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

/* Basically the same as instrument_access_tmp. But to be used on RdTmp on
   a Store instruction. This one also gets the destination expression
   `address`. */
/* --------------------------------------------------------------------------*/
static inline IRTemp instrument_access_tmp_on_store(toolData *tool_data,
                                                    XArray *loads,
                                                    IRTemp tmp,
                                                    IRExpr *address,
                                                    IRType ty,
                                                    IRSB *sb) {
    LoadData key = (LoadData) { tmp, 0, 0 };
    Word first, last;

    if(VG_(lookupXA)(loads, &key, &first, &last)) {
        IRStmt *st;
        LoadData *load_data;
        IRTemp new_temp, access_temp = tmp;
        IRExpr **args;
        IRDirty *dirty;
                
        /* Insert widener if temp is too small for being an argument. */
        if(ty < tool_data->gWordTy) {
            access_temp = insert_size_widener(tool_data, tmp, ty, sb);
            
            if(access_temp == IRTemp_INVALID) {
                return IRTemp_INVALID;
            }
        }  
                
                
        new_temp = newIRTemp(sb->tyenv, tool_data->gWordTy);
        load_data = (LoadData*) VG_(indexXA)(loads, first);
        args = mkIRExprVec_4(mkIRExpr_HWord((HWord) tool_data),
                             IRExpr_RdTmp(access_temp), 
                             address,
                             IRExpr_RdTmp(load_data->state_list_index));
        dirty = unsafeIRDirty_0_N(0,
                                  "fi_reg_flip_or_leave_before_store",
                                   VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_before_store),
                                   args);

        /* Skip registering memory writing if not enabled. */
        if(tool_data->write_back_flip) {
            /* Open issue: Can we predict the exact byte here w.r.t to full_size? */
            dirty->mAddr = load_data->addr;
            dirty->mSize = 1;
            dirty->mFx = Ifx_Write;
        }
        dirty->tmp = new_temp;

        st = IRStmt_Dirty(dirty);
        addStmtToIRSB(sb, st);

        return new_temp;
    }

    return IRTemp_INVALID;
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline Bool fi_reg_instrument_store(toolData *tool_data,
                                    XArray *loads,
                                    XArray *replacements,
                                    IRExpr **expr,
                                    IRExpr *address,
                                    IRSB *sb) {
    /* RdTmp is of course the only relevant expression. */
    if((*expr)->tag == Iex_RdTmp) {
        add_replacement(replacements,
                        (*expr)->Iex.RdTmp.tmp,
                        instrument_access_tmp_on_store(
                            tool_data,
                            loads,
                            (*expr)->Iex.RdTmp.tmp,
                            address,
                            typeOfIRTemp(sb->tyenv, (*expr)->Iex.RdTmp.tmp),
                            sb));
        replace_temps(replacements, expr);

        return True;
    }

    return False;
}

/* Helper to add a replacement entry into `list`, replacing `old_temp` by 
   `new_temp`. */
/* --------------------------------------------------------------------------*/
static inline void add_replacement(XArray *list, IRTemp old_temp, IRTemp new_temp) {
    if(new_temp != IRTemp_INVALID) {
        ReplaceData data = (ReplaceData) { old_temp, new_temp };
        VG_(addToXA)(list, &data);
        VG_(sortXA)(list);
    }
}

/* Checks a given expression for temps that need to be replaced. */
/* --------------------------------------------------------------------------*/
static inline void replace_temps(XArray *replacements, IRExpr **expr) {
    ReplaceData key = (ReplaceData) { (*expr)->Iex.RdTmp.tmp, 0 };
    Word first, last;

    if(VG_(lookupXA)(replacements, &key, &first, &last)) {
        /* Unless copied, all uses of an IRTemp share the same IRExpr,
           so otherwise this would change previous uses as well. */
        *expr = deepCopyIRExpr(*expr);
        ReplaceData *replace_data = (ReplaceData*) VG_(indexXA)(replacements, first);
        (*expr)->Iex.RdTmp.tmp = replace_data->new_temp;
    } 
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_add_pre_dirty_modifiers(toolData *tool_data, IRDirty *st, IRSB *sb) {
    /* This seems to be the only reliable test. */
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

/* This method checks the signature of dirty-call `st` and fxState `nFx` to
   determine the offsets that are used. */
/* --------------------------------------------------------------------------*/
static inline void analyze_dirty_and_add_modifiers(toolData *tool_data,
                                                   IRDirty *st,
                                                   Int nFx,
                                                   IRSB *sb) {
    /* Just some variables to deal with anonymous type st->fxState[i].*/
    UShort offset = st->fxState[nFx].offset, 
           size = st->fxState[nFx].size;
    UChar nRepeats = st->fxState[nFx].nRepeats, 
          repeatLen = st->fxState[nFx].repeatLen;
    
    if(nRepeats > 0) {
        UShort i = 0;

        for(; i <= nRepeats; ++i) {
            Int start_offset = offset + i * repeatLen; 
            Int j = start_offset, until = start_offset + size;

            for(; j < until; ++j) {
                add_modifier_for_offset(tool_data, j, start_offset, sb); 
            }
        }
    } else {
        Int j = offset, until = offset + size;

        for(; j < until; ++j) {
           add_modifier_for_offset(tool_data, j, offset, sb); 
        }
    }
}

/* For a given offset `j`, originating from offset `initial_offset`,
   this method will look for the occupancies of this offset and prevents
   collisions (multiple calls on the same IRTemp). */
/* --------------------------------------------------------------------------*/
static inline void add_modifier_for_offset(toolData *tool_data,
                                           Int j,
                                           Int initial_offset,
                                           IRSB *sb) {
    IRTemp temp = tool_data->reg_temp_occupancies[j];

    if(temp != IRTemp_INVALID) {
        SizeT size = sizeofIRType(typeOfIRTemp(sb->tyenv, temp));

        if(j > 0 && (j-1) >= initial_offset) {
            if(tool_data->reg_temp_occupancies[j-1] != temp) {
                add_modifier_for_register(tool_data, j, size, sb);
            }
        } else {
            add_modifier_for_register(tool_data, j, size, sb);
        }
    }
}

/* Wrapper for fi_reg_flip_or_leave_no_state_list to be called from VEX IR,
   this will read the data from `offset` at `bp` and write back the flipped
   value. This is called before reg-reading IRDirty calls. */
/* --------------------------------------------------------------------------*/
static void VEX_REGPARM(2) fi_reg_flip_or_leave_no_state_list_wrap(void *bp,
                                                                   toolData *tool_data,
                                                                   Int offset) {
    /* Validity test.*/
    if(tool_data->reg_origins[offset] != 0) {
        UWord data = *(UWord*)(((UChar*) bp) + offset);

        data = fi_reg_flip_or_leave_no_state_list(tool_data,
                                                  data,
                                                  offset);
        *(UWord*)(((UChar*) bp) + offset) = data;
    }
}

/* A method that is configuring and inserting the helper function before an
   IRDirty calls. */
/* --------------------------------------------------------------------------*/
static inline void add_modifier_for_register(toolData *tool_data,
                                             Int offset,
                                             SizeT size,
                                             IRSB *sb) {
    IRStmt *st;
    IRExpr **args = mkIRExprVec_2(mkIRExpr_HWord((HWord) tool_data),
                                  mkIRExpr_HWord(offset));

    IRDirty *di = unsafeIRDirty_0_N(2,
                                   "fi_reg_flip_or_leave_no_state_list_wrap",
                                    VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_no_state_list_wrap),
                                    args);
    di->nFxState = 1;
    di->fxState[0].fx = Ifx_Modify;
    di->fxState[0].offset = offset;
    di->fxState[0].size = size;
    di->fxState[0].nRepeats = 0;
    di->fxState[0].repeatLen = 0;
    di->needsBBP = True;

    st = IRStmt_Dirty(di);
    addStmtToIRSB(sb, st);
}
