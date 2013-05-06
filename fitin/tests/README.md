# FITIn examples and tests

## About

Here, you can see tests that have been used for evaluating and
developing this solution.

## Run

Please install an official Ruby v1.9+ release in order to run the tests
automatically (http://www.ruby-lang.org/en/).

Once installed, please execute ```gem install rake```. Now, the
```rake``` command should be available in your shell.

Execute ```rake TASK``` with TASK being one of the following commands:

* ```tests```: Builds and executes the tests.
* ```clean```: Removes binary test files.
* ```benchmark_dhry```: Builds and executes a tiny benchmark containing
  three samples.

With ```tests``` being the default task.

Test runs and options are specified by ```tests.json```, other
parameters (paths, benchmark runs) are located inside of ```rake.config.rb```.

## benchmark\_dhry

This builds and executes a Dhrystone implementation preserved from the
original FITIn used for demonstrational purposes. There will be three
steps with multiple runs:

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
