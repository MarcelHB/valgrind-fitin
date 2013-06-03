/*
 *************************************************************************
 *
 *                   "DHRYSTONE" Benchmark Program
 *                   -----------------------------
 *
 *  Version:    C, Version 2.1
 *
 *  File:       dhry_2.c (part 3 of 3)
 *
 *  Date:       May 25, 1988
 *
 *  Author:     Reinhold P. Weicker
 *
 *************************************************************************
 */

#include "dhry.h"

#ifndef REG
#define REG
/* REG becomes defined as empty */
/* i.e. no register variables   */
#else
#define REG register
#endif

extern  Int     Int_Glob;
extern  Char    Ch_1_Glob;

Boolean Func_3 (Enumeration Enum_Par_Val);

void Proc_6 (Enumeration Enum_Val_Par, Enumeration *Enum_Ref_Par)
/*********************************/
/* executed once */
/* Enum_Val_Par == Ident_3, Enum_Ref_Par becomes Ident_2 */

{
//	FITIN_MONITOR_VARIABLE(Enum_Val_Par);
//	FITIN_MONITOR_MEMORY(Enum_Ref_Par, sizeof(Enumeration));

    *Enum_Ref_Par = Enum_Val_Par;
    if (! Func_3 (Enum_Val_Par))
        /* then, not executed */
    {
        *Enum_Ref_Par = Ident_4;
    }
    switch (Enum_Val_Par) {
        case Ident_1:
            *Enum_Ref_Par = Ident_1;
            break;
        case Ident_2:
            if (Int_Glob > 100)
                /* then */
            {
                *Enum_Ref_Par = Ident_1;
            } else {
                *Enum_Ref_Par = Ident_4;
            }
            break;
        case Ident_3: /* executed */
            *Enum_Ref_Par = Ident_2;
            break;
        case Ident_4:
            break;
        case Ident_5:
            *Enum_Ref_Par = Ident_3;
            break;
    } /* switch */
} /* Proc_6 */


void Proc_7 (One_Fifty Int_1_Par_Val, One_Fifty Int_2_Par_Val,
             One_Fifty *Int_Par_Ref)
/**********************************************/
/* executed three times                                      */
/* first call:      Int_1_Par_Val == 2, Int_2_Par_Val == 3,  */
/*                  Int_Par_Ref becomes 7                    */
/* second call:     Int_1_Par_Val == 10, Int_2_Par_Val == 5, */
/*                  Int_Par_Ref becomes 17                   */
/* third call:      Int_1_Par_Val == 6, Int_2_Par_Val == 10, */
/*                  Int_Par_Ref becomes 18                   */

{
//	FITIN_MONITOR_VARIABLE(Int_1_Par_Val);
//	FITIN_MONITOR_VARIABLE(Int_2_Par_Val);
//	FITIN_MONITOR_MEMORY(Int_Par_Ref, sizeof(One_Fifty));

    One_Fifty Int_Loc;
//	FITIN_MONITOR_VARIABLE(Int_Loc);

    Int_Loc = Int_1_Par_Val + 2;
    *Int_Par_Ref = Int_2_Par_Val + Int_Loc;
} /* Proc_7 */


void Proc_8 (Arr_1_Dim Arr_1_Par_Ref, Arr_2_Dim Arr_2_Par_Ref,
             Int Int_1_Par_Val, Int Int_2_Par_Val)
/*********************************************************************/
/* executed once      */
/* Int_Par_Val_1 == 3 */
/* Int_Par_Val_2 == 7 */

{
    //FIXME: how to derive Int and 50
    //FITIN_MONITOR_ARRAY(Arr_1_Par_Ref, 50);
    //FIXME: how is the concrete memorylayout of two dim arrays
    //FITIN_MONITOR_ARRAY(Arr_2_Par_Ref, sizeof(Int), 50 * 50);
//	FITIN_MONITOR_VARIABLE(Int_1_Par_Val);
//	FITIN_MONITOR_VARIABLE(Int_2_Par_Val);

    REG One_Fifty Int_Index;
    REG One_Fifty Int_Loc;
//	FITIN_MONITOR_VARIABLE(Int_Index);
//	FITIN_MONITOR_VARIABLE(Int_Loc);

    Int_Loc = Int_1_Par_Val + 5;
    Arr_1_Par_Ref [Int_Loc] = Int_2_Par_Val;
    Arr_1_Par_Ref [Int_Loc+1] = Arr_1_Par_Ref [Int_Loc];
    Arr_1_Par_Ref [Int_Loc+30] = Int_Loc;
    for (Int_Index = Int_Loc; Int_Index <= Int_Loc+1; ++Int_Index) {
        Arr_2_Par_Ref [Int_Loc] [Int_Index] = Int_Loc;
    }
    Arr_2_Par_Ref [Int_Loc] [Int_Loc-1] += 1;
    Arr_2_Par_Ref [Int_Loc+20] [Int_Loc] = Arr_1_Par_Ref [Int_Loc];
    Int_Glob = 5;
} /* Proc_8 */


Enumeration Func_1 (Capital_Letter Ch_1_Par_Val,
                    Capital_Letter Ch_2_Par_Val)
/*************************************************/
/* executed three times                                         */
/* first call:      Ch_1_Par_Val == 'H', Ch_2_Par_Val == 'R'    */
/* second call:     Ch_1_Par_Val == 'A', Ch_2_Par_Val == 'C'    */
/* third call:      Ch_1_Par_Val == 'B', Ch_2_Par_Val == 'C'    */

{
//	FITIN_MONITOR_VARIABLE(Ch_1_Par_Val);
//	FITIN_MONITOR_VARIABLE(Ch_2_Par_Val);

    Capital_Letter        Ch_1_Loc;
    Capital_Letter        Ch_2_Loc;
//	FITIN_MONITOR_VARIABLE(Ch_1_Loc);
//	FITIN_MONITOR_VARIABLE(Ch_2_Loc);

    Ch_1_Loc = Ch_1_Par_Val;
    Ch_2_Loc = Ch_1_Loc;
    if (Ch_2_Loc != Ch_2_Par_Val)
        /* then, executed */
    {
        return (Ident_1);
    } else { /* not executed */
        Ch_1_Glob = Ch_1_Loc;
        return (Ident_2);
    }
} /* Func_1 */

//FIXME: do string monitoring here?
Boolean Func_2 (Str_30 Str_1_Par_Ref, Str_30 Str_2_Par_Ref)
/*************************************************/
/* executed once */
/* Str_1_Par_Ref == "DHRYSTONE PROGRAM, 1'ST STRING" */
/* Str_2_Par_Ref == "DHRYSTONE PROGRAM, 2'ND STRING" */

{
    REG One_Thirty        Int_Loc;
    Capital_Letter    Ch_Loc;

    Int_Loc = 2;
    while (Int_Loc <= 2) /* loop body executed once */
        if (Func_1 (Str_1_Par_Ref[Int_Loc],
                    Str_2_Par_Ref[Int_Loc+1]) == Ident_1)
            /* then, executed */
        {
            Ch_Loc = 'A';
            Int_Loc += 1;
        } /* if, while */
    if (Ch_Loc >= 'W' && Ch_Loc < 'Z')
        /* then, not executed */
    {
        Int_Loc = 7;
    }
    if (Ch_Loc == 'R')
        /* then, not executed */
    {
        return (true);
    } else { /* executed */
        if (strcmp (Str_1_Par_Ref, Str_2_Par_Ref) > 0)
            /* then, not executed */
        {
            Int_Loc += 7;
            Int_Glob = Int_Loc;
            return (true);
        } else { /* executed */
            return (false);
        }
    } /* if Ch_Loc */
} /* Func_2 */


Boolean Func_3 (Enumeration Enum_Par_Val)
/***************************/
/* executed once        */
/* Enum_Par_Val == Ident_3 */

{
//	FITIN_MONITOR_VARIABLE(Enum_Par_Val);

    Enumeration Enum_Loc;
//	FITIN_MONITOR_VARIABLE(Enum_Loc);

    Enum_Loc = Enum_Par_Val;
    if (Enum_Loc == Ident_3)
        /* then, executed */
    {
        return (true);
    } else { /* not executed */
        return (false);
    }
} /* Func_3 */
