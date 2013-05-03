CC = "gcc"
CFLAGS = "-O0"
LDFLAGS = ""

TEST_BIN = "bin"
TEST_BUILT_FILE = "built"
TEST_DATA = "tests.json"

VALGRIND = "../../bin/valgrind"
VG_FLAGS = "--tool=fitin"

BENCHMARK_RUNS = 10
BENCHMARK_DHRY_VG = "--mod-bit=3 --mod-load-time=10"
