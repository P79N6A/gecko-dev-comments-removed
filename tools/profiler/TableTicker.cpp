




#include <algorithm>
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


#include "JSStreamWriter.h"


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


#include "jsfriendapi.h"
#include "js/ProfilingFrameIterator.h"

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

void TableTicker::StreamMetaJSCustomObject(JSStreamWriter& b)
{
  b.BeginObject();

    b.NameValue("version", 2);
    b.NameValue("interval", interval());
    b.NameValue("stackwalk", mUseStackWalk);
    b.NameValue("jank", mJankOnly);
    b.NameValue("processType", XRE_GetProcessType());

    mozilla::TimeDuration delta = mozilla::TimeStamp::Now() - sStartTime;
    b.NameValue("startTime", static_cast<double>(PR_Now()/1000.0 - delta.ToMilliseconds()));

    nsresult res;
    nsCOMPtr<nsIHttpProtocolHandler> http = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &res);
    if (!NS_FAILED(res)) {
      nsAutoCString string;

      res = http->GetPlatform(string);
      if (!NS_FAILED(res))
        b.NameValue("platform", string.Data());

      res = http->GetOscpu(string);
      if (!NS_FAILED(res))
        b.NameValue("oscpu", string.Data());

      res = http->GetMisc(string);
      if (!NS_FAILED(res))
        b.NameValue("misc", string.Data());
    }

    nsCOMPtr<nsIXULRuntime> runtime = do_GetService("@mozilla.org/xre/runtime;1");
    if (runtime) {
      nsAutoCString string;

      res = runtime->GetXPCOMABI(string);
      if (!NS_FAILED(res))
        b.NameValue("abi", string.Data());

      res = runtime->GetWidgetToolkit(string);
      if (!NS_FAILED(res))
        b.NameValue("toolkit", string.Data());
    }

    nsCOMPtr<nsIXULAppInfo> appInfo = do_GetService("@mozilla.org/xre/app-info;1");
    if (appInfo) {
      nsAutoCString string;

      res = appInfo->GetName(string);
      if (!NS_FAILED(res))
        b.NameValue("product", string.Data());
    }

  b.EndObject();
}

void TableTicker::ToStreamAsJSON(std::ostream& stream)
{
  JSStreamWriter b(stream);
  StreamJSObject(b);
}

JSObject* TableTicker::ToJSObject(JSContext *aCx)
{
  JS::RootedValue val(aCx);
  std::stringstream ss;
  {
    
    
    JSStreamWriter b(ss);
    StreamJSObject(b);
    NS_ConvertUTF8toUTF16 js_string(nsDependentCString(ss.str().c_str()));
    JS_ParseJSON(aCx, static_cast<const char16_t*>(js_string.get()),
                 js_string.Length(), &val);
  }
  return &val.toObject();
}

struct SubprocessClosure {
  explicit SubprocessClosure(JSStreamWriter *aWriter)
    : mWriter(aWriter)
  {}

  JSStreamWriter* mWriter;
};

void SubProcessCallback(const char* aProfile, void* aClosure)
{
  
  
  SubprocessClosure* closure = (SubprocessClosure*)aClosure;

  
  closure->mWriter->Value(aProfile);
}


#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
static
void BuildJavaThreadJSObject(JSStreamWriter& b)
{
  b.BeginObject();

    b.NameValue("name", "Java Main Thread");

    b.Name("samples");
    b.BeginArray();

      
      for (int sampleId = 0; true; sampleId++) {
        bool firstRun = true;
        
        for (int frameId = 0; true; frameId++) {
          nsCString result;
          bool hasFrame = AndroidBridge::Bridge()->GetFrameNameJavaProfiling(0, sampleId, frameId, result);
          
          if (!hasFrame) {
            
            if (!firstRun) {
                b.EndArray();
              b.EndObject();
            }
            break;
          }
          
          if (firstRun) {
            firstRun = false;

            double sampleTime =
              mozilla::widget::android::GeckoJavaSampler::GetSampleTimeJavaProfiling(0, sampleId);

            b.BeginObject();
              b.NameValue("time", sampleTime);

              b.Name("frames");
              b.BeginArray();
          }
          
          b.BeginObject();
            b.NameValue("location", result.BeginReading());
          b.EndObject();
        }
        
        if (firstRun) {
          break;
        }
      }

    b.EndArray();

  b.EndObject();
}
#endif

void TableTicker::StreamJSObject(JSStreamWriter& b)
{
  b.BeginObject();
    
    b.NameValue("libs", GetSharedLibraryInfoString().c_str());

    
    b.Name("meta");
    StreamMetaJSCustomObject(b);

    
    b.Name("threads");
    b.BeginArray();

      SetPaused(true);

      {
        mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

        for (size_t i = 0; i < sRegisteredThreads->size(); i++) {
          
          if (!sRegisteredThreads->at(i)->Profile())
            continue;

          
          

          MutexAutoLock lock(*sRegisteredThreads->at(i)->Profile()->GetMutex());

          sRegisteredThreads->at(i)->Profile()->StreamJSObject(b);
        }
      }

      if (Sampler::CanNotifyObservers()) {
        
        
        SubprocessClosure closure(&b);
        nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
        if (os) {
          nsRefPtr<ProfileSaveEvent> pse = new ProfileSaveEvent(SubProcessCallback, &closure);
          os->NotifyObservers(pse, "profiler-subprocess", nullptr);
        }
      }

  #if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
      if (ProfileJava()) {
        mozilla::widget::android::GeckoJavaSampler::PauseJavaProfiling();

        BuildJavaThreadJSObject(b);

        mozilla::widget::android::GeckoJavaSampler::UnpauseJavaProfiling();
      }
  #endif

      SetPaused(false);
    b.EndArray();

  b.EndObject();
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
void addPseudoEntry(volatile StackEntry &entry, ThreadProfile &aProfile,
                    PseudoStack *stack, void *lastpc)
{
  
  
  if (entry.hasFlag(StackEntry::ASMJS))
    return;

  int lineno = -1;

  
  
  const char* sampleLabel = entry.label();
  if (entry.isCopyLabel()) {
    
    

    addDynamicTag(aProfile, 'c', sampleLabel);
    if (entry.isJs()) {
      if (!entry.pc()) {
        
        MOZ_ASSERT(&entry == &stack->mStack[stack->stackSize() - 1]);
        
        if (lastpc) {
          jsbytecode *jspc = js::ProfilingGetPC(stack->mRuntime, entry.script(),
                                                lastpc);
          if (jspc) {
            lineno = JS_PCToLineNumber(entry.script(), jspc);
          }
        }
      } else {
        lineno = JS_PCToLineNumber(entry.script(), entry.pc());
      }
    } else {
      lineno = entry.line();
    }
  } else {
    aProfile.addTag(ProfileEntry('c', sampleLabel));

    
    
    if (entry.isCpp()) {
      lineno = entry.line();
    }
  }

  if (lineno != -1) {
    aProfile.addTag(ProfileEntry('n', lineno));
  }

  uint32_t category = entry.category();
  MOZ_ASSERT(!(category & StackEntry::IS_CPP_ENTRY));
  MOZ_ASSERT(!(category & StackEntry::FRAME_LABEL_COPY));

  if (category) {
    aProfile.addTag(ProfileEntry('y', (int)category));
  }
}

struct NativeStack
{
  void** pc_array;
  void** sp_array;
  size_t size;
  size_t count;
};

struct JSFrame
{
    void* stackAddress;
    const char* label;
};

static
void mergeStacksIntoProfile(ThreadProfile& aProfile, TickSample* aSample, NativeStack& aNativeStack)
{
  PseudoStack* pseudoStack = aProfile.GetPseudoStack();
  volatile StackEntry *pseudoFrames = pseudoStack->mStack;
  uint32_t pseudoCount = pseudoStack->stackSize();

  
  
  

  JSFrame jsFrames[1000];
  uint32_t jsCount = 0;
  if (aSample && pseudoStack->mRuntime) {
    JS::ProfilingFrameIterator::RegisterState registerState;
    registerState.pc = aSample->pc;
    registerState.sp = aSample->sp;
#ifdef ENABLE_ARM_LR_SAVING
    registerState.lr = aSample->lr;
#endif

    JS::ProfilingFrameIterator jsIter(pseudoStack->mRuntime, registerState);
    for (; jsCount < mozilla::ArrayLength(jsFrames) && !jsIter.done(); ++jsCount, ++jsIter) {
      jsFrames[jsCount].stackAddress = jsIter.stackAddress();
      jsFrames[jsCount].label = jsIter.label();
    }
  }

  
  aProfile.addTag(ProfileEntry('s', "(root)"));

  
  
  
  
  
  uint32_t pseudoIndex = 0;
  int32_t jsIndex = jsCount - 1;
  int32_t nativeIndex = aNativeStack.count - 1;

  
  while (pseudoIndex != pseudoCount || jsIndex >= 0 || nativeIndex >= 0) {
    
    
    
    if (pseudoIndex != pseudoCount) {
      volatile StackEntry &pseudoFrame = pseudoFrames[pseudoIndex];

      
      
      
      
      if (pseudoFrame.isJs()) {
          addPseudoEntry(pseudoFrame, aProfile, pseudoStack, nullptr);
          pseudoIndex++;
          continue;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (pseudoFrame.hasFlag(StackEntry::ASMJS)) {
        void *stopStackAddress = nullptr;
        for (uint32_t i = pseudoIndex + 1; i != pseudoCount; i++) {
          if (pseudoFrames[i].isCpp()) {
            stopStackAddress = pseudoFrames[i].stackAddress();
            break;
          }
        }

        if (nativeIndex >= 0) {
          stopStackAddress = std::max(stopStackAddress, aNativeStack.sp_array[nativeIndex]);
        }

        while (jsIndex >= 0 && jsFrames[jsIndex].stackAddress > stopStackAddress) {
          addDynamicTag(aProfile, 'c', jsFrames[jsIndex].label);
          jsIndex--;
        }

        pseudoIndex++;
        continue;
      }

      
      if ((jsIndex < 0 || pseudoFrame.stackAddress() > jsFrames[jsIndex].stackAddress) &&
          (nativeIndex < 0 || pseudoFrame.stackAddress() > aNativeStack.sp_array[nativeIndex]))
      {
        
        addPseudoEntry(pseudoFrame, aProfile, pseudoStack, nullptr);
        pseudoIndex++;
        continue;
      }
    }

    if (jsIndex >= 0) {
      
      JSFrame &jsFrame = jsFrames[jsIndex];
      if ((pseudoIndex == pseudoCount || jsFrame.stackAddress > pseudoFrames[pseudoIndex].stackAddress()) &&
          (nativeIndex < 0 || jsFrame.stackAddress > aNativeStack.sp_array[nativeIndex]))
      {
        
        addDynamicTag(aProfile, 'c', jsFrame.label);
        jsIndex--;
        continue;
      }
    }

    
    
    MOZ_ASSERT(nativeIndex >= 0);
    aProfile.addTag(ProfileEntry('l', (void*)aNativeStack.pc_array[nativeIndex]));
    nativeIndex--;
  }
}

#ifdef USE_NS_STACKWALK
static
void StackWalkCallback(uint32_t aFrameNumber, void* aPC, void* aSP,
                       void* aClosure)
{
  NativeStack* nativeStack = static_cast<NativeStack*>(aClosure);
  MOZ_ASSERT(nativeStack->count < nativeStack->size);
  nativeStack->sp_array[nativeStack->count] = aSP;
  nativeStack->pc_array[nativeStack->count] = aPC;
  nativeStack->count++;
}

void TableTicker::doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample)
{
#ifndef XP_MACOSX
  uintptr_t thread = GetThreadHandle(aSample->threadProfile->GetPlatformData());
  MOZ_ASSERT(thread);
#endif
  void* pc_array[1000];
  void* sp_array[1000];
  NativeStack nativeStack = {
    pc_array,
    sp_array,
    mozilla::ArrayLength(pc_array),
    0
  };

  
  
  
  
  StackWalkCallback( 0, aSample->pc, aSample->sp, &nativeStack);

  uint32_t maxFrames = uint32_t(nativeStack.size - nativeStack.count);
#ifdef XP_MACOSX
  pthread_t pt = GetProfiledThread(aSample->threadProfile->GetPlatformData());
  void *stackEnd = reinterpret_cast<void*>(-1);
  if (pt)
    stackEnd = static_cast<char*>(pthread_get_stackaddr_np(pt));
  nsresult rv = NS_OK;
  if (aSample->fp >= aSample->sp && aSample->fp <= stackEnd)
    rv = FramePointerStackWalk(StackWalkCallback,  0,
                               maxFrames, &nativeStack,
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
                             &nativeStack, thread, platformData);
#endif
  if (NS_SUCCEEDED(rv))
    mergeStacksIntoProfile(aProfile, aSample, nativeStack);
}
#endif

#ifdef USE_EHABI_STACKWALK
void TableTicker::doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample)
{
  void *pc_array[1000];
  void *sp_array[1000];
  NativeStack nativeStack = {
    pc_array,
    sp_array,
    mozilla::ArrayLength(pc_array),
    0
  };

  const mcontext_t *mcontext = &reinterpret_cast<ucontext_t *>(aSample->context)->uc_mcontext;
  mcontext_t savedContext;
  PseudoStack *pseudoStack = aProfile.GetPseudoStack();

  nativeStack.count = 0;
  
  
  
  
  for (uint32_t i = pseudoStack->stackSize(); i > 0; --i) {
    
    
    volatile StackEntry &entry = pseudoStack->mStack[i - 1];
    if (!entry.isJs() && strcmp(entry.label(), "EnterJIT") == 0) {
      
      
      
      
      uint32_t *vSP = reinterpret_cast<uint32_t*>(entry.stackAddress());

      nativeStack.count += EHABIStackWalk(*mcontext,
                                           vSP,
                                          sp_array + nativeStack.count,
                                          pc_array + nativeStack.count,
                                          nativeStack.size - nativeStack.count);

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

  
  
  nativeStack.count += EHABIStackWalk(*mcontext,
                                      aProfile.GetStackTop(),
                                      sp_array + nativeStack.count,
                                      pc_array + nativeStack.count,
                                      nativeStack.size - nativeStack.count);

  mergeStacksIntoProfile(aProfile, aSample, nativeStack);
}

#endif

static
void doSampleStackTrace(ThreadProfile &aProfile, TickSample *aSample, bool aAddLeafAddresses)
{
  NativeStack nativeStack = { nullptr, nullptr, 0, 0 };
  mergeStacksIntoProfile(aProfile, aSample, nativeStack);

#ifdef ENABLE_SPS_LEAF_DATA
  if (aSample && aAddLeafAddresses) {
    aProfile.addTag(ProfileEntry('l', (void*)aSample->pc));
#ifdef ENABLE_ARM_LR_SAVING
    aProfile.addTag(ProfileEntry('L', (void*)aSample->lr));
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
  stack->updateGeneration(currThreadProfile.GetGenerationID());
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
        mozilla::TimeDuration delta = sample->timestamp - sLastTracerEvent;
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
    doSampleStackTrace(currThreadProfile, sample, mAddLeafAddresses);
  }
#else
  doSampleStackTrace(currThreadProfile, sample, mAddLeafAddresses);
#endif

  if (recordSample)
    currThreadProfile.flush();

  if (sample && currThreadProfile.GetThreadResponsiveness()->HasData()) {
    mozilla::TimeDuration delta = currThreadProfile.GetThreadResponsiveness()->GetUnresponsiveDuration(sample->timestamp);
    currThreadProfile.addTag(ProfileEntry('r', static_cast<float>(delta.ToMilliseconds())));
  }

  if (sample) {
    mozilla::TimeDuration delta = sample->timestamp - sStartTime;
    currThreadProfile.addTag(ProfileEntry('t', static_cast<float>(delta.ToMilliseconds())));
  }

  
  if (sample && sample->rssMemory != 0) {
    currThreadProfile.addTag(ProfileEntry('R', static_cast<float>(sample->rssMemory)));
  }

  
  if (sample && sample->ussMemory != 0) {
    currThreadProfile.addTag(ProfileEntry('U', static_cast<float>(sample->ussMemory)));
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

  ThreadInfo* info = new ThreadInfo("SyncProfile", tid, NS_IsMainThread(), stack, nullptr);
  SyncProfile* profile = new SyncProfile(info, GET_BACKTRACE_DEFAULT_ENTRY);
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
  doSampleStackTrace(*syncProfile, nullptr, false);
  syncProfile->EndUnwind();

  printf_stderr("Backtrace:\n");
  syncProfile->IterateTags(print_callback);
  ThreadInfo* info = syncProfile->GetThreadInfo();
  delete syncProfile;
  delete info;
}


