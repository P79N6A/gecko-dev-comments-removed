





































#include <sys/stat.h>   
#include <fcntl.h>      
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <semaphore.h>
#include "sps_sampler.h"
#include "platform.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "prenv.h"

using std::string;

pthread_key_t pkey_stack;
pthread_key_t pkey_ticker;

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

  ProfileEntry(char aTagName, float aTagFloat)
    : mTagFloat(aTagFloat)
    , mLeafAddress(0)
    , mTagName(aTagName)
  { }

  string TagToString(Profile *profile);
  void WriteTag(Profile *profile, FILE* stream);

private:
  union {
    const char* mTagData;
    float mTagFloat;
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
    , mReadPos(0)
    , mEntrySize(aEntrySize)
  {
    mEntries = new ProfileEntry[mEntrySize];
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
  }

  void ToString(string* profile)
  {
    
    
#ifdef ENABLE_SPS_LEAF_DATA
    mMaps = getmaps(getpid());
#endif

    *profile = "";
    int oldReadPos = mReadPos;
    while (mReadPos != mWritePos) {
      *profile += mEntries[mReadPos].TagToString(this);
      mReadPos = (mReadPos + 1) % mEntrySize;
    }
    mReadPos = oldReadPos;
  }

  void WriteProfile(FILE* stream)
  {
    
    
#ifdef ENABLE_SPS_LEAF_DATA
    mMaps = getmaps(getpid());
#endif

    int oldReadPos = mReadPos;
    while (mReadPos != mWritePos) {
      mEntries[mReadPos].WriteTag(this, stream);
      mReadPos = (mReadPos + 1) % mEntrySize;
    }
    mReadPos = oldReadPos;
  }

#ifdef ENABLE_SPS_LEAF_DATA
  MapInfo& getMap()
  {
    return mMaps;
  }
#endif
private:
  
  
  ProfileEntry *mEntries;
  int mWritePos; 
  int mReadPos;  
  int mEntrySize;
#ifdef ENABLE_SPS_LEAF_DATA
  MapInfo mMaps;
#endif
};

class SaveProfileTask;

class TableTicker: public Sampler {
 public:
  explicit TableTicker(int aInterval, int aEntrySize, Stack *aStack)
    : Sampler(aInterval, true)
    , mProfile(aEntrySize)
    , mStack(aStack)
    , mSaveRequested(false)
  {
    mProfile.addTag(ProfileEntry('m', "Start"));
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
  Profile mProfile;
  Stack *mStack;
  bool mSaveRequested;
};





class SaveProfileTask : public nsRunnable {
public:
  SaveProfileTask() {}

  NS_IMETHOD Run() {
    TableTicker *t = (TableTicker*)pthread_getspecific(pkey_ticker);

    char buff[PATH_MAX];
#ifdef ANDROID
  #define FOLDER "/sdcard/"
#else
  #define FOLDER "/tmp/"
#endif
    snprintf(buff, PATH_MAX, FOLDER "profile_%i_%i.txt", XRE_GetProcessType(), getpid());

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


void TableTicker::Tick(TickSample* sample)
{
  
  int i = 0;
  const char *marker = mStack->getMarker(i++);
  for (int i = 0; marker != NULL; i++) {
    mProfile.addTag(ProfileEntry('m', marker));
    marker = mStack->getMarker(i++);
  }
  mStack->mQueueClearMarker = true;

  
  
  
  for (int i = 0; i < mStack->mStackPointer; i++) {
    if (i == 0) {
      Address pc = 0;
      if (sample) {
        pc = sample->pc;
      }
      mProfile.addTag(ProfileEntry('s', mStack->mStack[i], pc));
    } else {
      mProfile.addTag(ProfileEntry('c', mStack->mStack[i]));
    }
  }

  if (!sLastTracerEvent.IsNull()) {
    TimeDuration delta = TimeStamp::Now() - sLastTracerEvent;
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
  } else {
    tag += string(1, mTagName) + string("-") + string(mTagData) + string("\n");
  }

#ifdef ENABLE_SPS_LEAF_DATA
  if (mLeafAddress) {
    bool found = false;
    char tagBuff[1024];
    MapInfo& maps = profile->getMap();
    unsigned long pc = (unsigned long)mLeafAddress;
    
    for (size_t i = 0; i < maps.GetSize(); i++) {
      MapEntry &e = maps.GetEntry(i);
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

void ProfileEntry::WriteTag(Profile *profile, FILE *stream)
{
  fprintf(stream, "%c-%s\n", mTagName, mTagData);

#ifdef ENABLE_SPS_LEAF_DATA
  if (mLeafAddress) {
    bool found = false;
    MapInfo& maps = profile->getMap();
    unsigned long pc = (unsigned long)mLeafAddress;
    
    for (size_t i = 0; i < maps.GetSize(); i++) {
      MapEntry &e = maps.GetEntry(i);
      if (pc > e.GetStart() && pc < e.GetEnd()) {
        if (e.GetName()) {
          found = true;
          fprintf(stream, "l-%s@%li\n", e.GetName(), pc - e.GetStart());
          break;
        }
      }
    }
    if (!found) {
      fprintf(stream, "l-???@%li\n", pc);
    }
  }
#endif
}

#define PROFILE_DEFAULT_ENTRY 100000
#define PROFILE_DEFAULT_INTERVAL 10

void mozilla_sampler_init()
{
  
  
  
  
  
  if (pthread_key_create(&pkey_stack, NULL) ||
        pthread_key_create(&pkey_ticker, NULL)) {
    LOG("Failed to init.");
    return;
  }

  Stack *stack = new Stack();
  pthread_setspecific(pkey_stack, stack);

  
  
  
  const char *val = PR_GetEnv("MOZ_PROFILER_STARTUP");
  if (!val || !*val) {
    return;
  }

  mozilla_sampler_start(PROFILE_DEFAULT_ENTRY, PROFILE_DEFAULT_INTERVAL);
}

void mozilla_sampler_deinit()
{
  mozilla_sampler_stop();
  
  
  
}

void mozilla_sampler_save() {
  TableTicker *t = (TableTicker*)pthread_getspecific(pkey_ticker);
  if (!t) {
    return;
  }

  t->RequestSave();
  
  
  t->HandleSaveRequest();
}

char* mozilla_sampler_get_profile() {
  TableTicker *t = (TableTicker*)pthread_getspecific(pkey_ticker);
  if (!t) {
    return NULL;
  }

  string profile;
  t->GetProfile()->ToString(&profile);

  char *rtn = (char*)malloc( (strlen(profile.c_str())+1) * sizeof(char) );
  strcpy(rtn, profile.c_str());
  return rtn;
}


void mozilla_sampler_start(int aProfileEntries, int aInterval)
{
  Stack *stack = (Stack*)pthread_getspecific(pkey_stack);
  if (!stack) {
    ASSERT(false);
    return;
  }

  mozilla_sampler_stop();

  TableTicker *t = new TableTicker(aInterval, aProfileEntries, stack);
  pthread_setspecific(pkey_ticker, t);
  t->Start();
}

void mozilla_sampler_stop()
{
  TableTicker *t = (TableTicker*)pthread_getspecific(pkey_ticker);
  if (!t) {
    return;
  }

  t->Stop();
  pthread_setspecific(pkey_ticker, NULL);
}

bool mozilla_sampler_is_active()
{
  TableTicker *t = (TableTicker*)pthread_getspecific(pkey_ticker);
  if (!t) {
    return false;
  }

  return t->IsActive();
}

float sResponsivenessTimes[100];
float sCurrResponsiveness = 0.f;
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

  sLastTracerEvent = aTime;
}

const float* mozilla_sampler_get_responsiveness()
{
  return sResponsivenessTimes;
}

