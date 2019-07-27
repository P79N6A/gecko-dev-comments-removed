




#ifndef MOZ_PLATFORM_LINUX_LUL_H
#define MOZ_PLATFORM_LINUX_LUL_H




void
read_procmaps(lul::LUL* aLUL);


void
logging_sink_for_LUL(const char* str);



#if defined(__GLIBC__)
#include <unistd.h>
#include <sys/syscall.h>
static inline pid_t gettid()
{
  return (pid_t) syscall(SYS_gettid);
}
#endif


extern lul::LUL* sLUL;

#endif 
