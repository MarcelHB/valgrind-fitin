# FITIn - A simple fault injection tool

## About

FITIn is a tool to be used by Valgrind that allows flipping a bit of
data at a certain time.

## Accesses and Flips

In order to understand when and how FITIn is doing bit flips, please
read the following information carefully.

In the source code, you specify which start addresses you want
to monitor inside a selected function (default: ```main```).

Whenever FITIn detects a read from a memory beginning at a monitored
start address, a counter is increased - even if the data has been loaded
into a register.

Next to some other customizable constraints, you can use this counter
to schedule bit-flips, performed by the Lua-callback ```flip_value```.

FITIn will also treat reads of memory and registers by syscalls.

To get a better feeling for 'accesses', think of your C code while it
is not optimized and spot any reading from a monitored variable.

Things that do not count as access (with ```a``` being monitored):

* Copies: ```b = a;```, the copy is one access to ```a``` but any access
  to ```b``` is out of scope.
* Identity assignments: ```a=a;```
* Anonymous expressions: ```(a + b) * c```, again there is only one
  access before the addition.
* Irregular access: ```((char*)&a) + b)```, as this is not touching the
  start address at execution time (but ```a``` is wide enough, e.g. by
  ```int a;```).

## Fortran

Please have a look at ```fortran/README```.

## Use

FITIn recommends annotating source code in order to work conveniently.

### Code

Please include ```fi_client.h``` from the Valgrind include-directory.
For setting up monitoring of access to memory, use one of the following
macros:

* ```FITIN_MONITOR_VARIABLE(var)```
* ```FITIN_MONITOR_MEMORY(addr, size)```
* ```FITIN_BREAKPOINT```
* ```FITIN_BREAKPOINTi(a1,..,ai)``` (integer messages)

Then compile the source code. Deactivating any optimization levels is
strongly recommended, as optimazations by the compiler may render this
tool useless.

### Command line

Start FITIn by specifying ```--tool=fitin``` followed by options and the
program to run.

Control over FITIn has been moved to Lua scripts completely. You need to
provide the path to a Lua script by ```--control-script=```. Please read
the file ```sample.lua``` for a documentation of callbacks and
limitations of the Lua-runtime.

For additional options of Valgrind and FITIn, please consult
```--tool=fitin --help```.

By increasing the verbosity Level of Valgrind, you will also receive
additional information about FITIn.

### Examples, Tests

Have a look at the ```tests``` folder. Please note that the tests are not
built by the ordinary ```make``` command and require Ruby if used by the
test suite. 

For more information, please consult the ```README``` inside.

## Limitations

* If accessing data from memory, the tool focuses on matching start
  addresses being monitored. For uses of different alignments, use
  ```FITIN_MONITOR_MEMORY``` instead of ```FITIN_MONITOR_VARIABLE```
  for every byte that may be a start address.
* Limited support for rotating register files: not respected for IRDirty
  helpers.

## Reporting of Bugs, Feature Requests, Support

Please post anything like that to
https://github.com/MarcelHB/valgrind-fitin/issues

