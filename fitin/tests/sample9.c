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
    unsigned int a = 0;
    FITIN_MONITOR_VARIABLE(a);

    /* This will only load a partial of a. */
    unsigned char *pa;
    pa = (unsigned char*)&a; 

    /* If --modBit is > 7, this should cause a secondary flip here. */
    *pa = *pa + 1;
    
    /* So in fact, there will be different output here.*/
    printf("%u\n", a);

    return 0;
}
