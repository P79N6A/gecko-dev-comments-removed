


























#ifndef __DLFCN_H__
#define __DLFCN_H__

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct {
    const char *dli_fname;  

    void       *dli_fbase;  

    const char *dli_sname;  

    void       *dli_saddr;  

} Dl_info;

extern void*        dlopen(const char*  filename, int flag);
extern int          dlclose(void*  handle);
extern const char*  dlerror(void);
extern void*        dlsym(void*  handle, const char*  symbol);
extern int          dladdr(void* addr, Dl_info *info);

enum {
  RTLD_NOW  = 0,
  RTLD_LAZY = 1,

  RTLD_LOCAL  = 0,
  RTLD_GLOBAL = 2,
};

#define RTLD_DEFAULT  ((void*) 0xffffffff)
#define RTLD_NEXT     ((void*) 0xfffffffe)

__END_DECLS

#endif 


