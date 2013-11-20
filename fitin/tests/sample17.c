/*--------------------------------------------------------------------*/
/*--- FITIn Test Suite                                                */
/*--------------------------------------------------------------------*/

/*
   This file is part of the FITIn test suite.

   Copyright (C) 2013 Marcel Heing-Becker <marcel.heing@tu-harburg.de>

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
#include <stdio.h>

#include "../../include/valgrind/fi_client.h"

int main() {
    float a = -1.5, b = 1.5, c = 0;
    FITIN_MONITOR_VARIABLE(a);

    /* As x86 uses stack FPU registers, we have to use SSE2 to test
       floats equivalently to sampl112. XMMx is not visible there. */

    /* TODO: Check where a/b are located in XMMx and adjust test. */

    asm("movss %0, %%xmm0"::"m"(a):"xmm0");
    asm("movss %0, %%xmm1"::"m"(b):"xmm1");
    asm("addss %xmm0, %xmm1");
    asm("movss %%xmm1, %0":"=m"(b));
    asm("mulss %xmm0, %xmm1");
    asm("movss %%xmm1, %0":"=m"(b));
    asm("subss %xmm0, %xmm1");
    asm("movss %%xmm1, %0":"=m"(b));
    asm("movss %%xmm0, %0":"=m"(c));

    printf("%.2f\n", c);
    printf("%.2f\n", b);
    return 0;
}
