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
#include <sys/time.h>

#include "../../include/valgrind/fi_client.h"

/* NOTE: This test does not produce a flip here under MacOSX. */

int main() {
    struct timeval tv, tv_copy;
    unsigned char byte, byte_copy, diff;
    FITIN_MONITOR_MEMORY(((unsigned char*) &tv)+7, 1);

    gettimeofday(&tv, NULL);
    tv_copy = tv;
    settimeofday(&tv, NULL);

    byte = *(((unsigned char*) &tv)+7); 
    byte_copy = *(((unsigned char*) &tv_copy)+7);
    /* There should be a difference of one bit somewhere. */
    diff = byte ^ byte_copy;

    printf("%u\n", diff);

    return 0;
}
