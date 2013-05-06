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

/** This test won't flip anything for the actual logic, but at least
 *  it ensure the register tracking feature is working and that
 *  FITIn-reg can deal with smaller loads, as Valgrind generates
 *  according code for some reason.
 *
 *  UPDATE: After changes to size-aware loads, this test won't show 
 *  the effect of register tunnling anymore! Just keep it for testing
 *  this case.
 */
int main() {
    char a = 1, b = 0, c = 1;
    FITIN_MONITOR_VARIABLE(a);

    asm("movzxb %0, %%eax"::"m"(a):"%eax");
    asm("inc %eax");
    asm("movl %%eax, %0":"=m"(b));
    c += b;

    printf("%d\n", b);

    return 0;
}
