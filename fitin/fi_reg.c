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
#include "pub_tool_debuginfo.h"

/* ------- Protoypes, please scroll down for more info  --------*/
static void add_replacement(XArray *list, IRTemp old, IRTemp new);

static void add_modifier_for_register(ToolData *tool_data,
                                      Int offset,
                                      SizeT size,
                                      IRSB *sb);

static void add_modifier_for_offset(ToolData *tool_data,
                                    Int j,
                                    Int initial_offset,
                                    IRSB *sb);

static void analyze_dirty_and_add_modifiers(ToolData *tool_data,
                                            IRDirty *di,
                                            Int nFx,
                                            IRSB *sb);

static void flip_bits(void *data,
                      SizeT size,
                      ULong *bit_table,
                      SizeT table_size);

static UChar flip_byte_at_bit(UChar byte, UChar bit);

static void flip_long_bits(ULong *dest, ULong pattern);

static void flip_byte_bits(UChar *dest, UChar pattern);

static void flip_or_leave(ToolData *tool_data, void *data, LoadState *state);

static void flip_or_leave_on_buffer(ToolData *tool_data, 
                                    UChar *buffer,
                                    Int offset,
                                    SizeT size);

static UChar* get_destination_address(void *a, SizeT size, UChar bit);

static void replace_temps(XArray *replacements, IRExpr **expr);

static void replace_temp(IRTemp temp, IRExpr **expr);

static void update_reg_load_sizes(ToolData *tool_data,
                                  Int offset,
                                  SizeT left,
                                  SizeT full_size);

static void update_reg_origins(ToolData *tool_data, 
                               Int offset,
                               SizeT size,
                               Addr origin_offset);

static IRTemp instrument_access_tmp(ToolData *tool_data,
                                    XArray *loads,
                                    IRTemp tmp,
                                    IRSB *sb);

static IRTemp insert_64bit_resizer(IRTemp src_tmp, IRSB *sb);

static HChar* get_debug_sym_description(ToolData*, Addr);

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_add_temp_load(XArray *list, LoadData *data) {
    VG_(addToXA)(list, data);
    VG_(sortXA)(list);
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
Int fi_reg_compare_loads(const void *l1, const void *l2) {
    IRTemp t1 = ((LoadData*)l1)->dest_temp;
    IRTemp t2 = ((LoadData*)l2)->dest_temp;

    return (t1 == t2) ? 0 : ((t1 < t2) ? -1 : 1);
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
Int fi_reg_compare_replacements(const void *r1, const void *r2) {
    IRTemp t1 = ((ReplaceData*)r1)->old_temp;
    IRTemp t2 = ((ReplaceData*)r2)->old_temp;

    return (t1 == t2) ? 0 : ((t1 < t2) ? -1 : 1);
}

/* Copies data from load state list at `state_list_index' into the register
   shadow fields, beginning at `offset' for `size' bytes. This includes the
   original address, the size of the LD instruction and the monitorable size. */ 
/* ---------------------------------------------------------------------------*/
static void VEX_REGPARM(3) fi_reg_set_occupancy_origin(ToolData *tool_data,
                                                       Int offset,
                                                       SizeT size,
                                                       Word state_list_index) {
    /* After an injection, update any (remaining) information as irrelevant. */
    if(tool_data->runtime_active) {
        LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);
        /* May be irrelevant at run time! */
        if(state->relevant) {
            update_reg_origins(tool_data, offset, size, state->location);
            update_reg_load_sizes(tool_data, offset, size, state->full_size);
        } else {
            update_reg_origins(tool_data, offset, size, (Addr) NULL);
            update_reg_load_sizes(tool_data, offset, size, 0);
        }
    } else {
        update_reg_origins(tool_data, offset, size, (Addr) NULL);
        update_reg_load_sizes(tool_data, offset, size, 0);
    }
}

/* Fills the register shadow fields beginning at `offset' for `size' bytes
   with data indicating irrelevant contents. We need this additional dirty
   as we we cannot pass an IRTemp_INVALID as state_list_index for the
   previous one. */
/* --------------------------------------------------------------------------*/
static void VEX_REGPARM(3) fi_reg_set_occupancy_origin_irrelevant(ToolData *tool_data, 
                                                                  Int offset,
                                                                  SizeT size) {
    update_reg_origins(tool_data, offset, size, (Addr) NULL);
    update_reg_load_sizes(tool_data, offset, size, 0);
}

/* Function that is doing the actual writing to `tool_data'. Starting at
   `offset', it writes the origin for any `size' bytes and adjusts the
   address.*/
/* --------------------------------------------------------------------------*/
static inline void update_reg_origins(ToolData *tool_data, 
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

/* Function that is doing the actual writing to `tool_data'. Starting at
   `offset', it writes for the loaded size and the original monitoring size
   down to the shadows field. Starting at `offset', it writes down as many
   bytes as `left' is large, 0 meaning 'last byte' of this load.*/
/* --------------------------------------------------------------------------*/
static inline void update_reg_load_sizes(ToolData *tool_data,
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
inline Bool fi_reg_set_passive_occupancy(ToolData *tool_data,
                                         XArray *loads,
                                         IRTemp *pre_reg_markers,
                                         Int offset,
                                         IRExpr *expr,
                                         IRSB *sb) {
    if(expr->tag == Iex_RdTmp) {
        if(pre_reg_markers[offset*2] == expr->Iex.RdTmp.tmp) {
            IRTemp original_tmp = pre_reg_markers[offset*2+1];
            Word first, last;
            LoadData key = (LoadData) { original_tmp, Ity_INVALID, NULL, 0 };

            if(VG_(lookupXA)(loads, &key, &first, &last)) {
                LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);

                IRType ty = typeOfIRTemp(sb->tyenv, expr->Iex.RdTmp.tmp);
                SizeT size = sizeofIRType(ty);

                IRExpr **args = mkIRExprVec_4(mkIRExpr_HWord((HWord) tool_data),
                                              mkIRExpr_HWord(offset),
                                              mkIRExpr_HWord(size),
                                              IRExpr_RdTmp(load_data->state_list_index));
                IRDirty *dirty = unsafeIRDirty_0_N(3,
                                                   "fi_reg_set_occupancy_origin",
                                                   VG_(fnptr_to_fnentry)(&fi_reg_set_occupancy_origin),
                                                   args);

                IRStmt *st = IRStmt_Dirty(dirty);
                addStmtToIRSB(sb, st);

                tool_data->reg_temp_occupancies[offset] = expr->Iex.RdTmp.tmp;

                return True;
            }
        }
    }

    return False;
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_set_occupancy(ToolData *tool_data,
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
            dirty = unsafeIRDirty_0_N(3,
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

/* Updates the state list at `state_list_index' for the type `ty'. */
/* --------------------------------------------------------------------------*/
static void VEX_REGPARM(3) fi_reg_update_type(ToolData *tool_data, 
                                              Word state_list_index,
                                              IRType ty) {
    if(tool_data->runtime_active) {
        LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);
        state->size = sizeofIRType(ty);
    }
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline Bool fi_reg_add_load_on_get(ToolData *tool_data,
                                   XArray *loads,
                                   IRTemp new_temp,
                                   IRType ty,
                                   IRExpr *expr,
                                   IRSB *sb) {
    if(expr->tag == Iex_Get) {
        Word first, last;
        Int offset = expr->Iex.Get.offset;
        IRTemp temp = tool_data->reg_temp_occupancies[offset];
        LoadData load_key;

        load_key.dest_temp = temp;

        /* Does not contain anything interesting for us. */
        if(temp == IRTemp_INVALID) {
            return True;
        }

        if(VG_(lookupXA)(loads, &load_key, &first, &last)) {
            /* Load the load data of the original temp, and insert a copy
               that only contains the new IRTemp, created by WrTmp. */
            LoadData *load_data = (LoadData*) VG_(indexXA)(loads, first);

            LoadData new_load_data = *load_data;
            new_load_data.dest_temp = new_temp;
            /* Adjust for retrieval size.*/
            new_load_data.ty = ty;

            /* We must inform the state list about the new type/size. */
            if(ty != load_data->ty) {
                IRStmt *st;
                IRExpr **args;
                IRDirty *dirty;

                args = mkIRExprVec_3(mkIRExpr_HWord((HWord) tool_data),
                                     IRExpr_RdTmp(new_load_data.state_list_index),
                                     mkIRExpr_HWord(ty));

                dirty = unsafeIRDirty_0_N(3,
                                  "fi_reg_update_type",
                                  VG_(fnptr_to_fnentry)(&fi_reg_update_type),
                                  args);
                st = IRStmt_Dirty(dirty);
                addStmtToIRSB(sb, st);
            }

            fi_reg_add_temp_load(loads, &new_load_data);
        }

        return True;
    }

    return False;
}

/* This dirty call must be called before any RdTmp access to an IRTemp which
   has been set up for tracking by preLoadHelper. It gets the value by `data`
   and the index to use for looking of loading information.
   
   The return value may be flipped or not but must be used for replacing the
   original acccessed IRTemp in either case. */
/* --------------------------------------------------------------------------*/
static UWord VEX_REGPARM(3) fi_reg_flip_or_leave(ToolData *tool_data,
                                                 UWord data,
                                                 Word state_list_index) {
    tool_data->loads++;

    LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

    if(tool_data->runtime_active) {
        if(state->data != NULL) {
            flip_or_leave(tool_data, state->data, state);
            return *(UWord*)state->data;
        } else {
            flip_or_leave(tool_data, &data, state);
        }
    } else if(state->data != NULL) {
        return *(UWord*)state->data;
    }

    return data;
}

/* Similar to fi_reg_flip_or_leave, but instead of accepting and returning the
   value to flip, we use a pointer. The flipped value is located at the 
   address which is returned by this function. */
/* --------------------------------------------------------------------------*/
static void* VEX_REGPARM(2) fi_reg_flip_or_leave_ext(ToolData *tool_data,
                                                     Word state_list_index) {
    tool_data->loads++;

    LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

    if(tool_data->runtime_active) {
        if(state->data == NULL) {
            state->data = (void*) VG_(calloc)("fi.reg.external_copy", state->size, 1);
            VG_(memcpy)(state->data, (void*) state->location, state->original_size);
        }

        flip_or_leave(tool_data, state->data, state);
        return state->data;
    } else {
        if(state->data != NULL) {
            return state->data;
        } else {
            return (void*) state->location;
        }
    }
}
/* This dirty call must be used before ST. It will ensure that flipping is only
   done if the original address of the IRTemp and the destination `address' are
   different. */
/* --------------------------------------------------------------------------*/
static UWord VEX_REGPARM(3) fi_reg_flip_or_leave_before_store(ToolData *tool_data,
                                                              UWord data,
                                                              Addr address,
                                                              Word state_list_index) {

    LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

    /* Just count those accesses that do not write back a value to its origin.
       Otherwise, this may result in total > monitored even on 100% effective access
       coverage. */
    if(state->location != address) {
        tool_data->loads++;
    }

    if(tool_data->runtime_active) {
        if(state->location != address) {
            if(state->data != NULL) {
                flip_or_leave(tool_data, state->data, state);
                return *(UWord*)state->data;
            } else {
                flip_or_leave(tool_data, &data, state);
            }
        }
    } else if(state->data != NULL) {
        return *(UWord*)state->data;
    }

    return data;
}

/* Same as above, but also only with pointers. */
/* --------------------------------------------------------------------------*/
static void* VEX_REGPARM(3) fi_reg_flip_or_leave_before_store_ext(ToolData *tool_data,
                                                                  Addr location,
                                                                  Word state_list_index) {
    tool_data->loads++;

    LoadState *state = (LoadState*) VG_(indexXA)(tool_data->load_states, state_list_index);

    if(tool_data->runtime_active && state->location != location) {
        if(state->data == NULL) {
            state->data = (void*) VG_(calloc)("fi.reg.external_copy", state->size, 1);
            VG_(memcpy)(state->data, (void*) state->location, state->original_size);
        }

        flip_or_leave(tool_data, state->data, state);
        return state->data;
    } else {
        if(state->data != NULL) {
            return state->data;
        } else {
            return (void*) state->location;
        }
    }
}

/* Helper function to extract Lua table contents into an array being returned.
   Here, we expect `lua' to have a table on top of its stack when called. The
   extracted number of ULongs is written into `size'. */
/* --------------------------------------------------------------------------*/
static inline ULong* get_lua_table(lua_State *lua, SizeT *size);
static inline ULong* get_lua_table(lua_State *lua, SizeT *size) {
    Int i = 0;
    *size = 0;

    if(!lua_istable(lua,-1)) {
        return NULL;
    }

    /* We need to count before iterating. */
    lua_pushnil(lua);
    for(; lua_next(lua, -2); ++i) {
        lua_pop(lua, 1);
    }

    if(i > 0) {
        ULong *table = (ULong*) VG_(calloc)("fitin.lua.get_lua_table", i, sizeof(ULong));
        lua_pushnil(lua);
        *size = i * sizeof(ULong);
        i = 0;

        for(; lua_next(lua, -2); ++i) {
            tl_assert(lua_isnumber(lua, -1));
            table[i] = lua_tounsigned(lua, -1);
            lua_pop(lua, 1);
        }

        return table;
    } else {
        return NULL;
    }
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
int lua_persist_flip(lua_State *lua) {
    LuaFlipPassData *data = (LuaFlipPassData*) lua_touserdata(lua, -2);

    if(data != NULL && data->type != MEMORY && lua_istable(lua, -1)) {
        SizeT size = 0;
        ULong* table = get_lua_table(lua, &size); 

        if(size > 0) {
            void* origin = NULL; 

            /* NORMAL: we have a load state */
            if(data->type == NORMAL) {
                LoadState *state = data->state;
                origin = (void*) state->location;

                flip_bits(origin, state->full_size, table, size);
            /* REG_TABLE: we have the offset where to look up size + origin */
            } else if(data->type == REG_TABLE) {
                UInt offset = data->offset, full_offset = offset * 2 + 1;
                ToolData *td = data->td;
                origin = (void*) td->reg_origins[offset];

                flip_bits(origin, td->reg_load_sizes[full_offset], table, size);
            }

            if(VG_(clo_verbosity) > 1) {
                VG_(printf)("[FITIn] FLIP (secondary)! Data at %p\n", origin);
            }
        }

        VG_(free)(table);
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
int lua_get_debug_description(lua_State *lua) {
    LuaFlipPassData *data = (LuaFlipPassData*) lua_touserdata(lua, -1);

    if(data != NULL) {
        ToolData *td = data->td;

        if(td->with_debug_symbols) {
            HChar *descr = get_debug_sym_description(td, data->addr);
            lua_pushstring(lua, descr);
            VG_(free)(descr);

            return 1;
        }
    }

    lua_pushstring(lua, "");
    return 1;
}

/* --------------------------------------------------------------------------*/
int lua_flip_on_memory(lua_State *lua) {
    if(lua_istable(lua, -1)) {
        SizeT table_size = 0;
        ULong *table = get_lua_table(lua, &table_size);
        void* address = (void*) lua_tonumber(lua, -3);
        SizeT size = (SizeT) lua_tonumber(lua, -2);

        flip_bits(address, size, table, table_size);

        VG_(free)(table);
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
static inline HChar* get_debug_sym_description(ToolData *tool_data, Addr a) {
    HChar *description = NULL;

    if(tool_data->with_debug_symbols) {
        XArray* dname1v = VG_(newXA)(VG_(malloc), "fitin.description", VG_(free), sizeof(HChar));
        XArray* dname2v = VG_(newXA)(VG_(malloc), "fitin.description", VG_(free), sizeof(HChar));

        if (VG_(get_data_description)(dname1v, dname2v, a)) {
            HChar *d1 = (HChar*) VG_(indexXA)(dname1v, 0);
            HChar *d2 = (HChar*) VG_(indexXA)(dname2v, 0);
            SizeT d1s = VG_(strlen)(d1);
            SizeT d2s = VG_(strlen)(d2);
            SizeT str_size = d1s + d2s + 3;

            description = (HChar*) VG_(malloc)("fitin.description", str_size);
            VG_(memcpy)(description, d1, d1s);
            VG_(memcpy)(description + d1s, "; ", 2);
            VG_(memcpy)(description + d1s + 2, d2, d2s + 1);
        }

        VG_(deleteXA)(dname1v);
        VG_(deleteXA)(dname2v);
    }

    return description;
}

/* The method that is performing the bit-flip if applicable. It takes place 
   inside of `data' and will be returned. */
/* --------------------------------------------------------------------------*/
static inline void flip_or_leave(ToolData *tool_data,
                                 void *data,
                                 LoadState *state) {

    if(state->relevant) {
        tool_data->monLoadCnt++;

        if(tool_data->available_callbacks & CALLBACK_FLIP) {
            LuaFlipPassData lua_data = { tool_data, state, NORMAL, 0, state->location };
            lua_getglobal(tool_data->lua, "flip_value");
            lua_pushlightuserdata(tool_data->lua, &lua_data);
            lua_pushinteger(tool_data->lua, state->location);
            lua_pushinteger(tool_data->lua, tool_data->monLoadCnt);
            lua_pushinteger(tool_data->lua, (ULong) state->original_size);

            if(lua_pcall(tool_data->lua, 4, 1, 0) == 0) {
                SizeT size = 0;
                ULong *table = get_lua_table(tool_data->lua, &size); 

                if(size > 0) {
                    flip_bits(data, state->size, table, size);
                    VG_(free)(table);
                }

                lua_pop(tool_data->lua, 1);
            } else {
                VG_(printf)("LUA: %s\n", lua_tostring(tool_data->lua, -1));
            }
        }
    }
}

/* --------------------------------------------------------------------------*/
static inline void flip_or_leave_mem(ToolData *tool_data, Addr a, SizeT size) {
    tool_data->loads++;
    tool_data->monLoadCnt++;

    if(tool_data->available_callbacks & CALLBACK_FLIP) {
        LuaFlipPassData lua_data = { NULL, NULL, MEMORY, 0, a };
        lua_getglobal(tool_data->lua, "flip_value");
        lua_pushlightuserdata(tool_data->lua, &lua_data);
        lua_pushinteger(tool_data->lua, (ULong) a);
        lua_pushinteger(tool_data->lua, tool_data->monLoadCnt);
        lua_pushinteger(tool_data->lua, (ULong) size);

        if(lua_pcall(tool_data->lua, 4, 1, 0) == 0) {
            SizeT table_size = 0;
            ULong *table = get_lua_table(tool_data->lua, &table_size); 

            if(table_size > 0) {
                flip_bits((void*) a, size, table, table_size);
                VG_(free)(table);
            }

            lua_pop(tool_data->lua, 1);
        } else {
            VG_(printf)("LUA: %s\n", lua_tostring(tool_data->lua, -1));
        }
    }
}

/* Wrapper for fi_reg_flip_or_leave_mem to be inserted before an IRDirty that
   reads on `a' by `size' bytes. */
/* --------------------------------------------------------------------------*/
static void VEX_REGPARM(3) fi_reg_flip_or_leave_mem_wrap(ToolData *tool_data,
                                                         Addr a,
                                                         SizeT size) {
    if(tool_data->runtime_active) {
        fi_reg_flip_or_leave_mem(tool_data, a, size);
    }
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_flip_or_leave_mem(ToolData *tool_data, Addr a, SizeT size) {
    Word first, last;
    Int mem_offset = 0;

    for(; mem_offset <= size; ++mem_offset) {
        Monitorable key;
        key.monAddr = a + mem_offset;

        if(VG_(lookupXA)(tool_data->monitorables, &key, &first, &last)) {
            Monitorable *mon = (Monitorable*) VG_(indexXA)(tool_data->monitorables, first);

            if(mon->monValid) {
                flip_or_leave_mem(tool_data, a, mon->monSize);
            }
        }
    }
}

/* Helper to extract a byte-precise address. This can be used to do the actual
   flip. It tests whether `bit' is located inside of `a' + `size'. If not,
   this will return NULL. */
/* --------------------------------------------------------------------------*/
static inline UChar* get_destination_address(void *a, SizeT size, UChar bit) {
    UChar bit_size = size * 8; 

    if(bit < bit_size) {
        UChar byte = bit / 8;
        return ((UChar*) a) + byte;
    }

    return NULL;
}

/* Flip peforming method, to flip `bit' in the given `byte'. Reduces code
   duplication. */
/* --------------------------------------------------------------------------*/
static inline UChar flip_byte_at_bit(UChar byte, UChar bit) {
    return byte ^ (1 << bit); 
}

/* --------------------------------------------------------------------------*/
static inline void flip_long_bits(ULong *dest, ULong pattern) {
    *dest = *dest ^ pattern;
}

/* --------------------------------------------------------------------------*/
static inline void flip_byte_bits(UChar *dest, UChar pattern) {
    *dest = *dest ^ pattern;
}

/* --------------------------------------------------------------------------*/
static inline void flip_bits(void *data,
                             SizeT size,
                             ULong *bit_table,
                             SizeT table_size) {

    SizeT min_size = size <= table_size ? size : table_size;
    Int longs = min_size / sizeof(ULong), i = longs - 1;

    for(; i >= 0; --i) {
        flip_long_bits(((ULong*)data) + i, bit_table[i]);
    }

    Int long_offset = longs * sizeof(ULong), left = min_size - long_offset;
    i = left - 1;

    /* Flip remaining bytes. */
    for(; i >= 0; --i) {
        Int idx = long_offset + i;
        flip_byte_bits(((UChar*)data) + idx, ((UChar*)bit_table)[idx]);
    }
}

/* This is an important method to be called before inserting any flip-helper.
   As loads may be < UWord, but arguments are required to have UWord-size,
   this method inserts casts to change the size to UWord, not respecting signs. */
/* --------------------------------------------------------------------------*/
static inline IRTemp insert_size_widener(ToolData *tool_data, 
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
   all necessary things, and inserting the helper. Needs `loads' for
   doing a relevance check for `tmp'. */
/* --------------------------------------------------------------------------*/
static inline IRTemp instrument_access_tmp(ToolData *tool_data,
                                           XArray *loads,
                                           IRTemp tmp,
                                           IRSB *sb) {

    LoadData key = (LoadData) { tmp, 0, 0 };
    Word first, last;

    if(VG_(lookupXA)(loads, &key, &first, &last)) {
        IRStmt *st;
        LoadData *load_data, new_load_data;
        IRTemp new_temp, access_temp = tmp;
        IRExpr **args;
        IRDirty *dirty;
        IRType ty = typeOfIRTemp(sb->tyenv, tmp);
        Bool reload = False;
        load_data = (LoadData*) VG_(indexXA)(loads, first);

        if(load_data->passive) {
            return IRTemp_INVALID;
        }

        if(ty <= tool_data->gWordTy) {
            /* Insert widener if temp is too small for being an argument. */
            if(ty < tool_data->gWordTy) {
                access_temp = insert_size_widener(tool_data, tmp, ty, sb);

                if(access_temp == IRTemp_INVALID) {
                    return IRTemp_INVALID;
                }
            }

            new_temp = newIRTemp(sb->tyenv, ty);
            args = mkIRExprVec_3(mkIRExpr_HWord((HWord) tool_data),
                                 IRExpr_RdTmp(access_temp),
                                 IRExpr_RdTmp(load_data->state_list_index));
            dirty = unsafeIRDirty_0_N(3,
                                      "fi_reg_flip_or_leave",
                                      VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave),
                                      args);
        } else {
            new_temp = newIRTemp(sb->tyenv, tool_data->gWordTy);
            args = mkIRExprVec_2(mkIRExpr_HWord((HWord) tool_data),
                                 IRExpr_RdTmp(load_data->state_list_index));
            dirty = unsafeIRDirty_0_N(2,
                                      "fi_reg_flip_or_leave_ext",
                                      VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_ext),
                                      args);
            reload = True;
        }

        /* Must assume `persist_flip' call. Claim what is possible:
           Cannot cover e.g. manual `flip_on_address' calls. */
        dirty->mAddr = load_data->addr;
        dirty->mSize = sizeofIRType(ty);
        dirty->mFx = Ifx_Write;
        dirty->tmp = new_temp;

        st = IRStmt_Dirty(dirty);
        addStmtToIRSB(sb, st);

        if(reload) {
            IRExpr *expr = IRExpr_Load(load_data->end, load_data->ty, IRExpr_RdTmp(new_temp));
            new_temp = newIRTemp(sb->tyenv, load_data->ty);
            addStmtToIRSB(sb, IRStmt_WrTmp(new_temp, expr));
        }

        /* We have to treat the newly introduced value equally to the original one. */
        new_load_data = *load_data;
        new_load_data.dest_temp = new_temp;

        VG_(addToXA)(loads, &new_load_data);
        VG_(sortXA)(loads);

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
inline void  fi_reg_instrument_access(ToolData *tool_data,
                                      XArray *loads,
                                      XArray *replacements,
                                      IRExpr **expr,
                                      IRSB *sb,
                                      Bool replace_only) {
    switch((*expr)->tag) {
        case Iex_GetI:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.GetI.ix);
            break;
        case Iex_RdTmp: {
            /* Stop recursion if arrived on a RdTmp. And replace
               it if there are replacement entries for this one. */
            replace_temps(replacements, expr);
            if(!replace_only) {
                IRTemp new_temp = instrument_access_tmp(tool_data,
                                                        loads,
                                                        (*expr)->Iex.RdTmp.tmp,
                                                        sb);
                if(new_temp != IRTemp_INVALID) {
                    add_replacement(replacements,
                                    (*expr)->Iex.RdTmp.tmp,
                                    new_temp);

                    replace_temp(new_temp, expr);
                }
            }
            break;
        }
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
        case Iex_ITE:
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.ITE.cond);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.ITE.iftrue);
            INSTRUMENT_NESTED_ACCESS((*expr)->Iex.ITE.iffalse);
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

/* --------------------------------------------------------------------------*/
inline Bool fi_reg_skip_on_resize(IRExpr *expr) {
    if(expr->tag == Iex_Unop) {
        IROp op = expr->Iex.Unop.op;
        /* This is hopefully complete/correct. Doc is not always that precise here. */
        if((op >= Iop_DivModU64to32 && op <= Iop_1Sto64) ||
           (op >= Iop_F64toI16S && op <= Iop_F64toF32) ||
           (op >= Iop_F64HLtoF128 && op <= Iop_F128LOtoF64) ||
           (op >= Iop_I32StoF128 && op <= Iop_F128toF32) ||
           (op >= Iop_QNarrowBin16Sto8Ux8 && op <= Iop_NarrowBin32to16x4) ||
           (op >= Iop_D32toD64 && op <= Iop_D128toF128) ||
           (op >= Iop_D64HLtoD128 && op <= Iop_D128LOtoD64) ||
           (op >= Iop_I32UtoFx4 && op <= Iop_QFtoI32Sx4_RZ) ||
           (op >= Iop_F32toF16x4 && op <= Iop_F16toF32x4) ||
           (op >= Iop_V128to64 && op <= Iop_V128to32) ||
           (op >= Iop_QNarrowBin16Sto8Ux16 && op <= Iop_Widen32Sto64x2) ||
           (op >= Iop_V256to64_0 && Iop_V128HLtoV256)
           ) {
            return True;
        }
    }

    return False;
}

/* This method adds a resizer for 32->64 or 64-32 if converting operation `op`
   is incompatible to the temp of `data'.

   This will produce many neutralizing statements, but it works while preserving
   usability of the replacing-algorithm. Should be optimized. */
/* --------------------------------------------------------------------------*/
static inline IRTemp insert_64bit_resizer(IRTemp src_tmp, IRSB *sb) {
   IROp op = Iop_32Uto64;
   IRType new_type = Ity_I64;
   IRType original_type = typeOfIRTemp(sb->tyenv, src_tmp);

   /* 32 -> 64 */
   if(original_type == Ity_I64) {
       op = Iop_64to32;
       new_type = Ity_I32;
   }
   
   IRTemp new_temp = newIRTemp(sb->tyenv, new_type);
   IRStmt *st = IRStmt_WrTmp(
                             new_temp,
                             IRExpr_Unop(
                                 op,
                                 IRExpr_RdTmp(
                                     src_tmp
                                 )
                             )
                          );
   addStmtToIRSB(sb, st);

   return new_temp;
}

/* Basically the same as instrument_access_tmp. But to be used on RdTmp on
   a Store instruction. This one also gets the destination expression
   `address'. */
/* --------------------------------------------------------------------------*/
static inline IRTemp instrument_access_tmp_on_store(ToolData *tool_data,
                                                    XArray *loads,
                                                    IRTemp tmp,
                                                    IRExpr *address,
                                                    IRType ty,
                                                    IRSB *sb) {
    LoadData key = (LoadData) { tmp, 0, 0 };
    Word first, last;

    if(VG_(lookupXA)(loads, &key, &first, &last)) {
        IRStmt *st;
        LoadData *load_data, new_load_data;
        IRTemp new_temp, access_temp = tmp;
        IRExpr **args;
        Bool reload = False;
        IRDirty *dirty;
        load_data = (LoadData*) VG_(indexXA)(loads, first);

        if(load_data->passive) {
            return IRTemp_INVALID;
        }

        if(ty <= tool_data->gWordTy) {
            /* Insert widener if temp is too small for being an argument. */
            if(ty < tool_data->gWordTy) {
                access_temp = insert_size_widener(tool_data, tmp, ty, sb);
                
                if(access_temp == IRTemp_INVALID) {
                    return IRTemp_INVALID;
                }
            } 
                    
            new_temp = newIRTemp(sb->tyenv, ty);
            args = mkIRExprVec_4(mkIRExpr_HWord((HWord) tool_data),
                                 IRExpr_RdTmp(access_temp), 
                                 address,
                                 IRExpr_RdTmp(load_data->state_list_index));
            dirty = unsafeIRDirty_0_N(3,
                                      "fi_reg_flip_or_leave_before_store",
                                       VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_before_store),
                                       args);
        } else {
            new_temp = newIRTemp(sb->tyenv, tool_data->gWordTy);
            args = mkIRExprVec_3(mkIRExpr_HWord((HWord) tool_data),
                                 address,
                                 IRExpr_RdTmp(load_data->state_list_index));
            dirty = unsafeIRDirty_0_N(3,
                                      "fi_reg_flip_or_leave_before_store_ext",
                                      VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_before_store_ext),
                                      args);
            reload = True;

        }

        /* See instrument_access_tmp. */
        dirty->mAddr = load_data->addr;
        dirty->mSize = sizeofIRType(ty);
        dirty->mFx = Ifx_Write;
        dirty->tmp = new_temp;

        st = IRStmt_Dirty(dirty);
        addStmtToIRSB(sb, st);

        if(reload) {
            IRExpr *expr = IRExpr_Load(load_data->end, load_data->ty, IRExpr_RdTmp(new_temp));
            new_temp = newIRTemp(sb->tyenv, load_data->ty);
            addStmtToIRSB(sb, IRStmt_WrTmp(new_temp, expr));
        }

        new_load_data = *load_data;
        new_load_data.dest_temp = new_temp;

        VG_(addToXA)(loads, &new_load_data);
        VG_(sortXA)(loads);

        return new_temp;
    }

    return IRTemp_INVALID;
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline Bool fi_reg_instrument_store(ToolData *tool_data,
                                    XArray *loads,
                                    XArray *replacements,
                                    IRExpr **expr,
                                    IRExpr *address,
                                    IRSB *sb) {
    /* RdTmp is of course the only relevant expression. */
    if((*expr)->tag == Iex_RdTmp) {
        replace_temps(replacements, expr);

        IRTemp new_temp = instrument_access_tmp_on_store(tool_data,
                            loads,
                            (*expr)->Iex.RdTmp.tmp,
                            address,
                            typeOfIRTemp(sb->tyenv, (*expr)->Iex.RdTmp.tmp),
                            sb);

        if(new_temp != IRTemp_INVALID) {
            add_replacement(replacements,
                            (*expr)->Iex.RdTmp.tmp,
                            new_temp);

            replace_temp(new_temp, expr);
        }

        return True;
    }

    return False;
}

/* Helper to add a replacement entry into `list', replacing `old_temp' by
   `new_temp'. */
/* --------------------------------------------------------------------------*/
static inline void add_replacement(XArray *list, IRTemp old_temp, IRTemp new_temp) {
    ReplaceData data = (ReplaceData) { old_temp, new_temp };
    VG_(addToXA)(list, &data);
    VG_(sortXA)(list);
}

/* Checks a given expression for temps that need to be replaced. */
/* --------------------------------------------------------------------------*/
static inline void replace_temps(XArray *replacements, IRExpr **expr) {
    ReplaceData key = (ReplaceData) { (*expr)->Iex.RdTmp.tmp, 0 };
    Word first, last;
    Bool found = False;

    while(VG_(lookupXA)(replacements, &key, &first, &last)) {
        found = True;
        ReplaceData *replace_data = (ReplaceData*) VG_(indexXA)(replacements, first);
        key.old_temp = replace_data->new_temp;
    }

    if(found) {
        /* Unless copied, all uses of an IRTemp share the same IRExpr,
           so otherwise this would change previous uses as well. */
        *expr = deepCopyIRExpr(*expr);
        ReplaceData *replace_data = (ReplaceData*) VG_(indexXA)(replacements, first);
        (*expr)->Iex.RdTmp.tmp = replace_data->new_temp;
    }
}

/* To be used if the new temp `temp' is already determined to replace the one
   of `expr'. */
/* --------------------------------------------------------------------------*/
static inline void replace_temp(IRTemp temp, IRExpr **expr) {
    *expr = deepCopyIRExpr(*expr);
    (*expr)->Iex.RdTmp.tmp = temp;
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_add_pre_dirty_modifiers(ToolData *tool_data, IRDirty *st, IRSB *sb) {
    if(st->nFxState > 0) {
        Int i = 0;
        for(; i < st->nFxState; ++i) {
            if(st->fxState[i].fx == Ifx_Read ||
               st->fxState[i].fx == Ifx_Modify) {
                analyze_dirty_and_add_modifiers(tool_data, st, i, sb);
            }
        }
    }
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_add_pre_dirty_modifiers_mem(ToolData *tool_data,
                                               IRExpr *address,
                                               Int size,
                                               IRSB *sb) {
    IRStmt *st;
    IRExpr **args = mkIRExprVec_3(mkIRExpr_HWord((HWord) tool_data),
                                  address,
                                  mkIRExpr_HWord(size));

    IRDirty *di = unsafeIRDirty_0_N(3,
                                   "fi_reg_flip_or_leave_mem_wrap",
                                    VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_mem_wrap),
                                    args);
    di->mFx = Ifx_Modify;
    di->mSize = size;
    di->mAddr = address;

    st = IRStmt_Dirty(di);
    addStmtToIRSB(sb, st);

}

/* This method checks the signature of dirty-call `st' and fxState `nFx' to
   determine the offsets that are used. */
/* --------------------------------------------------------------------------*/
static inline void analyze_dirty_and_add_modifiers(ToolData *tool_data,
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

/* For a given offset `j', originating from offset `initial_offset',
   this method will look for the occupancies of this offset and prevents
   collisions (multiple calls on the same IRTemp). */
/* --------------------------------------------------------------------------*/
static inline void add_modifier_for_offset(ToolData *tool_data,
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

/* Wrapper for fi_reg_flip_or_leave_registers to be called from VEX IR,
   this will read the data from `offset' at `bp' and write back the flipped
   value. This is called before reg-reading IRDirty calls. */
/* --------------------------------------------------------------------------*/
static void VEX_REGPARM(0) fi_reg_flip_or_leave_registers_wrap(void *bp,
                                                               ToolData *tool_data,
                                                               SizeT size,
                                                               Int offset) {
    if(tool_data->runtime_active) {
        fi_reg_flip_or_leave_registers(tool_data, ((UChar*) bp) + offset, offset, size);
    }
}

/* A method that is configuring and inserting the helper function before an
   IRDirty calls. */
/* --------------------------------------------------------------------------*/
static inline void add_modifier_for_register(ToolData *tool_data,
                                             Int offset,
                                             SizeT size,
                                             IRSB *sb) {
    IRStmt *st;
    IRExpr **args = mkIRExprVec_4(IRExpr_BBPTR(),
                                  mkIRExpr_HWord((HWord) tool_data),
                                  mkIRExpr_HWord(size),
                                  mkIRExpr_HWord(offset));

    IRDirty *di = unsafeIRDirty_0_N(0,
                                   "fi_reg_flip_or_leave_registers_wrap",
                                    VG_(fnptr_to_fnentry)(&fi_reg_flip_or_leave_registers_wrap),
                                    args);
    di->nFxState = 1;
    di->fxState[0].fx = Ifx_Modify;
    di->fxState[0].offset = offset;
    di->fxState[0].size = size;
    di->fxState[0].nRepeats = 0;
    di->fxState[0].repeatLen = 0;

    st = IRStmt_Dirty(di);
    addStmtToIRSB(sb, st);
}

/* See fi_reg.h */
/* --------------------------------------------------------------------------*/
inline void fi_reg_flip_or_leave_registers(ToolData *tool_data,
                                           UChar *buffer,
                                           PtrdiffT offset,
                                           SizeT size) {
    Int i = 0, from = 0, until = -1;
    Bool open = False;
    SizeT last_left = 0;

    /* Look for all possible fragments of used registers. */
    for(; i < size; ++i) {
        if(tool_data->reg_origins[offset + i] != 0) {
            if(!open) {
                open = True;
                from = i;
            }

            SizeT left = tool_data->reg_load_sizes[(offset + i)*2];
            if(left == 0) {
                until = i;
            } else if(i > 0 && last_left <= left) {
                until = i - 1;
            }

            last_left = left;
        } else {
            /* Fragment followed by 0. */
            if(open) {
                until = i;
            }
        }

        if(until > 0) {
            open = False;
            flip_or_leave_on_buffer(tool_data,
                                    buffer,
                                    offset + from,
                                    (until - from) + 1); 
            until = 0;
        }
    }

    /* Fragment finishing on last byte (or larger). */
    if(open) {
        flip_or_leave_on_buffer(tool_data,
                                buffer,
                                offset + from,
                                (until - from) + 1); 
    }
}

/* Performs a flip on a given buffer `buffer' within `size',  supports write-back
   for register `offset'.*/
/* -------------------------------------------------------------------------------*/
static inline void flip_or_leave_on_buffer(ToolData *tool_data, 
                                           UChar *buffer,
                                           Int offset,
                                           SizeT size) {
    tool_data->loads++;
    tool_data->monLoadCnt++;

    void *origin = (void*) tool_data->reg_origins[offset];

    if(tool_data->available_callbacks & CALLBACK_FLIP) {
        LuaFlipPassData lua_data = { tool_data, NULL, REG_TABLE, offset, (Addr) origin };
        lua_getglobal(tool_data->lua, "flip_value");
        lua_pushlightuserdata(tool_data->lua, &lua_data);
        lua_pushinteger(tool_data->lua, (ULong) origin);
        lua_pushinteger(tool_data->lua, tool_data->monLoadCnt);
        lua_pushinteger(tool_data->lua, (ULong) size);

        if(lua_pcall(tool_data->lua, 4, 1, 0) == 0) {
            SizeT table_size = 0;
            ULong *table = get_lua_table(tool_data->lua, &table_size); 

            if(table_size > 0) {
                flip_bits(buffer, size, table, table_size);
                VG_(free)(table);
            }

            lua_pop(tool_data->lua, 1);
        } else {
            VG_(printf)("LUA: %s\n", lua_tostring(tool_data->lua, -1));
        }
    }
}
