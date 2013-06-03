# Copyright (C) 2012 Gustav Munkby, Hamburg University of Technology (TUHH)
# Get root dir... assumes that the file was sourced from a
# script inside the source tree (this file should be in root)
root=$(readlink -f ${BASH_SOURCE:-$0})
while [ \! -f "$root/cmds.sh" ]; do
  root=${root%/*}
done
redo-ifchange "$root/cmds.sh"

function c++() {
  errfile=/tmp/does.err.$$
  depfile=/tmp/does.dep.$$
  # Annoyingly, when the output is redirected to a file, gcc resets the dependencies and only stores
  # the dependencies for the last file on the command line. Thus, we inject cat on the command line to
  # get all the dependencies. Unfortunately, we thereby loose the return code, and to we work around
  # that by writing to $errfile. In bash we could have used PIPESTATUS, but that isn't very portable.
  (g++ -I"$root/include" -I. -Wall -Werror -Wextra -MT x -MD -MF /dev/stdout "$@"; echo $? > $errfile) | cat > $depfile
  read result < $errfile
  sed 's,^x: *,,;s,\\$,,' < $depfile | sort -u | xargs redo-ifchange
  rm -- $depfile $errfile 2> /dev/null
  return $result
}

