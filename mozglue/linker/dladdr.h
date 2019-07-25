




































#include <dlfcn.h>

#ifndef HAVE_DLADDR
typedef struct {
  const char *dli_fname;
  void *dli_fbase;
  const char *dli_sname;
  void *dli_saddr;
} Dl_info;
extern int dladdr(void *addr, Dl_info *info) __attribute__((weak));
#define HAVE_DLADDR 1
#endif
