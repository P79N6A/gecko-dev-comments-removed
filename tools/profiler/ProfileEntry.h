





#ifndef MOZ_PROFILE_ENTRY_H
#define MOZ_PROFILE_ENTRY_H

#include <ostream>
#include "GeckoProfiler.h"
#include "platform.h"
#include "ProfileJSONWriter.h"
#include "ProfilerBacktrace.h"
#include "mozilla/RefPtr.h"
#include <string>
#include <map>
#ifndef SPS_STANDALONE
#include "js/ProfilingFrameIterator.h"
#include "js/TrackedOptimizationInfo.h"
#include "nsHashKeys.h"
#include "nsDataHashtable.h"
#endif
#include "mozilla/Maybe.h"
#include "mozilla/Vector.h"
#ifndef SPS_STANDALONE
#include "gtest/MozGtestFriend.h"
#else
#define FRIEND_TEST(a, b)
#endif
#include "mozilla/HashFunctions.h"
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
  ProfileEntry(char aTagName, double aTagDouble);
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
    double      mTagDouble;
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

  struct StringKey {

    explicit StringKey(const char* aStr)
     : mStr(strdup(aStr))
    {
      mHash = mozilla::HashString(mStr);
    }

    StringKey(const StringKey& aOther)
      : mStr(strdup(aOther.mStr))
    {
      mHash = aOther.mHash;
    }

    ~StringKey() {
      free(mStr);
    }

    uint32_t Hash() const;
    bool operator==(const StringKey& aOther) const {
      return strcmp(mStr, aOther.mStr) == 0;
    }
    bool operator<(const StringKey& aOther) const {
      return mHash < aOther.mHash;
    }

  private:
    uint32_t mHash;
    char* mStr;
  };
private:
  SpliceableChunkedJSONWriter mStringTableWriter;
  std::map<StringKey, uint32_t> mStringToIndexMap;
};

class UniqueStacks
{
public:
  struct FrameKey {
    std::string mLocation;
    mozilla::Maybe<unsigned> mLine;
    mozilla::Maybe<unsigned> mCategory;
    mozilla::Maybe<void*> mJITAddress;
    mozilla::Maybe<uint32_t> mJITDepth;

    explicit FrameKey(const char* aLocation)
     : mLocation(aLocation)
    {
      mHash = Hash();
    }

    FrameKey(const FrameKey& aToCopy)
     : mLocation(aToCopy.mLocation)
     , mLine(aToCopy.mLine)
     , mCategory(aToCopy.mCategory)
     , mJITAddress(aToCopy.mJITAddress)
     , mJITDepth(aToCopy.mJITDepth)
    {
      mHash = Hash();
    }

    FrameKey(void* aJITAddress, uint32_t aJITDepth)
     : mJITAddress(mozilla::Some(aJITAddress))
     , mJITDepth(mozilla::Some(aJITDepth))
    {
      mHash = Hash();
    }

    uint32_t Hash() const;
    bool operator==(const FrameKey& aOther) const;
    bool operator<(const FrameKey& aOther) const {
      return mHash < aOther.mHash;
    }

  private:
    uint32_t mHash;
  };

  
  struct MOZ_STACK_CLASS OnStackFrameKey : public FrameKey {
    explicit OnStackFrameKey(const char* aLocation)
      : FrameKey(aLocation)
#ifndef SPS_STANDALONE
      , mJITFrameHandle(nullptr)
#endif
    { }

    OnStackFrameKey(const OnStackFrameKey& aToCopy)
      : FrameKey(aToCopy)
#ifndef SPS_STANDALONE
      , mJITFrameHandle(aToCopy.mJITFrameHandle)
#endif
    { }

#ifndef SPS_STANDALONE
    const JS::ForEachProfiledFrameOp::FrameHandle* mJITFrameHandle;

    OnStackFrameKey(void* aJITAddress, unsigned aJITDepth)
      : FrameKey(aJITAddress, aJITDepth)
      , mJITFrameHandle(nullptr)
    { }

    OnStackFrameKey(void* aJITAddress, unsigned aJITDepth,
                    const JS::ForEachProfiledFrameOp::FrameHandle& aJITFrameHandle)
      : FrameKey(aJITAddress, aJITDepth)
      , mJITFrameHandle(&aJITFrameHandle)
    { }
#endif
  };

  struct StackKey {
    mozilla::Maybe<uint32_t> mPrefixHash;
    mozilla::Maybe<uint32_t> mPrefix;
    uint32_t mFrame;

    explicit StackKey(uint32_t aFrame)
     : mFrame(aFrame)
    {
      mHash = Hash();
    }

    uint32_t Hash() const;
    bool operator==(const StackKey& aOther) const;
    bool operator<(const StackKey& aOther) const {
      return mHash < aOther.mHash;
    }

    void UpdateHash(uint32_t aPrefixHash, uint32_t aPrefix, uint32_t aFrame) {
      mPrefixHash = mozilla::Some(aPrefixHash);
      mPrefix = mozilla::Some(aPrefix);
      mFrame = aFrame;
      mHash = Hash();
    }

  private:
    uint32_t mHash;
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

  
  
  
  
  std::map<void*, uint32_t> mJITFrameDepthMap;

  uint32_t mFrameCount;
  SpliceableChunkedJSONWriter mFrameTableWriter;
#ifdef SPS_STANDALNOE
  std::map<FrameKey, uint32_t> mFrameToIndexMap;
#else
  nsDataHashtable<nsGenericHashKey<FrameKey>, uint32_t> mFrameToIndexMap;
#endif

  SpliceableChunkedJSONWriter mStackTableWriter;

  
  
  
#ifdef SPS_STANDALONE
  std::map<StackKey, uint32_t> mStackToIndexMap;
#else
  nsDataHashtable<nsGenericHashKey<StackKey>, uint32_t> mStackToIndexMap;
#endif
};

class ProfileBuffer : public mozilla::RefCounted<ProfileBuffer> {
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(ProfileBuffer)

  explicit ProfileBuffer(int aEntrySize);

  virtual ~ProfileBuffer();

  void addTag(const ProfileEntry& aTag);
  void StreamSamplesToJSON(SpliceableJSONWriter& aWriter, int aThreadId, double aSinceTime,
                           JSRuntime* rt, UniqueStacks& aUniqueStacks);
  void StreamMarkersToJSON(SpliceableJSONWriter& aWriter, int aThreadId, double aSinceTime,
                           UniqueStacks& aUniqueStacks);
  void DuplicateLastSample(int aThreadId);

  void addStoredMarker(ProfilerMarker* aStoredMarker);

  
  void deleteExpiredStoredMarkers();
  void reset();

protected:
  char* processDynamicTag(int readPos, int* tagsConsumed, char* tagBuff);
  int FindLastSampleOfThread(int aThreadId);

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
  Mutex& GetMutex();
  void StreamJSON(SpliceableJSONWriter& aWriter, double aSinceTime = 0);

  



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
#ifndef SPS_STANDALONE
  ThreadResponsiveness* GetThreadResponsiveness() { return &mRespInfo; }
#endif
  void SetPendingDelete()
  {
    mPseudoStack = nullptr;
    mPlatformData = nullptr;
  }

  uint32_t bufferGeneration() const {
    return mBuffer->mGeneration;
  }

protected:
  void StreamSamplesAndMarkers(SpliceableJSONWriter& aWriter, double aSinceTime,
                               UniqueStacks& aUniqueStacks);

private:
  FRIEND_TEST(ThreadProfile, InsertOneTag);
  FRIEND_TEST(ThreadProfile, InsertOneTagWithTinyBuffer);
  FRIEND_TEST(ThreadProfile, InsertTagsNoWrap);
  FRIEND_TEST(ThreadProfile, InsertTagsWrap);
  FRIEND_TEST(ThreadProfile, MemoryMeasure);
  ThreadInfo* mThreadInfo;

  const mozilla::RefPtr<ProfileBuffer> mBuffer;

  
  
  
  
  mozilla::UniquePtr<char[]> mSavedStreamedSamples;
  mozilla::UniquePtr<char[]> mSavedStreamedMarkers;
  mozilla::Maybe<UniqueStacks> mUniqueStacks;

  PseudoStack*   mPseudoStack;
  mozilla::UniquePtr<Mutex>  mMutex;
  int            mThreadId;
  bool           mIsMainThread;
  PlatformData*  mPlatformData;  
  void* const    mStackTop;
#ifndef SPS_STANDALONE
  ThreadResponsiveness mRespInfo;
#endif

  
  
  
#ifdef XP_LINUX
public:
  int64_t        mRssMemory;
  int64_t        mUssMemory;
#endif
};

#endif 
