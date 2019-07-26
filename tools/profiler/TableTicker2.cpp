





#include <string>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#if defined(ANDROID)
# include "android-signal-defs.h"
#endif


#include "PlatformMacros.h"
#include "sps_sampler.h"
#include "platform.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "shared-libraries.h"
#include "mozilla/StackWalk.h"
#include "ProfileEntry2.h"
#include "UnwinderThread2.h"


#include "JSObjectBuilder.h"
#include "nsIJSRuntimeService.h"


#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsIHttpProtocolHandler.h"
#include "nsServiceManagerUtils.h"
#include "nsIXULRuntime.h"
#include "nsIXULAppInfo.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"


#include "jsdbgapi.h"










#undef HAVE_NATIVE_UNWIND
#if defined(MOZ_PROFILING) \
    && (defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
        || defined(SPS_PLAT_x86_linux))
# define HAVE_NATIVE_UNWIND
#endif

#if 0 && defined(MOZ_PROFILING) && !defined(SPS_PLAT_x86_windows)
# warning MOZ_PROFILING
#endif

#if 0 && defined(HAVE_NATIVE_UNWIND)
# warning HAVE_NATIVE_UNWIND
#endif

typedef  enum { UnwINVALID, UnwNATIVE, UnwPSEUDO, UnwCOMBINED }  UnwMode;



static UnwMode sUnwindMode     = UnwINVALID;
static int     sUnwindInterval = 0;


using std::string;
using namespace mozilla;

#ifdef XP_WIN
 #include <windows.h>
 #define getpid GetCurrentProcessId
#else
 #include <unistd.h>
#endif

#ifndef MAXPATHLEN
 #ifdef PATH_MAX
  #define MAXPATHLEN PATH_MAX
 #elif defined(MAX_PATH)
  #define MAXPATHLEN MAX_PATH
 #elif defined(_MAX_PATH)
  #define MAXPATHLEN _MAX_PATH
 #elif defined(CCHMAXPATH)
  #define MAXPATHLEN CCHMAXPATH
 #else
  #define MAXPATHLEN 1024
 #endif
#endif

#if _MSC_VER
 #define snprintf _snprintf
#endif

class TableTicker2;

mozilla::ThreadLocal<PseudoStack *> tlsPseudoStack;
static mozilla::ThreadLocal<TableTicker2 *> tlsTicker;




bool stack_key_initialized;

static TimeStamp sLastTracerEvent; 
static int       sFrameNumber = 0;
static int       sLastFrameNumber = 0;


static bool
hasFeature(const char** aFeatures, uint32_t aFeatureCount, const char* aFeature)
{
  for (size_t i = 0; i < aFeatureCount; i++) {
    if (0) LOGF("hasFeature %s: %lu %s", aFeature, (unsigned long)i, aFeatures[i]);
    if (strcmp(aFeatures[i], aFeature) == 0)
      return true;
  }
  return false;
}

class TableTicker2: public Sampler {
 public:
  TableTicker2(int aInterval, int aEntrySize, PseudoStack *aStack,
              const char** aFeatures, uint32_t aFeatureCount)
    : Sampler(aInterval, true)
    , mPrimaryThreadProfile(aEntrySize, aStack)
    , mStartTime(TimeStamp::Now())
    , mSaveRequested(false)
  {
    mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");

    
    mJankOnly = hasFeature(aFeatures, aFeatureCount, "jank");
    mProfileJS = hasFeature(aFeatures, aFeatureCount, "js");
    mPrimaryThreadProfile.addTag(ProfileEntry2('m', "Start"));
  }

  ~TableTicker2() { if (IsActive()) Stop(); }

  virtual void SampleStack(TickSample* sample) {}

  
  virtual void Tick(TickSample* sample);

  
  virtual void RequestSave()
  {
    mSaveRequested = true;
  }

  virtual void HandleSaveRequest();

  ThreadProfile2* GetPrimaryThreadProfile()
  {
    return &mPrimaryThreadProfile;
  }

  JSObject *ToJSObject(JSContext *aCx);
  JSCustomObject *GetMetaJSCustomObject(JSAObjectBuilder& b);

  const bool ProfileJS() { return mProfileJS; }

private:
  
  
  void doNativeBacktrace(ThreadProfile2 &aProfile, TickSample* aSample);

private:
  
  ThreadProfile2 mPrimaryThreadProfile;
  TimeStamp mStartTime;
  bool mSaveRequested;
  bool mUseStackWalk;
  bool mJankOnly;
  bool mProfileJS;
};





std::string GetSharedLibraryInfoString();

static JSBool
WriteCallback(const jschar *buf, uint32_t len, void *data)
{
  std::ofstream& stream = *static_cast<std::ofstream*>(data);
  nsAutoCString profile = NS_ConvertUTF16toUTF8(buf, len);
  stream << profile.Data();
  return JS_TRUE;
}





class SaveProfileTask : public nsRunnable {
public:
  SaveProfileTask() {}

  NS_IMETHOD Run() {
    TableTicker2 *t = tlsTicker.get();

    
    
    
    
    
    t->SetPaused(true);

    
#   if defined(SPS_PLAT_arm_android)
    nsCString tmpPath;
    tmpPath.AppendPrintf("/sdcard/profile_%i_%i.txt", XRE_GetProcessType(), getpid());
#   else
    nsCOMPtr<nsIFile> tmpFile;
    nsAutoCString tmpPath;
    if (NS_FAILED(NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmpFile)))) {
      LOG("Failed to find temporary directory.");
      return NS_ERROR_FAILURE;
    }
    tmpPath.AppendPrintf("profile_%i_%i.txt", XRE_GetProcessType(), getpid());

    nsresult rv = tmpFile->AppendNative(tmpPath);
    if (NS_FAILED(rv))
      return rv;

    rv = tmpFile->GetNativePath(tmpPath);
    if (NS_FAILED(rv))
      return rv;
#   endif

    
    
    JSRuntime *rt;
    JSContext *cx;
    nsCOMPtr<nsIJSRuntimeService> rtsvc 
      = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    if (!rtsvc || NS_FAILED(rtsvc->GetRuntime(&rt)) || !rt) {
      LOG("failed to get RuntimeService");
      return NS_ERROR_FAILURE;;
    }

    cx = JS_NewContext(rt, 8192);
    if (!cx) {
      LOG("Failed to get context");
      return NS_ERROR_FAILURE;
    }

    {
      JSAutoRequest ar(cx);
      static JSClass c = {
          "global", JSCLASS_GLOBAL_FLAGS,
          JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
          JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub
      };
      JSObject *obj = JS_NewGlobalObject(cx, &c, NULL);

      std::ofstream stream;
      stream.open(tmpPath.get());
      
      
      
      
      
      t->SetPaused(true);
      if (stream.is_open()) {
        JSAutoCompartment autoComp(cx, obj);
        JSObject* profileObj = mozilla_sampler_get_profile_data2(cx);
        jsval val = OBJECT_TO_JSVAL(profileObj);
        JS_Stringify(cx, &val, nullptr, JSVAL_NULL, WriteCallback, &stream);
        stream.close();
        LOGF("Saved to %s", tmpPath.get());
      } else {
        LOG("Fail to open profile log file.");
      }
    }
    JS_EndRequest(cx);
    JS_DestroyContext(cx);

    t->SetPaused(false);

    return NS_OK;
  }
};

void TableTicker2::HandleSaveRequest()
{
  if (!mSaveRequested)
    return;
  mSaveRequested = false;

  
  
  nsCOMPtr<nsIRunnable> runnable = new SaveProfileTask();
  NS_DispatchToMainThread(runnable);
}

JSCustomObject* TableTicker2::GetMetaJSCustomObject(JSAObjectBuilder& b)
{
  JSCustomObject *meta = b.CreateObject();

  b.DefineProperty(meta, "version", 2);
  b.DefineProperty(meta, "interval", interval());
  b.DefineProperty(meta, "stackwalk", mUseStackWalk);
  b.DefineProperty(meta, "jank", mJankOnly);
  b.DefineProperty(meta, "processType", XRE_GetProcessType());

  nsresult res;
  nsCOMPtr<nsIHttpProtocolHandler> http
    = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &res);
  if (!NS_FAILED(res)) {
    nsAutoCString string;

    res = http->GetPlatform(string);
    if (!NS_FAILED(res))
      b.DefineProperty(meta, "platform", string.Data());

    res = http->GetOscpu(string);
    if (!NS_FAILED(res))
      b.DefineProperty(meta, "oscpu", string.Data());

    res = http->GetMisc(string);
    if (!NS_FAILED(res))
      b.DefineProperty(meta, "misc", string.Data());
  }

  nsCOMPtr<nsIXULRuntime> runtime = do_GetService("@mozilla.org/xre/runtime;1");
  if (runtime) {
    nsAutoCString string;

    res = runtime->GetXPCOMABI(string);
    if (!NS_FAILED(res))
      b.DefineProperty(meta, "abi", string.Data());

    res = runtime->GetWidgetToolkit(string);
    if (!NS_FAILED(res))
      b.DefineProperty(meta, "toolkit", string.Data());
  }

  nsCOMPtr<nsIXULAppInfo> appInfo = do_GetService("@mozilla.org/xre/app-info;1");
  if (appInfo) {
    nsAutoCString string;

    res = appInfo->GetName(string);
    if (!NS_FAILED(res))
      b.DefineProperty(meta, "product", string.Data());
  }

  return meta;
}

JSObject* TableTicker2::ToJSObject(JSContext *aCx)
{
  JSObjectBuilder b(aCx);

  JSCustomObject *profile = b.CreateObject();

  
  b.DefineProperty(profile, "libs", GetSharedLibraryInfoString().c_str());

  
  JSCustomObject *meta = GetMetaJSCustomObject(b);
  b.DefineProperty(profile, "meta", meta);

  
  JSCustomArray *threads = b.CreateArray();
  b.DefineProperty(profile, "threads", threads);

  
  SetPaused(true);
  ThreadProfile2* prof = GetPrimaryThreadProfile();
  prof->GetMutex()->Lock();
  JSCustomObject* threadSamples = prof->ToJSObject(aCx);
  b.ArrayPush(threads, threadSamples);
  prof->GetMutex()->Unlock();
  SetPaused(false);

  return b.GetJSObject(profile);
}










static
void genProfileEntry2(UnwinderThreadBuffer* utb,
                     volatile StackEntry &entry,
                     PseudoStack *stack, void *lastpc)
{
  int lineno = -1;

  
  utb__addEntry( utb, ProfileEntry2('h', 'P') );
  
  if (entry.stackAddress() != 0) {
    utb__addEntry( utb, ProfileEntry2('S', entry.stackAddress()) );
  }

  
  
  const char* sampleLabel = entry.label();
  if (entry.isCopyLabel()) {
    
    

    utb__addEntry( utb, ProfileEntry2('c', "") );
    
    size_t strLen = strlen(sampleLabel) + 1;
    for (size_t j = 0; j < strLen;) {
      
      char text[sizeof(void*)];
      for (size_t pos = 0; pos < sizeof(void*) && j+pos < strLen; pos++) {
        text[pos] = sampleLabel[j+pos];
      }
      j += sizeof(void*)/sizeof(char);
      
      utb__addEntry( utb, ProfileEntry2('d', *((void**)(&text[0]))) );
    }
    if (entry.js()) {
      if (!entry.pc()) {
        
        MOZ_ASSERT(&entry == &stack->mStack[stack->stackSize() - 1]);
        
        if (lastpc) {
          jsbytecode *jspc = js::ProfilingGetPC(stack->mRuntime, entry.script(),
                                                lastpc);
          if (jspc) {
            lineno = JS_PCToLineNumber(NULL, entry.script(), jspc);
          }
        }
      } else {
        lineno = JS_PCToLineNumber(NULL, entry.script(), entry.pc());
      }
    } else {
      lineno = entry.line();
    }
  } else {
    utb__addEntry( utb, ProfileEntry2('c', sampleLabel) );
    lineno = entry.line();
  }
  if (lineno != -1) {
    utb__addEntry( utb, ProfileEntry2('n', lineno) );
  }

  
  utb__addEntry( utb, ProfileEntry2('h', 'Q') );
}




void genPseudoBacktraceEntries(UnwinderThreadBuffer* utb,
                               PseudoStack *aStack, TickSample *sample)
{
  
  
  
  uint32_t nInStack = aStack->stackSize();
  for (uint32_t i = 0; i < nInStack; i++) {
    genProfileEntry2(utb, aStack->mStack[i], aStack, nullptr);
  }
# ifdef ENABLE_SPS_LEAF_DATA
  if (sample) {
    utb__addEntry( utb, ProfileEntry2('l', (void*)sample->pc) );
#   ifdef ENABLE_ARM_LR_SAVING
    utb__addEntry( utb, ProfileEntry2('L', (void*)sample->lr) );
#   endif
  }
# endif
}


static unsigned int sLastSampledEventGeneration = 0;




static unsigned int sCurrentEventGeneration = 0;






void TableTicker2::Tick(TickSample* sample)
{
  

  UnwinderThreadBuffer* utb = uwt__acquire_empty_buffer();

  


  if (!utb)
    return;

  


  
  PseudoStack* stack = mPrimaryThreadProfile.GetPseudoStack();
  for (int i = 0; stack->getMarker(i) != NULL; i++) {
    utb__addEntry( utb, ProfileEntry2('m', stack->getMarker(i)) );
  }
  stack->mQueueClearMarker = true;

  bool recordSample = true;
  if (mJankOnly) {
    
    
    if (sLastSampledEventGeneration != sCurrentEventGeneration) {
      
      
      
      mPrimaryThreadProfile.erase();
    }
    sLastSampledEventGeneration = sCurrentEventGeneration;

    recordSample = false;
    
    
    if (!sLastTracerEvent.IsNull()) {
      TimeDuration delta = sample->timestamp - sLastTracerEvent;
      if (delta.ToMilliseconds() > 100.0) {
          recordSample = true;
      }
    }
  }

  
  
  
  
  
  switch (sUnwindMode) {
    case UnwNATIVE: 
      
      
      
      utb__addEntry( utb, ProfileEntry2('h', 'N') );
      break;
    case UnwPSEUDO: 
      
      genPseudoBacktraceEntries(utb, stack, sample);
      break;
    case UnwCOMBINED: 
      utb__addEntry( utb, ProfileEntry2('h', 'N') );
      genPseudoBacktraceEntries(utb, stack, sample);
      break;
    case UnwINVALID:
    default:
      MOZ_CRASH();
  }

  if (recordSample) {    
    
    utb__addEntry( utb, ProfileEntry2('h', 'F') );
  }

  
  if (!sLastTracerEvent.IsNull() && sample) {
    TimeDuration delta = sample->timestamp - sLastTracerEvent;
    utb__addEntry( utb, ProfileEntry2('r', delta.ToMilliseconds()) );
  }

  if (sample) {
    TimeDuration delta = sample->timestamp - mStartTime;
    utb__addEntry( utb, ProfileEntry2('t', delta.ToMilliseconds()) );
  }

  if (sLastFrameNumber != sFrameNumber) {
    utb__addEntry( utb, ProfileEntry2('f', sFrameNumber) );
    sLastFrameNumber = sFrameNumber;
  }

  







  



  


  if (sUnwindMode == UnwNATIVE || sUnwindMode == UnwCOMBINED) {
#   if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
       || defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_android)
    void* ucV = (void*)sample->context;
#   elif defined(SPS_PLAT_amd64_darwin)
    struct __darwin_mcontext64 mc;
    memset(&mc, 0, sizeof(mc));
    ucontext_t uc;
    memset(&uc, 0, sizeof(uc));
    uc.uc_mcontext = &mc;
    mc.__ss.__rip = (uint64_t)sample->pc;
    mc.__ss.__rsp = (uint64_t)sample->sp;
    mc.__ss.__rbp = (uint64_t)sample->fp;
    void* ucV = (void*)&uc;
#   elif defined(SPS_PLAT_x86_darwin)
    struct __darwin_mcontext32 mc;
    memset(&mc, 0, sizeof(mc));
    ucontext_t uc;
    memset(&uc, 0, sizeof(uc));
    uc.uc_mcontext = &mc;
    mc.__ss.__eip = (uint32_t)sample->pc;
    mc.__ss.__esp = (uint32_t)sample->sp;
    mc.__ss.__ebp = (uint32_t)sample->fp;
    void* ucV = (void*)&uc;
#   elif defined(SPS_OS_windows)
    

    void* ucV = NULL;
#   else
#     error "Unsupported platform"
#   endif
    uwt__release_full_buffer(&mPrimaryThreadProfile, utb, ucV);
  } else {
    uwt__release_full_buffer(&mPrimaryThreadProfile, utb, NULL);
  }
}





std::ostream& operator<<(std::ostream& stream, const ThreadProfile2& profile)
{
  int readPos = profile.mReadPos;
  while (readPos != profile.mLastFlushPos) {
    stream << profile.mEntries[readPos];
    readPos = (readPos + 1) % profile.mEntrySize;
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ProfileEntry2& entry)
{
  if (entry.mTagName == 'r' || entry.mTagName == 't') {
    stream << entry.mTagName << "-" << std::fixed << entry.mTagFloat << "\n";
  } else if (entry.mTagName == 'l' || entry.mTagName == 'L') {
    
    
    char tagBuff[1024];
    unsigned long long pc = (unsigned long long)(uintptr_t)entry.mTagPtr;
    snprintf(tagBuff, 1024, "%c-%#llx\n", entry.mTagName, pc);
    stream << tagBuff;
  } else if (entry.mTagName == 'd') {
    
  } else {
    stream << entry.mTagName << "-" << entry.mTagData << "\n";
  }
  return stream;
}

static const char* name_UnwMode(UnwMode m)
{
  switch (m) {
    case UnwINVALID:  return "invalid";
    case UnwNATIVE:   return "native";
    case UnwPSEUDO:   return "pseudo";
    case UnwCOMBINED: return "combined";
    default:          return "??name_UnwMode??";
  }
}


static void read_env_vars()
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





void mozilla_sampler_init2()
{
  if (stack_key_initialized)
    return;

  LOG("BEGIN mozilla_sampler_init2");
  if (!tlsPseudoStack.init() || !tlsTicker.init()) {
    LOG("Failed to init.");
    return;
  }
  stack_key_initialized = true;

  PseudoStack *stack = new PseudoStack();
  tlsPseudoStack.set(stack);

  
  
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

  
  OS::RegisterStartHandler();

  
  
  
  const char *val = PR_GetEnv("MOZ_PROFILER_STARTUP");
  if (!val || !*val) {
    return;
  }

  const char* features[] = {"js"
#if defined(XP_WIN) || defined(XP_MACOSX)
                         , "stackwalk"
#endif
                         };
  mozilla_sampler_start2(PROFILE_DEFAULT_ENTRY, PROFILE_DEFAULT_INTERVAL,
                         features, sizeof(features)/sizeof(const char*));
  LOG("END   mozilla_sampler_init2");
}

void mozilla_sampler_shutdown2()
{
  
  
  
  
  uwt__deinit();

  mozilla_sampler_stop2();
  
  
  
}

void mozilla_sampler_save2()
{
  TableTicker2 *t = tlsTicker.get();
  if (!t) {
    return;
  }

  t->RequestSave();
  
  
  t->HandleSaveRequest();
}

char* mozilla_sampler_get_profile2()
{
  TableTicker2 *t = tlsTicker.get();
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

JSObject *mozilla_sampler_get_profile_data2(JSContext *aCx)
{
  TableTicker2 *t = tlsTicker.get();
  if (!t) {
    return NULL;
  }

  return t->ToJSObject(aCx);
}


const char** mozilla_sampler_get_features2()
{
  static const char* features[] = {
#if defined(MOZ_PROFILING) && defined(HAVE_NATIVE_UNWIND)
    "stackwalk",
#endif
    "jank",
    "js",
    NULL
  };

  return features;
}


void mozilla_sampler_start2(int aProfileEntries, int aInterval,
                            const char** aFeatures, uint32_t aFeatureCount)
{
  if (!stack_key_initialized)
    mozilla_sampler_init2();

  

  if (sUnwindInterval > 0)
    aInterval = sUnwindInterval;

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    ASSERT(false);
    return;
  }

  mozilla_sampler_stop2();
  TableTicker2 *t
    = new TableTicker2(aInterval ? aInterval : PROFILE_DEFAULT_INTERVAL,
                      aProfileEntries ? aProfileEntries : PROFILE_DEFAULT_ENTRY,
                      stack, aFeatures, aFeatureCount);
  tlsTicker.set(t);
  t->Start();
  if (t->ProfileJS())
      stack->enableJSSampling();

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->NotifyObservers(nullptr, "profiler-started", nullptr);
}

void mozilla_sampler_stop2()
{
  if (!stack_key_initialized)
    mozilla_sampler_init2();

  TableTicker2 *t = tlsTicker.get();
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

bool mozilla_sampler_is_active2()
{
  if (!stack_key_initialized)
    mozilla_sampler_init2();

  TableTicker2 *t = tlsTicker.get();
  if (!t) {
    return false;
  }

  return t->IsActive();
}

static double sResponsivenessTimes[100];
static unsigned int sResponsivenessLoc = 0;
void mozilla_sampler_responsiveness2(TimeStamp aTime)
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

const double* mozilla_sampler_get_responsiveness2()
{
  return sResponsivenessTimes;
}

void mozilla_sampler_frame_number2(int frameNumber)
{
  sFrameNumber = frameNumber;
}

void mozilla_sampler_print_location2()
{
  
}

void mozilla_sampler_lock2()
{
  
}

void mozilla_sampler_unlock2()
{
  
}



