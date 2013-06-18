# FITIn examples and tests

## About

Here, you can see tests that have been used for evaluating and
developing this solution.

Not every platform and configuration will have all tests passing. Some
are arch- or OS-specific, some compilers generate very strange code that
will simply not work this way for FITIn.

## Configuration

It is important to set `-m32` or `-m64` in the `CFLAGS` and `LDFLAGS`
of `rake.config.rb`, depending on your Valgrind build. Otherwise,
Valgrind may complain about missing binaries (exit code 1).

## Run

Please install an official Ruby v1.9+ release in order to run the tests
automatically (http://www.ruby-lang.org/en/).

Once installed, please execute ```gem install rake```. Now, the
```rake``` command should be available in your shell.

For older Ruby (v1.8) versions, you need to install JSON as well:
```gem install json```

Execute ```rake TASK``` with TASK being one of the following commands:

* ```tests```: Builds and executes the tests.
* ```clean```: Removes binary test files.
* ```benchmark_dhry```: Execution time measurements of Dhrystone.
* ```benchmark_dhry_flops```: Dhrystone's own benchmark rating.
* ```benchmark_linpack```: Execution time measurements of Linpack.
* ```benchmark_linpack_flops```: Dhrystone's own benchmark rating (MFLOPS).

With ```tests``` being the default task. Benchmarks can be compiled on
Linux only!

Test runs and options are specified by ```tests.json```, other
parameters (paths, benchmark runs) are located inside of ```rake.config.rb```.

I noticed a crash if trying to run ```dhry_fitin``` under ```-m64``` and
```--include=$PWD/dhry_fitin```. This is caused by instructions
resulting from Valgrind recompilation at some point (```Proc_8```?) and
so far the reason is unknown but does not occur if omitting
instrumentation of ```fi_reg_flip_or_leave_before_store```. Nonetheless,
attempting to isolate this problem makes it disappear. This needs some
research. For now, ```-m32``` is always enforced here.

## benchmark\_dhry, benchmark\_linpack

This builds and executes a Dhrystone/Linpack implementation not customized
by FITIn. There will be three steps with multiple runs:

* dhry: No valgrind.
* ```--tool=none```: Runs dhry with Valgrind's 'none' tool.
* ```--tool=fitin```: Runs dhry with this FITIn binary.

At the end of each step, the script will print the average execution
time as seen from a launching shell.

Although the benchmark is not very precise as it includes a little noise
from Ruby and does not subtract Valgrind's launch time, it gives a
little impression about the overall slowdown of FITIn.

## Copyright

Please consult ```LICENSE.md``` as not all samples are subject of the
GNU Public License.
