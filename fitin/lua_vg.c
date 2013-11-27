#include <limits.h>
#include <lua_vg.h>

#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_mallocfree.h"
#include "pub_tool_options.h"

static struct lconv constant_lconv = {
    (char*) ".",
    (char*) "",
    (char*) "",
    (char*) "",
    (char*) "",
    (char*) "",
    (char*) "",
    (char*) "",
    (char*) "",
    (char*) "",
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX,
    CHAR_MAX
};

static const char *constant_lconv_name = "C";

static const char *generic_error_str = "Generic Error";
static const int generic_error_code = 1;

static int rseed = 1;
static int rcalls = 0;

extern int* __errno_location(void) {
    return &generic_error_code;
}

extern int lua_print(const char *format, ...) {
    va_list args;

    if(VG_(clo_verbosity) > 1) {
        va_start(args, format);
        VG_(vprintf)(format, args);
        va_end(args);
    }
}

/* isX(n) is only ASCII aware. */
extern int vg_islower(int n) {
    return (n >= 0x61 && n <= 0x7A);
}

extern int vg_isupper(int n) {
    return (n >= 0x41 && n <= 0x5A);
}

extern int vg_isalpha(int n) {
    return vg_islower(n) || vg_isupper(n);
}

extern int vg_iscntrl(int n) {
    return (n >= 0 && n <= 0x1F) || n == 0x7F;
}

extern int vg_isdigit(int n) {
    return (n >= 0x30 && n <= 0x39);
}

extern int vg_isalnum(int n) {
    return vg_isdigit(n) || vg_isalpha(n);
}

extern int vg_isspace(int n) {
    return n == 0x20;
}

extern int vg_isgraph(int n) {
    return !vg_iscntrl(n) && !vg_isspace(n);
}

extern int vg_ispunct(int n) {
    return vg_isgraph(n) && !vg_isalnum(n);
}

extern int vg_isxdigit(int n) {
    return vg_isdigit(n) || (n >= 0x41 && n <= 0x46);
}

extern int vg_toupper(int n) {
    if(vg_isalpha(n) && vg_islower(n)) {
        return n - 0x20;
    } else {
        return n;
    }
}

extern int vg_tolower(int n) {
    if(vg_isalpha(n) && vg_isupper(n)) {
        return n + 0x20;
    } else {
        return n;
    }
}

static int random(int seed) {
  /* Taken from GNU C Library, stdlib/rand_r.c, LGPLv2.1+ */
  unsigned int next = rseed;
  int result;

  next *= 1103515245;
  next += 12345;
  result = (unsigned int) (next / 65536) % 2048;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (unsigned int) (next / 65536) % 1024;

  rseed = next;

  return result;
}

extern struct lconv* vg_localeconv(void) {
    return &constant_lconv;
}

extern void* vg_memchr(void *p, int value, size_t num) {
    unsigned char c = (unsigned char) value;
    int i = 0;
    unsigned char *base = (unsigned char*) p;

    /* Maybe not the fastest, but ok for the moment. */
    for(; i < num; ++i) {
        if(*(base+i) == c) {
            return (void*)(base + i);
        }
    }

    return NULL;
}

extern double vg_pow(double b, double e) {
    int inte = (int)e, i = 1;
    double result = b;

    if(inte == 1) {
        return b;
    } else if(inte == 0) {
        return 1;
    } else if(b == 2.0d && inte > 0) {
        return 1 << inte;
    } else if(e > 0) {
        for(; i <= inte; ++i) {
            result *= b;
        }
    } else {
        for(; i <= inte; ++i) {
            result /= b;
        }
    }

    return result;
}

extern int vg_rand(void) {
    return random(rseed);
}

extern char* vg_setlocale(int category, const char* name) {
    return &constant_lconv_name;
}

extern void vg_srand(unsigned int n) {
    rseed = n;
}

extern char* vg_strerror(int num) {
    return generic_error_str;
}

static unsigned int tmp_files = 0;
static char *path_sep = "/";

extern char* vg_tmpnam(char *path) {
    HChar *dir = VG_(tmpdir)();
    HChar buf[12];

    if(dir != NULL) {
        VG_(memset)(buf, 0, sizeof(buf));
        VG_(sprintf)(buf, "%u", tmp_files++);

        size_t dir_length = VG_(strlen)(dir);
        size_t name_length = VG_(strlen)(dir);
        size_t total_length = dir_length + name_length + 1;

        dir = VG_(realloc)("fiti.lua.temppath", dir, dir_length + name_length + 2);
        VG_(memcpy)(dir + dir_length, path_sep, 1);
        VG_(memcpy)(dir + dir_length + 1, buf, name_length);
        VG_(memcpy)(dir + total_length, "\0", 1);

        if(path != NULL) {
            VG_(memcpy)(path, dir, total_length);
            VG_(free)(dir);
            return path;
        } else {
            return (char*)dir;
        }
    } else {
        return NULL;
    }
}

static const char* strftime_warning = "Time not supported.";

extern size_t vg_strftime(void *ptr, size_t size, char *format, ...) {
    size_t warning_length = VG_(strlen)(strftime_warning) + 1;

    if(warning_length <= size) {
        VG_(memcpy)(ptr, strftime_warning, warning_length);
        return warning_length;
    } else {
        VG_(memcpy)(ptr, strftime_warning, size - 1);
        VG_(memcpy)(ptr + size - 1, "\0", 1);
        return size;
    }
}
