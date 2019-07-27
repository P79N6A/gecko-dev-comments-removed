
















#include "nsTerminator.h"

#include "prthread.h"
#include "nsString.h"
#include "nsServiceManagerUtils.h"

#include "nsIObserverService.h"
#include "nsIPrefService.h"
#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#endif

#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"





#define FALLBACK_ASYNCSHUTDOWN_CRASH_AFTER_MS 60000



#define ADDITIONAL_WAIT_BEFORE_CRASH_MS 3000


#define TICK_DURATION 1000

namespace mozilla {

namespace {






Atomic<bool> gProgress(false);

struct Options {
  int32_t crashAfterMS;
};

void
Run(void* arg)
{
  PR_SetCurrentThreadName("Shutdown Hang Terminator");

  
  
  UniquePtr<Options> options((Options*)arg);
  int32_t crashAfterMS = options->crashAfterMS;
  options = nullptr;

  int32_t timeToLive = crashAfterMS;
  while (true) {
    
    
    
    
    
    
    
    
    
    
    PR_Sleep(TICK_DURATION);
    if (gProgress.exchange(false)) {
      
      
      timeToLive = crashAfterMS;
      continue;
    }
    timeToLive -= TICK_DURATION;
    if (timeToLive >= 0) {
      continue;
    }

    
    MOZ_CRASH("Shutdown too long, probably frozen, causing a crash.");
  }
}

} 

static char const *const sObserverTopics[] = {
  "quit-application",
  "profile-change-teardown",
  "profile-before-change",
  "xpcom-will-shutdown",
  "xpcom-shutdown",
};

NS_IMPL_ISUPPORTS(nsTerminator, nsIObserver)

nsTerminator::nsTerminator()
  : mInitialized(false)
{
}


nsresult
nsTerminator::SelfInit()
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os) {
    return NS_ERROR_UNEXPECTED;
  }

  for (size_t i = 0; i < ArrayLength(sObserverTopics); ++i) {
    DebugOnly<nsresult> rv = os->AddObserver(this, sObserverTopics[i], false);
#if defined(DEBUG)
    NS_WARN_IF(NS_FAILED(rv));
#endif 
  }
  return NS_OK;
}


void
nsTerminator::Start() {
  

  int32_t crashAfterMS =
    Preferences::GetInt("toolkit.asyncshutdown.crash_timeout",
                        FALLBACK_ASYNCSHUTDOWN_CRASH_AFTER_MS);

  
  
  crashAfterMS += ADDITIONAL_WAIT_BEFORE_CRASH_MS;

  UniquePtr<Options> options(new Options());
  options->crashAfterMS = crashAfterMS;

  
  
  DebugOnly<PRThread*> thread = PR_CreateThread(
    PR_SYSTEM_THREAD, 
    Run,
    options.release(),
    PR_PRIORITY_LOW,
    PR_GLOBAL_THREAD ,
    PR_UNJOINABLE_THREAD,
    0 
  );

  MOZ_ASSERT(thread);
  mInitialized = true;
}

NS_IMETHODIMP
nsTerminator::Observe(nsISupports *, const char *aTopic, const char16_t *)
{
  if (strcmp(aTopic, "profile-after-change") == 0) {
    return SelfInit();
  }

  

  
  
  
  if (!mInitialized) {
    Start();
  }

  
  gProgress.exchange(true);

#if defined(MOZ_CRASHREPORTER)
  
  unused << CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ShutdownProgress"),
                                               nsAutoCString(aTopic));
#endif 

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  MOZ_RELEASE_ASSERT(os);
  (void)os->RemoveObserver(this, aTopic);
  return NS_OK;
}

} 
