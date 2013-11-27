#ifndef __H_FITIN_LUA_VG
#define __H_FITIN_LUA_VG

#include <locale.h>
#include <time.h>

#include "pub_tool_libcbase.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcproc.h"
#include "pub_tool_libcsetjmp.h"
#include "pub_tool_mallocfree.h"

extern int lua_print(const char*, ...);
#define sprintf(b,f,args...) VG_(sprintf)((HChar*)b,(HChar*)f,args)


#define free(p) VG_(free)(p)
#define realloc(p,n) VG_(realloc)((HChar*) "fitin.lua", p, n);

extern void* vg_memchr(void*, int, size_t);
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
#define time(n) (time_t)VG_(read_millisecond_timer)
#define clock() (clock_t)VG_(read_millisecond_timer)

extern int* __errno_location(void);

#define exit(c) VG_(exit)(c)
#define getenv(s) VG_(getenv)((HChar*)s)
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

extern int vg_islower(int);
#define islower(n) vg_islower(n)
extern int vg_isupper(int);
#define isupper(n) vg_isupper(n)
extern int vg_isalpha(int);
#define isalpha(n) vg_isalpha(n)
extern int vg_iscntrl(int);
#define iscntrl(n) vg_iscntrl(n)
extern int vg_isdigit(int);
#define isdigit(n) vg_isdigit(n)
extern int vg_isalnum(int);
#define isalnum(n) vg_isalnum(n)
extern int vg_isspace(int);
#define isspace(n) vg_isspace(n)
extern int vg_isgraph(int);
#define isgraph(n) vg_isgraph(n)
extern int vg_ispunct(int);
#define ispunct(n) vg_ispunct(n)
extern int vg_isxdigit(int);
#define isxdigit(n) vg_isxdigit(n)
extern int vg_toupper(int);
#define toupper(n) vg_toupper(n)
extern int vg_tolower(int);
#define tolower(n) vg_tolower(n)

#endif
