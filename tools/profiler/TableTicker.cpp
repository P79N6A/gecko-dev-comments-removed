




#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "sps_sampler.h"
#include "platform.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "shared-libraries.h"
#include "mozilla/StackWalk.h"


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

#if defined(MOZ_PROFILING) && defined(ANDROID)
 #define USE_LIBUNWIND
 #include <libunwind.h>
 #include "android-signal-defs.h"
#endif

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

static const int DYNAMIC_MAX_STRING = 512;

mozilla::ThreadLocal<ProfileStack *> tlsStack;
mozilla::ThreadLocal<TableTicker *> tlsTicker;




bool stack_key_initialized;

TimeStamp sLastTracerEvent;
int sFrameNumber = 0;
int sLastFrameNumber = 0;

class ThreadProfile;

class ProfileEntry
{
public:
  ProfileEntry()
    : mTagData(NULL)
    , mTagName(0)
  { }

  
  ProfileEntry(char aTagName, const char *aTagData)
    : mTagData(aTagData)
    , mTagName(aTagName)
  { }

  ProfileEntry(char aTagName, void *aTagPtr)
    : mTagPtr(aTagPtr)
    , mTagName(aTagName)
  { }

  ProfileEntry(char aTagName, double aTagFloat)
    : mTagFloat(aTagFloat)
    , mTagName(aTagName)
  { }

  ProfileEntry(char aTagName, uintptr_t aTagOffset)
    : mTagOffset(aTagOffset)
    , mTagName(aTagName)
  { }

  ProfileEntry(char aTagName, Address aTagAddress)
    : mTagAddress(aTagAddress)
    , mTagName(aTagName)
  { }

  ProfileEntry(char aTagName, int aTagLine)
    : mTagLine(aTagLine)
    , mTagName(aTagName)
  { }

  friend std::ostream& operator<<(std::ostream& stream, const ProfileEntry& entry);

private:
  friend class ThreadProfile;
  union {
    const char* mTagData;
    char mTagChars[sizeof(void*)];
    void* mTagPtr;
    double mTagFloat;
    Address mTagAddress;
    uintptr_t mTagOffset;
    int mTagLine;
  };
  char mTagName;
};

#define PROFILE_MAX_ENTRY 100000
class ThreadProfile
{
public:
  ThreadProfile(int aEntrySize, ProfileStack *aStack)
    : mWritePos(0)
    , mLastFlushPos(0)
    , mReadPos(0)
    , mEntrySize(aEntrySize)
    , mStack(aStack)
  {
    mEntries = new ProfileEntry[mEntrySize];
  }

  ~ThreadProfile()
  {
    delete[] mEntries;
  }

  void addTag(ProfileEntry aTag)
  {
    
    mEntries[mWritePos] = aTag;
    mWritePos = (mWritePos + 1) % mEntrySize;
    if (mWritePos == mReadPos) {
      
      mEntries[mReadPos] = ProfileEntry();
      mReadPos = (mReadPos + 1) % mEntrySize;
    }
    
    
    if (mWritePos == mLastFlushPos) {
      mLastFlushPos = (mLastFlushPos + 1) % mEntrySize;
    }
  }

  
  void flush()
  {
    mLastFlushPos = mWritePos;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  void erase()
  {
    mWritePos = mLastFlushPos;
  }

  char* processDynamicTag(int readPos, int* tagsConsumed, char* tagBuff)
  {
    int readAheadPos = (readPos + 1) % mEntrySize;
    int tagBuffPos = 0;

    
    bool seenNullByte = false;
    while (readAheadPos != mLastFlushPos && !seenNullByte) {
      (*tagsConsumed)++;
      ProfileEntry readAheadEntry = mEntries[readAheadPos];
      for (size_t pos = 0; pos < sizeof(void*); pos++) {
        tagBuff[tagBuffPos] = readAheadEntry.mTagChars[pos];
        if (tagBuff[tagBuffPos] == '\0' || tagBuffPos == DYNAMIC_MAX_STRING-2) {
          seenNullByte = true;
          break;
        }
        tagBuffPos++;
      }
      if (!seenNullByte)
        readAheadPos = (readAheadPos + 1) % mEntrySize;
    }
    return tagBuff;
  }

  friend std::ostream& operator<<(std::ostream& stream, const ThreadProfile& profile);

  JSObject *ToJSObject(JSContext *aCx)
  {
    JSObjectBuilder b(aCx);

    JSObject *profile = b.CreateObject();
    JSObject *samples = b.CreateArray();
    b.DefineProperty(profile, "samples", samples);

    JSObject *sample = NULL;
    JSObject *frames = NULL;

    int readPos = mReadPos;
    while (readPos != mLastFlushPos) {
      
      int incBy = 1;
      ProfileEntry entry = mEntries[readPos];

      
      const char* tagStringData = entry.mTagData;
      int readAheadPos = (readPos + 1) % mEntrySize;
      char tagBuff[DYNAMIC_MAX_STRING];
      
      tagBuff[DYNAMIC_MAX_STRING-1] = '\0';

      if (readAheadPos != mLastFlushPos && mEntries[readAheadPos].mTagName == 'd') {
        tagStringData = processDynamicTag(readPos, &incBy, tagBuff);
      }

      switch (entry.mTagName) {
        case 's':
          sample = b.CreateObject();
          b.DefineProperty(sample, "name", tagStringData);
          frames = b.CreateArray();
          b.DefineProperty(sample, "frames", frames);
          b.ArrayPush(samples, sample);
          break;
        case 'r':
          {
            if (sample) {
              b.DefineProperty(sample, "responsiveness", entry.mTagFloat);
            }
          }
          break;
        case 'f':
          {
            if (sample) {
              b.DefineProperty(sample, "frameNumber", entry.mTagLine);
            }
          }
          break;
        case 't':
          {
            if (sample) {
              b.DefineProperty(sample, "time", entry.mTagFloat);
            }
          }
          break;
        case 'c':
        case 'l':
          {
            if (sample) {
              JSObject *frame = b.CreateObject();
              if (entry.mTagName == 'l') {
                
                
                
                unsigned long long pc = (unsigned long long)(uintptr_t)entry.mTagPtr;
                snprintf(tagBuff, DYNAMIC_MAX_STRING, "%#llx", pc);
                b.DefineProperty(frame, "location", tagBuff);
              } else {
                b.DefineProperty(frame, "location", tagStringData);
                readAheadPos = (readPos + incBy) % mEntrySize;
                if (readAheadPos != mLastFlushPos &&
                    mEntries[readAheadPos].mTagName == 'n') {
                  b.DefineProperty(frame, "line",
                                   mEntries[readAheadPos].mTagLine);
                  incBy++;
                }
              }
              b.ArrayPush(frames, frame);
            }
          }
      }
      readPos = (readPos + incBy) % mEntrySize;
    }

    return profile;
  }

  ProfileStack* GetStack()
  {
    return mStack;
  }
private:
  
  
  ProfileEntry *mEntries;
  int mWritePos; 
  int mLastFlushPos; 
  int mReadPos;  
  int mEntrySize;
  ProfileStack *mStack;
};

class SaveProfileTask;

static bool
hasFeature(const char** aFeatures, uint32_t aFeatureCount, const char* aFeature) {
  for(size_t i = 0; i < aFeatureCount; i++) {
    if (strcmp(aFeatures[i], aFeature) == 0)
      return true;
  }
  return false;
}

class TableTicker: public Sampler {
 public:
  TableTicker(int aInterval, int aEntrySize, ProfileStack *aStack,
              const char** aFeatures, uint32_t aFeatureCount)
    : Sampler(aInterval, true)
    , mPrimaryThreadProfile(aEntrySize, aStack)
    , mStartTime(TimeStamp::Now())
    , mSaveRequested(false)
  {
    mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");

    
    mJankOnly = hasFeature(aFeatures, aFeatureCount, "jank");
    mProfileJS = hasFeature(aFeatures, aFeatureCount, "js");
    mPrimaryThreadProfile.addTag(ProfileEntry('m', "Start"));
  }

  ~TableTicker() { if (IsActive()) Stop(); }

  virtual void SampleStack(TickSample* sample) {}

  
  virtual void Tick(TickSample* sample);

  
  virtual void RequestSave()
  {
    mSaveRequested = true;
  }

  virtual void HandleSaveRequest();

  ThreadProfile* GetPrimaryThreadProfile()
  {
    return &mPrimaryThreadProfile;
  }

  JSObject *ToJSObject(JSContext *aCx);
  JSObject *GetMetaJSObject(JSObjectBuilder& b);

  const bool ProfileJS() { return mProfileJS; }

private:
  
  void doBacktrace(ThreadProfile &aProfile, TickSample* aSample);

private:
  
  ThreadProfile mPrimaryThreadProfile;
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
    TableTicker *t = tlsTicker.get();

    
    
    
    
    
    t->SetPaused(true);

    
#ifdef ANDROID
    nsCString tmpPath;
    tmpPath.AppendPrintf("/sdcard/profile_%i_%i.txt", XRE_GetProcessType(), getpid());
#else
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
#endif

    
    
    JSRuntime *rt;
    JSContext *cx;
    nsCOMPtr<nsIJSRuntimeService> rtsvc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
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
        JSObject* profileObj = mozilla_sampler_get_profile_data(cx);
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

void TableTicker::HandleSaveRequest()
{
  if (!mSaveRequested)
    return;
  mSaveRequested = false;

  
  
  nsCOMPtr<nsIRunnable> runnable = new SaveProfileTask();
  NS_DispatchToMainThread(runnable);
}

JSObject* TableTicker::GetMetaJSObject(JSObjectBuilder& b)
{
  JSObject *meta = b.CreateObject();

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

JSObject* TableTicker::ToJSObject(JSContext *aCx)
{
  JSObjectBuilder b(aCx);

  JSObject *profile = b.CreateObject();

  
  b.DefineProperty(profile, "libs", GetSharedLibraryInfoString().c_str());

  
  JSObject *meta = GetMetaJSObject(b);
  b.DefineProperty(profile, "meta", meta);

  
  JSObject *threads = b.CreateArray();
  b.DefineProperty(profile, "threads", threads);

  
  SetPaused(true);
  JSObject* threadSamples = GetPrimaryThreadProfile()->ToJSObject(aCx);
  b.ArrayPush(threads, threadSamples);
  SetPaused(false);

  return profile;
}

static
void addProfileEntry(volatile StackEntry &entry, ThreadProfile &aProfile,
                     ProfileStack *stack, void *lastpc)
{
  int lineno = -1;

  
  
  const char* sampleLabel = entry.label();
  if (entry.isCopyLabel()) {
    
    

    aProfile.addTag(ProfileEntry('c', ""));
    
    size_t strLen = strlen(sampleLabel) + 1;
    for (size_t j = 0; j < strLen;) {
      
      char text[sizeof(void*)];
      for (size_t pos = 0; pos < sizeof(void*) && j+pos < strLen; pos++) {
        text[pos] = sampleLabel[j+pos];
      }
      j += sizeof(void*)/sizeof(char);
      
      aProfile.addTag(ProfileEntry('d', *((void**)(&text[0]))));
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
    aProfile.addTag(ProfileEntry('c', sampleLabel));
    lineno = entry.line();
  }
  if (lineno != -1) {
    aProfile.addTag(ProfileEntry('n', lineno));
  }
}

#ifdef USE_BACKTRACE
void TableTicker::doBacktrace(ThreadProfile &aProfile, TickSample* aSample)
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
  if (array->count >= array->size) {
    
    return;
  }
  array->sp_array[array->count] = aSP;
  array->array[array->count] = aPC;
  array->count++;
}

void TableTicker::doBacktrace(ThreadProfile &aProfile, TickSample* aSample)
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

#ifdef XP_MACOSX
  pthread_t pt = GetProfiledThread(platform_data());
  void *stackEnd = reinterpret_cast<void*>(-1);
  if (pt)
    stackEnd = static_cast<char*>(pthread_get_stackaddr_np(pt));
  nsresult rv = FramePointerStackWalk(StackWalkCallback, 0, &array, reinterpret_cast<void**>(aSample->fp), stackEnd);
#else
  nsresult rv = NS_StackWalk(StackWalkCallback, 0, &array, thread);
#endif
  if (NS_SUCCEEDED(rv)) {
    aProfile.addTag(ProfileEntry('s', "(root)"));

    ProfileStack* stack = aProfile.GetStack();
    int pseudoStackPos = 0;

    














    
    
    
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

#if defined(USE_LIBUNWIND) && defined(ANDROID)
void TableTicker::doBacktrace(ThreadProfile &aProfile, TickSample* aSample)
{
  void* pc_array[1000];
  size_t count = 0;

  unw_cursor_t cursor; unw_context_t uc;
  unw_word_t ip;
  unw_getcontext(&uc);

  
  
  
  
  unw_tdep_context_t *unw_ctx = reinterpret_cast<unw_tdep_context_t*> (&uc);
  mcontext_t& mcontext = reinterpret_cast<ucontext_t*> (aSample->context)->uc_mcontext;
#define REPLACE_REG(num) unw_ctx->regs[num] = mcontext.gregs[R##num]
  REPLACE_REG(0);
  REPLACE_REG(1);
  REPLACE_REG(2);
  REPLACE_REG(3);
  REPLACE_REG(4);
  REPLACE_REG(5);
  REPLACE_REG(6);
  REPLACE_REG(7);
  REPLACE_REG(8);
  REPLACE_REG(9);
  REPLACE_REG(10);
  REPLACE_REG(11);
  REPLACE_REG(12);
  REPLACE_REG(13);
  REPLACE_REG(14);
  REPLACE_REG(15);
#undef REPLACE_REG
  unw_init_local(&cursor, &uc);
  while (count < ArrayLength(pc_array) &&
         unw_step(&cursor) > 0) {
    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    pc_array[count++] = reinterpret_cast<void*> (ip);
  }

  aProfile.addTag(ProfileEntry('s', "(root)"));
  for (size_t i = count; i > 0; --i) {
    aProfile.addTag(ProfileEntry('l', reinterpret_cast<void*>(pc_array[i - 1])));
  }
}
#endif

static
void doSampleStackTrace(ProfileStack *aStack, ThreadProfile &aProfile, TickSample *sample)
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


unsigned int sLastSampledEventGeneration = 0;




unsigned int sCurrentEventGeneration = 0;





void TableTicker::Tick(TickSample* sample)
{
  
  ProfileStack* stack = mPrimaryThreadProfile.GetStack();
  for (int i = 0; stack->getMarker(i) != NULL; i++) {
    mPrimaryThreadProfile.addTag(ProfileEntry('m', stack->getMarker(i)));
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

#if defined(USE_BACKTRACE) || defined(USE_NS_STACKWALK) || defined(USE_LIBUNWIND)
  if (mUseStackWalk) {
    doBacktrace(mPrimaryThreadProfile, sample);
  } else {
    doSampleStackTrace(stack, mPrimaryThreadProfile, sample);
  }
#else
  doSampleStackTrace(stack, mPrimaryThreadProfile, sample);
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

std::ostream& operator<<(std::ostream& stream, const ThreadProfile& profile)
{
  int readPos = profile.mReadPos;
  while (readPos != profile.mLastFlushPos) {
    stream << profile.mEntries[readPos];
    readPos = (readPos + 1) % profile.mEntrySize;
  }
  return stream;
}

std::ostream& operator<<(std::ostream& stream, const ProfileEntry& entry)
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

void mozilla_sampler_init()
{
  if (!tlsStack.init() || !tlsTicker.init()) {
    LOG("Failed to init.");
    return;
  }
  stack_key_initialized = true;

  ProfileStack *stack = new ProfileStack();
  tlsStack.set(stack);

#if defined(USE_LIBUNWIND) && defined(ANDROID)
  
  putenv("UNW_ARM_UNWIND_METHOD=5");
#endif

  
  OS::RegisterStartHandler();

#if defined(USE_LIBUNWIND) && defined(__arm__) && defined(MOZ_CRASHREPORTER)
  
  
  
  return;
#endif

  
  
  
  const char *val = PR_GetEnv("MOZ_PROFILER_STARTUP");
  if (!val || !*val) {
    return;
  }

  const char* features = "js";
  mozilla_sampler_start(PROFILE_DEFAULT_ENTRY, PROFILE_DEFAULT_INTERVAL,
                        &features, 1);
}

void mozilla_sampler_deinit()
{
  mozilla_sampler_stop();
  
  
  
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
#if defined(MOZ_PROFILING) && (defined(USE_BACKTRACE) || defined(USE_NS_STACKWALK) || defined(USE_LIBUNWIND))
    "stackwalk",
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
    mozilla_sampler_init();

  ProfileStack *stack = tlsStack.get();
  if (!stack) {
    ASSERT(false);
    return;
  }

  mozilla_sampler_stop();

  TableTicker *t = new TableTicker(aInterval, aProfileEntries, stack,
                                   aFeatures, aFeatureCount);
  tlsTicker.set(t);
  t->Start();
  if (t->ProfileJS())
      stack->enableJSSampling();
}

void mozilla_sampler_stop()
{
  if (!stack_key_initialized)
    mozilla_sampler_init();

  TableTicker *t = tlsTicker.get();
  if (!t) {
    return;
  }

  bool disableJS = t->ProfileJS();

  t->Stop();
  delete t;
  tlsTicker.set(NULL);
  ProfileStack *stack = tlsStack.get();
  ASSERT(stack != NULL);

  if (disableJS)
    stack->disableJSSampling();
}

bool mozilla_sampler_is_active()
{
  if (!stack_key_initialized)
    mozilla_sampler_init();

  TableTicker *t = tlsTicker.get();
  if (!t) {
    return false;
  }

  return t->IsActive();
}

double sResponsivenessTimes[100];
double sCurrResponsiveness = 0.f;
unsigned int sResponsivenessLoc = 0;
void mozilla_sampler_responsiveness(TimeStamp aTime)
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
