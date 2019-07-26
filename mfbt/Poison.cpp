










#include "mozilla/Poison.h"

#include "mozilla/Assertions.h"
#ifdef _WIN32
# include <windows.h>
#elif !defined(__OS2__)
# include <unistd.h>
# include <sys/mman.h>
# ifndef MAP_ANON
#  ifdef MAP_ANONYMOUS
#   define MAP_ANON MAP_ANONYMOUS
#  else
#   error "Don't know how to get anonymous memory"
#  endif
# endif
#endif

extern "C" {
uintptr_t gMozillaPoisonValue;
uintptr_t gMozillaPoisonBase;
uintptr_t gMozillaPoisonSize;
}









#ifdef _WIN32
static void *
ReserveRegion(uintptr_t region, uintptr_t size)
{
  return VirtualAlloc((void *)region, size, MEM_RESERVE, PAGE_NOACCESS);
}

static void
ReleaseRegion(void *region, uintptr_t size)
{
  VirtualFree(region, size, MEM_RELEASE);
}

static bool
ProbeRegion(uintptr_t region, uintptr_t size)
{
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  if (region >= (uintptr_t)sinfo.lpMaximumApplicationAddress &&
      region + size >= (uintptr_t)sinfo.lpMaximumApplicationAddress) {
    return true;
  } else {
    return false;
  }
}

static uintptr_t
GetDesiredRegionSize()
{
  SYSTEM_INFO sinfo;
  GetSystemInfo(&sinfo);
  return sinfo.dwAllocationGranularity;
}

#define RESERVE_FAILED 0

#elif defined(__OS2__)
static void *
ReserveRegion(uintptr_t region, uintptr_t size)
{
  
  
  return (void*)0xFFFD0000;
}

static void
ReleaseRegion(void *region, uintptr_t size)
{
  return;
}

static bool
ProbeRegion(uintptr_t region, uintptr_t size)
{
  
  
  return false;
}

static uintptr_t
GetDesiredRegionSize()
{
  
  return 0x1000;
}

#define RESERVE_FAILED 0

#else 

static void *
ReserveRegion(uintptr_t region, uintptr_t size)
{
  return mmap(reinterpret_cast<void*>(region), size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
}

static void
ReleaseRegion(void *region, uintptr_t size)
{
  munmap(region, size);
}

static bool
ProbeRegion(uintptr_t region, uintptr_t size)
{
  if (madvise(reinterpret_cast<void*>(region), size, MADV_NORMAL)) {
    return true;
  } else {
    return false;
  }
}

static uintptr_t
GetDesiredRegionSize()
{
  return sysconf(_SC_PAGESIZE);
}

#define RESERVE_FAILED MAP_FAILED

#endif 

MOZ_STATIC_ASSERT(sizeof(uintptr_t) == 4 || sizeof(uintptr_t) == 8, "");
MOZ_STATIC_ASSERT(sizeof(uintptr_t) == sizeof(void *), "");

static uintptr_t
ReservePoisonArea(uintptr_t rgnsize)
{
  if (sizeof(uintptr_t) == 8) {
    
    
    
    return
      (((uintptr_t(0x7FFFFFFFu) << 31) << 1 | uintptr_t(0xF0DEAFFFu))
       & ~(rgnsize-1));

  } else {
    
    uintptr_t candidate = (0xF0DEAFFF & ~(rgnsize-1));
    void *result = ReserveRegion(candidate, rgnsize);
    if (result == (void *)candidate) {
      
      return candidate;
    }

    
    
    if (ProbeRegion(candidate, rgnsize)) {
      
      if (result != RESERVE_FAILED)
        ReleaseRegion(result, rgnsize);
      return candidate;
    }

    
    
    if (result != RESERVE_FAILED) {
      return uintptr_t(result);
    }

    
    
    result = ReserveRegion(0, rgnsize);
    if (result != RESERVE_FAILED) {
      return uintptr_t(result);
    }

    
    MOZ_CRASH();
    return 0;
  }
}

void
mozPoisonValueInit()
{
  gMozillaPoisonSize = GetDesiredRegionSize();
  gMozillaPoisonBase = ReservePoisonArea(gMozillaPoisonSize);

  if (gMozillaPoisonSize == 0) 
    return;

  gMozillaPoisonValue = gMozillaPoisonBase + gMozillaPoisonSize/2 - 1;
}
