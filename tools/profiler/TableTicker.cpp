




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


#include "ProfileJSONWriter.h"


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
#include "nsTArray.h"

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

#if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_x86_linux)
# define USE_LUL_STACKWALK
# include "LulMain.h"
# include "platform-linux-lul.h"
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

#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#else
# define VALGRIND_MAKE_MEM_DEFINED(_addr,_len)   ((void)0)
#endif





std::string GetSharedLibraryInfoString();

static bool
hasFeature(const char** aFeatures, uint32_t aFeatureCount, const char* aFeature) {
  for(size_t i = 0; i < aFeatureCount; i++) {
    if (strcmp(aFeatures[i], aFeature) == 0)
      return true;
  }
  return false;
}

TableTicker::TableTicker(double aInterval, int aEntrySize,
                         const char** aFeatures, uint32_t aFeatureCount,
                         const char** aThreadNameFilters, uint32_t aFilterCount)
  : Sampler(aInterval, true, aEntrySize)
  , mPrimaryThreadProfile(nullptr)
  , mBuffer(new ProfileBuffer(aEntrySize))
  , mSaveRequested(false)
#if defined(XP_WIN)
  , mIntelPowerGadget(nullptr)
#endif
{
  mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");

  mProfileJS = hasFeature(aFeatures, aFeatureCount, "js");
  mProfileJava = hasFeature(aFeatures, aFeatureCount, "java");
  mProfileGPU = hasFeature(aFeatures, aFeatureCount, "gpu");
  mProfilePower = hasFeature(aFeatures, aFeatureCount, "power");
  
  
  mProfileThreads = hasFeature(aFeatures, aFeatureCount, "threads") || aFilterCount > 0;
  mAddLeafAddresses = hasFeature(aFeatures, aFeatureCount, "leaf");
  mPrivacyMode = hasFeature(aFeatures, aFeatureCount, "privacy");
  mAddMainThreadIO = hasFeature(aFeatures, aFeatureCount, "mainthreadio");
  mProfileMemory = hasFeature(aFeatures, aFeatureCount, "memory");
  mTaskTracer = hasFeature(aFeatures, aFeatureCount, "tasktracer");
  mLayersDump = hasFeature(aFeatures, aFeatureCount, "layersdump");
  mDisplayListDump = hasFeature(aFeatures, aFeatureCount, "displaylistdump");
  mProfileRestyle = hasFeature(aFeatures, aFeatureCount, "restyle");

#if defined(XP_WIN)
  if (mProfilePower) {
    mIntelPowerGadget = new IntelPowerGadget();
    mProfilePower = mIntelPowerGadget->Init();
  }
#endif

  
  MOZ_ALWAYS_TRUE(mThreadNameFilters.resize(aFilterCount));
  for (uint32_t i = 0; i < aFilterCount; ++i) {
    mThreadNameFilters[i] = aThreadNameFilters[i];
  }

  bool ignore;
  sStartTime = mozilla::TimeStamp::ProcessCreation(ignore);

  {
    mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

    
    for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
      ThreadInfo* info = sRegisteredThreads->at(i);

      RegisterThread(info);
    }

    SetActiveSampler(this);
  }

#ifdef MOZ_TASK_TRACER
  if (mTaskTracer) {
    mozilla::tasktracer::StartLogging();
  }
#endif
}

TableTicker::~TableTicker()
{
  if (IsActive())
    Stop();

  SetActiveSampler(nullptr);

  
  {
    mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

    for (uint32_t i = 0; i < sRegisteredThreads->size(); i++) {
      ThreadInfo* info = sRegisteredThreads->at(i);
      ThreadProfile* profile = info->Profile();
      if (profile) {
        delete profile;
        info->SetProfile(nullptr);
      }
      
      
      if (info->IsPendingDelete()) {
        delete info;
        sRegisteredThreads->erase(sRegisteredThreads->begin() + i);
        i--;
      }
    }
  }
#if defined(XP_WIN)
  delete mIntelPowerGadget;
#endif
}

void TableTicker::HandleSaveRequest()
{
  if (!mSaveRequested)
    return;
  mSaveRequested = false;

  
  
  nsCOMPtr<nsIRunnable> runnable = new SaveProfileTask();
  NS_DispatchToMainThread(runnable);
}

void TableTicker::DeleteExpiredMarkers()
{
  mBuffer->deleteExpiredStoredMarkers();
}

void TableTicker::StreamTaskTracer(SpliceableJSONWriter& aWriter)
{
#ifdef MOZ_TASK_TRACER
  aWriter.StartArrayProperty("data");
    nsAutoPtr<nsTArray<nsCString>> data(mozilla::tasktracer::GetLoggedData(sStartTime));
    for (uint32_t i = 0; i < data->Length(); ++i) {
      aWriter.StringElement((data->ElementAt(i)).get());
    }
  aWriter.EndArray();

  aWriter.StartArrayProperty("threads");
    mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);
    for (size_t i = 0; i < sRegisteredThreads->size(); i++) {
      
      ThreadInfo* info = sRegisteredThreads->at(i);
      aWriter.StartObjectElement();
        if (XRE_GetProcessType() == GeckoProcessType_Plugin) {
          
          aWriter.StringProperty("name", "Plugin");
        } else {
          aWriter.StringProperty("name", info->Name());
        }
        aWriter.IntProperty("tid", static_cast<int>(info->ThreadId()));
      aWriter.EndObject();
    }
  aWriter.EndArray();

  aWriter.DoubleProperty("start", static_cast<double>(mozilla::tasktracer::GetStartTime()));
#endif
}


void TableTicker::StreamMetaJSCustomObject(SpliceableJSONWriter& aWriter)
{
  aWriter.IntProperty("version", 3);
  aWriter.DoubleProperty("interval", interval());
  aWriter.IntProperty("stackwalk", mUseStackWalk);
  aWriter.IntProperty("processType", XRE_GetProcessType());

  mozilla::TimeDuration delta = mozilla::TimeStamp::Now() - sStartTime;
  aWriter.DoubleProperty("startTime", static_cast<double>(PR_Now()/1000.0 - delta.ToMilliseconds()));

  nsresult res;
  nsCOMPtr<nsIHttpProtocolHandler> http = do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &res);
  if (!NS_FAILED(res)) {
    nsAutoCString string;

    res = http->GetPlatform(string);
    if (!NS_FAILED(res))
      aWriter.StringProperty("platform", string.Data());

    res = http->GetOscpu(string);
    if (!NS_FAILED(res))
      aWriter.StringProperty("oscpu", string.Data());

    res = http->GetMisc(string);
    if (!NS_FAILED(res))
      aWriter.StringProperty("misc", string.Data());
  }

  nsCOMPtr<nsIXULRuntime> runtime = do_GetService("@mozilla.org/xre/runtime;1");
  if (runtime) {
    nsAutoCString string;

    res = runtime->GetXPCOMABI(string);
    if (!NS_FAILED(res))
      aWriter.StringProperty("abi", string.Data());

    res = runtime->GetWidgetToolkit(string);
    if (!NS_FAILED(res))
      aWriter.StringProperty("toolkit", string.Data());
  }

  nsCOMPtr<nsIXULAppInfo> appInfo = do_GetService("@mozilla.org/xre/app-info;1");
  if (appInfo) {
    nsAutoCString string;

    res = appInfo->GetName(string);
    if (!NS_FAILED(res))
      aWriter.StringProperty("product", string.Data());
  }
}

void TableTicker::ToStreamAsJSON(std::ostream& stream, double aSinceTime)
{
  SpliceableJSONWriter b(mozilla::MakeUnique<OStreamJSONWriteFunc>(stream));
  StreamJSON(b, aSinceTime);
}

JSObject* TableTicker::ToJSObject(JSContext* aCx, double aSinceTime)
{
  JS::RootedValue val(aCx);
  {
    UniquePtr<char[]> buf = ToJSON(aSinceTime);
    NS_ConvertUTF8toUTF16 js_string(nsDependentCString(buf.get()));
    bool rv = JS_ParseJSON(aCx, static_cast<const char16_t*>(js_string.get()),
                           js_string.Length(), &val);
    if (!rv) {
#ifdef NIGHTLY_BUILD
      
      nsCOMPtr<nsIFile> path;
      nsresult rv = NS_GetSpecialDirectory("TmpD", getter_AddRefs(path));
      if (!NS_FAILED(rv)) {
        rv = path->Append(NS_LITERAL_STRING("bad-profile.json"));
        if (!NS_FAILED(rv)) {
          nsCString cpath;
          rv = path->GetNativePath(cpath);
          if (!NS_FAILED(rv)) {
            std::ofstream stream;
            stream.open(cpath.get());
            if (stream.is_open()) {
              stream << buf.get();
              stream.close();
              printf_stderr("Malformed profiler JSON dumped to %s! "
                            "Please upload to https://bugzil.la/1172157\n",
                            cpath.get());
            }
          }
        }
      }
#endif
    }
  }
  return &val.toObject();
}

UniquePtr<char[]> TableTicker::ToJSON(double aSinceTime)
{
  SpliceableChunkedJSONWriter b;
  StreamJSON(b, aSinceTime);
  return b.WriteFunc()->CopyData();
}

void TableTicker::ToJSObjectAsync(double aSinceTime,
                                  Promise* aPromise)
{
  if (NS_WARN_IF(mGatherer)) {
    return;
  }

  mGatherer = new ProfileGatherer(this, aSinceTime, aPromise);
  mGatherer->Start();
}

void TableTicker::ProfileGathered()
{
  mGatherer = nullptr;
}

struct SubprocessClosure {
  explicit SubprocessClosure(SpliceableJSONWriter* aWriter)
    : mWriter(aWriter)
  {}

  SpliceableJSONWriter* mWriter;
};

void SubProcessCallback(const char* aProfile, void* aClosure)
{
  
  
  SubprocessClosure* closure = (SubprocessClosure*)aClosure;

  
  closure->mWriter->StringElement(aProfile);
}


#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
static
void BuildJavaThreadJSObject(SpliceableJSONWriter& aWriter)
{
  aWriter.Start(SpliceableJSONWriter::SingleLineStyle);
  aWriter.StringProperty("name", "Java Main Thread");

  aWriter.StartArrayProperty("samples");

    
    for (int sampleId = 0; true; sampleId++) {
      bool firstRun = true;
      
      for (int frameId = 0; true; frameId++) {
        nsCString result;
        bool hasFrame = AndroidBridge::Bridge()->GetFrameNameJavaProfiling(0, sampleId, frameId, result);
        
        if (!hasFrame) {
          
          if (!firstRun) {
              aWriter.EndArray();
            aWriter.EndObject();
          }
          break;
        }
        
        if (firstRun) {
          firstRun = false;

          double sampleTime =
            mozilla::widget::GeckoJavaSampler::GetSampleTimeJavaProfiling(0, sampleId);

          aWriter.StartObjectElement();
            aWriter.DoubleProperty("time", sampleTime);

            aWriter.StartArrayProperty("frames");
        }
        
        aWriter.StartObjectElement();
          aWriter.StringProperty("location", result.BeginReading());
        aWriter.EndObject();
      }
      
      if (firstRun) {
        break;
      }
    }

  aWriter.EndArray();
  aWriter.End();
}
#endif

void TableTicker::StreamJSON(SpliceableJSONWriter& aWriter, double aSinceTime)
{
  aWriter.Start(SpliceableJSONWriter::SingleLineStyle);
  {
    
    aWriter.StringProperty("libs", GetSharedLibraryInfoString().c_str());

    
    aWriter.StartObjectProperty("meta");
      StreamMetaJSCustomObject(aWriter);
    aWriter.EndObject();

    
    if (TaskTracer()) {
      aWriter.StartObjectProperty("tasktracer");
      StreamTaskTracer(aWriter);
    }

    
    aWriter.StartArrayProperty("threads");
    {
      SetPaused(true);

      {
        mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

        for (size_t i = 0; i < sRegisteredThreads->size(); i++) {
          
          if (!sRegisteredThreads->at(i)->Profile())
            continue;

          
          

          MutexAutoLock lock(*sRegisteredThreads->at(i)->Profile()->GetMutex());

          sRegisteredThreads->at(i)->Profile()->StreamJSON(aWriter, aSinceTime);
        }
      }

      if (Sampler::CanNotifyObservers()) {
        
        
        SubprocessClosure closure(&aWriter);
        nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
        if (os) {
          nsRefPtr<ProfileSaveEvent> pse = new ProfileSaveEvent(SubProcessCallback, &closure);
          os->NotifyObservers(pse, "profiler-subprocess", nullptr);
        }
      }

  #if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
      if (ProfileJava()) {
        mozilla::widget::GeckoJavaSampler::PauseJavaProfiling();

        BuildJavaThreadJSObject(aWriter);

        mozilla::widget::GeckoJavaSampler::UnpauseJavaProfiling();
      }
  #endif

      SetPaused(false);
    }
    aWriter.EndArray();
  }
  aWriter.End();
}

void TableTicker::FlushOnJSShutdown(JSRuntime* aRuntime)
{
  SetPaused(true);

  {
    mozilla::MutexAutoLock lock(*sRegisteredThreadsMutex);

    for (size_t i = 0; i < sRegisteredThreads->size(); i++) {
      
      if (!sRegisteredThreads->at(i)->Profile() ||
          sRegisteredThreads->at(i)->IsPendingDelete()) {
        continue;
      }

      
      if (sRegisteredThreads->at(i)->Profile()->GetPseudoStack()->mRuntime != aRuntime) {
        continue;
      }

      MutexAutoLock lock(*sRegisteredThreads->at(i)->Profile()->GetMutex());
      sRegisteredThreads->at(i)->Profile()->FlushSamplesAndMarkers();
    }
  }

  SetPaused(false);
}

void PseudoStack::flushSamplerOnJSShutdown()
{
  MOZ_ASSERT(mRuntime);
  TableTicker* t = tlsTicker.get();
  if (t) {
    t->FlushOnJSShutdown(mRuntime);
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
void addPseudoEntry(volatile StackEntry &entry, ThreadProfile &aProfile,
                    PseudoStack *stack, void *lastpc)
{
  
  
  if (entry.hasFlag(StackEntry::BEGIN_PSEUDO_JS))
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

mozilla::Atomic<bool> WALKING_JS_STACK(false);

struct AutoWalkJSStack {
  bool walkAllowed;

  AutoWalkJSStack() : walkAllowed(false) {
    walkAllowed = WALKING_JS_STACK.compareExchange(false, true);
  }

  ~AutoWalkJSStack() {
    if (walkAllowed)
        WALKING_JS_STACK = false;
  }
};

static
void mergeStacksIntoProfile(ThreadProfile& aProfile, TickSample* aSample, NativeStack& aNativeStack)
{
  PseudoStack* pseudoStack = aProfile.GetPseudoStack();
  volatile StackEntry *pseudoFrames = pseudoStack->mStack;
  uint32_t pseudoCount = pseudoStack->stackSize();

  
  
  

  
  
  
  
  uint32_t startBufferGen;
  if (aSample->isSamplingCurrentThread) {
    startBufferGen = UINT32_MAX;
  } else {
    startBufferGen = aProfile.bufferGeneration();
  }
  uint32_t jsCount = 0;
  JS::ProfilingFrameIterator::Frame jsFrames[1000];
  
  if (pseudoStack->mRuntime && JS::IsProfilingEnabledForRuntime(pseudoStack->mRuntime)) {
    AutoWalkJSStack autoWalkJSStack;
    const uint32_t maxFrames = mozilla::ArrayLength(jsFrames);

    if (aSample && autoWalkJSStack.walkAllowed) {
      JS::ProfilingFrameIterator::RegisterState registerState;
      registerState.pc = aSample->pc;
      registerState.sp = aSample->sp;
#ifdef ENABLE_ARM_LR_SAVING
      registerState.lr = aSample->lr;
#endif

      JS::ProfilingFrameIterator jsIter(pseudoStack->mRuntime,
                                        registerState,
                                        startBufferGen);
      for (; jsCount < maxFrames && !jsIter.done(); ++jsIter) {
        
        if (aSample->isSamplingCurrentThread || jsIter.isAsmJS()) {
          uint32_t extracted = jsIter.extractStack(jsFrames, jsCount, maxFrames);
          jsCount += extracted;
          if (jsCount == maxFrames)
            break;
        } else {
          mozilla::Maybe<JS::ProfilingFrameIterator::Frame> frame =
            jsIter.getPhysicalFrameWithoutLabel();
          if (frame.isSome())
            jsFrames[jsCount++] = frame.value();
        }
      }
    }
  }

  
  aProfile.addTag(ProfileEntry('s', "(root)"));

  
  
  
  
  
  uint32_t pseudoIndex = 0;
  int32_t jsIndex = jsCount - 1;
  int32_t nativeIndex = aNativeStack.count - 1;

  uint8_t *lastPseudoCppStackAddr = nullptr;

  
  while (pseudoIndex != pseudoCount || jsIndex >= 0 || nativeIndex >= 0) {
    

    uint8_t *pseudoStackAddr = nullptr;
    uint8_t *jsStackAddr = nullptr;
    uint8_t *nativeStackAddr = nullptr;

    if (pseudoIndex != pseudoCount) {
      volatile StackEntry &pseudoFrame = pseudoFrames[pseudoIndex];

      if (pseudoFrame.isCpp())
        lastPseudoCppStackAddr = (uint8_t *) pseudoFrame.stackAddress();

      
      
      
      
      
      
      
      if (pseudoFrame.isJs() && pseudoFrame.isOSR()) {
          pseudoIndex++;
          continue;
      }

      MOZ_ASSERT(lastPseudoCppStackAddr);
      pseudoStackAddr = lastPseudoCppStackAddr;
    }

    if (jsIndex >= 0)
      jsStackAddr = (uint8_t *) jsFrames[jsIndex].stackAddress;

    if (nativeIndex >= 0)
      nativeStackAddr = (uint8_t *) aNativeStack.sp_array[nativeIndex];

    
    
    
    
    
    if (nativeStackAddr && (pseudoStackAddr == nativeStackAddr || jsStackAddr == nativeStackAddr)) {
      nativeStackAddr = nullptr;
      nativeIndex--;
      MOZ_ASSERT(pseudoStackAddr || jsStackAddr);
    }

    
    MOZ_ASSERT_IF(pseudoStackAddr, pseudoStackAddr != jsStackAddr &&
                                   pseudoStackAddr != nativeStackAddr);
    MOZ_ASSERT_IF(jsStackAddr, jsStackAddr != pseudoStackAddr &&
                               jsStackAddr != nativeStackAddr);
    MOZ_ASSERT_IF(nativeStackAddr, nativeStackAddr != pseudoStackAddr &&
                                   nativeStackAddr != jsStackAddr);

    
    if (pseudoStackAddr > jsStackAddr && pseudoStackAddr > nativeStackAddr) {
      MOZ_ASSERT(pseudoIndex < pseudoCount);
      volatile StackEntry &pseudoFrame = pseudoFrames[pseudoIndex];
      addPseudoEntry(pseudoFrame, aProfile, pseudoStack, nullptr);
      pseudoIndex++;
      continue;
    }

    
    if (jsStackAddr > nativeStackAddr) {
      MOZ_ASSERT(jsIndex >= 0);
      const JS::ProfilingFrameIterator::Frame& jsFrame = jsFrames[jsIndex];

      
      
      
      
      
      
      
      
      
      
      
      
      
      if (aSample->isSamplingCurrentThread ||
          jsFrame.kind == JS::ProfilingFrameIterator::Frame_AsmJS) {
        addDynamicTag(aProfile, 'c', jsFrame.label);
      } else {
        MOZ_ASSERT(jsFrame.kind == JS::ProfilingFrameIterator::Frame_Ion ||
                   jsFrame.kind == JS::ProfilingFrameIterator::Frame_Baseline);
        aProfile.addTag(ProfileEntry('J', jsFrames[jsIndex].returnAddress));
      }

      jsIndex--;
      continue;
    }

    
    
    if (nativeStackAddr) {
      MOZ_ASSERT(nativeIndex >= 0);
      aProfile
        .addTag(ProfileEntry('l', (void*)aNativeStack.pc_array[nativeIndex]));
    }
    if (nativeIndex >= 0) {
      nativeIndex--;
    }
  }

  
  
  
  
  if (!aSample->isSamplingCurrentThread && pseudoStack->mRuntime) {
    MOZ_ASSERT(aProfile.bufferGeneration() >= startBufferGen);
    uint32_t lapCount = aProfile.bufferGeneration() - startBufferGen;
    JS::UpdateJSRuntimeProfilerSampleBufferGen(pseudoStack->mRuntime,
                                               aProfile.bufferGeneration(),
                                               lapCount);
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


#ifdef USE_LUL_STACKWALK
void TableTicker::doNativeBacktrace(ThreadProfile &aProfile, TickSample* aSample)
{
  const mcontext_t* mc
    = &reinterpret_cast<ucontext_t *>(aSample->context)->uc_mcontext;

  lul::UnwindRegs startRegs;
  memset(&startRegs, 0, sizeof(startRegs));

# if defined(SPS_PLAT_amd64_linux)
  startRegs.xip = lul::TaggedUWord(mc->gregs[REG_RIP]);
  startRegs.xsp = lul::TaggedUWord(mc->gregs[REG_RSP]);
  startRegs.xbp = lul::TaggedUWord(mc->gregs[REG_RBP]);
# elif defined(SPS_PLAT_arm_android)
  startRegs.r15 = lul::TaggedUWord(mc->arm_pc);
  startRegs.r14 = lul::TaggedUWord(mc->arm_lr);
  startRegs.r13 = lul::TaggedUWord(mc->arm_sp);
  startRegs.r12 = lul::TaggedUWord(mc->arm_ip);
  startRegs.r11 = lul::TaggedUWord(mc->arm_fp);
  startRegs.r7  = lul::TaggedUWord(mc->arm_r7);
# elif defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_android)
  startRegs.xip = lul::TaggedUWord(mc->gregs[REG_EIP]);
  startRegs.xsp = lul::TaggedUWord(mc->gregs[REG_ESP]);
  startRegs.xbp = lul::TaggedUWord(mc->gregs[REG_EBP]);
# else
#   error "Unknown plat"
# endif

  





  lul::StackImage stackImg;

  {
#   if defined(SPS_PLAT_amd64_linux)
    uintptr_t rEDZONE_SIZE = 128;
    uintptr_t start = startRegs.xsp.Value() - rEDZONE_SIZE;
#   elif defined(SPS_PLAT_arm_android)
    uintptr_t rEDZONE_SIZE = 0;
    uintptr_t start = startRegs.r13.Value() - rEDZONE_SIZE;
#   elif defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_android)
    uintptr_t rEDZONE_SIZE = 0;
    uintptr_t start = startRegs.xsp.Value() - rEDZONE_SIZE;
#   else
#     error "Unknown plat"
#   endif
    uintptr_t end   = reinterpret_cast<uintptr_t>(aProfile.GetStackTop());
    uintptr_t ws    = sizeof(void*);
    start &= ~(ws-1);
    end   &= ~(ws-1);
    uintptr_t nToCopy = 0;
    if (start < end) {
      nToCopy = end - start;
      if (nToCopy > lul::N_STACK_BYTES)
        nToCopy = lul::N_STACK_BYTES;
    }
    MOZ_ASSERT(nToCopy <= lul::N_STACK_BYTES);
    stackImg.mLen       = nToCopy;
    stackImg.mStartAvma = start;
    if (nToCopy > 0) {
      memcpy(&stackImg.mContents[0], (void*)start, nToCopy);
      (void)VALGRIND_MAKE_MEM_DEFINED(&stackImg.mContents[0], nToCopy);
    }
  }

  
  
  
  const int MAX_NATIVE_FRAMES = 256;

  size_t scannedFramesAllowed = 0;

  uintptr_t framePCs[MAX_NATIVE_FRAMES];
  uintptr_t frameSPs[MAX_NATIVE_FRAMES];
  size_t framesAvail = mozilla::ArrayLength(framePCs);
  size_t framesUsed  = 0;
  size_t scannedFramesAcquired = 0;
  sLUL->Unwind( &framePCs[0], &frameSPs[0],
                &framesUsed, &scannedFramesAcquired,
                framesAvail, scannedFramesAllowed,
                &startRegs, &stackImg );

  NativeStack nativeStack = {
    reinterpret_cast<void**>(framePCs),
    reinterpret_cast<void**>(frameSPs),
    mozilla::ArrayLength(framePCs),
    0
  };

  nativeStack.count = framesUsed;

  mergeStacksIntoProfile(aProfile, aSample, nativeStack);

  
  
  sLUL->mStats.mContext += 1;
  sLUL->mStats.mCFI     += framesUsed - 1 - scannedFramesAcquired;
  sLUL->mStats.mScanned += scannedFramesAcquired;
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
  
  InplaceTick(sample);
}

void TableTicker::InplaceTick(TickSample* sample)
{
  ThreadProfile& currThreadProfile = *sample->threadProfile;

  currThreadProfile.addTag(ProfileEntry('T', currThreadProfile.ThreadId()));

  if (sample) {
    mozilla::TimeDuration delta = sample->timestamp - sStartTime;
    currThreadProfile.addTag(ProfileEntry('t', delta.ToMilliseconds()));
  }

  PseudoStack* stack = currThreadProfile.GetPseudoStack();

#if defined(USE_NS_STACKWALK) || defined(USE_EHABI_STACKWALK) || \
    defined(USE_LUL_STACKWALK)
  if (mUseStackWalk) {
    doNativeBacktrace(currThreadProfile, sample);
  } else {
    doSampleStackTrace(currThreadProfile, sample, mAddLeafAddresses);
  }
#else
  doSampleStackTrace(currThreadProfile, sample, mAddLeafAddresses);
#endif

  
  
  if (!sample->isSamplingCurrentThread) {
    ProfilerMarkerLinkedList* pendingMarkersList = stack->getPendingMarkers();
    while (pendingMarkersList && pendingMarkersList->peek()) {
      ProfilerMarker* marker = pendingMarkersList->popHead();
      currThreadProfile.addStoredMarker(marker);
      currThreadProfile.addTag(ProfileEntry('m', marker));
    }
  }

  if (sample && currThreadProfile.GetThreadResponsiveness()->HasData()) {
    mozilla::TimeDuration delta = currThreadProfile.GetThreadResponsiveness()->GetUnresponsiveDuration(sample->timestamp);
    currThreadProfile.addTag(ProfileEntry('r', delta.ToMilliseconds()));
  }

  
  if (sample && sample->rssMemory != 0) {
    currThreadProfile.addTag(ProfileEntry('R', static_cast<double>(sample->rssMemory)));
  }

  
  if (sample && sample->ussMemory != 0) {
    currThreadProfile.addTag(ProfileEntry('U', static_cast<double>(sample->ussMemory)));
  }

#if defined(XP_WIN)
  if (mProfilePower) {
    mIntelPowerGadget->TakeSample();
    currThreadProfile.addTag(ProfileEntry('p', static_cast<double>(mIntelPowerGadget->GetTotalPackagePowerInWatts())));
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

  profile->BeginUnwind();
  Tick(&sample);
  profile->EndUnwind();

  return profile;
}

void
TableTicker::GetBufferInfo(uint32_t *aCurrentPosition, uint32_t *aTotalSize, uint32_t *aGeneration)
{
  *aCurrentPosition = mBuffer->mWritePos;
  *aTotalSize = mBuffer->mEntrySize;
  *aGeneration = mBuffer->mGeneration;
}
