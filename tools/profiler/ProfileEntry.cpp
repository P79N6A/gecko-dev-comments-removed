




#include <ostream>
#include <sstream>
#include "platform.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "jsapi.h"


#include "JSStreamWriter.h"


#include "ProfileEntry.h"

#if _MSC_VER
 #define snprintf _snprintf
#endif




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

void ProfileEntry::log()
{
  
  
  
  
  
  
  
  
  
  switch (mTagName) {
    case 'm':
      LOGF("%c \"%s\"", mTagName, mTagMarker->GetMarkerName()); break;
    case 'c': case 's':
      LOGF("%c \"%s\"", mTagName, mTagData); break;
    case 'd': case 'l': case 'L': case 'B': case 'S':
      LOGF("%c %p", mTagName, mTagPtr); break;
    case 'n': case 'f': case 'y':
      LOGF("%c %d", mTagName, mTagInt); break;
    case 'h':
      LOGF("%c \'%c\'", mTagName, mTagChar); break;
    case 'r': case 't': case 'p': case 'R': case 'U':
      LOGF("%c %f", mTagName, mTagFloat); break;
    default:
      LOGF("'%c' unknown_tag", mTagName); break;
  }
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








#define DYNAMIC_MAX_STRING 512

ThreadProfile::ThreadProfile(ThreadInfo* aInfo, int aEntrySize)
  : mThreadInfo(aInfo)
  , mWritePos(0)
  , mLastFlushPos(0)
  , mReadPos(0)
  , mEntrySize(aEntrySize)
  , mPseudoStack(aInfo->Stack())
  , mMutex("ThreadProfile::mMutex")
  , mThreadId(aInfo->ThreadId())
  , mIsMainThread(aInfo->IsMainThread())
  , mPlatformData(aInfo->GetPlatformData())
  , mGeneration(0)
  , mPendingGenerationFlush(0)
  , mStackTop(aInfo->StackTop())
  , mRespInfo(MOZ_THIS_IN_INITIALIZER_LIST())
#ifdef XP_LINUX
  , mRssMemory(0)
  , mUssMemory(0)
#endif
{
  MOZ_COUNT_CTOR(ThreadProfile);
  mEntries = new ProfileEntry[mEntrySize];
}

ThreadProfile::~ThreadProfile()
{
  MOZ_COUNT_DTOR(ThreadProfile);
  delete[] mEntries;
}

void ThreadProfile::addTag(ProfileEntry aTag)
{
  
  mEntries[mWritePos] = aTag;
  mWritePos = mWritePos + 1;
  if (mWritePos >= mEntrySize) {
    mPendingGenerationFlush++;
    mWritePos = mWritePos % mEntrySize;
  }
  if (mWritePos == mReadPos) {
    
    mEntries[mReadPos] = ProfileEntry();
    mReadPos = (mReadPos + 1) % mEntrySize;
  }
  
  
  if (mWritePos == mLastFlushPos) {
    mLastFlushPos = (mLastFlushPos + 1) % mEntrySize;
  }
}


void ThreadProfile::flush()
{
  mLastFlushPos = mWritePos;
  mGeneration += mPendingGenerationFlush;
  mPendingGenerationFlush = 0;
}



















































void ThreadProfile::erase()
{
  mWritePos = mLastFlushPos;
  mPendingGenerationFlush = 0;
}

char* ThreadProfile::processDynamicTag(int readPos,
                                       int* tagsConsumed, char* tagBuff)
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

void ThreadProfile::IterateTags(IterateTagsCallback aCallback)
{
  MOZ_ASSERT(aCallback);

  int readPos = mReadPos;
  while (readPos != mLastFlushPos) {
    
    int incBy = 1;
    const ProfileEntry& entry = mEntries[readPos];

    
    const char* tagStringData = entry.mTagData;
    int readAheadPos = (readPos + 1) % mEntrySize;
    char tagBuff[DYNAMIC_MAX_STRING];
    
    tagBuff[DYNAMIC_MAX_STRING-1] = '\0';

    if (readAheadPos != mLastFlushPos && mEntries[readAheadPos].mTagName == 'd') {
      tagStringData = processDynamicTag(readPos, &incBy, tagBuff);
    }

    aCallback(entry, tagStringData);

    readPos = (readPos + incBy) % mEntrySize;
  }
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
    } else {
      b.NameValue("name", Name());
    }
    b.NameValue("tid", static_cast<int>(mThreadId));

    b.Name("samples");
    b.BeginArray();

      bool sample = false;
      int readPos = mReadPos;
      while (readPos != mLastFlushPos) {
        
        ProfileEntry entry = mEntries[readPos];

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
                while (framePos != mLastFlushPos && frame.mTagName != 's') {
                  int incBy = 1;
                  frame = mEntries[framePos];

                  
                  const char* tagStringData = frame.mTagData;
                  int readAheadPos = (framePos + 1) % mEntrySize;
                  char tagBuff[DYNAMIC_MAX_STRING];
                  
                  
                  tagBuff[DYNAMIC_MAX_STRING-1] = '\0';

                  if (readAheadPos != mLastFlushPos && mEntries[readAheadPos].mTagName == 'd') {
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
                      if (readAheadPos != mLastFlushPos &&
                          mEntries[readAheadPos].mTagName == 'n') {
                        b.NameValue("line", mEntries[readAheadPos].mTagInt);
                        incBy++;
                      }
                      readAheadPos = (framePos + incBy) % mEntrySize;
                      if (readAheadPos != mLastFlushPos &&
                          mEntries[readAheadPos].mTagName == 'y') {
                        b.NameValue("category", mEntries[readAheadPos].mTagInt);
                        incBy++;
                      }
                    b.EndObject();
                  }
                  framePos = (framePos + incBy) % mEntrySize;
                }
              b.EndArray();
            }
            break;
        }
        readPos = (readPos + 1) % mEntrySize;
      }
      if (sample) {
        b.EndObject();
      }
    b.EndArray();

    b.Name("markers");
    b.BeginArray();
      readPos = mReadPos;
      while (readPos != mLastFlushPos) {
        ProfileEntry entry = mEntries[readPos];
        if (entry.mTagName == 'm') {
           entry.getMarker()->StreamJSObject(b);
        }
        readPos = (readPos + 1) % mEntrySize;
      }
    b.EndArray();
  b.EndObject();
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

void ThreadProfile::DuplicateLastSample() {
  
  
  
  for (int readPos  = (mWritePos + mEntrySize - 1) % mEntrySize;
           readPos !=  (mReadPos + mEntrySize - 1) % mEntrySize;
           readPos  =   (readPos + mEntrySize - 1) % mEntrySize) {
    if (mEntries[readPos].mTagName == 's') {
      
      int copyEndIdx = mWritePos;
      
      for (;readPos != copyEndIdx; readPos = (readPos + 1) % mEntrySize) {
        switch (mEntries[readPos].mTagName) {
          
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
      break;
    }
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



