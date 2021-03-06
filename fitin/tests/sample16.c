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

/* This should be monitored as often as this function is called. */

/* Returning floats always seem to involve a float-register but not
   necessarily (never?) the same that holds the result. On AMD64 it's XMM0,
   on x86 ST(0). So we have two loads here. */
float just_do_it() {
    float a = 1.25;
    FITIN_MONITOR_VARIABLE(a);
    a = a + 1;

    return a;
}

int main() {
    int i = 0;
    float result = 0;

    for(; i < 10; ++i) {
        result = just_do_it();
    }

    printf("%.2f\n", result);

    return 0;
}
