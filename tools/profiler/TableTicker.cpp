




#include <string>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include "GeckoProfilerImpl.h"
#include "SaveProfileTask.h"
#include "ProfileEntry.h"
#include "platform.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "shared-libraries.h"
#include "mozilla/StackWalk.h"
#include "TableTicker.h"


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


#include "jsdbgapi.h"


#if defined(MOZ_PROFILING) && (defined(XP_UNIX) && !defined(XP_MACOSX))
 #ifndef ANDROID
  #define USE_BACKTRACE
 #endif
#endif
#ifdef USE_BACKTRACE
 #include <execinfo.h>
#endif

#if defined(MOZ_PROFILING) && (defined(XP_MACOSX) || defined(XP_WIN))
 #define USE_NS_STACKWALK
#endif
#ifdef USE_NS_STACKWALK
 #include "nsStackWalk.h"
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

JSCustomObject* TableTicker::GetMetaJSCustomObject(JSAObjectBuilder& b)
{
  JSCustomObject *meta = b.CreateObject();

  b.DefineProperty(meta, "version", 2);
  b.DefineProperty(meta, "interval", interval());
  b.DefineProperty(meta, "stackwalk", mUseStackWalk);
  b.DefineProperty(meta, "jank", mJankOnly);
  b.DefineProperty(meta, "processType", XRE_GetProcessType());

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
  JSCustomObject* profile = b.CreateObject();
  BuildJSObject(b, profile);
  JSObject* jsProfile = b.GetJSObject(profile);

  return jsProfile;
}

void TableTicker::BuildJSObject(JSAObjectBuilder& b, JSCustomObject* profile)
{
  
  b.DefineProperty(profile, "libs", GetSharedLibraryInfoString().c_str());

  
  JSCustomObject *meta = GetMetaJSCustomObject(b);
  b.DefineProperty(profile, "meta", meta);

  
  JSCustomArray *threads = b.CreateArray();
  b.DefineProperty(profile, "threads", threads);

  
  SetPaused(true);
  ThreadProfile* prof = GetPrimaryThreadProfile();
  prof->GetMutex()->Lock();
  JSCustomObject* threadSamples = b.CreateObject();
  prof->BuildJSObject(b, threadSamples);
  b.ArrayPush(threads, threadSamples);
  prof->GetMutex()->Unlock();
  SetPaused(false);
}




static
void addDynamicTag(ThreadProfile &aProfile, char aTagName, const char *aStr)
{
  aProfile.addTag(ProfileEntry(aTagName, ""));
  
  size_t strLen = strlen(aStr) + 1;
  for (size_t j = 0; j < strLen;) {
    
    char text[sizeof(void*)];
    int len = sizeof(void*)/sizeof(char);
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
    aProfile.addTag(ProfileEntry('c', sampleLabel));
    lineno = entry.line();
  }
  if (lineno != -1) {
    aProfile.addTag(ProfileEntry('n', lineno));
  }
}

#ifdef USE_BACKTRACE
void TableTicker::doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample)
{
  void *array[100];
  int count = backtrace (array, 100);

  aProfile.addTag(ProfileEntry('s', "(root)"));

  for (int i = 0; i < count; i++) {
    if( (intptr_t)array[i] == -1 ) break;
    aProfile.addTag(ProfileEntry('l', (void*)array[i]));
  }
}
#endif


#ifdef USE_NS_STACKWALK
typedef struct {
  void** array;
  void** sp_array;
  size_t size;
  size_t count;
} PCArray;

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
  uintptr_t thread = GetThreadHandle(platform_data());
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

  uint32_t maxFrames = array.size - array.count;
#ifdef XP_MACOSX
  pthread_t pt = GetProfiledThread(platform_data());
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
  platformData = aSample->context;
#endif 

  nsresult rv = NS_StackWalk(StackWalkCallback,  0, maxFrames,
                             &array, thread, platformData);
#endif
  if (NS_SUCCEEDED(rv)) {
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
  
  PseudoStack* stack = mPrimaryThreadProfile.GetPseudoStack();
  for (int i = 0; stack->getMarker(i) != NULL; i++) {
    addDynamicTag(mPrimaryThreadProfile, 'm', stack->getMarker(i));
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

#if defined(USE_BACKTRACE) || defined(USE_NS_STACKWALK)
  if (mUseStackWalk) {
    doNativeBacktrace(mPrimaryThreadProfile, sample);
  } else {
    doSampleStackTrace(stack, mPrimaryThreadProfile, mAddLeafAddresses ? sample : nullptr);
  }
#else
  doSampleStackTrace(stack, mPrimaryThreadProfile, mAddLeafAddresses ? sample : nullptr);
#endif

  if (recordSample)
    mPrimaryThreadProfile.flush();

  if (!sLastTracerEvent.IsNull() && sample) {
    TimeDuration delta = sample->timestamp - sLastTracerEvent;
    mPrimaryThreadProfile.addTag(ProfileEntry('r', delta.ToMilliseconds()));
  }

  if (sample) {
    TimeDuration delta = sample->timestamp - mStartTime;
    mPrimaryThreadProfile.addTag(ProfileEntry('t', delta.ToMilliseconds()));
  }

  if (sLastFrameNumber != sFrameNumber) {
    mPrimaryThreadProfile.addTag(ProfileEntry('f', sFrameNumber));
    sLastFrameNumber = sFrameNumber;
  }
}

static void print_callback(const ProfileEntry& entry, const char* tagStringData) {
  switch (entry.getTagName()) {
    case 's':
    case 'c':
      printf_stderr("  %s\n", tagStringData);
  }
}

void mozilla_sampler_print_location1()
{
  if (!stack_key_initialized)
    profiler_init();

  PseudoStack *stack = tlsPseudoStack.get();
  if (!stack) {
    MOZ_ASSERT(false);
    return;
  }

  ThreadProfile threadProfile(1000, stack);
  doSampleStackTrace(stack, threadProfile, NULL);

  threadProfile.flush();

  printf_stderr("Backtrace:\n");
  threadProfile.IterateTags(print_callback);
}


