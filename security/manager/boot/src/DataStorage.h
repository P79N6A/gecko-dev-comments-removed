





#ifndef mozilla_DataStorage_h
#define mozilla_DataStorage_h

#include "mozilla/Monitor.h"
#include "mozilla/Mutex.h"
#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsIObserver.h"
#include "nsIThread.h"
#include "nsITimer.h"
#include "nsString.h"

namespace mozilla {
























































enum DataStorageType {
  DataStorage_Persistent,
  DataStorage_Temporary,
  DataStorage_Private
};

class DataStorage : public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  
  explicit DataStorage(const nsString& aFilename);

  
  
  nsresult Init(bool& aDataWillPersist);
  
  
  
  
  
  nsCString Get(const nsCString& aKey, DataStorageType aType);
  
  
  nsresult Put(const nsCString& aKey, const nsCString& aValue,
               DataStorageType aType);
  
  void Remove(const nsCString& aKey, DataStorageType aType);
  
  nsresult Clear();

private:
  virtual ~DataStorage();

  class Writer;
  class Reader;

  class Entry
  {
  public:
    Entry();
    bool UpdateScore();

    uint32_t mScore;
    int32_t mLastAccessed; 
    nsCString mValue;
  };

  
  class KeyAndEntry
  {
  public:
    nsCString mKey;
    Entry mEntry;
  };

  typedef nsDataHashtable<nsCStringHashKey, Entry> DataStorageTable;

  void WaitForReady();
  nsresult AsyncWriteData(const MutexAutoLock& aProofOfLock);
  nsresult AsyncReadData(bool& aHaveProfileDir,
                         const MutexAutoLock& aProofOfLock);
  nsresult AsyncSetTimer(const MutexAutoLock& aProofOfLock);
  nsresult DispatchShutdownTimer(const MutexAutoLock& aProofOfLock);

  static nsresult ValidateKeyAndValue(const nsCString& aKey,
                                      const nsCString& aValue);
  static void TimerCallback(nsITimer* aTimer, void* aClosure);
  static PLDHashOperator WriteDataCallback(const nsACString& aKey, Entry aEntry,
                                           void* aArg);
  static PLDHashOperator EvictCallback(const nsACString& aKey, Entry aEntry,
                                       void* aArg);
  void SetTimer();
  void ShutdownTimer();
  void NotifyObservers(const char* aTopic);

  bool GetInternal(const nsCString& aKey, Entry* aEntry, DataStorageType aType,
                   const MutexAutoLock& aProofOfLock);
  nsresult PutInternal(const nsCString& aKey, Entry& aEntry,
                       DataStorageType aType,
                       const MutexAutoLock& aProofOfLock);
  void MaybeEvictOneEntry(DataStorageType aType,
                          const MutexAutoLock& aProofOfLock);
  DataStorageTable& GetTableForType(DataStorageType aType,
                                    const MutexAutoLock& aProofOfLock);

  Mutex mMutex; 
  DataStorageTable  mPersistentDataTable;
  DataStorageTable  mTemporaryDataTable;
  DataStorageTable  mPrivateDataTable;
  nsCOMPtr<nsIThread> mWorkerThread;
  nsCOMPtr<nsIFile> mBackingFile;
  nsCOMPtr<nsITimer> mTimer; 
  uint32_t mTimerDelay; 
  bool mPendingWrite; 
  bool mShuttingDown;
  

  Monitor mReadyMonitor; 
  bool mReady; 

  const nsString mFilename;
};

}; 

#endif 
