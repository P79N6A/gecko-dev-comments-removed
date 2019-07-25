






































#ifndef ANDROID_STUB_H
#define ANDROID_STUB_H

#include "dlfcn.h"
#ifdef ANDROID_VERSION
#if ANDROID_VERSION < 8


typedef struct {
  char *dli_fname;
} Dl_info;

#define dladdr(foo, bar) 0
#endif
#endif



#define _SYS_SYSINFO_H_

#include <sys/cdefs.h>
#include <linux/kernel.h>

#define sysinfo(foo) -1

#endif 
