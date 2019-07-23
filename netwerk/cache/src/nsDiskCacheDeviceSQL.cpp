





































#if defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>
#elif defined(XP_WIN)
#include <windows.h>
#elif defined(XP_OS2)
#define INCL_DOSERRORS
#include <os2.h>
#else

#endif

#include "private/pprio.h"

#include "prlong.h"

#include "nsCache.h"
#include "nsDiskCache.h"
#include "nsDiskCacheDeviceSQL.h"

#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsPrintfCString.h"
#include "nsCRT.h"

#include "mozIStorageService.h"
#include "mozIStorageStatement.h"
#include "mozStorageCID.h"

#include "nsICacheVisitor.h"
#include "nsISeekableStream.h"

static const char DISK_CACHE_DEVICE_ID[] = { "disk" };

#define LOG(args) CACHE_LOG_DEBUG(args)






static nsresult
EnsureDir(nsIFile *dir)
{
  PRBool exists;
  nsresult rv = dir->Exists(&exists);
  if (NS_SUCCEEDED(rv) && !exists)
    rv = dir->Create(nsIFile::DIRECTORY_TYPE, 0700);
  return rv;
}

static PRBool
DecomposeCacheEntryKey(const nsCString *fullKey,
                       const char **cid,
                       const char **key,
                       nsCString &buf)
{
  buf = *fullKey;

  PRInt32 colon = buf.FindChar(':');
  if (colon == kNotFound)
  {
    NS_ERROR("Invalid key");
    return PR_FALSE;
  }
  buf.SetCharAt('\0', colon);

  *cid = buf.get();
  *key = buf.get() + colon + 1;

  return PR_TRUE;
}

class AutoResetStatement
{
  public:
    AutoResetStatement(mozIStorageStatement *s)
      : mStatement(s) {}
    ~AutoResetStatement() { mStatement->Reset(); }
    mozIStorageStatement *operator->() { return mStatement; }
  private:
    mozIStorageStatement *mStatement;
};





#define DCACHE_HASH_MAX  LL_MAXINT
#define DCACHE_HASH_BITS 64









static PRUint64
DCacheHash(const char * key)
{
  PRUint64 h = 0;
  for (const PRUint8* s = (PRUint8*) key; *s != '\0'; ++s)
    h = (h >> (DCACHE_HASH_BITS - 4)) ^ (h << 4) ^ *s;
  return (h == 0 ? DCACHE_HASH_MAX : h);
}


nsresult
nsDiskCache::Truncate(PRFileDesc *  fd, PRUint32  newEOF)
{
  

#if defined(XP_UNIX) || defined(XP_BEOS)
  if (ftruncate(PR_FileDesc2NativeHandle(fd), newEOF) != 0) {
    NS_ERROR("ftruncate failed");
    return NS_ERROR_FAILURE;
  }

#elif defined(XP_WIN)
  PRInt32 cnt = PR_Seek(fd, newEOF, PR_SEEK_SET);
  if (cnt == -1)  return NS_ERROR_FAILURE;
  if (!SetEndOfFile((HANDLE) PR_FileDesc2NativeHandle(fd))) {
    NS_ERROR("SetEndOfFile failed");
    return NS_ERROR_FAILURE;
  }

#elif defined(XP_OS2)
  if (DosSetFileSize((HFILE) PR_FileDesc2NativeHandle(fd), newEOF) != NO_ERROR) {
    NS_ERROR("DosSetFileSize failed");
    return NS_ERROR_FAILURE;
  }
#else
  
#endif
  return NS_OK;
}






class nsDiskCacheDeviceInfo : public nsICacheDeviceInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICACHEDEVICEINFO

  nsDiskCacheDeviceInfo(nsDiskCacheDevice* device)
    : mDevice(device)
  {}
    
private:
  nsDiskCacheDevice* mDevice;
};

NS_IMPL_ISUPPORTS1(nsDiskCacheDeviceInfo, nsICacheDeviceInfo)

NS_IMETHODIMP
nsDiskCacheDeviceInfo::GetDescription(char **aDescription)
{
  *aDescription = nsCRT::strdup("Disk cache device");
  return *aDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsDiskCacheDeviceInfo::GetUsageReport(char ** usageReport)
{
  nsCAutoString buffer;
  buffer.AppendLiteral("\n<tr>\n<td><b>Cache Directory:</b></td>\n<td><tt> ");
  
  nsILocalFile *cacheDir = mDevice->CacheDirectory();
  if (!cacheDir)
    return NS_OK;

  nsAutoString path;
  nsresult rv = cacheDir->GetPath(path);
  if (NS_SUCCEEDED(rv))
    AppendUTF16toUTF8(path, buffer);
  else
    buffer.AppendLiteral("directory unavailable");
  buffer.AppendLiteral("</tt></td>\n</tr>\n");

  *usageReport = ToNewCString(buffer);
  if (!*usageReport)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheDeviceInfo::GetEntryCount(PRUint32 *aEntryCount)
{
  *aEntryCount = mDevice->EntryCount();
  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheDeviceInfo::GetTotalSize(PRUint32 *aTotalSize)
{
  *aTotalSize = mDevice->CacheSize();
  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheDeviceInfo::GetMaximumSize(PRUint32 *aMaximumSize)
{
  *aMaximumSize = mDevice->CacheCapacity();
  return NS_OK;
}






class nsDiskCacheBinding : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  static nsDiskCacheBinding *
      Create(nsIFile *cacheDir, const char *key, int generation);

  nsCOMPtr<nsIFile> mDataFile;
  int               mGeneration;
};

NS_IMPL_THREADSAFE_ISUPPORTS0(nsDiskCacheBinding)

nsDiskCacheBinding *
nsDiskCacheBinding::Create(nsIFile *cacheDir, const char *key, int generation)
{
  nsCOMPtr<nsIFile> file;
  cacheDir->Clone(getter_AddRefs(file));
  if (!file)
    return nsnull;

  PRUint64 hash = DCacheHash(key);

  PRUint32 dir1 = (PRUint32) (hash & 0x0F);
  PRUint32 dir2 = (PRUint32)((hash & 0xF0) >> 4);

  hash >>= 8;

  

  file->AppendNative(nsPrintfCString("%X", dir1));
  file->Create(nsIFile::DIRECTORY_TYPE, 00700);

  file->AppendNative(nsPrintfCString("%X", dir2));
  file->Create(nsIFile::DIRECTORY_TYPE, 00700);

  nsresult rv;
  char leaf[64];

  if (generation == -1)
  {
    file->AppendNative(NS_LITERAL_CSTRING("placeholder"));

    for (generation = 0; ; ++generation)
    {
      PR_snprintf(leaf, sizeof(leaf), "%014llX-%X", hash, generation);

      rv = file->SetNativeLeafName(nsDependentCString(leaf));
      if (NS_FAILED(rv))
        return nsnull;
      rv = file->Create(nsIFile::NORMAL_FILE_TYPE, 00600);
      if (NS_FAILED(rv) && rv != NS_ERROR_FILE_ALREADY_EXISTS)
        return nsnull;
      if (NS_SUCCEEDED(rv))
        break;
    }
  }
  else
  {
    PR_snprintf(leaf, sizeof(leaf), "%014llX-%X", hash, generation);
    rv = file->AppendNative(nsDependentCString(leaf));
    if (NS_FAILED(rv))
      return nsnull;
  }

  nsDiskCacheBinding *binding = new nsDiskCacheBinding;
  if (!binding)
    return nsnull;

  binding->mDataFile.swap(file);
  binding->mGeneration = generation;
  return binding;
}



static nsresult
GetCacheDataFile(nsIFile *cacheDir, const char *cid, const char *key,
                 int generation, nsCOMPtr<nsIFile> &file)
{
  cacheDir->Clone(getter_AddRefs(file));
  if (!file)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCAutoString fullKey;
  fullKey.Append(cid);
  fullKey.Append(':');
  fullKey.Append(key);

  PRUint64 hash = DCacheHash(fullKey.get());

  PRUint32 dir1 = (PRUint32) (hash & 0x0F);
  PRUint32 dir2 = (PRUint32)((hash & 0xF0) >> 4);

  hash >>= 8;

  file->AppendNative(nsPrintfCString("%X", dir1));
  file->AppendNative(nsPrintfCString("%X", dir2));

  char leaf[64];
  PR_snprintf(leaf, sizeof(leaf), "%014llX-%X", hash, generation);
  return file->AppendNative(nsDependentCString(leaf));
}






struct nsDiskCacheRecord
{
  const char    *clientID;
  const char    *key;
  const PRUint8 *metaData;
  PRUint32       metaDataLen;
  PRInt32        generation;
  PRInt32        flags;
  PRInt32        dataSize;
  PRInt32        fetchCount;
  PRInt64        lastFetched;
  PRInt64        lastModified;
  PRInt64        expirationTime;
};

static nsCacheEntry *
CreateCacheEntry(nsDiskCacheDevice *device,
                 const nsCString *fullKey,
                 const nsDiskCacheRecord &rec)
{
  if (rec.flags != 0)
  {
    LOG(("refusing to load busy entry\n"));
    return nsnull;
  }

  nsCacheEntry *entry;
  
  nsresult rv = nsCacheEntry::Create(fullKey->get(), 
                                     nsICache::STREAM_BASED,
                                     nsICache::STORE_ON_DISK,
                                     device, &entry);
  if (NS_FAILED(rv))
    return nsnull;

  entry->SetFetchCount((PRUint32) rec.fetchCount);
  entry->SetLastFetched(SecondsFromPRTime(rec.lastFetched));
  entry->SetLastModified(SecondsFromPRTime(rec.lastModified));
  entry->SetExpirationTime(SecondsFromPRTime(rec.expirationTime));
  entry->SetDataSize((PRUint32) rec.dataSize);

  entry->UnflattenMetaData((const char *) rec.metaData, rec.metaDataLen);

  
  nsDiskCacheBinding *binding =
      nsDiskCacheBinding::Create(device->CacheDirectory(),
                                 fullKey->get(),
                                 rec.generation);
  if (!binding)
  {
    delete entry;
    return nsnull;
  }
  entry->SetData(binding);

  return entry;
}






class nsDiskCacheEntryInfo : public nsICacheEntryInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICACHEENTRYINFO

  nsDiskCacheRecord *mRec;
};

NS_IMPL_ISUPPORTS1(nsDiskCacheEntryInfo, nsICacheEntryInfo)

NS_IMETHODIMP
nsDiskCacheEntryInfo::GetClientID(char **result)
{
  *result = nsCRT::strdup(mRec->clientID);
  return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsDiskCacheEntryInfo::GetDeviceID(char ** deviceID)
{
  *deviceID = nsCRT::strdup(DISK_CACHE_DEVICE_ID);
  return *deviceID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsDiskCacheEntryInfo::GetKey(nsACString &clientKey)
{
  clientKey.Assign(mRec->key);
  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheEntryInfo::GetFetchCount(PRInt32 *aFetchCount)
{
  *aFetchCount = mRec->fetchCount;
  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheEntryInfo::GetLastFetched(PRUint32 *aLastFetched)
{
  *aLastFetched = SecondsFromPRTime(mRec->lastFetched);
  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheEntryInfo::GetLastModified(PRUint32 *aLastModified)
{
  *aLastModified = SecondsFromPRTime(mRec->lastModified);
  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheEntryInfo::GetExpirationTime(PRUint32 *aExpirationTime)
{
  *aExpirationTime = SecondsFromPRTime(mRec->expirationTime);
  return NS_OK;
}

NS_IMETHODIMP
nsDiskCacheEntryInfo::IsStreamBased(PRBool *aStreamBased)
{
  *aStreamBased = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsDiskCacheEntryInfo::GetDataSize(PRUint32 *aDataSize)
{
  *aDataSize = mRec->dataSize;
  return NS_OK;
}






nsDiskCacheDevice::nsDiskCacheDevice()
  : mDB(nsnull)
  , mCacheCapacity(0)
  , mDeltaCounter(0)
{
}

nsDiskCacheDevice::~nsDiskCacheDevice()
{
  Shutdown();
}

PRUint32
nsDiskCacheDevice::CacheSize()
{
  AutoResetStatement statement(mStatement_CacheSize);

  PRBool hasRows;
  nsresult rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && hasRows, 0);
  
  return (PRUint32) statement->AsInt32(0);
}

PRUint32
nsDiskCacheDevice::EntryCount()
{
  AutoResetStatement statement(mStatement_EntryCount);

  PRBool hasRows;
  nsresult rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && hasRows, 0);

  return (PRUint32) statement->AsInt32(0);
}

nsresult
nsDiskCacheDevice::UpdateEntry(nsCacheEntry *entry)
{
  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(entry->Key(), &cid, &key, keyBuf))
    return NS_ERROR_UNEXPECTED;

  nsCString metaDataBuf;
  PRUint32 mdSize = entry->MetaDataSize();
  if (!EnsureStringLength(metaDataBuf, mdSize))
    return NS_ERROR_OUT_OF_MEMORY;
  char *md = metaDataBuf.BeginWriting();
  entry->FlattenMetaData(md, mdSize);

  nsDiskCacheRecord rec;
  rec.metaData = (const PRUint8 *) md;
  rec.metaDataLen = mdSize;
  rec.flags = 0;  
  rec.dataSize = entry->DataSize();
  rec.fetchCount = entry->FetchCount();
  rec.lastFetched = PRTimeFromSeconds(entry->LastFetched());
  rec.lastModified = PRTimeFromSeconds(entry->LastModified());
  rec.expirationTime = PRTimeFromSeconds(entry->ExpirationTime());

  AutoResetStatement statement(mStatement_UpdateEntry);

  nsresult rv;
  rv  = statement->BindDataParameter(0, rec.metaData, rec.metaDataLen);
  rv |= statement->BindInt32Parameter(1, rec.flags);
  rv |= statement->BindInt32Parameter(2, rec.dataSize);
  rv |= statement->BindInt32Parameter(3, rec.fetchCount);
  rv |= statement->BindInt64Parameter(4, rec.lastFetched);
  rv |= statement->BindInt64Parameter(5, rec.lastModified);
  rv |= statement->BindInt64Parameter(6, rec.expirationTime);
  rv |= statement->BindCStringParameter(7, cid);
  rv |= statement->BindCStringParameter(8, key);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!hasRows, "UPDATE should not result in output");
  return rv;
}

nsresult
nsDiskCacheDevice::UpdateEntrySize(nsCacheEntry *entry, PRUint32 newSize)
{
  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(entry->Key(), &cid, &key, keyBuf))
    return NS_ERROR_UNEXPECTED;

  AutoResetStatement statement(mStatement_UpdateEntrySize);

  nsresult rv;
  rv  = statement->BindInt32Parameter(0, newSize);
  rv |= statement->BindCStringParameter(1, cid);
  rv |= statement->BindCStringParameter(2, key);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!hasRows, "UPDATE should not result in output");
  return rv;
}

nsresult
nsDiskCacheDevice::DeleteEntry(nsCacheEntry *entry, PRBool deleteData)
{
  if (deleteData)
  {
    nsresult rv = DeleteData(entry);
    if (NS_FAILED(rv))
      return rv;
  }

  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(entry->Key(), &cid, &key, keyBuf))
    return NS_ERROR_UNEXPECTED;

  AutoResetStatement statement(mStatement_DeleteEntry);

  nsresult rv;
  rv  = statement->BindCStringParameter(0, cid);
  rv |= statement->BindCStringParameter(1, key);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!hasRows, "DELETE should not result in output");
  return rv;
}

nsresult
nsDiskCacheDevice::DeleteData(nsCacheEntry *entry)
{
  nsDiskCacheBinding *binding = (nsDiskCacheBinding *) entry->Data();
  NS_ENSURE_STATE(binding);

  return binding->mDataFile->Remove(PR_FALSE);
}

nsresult
nsDiskCacheDevice::EnableEvictionObserver()
{
#if 0
  
  

  int res =
      sqlite3_exec(mDB, "CREATE TEMP TRIGGER cache_on_delete AFTER DELETE"
                        " ON moz_cache FOR EACH ROW BEGIN SELECT"
                        " cache_eviction_observer("
                        "  OLD.clientID, OLD.key, OLD.generation);"
                        " END;", NULL, NULL, NULL);
  NS_ENSURE_SQLITE_RESULT(res, NS_ERROR_UNEXPECTED);
#endif

  return NS_OK;
}

nsresult
nsDiskCacheDevice::DisableEvictionObserver()
{
#if 0
  int res = sqlite3_exec(mDB, "DROP TRIGGER cache_on_delete;",
                         NULL, NULL, NULL);
  NS_ENSURE_SQLITE_RESULT(res, NS_ERROR_UNEXPECTED);
#endif

  return NS_OK;
}

#if 0
 void
nsDiskCacheDevice::EvictionObserver(sqlite3_context *ctx, int narg,
                                    sqlite3_value **values)
{
  LOG(("nsDiskCacheDevice::EvictionObserver\n"));

  nsDiskCacheDevice *device = (nsDiskCacheDevice *) sqlite3_user_data(ctx);

  NS_ASSERTION(narg == 3, "unexpected number of arguments");
  const char *cid = (const char *) sqlite3_value_text(values[0]);
  const char *key = (const char *) sqlite3_value_text(values[1]);
  int generation  =                sqlite3_value_int(values[2]);

  nsCOMPtr<nsIFile> file;
  nsresult rv = GetCacheDataFile(device->CacheDirectory(), cid, key,
                                 generation, file);
  if (NS_FAILED(rv))
  {
    LOG(("GetCacheDataFile [cid=%s key=%s generation=%d] failed [rv=%x]!\n",
        cid, key, generation, rv));
    return;
  }

#if defined(PR_LOGGING)
  nsCAutoString path;
  file->GetNativePath(path);
  LOG(("  removing %s\n", path.get()));
#endif

  file->Remove(PR_FALSE);
}
#endif






nsresult
nsDiskCacheDevice::Init()
{
  NS_ENSURE_TRUE(!mDB, NS_ERROR_ALREADY_INITIALIZED);

  
  NS_ENSURE_TRUE(mCacheDirectory, NS_ERROR_UNEXPECTED);

  
  nsresult rv = EnsureDir(mCacheDirectory);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> indexFile; 
  rv = mCacheDirectory->Clone(getter_AddRefs(indexFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = indexFile->AppendNative(NS_LITERAL_CSTRING("index.db"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageService> ss =
      do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ss->OpenDatabase(indexFile, getter_AddRefs(mDB));
  NS_ENSURE_SUCCESS(rv, rv);

  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA synchronous = OFF;"));

  

  
  

  
  
  
  
  
  mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE moz_cache (\n"
                         "  ClientID        TEXT,\n"
                         "  Key             TEXT,\n"
                         "  MetaData        BLOB,\n"
                         "  Generation      INTEGER,\n"
                         "  Flags           INTEGER,\n"
                         "  DataSize        INTEGER,\n"
                         "  FetchCount      INTEGER,\n"
                         "  LastFetched     INTEGER,\n"
                         "  LastModified    INTEGER,\n"
                         "  ExpirationTime  INTEGER\n"
                         ");\n"));
  

  mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE UNIQUE INDEX moz_cache_index"
                         " ON moz_cache (ClientID, Key);"));
  

#if 0
  
  res = sqlite3_create_function(mDB, "cache_eviction_observer",
                                3, SQLITE_UTF8, this,
                                nsDiskCacheDevice::EvictionObserver,
                                NULL, NULL);
  if (res != SQLITE_OK)
    LOG(("sqlite3_create_function failed [res=%d]\n", res));
#endif

  
  struct {
    nsCOMPtr<mozIStorageStatement> &statement;
    const char *sql;
  } prepared[] = {
    { mStatement_CacheSize,       "SELECT Sum(DataSize) from moz_cache;" },
    { mStatement_EntryCount,      "SELECT count(*) from moz_cache;" },
    { mStatement_UpdateEntry,     "UPDATE moz_cache SET MetaData = ?, Flags = ?, DataSize = ?, FetchCount = ?, LastFetched = ?, LastModified = ?, ExpirationTime = ? WHERE ClientID = ? AND Key = ?;" },
    { mStatement_UpdateEntrySize, "UPDATE moz_cache SET DataSize = ? WHERE ClientID = ? AND Key = ?;" },
    { mStatement_DeleteEntry,     "DELETE FROM moz_cache WHERE ClientID = ? AND Key = ?;" },
    { mStatement_FindEntry,       "SELECT MetaData, Generation, Flags, DataSize, FetchCount, LastFetched, LastModified, ExpirationTime FROM moz_cache WHERE ClientID = ? AND Key = ?;" },
    { mStatement_BindEntry,       "INSERT INTO moz_cache VALUES(?,?,?,?,?,?,?,?,?,?);" }
  };
  for (PRUint32 i=0; i<NS_ARRAY_LENGTH(prepared); ++i)
  {
    rv |= mDB->CreateStatement(nsDependentCString(prepared[i].sql),
                               getter_AddRefs(prepared[i].statement));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDiskCacheDevice::Shutdown()
{
  NS_ENSURE_TRUE(mDB, NS_ERROR_NOT_INITIALIZED);

#if 0
  int res;

  
  res = sqlite3_create_function(mDB, "cache_eviction_observer", 3, SQLITE_UTF8,
                                NULL, NULL, NULL, NULL);
  if (res != SQLITE_OK)
    LOG(("sqlite3_create_function failed [res=%d]\n", res));
  
  res = sqlite3_close(mDB);
  NS_ENSURE_SQLITE_RESULT(res, NS_ERROR_UNEXPECTED);

  mDB = nsnull;
#endif

  mDB = 0;
  return NS_OK;
}

const char *
nsDiskCacheDevice::GetDeviceID()
{
  return DISK_CACHE_DEVICE_ID;
}

nsCacheEntry *
nsDiskCacheDevice::FindEntry(nsCString *fullKey, PRBool *collision)
{
  LOG(("nsDiskCacheDevice::FindEntry [key=%s]\n", fullKey->get()));

  

  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(fullKey, &cid, &key, keyBuf))
    return nsnull;

  AutoResetStatement statement(mStatement_FindEntry);

  nsresult rv;
  rv  = statement->BindCStringParameter(0, cid);
  rv |= statement->BindCStringParameter(1, key);
  NS_ENSURE_SUCCESS(rv, nsnull);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  if (NS_FAILED(rv) || !hasRows)
    return nsnull; 

  nsDiskCacheRecord rec;
  statement->AsSharedBlob(0, (const void **) &rec.metaData, &rec.metaDataLen);
  rec.generation     = statement->AsInt32(1);
  rec.flags          = statement->AsInt32(2);
  rec.dataSize       = statement->AsInt32(3);
  rec.fetchCount     = statement->AsInt32(4);
  rec.lastFetched    = statement->AsInt64(5);
  rec.lastModified   = statement->AsInt64(6);
  rec.expirationTime = statement->AsInt64(7);

  LOG(("entry: [%u %d %d %d %d %lld %lld %lld]\n",
        rec.metaDataLen,
        rec.generation,
        rec.flags,
        rec.dataSize,
        rec.fetchCount,
        rec.lastFetched,
        rec.lastModified,
        rec.expirationTime));

  return CreateCacheEntry(this, fullKey, rec);

  
  
}

nsresult
nsDiskCacheDevice::DeactivateEntry(nsCacheEntry *entry)
{
  LOG(("nsDiskCacheDevice::DeactivateEntry [key=%s]\n", entry->Key()->get()));

  
  
  

  if (entry->IsDoomed())
  {
    

    
    
    
    DeleteData(entry);
  }
  else
  {
    

    
    

    UpdateEntry(entry);
  }

  delete entry;
  return NS_OK;
}

nsresult
nsDiskCacheDevice::BindEntry(nsCacheEntry *entry)
{
  LOG(("nsDiskCacheDevice::BindEntry [key=%s]\n", entry->Key()->get()));

  NS_ENSURE_STATE(!entry->Data());

  
  
  

  

  
  
  

  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(entry->Key(), &cid, &key, keyBuf))
    return NS_ERROR_UNEXPECTED;

  
  nsRefPtr<nsDiskCacheBinding> binding =
      nsDiskCacheBinding::Create(mCacheDirectory, entry->Key()->get(), -1);
  if (!binding)
    return NS_ERROR_OUT_OF_MEMORY;

  nsDiskCacheRecord rec;
  rec.clientID = cid;
  rec.key = key;
  rec.metaData = NULL; 
  rec.metaDataLen = 0;
  rec.generation = binding->mGeneration;
  rec.flags = 0x1;  
  rec.dataSize = 0;
  rec.fetchCount = entry->FetchCount();
  rec.lastFetched = PRTimeFromSeconds(entry->LastFetched());
  rec.lastModified = PRTimeFromSeconds(entry->LastModified());
  rec.expirationTime = PRTimeFromSeconds(entry->ExpirationTime());

  AutoResetStatement statement(mStatement_BindEntry);

  nsresult rv;
  rv  = statement->BindCStringParameter(0, rec.clientID);
  rv |= statement->BindCStringParameter(1, rec.key);
  rv |= statement->BindDataParameter(2, rec.metaData, rec.metaDataLen);
  rv |= statement->BindInt32Parameter(3, rec.generation);
  rv |= statement->BindInt32Parameter(4, rec.flags);
  rv |= statement->BindInt32Parameter(5, rec.dataSize);
  rv |= statement->BindInt32Parameter(6, rec.fetchCount);
  rv |= statement->BindInt64Parameter(7, rec.lastFetched);
  rv |= statement->BindInt64Parameter(8, rec.lastModified);
  rv |= statement->BindInt64Parameter(9, rec.expirationTime);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!hasRows, "INSERT should not result in output");

  entry->SetData(binding);
  return NS_OK;
}

void
nsDiskCacheDevice::DoomEntry(nsCacheEntry *entry)
{
  LOG(("nsDiskCacheDevice::DoomEntry [key=%s]\n", entry->Key()->get()));

  
  

  
  
  
  DeleteEntry(entry, PR_FALSE);
}

nsresult
nsDiskCacheDevice::OpenInputStreamForEntry(nsCacheEntry      *entry,
                                           nsCacheAccessMode  mode,
                                           PRUint32           offset,
                                           nsIInputStream   **result)
{
  LOG(("nsDiskCacheDevice::OpenInputStreamForEntry [key=%s]\n", entry->Key()->get()));

  *result = nsnull;

  NS_ENSURE_TRUE(offset < entry->DataSize(), NS_ERROR_INVALID_ARG);

  
  

  nsDiskCacheBinding *binding = (nsDiskCacheBinding *) entry->Data();
  NS_ENSURE_STATE(binding);

  nsCOMPtr<nsIInputStream> in;
  NS_NewLocalFileInputStream(getter_AddRefs(in), binding->mDataFile, PR_RDONLY);
  if (!in)
    return NS_ERROR_UNEXPECTED;

  
  if (offset != 0)
  {
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(in);
    NS_ENSURE_TRUE(seekable, NS_ERROR_UNEXPECTED);

    seekable->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  }

  in.swap(*result);
  return NS_OK;
}

nsresult
nsDiskCacheDevice::OpenOutputStreamForEntry(nsCacheEntry       *entry,
                                            nsCacheAccessMode   mode,
                                            PRUint32            offset,
                                            nsIOutputStream   **result)
{
  LOG(("nsDiskCacheDevice::OpenOutputStreamForEntry [key=%s]\n", entry->Key()->get()));

  *result = nsnull;

  NS_ENSURE_TRUE(offset <= entry->DataSize(), NS_ERROR_INVALID_ARG);

  
  

  nsDiskCacheBinding *binding = (nsDiskCacheBinding *) entry->Data();
  NS_ENSURE_STATE(binding);

  nsCOMPtr<nsIOutputStream> out;
  NS_NewLocalFileOutputStream(getter_AddRefs(out), binding->mDataFile,
                              PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
                              00600);
  if (!out)
    return NS_ERROR_UNEXPECTED;

  
  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(out);
  NS_ENSURE_TRUE(seekable, NS_ERROR_UNEXPECTED);
  if (offset != 0)
    seekable->Seek(nsISeekableStream::NS_SEEK_SET, offset);

  
  seekable->SetEOF();

  nsCOMPtr<nsIOutputStream> bufferedOut;
  NS_NewBufferedOutputStream(getter_AddRefs(bufferedOut), out, 16 * 1024);
  if (!bufferedOut)
    return NS_ERROR_UNEXPECTED;

  bufferedOut.swap(*result);
  return NS_OK;
}

nsresult
nsDiskCacheDevice::GetFileForEntry(nsCacheEntry *entry, nsIFile **result)
{
  LOG(("nsDiskCacheDevice::GetFileForEntry [key=%s]\n", entry->Key()->get()));

  nsDiskCacheBinding *binding = (nsDiskCacheBinding *) entry->Data();
  NS_ENSURE_STATE(binding);

  NS_IF_ADDREF(*result = binding->mDataFile);
  return NS_OK;
}

nsresult
nsDiskCacheDevice::OnDataSizeChange(nsCacheEntry *entry, PRInt32 deltaSize)
{
  LOG(("nsDiskCacheDevice::OnDataSizeChange [key=%s delta=%d]\n",
      entry->Key()->get(), deltaSize));

  
  

  
  






  const PRInt32 DELTA_THRESHOLD = 1<<14; 

  mDeltaCounter += deltaSize; 

  
  if (mDeltaCounter >= DELTA_THRESHOLD)
  {
    PRUint32 targetCap, delta = mDeltaCounter;
    if (delta <= mCacheCapacity)
      targetCap = mCacheCapacity - delta;
    else
      targetCap = 0;
    EvictDiskCacheEntries(targetCap);
    mDeltaCounter = 0; 
  }

  return NS_OK;
}

nsresult
nsDiskCacheDevice::Visit(nsICacheVisitor *visitor)
{
  NS_ENSURE_TRUE(Initialized(), NS_ERROR_NOT_INITIALIZED);

  

  nsCOMPtr<nsICacheDeviceInfo> deviceInfo =
      new nsDiskCacheDeviceInfo(this);

  PRBool keepGoing;
  nsresult rv = visitor->VisitDevice(DISK_CACHE_DEVICE_ID, deviceInfo,
                                     &keepGoing);
  if (NS_FAILED(rv))
    return rv;
  
  if (!keepGoing)
    return NS_OK;

  

  nsDiskCacheRecord rec;
  nsRefPtr<nsDiskCacheEntryInfo> info = new nsDiskCacheEntryInfo;
  if (!info)
    return NS_ERROR_OUT_OF_MEMORY;
  info->mRec = &rec;

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDB->CreateStatement(
      NS_LITERAL_CSTRING("SELECT * FROM moz_cache;"),
      getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  for (;;)
  {
    rv = statement->ExecuteStep(&hasRows);
    if (NS_FAILED(rv) || !hasRows)
      break;

    rec.clientID       = statement->AsSharedCString(0, NULL);
    rec.key            = statement->AsSharedCString(1, NULL);
    statement->AsSharedBlob(2, (const void **) &rec.metaData, &rec.metaDataLen);
    rec.generation     = statement->AsInt32(3);
    rec.flags          = statement->AsInt32(4);
    rec.dataSize       = statement->AsInt32(5);
    rec.fetchCount     = statement->AsInt32(6);
    rec.lastFetched    = statement->AsInt64(7);
    rec.lastModified   = statement->AsInt64(8);
    rec.expirationTime = statement->AsInt64(9);

    PRBool keepGoing;
    rv = visitor->VisitEntry(DISK_CACHE_DEVICE_ID, info, &keepGoing);
    if (NS_FAILED(rv) || !keepGoing)
      break;
  }

  info->mRec = nsnull;
  return NS_OK;
}

nsresult
nsDiskCacheDevice::EvictEntries(const char *clientID)
{
  LOG(("nsDiskCacheDevice::EvictEntries [cid=%s]\n", clientID ? clientID : ""));

  

  
  

  nsresult rv = EnableEvictionObserver();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  const char *deleteCmd;
  if (clientID)
  {
    deleteCmd =
      PR_smprintf("DELETE FROM moz_cache WHERE ClientID=%q AND Flags=0;",
                  clientID);
    if (!deleteCmd)
      return NS_ERROR_OUT_OF_MEMORY;
  }
  else
  {
    deleteCmd = "DELETE FROM moz_cache WHERE Flags = 0;";
  }

  rv = mDB->ExecuteSimpleSQL(nsDependentCString(deleteCmd));
  if (clientID)
    PR_smprintf_free((char *) deleteCmd);
  NS_ENSURE_SUCCESS(rv, rv);

  DisableEvictionObserver();

  return NS_OK;
}

nsresult
nsDiskCacheDevice::EvictDiskCacheEntries(PRUint32 desiredCapacity)
{
  LOG(("nsDiskCacheDevice::EvictDiskCacheEntries [goal=%u delta=%d]\n",
      desiredCapacity, mCacheCapacity - desiredCapacity));

  
  

  
  
  
  

  nsresult rv = EnableEvictionObserver();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 lastCacheSize = PR_UINT32_MAX, cacheSize;
  for (;;)
  {
    cacheSize = CacheSize();
    if (cacheSize <= desiredCapacity)
      break;
    if (cacheSize == lastCacheSize)
    {
      LOG(("unable to reduce cache size to target capacity!\n"));
      break;
    }

    rv = mDB->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DELETE FROM moz_cache WHERE LastFetched IN ("
                           " SELECT Min(LastFetched) FROM moz_cache"
                           " WHERE Flags=0);"));
    if (NS_FAILED(rv))
    {
      LOG(("failure while deleting Min(LastFetched)\n"));
      break;
    }

    lastCacheSize = cacheSize;
  }

  DisableEvictionObserver();
  return NS_OK;
}






void
nsDiskCacheDevice::SetCacheParentDirectory(nsILocalFile *parentDir)
{
  if (Initialized())
  {
    NS_ERROR("cannot switch cache directory once initialized");
    return;
  }

  if (!parentDir)
  {
    mCacheDirectory = nsnull;
    return;
  }

  
  nsresult rv = EnsureDir(parentDir);
  if (NS_FAILED(rv))
  {
    NS_WARNING("unable to create parent directory");
    return;
  }

  
  nsCOMPtr<nsIFile> dir;
  rv = parentDir->Clone(getter_AddRefs(dir));
  if (NS_FAILED(rv))
    return;
  rv = dir->AppendNative(NS_LITERAL_CSTRING("cache_sql"));
  if (NS_FAILED(rv))
    return;

  mCacheDirectory = do_QueryInterface(dir);
}

void
nsDiskCacheDevice::SetCapacity(PRUint32 capacity)
{
  mCacheCapacity = capacity * 1024;
  if (Initialized())
    EvictDiskCacheEntries(mCacheCapacity);
}
