





































#include <string>
#include <stdio.h>
#include "sps_sampler.h"
#include "platform.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "shared-libraries.h"
#include "mozilla/StringBuilder.h"


#if defined(MOZ_PROFILING) && (defined(XP_MACOSX) || defined(XP_UNIX))
 #ifndef ANDROID
  #define USE_BACKTRACE
 #endif
#endif
#ifdef USE_BACKTRACE
 #include <execinfo.h>
#endif

#if defined(MOZ_PROFILING) && defined(XP_WIN)
 #define USE_NS_STACKWALK
#endif
#ifdef USE_NS_STACKWALK
 #include "nsStackWalk.h"
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


mozilla::tls::key pkey_stack;
mozilla::tls::key pkey_ticker;




bool stack_key_initialized;

TimeStamp sLastTracerEvent;

class Profile;

class ProfileEntry
{
public:
  ProfileEntry()
    : mTagData(NULL)
    , mLeafAddress(0)
    , mTagName(0)
  { }

  
  ProfileEntry(char aTagName, const char *aTagData)
    : mTagData(aTagData)
    , mLeafAddress(0)
    , mTagName(aTagName)
  { }

  
  ProfileEntry(char aTagName, const char *aTagData, Address aLeafAddress)
    : mTagData(aTagData)
    , mLeafAddress(aLeafAddress)
    , mTagName(aTagName)
  { }

  ProfileEntry(char aTagName, double aTagFloat)
    : mTagFloat(aTagFloat)
    , mLeafAddress(0)
    , mTagName(aTagName)
  { }

  ProfileEntry(char aTagName, uintptr_t aTagOffset)
    : mTagOffset(aTagOffset)
    , mLeafAddress(0)
    , mTagName(aTagName)
  { }

  string TagToString(Profile *profile);

private:
  union {
    const char* mTagData;
    double mTagFloat;
    Address mTagAddress;
    uintptr_t mTagOffset;
  };
  Address mLeafAddress;
  char mTagName;
};

#define PROFILE_MAX_ENTRY 100000
class Profile
{
public:
  Profile(int aEntrySize)
    : mWritePos(0)
    , mLastFlushPos(0)
    , mReadPos(0)
    , mEntrySize(aEntrySize)
  {
    mEntries = new ProfileEntry[mEntrySize];
    mNeedsSharedLibraryInfo = true;
  }

  ~Profile()
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

  void ToString(StringBuilder &profile)
  {
    if (mNeedsSharedLibraryInfo) {
      
      
      mSharedLibraryInfo = SharedLibraryInfo::GetInfoForSelf();
    }

    
    int oldReadPos = mReadPos;
    while (mReadPos != mLastFlushPos) {
      profile.Append(mEntries[mReadPos].TagToString(this).c_str());
      mReadPos = (mReadPos + 1) % mEntrySize;
    }
    mReadPos = oldReadPos;
  }

  void WriteProfile(FILE* stream)
  {
    if (mNeedsSharedLibraryInfo) {
      
      
      mSharedLibraryInfo = SharedLibraryInfo::GetInfoForSelf();
    }

    
    int oldReadPos = mReadPos;
    while (mReadPos != mLastFlushPos) {
      string tag = mEntries[mReadPos].TagToString(this);
      fwrite(tag.data(), 1, tag.length(), stream);
      mReadPos = (mReadPos + 1) % mEntrySize;
    }
    mReadPos = oldReadPos;
  }

  SharedLibraryInfo& getSharedLibraryInfo()
  {
    return mSharedLibraryInfo;
  }
private:
  
  
  ProfileEntry *mEntries;
  int mWritePos; 
  int mLastFlushPos; 
  int mReadPos;  
  int mEntrySize;
  bool mNeedsSharedLibraryInfo;
  SharedLibraryInfo mSharedLibraryInfo;
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
  TableTicker(int aInterval, int aEntrySize, Stack *aStack,
              const char** aFeatures, uint32_t aFeatureCount)
    : Sampler(aInterval, true)
    , mProfile(aEntrySize)
    , mStack(aStack)
    , mSaveRequested(false)
  {
    mUseStackWalk = hasFeature(aFeatures, aFeatureCount, "stackwalk");
    mProfile.addTag(ProfileEntry('m', "Start"));
    
    mJankOnly = hasFeature(aFeatures, aFeatureCount, "jank");
  }

  ~TableTicker() { if (IsActive()) Stop(); }

  virtual void SampleStack(TickSample* sample) {}

  
  virtual void Tick(TickSample* sample);

  
  virtual void RequestSave()
  {
    mSaveRequested = true;
  }

  virtual void HandleSaveRequest();

  Stack* GetStack()
  {
    return mStack;
  }

  Profile* GetProfile()
  {
    return &mProfile;
  }

private:
  
  void doBacktrace(Profile &aProfile);

private:
  Profile mProfile;
  Stack *mStack;
  bool mSaveRequested;
  bool mUseStackWalk;
  bool mJankOnly;
};





class SaveProfileTask : public nsRunnable {
public:
  SaveProfileTask() {}

  NS_IMETHOD Run() {
    TableTicker *t = mozilla::tls::get<TableTicker>(pkey_ticker);

    char buff[MAXPATHLEN];
#ifdef ANDROID
  #define FOLDER "/sdcard/"
#elif defined(XP_WIN)
  #define FOLDER "%TEMP%\\"
#else
  #define FOLDER "/tmp/"
#endif

    snprintf(buff, MAXPATHLEN, "%sprofile_%i_%i.txt", FOLDER, XRE_GetProcessType(), getpid());

#ifdef XP_WIN
    
    {
      char tmp[MAXPATHLEN];
      ExpandEnvironmentStringsA(buff, tmp, mozilla::ArrayLength(tmp));
      strcpy(buff, tmp);
    }
#endif

    FILE* stream = ::fopen(buff, "w");
    if (stream) {
      t->GetProfile()->WriteProfile(stream);
      ::fclose(stream);
      LOG("Saved to " FOLDER "profile_TYPE_PID.txt");
    } else {
      LOG("Fail to open profile log file.");
    }

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

#ifdef USE_BACKTRACE
void TableTicker::doBacktrace(Profile &aProfile)
{
  void *array[100];
  int count = backtrace (array, 100);

  aProfile.addTag(ProfileEntry('s', "(root)", 0));

  for (int i = 0; i < count; i++) {
    if( (intptr_t)array[i] == -1 ) break;
    aProfile.addTag(ProfileEntry('l', (const char*)array[i]));
  }
}
#endif

#ifdef USE_NS_STACKWALK
typedef struct {
  void** array;
  size_t size;
  size_t count;
} PCArray;

static
void StackWalkCallback(void* aPC, void* aClosure)
{
  PCArray* array = static_cast<PCArray*>(aClosure);
  if (array->count >= array->size) {
    
    return;
  }
  array->array[array->count++] = aPC;
}

void TableTicker::doBacktrace(Profile &aProfile)
{
  uintptr_t thread = GetThreadHandle(platform_data());
  MOZ_ASSERT(thread);
  void* pc_array[1000];
  PCArray array = {
    pc_array,
    mozilla::ArrayLength(pc_array),
    0
  };
  nsresult rv = NS_StackWalk(StackWalkCallback, 0, &array, thread);
  if (NS_SUCCEEDED(rv)) {
    aProfile.addTag(ProfileEntry('s', "(root)", 0));

    for (size_t i = array.count; i > 0; --i) {
      aProfile.addTag(ProfileEntry('l', (const char*)array.array[i - 1]));
    }
  }
}
#endif

static
void doSampleStackTrace(Stack *aStack, Profile &aProfile, TickSample *sample)
{
  
  
  
  for (int i = 0; i < aStack->mStackPointer; i++) {
    if (i == 0) {
      Address pc = 0;
      if (sample) {
        pc = sample->pc;
      }
      aProfile.addTag(ProfileEntry('s', aStack->mStack[i], pc));
    } else {
      aProfile.addTag(ProfileEntry('c', aStack->mStack[i]));
    }
  }
}


unsigned int sLastSampledEventGeneration = 0;




unsigned int sCurrentEventGeneration = 0;





void TableTicker::Tick(TickSample* sample)
{
  
  int i = 0;
  const char *marker = mStack->getMarker(i++);
  for (int i = 0; marker != NULL; i++) {
    mProfile.addTag(ProfileEntry('m', marker));
    marker = mStack->getMarker(i++);
  }
  mStack->mQueueClearMarker = true;

  
  
  if (sLastSampledEventGeneration != sCurrentEventGeneration) {
    
    
    
    mProfile.erase();
  }
  sLastSampledEventGeneration = sCurrentEventGeneration;

  bool recordSample = true;
  if (mJankOnly) {
    recordSample = false;
    
    if (!sLastTracerEvent.IsNull()) {
      TimeDuration delta = sample->timestamp - sLastTracerEvent;
      if (delta.ToMilliseconds() > 100.0) {
          recordSample = true;
      }
    }
  }

#if defined(USE_BACKTRACE) || defined(USE_NS_STACKWALK)
  if (mUseStackWalk) {
    doBacktrace(mProfile);
  } else {
    doSampleStackTrace(mStack, mProfile, sample);
  }
#else
  doSampleStackTrace(mStack, mProfile, sample);
#endif

  if (recordSample)
    mProfile.flush();

  if (!mJankOnly && !sLastTracerEvent.IsNull() && sample) {
    TimeDuration delta = sample->timestamp - sLastTracerEvent;
    mProfile.addTag(ProfileEntry('r', delta.ToMilliseconds()));
  }
}

string ProfileEntry::TagToString(Profile *profile)
{
  string tag = "";
  if (mTagName == 'r') {
    char buff[50];
    snprintf(buff, 50, "%-40f", mTagFloat);
    tag += string(1, mTagName) + string("-") + string(buff) + string("\n");
  } else if (mTagName == 'l') {
    bool found = false;
    char tagBuff[1024];
    SharedLibraryInfo& shlibInfo = profile->getSharedLibraryInfo();
    Address pc = mTagAddress;
    
    for (size_t i = 0; i < shlibInfo.GetSize(); i++) {
      SharedLibrary &e = shlibInfo.GetEntry(i);
      if (pc > (Address)e.GetStart() && pc < (Address)e.GetEnd()) {
        if (e.GetName()) {
          found = true;

          snprintf(tagBuff, 1024, "l-%s@%p\n", e.GetName(), pc - e.GetStart());
          tag += string(tagBuff);

          break;
        }
      }
    }
    if (!found) {
      snprintf(tagBuff, 1024, "l-???@%p\n", pc);
      tag += string(tagBuff);
    }
  } else {
    tag += string(1, mTagName) + string("-") + string(mTagData) + string("\n");
  }

#ifdef ENABLE_SPS_LEAF_DATA
  if (mLeafAddress) {
    bool found = false;
    char tagBuff[1024];
    SharedLibraryInfo& shlibInfo = profile->getSharedLibraryInfo();
    unsigned long pc = (unsigned long)mLeafAddress;
    
    for (size_t i = 0; i < shlibInfo.GetSize(); i++) {
      SharedLibrary &e = shlibInfo.GetEntry(i);
      if (pc > e.GetStart() && pc < e.GetEnd()) {
        if (e.GetName()) {
          found = true;
          snprintf(tagBuff, 1024, "l-%900s@%llu\n", e.GetName(), pc - e.GetStart());
          tag += string(tagBuff);
          break;
        }
      }
    }
    if (!found) {
      snprintf(tagBuff, 1024, "l-???@%llu\n", pc);
      tag += string(tagBuff);
    }
  }
#endif
  return tag;
}

#define PROFILE_DEFAULT_ENTRY 100000
#define PROFILE_DEFAULT_INTERVAL 10
#define PROFILE_DEFAULT_FEATURES NULL
#define PROFILE_DEFAULT_FEATURE_COUNT 0

void mozilla_sampler_init()
{
  
  if (!mozilla::tls::create(&pkey_stack) ||
      !mozilla::tls::create(&pkey_ticker)) {
    LOG("Failed to init.");
    return;
  }
  stack_key_initialized = true;

  Stack *stack = new Stack();
  mozilla::tls::set(pkey_stack, stack);

  
  
  
  const char *val = PR_GetEnv("MOZ_PROFILER_STARTUP");
  if (!val || !*val) {
    return;
  }

  mozilla_sampler_start(PROFILE_DEFAULT_ENTRY, PROFILE_DEFAULT_INTERVAL,
                        PROFILE_DEFAULT_FEATURES, PROFILE_DEFAULT_FEATURE_COUNT);
}

void mozilla_sampler_deinit()
{
  mozilla_sampler_stop();
  
  
  
}

void mozilla_sampler_save()
{
  TableTicker *t = mozilla::tls::get<TableTicker>(pkey_ticker);
  if (!t) {
    return;
  }

  t->RequestSave();
  
  
  t->HandleSaveRequest();
}

char* mozilla_sampler_get_profile()
{
  TableTicker *t = mozilla::tls::get<TableTicker>(pkey_ticker);
  if (!t) {
    return NULL;
  }

  StringBuilder profile;
  t->GetProfile()->ToString(profile);

  char *rtn = (char*)malloc( (profile.Length()+1) * sizeof(char) );
  strcpy(rtn, profile.Buffer());
  return rtn;
}

const char** mozilla_sampler_get_features()
{
  static const char* features[] = {
#if defined(MOZ_PROFILING) && (defined(USE_BACKTRACE) || defined(USE_NS_STACKWALK))
    "stackwalk",
#endif
    NULL
  };

  return features;
}


void mozilla_sampler_start(int aProfileEntries, int aInterval,
                           const char** aFeatures, uint32_t aFeatureCount)
{
  Stack *stack = mozilla::tls::get<Stack>(pkey_stack);
  if (!stack) {
    ASSERT(false);
    return;
  }

  mozilla_sampler_stop();

  TableTicker *t = new TableTicker(aInterval, aProfileEntries, stack,
                                   aFeatures, aFeatureCount);
  mozilla::tls::set(pkey_ticker, t);
  t->Start();
}

void mozilla_sampler_stop()
{
  TableTicker *t = mozilla::tls::get<TableTicker>(pkey_ticker);
  if (!t) {
    return;
  }

  t->Stop();
  mozilla::tls::set(pkey_ticker, (Stack*)NULL);
}

bool mozilla_sampler_is_active()
{
  TableTicker *t = mozilla::tls::get<TableTicker>(pkey_ticker);
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

