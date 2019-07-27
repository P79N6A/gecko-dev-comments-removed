



#include <ostream>
#include <fstream>
#include <sstream>
#include <errno.h>

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
#include "nsXULAppAPI.h"

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
  #include "AndroidBridge.h"
  using namespace mozilla::widget::android;
#endif

mozilla::ThreadLocal<PseudoStack *> tlsPseudoStack;
mozilla::ThreadLocal<TableTicker *> tlsTicker;
mozilla::ThreadLocal<void *> tlsStackTop;




bool stack_key_initialized;

mozilla::TimeStamp   sLastTracerEvent; 
mozilla::TimeStamp   sStartTime;
int         sFrameNumber = 0;
int         sLastFrameNumber = 0;
int         sInitCount = 0; 
static bool sIsProfiling = false; 


const char* PROFILER_MODE = "MOZ_PROFILER_MODE";
const char* PROFILER_INTERVAL = "MOZ_PROFILER_INTERVAL";
const char* PROFILER_ENTRIES = "MOZ_PROFILER_ENTRIES";
const char* PROFILER_STACK = "MOZ_PROFILER_STACK_SCAN";
const char* PROFILER_FEATURES = "MOZ_PROFILING_FEATURES";


unsigned int sLastSampledEventGeneration = 0;




unsigned int sCurrentEventGeneration = 0;





std::vector<ThreadInfo*>* Sampler::sRegisteredThreads = nullptr;
mozilla::Mutex* Sampler::sRegisteredThreadsMutex = nullptr;

TableTicker* Sampler::sActiveSampler;

static mozilla::StaticAutoPtr<mozilla::ProfilerIOInterposeObserver>
                                                            sInterposeObserver;






static const char * gGeckoThreadName = "GeckoMain";

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

ThreadInfo::ThreadInfo(const char* aName, int aThreadId,
                       bool aIsMainThread, PseudoStack* aPseudoStack,
                       void* aStackTop)
  : mName(strdup(aName))
  , mThreadId(aThreadId)
  , mIsMainThread(aIsMainThread)
  , mPseudoStack(aPseudoStack)
  , mPlatformData(Sampler::AllocPlatformData(aThreadId))
  , mProfile(nullptr)
  , mStackTop(aStackTop)
{
  mThread = NS_GetCurrentThread();
}

ThreadInfo::~ThreadInfo() {
  free(mName);

  if (mProfile)
    delete mProfile;

  Sampler::FreePlatformData(mPlatformData);
}

ProfilerMarker::ProfilerMarker(const char* aMarkerName,
    ProfilerMarkerPayload* aPayload,
    float aTime)
  : mMarkerName(strdup(aMarkerName))
  , mPayload(aPayload)
  , mTime(aTime)
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

float
ProfilerMarker::GetTime() {
  return mTime;
}

void ProfilerMarker::StreamJSObject(JSStreamWriter& b) const {
  b.BeginObject();
    b.NameValue("name", GetMarkerName());
    
    
    
    if (mPayload) {
      b.Name("data");
      mPayload->StreamPayload(b);
    }
    b.NameValue("time", mTime);
  b.EndObject();
}

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

    bool req2 = PR_GetEnv("MOZ_PROFILER_NEW") != nullptr; 

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


bool moz_profiler_verbose()
{
  
  static int status = 0; 

  if (status == 0) {
    if (PR_GetEnv("MOZ_PROFILER_VERBOSE") != nullptr)
      status = 2;
    else
      status = 1;
  }

  return status == 2;
}

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

bool set_profiler_mode(const char* mode) {
  if (mode) {
    if (0 == strcmp(mode, "pseudo")) {
      sUnwindMode = UnwPSEUDO;
      return true;
    }
    else if (0 == strcmp(mode, "native") && is_native_unwinding_avail()) {
      sUnwindMode = UnwNATIVE;
      return true;
    }
    else if (0 == strcmp(mode, "combined") && is_native_unwinding_avail()) {
      sUnwindMode = UnwCOMBINED;
      return true;
    } else {
      return false;
    }
  }

  return true;
}

bool set_profiler_interval(const char* interval) {
  if (interval) {
    errno = 0;
    long int n = strtol(interval, (char**)nullptr, 10);
    if (errno == 0 && n >= 1 && n <= 1000) {
      sUnwindInterval = n;
      return true;
    }
    return false;
  }

  return true;
}

bool set_profiler_entries(const char* entries) {
  if (entries) {
    errno = 0;
    long int n = strtol(entries, (char**)nullptr, 10);
    if (errno == 0 && n > 0) {
      sProfileEntries = n;
      return true;
    }
    return false;
  }

  return true;
}

bool set_profiler_scan(const char* scanCount) {
  if (scanCount) {
    errno = 0;
    long int n = strtol(scanCount, (char**)nullptr, 10);
    if (errno == 0 && n >= 0 && n <= 100) {
      sUnwindStackScan = n;
      return true;
    }
    return false;
  }

  return true;
}

bool is_native_unwinding_avail() {
# if defined(HAVE_NATIVE_UNWIND)
  return true;
#else
  return false;
#endif
}


void read_profiler_env_vars()
{
  bool nativeAvail = is_native_unwinding_avail();

  
  sUnwindMode     = nativeAvail ? UnwCOMBINED : UnwPSEUDO;
  sUnwindInterval = 0;  
  sProfileEntries = 0;

  const char* stackMode = PR_GetEnv(PROFILER_MODE);
  const char* interval = PR_GetEnv(PROFILER_INTERVAL);
  const char* entries = PR_GetEnv(PROFILER_ENTRIES);
  const char* scanCount = PR_GetEnv(PROFILER_STACK);

  if (!set_profiler_mode(stackMode) ||
      !set_profiler_interval(interval) ||
      !set_profiler_entries(entries) ||
      !set_profiler_scan(scanCount)) {
      profiler_usage();
  } else {
    LOG( "SPS:");
    LOGF("SPS: Unwind mode       = %s", name_UnwMode(sUnwindMode));
    LOGF("SPS: Sampling interval = %d ms (zero means \"platform default\")",
        (int)sUnwindInterval);
    LOGF("SPS: Entry store size  = %d (zero means \"platform default\")",
        (int)sProfileEntries);
    LOGF("SPS: UnwindStackScan   = %d (max dubious frames per unwind).",
        (int)sUnwindStackScan);
    LOG( "SPS: Use env var MOZ_PROFILER_MODE=help for further information.");
    LOG( "SPS: Note that MOZ_PROFILER_MODE=help sets all values to defaults.");
    LOG( "SPS:");
  }
}

void profiler_usage() {
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
  LOG( "SPS:   Needs to be set to use LUL-based unwinding.");
  LOG( "SPS: ");
  LOG( "SPS:   MOZ_PROFILER_LUL_TEST");
  LOG( "SPS:   If set to any value, runs LUL unit tests at startup of");
  LOG( "SPS:   the unwinder thread, and prints a short summary of results.");
  LOG( "SPS: ");
  LOGF("SPS:   This platform %s native unwinding.",
       is_native_unwinding_avail() ? "supports" : "does not support");
  LOG( "SPS: ");

  
  sUnwindMode       = is_native_unwinding_avail() ? UnwCOMBINED : UnwPSEUDO;
  sUnwindInterval   = 0;  
  sProfileEntries   = 0;
  sUnwindStackScan  = 0;

  LOG( "SPS:");
  LOGF("SPS: Unwind mode       = %s", name_UnwMode(sUnwindMode));
  LOGF("SPS: Sampling interval = %d ms (zero means \"platform default\")",
       (int)sUnwindInterval);
  LOGF("SPS: Entry store size  = %d (zero means \"platform default\")",
       (int)sProfileEntries);
  LOGF("SPS: UnwindStackScan   = %d (max dubious frames per unwind).",
       (int)sUnwindStackScan);
  LOG( "SPS: Use env var MOZ_PROFILER_MODE=help for further information.");
  LOG( "SPS: Note that MOZ_PROFILER_MODE=help sets all values to defaults.");
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

bool is_main_thread_name(const char* aName) {
  if (!aName) {
    return false;
  }
  return strcmp(aName, gGeckoThreadName) == 0;
}

#ifdef HAVE_VA_COPY
#define VARARGS_ASSIGN(foo, bar)        VA_COPY(foo,bar)
#elif defined(HAVE_VA_LIST_AS_ARRAY)
#define VARARGS_ASSIGN(foo, bar)     foo[0] = bar[0]
#else
#define VARARGS_ASSIGN(foo, bar)     (foo) = (bar)
#endif

static void
profiler_log(const char *fmt, va_list args)
{
  if (profiler_is_active()) {
    
    
    char buf[2048];
    va_list argsCpy;
    VARARGS_ASSIGN(argsCpy, args);
    int required = vsnprintf(buf, sizeof(buf), fmt, argsCpy);
    va_end(argsCpy);

    if (required < 0) {
      return; 
    } else if (required < 2048) {
      profiler_tracing("log", buf, TRACING_EVENT);
    } else {
      char* heapBuf = new char[required+1];
      va_list argsCpy;
      VARARGS_ASSIGN(argsCpy, args);
      vsnprintf(heapBuf, required+1, fmt, argsCpy);
      va_end(argsCpy);
      
      
      
      profiler_tracing("log", heapBuf, TRACING_EVENT);
      delete[] heapBuf;
    }
  }
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

  bool isMainThread = true;
#ifdef XP_WIN
  
  isMainThread = !(XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro);
#endif
  Sampler::RegisterCurrentThread(isMainThread ?
                                   gGeckoThreadName : "Application Thread",
                                 stack, isMainThread, stackTop);

  
  
  
  read_profiler_env_vars();

  
  OS::Startup();

  set_stderr_callback(profiler_log);

  
  
  
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
                         
                         nullptr, 0);
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

  set_stderr_callback(nullptr);

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
    return nullptr;
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
    return nullptr;
  }

  return t->ToJSObject(aCx);
}

void mozilla_sampler_save_profile_to_file(const char* aFilename)
{
  TableTicker *t = tlsTicker.get();
  if (!t) {
    return;
  }

  std::ofstream stream;
  stream.open(aFilename);
  if (stream.is_open()) {
    t->ToStreamAsJSON(stream);
    stream.close();
    LOGF("Saved to %s", aFilename);
  } else {
    LOG("Fail to open profile log file.");
  }
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
    
    "memory",
#if defined(XP_WIN)
    
    "power",
#endif
    nullptr
  };

  return features;
}


void mozilla_sampler_start(int aProfileEntries, double aInterval,
                           const char** aFeatures, uint32_t aFeatureCount,
                           const char** aThreadNameFilters, uint32_t aFilterCount)

{
  LOG("BEGIN mozilla_sampler_start");

  if (!stack_key_initialized)
    profiler_init(nullptr);

  

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
        thread_profile->GetPseudoStack()->reinitializeOnResume();
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
    mozilla::widget::android::GeckoJavaSampler::StartJavaProfiling(javaInterval, 1000);
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

  if (Sampler::CanNotifyObservers()) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os)
      os->NotifyObservers(nullptr, "profiler-started", nullptr);
  }

  LOG("END   mozilla_sampler_start");
}

void mozilla_sampler_stop()
{
  LOG("BEGIN mozilla_sampler_stop");

  if (!stack_key_initialized)
    profiler_init(nullptr);

  TableTicker *t = tlsTicker.get();
  if (!t) {
    LOG("END   mozilla_sampler_stop-early");
    return;
  }

  bool disableJS = t->ProfileJS();
  bool unwinderThreader = t->HasUnwinderThread();

  
  
  
  
  if (unwinderThreader) {
    uwt__stop();
  }

  t->Stop();
  delete t;
  tlsTicker.set(nullptr);

  if (disableJS) {
    PseudoStack *stack = tlsPseudoStack.get();
    ASSERT(stack != nullptr);
    stack->disableJSSampling();
  }

  if (unwinderThreader) {
    uwt__deinit();
  }

  mozilla::IOInterposer::Unregister(mozilla::IOInterposeObserver::OpAll,
                                    sInterposeObserver);
  sInterposeObserver = nullptr;

  sIsProfiling = false;

  if (Sampler::CanNotifyObservers()) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os)
      os->NotifyObservers(nullptr, "profiler-stopped", nullptr);
  }

  LOG("END   mozilla_sampler_stop");
}

bool mozilla_sampler_is_paused() {
  if (Sampler::GetActiveSampler()) {
    return Sampler::GetActiveSampler()->IsPaused();
  } else {
    return false;
  }
}

void mozilla_sampler_pause() {
  if (Sampler::GetActiveSampler()) {
    Sampler::GetActiveSampler()->SetPaused(true);
  }
}

void mozilla_sampler_resume() {
  if (Sampler::GetActiveSampler()) {
    Sampler::GetActiveSampler()->SetPaused(false);
  }
}

bool mozilla_sampler_is_active()
{
  return sIsProfiling;
}

void mozilla_sampler_responsiveness(const mozilla::TimeStamp& aTime)
{
  sCurrentEventGeneration++;

  sLastTracerEvent = aTime;
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
  if (sInitCount == 0) {
    return false;
  }

#if defined(MOZ_WIDGET_GONK) && !defined(MOZ_PROFILING)
  
  
  
  if (!profiler_is_active()) {
    return false;
  }
#endif

  PseudoStack* stack = new PseudoStack();
  tlsPseudoStack.set(stack);
  bool isMainThread = is_main_thread_name(aName);
  return Sampler::RegisterCurrentThread(aName, stack, isMainThread, stackTop);
}

void mozilla_sampler_unregister_thread()
{
  if (sInitCount == 0) {
    return;
  }

  Sampler::UnregisterCurrentThread();

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    return;
  }
  delete stack;
  tlsPseudoStack.set(nullptr);
}

void mozilla_sampler_sleep_start() {
    if (sInitCount == 0) {
	return;
    }

    PseudoStack *stack = tlsPseudoStack.get();
    if (stack == nullptr) {
      return;
    }
    stack->setSleeping(1);
}

void mozilla_sampler_sleep_end() {
    if (sInitCount == 0) {
	return;
    }

    PseudoStack *stack = tlsPseudoStack.get();
    if (stack == nullptr) {
      return;
    }
    stack->setSleeping(0);
}

double mozilla_sampler_time(const mozilla::TimeStamp& aTime)
{
  if (!mozilla_sampler_is_active()) {
    return 0.0;
  }
  mozilla::TimeDuration delta = aTime - sStartTime;
  return delta.ToMilliseconds();
}

double mozilla_sampler_time()
{
  return mozilla_sampler_time(mozilla::TimeStamp::Now());
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

void mozilla_sampler_tracing(const char* aCategory, const char* aInfo,
                             ProfilerBacktrace* aCause,
                             TracingMetadata aMetaData)
{
  mozilla_sampler_add_marker(aInfo, new ProfilerMarkerTracing(aCategory, aMetaData, aCause));
}

void mozilla_sampler_add_marker(const char *aMarker, ProfilerMarkerPayload *aPayload)
{
  
  
  nsAutoPtr<ProfilerMarkerPayload> payload(aPayload);

  if (!stack_key_initialized)
    return;

  
  
  if (!profiler_is_active()) {
    return;
  }

  
  if (profiler_in_privacy_mode()) {
    return;
  }

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    return;
  }
  mozilla::TimeDuration delta = mozilla::TimeStamp::Now() - sStartTime;
  stack->addMarker(aMarker, payload.forget(), static_cast<float>(delta.ToMilliseconds()));
}





