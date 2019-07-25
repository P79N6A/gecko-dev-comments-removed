


























#include "mozilla/AvailableMemoryTracker.h"
#include "nsThread.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsWindowsDllInterceptor.h"
#include "prinrval.h"
#include "pratom.h"
#include "prenv.h"
#include <windows.h>

namespace mozilla {
namespace AvailableMemoryTracker {







#ifdef LOGGING_ENABLED

#define LOG(msg)       \
  do {                 \
    safe_write(msg);   \
    safe_write("\n");  \
  } while(0)

#define LOG2(m1, m2)   \
  do {                 \
    safe_write(m1);    \
    safe_write(m2);    \
    safe_write("\n");  \
  } while(0)

#define LOG3(m1, m2, m3) \
  do {                   \
    safe_write(m1);      \
    safe_write(m2);      \
    safe_write(m3);      \
    safe_write("\n");    \
  } while(0)

#define LOG4(m1, m2, m3, m4) \
  do {                       \
    safe_write(m1);          \
    safe_write(m2);          \
    safe_write(m3);          \
    safe_write(m4);          \
    safe_write("\n");        \
  } while(0)

#else

#define LOG(msg)
#define LOG2(m1, m2)
#define LOG3(m1, m2, m3)
#define LOG4(m1, m2, m3, m4)

#endif

void safe_write(const char *a)
{
  
  fputs(a, stdout);
}

void safe_write(PRUint64 x)
{
  
  const int max_len = 21;
  char buf[max_len];
  buf[max_len - 1] = '\0';

  PRUint32 i;
  for (i = max_len - 2; i >= 0 && x > 0; i--)
  {
    buf[i] = "0123456789"[x % 10];
    x /= 10;
  }

  safe_write(&buf[i + 1]);
}

#ifdef DEBUG
#define DEBUG_WARN_IF_FALSE(cond, msg)          \
  do {                                          \
    if (!(cond)) {                              \
      safe_write(__FILE__);                     \
      safe_write(":");                          \
      safe_write(__LINE__);                     \
      safe_write(" ");                          \
      safe_write(msg);                          \
      safe_write("\n");                         \
    }                                           \
  } while(0)
#else
#define DEBUG_WARN_IF_FALSE(cond, msg)
#endif

PRUint32 sLowVirtualMemoryThreshold = 0;
PRUint32 sLowPhysicalMemoryThreshold = 0;
PRUint32 sLowPhysicalMemoryNotificationIntervalMS = 0;

WindowsDllInterceptor sKernel32Intercept;
WindowsDllInterceptor sGdi32Intercept;



volatile bool sHasScheduledOneLowMemoryNotification = false;
volatile PRIntervalTime sLastLowMemoryNotificationTime;



void* (WINAPI *sVirtualAllocOrig)
  (LPVOID aAddress, SIZE_T aSize, DWORD aAllocationType, DWORD aProtect);

void* (WINAPI *sMapViewOfFileOrig)
  (HANDLE aFileMappingObject, DWORD aDesiredAccess,
   DWORD aFileOffsetHigh, DWORD aFileOffsetLow,
   SIZE_T aNumBytesToMap);

HBITMAP (WINAPI *sCreateDIBSectionOrig)
  (HDC aDC, const BITMAPINFO *aBitmapInfo,
   UINT aUsage, VOID **aBits,
   HANDLE aSection, DWORD aOffset);

void CheckMemAvailable()
{
  MEMORYSTATUSEX stat;
  stat.dwLength = sizeof(stat);
  bool success = GlobalMemoryStatusEx(&stat);

  DEBUG_WARN_IF_FALSE(success, "GlobalMemoryStatusEx failed.");

  if (success)
  {
    
    if (stat.ullAvailVirtual < sLowVirtualMemoryThreshold * 1024 * 1024) {
      
      
      
      LOG("Detected low virtual memory.");
      ScheduleMemoryPressureEvent();
    }
    else if (stat.ullAvailPhys < sLowPhysicalMemoryThreshold * 1024 * 1024) {
      LOG("Detected low physical memory.");
      
      
      
      
      
      
      PRIntervalTime interval = PR_IntervalNow() - sLastLowMemoryNotificationTime;
      if (!sHasScheduledOneLowMemoryNotification ||
          PR_IntervalToMilliseconds(interval) >=
            sLowPhysicalMemoryNotificationIntervalMS) {

        
        
        
        
        
        
        sHasScheduledOneLowMemoryNotification = true;
        sLastLowMemoryNotificationTime = PR_IntervalNow();

        LOG("Scheduling memory pressure notification.");
        ScheduleMemoryPressureEvent();
      }
      else {
        LOG("Not scheduling low physical memory notification, "
            "because not enough time has elapsed since last one.");
      }
    }
  }
}

LPVOID WINAPI
VirtualAllocHook(LPVOID aAddress, SIZE_T aSize,
                 DWORD aAllocationType,
                 DWORD aProtect)
{
  
  
  
  
  
  
  
  
  
  
  

  LPVOID result = sVirtualAllocOrig(aAddress, aSize, aAllocationType, aProtect);

  
  
  
  if ((sLowVirtualMemoryThreshold != 0 && aAllocationType & MEM_RESERVE) ||
      (sLowPhysicalMemoryThreshold != 0 &&  aAllocationType & MEM_COMMIT)) {
    LOG3("VirtualAllocHook(size=", aSize, ")");
    CheckMemAvailable();
  }

  return result;
}

LPVOID WINAPI
MapViewOfFileHook(HANDLE aFileMappingObject,
                  DWORD aDesiredAccess,
                  DWORD aFileOffsetHigh,
                  DWORD aFileOffsetLow,
                  SIZE_T aNumBytesToMap)
{
  LPVOID result = sMapViewOfFileOrig(aFileMappingObject, aDesiredAccess,
                                     aFileOffsetHigh, aFileOffsetLow,
                                     aNumBytesToMap);
  LOG("MapViewOfFileHook");
  CheckMemAvailable();
  return result;
}

HBITMAP WINAPI
CreateDIBSectionHook(HDC aDC,
                     const BITMAPINFO *aBitmapInfo,
                     UINT aUsage,
                     VOID **aBits,
                     HANDLE aSection,
                     DWORD aOffset)
{
  
  
  

  
  PRBool doCheck = PR_FALSE;
  if (!aSection && aBitmapInfo) {
    PRUint16 bitCount = aBitmapInfo->bmiHeader.biBitCount;
    if (bitCount == 0) {
      
      
      
      bitCount = 32;
    }

    
    
    
    PRInt64 size = bitCount * aBitmapInfo->bmiHeader.biWidth *
                              aBitmapInfo->bmiHeader.biHeight;
    if (size < 0)
      size *= -1;

    
    
    if (size > 1024 * 1024 * 8) {
      LOG3("CreateDIBSectionHook: Large allocation (size=", size, ")");
      doCheck = PR_TRUE;
    }
  }

  HBITMAP result = sCreateDIBSectionOrig(aDC, aBitmapInfo, aUsage, aBits,
                                         aSection, aOffset);

  if (doCheck) {
    CheckMemAvailable();
  }

  return result;
}

void Init()
{
  
  
  if (sizeof(void*) > 4) {
    sLowVirtualMemoryThreshold = 0;
  }
  else {
    Preferences::AddUintVarCache(&sLowVirtualMemoryThreshold,
        "memory.low_virtual_mem_threshold_mb", 128);
  }

  Preferences::AddUintVarCache(&sLowPhysicalMemoryThreshold,
      "memory.low_physical_mem_threshold_mb", 0);
  Preferences::AddUintVarCache(&sLowPhysicalMemoryNotificationIntervalMS,
      "memory.low_physical_memory_notification_interval_ms", 10000);

  
  
  
  

  if (!PR_GetEnv("MOZ_PGO_INSTRUMENTED") &&
      (sLowVirtualMemoryThreshold != 0 || sLowPhysicalMemoryThreshold != 0)) {
    sKernel32Intercept.Init("Kernel32.dll");
    sKernel32Intercept.AddHook("VirtualAlloc",
      reinterpret_cast<intptr_t>(VirtualAllocHook),
      (void**) &sVirtualAllocOrig);
    sKernel32Intercept.AddHook("MapViewOfFile",
      reinterpret_cast<intptr_t>(MapViewOfFileHook),
      (void**) &sMapViewOfFileOrig);

    sGdi32Intercept.Init("Gdi32.dll");
    sGdi32Intercept.AddHook("CreateDIBSection",
      reinterpret_cast<intptr_t>(CreateDIBSectionHook),
      (void**) &sCreateDIBSectionOrig);
  }
}

} 
} 
