





































#include <sys/stat.h>   
#include <fcntl.h>      
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>
#include "sps_sampler.h"
#include "platform.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "prenv.h"

pthread_key_t pkey_stack;
pthread_key_t pkey_ticker;

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

  void WriteTag(Profile *profile, FILE* stream);

private:
  const char* mTagData;
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

  void WriteProfile(FILE* stream)
  {
    LOG("Save profile");
    
    
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
  explicit TableTicker(int aEntrySize, int aInterval)
    : Sampler(aInterval, true)
    , mProfile(aEntrySize)
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
    return &mStack;
  }

  Profile* GetProfile()
  {
    return &mProfile;
  }
 private:
  Profile mProfile;
  Stack mStack;
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
      LOG("Saved to " FOLDER "profile_TYPE_ID.txt");
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
  const char *marker = mStack.getMarker(i++);
  for (int i = 0; marker != NULL; i++) {
    mProfile.addTag(ProfileEntry('m', marker));
    marker = mStack.getMarker(i++);
  }
  mStack.mQueueClearMarker = true;

  
  
  
  for (int i = 0; i < mStack.mStackPointer; i++) {
    if (i == 0) {
      Address pc = 0;
      if (sample) {
        pc = sample->pc;
      }
      mProfile.addTag(ProfileEntry('s', mStack.mStack[i], pc));
    } else {
      mProfile.addTag(ProfileEntry('c', mStack.mStack[i]));
    }
  }
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
void mozilla_sampler_init()
{
  const char *val = PR_GetEnv("MOZ_PROFILER_SPS");
  if (!val || !*val) {
    return;
  }

  
  
  
  
  
  if (pthread_key_create(&pkey_stack, NULL) ||
        pthread_key_create(&pkey_ticker, NULL)) {
    LOG("Failed to init.");
    return;
  }

  TableTicker *t = new TableTicker(PROFILE_DEFAULT_ENTRY, 10);
  pthread_setspecific(pkey_ticker, t);
  pthread_setspecific(pkey_stack, t->GetStack());

  t->Start();
}

void mozilla_sampler_deinit()
{
  TableTicker *t = (TableTicker*)pthread_getspecific(pkey_ticker);
  if (!t) {
    return;
  }

  t->Stop();
  pthread_setspecific(pkey_stack, NULL);
  
  
  
}

