
















#include "nsTerminator.h"

#include "prthread.h"
#include "prmon.h"
#include "plstr.h"
#include "prio.h"

#include "nsString.h"
#include "nsServiceManagerUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"

#include "nsIObserverService.h"
#include "nsIPrefService.h"
#if defined(MOZ_CRASHREPORTER)
#include "nsExceptionHandler.h"
#endif

#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/MemoryChecking.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"
#include "mozilla/Telemetry.h"





#define FALLBACK_ASYNCSHUTDOWN_CRASH_AFTER_MS 60000



#define ADDITIONAL_WAIT_BEFORE_CRASH_MS 3000


#define TICK_DURATION 1000

namespace mozilla {

namespace {




PRThread* CreateSystemThread(void (*start)(void* arg),
                             void* arg)
{
  PRThread* thread = PR_CreateThread(
    PR_SYSTEM_THREAD, 
    start,
    arg,
    PR_PRIORITY_LOW,
    PR_GLOBAL_THREAD ,
    PR_UNJOINABLE_THREAD,
    0 
  );
  MOZ_LSAN_INTENTIONALLY_LEAK_OBJECT(thread); 
  return thread;
}




























Atomic<uint32_t> gHeartbeat(0);

struct Options {
  


  uint32_t crashAfterTicks;
};




void
RunWatchdog(void* arg)
{
  PR_SetCurrentThreadName("Shutdown Hang Terminator");

  
  
  UniquePtr<Options> options((Options*)arg);
  uint32_t crashAfterTicks = options->crashAfterTicks;
  options = nullptr;

  const uint32_t timeToLive = crashAfterTicks;
  while (true) {
    
    
    
    
    
    
    
    
    
    
    PR_Sleep(TICK_DURATION);

    if (gHeartbeat++ < timeToLive) {
      continue;
    }

    
    MOZ_CRASH("Shutdown too long, probably frozen, causing a crash.");
  }
}












class PR_CloseDelete
{
public:
  MOZ_CONSTEXPR PR_CloseDelete() {}

  PR_CloseDelete(const PR_CloseDelete& aOther)
  {}

  void operator()(PRFileDesc* aPtr) const
  {
    PR_Close(aPtr);
  }
};



























Atomic<nsCString*> gWriteData(nullptr);
PRMonitor* gWriteReady = nullptr;

void RunWriter(void* arg)
{
  PR_SetCurrentThreadName("Shutdown Statistics Writer");

  MOZ_LSAN_INTENTIONALLY_LEAK_OBJECT(arg);
  
  

  

  nsCString destinationPath(static_cast<char*>(arg));
  nsAutoCString tmpFilePath;
  tmpFilePath.Append(destinationPath);
  tmpFilePath.AppendLiteral(".tmp");

  
  unused << PR_Delete(tmpFilePath.get());
  unused << PR_Delete(destinationPath.get());

  while (true) {
    
    
    
    
    
    
    
    
    
    
    
    
    UniquePtr<nsCString> data(gWriteData.exchange(nullptr));
    if (!data) {
      
      
      PR_EnterMonitor(gWriteReady);
      PR_Wait(gWriteReady, PR_INTERVAL_NO_TIMEOUT);
      PR_ExitMonitor(gWriteReady);
      continue;
    }

    MOZ_LSAN_INTENTIONALLY_LEAK_OBJECT(data.get());
    
    

    
    
    
    
    
    
    
    UniquePtr<PRFileDesc, PR_CloseDelete>
      tmpFileDesc(PR_Open(tmpFilePath.get(),
                          PR_WRONLY | PR_TRUNCATE | PR_CREATE_FILE,
                          00600));

    
    
    MOZ_LSAN_INTENTIONALLY_LEAK_OBJECT(tmpFileDesc.get());

    if (tmpFileDesc == nullptr) {
      break;
    }
    if (PR_Write(tmpFileDesc.get(), data->get(), data->Length()) == -1) {
      break;
    }
    tmpFileDesc.reset();

    
    
    
    
    
    
    
    if (PR_Rename(tmpFilePath.get(), destinationPath.get()) != PR_SUCCESS) {
      break;
    }
  }
}








struct ShutdownStep
{
  char const* const mTopic;
  int mTicks;

  MOZ_CONSTEXPR ShutdownStep(const char *const topic)
    : mTopic(topic)
    , mTicks(-1)
  {}

};

static ShutdownStep sShutdownSteps[] = {
  ShutdownStep("quit-application"),
  ShutdownStep("profile-change-teardown"),
  ShutdownStep("profile-before-change"),
  ShutdownStep("xpcom-will-shutdown"),
  ShutdownStep("xpcom-shutdown"),
};

} 

NS_IMPL_ISUPPORTS(nsTerminator, nsIObserver)

nsTerminator::nsTerminator()
  : mInitialized(false)
  , mCurrentStep(-1)
{
}


nsresult
nsTerminator::SelfInit()
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (!os) {
    return NS_ERROR_UNEXPECTED;
  }

  for (size_t i = 0; i < ArrayLength(sShutdownSteps); ++i) {
    DebugOnly<nsresult> rv = os->AddObserver(this, sShutdownSteps[i].mTopic, false);
#if defined(DEBUG)
    NS_WARN_IF(NS_FAILED(rv));
#endif 
  }

  return NS_OK;
}


void
nsTerminator::Start()
{
  MOZ_ASSERT(!mInitialized);
  StartWatchdog();
  StartWriter();
  mInitialized = true;
}



void
nsTerminator::StartWatchdog()
{
  int32_t crashAfterMS =
    Preferences::GetInt("toolkit.asyncshutdown.crash_timeout",
                        FALLBACK_ASYNCSHUTDOWN_CRASH_AFTER_MS);
  
  if (crashAfterMS <= 0) {
    crashAfterMS = FALLBACK_ASYNCSHUTDOWN_CRASH_AFTER_MS;
  }

  
  
  if (crashAfterMS > INT32_MAX - ADDITIONAL_WAIT_BEFORE_CRASH_MS) {
    
    crashAfterMS = INT32_MAX;
  } else {
    crashAfterMS += ADDITIONAL_WAIT_BEFORE_CRASH_MS;
  }

  UniquePtr<Options> options(new Options());
  options->crashAfterTicks = crashAfterMS / TICK_DURATION;

  DebugOnly<PRThread*> watchdogThread = CreateSystemThread(RunWatchdog,
                                                options.release());
  MOZ_ASSERT(watchdogThread);
}




void
nsTerminator::StartWriter()
{

  if (!Telemetry::CanRecord()) {
    return;
  }
  nsCOMPtr<nsIFile> profLD;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                       getter_AddRefs(profLD));
  if (NS_FAILED(rv)) {
    return;
  }

  rv = profLD->Append(NS_LITERAL_STRING("ShutdownDuration.json"));
  if (NS_FAILED(rv)) {
    return;
  }

  nsAutoString path;
  rv = profLD->GetPath(path);
  if (NS_FAILED(rv)) {
    return;
  }

  gWriteReady = PR_NewMonitor();
  MOZ_LSAN_INTENTIONALLY_LEAK_OBJECT(gWriteReady); 
  PRThread* writerThread = CreateSystemThread(RunWriter,
                                              ToNewUTF8String(path));

  if (!writerThread) {
    return;
  }
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

  UpdateHeartbeat(aTopic);
  UpdateTelemetry();
  UpdateCrashReport(aTopic);

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  MOZ_RELEASE_ASSERT(os);
  (void)os->RemoveObserver(this, aTopic);

  return NS_OK;
}

void
nsTerminator::UpdateHeartbeat(const char* aTopic)
{
  
  uint32_t ticks = gHeartbeat.exchange(0);
  if (mCurrentStep > 0) {
    sShutdownSteps[mCurrentStep].mTicks = ticks;
  }

  
  
  int nextStep = -1;
  for (size_t i = 0; i < ArrayLength(sShutdownSteps); ++i) {
    if (strcmp(sShutdownSteps[i].mTopic, aTopic) == 0) {
      nextStep = i;
      break;
    }
  }
  MOZ_ASSERT(nextStep != -1);
  mCurrentStep = nextStep;
}

void
nsTerminator::UpdateTelemetry()
{
  if (!Telemetry::CanRecord() || !gWriteReady) {
    return;
  }

  
  
  
  
  
  
  

  
  UniquePtr<nsCString> telemetryData(new nsCString());
  telemetryData->AppendLiteral("{");
  size_t fields = 0;
  for (size_t i = 0; i < ArrayLength(sShutdownSteps); ++i) {
    if (sShutdownSteps[i].mTicks < 0) {
      
      continue;
    }
    if (fields++ > 0) {
      telemetryData->Append(", ");
    }
    telemetryData->AppendLiteral("\"");
    telemetryData->Append(sShutdownSteps[i].mTopic);
    telemetryData->AppendLiteral("\": ");
    telemetryData->AppendInt(sShutdownSteps[i].mTicks);
  }
  telemetryData->AppendLiteral("}");

  if (fields == 0) {
    
      return;
  }

  
  
  
  delete gWriteData.exchange(telemetryData.release()); 

  
  PR_EnterMonitor(gWriteReady);
  PR_Notify(gWriteReady);
  PR_ExitMonitor(gWriteReady);
}

void
nsTerminator::UpdateCrashReport(const char* aTopic)
{
#if defined(MOZ_CRASHREPORTER)
  
  nsAutoCString report(aTopic);

  unused << CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("ShutdownProgress"),
                                               report);
#endif 
}


} 
