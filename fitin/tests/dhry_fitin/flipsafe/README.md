
= FlipSafe

== DESCRIPTION:

C++ type library implementation of various hardware fault
tolerance techniques.

== SYNOPSIS:

  #include "flipsafe/include/data.hh"

  sihft::dup<int> duplicated;

== REQUIREMENTS:

 - redo [http://github.com/apenwarr/redo] (tested 0.05)
 - gcc [http://gcc.gnu.org/] (tested 4.6.2)
 - boost [http://www.boost.org] (tested 1.48.0)

== BUILD:

Build and runt he tests:

  cd test
  redo

The build-scripts are very simple, so just
looking at `test/*.do` should describe how to
compile the tests.

== LICENSE:

(The MIT License)

Copyright (c) 2011 Gustav Munkby, Hamburg University of Technology (TUHH)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

