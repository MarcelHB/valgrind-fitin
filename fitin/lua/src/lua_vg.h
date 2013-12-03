#ifndef __H_FITIN_LUA_VG
#define __H_FITIN_LUA_VG

#include <locale.h>
#include <time.h>
#include <stdio.h>

#include "pub_tool_libcbase.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_libcsetjmp.h"
#include "pub_tool_mallocfree.h"

extern void lua_print(const char*, ...);
#define sprintf(b,f,args...) VG_(sprintf)((HChar*)b,(HChar*)f,args)

#define free(p) VG_(free)(p)
#define realloc(p,n) VG_(realloc)((HChar*) "fitin.lua", p, n);

extern const void* vg_memchr(const void*, int, size_t);
#define memchr(p,v,n) vg_memchr(p,v,n)
#define memcmp(p1,p2,n) VG_(memcmp)(p1,p2,n)
#define memcpy(d,s,sz) VG_(memcpy)(d,s,sz)
#define strchr(s,c) VG_(strchr)(s,c)
#define strcmp(s1,s2) VG_(strcmp)((HChar*) s1, (HChar*) s2)
#define strcpy(d,s) VG_(strcpy)((HChar*) d, (HChar*) s)
#define strcoll(s1,s2) VG_(strcmp)((HChar*) s1, (HChar*) s2)
#define strlen(s) VG_(strlen)(s)
#define strpbrk(s,a) VG_(strpbrk)((HChar*) s, (HChar*) a)
#define strspn(s,a) VG_(strspn)((HChar*)s, (HChar*)a)
#define strstr(s1,s2) VG_(strstr)((HChar*) s1, (HChar*) s2)
#define strtod(c,e) VG_(strtod)((HChar*) c, (HChar**) e)
extern char* vg_strerror(int);
#define strerror(n) vg_strerror(n)

#define longjmp(e,v) VG_MINIMAL_LONGJMP((ULong*) e)
#define setjmp(e) VG_MINIMAL_SETJMP((ULong*) e)

extern struct lconv* vg_localeconv(void);
#define localeconv() vg_localeconv()
extern char* vg_setlocale(int, const char*);
#define setlocale(c,l) vg_setlocale(c,l)

/* All we can do ... */
#define clock() (clock_t)VG_(read_millisecond_timer)
extern double vg_difftime(time_t, time_t);
#define difftime(e,b) vg_difftime(e,b)
extern struct tm* vg_gmtime(const time_t*);
#define gmtime(t) vg_gmtime(t)
extern struct tm* vg_localtime(time_t t);
#define localtime(t) vg_localtime(t)
extern time_t vg_mktime(struct tm*);
#define mktime(tm) vg_mktime(tm)
extern size_t vg_strftime(void*, size_t, char*, struct tm*);
#define strftime(p,s,f,tm) vg_strftime(p,s,f,tm)
extern time_t vg_time(time_t*);
#define time(n) vg_time(n)

extern int* __errno_location(void);

#define exit(c) VG_(exit)(c)
#define getenv(s) VG_(getenv)((HChar*) s)
#define system(c) VG_(system)((HChar*) c)

#define abs(n) ((n < 0) ? -(n) : n)
#define floor(d) ((long long) d)
#define ceil(d) (floor(d)+1)
extern double vg_pow(double, double);
#define pow(b,e) vg_pow(b,e)
extern int vg_rand(void);
#define ldexp(r,e) (r * pow(2,e))
#define rand() vg_rand()
extern void vg_srand(unsigned int);
#define srand(n) vg_srand(n)

#define clearerr(f);
#define rename(f,t) VG_(rename)((HChar*)f,(HChar*)t)
#define remove(f) VG_(unlink)((HChar*)f)
extern char* vg_tmpnam(char*);
#define tmpnam(f) vg_tmpnam(f)
extern FILE* vg_tmpfile(void);
#define tmpfile() vg_tmpfile()
#define tmpfile64() vg_tmpfile()

extern int vg_islower(int);
#define islower(n) vg_islower(n)
extern int vg_isupper(int);
#define isupper(n) vg_isupper(n)
extern int vg_isalpha(int);
#define isalpha(n) vg_isalpha(n)
extern int vg_iscntrl(int);
#define iscntrl(n) vg_iscntrl(n)
#define isdigit(n) VG_(isdigit)(n)
extern int vg_isalnum(int);
#define isalnum(n) vg_isalnum(n)
#define isspace(n) VG_(isspace)(n)
extern int vg_isgraph(int);
#define isgraph(n) vg_isgraph(n)
extern int vg_ispunct(int);
#define ispunct(n) vg_ispunct(n)
extern int vg_isxdigit(int);
#define isxdigit(n) vg_isxdigit(n)
extern int vg_toupper(int);
#define toupper(n) vg_toupper(n)
#define tolower(n) VG_(tolower)(n)

typedef struct vg_FILE {
    Int fd;
    UChar state_bits; /* EOF | Error | ... */
    UChar mode_bits; /* read | write | append | exists | a+ */
    Long size;
    Long pos;
    Long append_pos;
} vg_FILE;

extern int vg_fclose(FILE*);
#define fclose(f) vg_fclose(f)
extern int vg_feof(FILE*);
#define feof(f) vg_feof(f)
extern int vg_ferror(FILE*);
#define ferror(f) vg_ferror(f)
extern int vg_fflush(FILE*);
#define fflush(f) vg_fflush(f)
extern char* vg_fgets(char*, int, FILE*);
#define fgets(v,n,f) vg_fgets(v,n,f)
extern FILE* vg_fopen(const char*, const char*);
#define fopen(f,m) vg_fopen(f,m)
#define fopen64(f,m) vg_fopen(f,m)
extern int vg_fprintf(FILE*, const char*, ...);
#define fprintf(f,ff,args...) vg_fprintf(f, ff, args)
extern int vg_fread(void*, size_t, size_t, FILE*);
#define fread(p,s,c,f) vg_fread(p,s,c,f)
extern FILE* vg_freopen(const char*, const char*, FILE*);
#define freopen(p,m,f) vg_freopen(p,m,f)
extern int vg_fscanf(FILE*, const char* f, ...);
#define fscanf(f,ff,args...) vg_fscanf(f,ff,args)
extern int vg_fseek(FILE*, long int, int);
#define fseek(f,p,o) vg_fseek(f,p,o)
extern long int vg_ftell(FILE*);
#define ftell(f) vg_ftell(f)
extern int vg_ungetc(int, FILE*);
#define ungetc(c,f) vg_ungetc(c,f)
extern size_t vg_fwrite(void*, size_t, size_t, FILE*);
#define fwrite(p,s,c,f) vg_fwrite(p,s,c,f)
extern int vg_getc(FILE*);
#define getc(f) vg_getc(f)

#endif
