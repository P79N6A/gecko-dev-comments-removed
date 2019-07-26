





#ifndef MOZ_PROFILE_ENTRY_H
#define MOZ_PROFILE_ENTRY_H

#include <ostream>
#include "GeckoProfiler.h"
#include "platform.h"
#include "JSStreamWriter.h"
#include "ProfilerBacktrace.h"
#include "mozilla/Mutex.h"
#include "gtest/MozGtestFriend.h"

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
  friend class ThreadProfile;
  union {
    const char* mTagData;
    char        mTagChars[sizeof(void*)];
    void*       mTagPtr;
    ProfilerMarker* mTagMarker;
    float       mTagFloat;
    Address     mTagAddress;
    uintptr_t   mTagOffset;
    int         mTagLine;
    char        mTagChar;
  };
  char mTagName;
};

#pragma pack(pop)

typedef void (*IterateTagsCallback)(const ProfileEntry& entry, const char* tagStringData);

class ThreadProfile
{
public:
  ThreadProfile(const char* aName, int aEntrySize, PseudoStack *aStack,
                Thread::tid_t aThreadId, PlatformData* aPlatformData,
                bool aIsMainThread, void *aStackTop);
  virtual ~ThreadProfile();
  void addTag(ProfileEntry aTag);
  void flush();
  void erase();
  char* processDynamicTag(int readPos, int* tagsConsumed, char* tagBuff);
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
  const char* Name() const { return mName; }
  Thread::tid_t ThreadId() const { return mThreadId; }

  PlatformData* GetPlatformData() { return mPlatformData; }
  int GetGenerationID() const { return mGeneration; }
  bool HasGenerationExpired(int aGenID) {
    return aGenID + 2 <= mGeneration;
  }
  void* GetStackTop() const { return mStackTop; }
  void DuplicateLastSample();
private:
  FRIEND_TEST(ThreadProfile, InsertOneTag);
  FRIEND_TEST(ThreadProfile, InsertOneTagWithTinyBuffer);
  FRIEND_TEST(ThreadProfile, InsertTagsNoWrap);
  FRIEND_TEST(ThreadProfile, InsertTagsWrap);
  FRIEND_TEST(ThreadProfile, MemoryMeasure);
  
  
  ProfileEntry*  mEntries;
  int            mWritePos; 
  int            mLastFlushPos; 
  int            mReadPos;  
  int            mEntrySize;
  PseudoStack*   mPseudoStack;
  mozilla::Mutex mMutex;
  char*          mName;
  Thread::tid_t  mThreadId;
  bool           mIsMainThread;
  PlatformData*  mPlatformData;  
  int            mGeneration;
  int            mPendingGenerationFlush;
  void* const    mStackTop;

  
  
  
#ifdef XP_LINUX
public:
  int64_t        mRssMemory;
#endif
};

std::ostream& operator<<(std::ostream& stream, const ThreadProfile& profile);

#endif 
