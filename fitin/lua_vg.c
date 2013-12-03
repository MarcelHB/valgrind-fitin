#include <limits.h>
#include <lua_vg.h>
#include <fcntl.h>

#include "pub_tool_libcassert.h"
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

extern int* __errno_location(void) {
    return (int*)&generic_error_code;
}

extern void lua_print(const char *format, ...) {
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

extern int vg_isalnum(int n) {
    return VG_(isdigit)(n) || vg_isalpha(n);
}

extern int vg_isgraph(int n) {
    return !vg_iscntrl(n) && !VG_(isspace)(n);
}

extern int vg_ispunct(int n) {
    return vg_isgraph(n) && !vg_isalnum(n);
}

extern int vg_isxdigit(int n) {
    return VG_(isdigit)(n) || (n >= 0x41 && n <= 0x46);
}

extern int vg_toupper(int n) {
    if(vg_isalpha(n) && vg_islower(n)) {
        return n - 0x20;
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

extern double vg_difftime(time_t end, time_t begin) {
    /* As we only have relative ms values, we can be as simple as that. */
    return (double)(end - begin);
}

extern const void* vg_memchr(const void *p, int value, size_t num) {
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

extern struct tm* vg_gmtime(const time_t *t) {
    struct tm *_tm = (struct tm*)VG_(calloc)("fitin.lua.gmtime", 1, sizeof(struct tm));
    time_t *as_time_t = (time_t*)_tm;
    *as_time_t = *t;
    
    return _tm;
}

extern struct tm* vg_localtime(time_t t) {
    struct tm *_tm = (struct tm*)VG_(calloc)("fitin.lua.localtime", 1, sizeof(struct tm));
    time_t *as_time_t = (time_t*)_tm;
    *as_time_t = t;

    return _tm;
}

extern time_t vg_mktime(struct tm *t) {
    return *((time_t*)t);
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
    return (char*)&constant_lconv_name;
}

extern void vg_srand(unsigned int n) {
    rseed = n;
}

extern char* vg_strerror(int num) {
    return (char*)generic_error_str;
}

static unsigned int tmp_files = 0;
static const char *path_sep = "/";

extern char* vg_tmpnam(char *path) {
    HChar *dir = (HChar*)VG_(tmpdir)();
    HChar buf[12];

    if(dir != NULL) {
        VG_(memset)(buf, 0, sizeof(buf));
        VG_(sprintf)(buf, "%u", tmp_files++);

        size_t dir_length = VG_(strlen)(dir);
        size_t name_length = VG_(strlen)(buf);
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

extern FILE* vg_tmpfile(void) {
    char buf[1024];
    VG_(memset)(buf, 0, sizeof(buf));
    vg_tmpnam((char*)buf);
    return vg_fopen(buf, "wb+");
}

extern size_t vg_strftime(void *ptr, size_t size, char *format, struct tm *_tm) {
    HChar buf[50];
    VG_(memset)(buf, 0, sizeof(buf));
    VG_(sprintf)(buf, "%u\n", (unsigned int) *((time_t*)_tm));
    size_t val_length = VG_(strlen)(buf) + 1;
    
    if(val_length <= size) {
        VG_(strcpy)((HChar*)ptr, (HChar*)buf);
        return val_length - 1;
    } else {
        return 0;
    }
}

extern time_t vg_time(time_t *t) {
    time_t rtime = (time_t)VG_(read_millisecond_timer)();
    if(t != NULL) {
        *t = rtime;
    }

    return rtime;
}

void vg_fopen_solve_flag_bits(vg_FILE*, const char*);
void vg_fopen_solve_flag_bits(vg_FILE* vgf, const char *mode) {
    UChar bits = 0;
    UChar *ptr = (UChar*)mode;

    while(*ptr != 0) {
        switch(*(ptr++)) {
            case 'r':
                bits |= 9; /* For read, must exist. */
                break;
            case 'w':
                bits |= 2; /* For write. */
                break;
            case 'a':
                bits |= 6; /* For write, append. */
                break;
            case '+':
                switch(bits) {
                    case 2:
                        bits |= 1; /* For read/write. */
                        break;
                    case 6:
                        bits |= 17; /* For read/write, append, a+-mode. */
                        break;
                    case 9:
                        bits |= 2; /* For read/write, must exist. */
                        break;
                }
                break;
            default:
                break;
        }
    }

    vgf->mode_bits = bits;
}

Int vg_fopen_translate_flag_bits(UChar);
Int vg_fopen_translate_flag_bits(UChar fbits) {
    Int sysc_bits = 0;

    if((fbits & 3) == 3) {
       sysc_bits |= O_RDWR; 
    } else if(fbits & 2) {
       sysc_bits |= O_WRONLY;
    } else if(fbits & 1) {
       sysc_bits |= O_RDONLY;
    }

    if(fbits & 4) {
        sysc_bits |= O_APPEND;
    } else if((fbits & 8) == 0) {
        sysc_bits |= O_CREAT;
    }

    return sysc_bits;
}

Long vg_fgetsize(const char*);
Long vg_fgetsize(const char *path) {
    struct vg_stat vgs;
    SysRes res = VG_(stat)(path, &vgs);

#ifdef VGO_linux
    if(res._isError) {
        return -1;
    }
#endif

    return vgs.size;
}

extern FILE* vg_fopen(const char* path, const char *mode) {
    vg_FILE *vgf = VG_(malloc)("fitin.lua.fopen", sizeof(vg_FILE));
    vg_fopen_solve_flag_bits(vgf, mode);
    Int flag_bits = vg_fopen_translate_flag_bits(vgf->mode_bits);

    vgf->fd = VG_(fd_open)(path, flag_bits, 0644);
    vgf->size = vg_fgetsize(path);
    vgf->state_bits = 0;

    if(vgf->mode_bits & 4) {
        vgf->pos = vgf->size;
    } else {
        vgf->pos = 0;
    }

    if(vgf->fd == -1 || vgf->size == -1) {
        VG_(free)(vgf);
        return NULL;
    }

    return (FILE*)vgf;
}

extern int vg_fclose(FILE *f) {
    vg_FILE *vgf = (vg_FILE*)f;
    VG_(close)(vgf->fd);
    VG_(free)(vgf);
    return 0;
}

extern int vg_feof(FILE* f) {
    vg_FILE *vgf = (vg_FILE*)f;
    return vgf->state_bits & 1;
}

extern int vg_ferror(FILE* f) {
    vg_FILE *vgf = (vg_FILE*)f;
    return vgf->state_bits & 2;
}

void vg_maybe_set_eof(vg_FILE*);
void vg_maybe_set_eof(vg_FILE *vgf) {
    if(vgf->pos > vgf->size) {
        vgf->state_bits |= 1;
    }
}

extern int vg_fflush(FILE *f) {
    return 0;
}

Int vg_allowed_to_read(vg_FILE*);
Int vg_allowed_to_read(vg_FILE* vgf) {
    return vgf->mode_bits & 1;
}

Int vg_allowed_to_write(vg_FILE*);
Int vg_allowed_to_write(vg_FILE* vgf) {
    return vgf->mode_bits & 2;
}

void vg_set_error(vg_FILE*);
void vg_set_error(vg_FILE *vgf) {
    vgf->state_bits |= 2;
}

extern char* vg_fgets(char *str, int num, FILE *f) {
    vg_FILE *vgf = (vg_FILE*)f;
    Int i = 0;
    Int length = num - 1;

    if(!vg_allowed_to_read(vgf)) {
        vg_set_error(vgf);
        return NULL;
    }

    if(vg_feof(f)) {
        return NULL;
    }

    if(length <= 0) {
        return str;
    }

    ULong future_pos = vgf->pos + length;
    tl_assert(future_pos >= vgf->pos);
    tl_assert(future_pos >= length);

    VG_(memset)(str, 0, num);
    UChar was_r = 0;

    /* Do this inline, should save many instructions. */
    for(; i < length; ++i) {
        UChar buf;

        Int res = VG_(read)(vgf->fd, &buf, 1);
        if(res == -1) {
            vg_set_error(vgf);
            break;
        }

        vg_maybe_set_eof(vgf);
        vgf->append_pos = vgf->pos++;

        if(buf == 0xD) {
            was_r = 1;
        } else if(buf == 0xA) {
            break;
        } else {
            if(was_r) {
                str[i-1] = '\r';
                was_r = 0;
            }
            str[i] = buf;
        }

        if(vg_feof(f)) {
            break;
        }
    }

    return str;
}

/* We know that Lua uses this only once for LUA_NUMBER_FMT = %.14g */
extern int vg_fprintf(FILE *f, const char *format, ...) {
    va_list args;
    char buf[512]; /* Should be sufficient for Lua's case. */
    va_start(args, format);
    VG_(vsprintf)((HChar*)&buf, format, args);
    va_end(args);

    return vg_fwrite(&buf, VG_(strlen)((HChar*)&buf), 1, f);
}

extern int vg_fread(void *ptr, size_t size, size_t count, FILE* f) {
    vg_FILE *vgf = (vg_FILE*)f;

    if(!vg_allowed_to_read(vgf)) {
        vg_set_error(vgf);
        return 0;
    }

    if(size == 0 || count == 0 || vg_feof(f)) {
        return 0;
    }

    ULong total = size * count;
    tl_assert(total >= size);
    tl_assert(total >= count);
    ULong future_pos = vgf->pos + total;
    tl_assert(future_pos >= vgf->pos);
    tl_assert(future_pos >= total);

    Int res = VG_(read)(vgf->fd, ptr, total);
    if(res == -1) {
        vg_set_error(vgf);
    }
    vgf->pos = vgf->append_pos = future_pos;

    vg_maybe_set_eof(vgf);

    return res;
}

extern FILE* vg_freopen(const char *path, const char *mode, FILE* f) {
    vg_FILE *vgf = (vg_FILE*)f;
    vg_fopen_solve_flag_bits(vgf, mode);
    Int flag_bits = vg_fopen_translate_flag_bits(vgf->mode_bits);

    vgf->fd = VG_(fd_open)(path, flag_bits, 0644);
    vgf->size = vg_fgetsize(path);
    vgf->state_bits = 0;

    if(vgf->mode_bits & 4) {
        vgf->pos = vgf->append_pos = vgf->size;
    } else {
        vgf->pos = vgf->append_pos = 0;
    }

    if(vgf->fd == -1 || vgf->size == -1) {
        VG_(free)(vgf);
        return NULL;
    }

    return (FILE*)vgf;
}

extern int vg_fscanf(FILE *f, const char *format, ...) {
    va_list args;
    vg_FILE *vgf = (vg_FILE*)f;

    /* Buffer everything left. */
    Long left = vgf->size - vgf->append_pos;
    if(left > 512) {
        left = 512;
    }
    HChar *fbuffer = VG_(calloc)("fitin.lua.fscanf", left, 1);

    /* We should only have this case in Lua. */
    if(VG_(strcmp)(format, "%lf") == 0) {
        vg_fread(fbuffer, left, 1, f);
        double dbl = VG_(strtod)(fbuffer, NULL);
        va_start(args, format);
        double *addr = va_arg(args, double*);
        *addr = dbl;
        va_end(args);
    } else {
        VG_(free)(fbuffer);
        return 0;
    }

    VG_(free)(fbuffer);
    return 1;
}

extern int vg_fseek(FILE *f, long int offset, int origin) {
    vg_FILE *vgf = (vg_FILE*)f;
    vgf->pos = VG_(lseek)(vgf->fd, offset, origin);
    return 0;
}

extern long int vg_ftell(FILE *f) {
    vg_FILE *vgf = (vg_FILE*)f;
    return vgf->pos;
}

extern size_t vg_fwrite(void *ptr, size_t size, size_t count, FILE *f) {
    vg_FILE *vgf = (vg_FILE*)f;

    if(!vg_allowed_to_write(vgf)) {
        vg_set_error(vgf);
        return 0;
    }

    if(size == 0 || count == 0) {
        return 0;
    }

    if(vgf->mode_bits & 16) {
        vgf->pos = vgf->append_pos;
    }

    ULong total = size * count;
    tl_assert(total >= size);
    tl_assert(total >= count);
    ULong future_pos = vgf->pos + total;
    tl_assert(future_pos >= vgf->pos);
    tl_assert(future_pos >= total);

    Int res = VG_(write)(vgf->fd, ptr, total);

    if(res != total) {
        vg_set_error(vgf);
    }
    vgf->pos = future_pos;
    Long new_size = vgf->size + total;
    tl_assert(new_size >= vgf->size);
    vgf->size = new_size;

    return res;
}

extern int vg_getc(FILE* f) {
    vg_FILE *vgf = (vg_FILE*)f;
    Int result = 0;

    vg_fread(&result, 1, 1, f);
    if(vg_feof(f) || vgf->state_bits & 2) {
        return EOF;
    } else {
        return result;
    }
}

extern int vg_ungetc(int c, FILE *f) {
    vg_FILE *vgf = (vg_FILE*)f;
    vgf->pos--;
    vg_maybe_set_eof(vgf);
    return c;
}


