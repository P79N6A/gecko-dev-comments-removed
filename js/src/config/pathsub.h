




































#ifndef pathsub_h___
#define pathsub_h___





#include <limits.h>
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif




#undef NAME_MAX
#define NAME_MAX 256

extern char *program;

extern void fail(char *format, ...);
extern char *getcomponent(char *path, char *name);
extern char *ino2name(ino_t ino, char *dir);
extern void *xmalloc(size_t size);
extern char *xstrdup(char *s);
extern char *xbasename(char *path);
extern void xchdir(char *dir);


extern int relatepaths(char *from, char *to, char *outpath);


extern void reversepath(char *inpath, char *name, int len, char *outpath);

#endif 
