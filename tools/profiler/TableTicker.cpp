




#include <string>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include "GeckoProfiler.h"
#include "SaveProfileTask.h"
#include "ProfileEntry.h"
#include "SyncProfile.h"
#include "platform.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "prtime.h"
#include "shared-libraries.h"
#include "mozilla/StackWalk.h"
#include "TableTicker.h"
#include "nsXULAppAPI.h"


#include "JSObjectBuilder.h"
#include "JSCustomObjectBuilder.h"


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
#include "PlatformMacros.h"

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
  #include "AndroidBridge.h"
#endif


#include "js/OldDebugAPI.h"

#if defined(MOZ_PROFILING) && (defined(XP_MACOSX) || defined(XP_WIN))
 #define USE_NS_STACKWALK
#endif
#ifdef USE_NS_STACKWALK
 #include "nsStackWalk.h"
#endif

#if defined(XP_WIN)
typedef CONTEXT tickcontext_t;
#elif defined(LINUX)
#include <ucontext.h>
typedef ucontext_t tickcontext_t;
#endif

#if defined(LINUX) || defined(XP_MACOSX)
#include <sys/types.h>
pid_t gettid();
#endif

#if defined(SPS_ARCH_arm) && defined(MOZ_WIDGET_GONK)
 
 #define USE_EHABI_STACKWALK
#endif
#ifdef USE_EHABI_STACKWALK
 #include "EHABIStackWalk.h"
#endif

using std::string;
using namespace mozilla;

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




std::string GetSharedLibraryInfoString();

void TableTicker::HandleSaveRequest()
{
  if (!mSaveRequested)
    return;
  mSaveRequested = false;

  
  
  nsCOMPtr<nsIRunnable> runnable = new SaveProfileTask();
  NS_DispatchToMainThread(runnable);
}

template <typename Builder>
typename Builder::Object TableTicker::GetMetaJSCustomObject(Builder& b)
{
  typename Builder::RootedObject meta(b.context(), b.CreateObject());

  b.DefineProperty(meta, "version", 2);
  b.DefineProperty(meta, "interval", interval());
  b.DefineProperty(meta, "stackwalk", mUseStackWalk);
  b.DefineProperty(meta, "jank", mJankOnly);
  b.DefineProperty(meta, "processType", XRE_GetProcessType());

  TimeDuration delta = TimeStamp::Now() - sStartTime;
  b.DefineProperty(meta, "startTime", static_cast<float>(PR_Now()/1000.0 - delta.ToMilliseconds()));

  nsresult res;
  nsCOMPtr<nsIHttpProtocolHandler> http = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &res);
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

void TableTicker::ToStreamAsJSON(std::ostream& stream)
{
  JSCustomObjectBuilder b;
  JSCustomObject* profile = b.CreateObject();
  BuildJSObject(b, profile);
  b.Serialize(profile, stream);
  b.DeleteObject(profile);
}

JSObject* TableTicker::ToJSObject(JSContext *aCx)
{
  JSObjectBuilder b(aCx);
  JS::RootedObject profile(aCx, b.CreateObject());
  BuildJSObject(b, profile);
  return profile;
}

template <typename Builder>
struct SubprocessClosure {
  SubprocessClosure(Builder *aBuilder, typename Builder::ArrayHandle aThreads)
    : mBuilder(aBuilder), mThreads(aThreads)
  {}

  Builder* mBuilder;
  typename Builder::ArrayHandle mThreads;
};

template <typename Builder>
void SubProcessCallback(const char* aProfile, void* aClosure)
{
  
  
  SubprocessClosure<Builder>* closure = (SubprocessClosure<Builder>*)aClosure;

  closure->mBuilder->ArrayPush(closure->mThreads, aProfile);
}

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
template <typename Builder>
static
typename Builder::Object BuildJavaThreadJSObject(Builder& b)
{
  typename Builder::RootedObject javaThread(b.context(), b.CreateObject());
  b.DefineProperty(javaThread, "name", "Java Main Thread");

  typename Builder::RootedArray samples(b.context(), b.CreateArray());
  b.DefineProperty(javaThread, "samples", samples);

  int sampleId = 0;
  while (true) {
    int frameId = 0;
    typename Builder::RootedObject sample(b.context());
    typename Builder::RootedArray frames(b.context());
    while (true) {
      nsCString result;
      bool hasFrame = AndroidBridge::Bridge()->GetFrameNameJavaProfiling(0, sampleId, frameId, result);
      if (!hasFrame) {
        if (frames) {
          b.DefineProperty(sample, "frames", frames);
        }
        break;
      }
      if (!sample) {
        sample = b.CreateObject();
        frames = b.CreateArray();
        b.DefineProperty(sample, "frames", frames);
        b.ArrayPush(samples, sample);

        double sampleTime =
          mozilla::widget::android::GeckoJavaSampler::GetSampleTimeJavaProfiling(0, sampleId);
        b.DefineProperty(sample, "time", sampleTime);
      }
      typename Builder::RootedObject frame(b.context(), b.CreateObject());
      b.DefineProperty(frame, "location", result.BeginReading());
      b.ArrayPush(frames, frame);
      frameId++;
    }
    if (frameId == 0) {
      break;
    }
    sampleId++;
  }

  return javaThread;
}
#endif

template <typename Builder>
void TableTicker::BuildJSObject(Builder& b, typename Builder::ObjectHandle profile)
{
  
  b.DefineProperty(profile, "libs", GetSharedLibraryInfoString().c_str());

  
  typename Builder::RootedObject meta(b.context(), GetMetaJSCustomObject(b));
  b.DefineProperty(profile, "meta", meta);

  
  typename Builder::RootedArray threads(b.context(), b.CreateArray());
  b.DefineProperty(profile, "threads", threads);

  SetPaused(true);

  {
    mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

    for (size_t i = 0; i < sRegisteredThreads->size(); i++) {
      
      if (!sRegisteredThreads->at(i)->Profile())
        continue;

      MutexAutoLock lock(*sRegisteredThreads->at(i)->Profile()->GetMutex());

      typename Builder::RootedObject threadSamples(b.context(), b.CreateObject());
      sRegisteredThreads->at(i)->Profile()->BuildJSObject(b, threadSamples);
      b.ArrayPush(threads, threadSamples);
    }
  }

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
  if (ProfileJava()) {
    mozilla::widget::android::GeckoJavaSampler::PauseJavaProfiling();

    typename Builder::RootedObject javaThread(b.context(), BuildJavaThreadJSObject(b));
    b.ArrayPush(threads, javaThread);

    mozilla::widget::android::GeckoJavaSampler::UnpauseJavaProfiling();
  }
#endif

  SetPaused(false);

  
  
  SubprocessClosure<Builder> closure(&b, threads);
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    nsRefPtr<ProfileSaveEvent> pse = new ProfileSaveEvent(SubProcessCallback<Builder>, &closure);
    os->NotifyObservers(pse, "profiler-subprocess", nullptr);
  }
}




static
void addDynamicTag(ThreadProfile &aProfile, char aTagName, const char *aStr)
{
  aProfile.addTag(ProfileEntry(aTagName, ""));
  
  size_t strLen = strlen(aStr) + 1;
  for (size_t j = 0; j < strLen;) {
    
    char text[sizeof(void*)];
    size_t len = sizeof(void*)/sizeof(char);
    if (j+len >= strLen) {
      len = strLen - j;
    }
    memcpy(text, &aStr[j], len);
    j += sizeof(void*)/sizeof(char);
    
    aProfile.addTag(ProfileEntry('d', *((void**)(&text[0]))));
  }
}

static
void addProfileEntry(volatile StackEntry &entry, ThreadProfile &aProfile,
                     PseudoStack *stack, void *lastpc)
{
  int lineno = -1;

  
  
  const char* sampleLabel = entry.label();
  if (entry.isCopyLabel()) {
    
    

    addDynamicTag(aProfile, 'c', sampleLabel);
    if (entry.js()) {
      if (!entry.pc()) {
        
        MOZ_ASSERT(&entry == &stack->mStack[stack->stackSize() - 1]);
        
        if (lastpc) {
          jsbytecode *jspc = js::ProfilingGetPC(stack->mRuntime, entry.script(),
                                                lastpc);
          if (jspc) {
            lineno = JS_PCToLineNumber(nullptr, entry.script(), jspc);
          }
        }
      } else {
        lineno = JS_PCToLineNumber(nullptr, entry.script(), entry.pc());
      }
    } else {
      lineno = entry.line();
    }
  } else {
    aProfile.addTag(ProfileEntry('c', sampleLabel));
    lineno = entry.line();
  }
  if (lineno != -1) {
    aProfile.addTag(ProfileEntry('n', lineno));
  }
}

#if defined(USE_NS_STACKWALK) || defined(USE_EHABI_STACKWALK)
typedef struct {
  void** array;
  void** sp_array;
  size_t size;
  size_t count;
} PCArray;

static void mergeNativeBacktrace(ThreadProfile &aProfile, const PCArray &array) {
  aProfile.addTag(ProfileEntry('s', "(root)"));

  PseudoStack* stack = aProfile.GetPseudoStack();
  uint32_t pseudoStackPos = 0;

  














  
  
  
  for (size_t i = array.count; i > 0; --i) {
    while (pseudoStackPos < stack->stackSize()) {
      volatile StackEntry& entry = stack->mStack[pseudoStackPos];

      if (entry.stackAddress() < array.sp_array[i-1] && entry.stackAddress())
        break;

      addProfileEntry(entry, aProfile, stack, array.array[0]);
      pseudoStackPos++;
    }

    aProfile.addTag(ProfileEntry('l', (void*)array.array[i-1]));
  }
}

#endif

#ifdef USE_NS_STACKWALK
static
void StackWalkCallback(void* aPC, void* aSP, void* aClosure)
{
  PCArray* array = static_cast<PCArray*>(aClosure);
  MOZ_ASSERT(array->count < array->size);
  array->sp_array[array->count] = aSP;
  array->array[array->count] = aPC;
  array->count++;
}

void TableTicker::doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample)
{
#ifndef XP_MACOSX
  uintptr_t thread = GetThreadHandle(aSample->threadProfile->GetPlatformData());
  MOZ_ASSERT(thread);
#endif
  void* pc_array[1000];
  void* sp_array[1000];
  PCArray array = {
    pc_array,
    sp_array,
    mozilla::ArrayLength(pc_array),
    0
  };

  
  StackWalkCallback(aSample->pc, aSample->sp, &array);

  uint32_t maxFrames = uint32_t(array.size - array.count);
#ifdef XP_MACOSX
  pthread_t pt = GetProfiledThread(aSample->threadProfile->GetPlatformData());
  void *stackEnd = reinterpret_cast<void*>(-1);
  if (pt)
    stackEnd = static_cast<char*>(pthread_get_stackaddr_np(pt));
  nsresult rv = NS_OK;
  if (aSample->fp >= aSample->sp && aSample->fp <= stackEnd)
    rv = FramePointerStackWalk(StackWalkCallback,  0,
                               maxFrames, &array,
                               reinterpret_cast<void**>(aSample->fp), stackEnd);
#else
  void *platformData = nullptr;
#ifdef XP_WIN
  if (aSample->isSamplingCurrentThread) {
    
    
    thread = 0;
  }
  platformData = aSample->context;
#endif 

  nsresult rv = NS_StackWalk(StackWalkCallback,  0, maxFrames,
                             &array, thread, platformData);
#endif
  if (NS_SUCCEEDED(rv))
    mergeNativeBacktrace(aProfile, array);
}
#endif

#ifdef USE_EHABI_STACKWALK
void TableTicker::doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample)
{
  void *pc_array[1000];
  void *sp_array[1000];
  PCArray array = {
    pc_array,
    sp_array,
    mozilla::ArrayLength(pc_array),
    0
  };

  const mcontext_t *mcontext = &reinterpret_cast<ucontext_t *>(aSample->context)->uc_mcontext;
  mcontext_t savedContext;
  PseudoStack *pseudoStack = aProfile.GetPseudoStack();

  array.count = 0;
  
  
  
  
  for (uint32_t i = pseudoStack->stackSize(); i > 0; --i) {
    
    
    volatile StackEntry &entry = pseudoStack->mStack[i - 1];
    if (!entry.js() && strcmp(entry.label(), "EnterJIT") == 0) {
      
      
      
      
      uint32_t *vSP = reinterpret_cast<uint32_t*>(entry.stackAddress());

      array.count += EHABIStackWalk(*mcontext,
                                     vSP,
                                    sp_array + array.count,
                                    pc_array + array.count,
                                    array.size - array.count);

      memset(&savedContext, 0, sizeof(savedContext));
      
      savedContext.arm_r4 = *vSP++;
      savedContext.arm_r5 = *vSP++;
      savedContext.arm_r6 = *vSP++;
      savedContext.arm_r7 = *vSP++;
      savedContext.arm_r8 = *vSP++;
      savedContext.arm_r9 = *vSP++;
      savedContext.arm_r10 = *vSP++;
      savedContext.arm_fp = *vSP++;
      savedContext.arm_lr = *vSP++;
      savedContext.arm_sp = reinterpret_cast<uint32_t>(vSP);
      savedContext.arm_pc = savedContext.arm_lr;
      mcontext = &savedContext;
    }
  }

  
  
  array.count += EHABIStackWalk(*mcontext,
                                aProfile.GetStackTop(),
                                sp_array + array.count,
                                pc_array + array.count,
                                array.size - array.count);

  mergeNativeBacktrace(aProfile, array);
}

#endif

static
void doSampleStackTrace(PseudoStack *aStack, ThreadProfile &aProfile, TickSample *sample)
{
  
  
  
  aProfile.addTag(ProfileEntry('s', "(root)"));
  for (uint32_t i = 0; i < aStack->stackSize(); i++) {
    addProfileEntry(aStack->mStack[i], aProfile, aStack, nullptr);
  }
#ifdef ENABLE_SPS_LEAF_DATA
  if (sample) {
    aProfile.addTag(ProfileEntry('l', (void*)sample->pc));
#ifdef ENABLE_ARM_LR_SAVING
    aProfile.addTag(ProfileEntry('L', (void*)sample->lr));
#endif
  }
#endif
}

void TableTicker::Tick(TickSample* sample)
{
  if (HasUnwinderThread()) {
    UnwinderTick(sample);
  } else {
    InplaceTick(sample);
  }
}

void TableTicker::InplaceTick(TickSample* sample)
{
  ThreadProfile& currThreadProfile = *sample->threadProfile;

  PseudoStack* stack = currThreadProfile.GetPseudoStack();
  bool recordSample = true;
#if defined(XP_WIN)
  bool powerSample = false;
#endif

  

  if (!sample->isSamplingCurrentThread) {
    
    ProfilerMarkerLinkedList* pendingMarkersList = stack->getPendingMarkers();
    while (pendingMarkersList && pendingMarkersList->peek()) {
      ProfilerMarker* marker = pendingMarkersList->popHead();
      stack->addStoredMarker(marker);
      currThreadProfile.addTag(ProfileEntry('m', marker));
    }
    stack->updateGeneration(currThreadProfile.GetGenerationID());

#if defined(XP_WIN)
    if (mProfilePower) {
      mIntelPowerGadget->TakeSample();
      powerSample = true;
    }
#endif

    if (mJankOnly) {
      
      
      if (sLastSampledEventGeneration != sCurrentEventGeneration) {
        
        
        
        currThreadProfile.erase();
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
  }

#if defined(USE_NS_STACKWALK) || defined(USE_EHABI_STACKWALK)
  if (mUseStackWalk) {
    doNativeBacktrace(currThreadProfile, sample);
  } else {
    doSampleStackTrace(stack, currThreadProfile, mAddLeafAddresses ? sample : nullptr);
  }
#else
  doSampleStackTrace(stack, currThreadProfile, mAddLeafAddresses ? sample : nullptr);
#endif

  if (recordSample)
    currThreadProfile.flush();

  if (!sLastTracerEvent.IsNull() && sample && currThreadProfile.IsMainThread()) {
    TimeDuration delta = sample->timestamp - sLastTracerEvent;
    currThreadProfile.addTag(ProfileEntry('r', static_cast<float>(delta.ToMilliseconds())));
  }

  if (sample) {
    TimeDuration delta = sample->timestamp - sStartTime;
    currThreadProfile.addTag(ProfileEntry('t', static_cast<float>(delta.ToMilliseconds())));
  }

#if defined(XP_WIN)
  if (powerSample) {
    currThreadProfile.addTag(ProfileEntry('p', static_cast<float>(mIntelPowerGadget->GetTotalPackagePowerInWatts())));
  }
#endif

  if (sLastFrameNumber != sFrameNumber) {
    currThreadProfile.addTag(ProfileEntry('f', sFrameNumber));
    sLastFrameNumber = sFrameNumber;
  }
}

namespace {

SyncProfile* NewSyncProfile()
{
  PseudoStack* stack = tlsPseudoStack.get();
  if (!stack) {
    MOZ_ASSERT(stack);
    return nullptr;
  }
  Thread::tid_t tid = Thread::GetCurrentId();

  SyncProfile* profile = new SyncProfile("SyncProfile",
                                         GET_BACKTRACE_DEFAULT_ENTRY,
                                         stack, tid, NS_IsMainThread());
  return profile;
}

} 

SyncProfile* TableTicker::GetBacktrace()
{
  SyncProfile* profile = NewSyncProfile();

  TickSample sample;
  sample.threadProfile = profile;

#if defined(HAVE_NATIVE_UNWIND)
#if defined(XP_WIN) || defined(LINUX)
  tickcontext_t context;
  sample.PopulateContext(&context);
#elif defined(XP_MACOSX)
  sample.PopulateContext(nullptr);
#endif
#endif

  sample.isSamplingCurrentThread = true;
  sample.timestamp = mozilla::TimeStamp::Now();

  if (!HasUnwinderThread()) {
    profile->BeginUnwind();
  }

  Tick(&sample);

  if (!HasUnwinderThread()) {
    profile->EndUnwind();
  }

  return profile;
}

static void print_callback(const ProfileEntry& entry, const char* tagStringData)
{
  switch (entry.getTagName()) {
    case 's':
    case 'c':
      printf_stderr("  %s\n", tagStringData);
  }
}

void mozilla_sampler_print_location1()
{
  if (!stack_key_initialized)
    profiler_init(nullptr);

  SyncProfile* syncProfile = NewSyncProfile();
  if (!syncProfile) {
    return;
  }

  syncProfile->BeginUnwind();
  doSampleStackTrace(syncProfile->GetPseudoStack(), *syncProfile, nullptr);
  syncProfile->EndUnwind();

  printf_stderr("Backtrace:\n");
  syncProfile->IterateTags(print_callback);
  delete syncProfile;
}


