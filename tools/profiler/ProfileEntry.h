





#ifndef MOZ_PROFILE_ENTRY_H
#define MOZ_PROFILE_ENTRY_H

#include <ostream>
#include "GeckoProfiler.h"
#include "platform.h"
#include "JSStreamWriter.h"
#include "ProfilerBacktrace.h"
#include "nsRefPtr.h"
#include "mozilla/Mutex.h"
#include "gtest/MozGtestFriend.h"
#include "mozilla/UniquePtr.h"

class ThreadProfile;

#pragma pack(push, 1)

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
  friend std::ostream& operator<<(std::ostream& stream, const ProfileEntry& entry);
  bool is_ent_hint(char hintChar);
  bool is_ent_hint();
  bool is_ent(char tagName);
  void* get_tagPtr();
  void log();
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

#pragma pack(pop)

typedef void (*IterateTagsCallback)(const ProfileEntry& entry, const char* tagStringData);

class ProfileBuffer {
public:
  NS_INLINE_DECL_REFCOUNTING(ProfileBuffer)

  explicit ProfileBuffer(int aEntrySize);

  void addTag(const ProfileEntry& aTag);
  void IterateTagsForThread(IterateTagsCallback aCallback, int aThreadId);
  void StreamSamplesToJSObject(JSStreamWriter& b, int aThreadId);
  void StreamMarkersToJSObject(JSStreamWriter& b, int aThreadId);
  void DuplicateLastSample(int aThreadId);

  void addStoredMarker(ProfilerMarker* aStoredMarker);
  void deleteExpiredStoredMarkers();

  std::ostream& StreamToOStream(std::ostream& stream, int aThreadId) const;

protected:
  char* processDynamicTag(int readPos, int* tagsConsumed, char* tagBuff);
  int FindLastSampleOfThread(int aThreadId);

  ~ProfileBuffer() {}

public:
  
  mozilla::UniquePtr<ProfileEntry[]> mEntries;

  
  
  int mWritePos;

  
  int mReadPos;

  
  int mEntrySize;

  
  int mGeneration;

  
  ProfilerMarkerLinkedList mStoredMarkers;
};

class ThreadProfile
{
public:
  ThreadProfile(ThreadInfo* aThreadInfo, ProfileBuffer* aBuffer);
  virtual ~ThreadProfile();
  void addTag(const ProfileEntry& aTag);

  




  void addStoredMarker(ProfilerMarker *aStoredMarker);

  void IterateTags(IterateTagsCallback aCallback);
  friend std::ostream& operator<<(std::ostream& stream,
                                  const ThreadProfile& profile);
  void ToStreamAsJSON(std::ostream& stream);
  JSObject *ToJSObject(JSContext *aCx);
  PseudoStack* GetPseudoStack();
  mozilla::Mutex* GetMutex();
  void StreamJSObject(JSStreamWriter& b);
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
private:
  FRIEND_TEST(ThreadProfile, InsertOneTag);
  FRIEND_TEST(ThreadProfile, InsertOneTagWithTinyBuffer);
  FRIEND_TEST(ThreadProfile, InsertTagsNoWrap);
  FRIEND_TEST(ThreadProfile, InsertTagsWrap);
  FRIEND_TEST(ThreadProfile, MemoryMeasure);
  ThreadInfo* mThreadInfo;

  const nsRefPtr<ProfileBuffer> mBuffer;

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

std::ostream& operator<<(std::ostream& stream, const ThreadProfile& profile);

#endif 
