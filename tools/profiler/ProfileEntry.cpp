




#include <ostream>
#include "platform.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"


#include "JSObjectBuilder.h"
#include "JSCustomObjectBuilder.h"


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

ProfileEntry::ProfileEntry(char aTagName, int aTagLine)
  : mTagLine(aTagLine)
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
    case 'n': case 'f':
      LOGF("%c %d", mTagName, mTagLine); break;
    case 'h':
      LOGF("%c \'%c\'", mTagName, mTagChar); break;
    case 'r': case 't': case 'p':
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

ThreadProfile::ThreadProfile(const char* aName, int aEntrySize,
                             PseudoStack *aStack, Thread::tid_t aThreadId,
                             PlatformData* aPlatform,
                             bool aIsMainThread, void *aStackTop)
  : mWritePos(0)
  , mLastFlushPos(0)
  , mReadPos(0)
  , mEntrySize(aEntrySize)
  , mPseudoStack(aStack)
  , mMutex("ThreadProfile::mMutex")
  , mName(strdup(aName))
  , mThreadId(aThreadId)
  , mIsMainThread(aIsMainThread)
  , mPlatformData(aPlatform)
  , mGeneration(0)
  , mPendingGenerationFlush(0)
  , mStackTop(aStackTop)
{
  mEntries = new ProfileEntry[mEntrySize];
}

ThreadProfile::~ThreadProfile()
{
  free(mName);
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
  JSCustomObjectBuilder b;
  JSCustomObject *profile = b.CreateObject();
  BuildJSObject(b, profile);
  b.Serialize(profile, stream);
  b.DeleteObject(profile);
}

JSObject* ThreadProfile::ToJSObject(JSContext *aCx)
{
  JSObjectBuilder b(aCx);
  JS::RootedObject profile(aCx, b.CreateObject());
  BuildJSObject(b, profile);
  return profile;
}

template <typename Builder>
void ThreadProfile::BuildJSObject(Builder& b,
                                  typename Builder::ObjectHandle profile)
{
  
  if (XRE_GetProcessType() == GeckoProcessType_Plugin) {
    
    b.DefineProperty(profile, "name", "Plugin");
  } else {
    b.DefineProperty(profile, "name", mName);
  }

  b.DefineProperty(profile, "tid", static_cast<int>(mThreadId));

  typename Builder::RootedArray samples(b.context(), b.CreateArray());
  b.DefineProperty(profile, "samples", samples);

  typename Builder::RootedArray markers(b.context(), b.CreateArray());
  b.DefineProperty(profile, "markers", markers);

  typename Builder::RootedObject sample(b.context());
  typename Builder::RootedArray frames(b.context());

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
      case 'm':
        {
          entry.getMarker()->BuildJSObject(b, markers);
        }
        break;
      case 'r':
        {
          if (sample) {
            b.DefineProperty(sample, "responsiveness", entry.mTagFloat);
          }
        }
        break;
      case 'p':
        {
          if (sample) {
            b.DefineProperty(sample, "power", entry.mTagFloat);
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
      case 's':
        sample = b.CreateObject();
        b.DefineProperty(sample, "name", tagStringData);
        frames = b.CreateArray();
        b.DefineProperty(sample, "frames", frames);
        b.ArrayPush(samples, sample);
        
      case 'c':
      case 'l':
        {
          if (sample) {
            typename Builder::RootedObject frame(b.context(), b.CreateObject());
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
}

template void ThreadProfile::BuildJSObject<JSObjectBuilder>(JSObjectBuilder& b,
                                                            JS::HandleObject profile);
template void ThreadProfile::BuildJSObject<JSCustomObjectBuilder>(JSCustomObjectBuilder& b,
                                                                  JSCustomObject *profile);

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



