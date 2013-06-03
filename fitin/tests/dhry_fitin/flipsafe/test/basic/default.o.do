# Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
source ../../cmds.sh
test -d build || mkdir build 
s=${1#build/}
case $s in
  noprotect-*)
    s=${s#noprotect-}
    cppflags=-DPROTECTION_DISABLED
    ;;
esac
c++ --std=c++0x -c ${s%-O?}.cc -o $3 -O${s##*-O} -Dbasic=${s//-/_} $cppflags -fdump-tree-all
