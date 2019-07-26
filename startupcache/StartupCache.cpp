





#include "prio.h"
#include "prtypes.h"
#include "pldhash.h"
#include "nsXPCOMStrings.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/scache/StartupCache.h"

#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsComponentManagerUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIClassInfo.h"
#include "nsIFile.h"
#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIOutputStream.h"
#include "nsIStartupCache.h"
#include "nsIStorageStream.h"
#include "nsIStreamBufferAccess.h"
#include "nsIStringStream.h"
#include "nsISupports.h"
#include "nsITimer.h"
#include "nsIZipWriter.h"
#include "nsIZipReader.h"
#include "nsWeakReference.h"
#include "nsZipArchive.h"
#include "mozilla/Omnijar.h"
#include "prenv.h"
#include "mozilla/Telemetry.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "nsIProtocolHandler.h"

#ifdef IS_BIG_ENDIAN
#define SC_ENDIAN "big"
#else
#define SC_ENDIAN "little"
#endif

#if PR_BYTES_PER_WORD == 4
#define SC_WORDSIZE "4"
#else
#define SC_WORDSIZE "8"
#endif

namespace mozilla {
namespace scache {

class StartupCacheMappingReporter MOZ_FINAL : public MemoryUniReporter
{
public:
  StartupCacheMappingReporter()
    : MemoryUniReporter("explicit/startup-cache/mapping",
                         KIND_NONHEAP, UNITS_BYTES,
"Memory used to hold the mapping of the startup cache from file.  This memory "
"is likely to be swapped out shortly after start-up.")
  {}
private:
  int64_t Amount() MOZ_OVERRIDE
  {
    mozilla::scache::StartupCache* sc =
      mozilla::scache::StartupCache::GetSingleton();
    return sc ? sc->SizeOfMapping() : 0;
  }
};

class StartupCacheDataReporter MOZ_FINAL : public MemoryUniReporter
{
public:
  StartupCacheDataReporter()
    : MemoryUniReporter("explicit/startup-cache/data", KIND_HEAP, UNITS_BYTES,
"Memory used by the startup cache for things other than the file mapping.")
  {}
private:
  int64_t Amount() MOZ_OVERRIDE
  {
    mozilla::scache::StartupCache* sc =
      mozilla::scache::StartupCache::GetSingleton();
    return sc ? sc->HeapSizeOfIncludingThis(MallocSizeOf) : 0;
  }
};

static const char sStartupCacheName[] = "startupCache." SC_WORDSIZE "." SC_ENDIAN;
static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);

StartupCache*
StartupCache::GetSingleton()
{
  if (!gStartupCache) {
    if (XRE_GetProcessType() != GeckoProcessType_Default) {
      return nullptr;
    }

    StartupCache::InitSingleton();
  }

  return StartupCache::gStartupCache;
}

void
StartupCache::DeleteSingleton()
{
  delete StartupCache::gStartupCache;
  StartupCache::gStartupCache = nullptr;
}

nsresult
StartupCache::InitSingleton()
{
  nsresult rv;
  StartupCache::gStartupCache = new StartupCache();

  rv = StartupCache::gStartupCache->Init();
  if (NS_FAILED(rv)) {
    delete StartupCache::gStartupCache;
    StartupCache::gStartupCache = nullptr;
  }
  return rv;
}

StartupCache* StartupCache::gStartupCache;
bool StartupCache::gShutdownInitiated;
bool StartupCache::gIgnoreDiskCache;
enum StartupCache::TelemetrifyAge StartupCache::gPostFlushAgeAction = StartupCache::IGNORE_AGE;

StartupCache::StartupCache()
  : mArchive(nullptr), mStartupWriteInitiated(false), mWriteThread(nullptr)
{ }

StartupCache::~StartupCache()
{
  if (mTimer) {
    mTimer->Cancel();
  }

  
  
  
  WaitOnWriteThread();

  
  
  
  
  if (!mArchive) {
    WriteToDisk();
  }

  gStartupCache = nullptr;
  NS_UnregisterMemoryReporter(mMappingReporter);
  NS_UnregisterMemoryReporter(mDataReporter);
}

nsresult
StartupCache::Init()
{
  
  nsCOMPtr<nsIProtocolHandler> jarInitializer(do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "jar"));

  nsresult rv;

  
  
  char *env = PR_GetEnv("MOZ_STARTUP_CACHE");
  if (env) {
    rv = NS_NewLocalFile(NS_ConvertUTF8toUTF16(env), false, getter_AddRefs(mFile));
  } else {
    nsCOMPtr<nsIFile> file;
    rv = NS_GetSpecialDirectory("ProfLDS",
                                getter_AddRefs(file));
    if (NS_FAILED(rv)) {
      
      return rv;
    }

    nsCOMPtr<nsIFile> profDir;
    NS_GetSpecialDirectory("ProfDS", getter_AddRefs(profDir));
    if (profDir) {
      bool same;
      if (NS_SUCCEEDED(profDir->Equals(file, &same)) && !same) {
        
        
        if (NS_SUCCEEDED(
              profDir->AppendNative(NS_LITERAL_CSTRING("startupCache")))) {
          profDir->Remove(true);
        }
      }
    }

    rv = file->AppendNative(NS_LITERAL_CSTRING("startupCache"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = file->Create(nsIFile::DIRECTORY_TYPE, 0777);
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_ALREADY_EXISTS)
      return rv;

    rv = file->AppendNative(NS_LITERAL_CSTRING(sStartupCacheName));

    NS_ENSURE_SUCCESS(rv, rv);

    mFile = do_QueryInterface(file);
  }

  NS_ENSURE_TRUE(mFile, NS_ERROR_UNEXPECTED);

  mObserverService = do_GetService("@mozilla.org/observer-service;1");

  if (!mObserverService) {
    NS_WARNING("Could not get observerService.");
    return NS_ERROR_UNEXPECTED;
  }

  mListener = new StartupCacheListener();
  rv = mObserverService->AddObserver(mListener, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                     false);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mObserverService->AddObserver(mListener, "startupcache-invalidate",
                                     false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadArchive(RECORD_AGE);

  
  
  if (gIgnoreDiskCache || (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)) {
    NS_WARNING("Failed to load startupcache file correctly, removing!");
    InvalidateCache();
  }

  mMappingReporter = new StartupCacheMappingReporter();
  mDataReporter    = new StartupCacheDataReporter();
  NS_RegisterMemoryReporter(mMappingReporter);
  NS_RegisterMemoryReporter(mDataReporter);

  return NS_OK;
}




nsresult
StartupCache::LoadArchive(enum TelemetrifyAge flag)
{
  if (gIgnoreDiskCache)
    return NS_ERROR_FAILURE;

  bool exists;
  mArchive = nullptr;
  nsresult rv = mFile->Exists(&exists);
  if (NS_FAILED(rv) || !exists)
    return NS_ERROR_FILE_NOT_FOUND;
  
  mArchive = new nsZipArchive();
  rv = mArchive->OpenArchive(mFile);
  if (NS_FAILED(rv) || flag == IGNORE_AGE)
    return rv;

  nsCString comment;
  if (!mArchive->GetComment(comment)) {
    return rv;
  }

  const char *data;
  size_t len = NS_CStringGetData(comment, &data);
  PRTime creationStamp;
  
  
  if (len == sizeof(creationStamp)) {
    memcpy(&creationStamp, data, len);
    PRTime current = PR_Now();
    int64_t diff = current - creationStamp;

    
    
    int64_t usec_per_hour = PR_USEC_PER_SEC * int64_t(3600);
    int64_t hour_diff = (diff + usec_per_hour - 1) / usec_per_hour;
    mozilla::Telemetry::Accumulate(Telemetry::STARTUP_CACHE_AGE_HOURS,
                                   hour_diff);
  }

  return rv;
}

namespace {

nsresult
GetBufferFromZipArchive(nsZipArchive *zip, bool doCRC, const char* id,
                        char** outbuf, uint32_t* length)
{
  if (!zip)
    return NS_ERROR_NOT_AVAILABLE;

  nsZipItemPtr<char> zipItem(zip, id, doCRC);
  if (!zipItem)
    return NS_ERROR_NOT_AVAILABLE;

  *outbuf = zipItem.Forget();
  *length = zipItem.Length();
  return NS_OK;
}

} 



nsresult
StartupCache::GetBuffer(const char* id, char** outbuf, uint32_t* length) 
{
  NS_ASSERTION(NS_IsMainThread(), "Startup cache only available on main thread");
  WaitOnWriteThread();
  if (!mStartupWriteInitiated) {
    CacheEntry* entry; 
    nsDependentCString idStr(id);
    mTable.Get(idStr, &entry);
    if (entry) {
      *outbuf = new char[entry->size];
      memcpy(*outbuf, entry->data, entry->size);
      *length = entry->size;
      return NS_OK;
    }
  }

  nsresult rv = GetBufferFromZipArchive(mArchive, true, id, outbuf, length);
  if (NS_SUCCEEDED(rv))
    return rv;

  nsRefPtr<nsZipArchive> omnijar = mozilla::Omnijar::GetReader(mozilla::Omnijar::APP);
  
  rv = GetBufferFromZipArchive(omnijar, false, id, outbuf, length);
  if (NS_SUCCEEDED(rv))
    return rv;

  omnijar = mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
  
  return GetBufferFromZipArchive(omnijar, false, id, outbuf, length);
}


nsresult
StartupCache::PutBuffer(const char* id, const char* inbuf, uint32_t len) 
{
  NS_ASSERTION(NS_IsMainThread(), "Startup cache only available on main thread");
  WaitOnWriteThread();
  if (StartupCache::gShutdownInitiated) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoArrayPtr<char> data(new char[len]);
  memcpy(data, inbuf, len);

  nsDependentCString idStr(id);
  
  CacheEntry* entry; 
  
#ifdef DEBUG
  mTable.Get(idStr, &entry);
  NS_ASSERTION(entry == nullptr, "Existing entry in StartupCache.");
  
  if (mArchive) {
    nsZipItem* zipItem = mArchive->GetItem(id);
    NS_ASSERTION(zipItem == nullptr, "Existing entry in disk StartupCache.");
  }
#endif
  
  entry = new CacheEntry(data.forget(), len);
  mTable.Put(idStr, entry);
  return ResetStartupWriteTimer();
}

size_t
StartupCache::SizeOfMapping() 
{
    return mArchive ? mArchive->SizeOfMapping() : 0;
}

size_t
StartupCache::HeapSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
{
    
    
    return aMallocSizeOf(this) +
           mTable.SizeOfExcludingThis(SizeOfEntryExcludingThis, aMallocSizeOf);
}

 size_t
StartupCache::SizeOfEntryExcludingThis(const nsACString& key, const nsAutoPtr<CacheEntry>& data,
                                       mozilla::MallocSizeOf mallocSizeOf, void *)
{
    return data->SizeOfExcludingThis(mallocSizeOf);
}

struct CacheWriteHolder
{
  nsCOMPtr<nsIZipWriter> writer;
  nsCOMPtr<nsIStringInputStream> stream;
  PRTime time;
};

PLDHashOperator
CacheCloseHelper(const nsACString& key, nsAutoPtr<CacheEntry>& data, 
                 void* closure) 
{
  nsresult rv;
 
  CacheWriteHolder* holder = (CacheWriteHolder*) closure;  
  nsIStringInputStream* stream = holder->stream;
  nsIZipWriter* writer = holder->writer;

  stream->ShareData(data->data, data->size);

#ifdef DEBUG
  bool hasEntry;
  rv = writer->HasEntry(key, &hasEntry);
  NS_ASSERTION(NS_SUCCEEDED(rv) && hasEntry == false, 
               "Existing entry in disk StartupCache.");
#endif
  rv = writer->AddEntryStream(key, holder->time, true, stream, false);
  
  if (NS_FAILED(rv)) {
    NS_WARNING("cache entry deleted but not written to disk.");
  }
  return PL_DHASH_REMOVE;
}






void
StartupCache::WriteToDisk() 
{
  nsresult rv;
  mStartupWriteInitiated = true;

  if (mTable.Count() == 0)
    return;

  nsCOMPtr<nsIZipWriter> zipW = do_CreateInstance("@mozilla.org/zipwriter;1");
  if (!zipW)
    return;

  rv = zipW->Open(mFile, PR_RDWR | PR_CREATE_FILE);
  if (NS_FAILED(rv)) {
    NS_WARNING("could not open zipfile for write");
    return;
  } 

  
  
  
  
  PRTime now = PR_Now();
  if (!mArchive) {
    nsCString comment;
    comment.Assign((char *)&now, sizeof(now));
    zipW->SetComment(comment);
  }

  nsCOMPtr<nsIStringInputStream> stream 
    = do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("Couldn't create string input stream.");
    return;
  }

  CacheWriteHolder holder;
  holder.stream = stream;
  holder.writer = zipW;
  holder.time = now;

  mTable.Enumerate(CacheCloseHelper, &holder);

  
  mArchive = nullptr;
  zipW->Close();

  
  gIgnoreDiskCache = false;

  
  LoadArchive(gPostFlushAgeAction);
  
  return;
}

void
StartupCache::InvalidateCache() 
{
  WaitOnWriteThread();
  mTable.Clear();
  mArchive = nullptr;
  nsresult rv = mFile->Remove(false);
  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_TARGET_DOES_NOT_EXIST &&
      rv != NS_ERROR_FILE_NOT_FOUND) {
    gIgnoreDiskCache = true;
    mozilla::Telemetry::Accumulate(Telemetry::STARTUP_CACHE_INVALID, true);
    return;
  }
  gIgnoreDiskCache = false;
  LoadArchive(gPostFlushAgeAction);
}

void
StartupCache::IgnoreDiskCache()
{
  gIgnoreDiskCache = true;
  if (gStartupCache)
    gStartupCache->InvalidateCache();
}






void
StartupCache::WaitOnWriteThread()
{
  NS_ASSERTION(NS_IsMainThread(), "Startup cache should only wait for io thread on main thread");
  if (!mWriteThread || mWriteThread == PR_GetCurrentThread())
    return;

  PR_JoinThread(mWriteThread);
  mWriteThread = nullptr;
}

void 
StartupCache::ThreadedWrite(void *aClosure)
{
  PR_SetCurrentThreadName("StartupCache");
  gStartupCache->WriteToDisk();
}






void
StartupCache::WriteTimeout(nsITimer *aTimer, void *aClosure)
{
  gStartupCache->mWriteThread = PR_CreateThread(PR_USER_THREAD,
                                                StartupCache::ThreadedWrite,
                                                nullptr,
                                                PR_PRIORITY_NORMAL,
                                                PR_LOCAL_THREAD,
                                                PR_JOINABLE_THREAD,
                                                0);
}



NS_IMPL_ISUPPORTS1(StartupCacheListener, nsIObserver)

nsresult
StartupCacheListener::Observe(nsISupports *subject, const char* topic, const PRUnichar* data)
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc)
    return NS_OK;

  if (strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    
    sc->WaitOnWriteThread();
    StartupCache::gShutdownInitiated = true;
  } else if (strcmp(topic, "startupcache-invalidate") == 0) {
    sc->InvalidateCache();
  }
  return NS_OK;
} 

nsresult
StartupCache::GetDebugObjectOutputStream(nsIObjectOutputStream* aStream,
                                         nsIObjectOutputStream** aOutStream) 
{
  NS_ENSURE_ARG_POINTER(aStream);
#ifdef DEBUG
  StartupCacheDebugOutputStream* stream
    = new StartupCacheDebugOutputStream(aStream, &mWriteObjectMap);
  NS_ADDREF(*aOutStream = stream);
#else
  NS_ADDREF(*aOutStream = aStream);
#endif
  
  return NS_OK;
}

nsresult
StartupCache::ResetStartupWriteTimer()
{
  mStartupWriteInitiated = false;
  nsresult rv;
  if (!mTimer)
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  else
    rv = mTimer->Cancel();
  NS_ENSURE_SUCCESS(rv, rv);
  
  mTimer->InitWithFuncCallback(StartupCache::WriteTimeout, this, 60000,
                               nsITimer::TYPE_ONE_SHOT);
  return NS_OK;
}

nsresult
StartupCache::RecordAgesAlways()
{
  gPostFlushAgeAction = RECORD_AGE;
  return NS_OK;
}


#ifdef DEBUG
NS_IMPL_ISUPPORTS3(StartupCacheDebugOutputStream, nsIObjectOutputStream, 
                   nsIBinaryOutputStream, nsIOutputStream)

bool
StartupCacheDebugOutputStream::CheckReferences(nsISupports* aObject)
{
  nsresult rv;
  
  nsCOMPtr<nsIClassInfo> classInfo = do_QueryInterface(aObject);
  if (!classInfo) {
    NS_ERROR("aObject must implement nsIClassInfo");
    return false;
  }
  
  uint32_t flags;
  rv = classInfo->GetFlags(&flags);
  NS_ENSURE_SUCCESS(rv, false);
  if (flags & nsIClassInfo::SINGLETON)
    return true;
  
  nsISupportsHashKey* key = mObjectMap->GetEntry(aObject);
  if (key) {
    NS_ERROR("non-singleton aObject is referenced multiple times in this" 
                  "serialization, we don't support that.");
    return false;
  }

  mObjectMap->PutEntry(aObject);
  return true;
}


nsresult
StartupCacheDebugOutputStream::WriteObject(nsISupports* aObject, bool aIsStrongRef)
{
  nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));
  
  NS_ASSERTION(rootObject.get() == aObject,
               "bad call to WriteObject -- call WriteCompoundObject!");
  bool check = CheckReferences(aObject);
  NS_ENSURE_TRUE(check, NS_ERROR_FAILURE);
  return mBinaryStream->WriteObject(aObject, aIsStrongRef);
}

nsresult
StartupCacheDebugOutputStream::WriteSingleRefObject(nsISupports* aObject)
{
  nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));
  
  NS_ASSERTION(rootObject.get() == aObject,
               "bad call to WriteSingleRefObject -- call WriteCompoundObject!");
  bool check = CheckReferences(aObject);
  NS_ENSURE_TRUE(check, NS_ERROR_FAILURE);
  return mBinaryStream->WriteSingleRefObject(aObject);
}

nsresult
StartupCacheDebugOutputStream::WriteCompoundObject(nsISupports* aObject,
                                                const nsIID& aIID,
                                                bool aIsStrongRef)
{
  nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));
  
  nsCOMPtr<nsISupports> roundtrip;
  rootObject->QueryInterface(aIID, getter_AddRefs(roundtrip));
  NS_ASSERTION(roundtrip.get() == aObject,
               "bad aggregation or multiple inheritance detected by call to "
               "WriteCompoundObject!");

  bool check = CheckReferences(aObject);
  NS_ENSURE_TRUE(check, NS_ERROR_FAILURE);
  return mBinaryStream->WriteCompoundObject(aObject, aIID, aIsStrongRef);
}

nsresult
StartupCacheDebugOutputStream::WriteID(nsID const& aID) 
{
  return mBinaryStream->WriteID(aID);
}

char*
StartupCacheDebugOutputStream::GetBuffer(uint32_t aLength, uint32_t aAlignMask)
{
  return mBinaryStream->GetBuffer(aLength, aAlignMask);
}

void
StartupCacheDebugOutputStream::PutBuffer(char* aBuffer, uint32_t aLength)
{
  mBinaryStream->PutBuffer(aBuffer, aLength);
}
#endif 

StartupCacheWrapper* StartupCacheWrapper::gStartupCacheWrapper = nullptr;

NS_IMPL_ISUPPORTS1(StartupCacheWrapper, nsIStartupCache)

StartupCacheWrapper* StartupCacheWrapper::GetSingleton() 
{
  if (!gStartupCacheWrapper)
    gStartupCacheWrapper = new StartupCacheWrapper();

  NS_ADDREF(gStartupCacheWrapper);
  return gStartupCacheWrapper;
}

nsresult 
StartupCacheWrapper::GetBuffer(const char* id, char** outbuf, uint32_t* length) 
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  return sc->GetBuffer(id, outbuf, length);
}

nsresult
StartupCacheWrapper::PutBuffer(const char* id, const char* inbuf, uint32_t length) 
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  return sc->PutBuffer(id, inbuf, length);
}

nsresult
StartupCacheWrapper::InvalidateCache() 
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  sc->InvalidateCache();
  return NS_OK;
}

nsresult
StartupCacheWrapper::IgnoreDiskCache()
{
  StartupCache::IgnoreDiskCache();
  return NS_OK;
}

nsresult 
StartupCacheWrapper::GetDebugObjectOutputStream(nsIObjectOutputStream* stream,
                                                nsIObjectOutputStream** outStream) 
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  return sc->GetDebugObjectOutputStream(stream, outStream);
}

nsresult
StartupCacheWrapper::StartupWriteComplete(bool *complete)
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  sc->WaitOnWriteThread();
  *complete = sc->mStartupWriteInitiated && sc->mTable.Count() == 0;
  return NS_OK;
}

nsresult
StartupCacheWrapper::ResetStartupWriteTimer()
{
  StartupCache* sc = StartupCache::GetSingleton();
  return sc ? sc->ResetStartupWriteTimer() : NS_ERROR_NOT_INITIALIZED;
}

nsresult
StartupCacheWrapper::GetObserver(nsIObserver** obv) {
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  NS_ADDREF(*obv = sc->mListener);
  return NS_OK;
}

nsresult
StartupCacheWrapper::RecordAgesAlways() {
  StartupCache *sc = StartupCache::GetSingleton();
  return sc ? sc->RecordAgesAlways() : NS_ERROR_NOT_INITIALIZED;
}

} 
} 
