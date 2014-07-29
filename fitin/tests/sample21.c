/*--------------------------------------------------------------------*/
/*--- FITIn Test Suite                                                */
/*--------------------------------------------------------------------*/

/*
   This file is part of the FITIn test suite.

   Copyright (C) 2014 Marcel Heing-Becker <marcel.heing@tu-harburg.de>

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
    int val = 63;
    FITIN_MONITOR_VARIABLE(val);
    FITIN_BREAKPOINT5(1, 4, 3, 0, -1);
    FITIN_BREAKPOINT4(2, 4, 3, 0);
    FITIN_BREAKPOINT3(3, 4, 3);
    FITIN_BREAKPOINT2(4, 4);
    FITIN_BREAKPOINT1(5);
    FITIN_BREAKPOINT;
    int val2 = val + 1;

    printf("%d\n", val2);
    return 0;
}
