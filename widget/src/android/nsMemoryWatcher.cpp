




































#include "nsMemoryWatcher.h"
#include "nsComponentManagerUtils.h"
#include "android/log.h"
#include "nsString.h"
#include "nsAppShell.h"
#include "nsIPropertyBag2.h"
#include "nsIServiceManager.h"
#include "nsXULAppAPI.h"

NS_IMPL_ISUPPORTS1(nsMemoryWatcher, nsITimerCallback)







#define DEFAULT_TIMER_INTERVAL 2000
#define DEFAULT_LOW_MEMORY_MARK 5000
#define DEFAULT_HIGH_MEMORY_MARK 10000

nsMemoryWatcher::nsMemoryWatcher()
  : mLowWaterMark(DEFAULT_LOW_MEMORY_MARK)
  , mHighWaterMark(DEFAULT_HIGH_MEMORY_MARK)
  , mLastLowNotification(0)
  , mLastHighNotification(0)
  , mMemInfoFile(nsnull)
{
}

nsMemoryWatcher::~nsMemoryWatcher()
{
  if (mTimer)
    StopWatching();
}

void
nsMemoryWatcher::StartWatching()
{
  if (mTimer)
    return;

  
  if (XRE_GetProcessType() != GeckoProcessType_Default)
      return;

  
  nsCOMPtr<nsIPropertyBag2> sysInfo = do_GetService("@mozilla.org/system-info;1");
  if (sysInfo) {
      nsCString deviceType;
      nsresult rv = sysInfo->GetPropertyAsACString(NS_LITERAL_STRING("device"),
                                                       deviceType);
      if (NS_SUCCEEDED(rv)) {
          if (! deviceType.EqualsLiteral("Nexus S"))
              return;
      }
  }

  __android_log_print(ANDROID_LOG_WARN, "Gecko",
                      "!!!!!!!!! Watching Memory....");


  mMemInfoFile = fopen("/proc/meminfo", "r");
  NS_ASSERTION(mMemInfoFile, "Could not open /proc/meminfo for reading.");

  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  NS_ASSERTION(mTimer, "Creating of a timer failed.");

  mTimer->InitWithCallback(this, DEFAULT_TIMER_INTERVAL, nsITimer::TYPE_REPEATING_SLACK);
}

void
nsMemoryWatcher::StopWatching()
{
  if (!mTimer)
    return;

  mTimer->Cancel();
  mTimer = nsnull;

  fclose(mMemInfoFile);
  mMemInfoFile = nsnull;
}

NS_IMETHODIMP
nsMemoryWatcher::Notify(nsITimer *aTimer)
{
  NS_ASSERTION(mMemInfoFile, "File* to /proc/meminfo is null");

  rewind(mMemInfoFile);

  long memFree = -1;
  char line[256];

  while (fgets(line, 256, mMemInfoFile)) {
      sscanf(line, "MemFree: %ld kB", &memFree);
  }
  NS_ASSERTION(memFree > 0, "Free memory should be greater than zero");

  if (memFree < mLowWaterMark) {
      __android_log_print(ANDROID_LOG_WARN, "Gecko",
                          "!!!!!!!!! Reached criticial memory level. MemFree = %ld",
                          memFree);

      if (PR_IntervalToSeconds(PR_IntervalNow() - mLastLowNotification) > 5) {
          nsAppShell::gAppShell->NotifyObservers(nsnull,
                                                 "memory-pressure",
                                                 NS_LITERAL_STRING("oom-kill").get());
          mLastLowNotification = PR_IntervalNow();
      }
      return NS_OK;
  }
  
  if (memFree < mHighWaterMark) {
      __android_log_print(ANDROID_LOG_WARN, "Gecko",
                          "!!!!!!!!! Reached low memory level. MemFree = %ld",
                          memFree);
      if (PR_IntervalToSeconds(PR_IntervalNow() - mLastHighNotification) > 5) {
          nsAppShell::gAppShell->NotifyObservers(nsnull,
                                                 "memory-pressure",
                                                 NS_LITERAL_STRING("low-memory").get());
          mLastHighNotification = PR_IntervalNow();
      }
      
      
      aTimer->SetDelay(DEFAULT_TIMER_INTERVAL / 10);
      return NS_OK;
  }
  
  
  aTimer->SetDelay(DEFAULT_TIMER_INTERVAL);
  return NS_OK;
}
