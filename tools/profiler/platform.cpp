



#include <iostream>
#include <fstream>
#include <sstream>
#include <errno.h>

#include "platform.h"
#include "PlatformMacros.h"
#include "prenv.h"
#include "mozilla/ThreadLocal.h"
#include "PseudoStack.h"
#include "TableTicker.h"
#include "UnwinderThread2.h"
#include "nsIObserverService.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "mozilla/Services.h"

mozilla::ThreadLocal<PseudoStack *> tlsPseudoStack;
mozilla::ThreadLocal<TableTicker *> tlsTicker;




bool stack_key_initialized;

TimeStamp sLastTracerEvent; 
int       sFrameNumber = 0;
int       sLastFrameNumber = 0;


unsigned int sLastSampledEventGeneration = 0;




unsigned int sCurrentEventGeneration = 0;





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


void read_env_vars()
{
  bool nativeAvail = false;
# if defined(HAVE_NATIVE_UNWIND)
  nativeAvail = true;
# endif

  MOZ_ASSERT(sUnwindMode     == UnwINVALID);
  MOZ_ASSERT(sUnwindInterval == 0);

  
  sUnwindMode     = nativeAvail ? UnwCOMBINED : UnwPSEUDO;
  sUnwindInterval = 0;  

  const char* strM = PR_GetEnv("MOZ_PROFILER_MODE");
  const char* strI = PR_GetEnv("MOZ_PROFILER_INTERVAL");

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
  LOGF("SPS:   This platform %s native unwinding.",
       nativeAvail ? "supports" : "does not support");
  LOG( "SPS: ");
  
  sUnwindMode       = nativeAvail ? UnwCOMBINED : UnwPSEUDO;
  sUnwindInterval   = 0;  

 out:
  LOG( "SPS:");
  LOGF("SPS: Unwind mode       = %s", name_UnwMode(sUnwindMode));
  LOGF("SPS: Sampling interval = %d ms (zero means \"platform default\")",
       (int)sUnwindInterval);
  LOG( "SPS: Use env var MOZ_PROFILER_MODE=help for further information.");
  LOG( "SPS:");

  return;
}




void mozilla_sampler_init()
{
  if (stack_key_initialized)
    return;

  LOG("BEGIN mozilla_sampler_init");
  if (!tlsPseudoStack.init() || !tlsTicker.init()) {
    LOG("Failed to init.");
    return;
  }
  stack_key_initialized = true;

  PseudoStack *stack = new PseudoStack();
  tlsPseudoStack.set(stack);

  if (sps_version2()) {
    
    
    read_env_vars();

    
    uwt__init();

# if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
     || defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_android) \
     || defined(SPS_PLAT_x86_windows) || defined(SPS_PLAT_amd64_windows) 
    
    int aLocal;
    uwt__register_thread_for_profiling( &aLocal );
# elif defined(SPS_PLAT_amd64_darwin) || defined(SPS_PLAT_x86_darwin)
    
# else
#   error "Unknown plat"
# endif
  }

  
  OS::RegisterStartHandler();

  
  
  
  const char *val = PR_GetEnv("MOZ_PROFILER_STARTUP");
  if (!val || !*val) {
    return;
  }

  const char* features[] = {"js"
                         , "leaf"
#if defined(XP_WIN) || defined(XP_MACOSX)
                         , "stackwalk"
#endif
                         };
  profiler_start(PROFILE_DEFAULT_ENTRY, PROFILE_DEFAULT_INTERVAL,
                         features, sizeof(features)/sizeof(const char*));
  LOG("END   mozilla_sampler_init");
}

void mozilla_sampler_shutdown()
{
  
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

  
  
  
  
  if (sps_version2()) {
    uwt__deinit();
  }

  profiler_stop();
  
  
  
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

  std::stringstream profile;
  t->SetPaused(true);
  profile << *(t->GetPrimaryThreadProfile());
  t->SetPaused(false);

  std::string profileString = profile.str();
  char *rtn = (char*)malloc( (profileString.length() + 1) * sizeof(char) );
  strcpy(rtn, profileString.c_str());
  return rtn;
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
    "jank",
    "js",
    NULL
  };

  return features;
}


void mozilla_sampler_start(int aProfileEntries, int aInterval,
                           const char** aFeatures, uint32_t aFeatureCount)
{
  if (!stack_key_initialized)
    profiler_init();

  

  if (sUnwindInterval > 0)
    aInterval = sUnwindInterval;

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    ASSERT(false);
    return;
  }

  
  profiler_stop();

  TableTicker* t;
  if (sps_version2()) {
    t = new BreakpadSampler(aInterval ? aInterval : PROFILE_DEFAULT_INTERVAL,
                           aProfileEntries ? aProfileEntries : PROFILE_DEFAULT_ENTRY,
                           stack, aFeatures, aFeatureCount);
  } else {
    t = new TableTicker(aInterval ? aInterval : PROFILE_DEFAULT_INTERVAL,
                        aProfileEntries ? aProfileEntries : PROFILE_DEFAULT_ENTRY,
                        stack, aFeatures, aFeatureCount);
  }
  tlsTicker.set(t);
  t->Start();
  if (t->ProfileJS())
      stack->enableJSSampling();

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->NotifyObservers(nullptr, "profiler-started", nullptr);
}

void mozilla_sampler_stop()
{
  if (!stack_key_initialized)
    profiler_init();

  TableTicker *t = tlsTicker.get();
  if (!t) {
    return;
  }

  bool disableJS = t->ProfileJS();

  t->Stop();
  delete t;
  tlsTicker.set(NULL);
  PseudoStack *stack = tlsPseudoStack.get();
  ASSERT(stack != NULL);

  if (disableJS)
    stack->disableJSSampling();

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->NotifyObservers(nullptr, "profiler-stopped", nullptr);
}

bool mozilla_sampler_is_active()
{
  if (!stack_key_initialized)
    profiler_init();

  TableTicker *t = tlsTicker.get();
  if (!t) {
    return false;
  }

  return t->IsActive();
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




