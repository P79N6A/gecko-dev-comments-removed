






#ifndef ANDROID_STUB_H
#define ANDROID_STUB_H



#define _SYS_SYSINFO_H_

#include <sys/cdefs.h>
#include <linux/kernel.h>

#if ANDROID_VERSION >= 21
#include <sys/resource.h>
#include <unistd.h>

static int getdtablesize(void)
{
    struct rlimit r;
    if (getrlimit(RLIMIT_NOFILE, &r) < 0) {
        return sysconf(_SC_OPEN_MAX);
    }
    return r.rlim_cur;
}
#else
#define RTLD_NOLOAD 0
extern int getdtablesize(void);
#endif

#define sysinfo(foo) -1

#endif 
