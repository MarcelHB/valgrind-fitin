CC = "gcc"
CFLAGS = "-O0 -m32"
LDFLAGS = "-m32"

TEST_BIN = "bin"
TEST_BUILT_FILE = "built"
TEST_DATA = "tests_lua.json"

VALGRIND = "../../bin/valgrind"
VG_FLAGS = "--tool=fitin"

BENCHMARK_RUNS = 10
BENCHMARK_DHRY_VG = "--control-script=$PWD/benchmark.lua"
