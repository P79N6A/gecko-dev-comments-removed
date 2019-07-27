





#include "DataStorage.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIObserverService.h"
#include "nsITimer.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "prprf.h"




static const uint32_t sDataStorageDefaultTimerDelay = 5u * 60u * 1000u;

static const uint32_t sMaxScore = UINT32_MAX;

static const uint32_t sMaxDataEntries = 1024;
static const int64_t sOneDayInMicroseconds = int64_t(24 * 60 * 60) *
                                             PR_USEC_PER_SEC;

namespace mozilla {

NS_IMPL_ISUPPORTS(DataStorage,
                  nsIObserver)

DataStorage::DataStorage(const nsString& aFilename)
  : mMutex("DataStorage::mMutex")
  , mPendingWrite(false)
  , mShuttingDown(false)
  , mReadyMonitor("DataStorage::mReadyMonitor")
  , mReady(false)
  , mFilename(aFilename)
{
}

DataStorage::~DataStorage()
{
}

nsresult
DataStorage::Init(bool& aDataWillPersist)
{
  
  if (!NS_IsMainThread()) {
    NS_NOTREACHED("DataStorage::Init called off main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  MutexAutoLock lock(mMutex);

  nsresult rv;
  rv = NS_NewThread(getter_AddRefs(mWorkerThread));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = AsyncReadData(aDataWillPersist, lock);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (NS_WARN_IF(!os)) {
    return NS_ERROR_FAILURE;
  }
  
  os->AddObserver(this, "last-pb-context-exited", false);
  
  os->AddObserver(this, "profile-before-change", false);

  
  mTimerDelay = Preferences::GetInt("test.datastorage.write_timer_ms",
                                    sDataStorageDefaultTimerDelay);
  rv = Preferences::AddStrongObserver(this, "test.datastorage.write_timer_ms");
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

class DataStorage::Reader : public nsRunnable
{
public:
  explicit Reader(DataStorage* aDataStorage)
    : mDataStorage(aDataStorage)
  {
  }
  ~Reader();

private:
  NS_DECL_NSIRUNNABLE

  static nsresult ParseLine(nsDependentCSubstring& aLine, nsCString& aKeyOut,
                            Entry& aEntryOut);

  nsRefPtr<DataStorage> mDataStorage;
};

DataStorage::Reader::~Reader()
{
  
  {
    MonitorAutoLock readyLock(mDataStorage->mReadyMonitor);
    mDataStorage->mReady = true;
    nsresult rv = mDataStorage->mReadyMonitor.NotifyAll();
    unused << NS_WARN_IF(NS_FAILED(rv));
  }

  
  nsCOMPtr<nsIRunnable> job =
    NS_NewRunnableMethodWithArg<const char*>(mDataStorage,
                                             &DataStorage::NotifyObservers,
                                             "data-storage-ready");
  nsresult rv = NS_DispatchToMainThread(job, NS_DISPATCH_NORMAL);
  unused << NS_WARN_IF(NS_FAILED(rv));
}

NS_IMETHODIMP
DataStorage::Reader::Run()
{
  nsresult rv;
  
  
  
  nsCOMPtr<nsIFile> file;
  {
    MutexAutoLock lock(mDataStorage->mMutex);
    
    if (!mDataStorage->mBackingFile) {
      return NS_OK;
    }
    rv = mDataStorage->mBackingFile->Clone(getter_AddRefs(file));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }
  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), file);
  
  if (NS_WARN_IF(NS_FAILED(rv) &&
                 rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST &&  
                 rv != NS_ERROR_FILE_NOT_FOUND)) {             
    return rv;
  }

  
  
  nsCString data;
  if (fileInputStream) {
    
    rv = NS_ConsumeStream(fileInputStream, 1u << 21, data);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  
  
  {
    MutexAutoLock lock(mDataStorage->mMutex);
    
    
    
    
    int32_t currentIndex = 0;
    int32_t newlineIndex = 0;
    do {
      newlineIndex = data.FindChar('\n', currentIndex);
      
      
      if (newlineIndex < 0 ||
          mDataStorage->mPersistentDataTable.Count() >= sMaxDataEntries) {
        break;
      }

      nsDependentCSubstring line(data, currentIndex,
                                 newlineIndex - currentIndex);
      currentIndex = newlineIndex + 1;
      nsCString key;
      Entry entry;
      nsresult parseRV = ParseLine(line, key, entry);
      if (NS_SUCCEEDED(parseRV)) {
        
        
        Entry newerEntry;
        bool present = mDataStorage->mPersistentDataTable.Get(key, &newerEntry);
        if (!present) {
          mDataStorage->mPersistentDataTable.Put(key, entry);
        }
      }
    } while (true);
  }

  return NS_OK;
}






nsresult
DataStorage::ValidateKeyAndValue(const nsCString& aKey, const nsCString& aValue)
{
  if (aKey.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }
  if (aKey.Length() > 256) {
    return NS_ERROR_INVALID_ARG;
  }
  int32_t delimiterIndex = aKey.FindChar('\t', 0);
  if (delimiterIndex >= 0) {
    return NS_ERROR_INVALID_ARG;
  }
  delimiterIndex = aKey.FindChar('\n', 0);
  if (delimiterIndex >= 0) {
    return NS_ERROR_INVALID_ARG;
  }
  delimiterIndex = aValue.FindChar('\n', 0);
  if (delimiterIndex >= 0) {
    return NS_ERROR_INVALID_ARG;
  }
  if (aValue.Length() > 1024) {
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}









nsresult
DataStorage::Reader::ParseLine(nsDependentCSubstring& aLine, nsCString& aKeyOut,
                               Entry& aEntryOut)
{
  
  int32_t scoreIndex;
  scoreIndex = aLine.FindChar('\t', 0) + 1;
  if (scoreIndex <= 0) {
    return NS_ERROR_UNEXPECTED;
  }
  int32_t accessedIndex = aLine.FindChar('\t', scoreIndex) + 1;
  if (accessedIndex <= 0) {
    return NS_ERROR_UNEXPECTED;
  }
  int32_t valueIndex = aLine.FindChar('\t', accessedIndex) + 1;
  if (valueIndex <= 0) {
    return NS_ERROR_UNEXPECTED;
  }

  
  nsDependentCSubstring keyPart(aLine, 0, scoreIndex - 1);
  nsDependentCSubstring scorePart(aLine, scoreIndex,
                                  accessedIndex - scoreIndex - 1);
  nsDependentCSubstring accessedPart(aLine, accessedIndex,
                                     valueIndex - accessedIndex - 1);
  nsDependentCSubstring valuePart(aLine, valueIndex);

  nsresult rv;
  rv = DataStorage::ValidateKeyAndValue(nsCString(keyPart),
                                        nsCString(valuePart));
  if (NS_FAILED(rv)) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  int32_t integer = nsCString(scorePart).ToInteger(&rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  if (integer < 0) {
    return NS_ERROR_UNEXPECTED;
  }
  aEntryOut.mScore = (uint32_t)integer;

  integer = nsCString(accessedPart).ToInteger(&rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (integer < 0) {
    return NS_ERROR_UNEXPECTED;
  }
  aEntryOut.mLastAccessed = integer;

  
  aKeyOut.Assign(keyPart);
  aEntryOut.mValue.Assign(valuePart);

  return NS_OK;
}

nsresult
DataStorage::AsyncReadData(bool& aHaveProfileDir,
                           const MutexAutoLock& )
{
  aHaveProfileDir = false;
  
  
  
  nsRefPtr<Reader> job(new Reader(this));
  nsresult rv;
  
  
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(mBackingFile));
  if (NS_FAILED(rv)) {
    mBackingFile = nullptr;
    return NS_OK;
  }

  rv = mBackingFile->Append(mFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mWorkerThread->Dispatch(job, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  aHaveProfileDir = true;
  return NS_OK;
}

void
DataStorage::WaitForReady()
{
  MonitorAutoLock readyLock(mReadyMonitor);
  while (!mReady) {
    nsresult rv = readyLock.Wait();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      break;
    }
  }
  MOZ_ASSERT(mReady);
}

nsCString
DataStorage::Get(const nsCString& aKey, DataStorageType aType)
{
  WaitForReady();
  MutexAutoLock lock(mMutex);

  Entry entry;
  bool foundValue = GetInternal(aKey, &entry, aType, lock);
  if (!foundValue) {
    return EmptyCString();
  }

  
  if (entry.UpdateScore()) {
    PutInternal(aKey, entry, aType, lock);
  }

  return entry.mValue;
}

bool
DataStorage::GetInternal(const nsCString& aKey, Entry* aEntry,
                         DataStorageType aType,
                         const MutexAutoLock& aProofOfLock)
{
  DataStorageTable& table = GetTableForType(aType, aProofOfLock);
  bool foundValue = table.Get(aKey, aEntry);
  return foundValue;
}

DataStorage::DataStorageTable&
DataStorage::GetTableForType(DataStorageType aType,
                             const MutexAutoLock& )
{
  switch (aType) {
    case DataStorage_Persistent:
      return mPersistentDataTable;
    case DataStorage_Temporary:
      return mTemporaryDataTable;
    case DataStorage_Private:
      return mPrivateDataTable;
  }

  MOZ_CRASH("given bad DataStorage storage type");
}



PLDHashOperator
DataStorage::EvictCallback(const nsACString& aKey, Entry aEntry, void* aArg)
{
  KeyAndEntry* toEvict = (KeyAndEntry*)aArg;
  if (aEntry.mScore < toEvict->mEntry.mScore) {
    toEvict->mKey = aKey;
    toEvict->mEntry = aEntry;
  }
  return PLDHashOperator::PL_DHASH_NEXT;
}








void
DataStorage::MaybeEvictOneEntry(DataStorageType aType,
                                const MutexAutoLock& aProofOfLock)
{
  DataStorageTable& table = GetTableForType(aType, aProofOfLock);
  if (table.Count() >= sMaxDataEntries) {
    KeyAndEntry toEvict;
    
    
    
    
    
    
    
    
    
    toEvict.mEntry.mScore = sMaxScore;
    table.EnumerateRead(EvictCallback, (void*)&toEvict);
    table.Remove(toEvict.mKey);
  }
}

nsresult
DataStorage::Put(const nsCString& aKey, const nsCString& aValue,
                 DataStorageType aType)
{
  WaitForReady();
  MutexAutoLock lock(mMutex);

  nsresult rv;
  rv = ValidateKeyAndValue(aKey, aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  Entry entry;
  bool exists = GetInternal(aKey, &entry, aType, lock);
  if (exists) {
    entry.UpdateScore();
  } else {
    MaybeEvictOneEntry(aType, lock);
  }
  entry.mValue = aValue;
  rv = PutInternal(aKey, entry, aType, lock);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

nsresult
DataStorage::PutInternal(const nsCString& aKey, Entry& aEntry,
                         DataStorageType aType,
                         const MutexAutoLock& aProofOfLock)
{
  DataStorageTable& table = GetTableForType(aType, aProofOfLock);
  table.Put(aKey, aEntry);

  if (aType == DataStorage_Persistent && !mPendingWrite) {
    return AsyncSetTimer(aProofOfLock);
  }

  return NS_OK;
}

void
DataStorage::Remove(const nsCString& aKey, DataStorageType aType)
{
  WaitForReady();
  MutexAutoLock lock(mMutex);

  DataStorageTable& table = GetTableForType(aType, lock);
  table.Remove(aKey);

  if (aType == DataStorage_Persistent && !mPendingWrite) {
    unused << AsyncSetTimer(lock);
  }
}

class DataStorage::Writer : public nsRunnable
{
public:
  Writer(nsCString& aData, DataStorage* aDataStorage)
    : mData(aData)
    , mDataStorage(aDataStorage)
  {
  }

private:
  NS_DECL_NSIRUNNABLE

  nsCString mData;
  nsRefPtr<DataStorage> mDataStorage;
};

NS_IMETHODIMP
DataStorage::Writer::Run()
{
  nsresult rv;
  
  
  
  nsCOMPtr<nsIFile> file;
  {
    MutexAutoLock lock(mDataStorage->mMutex);
    
    if (!mDataStorage->mBackingFile) {
      return NS_OK;
    }
    rv = mDataStorage->mBackingFile->Clone(getter_AddRefs(file));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), file,
                                   PR_CREATE_FILE | PR_TRUNCATE | PR_WRONLY);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  const char* ptr = mData.get();
  int32_t remaining = mData.Length();
  uint32_t written = 0;
  while (remaining > 0) {
    rv = outputStream->Write(ptr, remaining, &written);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
    remaining -= written;
    ptr += written;
  }

  
  nsCOMPtr<nsIRunnable> job =
    NS_NewRunnableMethodWithArg<const char*>(mDataStorage,
                                             &DataStorage::NotifyObservers,
                                             "data-storage-written");
  rv = NS_DispatchToMainThread(job, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}



PLDHashOperator
DataStorage::WriteDataCallback(const nsACString& aKey, Entry aEntry, void* aArg)
{
  nsCString* output = (nsCString*)aArg;
  output->Append(aKey);
  output->Append('\t');
  output->AppendInt(aEntry.mScore);
  output->Append('\t');
  output->AppendInt(aEntry.mLastAccessed);
  output->Append('\t');
  output->Append(aEntry.mValue);
  output->Append('\n');
  return PLDHashOperator::PL_DHASH_NEXT;
}

nsresult
DataStorage::AsyncWriteData(const MutexAutoLock& )
{
  if (mShuttingDown || !mBackingFile) {
    return NS_OK;
  }

  nsCString output;
  mPersistentDataTable.EnumerateRead(WriteDataCallback, (void*)&output);

  nsRefPtr<Writer> job(new Writer(output, this));
  nsresult rv = mWorkerThread->Dispatch(job, NS_DISPATCH_NORMAL);
  mPendingWrite = false;
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

nsresult
DataStorage::Clear()
{
  WaitForReady();
  MutexAutoLock lock(mMutex);
  mPersistentDataTable.Clear();
  mTemporaryDataTable.Clear();
  mPrivateDataTable.Clear();

  
  
  
  nsresult rv = AsyncWriteData(lock);
  if (NS_FAILED(rv)) {
    return rv;
  }
  return NS_OK;
}


void
DataStorage::TimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsRefPtr<DataStorage> aDataStorage = (DataStorage*)aClosure;
  MutexAutoLock lock(aDataStorage->mMutex);
  unused << aDataStorage->AsyncWriteData(lock);
}



nsresult
DataStorage::AsyncSetTimer(const MutexAutoLock& )
{
  if (mShuttingDown) {
    return NS_OK;
  }

  mPendingWrite = true;
  nsCOMPtr<nsIRunnable> job =
    NS_NewRunnableMethod(this, &DataStorage::SetTimer);
  nsresult rv = mWorkerThread->Dispatch(job, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return NS_OK;
}

void
DataStorage::SetTimer()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MutexAutoLock lock(mMutex);

  nsresult rv;
  if (!mTimer) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }
  }

  rv = mTimer->InitWithFuncCallback(TimerCallback, this,
                                    mTimerDelay, nsITimer::TYPE_ONE_SHOT);
  unused << NS_WARN_IF(NS_FAILED(rv));
}

void
DataStorage::NotifyObservers(const char* aTopic)
{
  
  if (!NS_IsMainThread()) {
    NS_NOTREACHED("DataStorage::NotifyObservers called off main thread");
    return;
  }

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->NotifyObservers(nullptr, aTopic, mFilename.get());
  }
}

nsresult
DataStorage::DispatchShutdownTimer(const MutexAutoLock& )
{
  nsCOMPtr<nsIRunnable> job =
    NS_NewRunnableMethod(this, &DataStorage::ShutdownTimer);
  nsresult rv = mWorkerThread->Dispatch(job, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  return NS_OK;
}

void
DataStorage::ShutdownTimer()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MutexAutoLock lock(mMutex);
  nsresult rv = mTimer->Cancel();
  unused << NS_WARN_IF(NS_FAILED(rv));
  mTimer = nullptr;
}





NS_IMETHODIMP
DataStorage::Observe(nsISupports* aSubject, const char* aTopic,
                     const char16_t* aData)
{
  
  if (!NS_IsMainThread()) {
    NS_NOTREACHED("DataStorage::Observe called off main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  nsresult rv;
  if (strcmp(aTopic, "last-pb-context-exited") == 0) {
    MutexAutoLock lock(mMutex);
    mPrivateDataTable.Clear();
  } else if (strcmp(aTopic, "profile-before-change") == 0) {
    {
      MutexAutoLock lock(mMutex);
      rv = AsyncWriteData(lock);
      mShuttingDown = true;
      unused << NS_WARN_IF(NS_FAILED(rv));
      if (mTimer) {
        rv = DispatchShutdownTimer(lock);
        unused << NS_WARN_IF(NS_FAILED(rv));
      }
    }
    
    
    
    rv = mWorkerThread->Shutdown();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  } else if (strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    MutexAutoLock lock(mMutex);
    mTimerDelay = Preferences::GetInt("test.datastorage.write_timer_ms",
                                      sDataStorageDefaultTimerDelay);
  }

  return NS_OK;
}

DataStorage::Entry::Entry()
  : mScore(0)
  , mLastAccessed((int32_t)(PR_Now() / sOneDayInMicroseconds))
{
}








bool
DataStorage::Entry::UpdateScore()
{

  int32_t nowInDays = (int32_t)(PR_Now() / sOneDayInMicroseconds);
  int32_t daysSinceAccessed = (nowInDays - mLastAccessed);

  
  mLastAccessed = nowInDays;

  
  
  if (daysSinceAccessed < 1) {
    return false;
  }

  
  if (mScore < sMaxScore) {
    mScore++;
  }
  return true;
}

} 
