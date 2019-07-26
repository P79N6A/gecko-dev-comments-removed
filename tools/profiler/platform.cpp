



#include <ostream>
#include <fstream>
#include <sstream>
#include <errno.h>

#include "IOInterposer.h"
#include "NSPRInterposer.h"
#include "ProfilerIOInterposeObserver.h"
#include "platform.h"
#include "PlatformMacros.h"
#include "prenv.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ThreadLocal.h"
#include "PseudoStack.h"
#include "TableTicker.h"
#include "UnwinderThread2.h"
#include "nsIObserverService.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "ProfilerMarkers.h"

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
  #include "AndroidBridge.h"
#endif

mozilla::ThreadLocal<PseudoStack *> tlsPseudoStack;
mozilla::ThreadLocal<TableTicker *> tlsTicker;
mozilla::ThreadLocal<void *> tlsStackTop;




bool stack_key_initialized;

TimeStamp   sLastTracerEvent; 
TimeStamp   sStartTime;
int         sFrameNumber = 0;
int         sLastFrameNumber = 0;
int         sInitCount = 0; 
static bool sIsProfiling = false; 


unsigned int sLastSampledEventGeneration = 0;




unsigned int sCurrentEventGeneration = 0;





std::vector<ThreadInfo*>* Sampler::sRegisteredThreads = nullptr;
mozilla::Mutex* Sampler::sRegisteredThreadsMutex = nullptr;

TableTicker* Sampler::sActiveSampler;

static mozilla::StaticAutoPtr<mozilla::ProfilerIOInterposeObserver>
                                                            sInterposeObserver;

void Sampler::Startup() {
  sRegisteredThreads = new std::vector<ThreadInfo*>();
  sRegisteredThreadsMutex = new mozilla::Mutex("sRegisteredThreads mutex");
}

void Sampler::Shutdown() {
  while (sRegisteredThreads->size() > 0) {
    delete sRegisteredThreads->back();
    sRegisteredThreads->pop_back();
  }

  delete sRegisteredThreadsMutex;
  delete sRegisteredThreads;

  
  
  sRegisteredThreadsMutex = nullptr;
  sRegisteredThreads = nullptr;
}

ThreadInfo::~ThreadInfo() {
  free(mName);

  if (mProfile)
    delete mProfile;

  Sampler::FreePlatformData(mPlatformData);
}

ProfilerMarker::ProfilerMarker(const char* aMarkerName,
    ProfilerMarkerPayload* aPayload)
  : mMarkerName(strdup(aMarkerName))
  , mPayload(aPayload)
{
}

ProfilerMarker::~ProfilerMarker() {
  free(mMarkerName);
  delete mPayload;
}

void
ProfilerMarker::SetGeneration(int aGenID) {
  mGenID = aGenID;
}

template<typename Builder> void
ProfilerMarker::BuildJSObject(Builder& b, typename Builder::ArrayHandle markers) const {
  typename Builder::RootedObject marker(b.context(), b.CreateObject());
  b.DefineProperty(marker, "name", GetMarkerName());
  
  
  
  if (mPayload) {
    typename Builder::RootedObject markerData(b.context(),
                                              mPayload->PreparePayload(b));
    b.DefineProperty(marker, "data", markerData);
  }
  b.ArrayPush(markers, marker);
}

template void
ProfilerMarker::BuildJSObject<JSCustomObjectBuilder>(JSCustomObjectBuilder& b,
                              JSCustomObjectBuilder::ArrayHandle markers) const;
template void
ProfilerMarker::BuildJSObject<JSObjectBuilder>(JSObjectBuilder& b,
                                    JSObjectBuilder::ArrayHandle markers) const;

PendingMarkers::~PendingMarkers() {
  clearMarkers();
  if (mSignalLock != false) {
    
    
    
    
    abort();
  }
}

void
PendingMarkers::addMarker(ProfilerMarker *aMarker) {
  mSignalLock = true;
  STORE_SEQUENCER();

  MOZ_ASSERT(aMarker);
  mPendingMarkers.insert(aMarker);

  
  while (mStoredMarkers.peek() &&
         mStoredMarkers.peek()->HasExpired(mGenID)) {
    delete mStoredMarkers.popHead();
  } 
  STORE_SEQUENCER();
  mSignalLock = false;
}

void
PendingMarkers::updateGeneration(int aGenID) {
  mGenID = aGenID;
}

void
PendingMarkers::addStoredMarker(ProfilerMarker *aStoredMarker) {
  aStoredMarker->SetGeneration(mGenID);
  mStoredMarkers.insert(aStoredMarker);
}

bool sps_version2()
{
  static int version = 0; 

  if (version == 0) {
    bool allow2 = false; 
#   if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
       || defined(SPS_PLAT_x86_linux)
    allow2 = true;
#   elif defined(SPS_PLAT_amd64_darwin) || defined(SPS_PLAT_x86_darwin) \
         || defined(SPS_PLAT_x86_windows) || defined(SPS_PLAT_x86_android) \
         || defined(SPS_PLAT_amd64_windows)
    allow2 = false;
#   else
#     error "Unknown platform"
#   endif

    bool req2 = PR_GetEnv("MOZ_PROFILER_NEW") != NULL; 

    bool elfhackd = false;
#   if defined(USE_ELF_HACK)
    bool elfhackd = true;
#   endif

    if (req2 && allow2) {
      version = 2;
      LOG("------------------- MOZ_PROFILER_NEW set -------------------");
    } else if (req2 && !allow2) {
      version = 1;
      LOG("--------------- MOZ_PROFILER_NEW requested, ----------------");
      LOG("---------- but is not available on this platform -----------");
    } else if (req2 && elfhackd) {
      version = 1;
      LOG("--------------- MOZ_PROFILER_NEW requested, ----------------");
      LOG("--- but this build was not done with --disable-elf-hack ----");
    } else {
      version = 1;
      LOG("----------------- MOZ_PROFILER_NEW not set -----------------");
    }
  }
  return version == 2;
}

#if !defined(ANDROID)

bool moz_profiler_verbose()
{
  
  static int status = 0; 

  if (status == 0) {
    if (PR_GetEnv("MOZ_PROFILER_VERBOSE") != NULL)
      status = 2;
    else
      status = 1;
  }

  return status == 2;
}
#endif

static inline const char* name_UnwMode(UnwMode m)
{
  switch (m) {
    case UnwINVALID:  return "invalid";
    case UnwNATIVE:   return "native";
    case UnwPSEUDO:   return "pseudo";
    case UnwCOMBINED: return "combined";
    default:          return "??name_UnwMode??";
  }
}


void read_profiler_env_vars()
{
  bool nativeAvail = false;
# if defined(HAVE_NATIVE_UNWIND)
  nativeAvail = true;
# endif

  MOZ_ASSERT(sUnwindMode     == UnwINVALID);
  MOZ_ASSERT(sUnwindInterval == 0);
  MOZ_ASSERT(sProfileEntries == 0);

  
  sUnwindMode     = nativeAvail ? UnwCOMBINED : UnwPSEUDO;
  sUnwindInterval = 0;  
  sProfileEntries = 0;

  const char* strM = PR_GetEnv("MOZ_PROFILER_MODE");
  const char* strI = PR_GetEnv("MOZ_PROFILER_INTERVAL");
  const char* strE = PR_GetEnv("MOZ_PROFILER_ENTRIES");
  const char* strF = PR_GetEnv("MOZ_PROFILER_STACK_SCAN");

  if (strM) {
    if (0 == strcmp(strM, "pseudo"))
      sUnwindMode = UnwPSEUDO;
    else if (0 == strcmp(strM, "native") && nativeAvail)
      sUnwindMode = UnwNATIVE;
    else if (0 == strcmp(strM, "combined") && nativeAvail)
      sUnwindMode = UnwCOMBINED;
    else goto usage;
  }

  if (strI) {
    errno = 0;
    long int n = strtol(strI, (char**)NULL, 10);
    if (errno == 0 && n >= 1 && n <= 1000) {
      sUnwindInterval = n;
    }
    else goto usage;
  }

  if (strE) {
    errno = 0;
    long int n = strtol(strE, (char**)NULL, 10);
    if (errno == 0 && n > 0) {
      sProfileEntries = n;
    }
    else goto usage;
  }

  if (strF) {
    errno = 0;
    long int n = strtol(strF, (char**)NULL, 10);
    if (errno == 0 && n >= 0 && n <= 100) {
      sUnwindStackScan = n;
    }
    else goto usage;
  }

  goto out;

 usage:
  LOG( "SPS: ");
  LOG( "SPS: Environment variable usage:");
  LOG( "SPS: ");
  LOG( "SPS:   MOZ_PROFILER_MODE=native    for native unwind only");
  LOG( "SPS:   MOZ_PROFILER_MODE=pseudo    for pseudo unwind only");
  LOG( "SPS:   MOZ_PROFILER_MODE=combined  for combined native & pseudo unwind");
  LOG( "SPS:   If unset, default is 'combined' on native-capable");
  LOG( "SPS:     platforms, 'pseudo' on others.");
  LOG( "SPS: ");
  LOG( "SPS:   MOZ_PROFILER_INTERVAL=<number>   (milliseconds, 1 to 1000)");
  LOG( "SPS:   If unset, platform default is used.");
  LOG( "SPS: ");
  LOG( "SPS:   MOZ_PROFILER_ENTRIES=<number>    (count, minimum of 1)");
  LOG( "SPS:   If unset, platform default is used.");
  LOG( "SPS: ");
  LOG( "SPS:   MOZ_PROFILER_VERBOSE");
  LOG( "SPS:   If set to any value, increases verbosity (recommended).");
  LOG( "SPS: ");
  LOG( "SPS:   MOZ_PROFILER_STACK_SCAN=<number>   (default is zero)");
  LOG( "SPS:   The number of dubious (stack-scanned) frames allowed");
  LOG( "SPS: ");
  LOG( "SPS:   MOZ_PROFILER_NEW");
  LOG( "SPS:   Needs to be set to use Breakpad-based unwinding.");
  LOG( "SPS: ");
  LOGF("SPS:   This platform %s native unwinding.",
       nativeAvail ? "supports" : "does not support");
  LOG( "SPS: ");
  
  sUnwindMode       = nativeAvail ? UnwCOMBINED : UnwPSEUDO;
  sUnwindInterval   = 0;  
  sProfileEntries   = 0;
  sUnwindStackScan  = 0;

 out:
  LOG( "SPS:");
  LOGF("SPS: Unwind mode       = %s", name_UnwMode(sUnwindMode));
  LOGF("SPS: Sampling interval = %d ms (zero means \"platform default\")",
       (int)sUnwindInterval);
  LOGF("SPS: Entry store size  = %d (zero means \"platform default\")",
       (int)sProfileEntries);
  LOGF("SPS: UnwindStackScan   = %d (max dubious frames per unwind).",
       (int)sUnwindStackScan);
  LOG( "SPS: Use env var MOZ_PROFILER_MODE=help for further information.");
  LOG( "SPS:");

  return;
}

void set_tls_stack_top(void* stackTop)
{
  
  
  
  
  uintptr_t stackTopR = (uintptr_t)stackTop;
  if (stackTop) {
    stackTopR = (stackTopR & ~(uintptr_t)4095) + (uintptr_t)4095;
  }
  tlsStackTop.set((void*)stackTopR);
}




void mozilla_sampler_init(void* stackTop)
{
  sInitCount++;

  if (stack_key_initialized)
    return;

  LOG("BEGIN mozilla_sampler_init");
  if (!tlsPseudoStack.init() || !tlsTicker.init() || !tlsStackTop.init()) {
    LOG("Failed to init.");
    return;
  }
  stack_key_initialized = true;

  Sampler::Startup();

  PseudoStack *stack = new PseudoStack();
  tlsPseudoStack.set(stack);

  Sampler::RegisterCurrentThread("GeckoMain", stack, true, stackTop);

  
  
  
  read_profiler_env_vars();

  
  OS::RegisterStartHandler();

  
  mozilla::IOInterposer::Init();
  
  mozilla::InitNSPRIOInterposing();

  
  
  
  const char *val = PR_GetEnv("MOZ_PROFILER_STARTUP");
  if (!val || !*val) {
    return;
  }

  const char* features[] = {"js"
                         , "leaf"
#if defined(XP_WIN) || defined(XP_MACOSX) || (defined(SPS_ARCH_arm) && defined(linux))
                         , "stackwalk"
#endif
#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
                         , "java"
#endif
                         };
  profiler_start(PROFILE_DEFAULT_ENTRY, PROFILE_DEFAULT_INTERVAL,
                         features, sizeof(features)/sizeof(const char*),
                         
                         NULL, 0);
  LOG("END   mozilla_sampler_init");
}

void mozilla_sampler_shutdown()
{
  sInitCount--;

  if (sInitCount > 0)
    return;

  
  TableTicker *t = tlsTicker.get();
  if (t) {
    const char *val = PR_GetEnv("MOZ_PROFILER_SHUTDOWN");
    if (val) {
      std::ofstream stream;
      stream.open(val);
      if (stream.is_open()) {
        t->ToStreamAsJSON(stream);
        stream.close();
      }
    }
  }

  profiler_stop();

  
  mozilla::IOInterposer::Unregister(mozilla::IOInterposeObserver::OpAll,
                                    sInterposeObserver);
  
  
  
  
  mozilla::ClearNSPRIOInterposing();
  sInterposeObserver = nullptr;

  Sampler::Shutdown();

  
  
  
}

void mozilla_sampler_save()
{
  TableTicker *t = tlsTicker.get();
  if (!t) {
    return;
  }

  t->RequestSave();
  
  
  t->HandleSaveRequest();
}

char* mozilla_sampler_get_profile()
{
  TableTicker *t = tlsTicker.get();
  if (!t) {
    return NULL;
  }

  std::stringstream stream;
  t->ToStreamAsJSON(stream);
  char* profile = strdup(stream.str().c_str());
  return profile;
}

JSObject *mozilla_sampler_get_profile_data(JSContext *aCx)
{
  TableTicker *t = tlsTicker.get();
  if (!t) {
    return NULL;
  }

  return t->ToJSObject(aCx);
}


const char** mozilla_sampler_get_features()
{
  static const char* features[] = {
#if defined(MOZ_PROFILING) && defined(HAVE_NATIVE_UNWIND)
    
    "stackwalk",
#endif
#if defined(ENABLE_SPS_LEAF_DATA)
    
    
    "leaf",
#endif
#if !defined(SPS_OS_windows)
    
    "unwinder",
#endif
    "java",
    
    "jank",
    
    
    "js",
    
    "threads",
    
    "privacy",
    
    "mainthreadio",
    NULL
  };

  return features;
}


void mozilla_sampler_start(int aProfileEntries, double aInterval,
                           const char** aFeatures, uint32_t aFeatureCount,
                           const char** aThreadNameFilters, uint32_t aFilterCount)

{
  if (!stack_key_initialized)
    profiler_init(NULL);

  

  if (sUnwindInterval > 0)
    aInterval = sUnwindInterval;

  
  if (sProfileEntries > 0)
    aProfileEntries = sProfileEntries;

  
  profiler_stop();

  TableTicker* t;
  t = new TableTicker(aInterval ? aInterval : PROFILE_DEFAULT_INTERVAL,
                      aProfileEntries ? aProfileEntries : PROFILE_DEFAULT_ENTRY,
                      aFeatures, aFeatureCount,
                      aThreadNameFilters, aFilterCount);
  if (t->HasUnwinderThread()) {
    
    uwt__init();
  }

  tlsTicker.set(t);
  t->Start();
  if (t->ProfileJS() || t->InPrivacyMode()) {
      mozilla::MutexAutoLock lock(*Sampler::sRegisteredThreadsMutex);
      std::vector<ThreadInfo*> threads = t->GetRegisteredThreads();

      for (uint32_t i = 0; i < threads.size(); i++) {
        ThreadInfo* info = threads[i];
        ThreadProfile* thread_profile = info->Profile();
        if (!thread_profile) {
          continue;
        }
        if (t->ProfileJS()) {
          thread_profile->GetPseudoStack()->enableJSSampling();
        }
        if (t->InPrivacyMode()) {
          thread_profile->GetPseudoStack()->mPrivacyMode = true;
        }
      }
  }

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
  if (t->ProfileJava()) {
    int javaInterval = aInterval;
    
    if (javaInterval < 10) {
      aInterval = 10;
    }
    mozilla::AndroidBridge::Bridge()->StartJavaProfiling(javaInterval, 1000);
  }
#endif

  if (t->AddMainThreadIO()) {
    if (!sInterposeObserver) {
      
      sInterposeObserver = new mozilla::ProfilerIOInterposeObserver();
    }
    mozilla::IOInterposer::Register(mozilla::IOInterposeObserver::OpAll,
                                    sInterposeObserver);
  }

  sIsProfiling = true;

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->NotifyObservers(nullptr, "profiler-started", nullptr);
}

void mozilla_sampler_stop()
{
  if (!stack_key_initialized)
    profiler_init(NULL);

  TableTicker *t = tlsTicker.get();
  if (!t) {
    return;
  }

  bool disableJS = t->ProfileJS();
  bool unwinderThreader = t->HasUnwinderThread();

  
  
  
  
  if (unwinderThreader) {
    uwt__stop();
  }

  t->Stop();
  delete t;
  tlsTicker.set(NULL);

  if (disableJS) {
    PseudoStack *stack = tlsPseudoStack.get();
    ASSERT(stack != NULL);
    stack->disableJSSampling();
  }

  if (unwinderThreader) {
    uwt__deinit();
  }

  mozilla::IOInterposer::Unregister(mozilla::IOInterposeObserver::OpAll,
                                    sInterposeObserver);

  sIsProfiling = false;

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->NotifyObservers(nullptr, "profiler-stopped", nullptr);
}

bool mozilla_sampler_is_active()
{
  return sIsProfiling;
}

static double sResponsivenessTimes[100];
static unsigned int sResponsivenessLoc = 0;
void mozilla_sampler_responsiveness(const TimeStamp& aTime)
{
  if (!sLastTracerEvent.IsNull()) {
    if (sResponsivenessLoc == 100) {
      for(size_t i = 0; i < 100-1; i++) {
        sResponsivenessTimes[i] = sResponsivenessTimes[i+1];
      }
      sResponsivenessLoc--;
    }
    TimeDuration delta = aTime - sLastTracerEvent;
    sResponsivenessTimes[sResponsivenessLoc++] = delta.ToMilliseconds();
  }
  sCurrentEventGeneration++;

  sLastTracerEvent = aTime;
}

const double* mozilla_sampler_get_responsiveness()
{
  return sResponsivenessTimes;
}

void mozilla_sampler_frame_number(int frameNumber)
{
  sFrameNumber = frameNumber;
}

void mozilla_sampler_print_location2()
{
  
}

void mozilla_sampler_lock()
{
  profiler_stop();
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->NotifyObservers(nullptr, "profiler-locked", nullptr);
}

void mozilla_sampler_unlock()
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->NotifyObservers(nullptr, "profiler-unlocked", nullptr);
}

bool mozilla_sampler_register_thread(const char* aName, void* stackTop)
{
#if defined(MOZ_WIDGET_GONK) && !defined(MOZ_PROFILING)
  
  
  
  if (!profiler_is_active()) {
    return false;
  }
#endif

  PseudoStack* stack = new PseudoStack();
  tlsPseudoStack.set(stack);

  return Sampler::RegisterCurrentThread(aName, stack, false, stackTop);
}

void mozilla_sampler_unregister_thread()
{
  Sampler::UnregisterCurrentThread();

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    return;
  }
  delete stack;
  tlsPseudoStack.set(nullptr);
}

double mozilla_sampler_time(const TimeStamp& aTime)
{
  if (!mozilla_sampler_is_active()) {
    return 0.0;
  }
  TimeDuration delta = aTime - sStartTime;
  return delta.ToMilliseconds();
}

double mozilla_sampler_time()
{
  return mozilla_sampler_time(TimeStamp::Now());
}

ProfilerBacktrace* mozilla_sampler_get_backtrace()
{
  if (!stack_key_initialized)
    return nullptr;

  
  if (!profiler_is_active()) {
    return nullptr;
  }

  
  if (profiler_in_privacy_mode()) {
    return nullptr;
  }

  TableTicker* t = tlsTicker.get();
  if (!t) {
    return nullptr;
  }

  return new ProfilerBacktrace(t->GetBacktrace());
}

void mozilla_sampler_free_backtrace(ProfilerBacktrace* aBacktrace)
{
  delete aBacktrace;
}

void mozilla_sampler_tracing(const char* aCategory, const char* aInfo,
                             TracingMetadata aMetaData)
{
  mozilla_sampler_add_marker(aInfo, new ProfilerMarkerTracing(aCategory, aMetaData));
}





