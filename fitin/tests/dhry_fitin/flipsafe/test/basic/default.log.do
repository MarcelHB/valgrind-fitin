# Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
redo-ifchange $1-O3.o
objdump -d $1-O3.o | grep '^ ' > $3
