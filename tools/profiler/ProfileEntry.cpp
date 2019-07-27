




#include <ostream>
#include <sstream>
#include "platform.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"


#include "jsapi.h"
#include "js/ProfilingFrameIterator.h"
#include "js/TrackedOptimizationInfo.h"


#include "JSStreamWriter.h"


#include "ProfileEntry.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
 #define snprintf _snprintf
#endif

using mozilla::Maybe;
using mozilla::Some;
using mozilla::Nothing;




ProfileEntry::ProfileEntry()
  : mTagData(nullptr)
  , mTagName(0)
{ }


ProfileEntry::ProfileEntry(char aTagName, const char *aTagData)
  : mTagData(aTagData)
  , mTagName(aTagName)
{ }

ProfileEntry::ProfileEntry(char aTagName, ProfilerMarker *aTagMarker)
  : mTagMarker(aTagMarker)
  , mTagName(aTagName)
{ }

ProfileEntry::ProfileEntry(char aTagName, void *aTagPtr)
  : mTagPtr(aTagPtr)
  , mTagName(aTagName)
{ }

ProfileEntry::ProfileEntry(char aTagName, float aTagFloat)
  : mTagFloat(aTagFloat)
  , mTagName(aTagName)
{ }

ProfileEntry::ProfileEntry(char aTagName, uintptr_t aTagOffset)
  : mTagOffset(aTagOffset)
  , mTagName(aTagName)
{ }

ProfileEntry::ProfileEntry(char aTagName, Address aTagAddress)
  : mTagAddress(aTagAddress)
  , mTagName(aTagName)
{ }

ProfileEntry::ProfileEntry(char aTagName, int aTagInt)
  : mTagInt(aTagInt)
  , mTagName(aTagName)
{ }

ProfileEntry::ProfileEntry(char aTagName, char aTagChar)
  : mTagChar(aTagChar)
  , mTagName(aTagName)
{ }

bool ProfileEntry::is_ent_hint(char hintChar) {
  return mTagName == 'h' && mTagChar == hintChar;
}

bool ProfileEntry::is_ent_hint() {
  return mTagName == 'h';
}

bool ProfileEntry::is_ent(char tagChar) {
  return mTagName == tagChar;
}

void* ProfileEntry::get_tagPtr() {
  
  return mTagPtr;
}








ProfileBuffer::ProfileBuffer(int aEntrySize)
  : mEntries(MakeUnique<ProfileEntry[]>(aEntrySize))
  , mWritePos(0)
  , mReadPos(0)
  , mEntrySize(aEntrySize)
  , mGeneration(0)
{
}

ProfileBuffer::~ProfileBuffer()
{
  while (mStoredMarkers.peek()) {
    delete mStoredMarkers.popHead();
  }
}


void ProfileBuffer::addTag(const ProfileEntry& aTag)
{
  mEntries[mWritePos++] = aTag;
  if (mWritePos == mEntrySize) {
    
    
    MOZ_ASSERT(mGeneration != UINT32_MAX);
    mGeneration++;
    mWritePos = 0;
  }
  if (mWritePos == mReadPos) {
    
    mEntries[mReadPos] = ProfileEntry();
    mReadPos = (mReadPos + 1) % mEntrySize;
  }
}

void ProfileBuffer::addStoredMarker(ProfilerMarker *aStoredMarker) {
  aStoredMarker->SetGeneration(mGeneration);
  mStoredMarkers.insert(aStoredMarker);
}

void ProfileBuffer::deleteExpiredStoredMarkers() {
  
  
  uint32_t generation = mGeneration;
  while (mStoredMarkers.peek() &&
         mStoredMarkers.peek()->HasExpired(generation)) {
    delete mStoredMarkers.popHead();
  }
}

void ProfileBuffer::reset() {
  mGeneration += 2;
  mReadPos = mWritePos = 0;
  deleteExpiredStoredMarkers();
}

#define DYNAMIC_MAX_STRING 512

char* ProfileBuffer::processDynamicTag(int readPos,
                                       int* tagsConsumed, char* tagBuff)
{
  int readAheadPos = (readPos + 1) % mEntrySize;
  int tagBuffPos = 0;

  
  bool seenNullByte = false;
  while (readAheadPos != mWritePos && !seenNullByte) {
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

void ProfileBuffer::IterateTagsForThread(IterateTagsCallback aCallback, int aThreadId)
{
  MOZ_ASSERT(aCallback);

  int readPos = mReadPos;
  int currentThreadID = -1;

  while (readPos != mWritePos) {
    const ProfileEntry& entry = mEntries[readPos];

    if (entry.mTagName == 'T') {
      currentThreadID = entry.mTagInt;
      readPos = (readPos + 1) % mEntrySize;
      continue;
    }

    
    int incBy = 1;

    
    const char* tagStringData = entry.mTagData;
    int readAheadPos = (readPos + 1) % mEntrySize;
    char tagBuff[DYNAMIC_MAX_STRING];
    
    tagBuff[DYNAMIC_MAX_STRING-1] = '\0';

    if (readAheadPos != mWritePos && mEntries[readAheadPos].mTagName == 'd') {
      tagStringData = processDynamicTag(readPos, &incBy, tagBuff);
    }

    if (currentThreadID == aThreadId) {
      aCallback(entry, tagStringData);
    }

    readPos = (readPos + incBy) % mEntrySize;
  }
}

class StreamOptimizationTypeInfoOp : public JS::ForEachTrackedOptimizationTypeInfoOp
{
  JSStreamWriter& mWriter;
  bool mStartedTypeList;

public:
  explicit StreamOptimizationTypeInfoOp(JSStreamWriter& b)
    : mWriter(b)
    , mStartedTypeList(false)
  { }

  void readType(const char *keyedBy, const char *name,
                const char *location, unsigned lineno) override {
    if (!mStartedTypeList) {
      mStartedTypeList = true;
      mWriter.BeginObject();
        mWriter.Name("types");
        mWriter.BeginArray();
    }

    mWriter.BeginObject();
      mWriter.NameValue("keyedBy", keyedBy);
      if (name) {
        mWriter.NameValue("name", name);
      }
      if (location) {
        mWriter.NameValue("location", location);
      }
      if (lineno != UINT32_MAX) {
        mWriter.NameValue("line", lineno);
      }
    mWriter.EndObject();
  }

  void operator()(JS::TrackedTypeSite site, const char *mirType) override {
    if (mStartedTypeList) {
      mWriter.EndArray();
      mStartedTypeList = false;
    } else {
      mWriter.BeginObject();
    }

      mWriter.NameValue("site", JS::TrackedTypeSiteString(site));
      mWriter.NameValue("mirType", mirType);
    mWriter.EndObject();
  }
};

class StreamOptimizationAttemptsOp : public JS::ForEachTrackedOptimizationAttemptOp
{
  JSStreamWriter& mWriter;

public:
  explicit StreamOptimizationAttemptsOp(JSStreamWriter& b)
    : mWriter(b)
  { }

  void operator()(JS::TrackedStrategy strategy, JS::TrackedOutcome outcome) override {
    mWriter.BeginObject();
      
      
      mWriter.NameValue("strategy", JS::TrackedStrategyString(strategy));
      mWriter.NameValue("outcome", JS::TrackedOutcomeString(outcome));
    mWriter.EndObject();
  }
};

class StreamJSFramesOp : public JS::ForEachProfiledFrameOp
{
  JSRuntime* mRuntime;
  void* mReturnAddress;
  UniqueJITOptimizations& mUniqueOpts;
  JSStreamWriter& mWriter;

public:
  StreamJSFramesOp(JSRuntime* aRuntime, void* aReturnAddr, UniqueJITOptimizations& aUniqueOpts,
                   JSStreamWriter& aWriter)
   : mRuntime(aRuntime)
   , mReturnAddress(aReturnAddr)
   , mUniqueOpts(aUniqueOpts)
   , mWriter(aWriter)
  { }

  void operator()(const char* label, bool mightHaveTrackedOptimizations) override {
    mWriter.BeginObject();
      mWriter.NameValue("location", label);
      JS::ProfilingFrameIterator::FrameKind frameKind =
        JS::GetProfilingFrameKindFromNativeAddr(mRuntime, mReturnAddress);
      MOZ_ASSERT(frameKind == JS::ProfilingFrameIterator::Frame_Ion ||
                 frameKind == JS::ProfilingFrameIterator::Frame_Baseline);
      const char* jitLevelString =
        (frameKind == JS::ProfilingFrameIterator::Frame_Ion) ? "ion"
                                                             : "baseline";
      mWriter.NameValue("implementation", jitLevelString);
      if (mightHaveTrackedOptimizations) {
        Maybe<unsigned> optsIndex = mUniqueOpts.getIndex(mReturnAddress, mRuntime);
        if (optsIndex.isSome()) {
          mWriter.NameValue("optsIndex", optsIndex.value());
        }
      }
    mWriter.EndObject();
  }
};

bool UniqueJITOptimizations::OptimizationKey::operator<(const OptimizationKey& other) const
{
  if (mEntryAddr == other.mEntryAddr) {
    return mIndex < other.mIndex;
  }
  return mEntryAddr < other.mEntryAddr;
}

Maybe<unsigned> UniqueJITOptimizations::getIndex(void* addr, JSRuntime* rt)
{
  void* entryAddr;
  Maybe<uint8_t> optIndex = JS::TrackedOptimizationIndexAtAddr(rt, addr, &entryAddr);
  if (optIndex.isNothing()) {
    return Nothing();
  }

  OptimizationKey key;
  key.mEntryAddr = entryAddr;
  key.mIndex = optIndex.value();

  auto iter = mOptToIndexMap.find(key);
  if (iter != mOptToIndexMap.end()) {
    MOZ_ASSERT(iter->second < mOpts.length());
    return Some(iter->second);
  }

  unsigned keyIndex = mOpts.length();
  mOptToIndexMap.insert(std::make_pair(key, keyIndex));
  MOZ_ALWAYS_TRUE(mOpts.append(key));
  return Some(keyIndex);
}

void UniqueJITOptimizations::stream(JSStreamWriter& b, JSRuntime* rt)
{
  for (size_t i = 0; i < mOpts.length(); i++) {
    b.BeginObject();
    b.Name("types");
    b.BeginArray();
    StreamOptimizationTypeInfoOp typeInfoOp(b);
    JS::ForEachTrackedOptimizationTypeInfo(rt, mOpts[i].mEntryAddr, mOpts[i].mIndex,
                                           typeInfoOp);
    b.EndArray();

    b.Name("attempts");
    b.BeginArray();
    JSScript *script;
    jsbytecode *pc;
    StreamOptimizationAttemptsOp attemptOp(b);
    JS::ForEachTrackedOptimizationAttempt(rt, mOpts[i].mEntryAddr, mOpts[i].mIndex,
                                          attemptOp, &script, &pc);
    b.EndArray();

    unsigned line, column;
    line = JS_PCToLineNumber(script, pc, &column);
    b.NameValue("line", line);
    b.NameValue("column", column);
    b.EndObject();
  }
}

void ProfileBuffer::StreamSamplesToJSObject(JSStreamWriter& b, int aThreadId, JSRuntime* rt,
                                            UniqueJITOptimizations& aUniqueOpts)
{
  bool sample = false;
  int readPos = mReadPos;
  int currentThreadID = -1;
  while (readPos != mWritePos) {
    ProfileEntry entry = mEntries[readPos];
    if (entry.mTagName == 'T') {
      currentThreadID = entry.mTagInt;
    }
    if (currentThreadID == aThreadId) {
      switch (entry.mTagName) {
        case 'r':
          {
            if (sample) {
              b.NameValue("responsiveness", entry.mTagFloat);
            }
          }
          break;
        case 'p':
          {
            if (sample) {
              b.NameValue("power", entry.mTagFloat);
            }
          }
          break;
        case 'R':
          {
            if (sample) {
              b.NameValue("rss", entry.mTagFloat);
            }
          }
          break;
        case 'U':
          {
            if (sample) {
              b.NameValue("uss", entry.mTagFloat);
            }
          }
          break;
        case 'f':
          {
            if (sample) {
              b.NameValue("frameNumber", entry.mTagInt);
            }
          }
          break;
        case 't':
          {
            if (sample) {
              b.NameValue("time", entry.mTagFloat);
            }
          }
          break;
        case 's':
          {
            
            if (sample) {
              b.EndObject();
            }
            
            b.BeginObject();

            sample = true;

            
            
            
            b.Name("frames");
            b.BeginArray();

              b.BeginObject();
                b.NameValue("location", "(root)");
              b.EndObject();

              int framePos = (readPos + 1) % mEntrySize;
              ProfileEntry frame = mEntries[framePos];
              while (framePos != mWritePos && frame.mTagName != 's' && frame.mTagName != 'T') {
                int incBy = 1;
                frame = mEntries[framePos];

                
                const char* tagStringData = frame.mTagData;
                int readAheadPos = (framePos + 1) % mEntrySize;
                char tagBuff[DYNAMIC_MAX_STRING];
                
                
                tagBuff[DYNAMIC_MAX_STRING-1] = '\0';

                if (readAheadPos != mWritePos && mEntries[readAheadPos].mTagName == 'd') {
                  tagStringData = processDynamicTag(framePos, &incBy, tagBuff);
                }

                
                
                
                
                if (frame.mTagName == 'l') {
                  b.BeginObject();
                    
                    
                    
                    unsigned long long pc = (unsigned long long)(uintptr_t)frame.mTagPtr;
                    snprintf(tagBuff, DYNAMIC_MAX_STRING, "%#llx", pc);
                    b.NameValue("location", tagBuff);
                  b.EndObject();
                } else if (frame.mTagName == 'c') {
                  b.BeginObject();
                    b.NameValue("location", tagStringData);
                    readAheadPos = (framePos + incBy) % mEntrySize;
                    if (readAheadPos != mWritePos &&
                        mEntries[readAheadPos].mTagName == 'n') {
                      b.NameValue("line", mEntries[readAheadPos].mTagInt);
                      incBy++;
                    }
                    readAheadPos = (framePos + incBy) % mEntrySize;
                    if (readAheadPos != mWritePos &&
                        mEntries[readAheadPos].mTagName == 'y') {
                      b.NameValue("category", mEntries[readAheadPos].mTagInt);
                      incBy++;
                    }
                  b.EndObject();
                } else if (frame.mTagName == 'J') {
                  void* pc = frame.mTagPtr;
                  StreamJSFramesOp framesOp(rt, pc, aUniqueOpts, b);
                  JS::ForEachProfiledFrame(rt, pc, framesOp);
                }
                framePos = (framePos + incBy) % mEntrySize;
              }
            b.EndArray();
          }
          break;
      }
    }
    readPos = (readPos + 1) % mEntrySize;
  }
  if (sample) {
    b.EndObject();
  }
}

void ProfileBuffer::StreamMarkersToJSObject(JSStreamWriter& b, int aThreadId)
{
  int readPos = mReadPos;
  int currentThreadID = -1;
  while (readPos != mWritePos) {
    ProfileEntry entry = mEntries[readPos];
    if (entry.mTagName == 'T') {
      currentThreadID = entry.mTagInt;
    } else if (currentThreadID == aThreadId && entry.mTagName == 'm') {
      entry.getMarker()->StreamJSObject(b);
    }
    readPos = (readPos + 1) % mEntrySize;
  }
}

int ProfileBuffer::FindLastSampleOfThread(int aThreadId)
{
  
  
  for (int readPos  = (mWritePos + mEntrySize - 1) % mEntrySize;
           readPos !=  (mReadPos + mEntrySize - 1) % mEntrySize;
           readPos  =   (readPos + mEntrySize - 1) % mEntrySize) {
    ProfileEntry entry = mEntries[readPos];
    if (entry.mTagName == 'T' && entry.mTagInt == aThreadId) {
      return readPos;
    }
  }

  return -1;
}

void ProfileBuffer::DuplicateLastSample(int aThreadId)
{
  int lastSampleStartPos = FindLastSampleOfThread(aThreadId);
  if (lastSampleStartPos == -1) {
    return;
  }

  MOZ_ASSERT(mEntries[lastSampleStartPos].mTagName == 'T');

  addTag(mEntries[lastSampleStartPos]);

  
  for (int readPos = (lastSampleStartPos + 1) % mEntrySize;
       readPos != mWritePos;
       readPos = (readPos + 1) % mEntrySize) {
    switch (mEntries[readPos].mTagName) {
      case 'T':
        
        return;
      case 't':
        
        addTag(ProfileEntry('t', static_cast<float>((mozilla::TimeStamp::Now() - sStartTime).ToMilliseconds())));
        break;
      case 'm':
        
        break;
      
      
      default:
        addTag(mEntries[readPos]);
        break;
    }
  }
}








ThreadProfile::ThreadProfile(ThreadInfo* aInfo, ProfileBuffer* aBuffer)
  : mThreadInfo(aInfo)
  , mBuffer(aBuffer)
  , mPseudoStack(aInfo->Stack())
  , mMutex("ThreadProfile::mMutex")
  , mThreadId(int(aInfo->ThreadId()))
  , mIsMainThread(aInfo->IsMainThread())
  , mPlatformData(aInfo->GetPlatformData())
  , mStackTop(aInfo->StackTop())
  , mRespInfo(this)
#ifdef XP_LINUX
  , mRssMemory(0)
  , mUssMemory(0)
#endif
{
  MOZ_COUNT_CTOR(ThreadProfile);
  MOZ_ASSERT(aBuffer);

  
  MOZ_ASSERT(aInfo->ThreadId() >= 0, "native thread ID is < 0");
  MOZ_ASSERT(aInfo->ThreadId() <= INT32_MAX, "native thread ID is > INT32_MAX");
}

ThreadProfile::~ThreadProfile()
{
  MOZ_COUNT_DTOR(ThreadProfile);
}

void ThreadProfile::addTag(const ProfileEntry& aTag)
{
  mBuffer->addTag(aTag);
}

void ThreadProfile::addStoredMarker(ProfilerMarker *aStoredMarker) {
  mBuffer->addStoredMarker(aStoredMarker);
}

void ThreadProfile::IterateTags(IterateTagsCallback aCallback)
{
  mBuffer->IterateTagsForThread(aCallback, mThreadId);
}

void ThreadProfile::ToStreamAsJSON(std::ostream& stream)
{
  JSStreamWriter b(stream);
  StreamJSObject(b);
}

void ThreadProfile::StreamJSObject(JSStreamWriter& b)
{
  b.BeginObject();
    
    if (XRE_GetProcessType() == GeckoProcessType_Plugin) {
      
      b.NameValue("name", "Plugin");
    } else if (XRE_GetProcessType() == GeckoProcessType_Content) {
      
      
      b.NameValue("name", "Content");
    } else {
      b.NameValue("name", Name());
    }
    b.NameValue("tid", static_cast<int>(mThreadId));

    UniqueJITOptimizations uniqueOpts;

    b.Name("samples");
    b.BeginArray();
      if (!mSavedStreamedSamples.empty()) {
        b.SpliceArrayElements(mSavedStreamedSamples.c_str());
        mSavedStreamedSamples.clear();
      }
      mBuffer->StreamSamplesToJSObject(b, mThreadId, mPseudoStack->mRuntime, uniqueOpts);
    b.EndArray();

    
    
    
    if (!mSavedStreamedOptimizations.empty()) {
      MOZ_ASSERT(uniqueOpts.empty());
      b.Name("optimizations");
      b.BeginArray();
        b.SpliceArrayElements(mSavedStreamedOptimizations.c_str());
        mSavedStreamedOptimizations.clear();
      b.EndArray();
    } else if (!uniqueOpts.empty()) {
      b.Name("optimizations");
      b.BeginArray();
        uniqueOpts.stream(b, mPseudoStack->mRuntime);
      b.EndArray();
    }

    b.Name("markers");
    b.BeginArray();
      if (!mSavedStreamedMarkers.empty()) {
        b.SpliceArrayElements(mSavedStreamedMarkers.c_str());
        mSavedStreamedMarkers.clear();
      }
      mBuffer->StreamMarkersToJSObject(b, mThreadId);
    b.EndArray();
  b.EndObject();
}

void ThreadProfile::FlushSamplesAndMarkers()
{
  
  
  MOZ_ASSERT(mPseudoStack->mRuntime);

  
  
  
  
  std::stringstream ss;
  JSStreamWriter b(ss);
  UniqueJITOptimizations uniqueOpts;
  b.BeginBareList();
    mBuffer->StreamSamplesToJSObject(b, mThreadId, mPseudoStack->mRuntime, uniqueOpts);
  b.EndBareList();
  mSavedStreamedSamples = ss.str();

  
  ss.str("");
  ss.clear();

  if (!uniqueOpts.empty()) {
    b.BeginBareList();
      uniqueOpts.stream(b, mPseudoStack->mRuntime);
    b.EndBareList();
    mSavedStreamedOptimizations = ss.str();
  }

  
  ss.str("");
  ss.clear();

  b.BeginBareList();
    mBuffer->StreamMarkersToJSObject(b, mThreadId);
  b.EndBareList();
  mSavedStreamedMarkers = ss.str();

  
  
  mBuffer->reset();
}

JSObject* ThreadProfile::ToJSObject(JSContext *aCx)
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

PseudoStack* ThreadProfile::GetPseudoStack()
{
  return mPseudoStack;
}

void ThreadProfile::BeginUnwind()
{
  mMutex.Lock();
}

void ThreadProfile::EndUnwind()
{
  mMutex.Unlock();
}

mozilla::Mutex* ThreadProfile::GetMutex()
{
  return &mMutex;
}

void ThreadProfile::DuplicateLastSample()
{
  mBuffer->DuplicateLastSample(mThreadId);
}



