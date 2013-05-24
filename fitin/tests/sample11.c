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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include "../../include/valgrind/fi_client.h"

int main() {
    char *buf = "0123456";
    char buf2[100] = { 0 };
    int fd1 = open("./fd1", O_RDWR|O_CREAT|O_TRUNC, 0666);
    int fd2 = open("./fd2", O_RDWR|O_CREAT|O_TRUNC, 0666);
    int sf_size = 3;

    write(fd1, buf, 7);
    lseek(fd1, 0, SEEK_SET);

    /* This will change the reigster shadow table, but obviously
       won't cause any impact on the actual call - or just something
       deeper down the sys call...
       
       So this is only visible (only if your compiler really
       preserves the distance 0x4) in trace-flags mode with
       additional debugging output. */
    FITIN_MONITOR_MEMORY(((char*)&sf_size)-0x4, 4);
    sendfile(fd2, fd1, NULL, sf_size);
    FITIN_UNMONITOR_MEMORY(((char*)&sf_size)-0x4, 4);

    lseek(fd2, 0, SEEK_SET);
    read(fd2, buf2, 7);
    printf("%s\n", buf2);

    close(fd1); close(fd2);

    return 0;
}
