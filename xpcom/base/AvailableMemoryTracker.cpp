





#include "mozilla/AvailableMemoryTracker.h"

#if defined(XP_WIN)
#include "prinrval.h"
#include "prenv.h"
#include "nsIMemoryReporter.h"
#include "nsMemoryPressure.h"
#endif

#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIRunnable.h"
#include "nsISupports.h"
#include "nsThreadUtils.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"

#if defined(XP_WIN)
#   include "nsWindowsDllInterceptor.h"
#   include <windows.h>
#endif

#if defined(MOZ_MEMORY)
#   include "mozmemory.h"
#endif  

using namespace mozilla;

namespace {

#if defined(XP_WIN)







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

void safe_write(uint64_t x)
{
  
  const unsigned int max_len = 21;
  char buf[max_len];
  buf[max_len - 1] = '\0';

  uint32_t i;
  for (i = max_len - 2; i < max_len && x > 0; i--)
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

uint32_t sLowVirtualMemoryThreshold = 0;
uint32_t sLowCommitSpaceThreshold = 0;
uint32_t sLowPhysicalMemoryThreshold = 0;
uint32_t sLowMemoryNotificationIntervalMS = 0;

Atomic<uint32_t> sNumLowVirtualMemEvents;
Atomic<uint32_t> sNumLowCommitSpaceEvents;
Atomic<uint32_t> sNumLowPhysicalMemEvents;

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
  NS_DispatchEventualMemoryPressure(MemPressure_New);
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
      ++sNumLowVirtualMemEvents;
      NS_DispatchEventualMemoryPressure(MemPressure_New);
    }
    else if (stat.ullAvailPageFile < sLowCommitSpaceThreshold * 1024 * 1024) {
      LOG("Detected low available page file space.");
      if (MaybeScheduleMemoryPressureEvent()) {
        ++sNumLowCommitSpaceEvents;
      }
    }
    else if (stat.ullAvailPhys < sLowPhysicalMemoryThreshold * 1024 * 1024) {
      LOG("Detected low physical memory.");
      if (MaybeScheduleMemoryPressureEvent()) {
        ++sNumLowPhysicalMemEvents;
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
    uint16_t bitCount = aBitmapInfo->bmiHeader.biBitCount;
    if (bitCount == 0) {
      
      
      
      bitCount = 32;
    }

    
    
    
    int64_t size = bitCount * aBitmapInfo->bmiHeader.biWidth *
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

static int64_t
LowMemoryEventsVirtualDistinguishedAmount()
{
  return sNumLowVirtualMemEvents;
}

class LowMemoryEventsVirtualReporter MOZ_FINAL : public MemoryUniReporter
{
public:
  LowMemoryEventsVirtualReporter()
    : MemoryUniReporter("low-memory-events/virtual",
                         KIND_OTHER, UNITS_COUNT_CUMULATIVE,
"Number of low-virtual-memory events fired since startup. We fire such an "
"event if we notice there is less than memory.low_virtual_mem_threshold_mb of "
"virtual address space available (if zero, this behavior is disabled).  The "
"process will probably crash if it runs out of virtual address space, so "
"this event is dire.")
  {}

private:
  int64_t Amount() MOZ_OVERRIDE
  {
    
    
    MOZ_ASSERT(sizeof(void*) == 4);

    return LowMemoryEventsVirtualDistinguishedAmount();
  }
};

class LowCommitSpaceEventsReporter MOZ_FINAL : public MemoryUniReporter
{
public:
  LowCommitSpaceEventsReporter()
    : MemoryUniReporter("low-commit-space-events",
                         KIND_OTHER, UNITS_COUNT_CUMULATIVE,
"Number of low-commit-space events fired since startup. We fire such an "
"event if we notice there is less than memory.low_commit_space_threshold_mb of "
"commit space available (if zero, this behavior is disabled).  Windows will "
"likely kill the process if it runs out of commit space, so this event is "
"dire.")
  {}

private:
  int64_t Amount() MOZ_OVERRIDE { return sNumLowCommitSpaceEvents; }
};

static int64_t
LowMemoryEventsPhysicalDistinguishedAmount()
{
  return sNumLowPhysicalMemEvents;
}

class LowMemoryEventsPhysicalReporter MOZ_FINAL : public MemoryUniReporter
{
public:
  LowMemoryEventsPhysicalReporter()
    : MemoryUniReporter("low-memory-events/physical",
                         KIND_OTHER, UNITS_COUNT_CUMULATIVE,
"Number of low-physical-memory events fired since startup. We fire such an "
"event if we notice there is less than memory.low_physical_memory_threshold_mb "
"of physical memory available (if zero, this behavior is disabled).  The "
"machine will start to page if it runs out of physical memory.  This may "
"cause it to run slowly, but it shouldn't cause it to crash.")
  {}

private:
  int64_t Amount() MOZ_OVERRIDE { return LowMemoryEventsPhysicalDistinguishedAmount(); }
};

#endif 







class nsJemallocFreeDirtyPagesRunnable MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
};

NS_IMPL_ISUPPORTS1(nsJemallocFreeDirtyPagesRunnable, nsIRunnable)

NS_IMETHODIMP
nsJemallocFreeDirtyPagesRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

#if defined(MOZ_MEMORY)
  jemalloc_free_dirty_pages();
#endif

  return NS_OK;
}






class nsMemoryPressureWatcher MOZ_FINAL : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  void Init();

private:
  static bool sFreeDirtyPages;
};

NS_IMPL_ISUPPORTS1(nsMemoryPressureWatcher, nsIObserver)

bool nsMemoryPressureWatcher::sFreeDirtyPages = false;






void
nsMemoryPressureWatcher::Init()
{
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();

  if (os) {
    os->AddObserver(this, "memory-pressure",  false);
  }

  Preferences::AddBoolVarCache(&sFreeDirtyPages, "memory.free_dirty_pages",
                               false);
}





NS_IMETHODIMP
nsMemoryPressureWatcher::Observe(nsISupports *subject, const char *topic,
                                 const PRUnichar *data)
{
  MOZ_ASSERT(!strcmp(topic, "memory-pressure"), "Unknown topic");

  if (sFreeDirtyPages) {
    nsRefPtr<nsIRunnable> runnable = new nsJemallocFreeDirtyPagesRunnable();

    NS_DispatchToMainThread(runnable);
  }

  return NS_OK;
}

} 

namespace mozilla {
namespace AvailableMemoryTracker {

void Activate()
{
#if defined(_M_IX86) && defined(XP_WIN)
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

  NS_RegisterMemoryReporter(new LowCommitSpaceEventsReporter());
  NS_RegisterMemoryReporter(new LowMemoryEventsPhysicalReporter());
  if (sizeof(void*) == 4) {
    NS_RegisterMemoryReporter(new LowMemoryEventsVirtualReporter());
  }
  RegisterLowMemoryEventsVirtualDistinguishedAmount(LowMemoryEventsVirtualDistinguishedAmount);
  RegisterLowMemoryEventsPhysicalDistinguishedAmount(LowMemoryEventsPhysicalDistinguishedAmount);
  sHooksActive = true;
#endif

  
  nsRefPtr<nsMemoryPressureWatcher> watcher = new nsMemoryPressureWatcher();
  watcher->Init();
}

void Init()
{
  
  
  
  
  
  
  
  

#if defined(_M_IX86) && defined(XP_WIN)
  
  
  
  
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
