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

/* NOTE: Under MacOSX and GCC4.2, this test will segfault. */

int main() {
    unsigned int a = 1, ref = 0, flip = 0;
    FITIN_MONITOR_VARIABLE(a);

    /* Valgrind will create a dirty helper here to fake CPUID. */
    asm("movl $0, %eax");
    asm("cpuid");
    asm("movl %%eax, %0":"=m"(ref));

    asm("movl %0, %%eax"::"m"(a):"%eax");
    asm("cpuid");
    asm("movl %%eax, %0":"=m"(flip));

    /* If a has been flipped to 0, this should be '1'. */
    printf("%u\n", flip == ref);

    return 0;
}
