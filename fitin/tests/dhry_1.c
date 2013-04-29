/*       gcc dhry_1.c dhry_2.c cpuidc64.o cpuida64.o -m64 -lrt -lc -lm -o dhry2
 *************************************************************************
 *
 *                   "DHRYSTONE" Benchmark Program
 *                   -----------------------------
 *
 *  Version:    C, Version 2.1
 *
 *  File:       dhry_1.c (part 2 of 3)
 *
 *  Date:       May 25, 1988
 *
 *  Author:     Reinhold P. Weicker
 *
 *************************************************************************
 *
 *     #define options not used
 */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "dhry.h"
//#include "cpuidh.h"

#ifdef CNNT
#define options   "Non-optimised"
#define opt "0"
#else
//    #define options   "Optimised"
#define options   "Opt 3 64 Bit"
#define opt ""
#endif


/* Global Variables: */

Rec_Pointer     Ptr_Glob,
                Next_Ptr_Glob;
Int             Int_Glob;
Boolean         Bool_Glob;
Char            Ch_1_Glob,
                Ch_2_Glob;
Int             Arr_1_Glob [50];
Int             Arr_2_Glob [50] [50];

Char Reg_Define[40] = "Register option      Selected.";


Enumeration Func_1 (Capital_Letter Ch_1_Par_Val,
                    Capital_Letter Ch_2_Par_Val);
/*
   forward declaration necessary since Enumeration may not simply be int
 */

#ifndef ROPT
#define REG
/* REG becomes defined as empty */
/* i.e. no register variables   */
#else
#define REG register
#endif

void Proc_1 (REG Rec_Pointer Ptr_Val_Par);
void Proc_2 (One_Fifty *Int_Par_Ref);
void Proc_3 (Rec_Pointer *Ptr_Ref_Par);
void Proc_4 ();
void Proc_5 ();
void Proc_6 (Enumeration Enum_Val_Par, Enumeration *Enum_Ref_Par);
void Proc_7 (One_Fifty Int_1_Par_Val, One_Fifty Int_2_Par_Val,
             One_Fifty *Int_Par_Ref);
void Proc_8 (Arr_1_Dim Arr_1_Par_Ref, Arr_2_Dim Arr_2_Par_Ref,
             Int Int_1_Par_Val, Int Int_2_Par_Val);

Boolean Func_2 (Str_30 Str_1_Par_Ref, Str_30 Str_2_Par_Ref);


/* variables for time measurement: */

// #define Too_Small_Time 2
/* Measurements should last at least 2 seconds */
/*
   double         User_Time;

   double          Microseconds,
   Dhrystones_Per_Second,
   Vax_Mips;
 */
/* end of variables for time measurement */


int main (int argc, char *argv[])
/*****/

/* main program, corresponds to procedures        */
/* Main and Proc_0 in the Ada version             */
{

    One_Fifty   Int_1_Loc;
    REG   One_Fifty   Int_2_Loc;
    One_Fifty   Int_3_Loc;
    REG   Char        Ch_Index;
    Enumeration Enum_Loc;
    Str_30      Str_1_Loc;
    Str_30      Str_2_Loc;
    REG   Int         Run_Index;
    REG   Int         Number_Of_Runs;
    Int         endit, count = 10;
    //         FILE        *Ap;
    Int         errors = 0;
    Int         i;
    //         Int         nopause = 1;

    FITIN_MONITOR_BOOL(Bool_Glob);
    /*
    #ifndef FLIPSAFE

      FITIN_MONITOR_VARIABLE(Ch_1_Glob);
      FITIN_MONITOR_VARIABLE(Ch_2_Glob);


      FITIN_MONITOR_VARIABLE(Int_1_Loc);
      FITIN_MONITOR_VARIABLE(Int_Glob);
      FITIN_MONITOR_VARIABLE(Int_2_Loc);
      FITIN_MONITOR_VARIABLE(Int_3_Loc);
      FITIN_MONITOR_VARIABLE(Ch_Index);
      FITIN_MONITOR_VARIABLE(Run_Index);
      FITIN_MONITOR_VARIABLE(Number_Of_Runs);

      FITIN_MONITOR_VARIABLE(i);
    #else
      FITIN_MONITOR_VARIABLE(Int_Glob);
      FITIN_MONITOR_VARIABLE(Ch_1_Glob);
      FITIN_MONITOR_VARIABLE(Ch_2_Glob);


      FITIN_MONITOR_VARIABLE(Int_1_Loc);
      FITIN_MONITOR_VARIABLE(Int_2_Loc);
      FITIN_MONITOR_VARIABLE(Int_3_Loc);
      FITIN_MONITOR_VARIABLE(Ch_Index);
      FITIN_MONITOR_VARIABLE(Run_Index);
      FITIN_MONITOR_VARIABLE(Number_Of_Runs);

      FITIN_MONITOR_VARIABLE(i);

    #endif
    */
    /***********************************************************************
     *         Change for compiler and optimisation used                   *
     ***********************************************************************/

    Next_Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));
    Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));

    Ptr_Glob->Ptr_Comp                    = Next_Ptr_Glob;
    Ptr_Glob->Discr                       = Ident_1;
    Ptr_Glob->variant.var_1.Enum_Comp     = Ident_3;
    Ptr_Glob->variant.var_1.Int_Comp      = 40;
    strcpy (Ptr_Glob->variant.var_1.Str_Comp,
            "DHRYSTONE PROGRAM, SOME STRING");
    strcpy (Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

    Arr_2_Glob [8][7] = 10;
    /* Was missing in published program. Without this statement,   */
    /* Arr_2_Glob [8][7] would have an undefined value.            */
    /* Warning: With 16-Bit processors and Number_Of_Runs > 32000, */
    /* overflow may occur for this array element.                  */

    printf("##########################################\n");

    printf ("\n");
    printf ("Dhrystone Benchmark, Version 2.1 (Language: C or C++)\n");
    printf ("\n");

    printf ("Optimisation    %s\n", options);
#ifdef ROPT
    printf ("Register option selected\n\n");
#else
    printf ("Register option not selected\n\n");
    strcpy(Reg_Define, "Register option  Not selected.");
#endif                 //"Register option      Selected."

    /*
       if (Reg)
       {
       printf ("Program compiled with 'register' attribute\n");
       printf ("\n");
       }
       else
       {
       printf ("Program compiled without 'register' attribute\n");
       printf ("\n");
       }
     */
    /* Initializations */
    if (argc > 1) {
        Number_Of_Runs = atol(argv[1]);
    } else {
        Number_Of_Runs = 50;
    }
    printf ("Execution starts, %d runs through Dhrystone\n",
            (int) Number_Of_Runs);

    //do
    //{

    //Number_Of_Runs = Number_Of_Runs * 2;
    count = count - 1;
    Arr_2_Glob [8][7] = 10;

    Proc_5();
    for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index) {

        Proc_4();
        /* Ch_1_Glob == 'A', Ch_2_Glob == 'B', Bool_Glob == true */
        Int_1_Loc = 2;
        Int_2_Loc = 3;
        strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
        Enum_Loc = Ident_2;
        Bool_Glob = Bool_Glob && ! Func_2 (Str_1_Loc, Str_2_Loc);
        /* Bool_Glob == 1 */
        while (Int_1_Loc < Int_2_Loc) { /* loop body executed once */
            Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
            /* Int_3_Loc == 7 */
            Proc_7 (Int_1_Loc, Int_2_Loc, &Int_3_Loc);
            /* Int_3_Loc == 7 */
            Int_1_Loc += 1;
        }   /* while */
        /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
        Proc_8 (Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
        /* Int_Glob == 5 */
        Proc_1 (Ptr_Glob);
        for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index)
            /* loop body executed twice */
        {
            if (Enum_Loc == Func_1 (Ch_Index, 'C'))
                /* then, not executed */
            {
                Proc_6 (Ident_1, &Enum_Loc);
                strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
                Int_2_Loc = Run_Index;
                Int_Glob = Run_Index;
            }
        }
        /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
        Int_2_Loc = Int_2_Loc * Int_1_Loc;
        Int_1_Loc = Int_2_Loc / Int_3_Loc;
        Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
        /* Int_1_Loc == 1, Int_2_Loc == 13, Int_3_Loc == 7 */
        Proc_2 (&Int_1_Loc);
        /* Int_1_Loc == 5 */

    }   /* loop "for Run_Index" */

    //}   /* calibrate/run do while */
    //while (count >0);

    printf ("\n");
    printf ("Final values (* implementation-dependent):\n");
    printf ("\n");
    printf ("Int_Glob:      ");
    if (Int_Glob == 5) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d  ", (int) Int_Glob);

    printf ("Bool_Glob:     ");
    if (Bool_Glob == true) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    // FITIN-reg: cast to int to have C compatibility (instead of bool)
    printf ("%d\n", (int) Bool_Glob);

    printf ("Ch_1_Glob:     ");
    if (Ch_1_Glob == 'A') {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%c  ", Ch_1_Glob);

    printf ("Ch_2_Glob:     ");
    if (Ch_2_Glob == 'B') {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%c\n",  Ch_2_Glob);

    printf ("Arr_1_Glob[8]: ");
    if (Arr_1_Glob[8] == 7) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d  ", (int)Arr_1_Glob[8]);

    printf ("Arr_2_Glob8/7: ");
    if (Arr_2_Glob[8][7] == Number_Of_Runs + 10) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%10d\n", (int)Arr_2_Glob[8][7]);

    printf ("Ptr_Glob->            ");
    printf ("  Ptr_Comp:       *    %p\n",  Ptr_Glob->Ptr_Comp);


    printf ("  Discr:       ");
    if (Ptr_Glob->Discr == 0) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d  ", Ptr_Glob->Discr);

    printf ("Enum_Comp:     ");
    if (Ptr_Glob->variant.var_1.Enum_Comp == 2) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d\n", Ptr_Glob->variant.var_1.Enum_Comp);

    printf ("  Int_Comp:    ");
    if (Ptr_Glob->variant.var_1.Int_Comp == 17) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d ", (int) Ptr_Glob->variant.var_1.Int_Comp);

    printf ("Str_Comp:      ");
    if (strcmp(Ptr_Glob->variant.var_1.Str_Comp,
               "DHRYSTONE PROGRAM, SOME STRING") == 0) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%s\n", Ptr_Glob->variant.var_1.Str_Comp);

    printf ("Next_Ptr_Glob->       ");
    printf ("  Ptr_Comp:       *    %p",  Next_Ptr_Glob->Ptr_Comp);
    printf (" same as above\n");

    printf ("  Discr:       ");
    if (Next_Ptr_Glob->Discr == 0) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d  ", Next_Ptr_Glob->Discr);

    printf ("Enum_Comp:     ");
    if (Next_Ptr_Glob->variant.var_1.Enum_Comp == 1) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d\n", Next_Ptr_Glob->variant.var_1.Enum_Comp);

    printf ("  Int_Comp:    ");
    if (Next_Ptr_Glob->variant.var_1.Int_Comp == 18) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d ", (int) Next_Ptr_Glob->variant.var_1.Int_Comp);

    printf ("Str_Comp:      ");
    if (strcmp(Next_Ptr_Glob->variant.var_1.Str_Comp,
               "DHRYSTONE PROGRAM, SOME STRING") == 0) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%s\n", Next_Ptr_Glob->variant.var_1.Str_Comp);

    printf ("Int_1_Loc:     ");
    if (Int_1_Loc == 5) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d  ", (int) Int_1_Loc);

    printf ("Int_2_Loc:     ");
    if (Int_2_Loc == 13) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d\n", (int) Int_2_Loc);

    printf ("Int_3_Loc:     ");
    if (Int_3_Loc == 7) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d  ", (int) Int_3_Loc);

    printf ("Enum_Loc:      ");
    if (Enum_Loc == 1) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%d  \n", Enum_Loc);


    printf ("Str_1_Loc:                             ");
    if (strcmp(Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING") == 0) {
        printf ("O.K.  ");
    }

    else {
        printf ("WRONG ");
    }
    printf ("%s\n", Str_1_Loc);

    printf ("Str_2_Loc:                             ");
    if (strcmp(Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING") == 0) {
        printf ("O.K.  ");
    } else {
        printf ("WRONG ");
    }
    printf ("%s\n", Str_2_Loc);


    printf ("\n");


    return 0;
}

void Proc_1 (REG Rec_Pointer Ptr_Val_Par)
/******************/

/* executed once */
{
    REG Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;
    /* == Ptr_Glob_Next */
    /* Local variable, initialized with Ptr_Val_Par->Ptr_Comp,    */
    /* corresponds to "rename" in Ada, "with" in Pascal           */

    structassign (*Ptr_Val_Par->Ptr_Comp, *Ptr_Glob);
    Ptr_Val_Par->variant.var_1.Int_Comp = 5;
    Next_Record->variant.var_1.Int_Comp
        = Ptr_Val_Par->variant.var_1.Int_Comp;
    Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
    Proc_3 (&Next_Record->Ptr_Comp);
    /* Ptr_Val_Par->Ptr_Comp->Ptr_Comp
       == Ptr_Glob->Ptr_Comp */
    if (Next_Record->Discr == Ident_1)
        /* then, executed */
    {
        Next_Record->variant.var_1.Int_Comp = 6;
        Proc_6 (Ptr_Val_Par->variant.var_1.Enum_Comp,
                &Next_Record->variant.var_1.Enum_Comp);
        Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
        Proc_7 (Next_Record->variant.var_1.Int_Comp, 10,
                &Next_Record->variant.var_1.Int_Comp);
    } else { /* not executed */
        structassign (*Ptr_Val_Par, *Ptr_Val_Par->Ptr_Comp);
    }
} /* Proc_1 */


void Proc_2 (One_Fifty *Int_Par_Ref)
/******************/
/* executed once */
/* *Int_Par_Ref == 1, becomes 4 */

{
    One_Fifty  Int_Loc;
    Enumeration   Enum_Loc;

    Int_Loc = *Int_Par_Ref + 10;
    do /* executed once */
        if (Ch_1_Glob == 'A')
            /* then, executed */
        {
            Int_Loc -= 1;
            *Int_Par_Ref = Int_Loc - Int_Glob;
            Enum_Loc = Ident_1;
        } /* if */
    while (Enum_Loc != Ident_1); /* true */
} /* Proc_2 */


void Proc_3 (Rec_Pointer *Ptr_Ref_Par)
/******************/
/* executed once */
/* Ptr_Ref_Par becomes Ptr_Glob */

{
    if (Ptr_Glob != Null)
        /* then, executed */
    {
        *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
    }
    Proc_7 (10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
} /* Proc_3 */


void Proc_4 () /* without parameters */
/*******/
/* executed once */
{
    Boolean Bool_Loc;
    FITIN_MONITOR_BOOL(Bool_Loc);

    Bool_Loc = Ch_1_Glob == 'A';
    /* Changed following line from "Bool_Glob = Bool_Loc | Bool_Glob;" */
    Bool_Glob = Bool_Loc && Bool_Glob;
    Ch_2_Glob = 'B';

    FITIN_UNMONITOR_BOOL(Bool_Loc);
} /* Proc_4 */


void Proc_5 () /* without parameters */
/*******/
/* executed once */
{
    Ch_1_Glob = 'A';
    Bool_Glob = true;
} /* Proc_5 */


/* Procedure for the assignment of structures,          */
/* if the C compiler doesn't support this feature       */
#ifdef  NOSTRUCTASSIGN
memcpy (d, s, l)
register Char   *d;
register Char   *s;
register Int    l;
{
    while (l--) {
        *d++ = *s++;
    }
}
#endif

#if FLIPSAFE
void sihft::fault_detected() throw() {
    exit(1);
};
#endif
