





#ifndef __FdPrintf_h__
#define __FdPrintf_h__







extern void FdPrintf(int aFd, const char* aFormat, ...)
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
;

#endif 
