


























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

#pragma GCC visibility push(default)
extern void* moz_mapped_dlopen(const char*  filename, int flag,
                               int fd, void *mem, unsigned int len, unsigned int offset);
extern void*        __wrap_dlopen(const char*  filename, int flag);
extern int          __wrap_dlclose(void*  handle);
extern const char*  __wrap_dlerror(void);
extern void*        __wrap_dlsym(void*  handle, const char*  symbol);
extern int          __wrap_dladdr(void* addr, Dl_info *info);
#pragma GCC visibility pop

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


