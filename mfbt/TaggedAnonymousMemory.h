






























#ifndef mozilla_TaggedAnonymousMemory_h
#define mozilla_TaggedAnonymousMemory_h

#ifndef XP_WIN

#include <sys/types.h>
#include <sys/mman.h>

#include "mozilla/Types.h"

#ifdef ANDROID

#ifdef __cplusplus
extern "C" {
#endif

MFBT_API void
MozTagAnonymousMemory(const void* aPtr, size_t aLength, const char* aTag);

MFBT_API void*
MozTaggedAnonymousMmap(void* aAddr, size_t aLength, int aProt, int aFlags,
                         int aFd, off_t aOffset, const char* aTag);

MFBT_API int
MozTaggedMemoryIsSupported(void);

#ifdef __cplusplus
} 
#endif

#else

static inline void
MozTagAnonymousMemory(const void* aPtr, size_t aLength, const char* aTag)
{
}

static inline void*
MozTaggedAnonymousMmap(void* aAddr, size_t aLength, int aProt, int aFlags,
                       int aFd, off_t aOffset, const char* aTag)
{
  return mmap(aAddr, aLength, aProt, aFlags, aFd, aOffset);
}

static inline int
MozTaggedMemoryIsSupported(void)
{
  return 0;
}

#endif

#endif

#endif
