





#include "mozilla/AvailableMemoryTracker.h"
#include "nsThread.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsWindowsDllInterceptor.h"
#include "prinrval.h"
#include "pratom.h"
#include "prenv.h"
#include "nsIMemoryReporter.h"
#include "nsPrintfCString.h"
#include <windows.h>

using namespace mozilla;

namespace {







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
PRUint32 sLowCommitSpaceThreshold = 0;
PRUint32 sLowPhysicalMemoryThreshold = 0;
PRUint32 sLowMemoryNotificationIntervalMS = 0;

PRUint32 sNumLowVirtualMemEvents = 0;
PRUint32 sNumLowCommitSpaceEvents = 0;
PRUint32 sNumLowPhysicalMemEvents = 0;

WindowsDllInterceptor sKernel32Intercept;
WindowsDllInterceptor sGdi32Intercept;


bool sInitialized = false;


bool sHooksActive = false;



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





bool MaybeScheduleMemoryPressureEvent()
{
  
  
  PRIntervalTime interval = PR_IntervalNow() - sLastLowMemoryNotificationTime;
  if (sHasScheduledOneLowMemoryNotification &&
      PR_IntervalToMilliseconds(interval) < sLowMemoryNotificationIntervalMS) {

    LOG("Not scheduling low physical memory notification, "
        "because not enough time has elapsed since last one.");
    return false;
  }

  
  
  
  
  
  
  sHasScheduledOneLowMemoryNotification = true;
  sLastLowMemoryNotificationTime = PR_IntervalNow();

  LOG("Scheduling memory pressure notification.");
  ScheduleMemoryPressureEvent();
  return true;
}

void CheckMemAvailable()
{
  if (!sHooksActive) {
    return;
  }

  MEMORYSTATUSEX stat;
  stat.dwLength = sizeof(stat);
  bool success = GlobalMemoryStatusEx(&stat);

  DEBUG_WARN_IF_FALSE(success, "GlobalMemoryStatusEx failed.");

  if (success)
  {
    
    if (stat.ullAvailVirtual < sLowVirtualMemoryThreshold * 1024 * 1024) {
      
      
      
      LOG("Detected low virtual memory.");
      PR_ATOMIC_INCREMENT(&sNumLowVirtualMemEvents);
      ScheduleMemoryPressureEvent();
    }
    else if (stat.ullAvailPageFile < sLowCommitSpaceThreshold * 1024 * 1024) {
      LOG("Detected low available page file space.");
      if (MaybeScheduleMemoryPressureEvent()) {
        PR_ATOMIC_INCREMENT(&sNumLowCommitSpaceEvents);
      }
    }
    else if (stat.ullAvailPhys < sLowPhysicalMemoryThreshold * 1024 * 1024) {
      LOG("Detected low physical memory.");
      if (MaybeScheduleMemoryPressureEvent()) {
        PR_ATOMIC_INCREMENT(&sNumLowPhysicalMemEvents);
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
      (sLowPhysicalMemoryThreshold != 0 && aAllocationType & MEM_COMMIT)) {
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
  
  
  

  
  bool doCheck = false;
  if (sHooksActive && !aSection && aBitmapInfo) {
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
      doCheck = true;
    }
  }

  HBITMAP result = sCreateDIBSectionOrig(aDC, aBitmapInfo, aUsage, aBits,
                                         aSection, aOffset);

  if (doCheck) {
    CheckMemAvailable();
  }

  return result;
}

class NumLowMemoryEventsReporter : public nsIMemoryReporter
{
  NS_IMETHOD GetProcess(nsACString &aProcess)
  {
    aProcess.Truncate();
    return NS_OK;
  }

  NS_IMETHOD GetKind(int *aKind)
  {
    *aKind = KIND_OTHER;
    return NS_OK;
  }

  NS_IMETHOD GetUnits(int *aUnits)
  {
    *aUnits = UNITS_COUNT_CUMULATIVE;
    return NS_OK;
  }
};

class NumLowVirtualMemoryEventsMemoryReporter : public NumLowMemoryEventsReporter
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetPath(nsACString &aPath)
  {
    aPath.AssignLiteral("low-memory-events/virtual");
    return NS_OK;
  }

  NS_IMETHOD GetAmount(PRInt64 *aAmount)
  {
    
    
    MOZ_ASSERT(sizeof(void*) == 4);

    *aAmount = sNumLowVirtualMemEvents;
    return NS_OK;
  }

  NS_IMETHOD GetDescription(nsACString &aDescription)
  {
    aDescription.AssignLiteral(
      "Number of low-virtual-memory events fired since startup. ");

    if (sLowVirtualMemoryThreshold == 0) {
      aDescription.AppendLiteral(
        "Tracking low-virtual-memory events is disabled, but you can enable it "
        "by giving the memory.low_virtual_mem_threshold_mb pref a non-zero "
        "value.");
    }
    else {
      aDescription.Append(nsPrintfCString(
        "We fire such an event if we notice there is less than %d MB of virtual "
        "address space available (controlled by the "
        "'memory.low_virtual_mem_threshold_mb' pref).  We'll likely crash if "
        "we run out of virtual address space, so this event is somewhat dire.",
        sLowVirtualMemoryThreshold));
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(NumLowVirtualMemoryEventsMemoryReporter, nsIMemoryReporter)

class NumLowCommitSpaceEventsMemoryReporter : public NumLowMemoryEventsReporter
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetPath(nsACString &aPath)
  {
    aPath.AssignLiteral("low-commit-space-events");
    return NS_OK;
  }

  NS_IMETHOD GetAmount(PRInt64 *aAmount)
  {
    *aAmount = sNumLowCommitSpaceEvents;
    return NS_OK;
  }

  NS_IMETHOD GetDescription(nsACString &aDescription)
  {
    aDescription.AssignLiteral(
      "Number of low-commit-space events fired since startup. ");

    if (sLowCommitSpaceThreshold == 0) {
      aDescription.Append(
        "Tracking low-commit-space events is disabled, but you can enable it "
        "by giving the memory.low_commit_space_threshold_mb pref a non-zero "
        "value.");
    }
    else {
      aDescription.Append(nsPrintfCString(
        "We fire such an event if we notice there is less than %d MB of "
        "available commit space (controlled by the "
        "'memory.low_commit_space_threshold_mb' pref).  Windows will likely "
        "kill us if we run out of commit space, so this event is somewhat dire.",
        sLowCommitSpaceThreshold));
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(NumLowCommitSpaceEventsMemoryReporter, nsIMemoryReporter)

class NumLowPhysicalMemoryEventsMemoryReporter : public NumLowMemoryEventsReporter
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD GetPath(nsACString &aPath)
  {
    aPath.AssignLiteral("low-memory-events/physical");
    return NS_OK;
  }

  NS_IMETHOD GetAmount(PRInt64 *aAmount)
  {
    *aAmount = sNumLowPhysicalMemEvents;
    return NS_OK;
  }

  NS_IMETHOD GetDescription(nsACString &aDescription)
  {
    aDescription.AssignLiteral(
      "Number of low-physical-memory events fired since startup. ");

    if (sLowPhysicalMemoryThreshold == 0) {
      aDescription.Append(
        "Tracking low-physical-memory events is disabled, but you can enable it "
        "by giving the memory.low_physical_memory_threshold_mb pref a non-zero "
        "value.");
    }
    else {
      aDescription.Append(nsPrintfCString(
        "We fire such an event if we notice there is less than %d MB of "
        "available physical memory (controlled by the "
        "'memory.low_physical_memory_threshold_mb' pref).  The machine will start "
        "to page if it runs out of physical memory; this may cause it to run "
        "slowly, but it shouldn't cause us to crash.",
        sLowPhysicalMemoryThreshold));
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(NumLowPhysicalMemoryEventsMemoryReporter, nsIMemoryReporter)

} 

namespace mozilla {
namespace AvailableMemoryTracker {

void Activate()
{
#if defined(_M_IX86)
  MOZ_ASSERT(sInitialized);
  MOZ_ASSERT(!sHooksActive);

  
  
  if (sizeof(void*) > 4) {
    sLowVirtualMemoryThreshold = 0;
  }
  else {
    Preferences::AddUintVarCache(&sLowVirtualMemoryThreshold,
        "memory.low_virtual_mem_threshold_mb", 128);
  }

  Preferences::AddUintVarCache(&sLowPhysicalMemoryThreshold,
      "memory.low_physical_memory_threshold_mb", 0);
  Preferences::AddUintVarCache(&sLowCommitSpaceThreshold,
      "memory.low_commit_space_threshold_mb", 128);
  Preferences::AddUintVarCache(&sLowMemoryNotificationIntervalMS,
      "memory.low_memory_notification_interval_ms", 10000);

  NS_RegisterMemoryReporter(new NumLowCommitSpaceEventsMemoryReporter());
  NS_RegisterMemoryReporter(new NumLowPhysicalMemoryEventsMemoryReporter());
  if (sizeof(void*) == 4) {
    NS_RegisterMemoryReporter(new NumLowVirtualMemoryEventsMemoryReporter());
  }
  sHooksActive = true;
#endif
}

void Init()
{
  
  
  
  
  
  
  
  

#if defined(_M_IX86)
  
  
  
  
  if (!PR_GetEnv("MOZ_PGO_INSTRUMENTED")) {
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

  sInitialized = true;
#endif
}

} 
} 
