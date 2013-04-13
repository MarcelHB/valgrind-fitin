#include "fi_reg.h"

static void combine_lists(XArray *list, XArray *tail);

// ----------------------------------------------------------------------------
inline void fi_reg_add_temp_load(XArray *list, IRTemp *dest, IRTemp *state) {
    LoadData *data = VG_(malloc)("fi.reg.load.list.add", sizeof(LoadData*));
    data->dest_temp = dest;
    data->state_temp = state;
    VG_(addToXA)(list, data);
}

// ----------------------------------------------------------------------------
inline Bool fi_reg_add_replacements(XArray *list, XArray *replacements) {
    if(replacements != NULL) {
        combine_lists(list, replacements);
        return True;
    }
    
    return False;
}

// ----------------------------------------------------------------------------
inline XArray* fi_reg_instrument_access(XArray *loads, IRExpr *expr, IRSB *sb) {
    return NULL; 
}

// ----------------------------------------------------------------------------
static inline void combine_lists(XArray *list, XArray *tail) {
    UInt i = 0;
    void *array = NULL;
    Word elements = 0;

    VG_(getContentsXA_UNSAFE)(tail, &array, &elements);
    for(; i < elements; ++i) {
        VG_(addToXA)(list, array + i);
    }

    VG_(deleteXA)(tail);
}
