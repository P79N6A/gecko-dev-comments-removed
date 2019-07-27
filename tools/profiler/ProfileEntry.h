





#ifndef MOZ_PROFILE_ENTRY_H
#define MOZ_PROFILE_ENTRY_H

#include <ostream>
#include "GeckoProfiler.h"
#include "platform.h"
#include "ProfileJSONWriter.h"
#include "ProfilerBacktrace.h"
#include "nsRefPtr.h"
#include "nsHashKeys.h"
#include "nsDataHashtable.h"
#include "js/ProfilingFrameIterator.h"
#include "js/TrackedOptimizationInfo.h"
#include "mozilla/Maybe.h"
#include "mozilla/Mutex.h"
#include "mozilla/Vector.h"
#include "gtest/MozGtestFriend.h"
#include "mozilla/UniquePtr.h"

class ThreadProfile;


#ifndef __arm__
#pragma pack(push, 1)
#endif

class ProfileEntry
{
public:
  ProfileEntry();

  
  ProfileEntry(char aTagName, const char *aTagData);
  ProfileEntry(char aTagName, void *aTagPtr);
  ProfileEntry(char aTagName, ProfilerMarker *aTagMarker);
  ProfileEntry(char aTagName, float aTagFloat);
  ProfileEntry(char aTagName, uintptr_t aTagOffset);
  ProfileEntry(char aTagName, Address aTagAddress);
  ProfileEntry(char aTagName, int aTagLine);
  ProfileEntry(char aTagName, char aTagChar);
  bool is_ent_hint(char hintChar);
  bool is_ent_hint();
  bool is_ent(char tagName);
  void* get_tagPtr();
  const ProfilerMarker* getMarker() {
    MOZ_ASSERT(mTagName == 'm');
    return mTagMarker;
  }

  char getTagName() const { return mTagName; }

private:
  FRIEND_TEST(ThreadProfile, InsertOneTag);
  FRIEND_TEST(ThreadProfile, InsertOneTagWithTinyBuffer);
  FRIEND_TEST(ThreadProfile, InsertTagsNoWrap);
  FRIEND_TEST(ThreadProfile, InsertTagsWrap);
  FRIEND_TEST(ThreadProfile, MemoryMeasure);
  friend class ProfileBuffer;
  union {
    const char* mTagData;
    char        mTagChars[sizeof(void*)];
    void*       mTagPtr;
    ProfilerMarker* mTagMarker;
    float       mTagFloat;
    Address     mTagAddress;
    uintptr_t   mTagOffset;
    int         mTagInt;
    char        mTagChar;
  };
  char mTagName;
};

#ifndef __arm__
#pragma pack(pop)
#endif

class UniqueJSONStrings
{
public:
  UniqueJSONStrings() {
    mStringTableWriter.StartBareList();
  }

  void SpliceStringTableElements(SpliceableJSONWriter& aWriter) {
    aWriter.TakeAndSplice(mStringTableWriter.WriteFunc());
  }

  void WriteProperty(mozilla::JSONWriter& aWriter, const char* aName, const char* aStr) {
    aWriter.IntProperty(aName, GetOrAddIndex(aStr));
  }

  void WriteElement(mozilla::JSONWriter& aWriter, const char* aStr) {
    aWriter.IntElement(GetOrAddIndex(aStr));
  }

  uint32_t GetOrAddIndex(const char* aStr);

private:
  SpliceableChunkedJSONWriter mStringTableWriter;
  nsDataHashtable<nsCharPtrHashKey, uint32_t> mStringToIndexMap;
};

class UniqueStacks
{
public:
  struct FrameKey {
    nsCString mLocation;
    mozilla::Maybe<unsigned> mLine;
    mozilla::Maybe<unsigned> mCategory;
    mozilla::Maybe<void*> mJITAddress;
    mozilla::Maybe<uint32_t> mJITDepth;

    explicit FrameKey(const char* aLocation)
     : mLocation(aLocation)
    { }

    FrameKey(const FrameKey& aToCopy)
     : mLocation(aToCopy.mLocation)
     , mLine(aToCopy.mLine)
     , mCategory(aToCopy.mCategory)
     , mJITAddress(aToCopy.mJITAddress)
     , mJITDepth(aToCopy.mJITDepth)
    { }

    FrameKey(void* aJITAddress, uint32_t aJITDepth)
     : mJITAddress(mozilla::Some(aJITAddress))
     , mJITDepth(mozilla::Some(aJITDepth))
    { }

    uint32_t Hash() const;
    bool operator==(const FrameKey& aOther) const;
  };

  
  struct MOZ_STACK_CLASS OnStackFrameKey : public FrameKey {
    const JS::ForEachProfiledFrameOp::FrameHandle* mJITFrameHandle;

    explicit OnStackFrameKey(const char* aLocation)
      : FrameKey(aLocation)
      , mJITFrameHandle(nullptr)
    { }

    OnStackFrameKey(const OnStackFrameKey& aToCopy)
      : FrameKey(aToCopy)
      , mJITFrameHandle(aToCopy.mJITFrameHandle)
    { }

    OnStackFrameKey(void* aJITAddress, unsigned aJITDepth)
      : FrameKey(aJITAddress, aJITDepth)
      , mJITFrameHandle(nullptr)
    { }

    OnStackFrameKey(void* aJITAddress, unsigned aJITDepth,
                    const JS::ForEachProfiledFrameOp::FrameHandle& aJITFrameHandle)
      : FrameKey(aJITAddress, aJITDepth)
      , mJITFrameHandle(&aJITFrameHandle)
    { }
  };

  struct StackKey {
    mozilla::Maybe<uint32_t> mPrefixHash;
    mozilla::Maybe<uint32_t> mPrefix;
    uint32_t mFrame;

    explicit StackKey(uint32_t aFrame)
     : mFrame(aFrame)
    { }

    uint32_t Hash() const;
    bool operator==(const StackKey& aOther) const;
  };

  class Stack {
  public:
    Stack(UniqueStacks& aUniqueStacks, const OnStackFrameKey& aRoot);

    void AppendFrame(const OnStackFrameKey& aFrame);
    uint32_t GetOrAddIndex() const;

  private:
    UniqueStacks& mUniqueStacks;
    StackKey mStack;
  };

  explicit UniqueStacks(JSRuntime* aRuntime);

  Stack BeginStack(const OnStackFrameKey& aRoot);
  uint32_t LookupJITFrameDepth(void* aAddr);
  void AddJITFrameDepth(void* aAddr, unsigned depth);
  void SpliceFrameTableElements(SpliceableJSONWriter& aWriter);
  void SpliceStackTableElements(SpliceableJSONWriter& aWriter);

private:
  uint32_t GetOrAddFrameIndex(const OnStackFrameKey& aFrame);
  uint32_t GetOrAddStackIndex(const StackKey& aStack);
  void StreamFrame(const OnStackFrameKey& aFrame);
  void StreamStack(const StackKey& aStack);

public:
  UniqueJSONStrings mUniqueStrings;

private:
  JSRuntime* mRuntime;

  
  
  
  
  nsDataHashtable<nsVoidPtrHashKey, uint32_t> mJITFrameDepthMap;

  uint32_t mFrameCount;
  SpliceableChunkedJSONWriter mFrameTableWriter;
  nsDataHashtable<nsGenericHashKey<FrameKey>, uint32_t> mFrameToIndexMap;

  SpliceableChunkedJSONWriter mStackTableWriter;
  nsDataHashtable<nsGenericHashKey<StackKey>, uint32_t> mStackToIndexMap;
};

class ProfileBuffer {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ProfileBuffer)

  explicit ProfileBuffer(int aEntrySize);

  void addTag(const ProfileEntry& aTag);
  void StreamSamplesToJSON(SpliceableJSONWriter& aWriter, int aThreadId, float aSinceTime,
                           JSRuntime* rt, UniqueStacks& aUniqueStacks);
  void StreamMarkersToJSON(SpliceableJSONWriter& aWriter, int aThreadId, float aSinceTime,
                           UniqueStacks& aUniqueStacks);
  void DuplicateLastSample(int aThreadId);

  void addStoredMarker(ProfilerMarker* aStoredMarker);

  
  void deleteExpiredStoredMarkers();
  void reset();

protected:
  char* processDynamicTag(int readPos, int* tagsConsumed, char* tagBuff);
  int FindLastSampleOfThread(int aThreadId);

  ~ProfileBuffer();

public:
  
  mozilla::UniquePtr<ProfileEntry[]> mEntries;

  
  
  int mWritePos;

  
  int mReadPos;

  
  int mEntrySize;

  
  uint32_t mGeneration;

  
  ProfilerMarkerLinkedList mStoredMarkers;
};






























































































class ThreadProfile
{
public:
  ThreadProfile(ThreadInfo* aThreadInfo, ProfileBuffer* aBuffer);
  virtual ~ThreadProfile();
  void addTag(const ProfileEntry& aTag);

  




  void addStoredMarker(ProfilerMarker *aStoredMarker);
  PseudoStack* GetPseudoStack();
  mozilla::Mutex* GetMutex();
  void StreamJSON(SpliceableJSONWriter& aWriter, float aSinceTime = 0);

  



  void FlushSamplesAndMarkers();

  void BeginUnwind();
  virtual void EndUnwind();
  virtual SyncProfile* AsSyncProfile() { return nullptr; }

  bool IsMainThread() const { return mIsMainThread; }
  const char* Name() const { return mThreadInfo->Name(); }
  int ThreadId() const { return mThreadId; }

  PlatformData* GetPlatformData() const { return mPlatformData; }
  void* GetStackTop() const { return mStackTop; }
  void DuplicateLastSample();

  ThreadInfo* GetThreadInfo() const { return mThreadInfo; }
  ThreadResponsiveness* GetThreadResponsiveness() { return &mRespInfo; }
  void SetPendingDelete()
  {
    mPseudoStack = nullptr;
    mPlatformData = nullptr;
  }

  uint32_t bufferGeneration() const {
    return mBuffer->mGeneration;
  }

protected:
  void StreamSamplesAndMarkers(SpliceableJSONWriter& aWriter, float aSinceTime,
                               UniqueStacks& aUniqueStacks);

private:
  FRIEND_TEST(ThreadProfile, InsertOneTag);
  FRIEND_TEST(ThreadProfile, InsertOneTagWithTinyBuffer);
  FRIEND_TEST(ThreadProfile, InsertTagsNoWrap);
  FRIEND_TEST(ThreadProfile, InsertTagsWrap);
  FRIEND_TEST(ThreadProfile, MemoryMeasure);
  ThreadInfo* mThreadInfo;

  const nsRefPtr<ProfileBuffer> mBuffer;

  
  
  
  
  mozilla::UniquePtr<char[]> mSavedStreamedSamples;
  mozilla::UniquePtr<char[]> mSavedStreamedMarkers;
  mozilla::Maybe<UniqueStacks> mUniqueStacks;

  PseudoStack*   mPseudoStack;
  mozilla::Mutex mMutex;
  int            mThreadId;
  bool           mIsMainThread;
  PlatformData*  mPlatformData;  
  void* const    mStackTop;
  ThreadResponsiveness mRespInfo;

  
  
  
#ifdef XP_LINUX
public:
  int64_t        mRssMemory;
  int64_t        mUssMemory;
#endif
};

#endif 
