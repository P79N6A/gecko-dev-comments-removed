






































#include "prio.h"
#include "prtypes.h"
#include "pldhash.h"
#include "mozilla/scache/StartupCache.h"

#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsComponentManagerUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIClassInfo.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
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

static const char sStartupCacheName[] = "startupCache." SC_WORDSIZE "." SC_ENDIAN;
static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);

StartupCache*
StartupCache::GetSingleton() 
{
  if (!gStartupCache)
    StartupCache::InitSingleton();

  return StartupCache::gStartupCache;
}

void
StartupCache::DeleteSingleton()
{
  delete StartupCache::gStartupCache;
}

nsresult
StartupCache::InitSingleton() 
{
  nsresult rv;
  StartupCache::gStartupCache = new StartupCache();

  rv = StartupCache::gStartupCache->Init();
  if (NS_FAILED(rv)) {
    delete StartupCache::gStartupCache;
    StartupCache::gStartupCache = nsnull;
  }
  return rv;
}

StartupCache* StartupCache::gStartupCache;
PRBool StartupCache::gShutdownInitiated;

StartupCache::StartupCache() 
  : mArchive(NULL), mStartupWriteInitiated(PR_FALSE) { }

StartupCache::~StartupCache() 
{
  if (mTimer) {
    mTimer->Cancel();
  }

  
  
  
  
  
  
  WriteToDisk();
  gStartupCache = nsnull;
}

nsresult
StartupCache::Init() 
{
  nsresult rv;
  mTable.Init();
#ifdef DEBUG
  mWriteObjectMap.Init();
#endif

  mZipW = do_CreateInstance("@mozilla.org/zipwriter;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> file;
  rv = NS_GetSpecialDirectory("ProfLDS",
                              getter_AddRefs(file));
  if (NS_FAILED(rv)) {
    
    return rv;
  }

  rv = file->AppendNative(NS_LITERAL_CSTRING("startupCache"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = file->Create(nsIFile::DIRECTORY_TYPE, 0777);
  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_ALREADY_EXISTS)
    return rv;

  rv = file->AppendNative(NS_LITERAL_CSTRING(sStartupCacheName));
  NS_ENSURE_SUCCESS(rv, rv);
  
  mFile = do_QueryInterface(file);
  NS_ENSURE_TRUE(mFile, NS_ERROR_UNEXPECTED);

  mObserverService = do_GetService("@mozilla.org/observer-service;1");
  
  if (!mObserverService) {
    NS_WARNING("Could not get observerService.");
    return NS_ERROR_UNEXPECTED;
  }
  
  mListener = new StartupCacheListener();  
  rv = mObserverService->AddObserver(mListener, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                     PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mObserverService->AddObserver(mListener, "startupcache-invalidate",
                                     PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = LoadArchive();
  
  
  
  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND) {
    NS_WARNING("Failed to load startupcache file correctly, removing!");
    InvalidateCache();
  }

  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mTimer->InitWithFuncCallback(StartupCache::WriteTimeout, this, 600000,
                                    nsITimer::TYPE_ONE_SHOT);

  return rv;
}

nsresult
StartupCache::LoadArchive() 
{
  PRBool exists;
  mArchive = NULL;
  nsresult rv = mFile->Exists(&exists);
  if (NS_FAILED(rv) || !exists)
    return NS_ERROR_FILE_NOT_FOUND;
  
  mArchive = new nsZipArchive();
  return mArchive->OpenArchive(mFile);
}



nsresult
StartupCache::GetBuffer(const char* id, char** outbuf, PRUint32* length) 
{
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

  if (mArchive) {
    nsZipItemPtr<char> zipItem(mArchive, id, true);
    if (zipItem) {
      *outbuf = zipItem.Forget();
      *length = zipItem.Length();
      return NS_OK;
    } 
  }

  return NS_ERROR_NOT_AVAILABLE;
}


nsresult
StartupCache::PutBuffer(const char* id, const char* inbuf, PRUint32 len) 
{
  nsresult rv;

  if (StartupCache::gShutdownInitiated) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoArrayPtr<char> data(new char[len]);
  memcpy(data, inbuf, len);

  nsDependentCString idStr(id);
  if (!mStartupWriteInitiated) {
    
    CacheEntry* entry; 

#ifdef DEBUG
    mTable.Get(idStr, &entry);
    NS_ASSERTION(entry == nsnull, "Existing entry in StartupCache.");

    if (mArchive) {
      nsZipItem* zipItem = mArchive->GetItem(id);
      NS_ASSERTION(zipItem == nsnull, "Existing entry in disk StartupCache.");
    }
#endif

    entry = new CacheEntry(data.forget(), len);
    mTable.Put(idStr, entry);
    return NS_OK;
  }
  
  rv = mZipW->Open(mFile, PR_RDWR | PR_CREATE_FILE);
  NS_ENSURE_SUCCESS(rv, rv);  

  
  
  
  
  
  
#ifdef DEBUG
  PRBool hasEntry;
  rv = mZipW->HasEntry(idStr, &hasEntry);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(hasEntry == PR_FALSE, "Existing entry in disk StartupCache.");
#endif

  nsCOMPtr<nsIStringInputStream> stream
    = do_CreateInstance("@mozilla.org/io/string-input-stream;1",
                        &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->AdoptData(data, len);
  NS_ENSURE_SUCCESS(rv, rv);
  data.forget();
  
  rv = mZipW->AddEntryStream(idStr, 0, 0, stream, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mArchive = NULL;
  rv = mZipW->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  
  return LoadArchive();
}

struct CacheWriteHolder
{
  nsCOMPtr<nsIZipWriter> writer;
  nsCOMPtr<nsIStringInputStream> stream;
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
  PRBool hasEntry;
  rv = writer->HasEntry(key, &hasEntry);
  NS_ASSERTION(NS_SUCCEEDED(rv) && hasEntry == PR_FALSE, 
               "Existing entry in disk StartupCache.");
#endif
  rv = writer->AddEntryStream(key, 0, 0, stream, false);
  
  if (NS_FAILED(rv)) {
    NS_WARNING("cache entry deleted but not written to disk.");
  }
  return PL_DHASH_REMOVE;
}

void
StartupCache::WriteToDisk() 
{
  nsresult rv;
  mStartupWriteInitiated = PR_TRUE;

  if (mTable.Count() == 0)
    return;

  rv = mZipW->Open(mFile, PR_RDWR | PR_CREATE_FILE);
  if (NS_FAILED(rv)) {
    NS_WARNING("could not open zipfile for write");
    return;
  } 

  nsCOMPtr<nsIStringInputStream> stream 
    = do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("Couldn't create string input stream.");
    return;
  }

  CacheWriteHolder holder;
  holder.stream = stream;
  holder.writer = mZipW;

  mTable.Enumerate(CacheCloseHelper, &holder);

  
  mArchive = NULL;
  mZipW->Close();
      
  
  LoadArchive();
  
  return;
}

void
StartupCache::InvalidateCache() 
{
  mTable.Clear();
  mArchive = NULL;

  
  
  mZipW->Close();
  mFile->Remove(false);
  LoadArchive();
}

void
StartupCache::WriteTimeout(nsITimer *aTimer, void *aClosure)
{
  StartupCache* sc = (StartupCache*) aClosure;
  sc->WriteToDisk();
}



NS_IMPL_THREADSAFE_ISUPPORTS1(StartupCacheListener, nsIObserver)

nsresult
StartupCacheListener::Observe(nsISupports *subject, const char* topic, const PRUnichar* data)
{
  nsresult rv = NS_OK;
  if (strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    StartupCache::gShutdownInitiated = PR_TRUE;
  } else if (strcmp(topic, "startupcache-invalidate") == 0) {
    StartupCache* sc = StartupCache::GetSingleton();
    if (sc)
      sc->InvalidateCache();
  }
  return rv;
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


#ifdef DEBUG
NS_IMPL_ISUPPORTS3(StartupCacheDebugOutputStream, nsIObjectOutputStream, 
                   nsIBinaryOutputStream, nsIOutputStream)

PRBool
StartupCacheDebugOutputStream::CheckReferences(nsISupports* aObject)
{
  nsresult rv;
  
  nsCOMPtr<nsIClassInfo> classInfo = do_QueryInterface(aObject);
  if (!classInfo) {
    NS_ERROR("aObject must implement nsIClassInfo");
    return PR_FALSE;
  }
  
  PRUint32 flags;
  rv = classInfo->GetFlags(&flags);
  NS_ENSURE_SUCCESS(rv, rv);
  if (flags & nsIClassInfo::SINGLETON)
    return PR_TRUE;
  
  nsISupportsHashKey* key = mObjectMap->GetEntry(aObject);
  if (key) {
    NS_ERROR("non-singleton aObject is referenced multiple times in this" 
                  "serialization, we don't support that.");
    return PR_FALSE;
  }

  mObjectMap->PutEntry(aObject);
  return PR_TRUE;
}


nsresult
StartupCacheDebugOutputStream::WriteObject(nsISupports* aObject, PRBool aIsStrongRef)
{
  nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));
  
  NS_ASSERTION(rootObject.get() == aObject,
               "bad call to WriteObject -- call WriteCompoundObject!");
  PRBool check = CheckReferences(aObject);
  NS_ENSURE_TRUE(check, NS_ERROR_FAILURE);
  return mBinaryStream->WriteObject(aObject, aIsStrongRef);
}

nsresult
StartupCacheDebugOutputStream::WriteSingleRefObject(nsISupports* aObject)
{
  nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));
  
  NS_ASSERTION(rootObject.get() == aObject,
               "bad call to WriteSingleRefObject -- call WriteCompoundObject!");
  PRBool check = CheckReferences(aObject);
  NS_ENSURE_TRUE(check, NS_ERROR_FAILURE);
  return mBinaryStream->WriteSingleRefObject(aObject);
}

nsresult
StartupCacheDebugOutputStream::WriteCompoundObject(nsISupports* aObject,
                                                const nsIID& aIID,
                                                PRBool aIsStrongRef)
{
  nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));
  
  nsCOMPtr<nsISupports> roundtrip;
  rootObject->QueryInterface(aIID, getter_AddRefs(roundtrip));
  NS_ASSERTION(roundtrip.get() == aObject,
               "bad aggregation or multiple inheritance detected by call to "
               "WriteCompoundObject!");

  PRBool check = CheckReferences(aObject);
  NS_ENSURE_TRUE(check, NS_ERROR_FAILURE);
  return mBinaryStream->WriteCompoundObject(aObject, aIID, aIsStrongRef);
}

nsresult
StartupCacheDebugOutputStream::WriteID(nsID const& aID) 
{
  return mBinaryStream->WriteID(aID);
}

char*
StartupCacheDebugOutputStream::GetBuffer(PRUint32 aLength, PRUint32 aAlignMask)
{
  return mBinaryStream->GetBuffer(aLength, aAlignMask);
}

void
StartupCacheDebugOutputStream::PutBuffer(char* aBuffer, PRUint32 aLength)
{
  mBinaryStream->PutBuffer(aBuffer, aLength);
}
#endif 

StartupCacheWrapper* StartupCacheWrapper::gStartupCacheWrapper = nsnull;

NS_IMPL_THREADSAFE_ISUPPORTS1(StartupCacheWrapper, nsIStartupCache)

StartupCacheWrapper* StartupCacheWrapper::GetSingleton() 
{
  if (!gStartupCacheWrapper)
    gStartupCacheWrapper = new StartupCacheWrapper();

  NS_ADDREF(gStartupCacheWrapper);
  return gStartupCacheWrapper;
}

nsresult 
StartupCacheWrapper::GetBuffer(const char* id, char** outbuf, PRUint32* length) 
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  return sc->GetBuffer(id, outbuf, length);
}

nsresult
StartupCacheWrapper::PutBuffer(const char* id, char* inbuf, PRUint32 length) 
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
StartupCacheWrapper::StartupWriteComplete(PRBool *complete)
{  
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  *complete = sc->mStartupWriteInitiated && sc->mTable.Count() == 0;
  return NS_OK;
}

nsresult
StartupCacheWrapper::ResetStartupWriteTimer()
{
  StartupCache* sc = StartupCache::GetSingleton();
  if (!sc) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  sc->mStartupWriteInitiated = PR_FALSE;
  
  
  sc->mTimer->Cancel();
  sc->mTimer->InitWithFuncCallback(StartupCache::WriteTimeout, sc, 10000,
                                   nsITimer::TYPE_ONE_SHOT);
  return NS_OK;
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

} 
} 
