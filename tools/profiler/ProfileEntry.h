




#ifndef MOZ_PROFILE_ENTRY_H
#define MOZ_PROFILE_ENTRY_H

#include <ostream>
#include "GeckoProfilerImpl.h"
#include "JSAObjectBuilder.h"
#include "platform.h"
#include "mozilla/Mutex.h"

class ThreadProfile;
class ThreadProfile;

class ProfileEntry
{
public:
  ProfileEntry();

  
  ProfileEntry(char aTagName, const char *aTagData);
  ProfileEntry(char aTagName, void *aTagPtr);
  ProfileEntry(char aTagName, double aTagFloat);
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

  char getTagName() const { return mTagName; }

private:
  friend class ThreadProfile;
  union {
    const char* mTagData;
    char        mTagChars[sizeof(void*)];
    void*       mTagPtr;
    double      mTagFloat;
    Address     mTagAddress;
    uintptr_t   mTagOffset;
    int         mTagLine;
    char        mTagChar;
  };
  char mTagName;
};

typedef void (*IterateTagsCallback)(const ProfileEntry& entry, const char* tagStringData);

class ThreadProfile
{
public:
  ThreadProfile(const char* aName, int aEntrySize, PseudoStack *aStack, int aThreadId, bool aIsMainThread);
  ~ThreadProfile();
  void addTag(ProfileEntry aTag);
  void flush();
  void erase();
  char* processDynamicTag(int readPos, int* tagsConsumed, char* tagBuff);
  void IterateTags(IterateTagsCallback aCallback);
  friend std::ostream& operator<<(std::ostream& stream,
                                  const ThreadProfile& profile);
  void ToStreamAsJSON(std::ostream& stream);
  JSCustomObject *ToJSObject(JSContext *aCx);
  PseudoStack* GetPseudoStack();
  mozilla::Mutex* GetMutex();
  void BuildJSObject(JSAObjectBuilder& b, JSCustomObject* profile);

  bool IsMainThread() const { return mIsMainThread; }
  const char* Name() const { return mName; }
  int ThreadId() const { return mThreadId; }

private:
  
  
  ProfileEntry* mEntries;
  int            mWritePos; 
  int            mLastFlushPos; 
  int            mReadPos;  
  int            mEntrySize;
  PseudoStack*   mPseudoStack;
  mozilla::Mutex mMutex;
  char*          mName;
  int            mThreadId;
  bool           mIsMainThread;
};

std::ostream& operator<<(std::ostream& stream, const ThreadProfile& profile);

#endif 
