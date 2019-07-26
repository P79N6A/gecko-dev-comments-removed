




#include <iostream>
#include "GeckoProfilerImpl.h"
#include "platform.h"
#include "nsThreadUtils.h"


#include "JSObjectBuilder.h"


#include "ProfileEntry2.h"

#if _MSC_VER
 #define snprintf _snprintf
#endif




ProfileEntry2::ProfileEntry2()
  : mTagData(NULL)
  , mTagName(0)
{ }


ProfileEntry2::ProfileEntry2(char aTagName, const char *aTagData)
  : mTagData(aTagData)
  , mTagName(aTagName)
{ }

ProfileEntry2::ProfileEntry2(char aTagName, void *aTagPtr)
  : mTagPtr(aTagPtr)
  , mTagName(aTagName)
{ }

ProfileEntry2::ProfileEntry2(char aTagName, double aTagFloat)
  : mTagFloat(aTagFloat)
  , mTagName(aTagName)
{ }

ProfileEntry2::ProfileEntry2(char aTagName, uintptr_t aTagOffset)
  : mTagOffset(aTagOffset)
  , mTagName(aTagName)
{ }

ProfileEntry2::ProfileEntry2(char aTagName, Address aTagAddress)
  : mTagAddress(aTagAddress)
  , mTagName(aTagName)
{ }

ProfileEntry2::ProfileEntry2(char aTagName, int aTagLine)
  : mTagLine(aTagLine)
  , mTagName(aTagName)
{ }

ProfileEntry2::ProfileEntry2(char aTagName, char aTagChar)
  : mTagChar(aTagChar)
  , mTagName(aTagName)
{ }

bool ProfileEntry2::is_ent_hint(char hintChar) {
  return mTagName == 'h' && mTagChar == hintChar;
}

bool ProfileEntry2::is_ent_hint() {
  return mTagName == 'h';
}

bool ProfileEntry2::is_ent(char tagChar) {
  return mTagName == tagChar;
}

void* ProfileEntry2::get_tagPtr() {
  
  return mTagPtr;
}

void ProfileEntry2::log()
{
  
  
  
  
  
  
  
  
  switch (mTagName) {
    case 'm': case 'c': case 's':
      LOGF("%c \"%s\"", mTagName, mTagData); break;
    case 'd': case 'l': case 'L': case 'S':
      LOGF("%c %p", mTagName, mTagPtr); break;
    case 'n': case 'f':
      LOGF("%c %d", mTagName, mTagLine); break;
    case 'h':
      LOGF("%c \'%c\'", mTagName, mTagChar); break;
    case 'r': case 't':
      LOGF("%c %f", mTagName, mTagFloat); break;
    default:
      LOGF("'%c' unknown_tag", mTagName); break;
  }
}








#define PROFILE_MAX_ENTRY  100000
#define DYNAMIC_MAX_STRING 512

ThreadProfile2::ThreadProfile2(int aEntrySize, PseudoStack *aStack)
  : mWritePos(0)
  , mLastFlushPos(0)
  , mReadPos(0)
  , mEntrySize(aEntrySize)
  , mPseudoStack(aStack)
  , mMutex("ThreadProfile2::mMutex")
{
  mEntries = new ProfileEntry2[mEntrySize];
}

ThreadProfile2::~ThreadProfile2()
{
  delete[] mEntries;
}

void ThreadProfile2::addTag(ProfileEntry2 aTag)
{
  
  mEntries[mWritePos] = aTag;
  mWritePos = (mWritePos + 1) % mEntrySize;
  if (mWritePos == mReadPos) {
    
    mEntries[mReadPos] = ProfileEntry2();
    mReadPos = (mReadPos + 1) % mEntrySize;
  }
  
  
  if (mWritePos == mLastFlushPos) {
    mLastFlushPos = (mLastFlushPos + 1) % mEntrySize;
  }
}


void ThreadProfile2::flush()
{
  mLastFlushPos = mWritePos;
}



















































void ThreadProfile2::erase()
{
  mWritePos = mLastFlushPos;
}

char* ThreadProfile2::processDynamicTag(int readPos,
                                       int* tagsConsumed, char* tagBuff)
{
  int readAheadPos = (readPos + 1) % mEntrySize;
  int tagBuffPos = 0;

  
  bool seenNullByte = false;
  while (readAheadPos != mLastFlushPos && !seenNullByte) {
    (*tagsConsumed)++;
    ProfileEntry2 readAheadEntry = mEntries[readAheadPos];
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

JSCustomObject* ThreadProfile2::ToJSObject(JSContext *aCx)
{
  JSObjectBuilder b(aCx);

  JSCustomObject *profile = b.CreateObject();
  JSCustomArray *samples = b.CreateArray();
  b.DefineProperty(profile, "samples", samples);

  JSCustomObject *sample = NULL;
  JSCustomArray *frames = NULL;

  int readPos = mReadPos;
  while (readPos != mLastFlushPos) {
    
    int incBy = 1;
    ProfileEntry2 entry = mEntries[readPos];

    
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
            JSCustomObject *frame = b.CreateObject();
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

PseudoStack* ThreadProfile2::GetPseudoStack()
{
  return mPseudoStack;
}

mozilla::Mutex* ThreadProfile2::GetMutex()
{
  return &mMutex;
}



