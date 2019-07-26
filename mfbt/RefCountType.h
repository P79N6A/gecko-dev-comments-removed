





#ifndef mozilla_RefCountType_h
#define mozilla_RefCountType_h

#include <stdint.h>











typedef uintptr_t MozRefCountType;








#ifdef XP_WIN
typedef unsigned long MozExternalRefCountType;
#else
typedef uint32_t MozExternalRefCountType;
#endif

#endif
