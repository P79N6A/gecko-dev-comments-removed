






































#include "nsCache.h"
#include "nsDiskCache.h"
#include "nsDiskCacheDeviceSQL.h"
#include "nsCacheService.h"

#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "nsEscape.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsString.h"
#include "nsPrintfCString.h"
#include "nsCRT.h"
#include "nsArrayUtils.h"
#include "nsIArray.h"
#include "nsIVariant.h"

#include "mozIStorageService.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "mozStorageHelper.h"

#include "nsICacheVisitor.h"
#include "nsISeekableStream.h"

static const char OFFLINE_CACHE_DEVICE_ID[] = { "offline" };
static NS_DEFINE_CID(kCacheServiceCID, NS_CACHESERVICE_CID);

#define LOG(args) CACHE_LOG_DEBUG(args)

static PRUint32 gNextTemporaryClientID = 0;





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

class EvictionObserver
{
  public:
  EvictionObserver(mozIStorageConnection *db,
                   nsOfflineCacheEvictionFunction *evictionFunction)
    : mDB(db), mEvictionFunction(evictionFunction)
    {
      mDB->ExecuteSimpleSQL(
          NS_LITERAL_CSTRING("CREATE TEMP TRIGGER cache_on_delete AFTER DELETE"
                             " ON moz_cache FOR EACH ROW BEGIN SELECT"
                             " cache_eviction_observer("
                             "  OLD.key, OLD.generation);"
                             " END;"));
      mEvictionFunction->Reset();
    }

    ~EvictionObserver()
    {
      mDB->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP TRIGGER cache_on_delete;"));
      mEvictionFunction->Reset();
    }

    void Apply() { return mEvictionFunction->Apply(); }

  private:
    mozIStorageConnection *mDB;
    nsRefPtr<nsOfflineCacheEvictionFunction> mEvictionFunction;
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





NS_IMPL_ISUPPORTS1(nsOfflineCacheEvictionFunction, mozIStorageFunction)



static nsresult
GetCacheDataFile(nsIFile *cacheDir, const char *key,
                 int generation, nsCOMPtr<nsIFile> &file)
{
  cacheDir->Clone(getter_AddRefs(file));
  if (!file)
    return NS_ERROR_OUT_OF_MEMORY;

  PRUint64 hash = DCacheHash(key);

  PRUint32 dir1 = (PRUint32) (hash & 0x0F);
  PRUint32 dir2 = (PRUint32)((hash & 0xF0) >> 4);

  hash >>= 8;

  file->AppendNative(nsPrintfCString("%X", dir1));
  file->AppendNative(nsPrintfCString("%X", dir2));

  char leaf[64];
  PR_snprintf(leaf, sizeof(leaf), "%014llX-%X", hash, generation);
  return file->AppendNative(nsDependentCString(leaf));
}

NS_IMETHODIMP
nsOfflineCacheEvictionFunction::OnFunctionCall(mozIStorageValueArray *values, nsIVariant **_retval)
{
  LOG(("nsOfflineCacheEvictionFunction::OnFunctionCall\n"));

  *_retval = nsnull;

  PRUint32 numEntries;
  nsresult rv = values->GetNumEntries(&numEntries);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(numEntries == 2, "unexpected number of arguments");

  PRUint32 valueLen;
  const char *key = values->AsSharedUTF8String(0, &valueLen);
  int generation  = values->AsInt32(1);

  nsCOMPtr<nsIFile> file;
  rv = GetCacheDataFile(mDevice->CacheDirectory(), key,
                        generation, file);
  if (NS_FAILED(rv))
  {
    LOG(("GetCacheDataFile [key=%s generation=%d] failed [rv=%x]!\n",
         key, generation, rv));
    return rv;
  }

  mItems.AppendObject(file);

  return NS_OK;
}

void
nsOfflineCacheEvictionFunction::Apply()
{
  LOG(("nsOfflineCacheEvictionFunction::Apply\n"));

  for (PRInt32 i = 0; i < mItems.Count(); i++) {
#if defined(PR_LOGGING)
    nsCAutoString path;
    mItems[i]->GetNativePath(path);
    LOG(("  removing %s\n", path.get()));
#endif

    mItems[i]->Remove(PR_FALSE);
  }

  Reset();
}





class nsOfflineCacheDeviceInfo : public nsICacheDeviceInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICACHEDEVICEINFO

  nsOfflineCacheDeviceInfo(nsOfflineCacheDevice* device)
    : mDevice(device)
  {}

private:
  nsOfflineCacheDevice* mDevice;
};

NS_IMPL_ISUPPORTS1(nsOfflineCacheDeviceInfo, nsICacheDeviceInfo)

NS_IMETHODIMP
nsOfflineCacheDeviceInfo::GetDescription(char **aDescription)
{
  *aDescription = NS_strdup("Offline cache device");
  return *aDescription ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsOfflineCacheDeviceInfo::GetUsageReport(char ** usageReport)
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
nsOfflineCacheDeviceInfo::GetEntryCount(PRUint32 *aEntryCount)
{
  *aEntryCount = mDevice->EntryCount();
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheDeviceInfo::GetTotalSize(PRUint32 *aTotalSize)
{
  *aTotalSize = mDevice->CacheSize();
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheDeviceInfo::GetMaximumSize(PRUint32 *aMaximumSize)
{
  *aMaximumSize = mDevice->CacheCapacity();
  return NS_OK;
}





class nsOfflineCacheBinding : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  static nsOfflineCacheBinding *
      Create(nsIFile *cacheDir, const nsCString *key, int generation);

  nsCOMPtr<nsIFile> mDataFile;
  int               mGeneration;
};

NS_IMPL_THREADSAFE_ISUPPORTS0(nsOfflineCacheBinding)

nsOfflineCacheBinding *
nsOfflineCacheBinding::Create(nsIFile *cacheDir,
                              const nsCString *fullKey,
                              int generation)
{
  nsCOMPtr<nsIFile> file;
  cacheDir->Clone(getter_AddRefs(file));
  if (!file)
    return nsnull;

  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(fullKey, &cid, &key, keyBuf))
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

  nsOfflineCacheBinding *binding = new nsOfflineCacheBinding;
  if (!binding)
    return nsnull;

  binding->mDataFile.swap(file);
  binding->mGeneration = generation;
  return binding;
}





struct nsOfflineCacheRecord
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
CreateCacheEntry(nsOfflineCacheDevice *device,
                 const nsCString *fullKey,
                 const nsOfflineCacheRecord &rec)
{
  if (rec.flags != 0)
  {
    LOG(("refusing to load busy entry\n"));
    return nsnull;
  }

  nsCacheEntry *entry;
  
  nsresult rv = nsCacheEntry::Create(fullKey->get(), 
                                     nsICache::STREAM_BASED,
                                     nsICache::STORE_OFFLINE,
                                     device, &entry);
  if (NS_FAILED(rv))
    return nsnull;

  entry->SetFetchCount((PRUint32) rec.fetchCount);
  entry->SetLastFetched(SecondsFromPRTime(rec.lastFetched));
  entry->SetLastModified(SecondsFromPRTime(rec.lastModified));
  entry->SetExpirationTime(SecondsFromPRTime(rec.expirationTime));
  entry->SetDataSize((PRUint32) rec.dataSize);

  entry->UnflattenMetaData((const char *) rec.metaData, rec.metaDataLen);

  
  nsOfflineCacheBinding *binding =
      nsOfflineCacheBinding::Create(device->CacheDirectory(),
                                    fullKey,
                                    rec.generation);
  if (!binding)
  {
    delete entry;
    return nsnull;
  }
  entry->SetData(binding);

  return entry;
}






class nsOfflineCacheEntryInfo : public nsICacheEntryInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICACHEENTRYINFO

  nsOfflineCacheRecord *mRec;
};

NS_IMPL_ISUPPORTS1(nsOfflineCacheEntryInfo, nsICacheEntryInfo)

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetClientID(char **result)
{
  *result = NS_strdup(mRec->clientID);
  return *result ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetDeviceID(char ** deviceID)
{
  *deviceID = NS_strdup(OFFLINE_CACHE_DEVICE_ID);
  return *deviceID ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetKey(nsACString &clientKey)
{
  clientKey.Assign(mRec->key);
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetFetchCount(PRInt32 *aFetchCount)
{
  *aFetchCount = mRec->fetchCount;
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetLastFetched(PRUint32 *aLastFetched)
{
  *aLastFetched = SecondsFromPRTime(mRec->lastFetched);
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetLastModified(PRUint32 *aLastModified)
{
  *aLastModified = SecondsFromPRTime(mRec->lastModified);
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetExpirationTime(PRUint32 *aExpirationTime)
{
  *aExpirationTime = SecondsFromPRTime(mRec->expirationTime);
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::IsStreamBased(PRBool *aStreamBased)
{
  *aStreamBased = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheEntryInfo::GetDataSize(PRUint32 *aDataSize)
{
  *aDataSize = mRec->dataSize;
  return NS_OK;
}






NS_IMPL_ISUPPORTS1(nsApplicationCacheNamespace, nsIApplicationCacheNamespace)

NS_IMETHODIMP
nsApplicationCacheNamespace::Init(PRUint32 itemType,
                                  const nsACString &namespaceSpec,
                                  const nsACString &data)
{
  mItemType = itemType;
  mNamespaceSpec = namespaceSpec;
  mData = data;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCacheNamespace::GetItemType(PRUint32 *out)
{
  *out = mItemType;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCacheNamespace::GetNamespaceSpec(nsACString &out)
{
  out = mNamespaceSpec;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCacheNamespace::GetData(nsACString &out)
{
  out = mData;
  return NS_OK;
}





class nsApplicationCache : public nsIApplicationCache
                         , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAPPLICATIONCACHE

  nsApplicationCache(nsOfflineCacheDevice *device,
                     const nsACString &group,
                     const nsACString &clientID);

  virtual ~nsApplicationCache();

  void MarkInvalid() { mValid = PR_FALSE; }

private:
  nsRefPtr<nsOfflineCacheDevice> mDevice;
  nsCString mGroup;
  nsCString mClientID;
  PRBool mValid;
};

NS_IMPL_ISUPPORTS2(nsApplicationCache,
                   nsIApplicationCache,
                   nsISupportsWeakReference)

nsApplicationCache::nsApplicationCache(nsOfflineCacheDevice *device,
                                       const nsACString &group,
                                       const nsACString &clientID)
  : mDevice(device)
  , mGroup(group)
  , mClientID(clientID)
  , mValid(PR_TRUE)
{
}

nsApplicationCache::~nsApplicationCache()
{
  mDevice->mCaches.Remove(mClientID);

  
  if (mValid && !mDevice->IsActiveCache(mGroup, mClientID))
    Discard();
}

NS_IMETHODIMP
nsApplicationCache::GetGroupID(nsACString &out)
{
  out = mGroup;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCache::GetClientID(nsACString &out)
{
  out = mClientID;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCache::GetActive(PRBool *out)
{
  *out = mDevice->IsActiveCache(mGroup, mClientID);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCache::Activate()
{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  mDevice->ActivateCache(mGroup, mClientID);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCache::Discard()
{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  mValid = PR_FALSE;

  if (mDevice->IsActiveCache(mGroup, mClientID))
  {
    mDevice->DeactivateGroup(mGroup);
  }

  return mDevice->EvictEntries(mClientID.get());
}

NS_IMETHODIMP
nsApplicationCache::MarkEntry(const nsACString &key,
                              PRUint32 typeBits)
{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  return mDevice->MarkEntry(mClientID, key, typeBits);
}


NS_IMETHODIMP
nsApplicationCache::UnmarkEntry(const nsACString &key,
                                PRUint32 typeBits)
{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  return mDevice->UnmarkEntry(mClientID, key, typeBits);
}

NS_IMETHODIMP
nsApplicationCache::GetTypes(const nsACString &key,
                             PRUint32 *typeBits)
{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  return mDevice->GetTypes(mClientID, key, typeBits);
}

NS_IMETHODIMP
nsApplicationCache::GatherEntries(PRUint32 typeBits,
                                  PRUint32 * count,
                                  char *** keys)
{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  return mDevice->GatherEntries(mClientID, typeBits, count, keys);
}

NS_IMETHODIMP
nsApplicationCache::AddNamespaces(nsIArray *namespaces)
{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  if (!namespaces)
    return NS_OK;

  mozStorageTransaction transaction(mDevice->mDB, PR_FALSE);

  PRUint32 length;
  nsresult rv = namespaces->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<nsIApplicationCacheNamespace> ns =
      do_QueryElementAt(namespaces, i);
    if (ns) {
      rv = mDevice->AddNamespace(mClientID, ns);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsApplicationCache::GetMatchingNamespace(const nsACString &key,
                                         nsIApplicationCacheNamespace **out)

{
  NS_ENSURE_TRUE(mValid, NS_ERROR_NOT_AVAILABLE);

  return mDevice->GetMatchingNamespace(mClientID, key, out);
}





NS_IMPL_ISUPPORTS1(nsOfflineCacheDevice, nsIApplicationCacheService)

nsOfflineCacheDevice::nsOfflineCacheDevice()
  : mDB(nsnull)
  , mCacheCapacity(0)
  , mDeltaCounter(0)
{
}

nsOfflineCacheDevice::~nsOfflineCacheDevice()
{
  Shutdown();
}


PRBool
nsOfflineCacheDevice::GetStrictFileOriginPolicy()
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);

    PRBool retval;
    if (prefs && NS_SUCCEEDED(prefs->GetBoolPref("security.fileuri.strict_origin_policy", &retval)))
        return retval;

    
    return PR_TRUE;
}

PRUint32
nsOfflineCacheDevice::CacheSize()
{
  AutoResetStatement statement(mStatement_CacheSize);

  PRBool hasRows;
  nsresult rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && hasRows, 0);
  
  return (PRUint32) statement->AsInt32(0);
}

PRUint32
nsOfflineCacheDevice::EntryCount()
{
  AutoResetStatement statement(mStatement_EntryCount);

  PRBool hasRows;
  nsresult rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && hasRows, 0);

  return (PRUint32) statement->AsInt32(0);
}

nsresult
nsOfflineCacheDevice::UpdateEntry(nsCacheEntry *entry)
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

  nsOfflineCacheRecord rec;
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
  rv  = statement->BindBlobParameter(0, rec.metaData, rec.metaDataLen);
  rv |= statement->BindInt32Parameter(1, rec.flags);
  rv |= statement->BindInt32Parameter(2, rec.dataSize);
  rv |= statement->BindInt32Parameter(3, rec.fetchCount);
  rv |= statement->BindInt64Parameter(4, rec.lastFetched);
  rv |= statement->BindInt64Parameter(5, rec.lastModified);
  rv |= statement->BindInt64Parameter(6, rec.expirationTime);
  rv |= statement->BindUTF8StringParameter(7, nsDependentCString(cid));
  rv |= statement->BindUTF8StringParameter(8, nsDependentCString(key));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!hasRows, "UPDATE should not result in output");
  return rv;
}

nsresult
nsOfflineCacheDevice::UpdateEntrySize(nsCacheEntry *entry, PRUint32 newSize)
{
  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(entry->Key(), &cid, &key, keyBuf))
    return NS_ERROR_UNEXPECTED;

  AutoResetStatement statement(mStatement_UpdateEntrySize);

  nsresult rv;
  rv  = statement->BindInt32Parameter(0, newSize);
  rv |= statement->BindUTF8StringParameter(1, nsDependentCString(cid));
  rv |= statement->BindUTF8StringParameter(2, nsDependentCString(key));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!hasRows, "UPDATE should not result in output");
  return rv;
}

nsresult
nsOfflineCacheDevice::DeleteEntry(nsCacheEntry *entry, PRBool deleteData)
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
  rv  = statement->BindUTF8StringParameter(0, nsDependentCString(cid));
  rv |= statement->BindUTF8StringParameter(1, nsDependentCString(key));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(!hasRows, "DELETE should not result in output");
  return rv;
}

nsresult
nsOfflineCacheDevice::DeleteData(nsCacheEntry *entry)
{
  nsOfflineCacheBinding *binding = (nsOfflineCacheBinding *) entry->Data();
  NS_ENSURE_STATE(binding);

  return binding->mDataFile->Remove(PR_FALSE);
}






nsOfflineCacheDevice *
nsOfflineCacheDevice::GetInstance()
{
  nsresult rv;
  nsCOMPtr<nsICacheService> serv = do_GetService(kCacheServiceCID, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsICacheService *iservice = static_cast<nsICacheService*>(serv.get());
  nsCacheService *cacheService = static_cast<nsCacheService*>(iservice);
  rv = cacheService->CreateOfflineDevice();
  NS_ENSURE_SUCCESS(rv, nsnull);

  NS_IF_ADDREF(cacheService->mOfflineDevice);
  return cacheService->mOfflineDevice;
}

nsresult
nsOfflineCacheDevice::Init()
{
  NS_ENSURE_TRUE(!mDB, NS_ERROR_ALREADY_INITIALIZED);

  
  NS_ENSURE_TRUE(mCacheDirectory, NS_ERROR_UNEXPECTED);

  
  nsresult rv = EnsureDir(mCacheDirectory);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> indexFile; 
  rv = mCacheDirectory->Clone(getter_AddRefs(indexFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = indexFile->AppendNative(NS_LITERAL_CSTRING("index.sqlite"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageService> ss =
      do_GetService("@mozilla.org/storage/service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ss->OpenDatabase(indexFile, getter_AddRefs(mDB));
  NS_ENSURE_SUCCESS(rv, rv);

  mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA synchronous = OFF;"));

  

  
  

  
  
  
  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_cache (\n"
                         "  ClientID        TEXT,\n"
                         "  Key             TEXT,\n"
                         "  MetaData        BLOB,\n"
                         "  Generation      INTEGER,\n"
                         "  Flags           INTEGER,\n"
                         "  DataSize        INTEGER,\n"
                         "  FetchCount      INTEGER,\n"
                         "  LastFetched     INTEGER,\n"
                         "  LastModified    INTEGER,\n"
                         "  ExpirationTime  INTEGER,\n"
                         "  ItemType        INTEGER DEFAULT 0\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mDB->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("ALTER TABLE moz_cache ADD ItemType INTEGER DEFAULT 0"));

  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_cache_groups (\n"
                         " GroupID TEXT PRIMARY KEY,\n"
                         " ActiveClientID TEXT\n"
                         ");\n"));
  NS_ENSURE_SUCCESS(rv, rv);

  mDB->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("ALTER TABLE moz_cache_groups "
                       "ADD ActivateTimeStamp INTEGER DEFAULT 0"));

  
  
  
  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS"
                         " moz_cache_namespaces (\n"
                         " ClientID TEXT,\n"
                         " NameSpace TEXT,\n"
                         " Data TEXT,\n"
                         " ItemType INTEGER\n"
                          ");\n"));
   NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_cache_index"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE UNIQUE INDEX IF NOT EXISTS "
                         " moz_cache_key_clientid_index"
                         " ON moz_cache (Key, ClientID);"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE UNIQUE INDEX IF NOT EXISTS"
                         " moz_cache_namespaces_clientid_index"
                         " ON moz_cache_namespaces (ClientID, NameSpace);"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDB->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS"
                         " moz_cache_namespaces_namespace_index"
                         " ON moz_cache_namespaces (NameSpace);"));
  NS_ENSURE_SUCCESS(rv, rv);


  mEvictionFunction = new nsOfflineCacheEvictionFunction(this);
  if (!mEvictionFunction) return NS_ERROR_OUT_OF_MEMORY;

  rv = mDB->CreateFunction(NS_LITERAL_CSTRING("cache_eviction_observer"), 2, mEvictionFunction);
  NS_ENSURE_SUCCESS(rv, rv);

  
  struct StatementSql {
    nsCOMPtr<mozIStorageStatement> &statement;
    const char *sql;
    StatementSql (nsCOMPtr<mozIStorageStatement> &aStatement, const char *aSql):
      statement (aStatement), sql (aSql) {}
  } prepared[] = {
    StatementSql ( mStatement_CacheSize,         "SELECT Sum(DataSize) from moz_cache;" ),
    
    StatementSql ( mStatement_DomainSize,        "SELECT 0;"),
    StatementSql ( mStatement_EntryCount,        "SELECT count(*) from moz_cache;" ),
    StatementSql ( mStatement_UpdateEntry,       "UPDATE moz_cache SET MetaData = ?, Flags = ?, DataSize = ?, FetchCount = ?, LastFetched = ?, LastModified = ?, ExpirationTime = ? WHERE ClientID = ? AND Key = ?;" ),
    StatementSql ( mStatement_UpdateEntrySize,   "UPDATE moz_cache SET DataSize = ? WHERE ClientID = ? AND Key = ?;" ),
    StatementSql ( mStatement_UpdateEntryFlags,  "UPDATE moz_cache SET Flags = ? WHERE ClientID = ? AND Key = ?;" ),
    StatementSql ( mStatement_DeleteEntry,       "DELETE FROM moz_cache WHERE ClientID = ? AND Key = ?;" ),
    StatementSql ( mStatement_FindEntry,         "SELECT MetaData, Generation, Flags, DataSize, FetchCount, LastFetched, LastModified, ExpirationTime, ItemType FROM moz_cache WHERE ClientID = ? AND Key = ?;" ),
    StatementSql ( mStatement_BindEntry,         "INSERT INTO moz_cache (ClientID, Key, MetaData, Generation, Flags, DataSize, FetchCount, LastFetched, LastModified, ExpirationTime) VALUES(?,?,?,?,?,?,?,?,?,?);" ),

    StatementSql ( mStatement_MarkEntry,         "UPDATE moz_cache SET ItemType = (ItemType | ?) WHERE ClientID = ? AND Key = ?;" ),
    StatementSql ( mStatement_UnmarkEntry,       "UPDATE moz_cache SET ItemType = (ItemType & ~?) WHERE ClientID = ? AND Key = ?;" ),
    StatementSql ( mStatement_GetTypes,          "SELECT ItemType FROM moz_cache WHERE ClientID = ? AND Key = ?;"),
    StatementSql ( mStatement_CleanupUnmarked,   "DELETE FROM moz_cache WHERE ClientID = ? AND Key = ? AND ItemType = 0;" ),
    StatementSql ( mStatement_GatherEntries,     "SELECT Key FROM moz_cache WHERE ClientID = ? AND (ItemType & ?) > 0;" ),

    StatementSql ( mStatement_ActivateClient,    "INSERT OR REPLACE INTO moz_cache_groups (GroupID, ActiveClientID, ActivateTimeStamp) VALUES (?, ?, ?);" ),
    StatementSql ( mStatement_DeactivateGroup,   "DELETE FROM moz_cache_groups WHERE GroupID = ?;" ),
    StatementSql ( mStatement_FindClient,        "SELECT ClientID, ItemType FROM moz_cache WHERE Key = ? ORDER BY LastFetched DESC, LastModified DESC;" ),

    
    
    StatementSql ( mStatement_FindClientByNamespace, "SELECT ns.ClientID, ns.ItemType FROM"
                                                     "  moz_cache_namespaces AS ns JOIN moz_cache_groups AS groups"
                                                     "  ON ns.ClientID = groups.ActiveClientID"
                                                     " WHERE ns.NameSpace <= ?1 AND ?1 GLOB ns.NameSpace || '*'"
                                                     " ORDER BY ns.NameSpace DESC, groups.ActivateTimeStamp DESC;"),
    StatementSql ( mStatement_FindNamespaceEntry,    "SELECT ns.NameSpace, ns.Data, ns.ItemType FROM "
                                                     "  moz_cache_namespaces AS ns JOIN moz_cache_groups AS groups"
                                                     "  ON ns.ClientID = groups.ActiveClientID"
                                                     " WHERE ClientID = ?1"
                                                     " AND NameSpace <= ?2 AND ?2 GLOB NameSpace || '*'"
                                                     " ORDER BY ns.NameSpace DESC, groups.ActivateTimeStamp DESC;"),
    StatementSql ( mStatement_InsertNamespaceEntry,  "INSERT INTO moz_cache_namespaces (ClientID, NameSpace, Data, ItemType) VALUES(?, ?, ?, ?);"),
  };
  for (PRUint32 i = 0; NS_SUCCEEDED(rv) && i < NS_ARRAY_LENGTH(prepared); ++i)
  {
    LOG(("Creating statement: %s\n", prepared[i].sql));

    rv = mDB->CreateStatement(nsDependentCString(prepared[i].sql),
                              getter_AddRefs(prepared[i].statement));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mDB->ExecuteSimpleSQL(
         NS_LITERAL_CSTRING("UPDATE moz_cache"
                            " SET Flags=(Flags & ~1)"
                            " WHERE (Flags & 1);"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitActiveCaches();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::InitActiveCaches()
{
  NS_ENSURE_TRUE(mCaches.Init(), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mActiveCachesByGroup.Init(), NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mActiveCaches.Init(5);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> statement;
  rv =
    mDB->CreateStatement(NS_LITERAL_CSTRING("SELECT GroupID, ActiveClientID"
                                            " FROM moz_cache_groups"),
                         getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  while (hasRows)
  {
    nsCAutoString group;
    statement->GetUTF8String(0, group);
    nsCString clientID;
    statement->GetUTF8String(1, clientID);

    mActiveCaches.Put(clientID);
    mActiveCachesByGroup.Put(group, new nsCString(clientID));

    rv = statement->ExecuteStep(&hasRows);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


PLDHashOperator
nsOfflineCacheDevice::ShutdownApplicationCache(const nsACString &key,
                                               nsIWeakReference *weakRef,
                                               void *ctx)
{
  nsCOMPtr<nsIApplicationCache> obj = do_QueryReferent(weakRef);
  if (obj)
  {
    nsApplicationCache *appCache = static_cast<nsApplicationCache*>(obj.get());
    appCache->MarkInvalid();
  }

  return PL_DHASH_NEXT;
}

nsresult
nsOfflineCacheDevice::Shutdown()
{
  NS_ENSURE_TRUE(mDB, NS_ERROR_NOT_INITIALIZED);

  if (mCaches.IsInitialized())
    mCaches.EnumerateRead(ShutdownApplicationCache, this);

  EvictionObserver evictionObserver(mDB, mEvictionFunction);

  
  nsresult rv = mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_cache WHERE rowid IN"
    "  (SELECT moz_cache.rowid FROM"
    "    moz_cache LEFT OUTER JOIN moz_cache_groups ON"
    "      (moz_cache.ClientID = moz_cache_groups.ActiveClientID)"
    "   WHERE moz_cache_groups.GroupID ISNULL)"));

  if (NS_FAILED(rv))
    NS_WARNING("Failed to clean up unused application caches.");
  else
    evictionObserver.Apply();

  
  rv = mDB->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_cache_namespaces WHERE rowid IN"
    "  (SELECT moz_cache_namespaces.rowid FROM"
    "    moz_cache_namespaces LEFT OUTER JOIN moz_cache_groups ON"
    "      (moz_cache_namespaces.ClientID = moz_cache_groups.ActiveClientID)"
    "   WHERE moz_cache_groups.GroupID ISNULL)"));

  if (NS_FAILED(rv))
    NS_WARNING("Failed to clean up namespaces.");

  mDB = 0;
  mEvictionFunction = 0;

  return NS_OK;
}

const char *
nsOfflineCacheDevice::GetDeviceID()
{
  return OFFLINE_CACHE_DEVICE_ID;
}

nsCacheEntry *
nsOfflineCacheDevice::FindEntry(nsCString *fullKey, PRBool *collision)
{
  LOG(("nsOfflineCacheDevice::FindEntry [key=%s]\n", fullKey->get()));

  

  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(fullKey, &cid, &key, keyBuf))
    return nsnull;

  AutoResetStatement statement(mStatement_FindEntry);

  nsresult rv;
  rv  = statement->BindUTF8StringParameter(0, nsDependentCString(cid));
  rv |= statement->BindUTF8StringParameter(1, nsDependentCString(key));
  NS_ENSURE_SUCCESS(rv, nsnull);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  if (NS_FAILED(rv) || !hasRows)
    return nsnull; 

  nsOfflineCacheRecord rec;
  statement->GetSharedBlob(0, &rec.metaDataLen,
                           (const PRUint8 **) &rec.metaData);
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

  nsCacheEntry *entry = CreateCacheEntry(this, fullKey, rec);

  if (entry)
  {
    
    nsOfflineCacheBinding *binding = (nsOfflineCacheBinding*)entry->Data();
    PRBool isFile;
    rv = binding->mDataFile->IsFile(&isFile);
    if (NS_FAILED(rv) || !isFile)
    {
      DeleteEntry(entry, PR_FALSE);
      delete entry;
      return nsnull;
    }

    statement->Reset();

    
    AutoResetStatement updateStatement(mStatement_UpdateEntryFlags);
    rec.flags |= 0x1;
    rv |= updateStatement->BindInt32Parameter(0, rec.flags);
    rv |= updateStatement->BindUTF8StringParameter(1, nsDependentCString(cid));
    rv |= updateStatement->BindUTF8StringParameter(2, nsDependentCString(key));
    if (NS_FAILED(rv))
    {
      delete entry;
      return nsnull;
    }

    rv = updateStatement->ExecuteStep(&hasRows);
    if (NS_FAILED(rv))
    {
      delete entry;
      return nsnull;
    }

    NS_ASSERTION(!hasRows, "UPDATE should not result in output");
  }

  return entry;
}

nsresult
nsOfflineCacheDevice::DeactivateEntry(nsCacheEntry *entry)
{
  LOG(("nsOfflineCacheDevice::DeactivateEntry [key=%s]\n",
       entry->Key()->get()));

  
  
  

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
nsOfflineCacheDevice::BindEntry(nsCacheEntry *entry)
{
  LOG(("nsOfflineCacheDevice::BindEntry [key=%s]\n", entry->Key()->get()));

  NS_ENSURE_STATE(!entry->Data());

  
  
  

  

  
  
  

  
  nsCAutoString keyBuf;
  const char *cid, *key;
  if (!DecomposeCacheEntryKey(entry->Key(), &cid, &key, keyBuf))
    return NS_ERROR_UNEXPECTED;

  
  nsRefPtr<nsOfflineCacheBinding> binding =
      nsOfflineCacheBinding::Create(mCacheDirectory, entry->Key(), -1);
  if (!binding)
    return NS_ERROR_OUT_OF_MEMORY;

  nsOfflineCacheRecord rec;
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
  rv  = statement->BindUTF8StringParameter(0, nsDependentCString(rec.clientID));
  rv |= statement->BindUTF8StringParameter(1, nsDependentCString(rec.key));
  rv |= statement->BindBlobParameter(2, rec.metaData, rec.metaDataLen);
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
nsOfflineCacheDevice::DoomEntry(nsCacheEntry *entry)
{
  LOG(("nsOfflineCacheDevice::DoomEntry [key=%s]\n", entry->Key()->get()));

  
  

  
  
  
  DeleteEntry(entry, PR_FALSE);
}

nsresult
nsOfflineCacheDevice::OpenInputStreamForEntry(nsCacheEntry      *entry,
                                              nsCacheAccessMode  mode,
                                              PRUint32           offset,
                                              nsIInputStream   **result)
{
  LOG(("nsOfflineCacheDevice::OpenInputStreamForEntry [key=%s]\n",
       entry->Key()->get()));

  *result = nsnull;

  NS_ENSURE_TRUE(offset < entry->DataSize(), NS_ERROR_INVALID_ARG);

  
  

  nsOfflineCacheBinding *binding = (nsOfflineCacheBinding *) entry->Data();
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
nsOfflineCacheDevice::OpenOutputStreamForEntry(nsCacheEntry       *entry,
                                               nsCacheAccessMode   mode,
                                               PRUint32            offset,
                                               nsIOutputStream   **result)
{
  LOG(("nsOfflineCacheDevice::OpenOutputStreamForEntry [key=%s]\n",
       entry->Key()->get()));

  *result = nsnull;

  NS_ENSURE_TRUE(offset <= entry->DataSize(), NS_ERROR_INVALID_ARG);

  
  

  nsOfflineCacheBinding *binding = (nsOfflineCacheBinding *) entry->Data();
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
nsOfflineCacheDevice::GetFileForEntry(nsCacheEntry *entry, nsIFile **result)
{
  LOG(("nsOfflineCacheDevice::GetFileForEntry [key=%s]\n",
       entry->Key()->get()));

  nsOfflineCacheBinding *binding = (nsOfflineCacheBinding *) entry->Data();
  NS_ENSURE_STATE(binding);

  NS_IF_ADDREF(*result = binding->mDataFile);
  return NS_OK;
}

nsresult
nsOfflineCacheDevice::OnDataSizeChange(nsCacheEntry *entry, PRInt32 deltaSize)
{
  LOG(("nsOfflineCacheDevice::OnDataSizeChange [key=%s delta=%d]\n",
      entry->Key()->get(), deltaSize));

  const PRInt32 DELTA_THRESHOLD = 1<<14; 

  
  

  PRUint32 oldSize = entry->DataSize();
  NS_ASSERTION(deltaSize >= 0 || PRInt32(oldSize) + deltaSize >= 0, "oops");
  PRUint32 newSize = PRInt32(oldSize) + deltaSize;
  UpdateEntrySize(entry, newSize);

  mDeltaCounter += deltaSize; 

  if (mDeltaCounter >= DELTA_THRESHOLD)
  {
    if (CacheSize() > mCacheCapacity) {
      
      
#ifdef DEBUG
      nsresult rv =
#endif
        nsCacheService::DoomEntry(entry);
      NS_ASSERTION(NS_SUCCEEDED(rv), "DoomEntry() failed.");
      return NS_ERROR_ABORT;
    }

    mDeltaCounter = 0; 
  }

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::Visit(nsICacheVisitor *visitor)
{
  NS_ENSURE_TRUE(Initialized(), NS_ERROR_NOT_INITIALIZED);

  

  nsCOMPtr<nsICacheDeviceInfo> deviceInfo =
      new nsOfflineCacheDeviceInfo(this);

  PRBool keepGoing;
  nsresult rv = visitor->VisitDevice(OFFLINE_CACHE_DEVICE_ID, deviceInfo,
                                     &keepGoing);
  if (NS_FAILED(rv))
    return rv;
  
  if (!keepGoing)
    return NS_OK;

  

  nsOfflineCacheRecord rec;
  nsRefPtr<nsOfflineCacheEntryInfo> info = new nsOfflineCacheEntryInfo;
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

    statement->GetSharedUTF8String(0, NULL, &rec.clientID);
    statement->GetSharedUTF8String(1, NULL, &rec.key);
    statement->GetSharedBlob(2, &rec.metaDataLen,
                             (const PRUint8 **) &rec.metaData);
    rec.generation     = statement->AsInt32(3);
    rec.flags          = statement->AsInt32(4);
    rec.dataSize       = statement->AsInt32(5);
    rec.fetchCount     = statement->AsInt32(6);
    rec.lastFetched    = statement->AsInt64(7);
    rec.lastModified   = statement->AsInt64(8);
    rec.expirationTime = statement->AsInt64(9);

    PRBool keepGoing;
    rv = visitor->VisitEntry(OFFLINE_CACHE_DEVICE_ID, info, &keepGoing);
    if (NS_FAILED(rv) || !keepGoing)
      break;
  }

  info->mRec = nsnull;
  return NS_OK;
}

nsresult
nsOfflineCacheDevice::EvictEntries(const char *clientID)
{
  LOG(("nsOfflineCacheDevice::EvictEntries [cid=%s]\n",
       clientID ? clientID : ""));

  

  
  
  EvictionObserver evictionObserver(mDB, mEvictionFunction);

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv;
  if (clientID)
  {
    rv = mDB->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_cache WHERE ClientID=? AND Flags = 0;"),
                              getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = statement->BindUTF8StringParameter(0, nsDependentCString(clientID));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    rv = mDB->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_cache WHERE Flags = 0;"),
                              getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  evictionObserver.Apply();

  statement = nsnull;
  
  if (clientID)
  {
    rv = mDB->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_cache_namespaces WHERE ClientID=?"),
                              getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = statement->BindUTF8StringParameter(0, nsDependentCString(clientID));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    rv = mDB->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_cache_namespaces;"),
                              getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::MarkEntry(const nsCString &clientID,
                                const nsACString &key,
                                PRUint32 typeBits)
{
  LOG(("nsOfflineCacheDevice::MarkEntry [cid=%s, key=%s, typeBits=%d]\n",
       clientID.get(), PromiseFlatCString(key).get(), typeBits));

  AutoResetStatement statement(mStatement_MarkEntry);
  nsresult rv = statement->BindInt32Parameter(0, typeBits);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(1, clientID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(2, key);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::UnmarkEntry(const nsCString &clientID,
                                  const nsACString &key,
                                  PRUint32 typeBits)
{
  LOG(("nsOfflineCacheDevice::UnmarkEntry [cid=%s, key=%s, typeBits=%d]\n",
       clientID.get(), PromiseFlatCString(key).get(), typeBits));

  AutoResetStatement statement(mStatement_UnmarkEntry);
  nsresult rv = statement->BindInt32Parameter(0, typeBits);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(1, clientID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(2, key);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  

  EvictionObserver evictionObserver(mDB, mEvictionFunction);

  AutoResetStatement cleanupStatement(mStatement_CleanupUnmarked);
  rv = cleanupStatement->BindUTF8StringParameter(0, clientID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cleanupStatement->BindUTF8StringParameter(1, key);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = cleanupStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  evictionObserver.Apply();

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::GetMatchingNamespace(const nsCString &clientID,
                                           const nsACString &key,
                                           nsIApplicationCacheNamespace **out)
{
  LOG(("nsOfflineCacheDevice::GetMatchingNamespace [cid=%s, key=%s]\n",
       clientID.get(), PromiseFlatCString(key).get()));

  nsresult rv;

  AutoResetStatement statement(mStatement_FindNamespaceEntry);

  rv = statement->BindUTF8StringParameter(0, clientID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(1, key);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  *out = nsnull;

  while (hasRows)
  {
    nsCString namespaceSpec;
    rv = statement->GetUTF8String(0, namespaceSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString data;
    rv = statement->GetUTF8String(1, data);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 itemType;
    rv = statement->GetInt32(2, &itemType);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    if ((itemType & nsIApplicationCacheNamespace::NAMESPACE_BYPASS) &&
        (namespaceSpec != key)) {
      rv = statement->ExecuteStep(&hasRows);
      NS_ENSURE_SUCCESS(rv, rv);

      continue;
    }

    nsCOMPtr<nsIApplicationCacheNamespace> ns =
      new nsApplicationCacheNamespace();
    if (!ns)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = ns->Init(itemType, namespaceSpec, data);
    NS_ENSURE_SUCCESS(rv, rv);

    ns.swap(*out);
    return NS_OK;
  }

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::CacheOpportunistically(const nsCString &clientID,
                                             const nsACString &key)
{
  
  

  return MarkEntry(clientID, key, nsIApplicationCache::ITEM_OPPORTUNISTIC);
}

nsresult
nsOfflineCacheDevice::GetTypes(const nsCString &clientID,
                               const nsACString &key,
                               PRUint32 *typeBits)
{
  LOG(("nsOfflineCacheDevice::GetTypes [cid=%s, key=%s]\n",
       clientID.get(), PromiseFlatCString(key).get()));

  AutoResetStatement statement(mStatement_GetTypes);
  nsresult rv = statement->BindUTF8StringParameter(0, clientID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(1, key);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasRows)
    return NS_ERROR_CACHE_KEY_NOT_FOUND;

  *typeBits = statement->AsInt32(0);

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::GatherEntries(const nsCString &clientID,
                                    PRUint32 typeBits,
                                    PRUint32 *count,
                                    char ***keys)
{
  LOG(("nsOfflineCacheDevice::GatherEntries [cid=%s, typeBits=%X]\n",
       clientID.get(), typeBits));

  AutoResetStatement statement(mStatement_GatherEntries);
  nsresult rv = statement->BindUTF8StringParameter(0, clientID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt32Parameter(1, typeBits);
  NS_ENSURE_SUCCESS(rv, rv);

  return RunSimpleQuery(mStatement_GatherEntries, 0, count, keys);
}

nsresult
nsOfflineCacheDevice::AddNamespace(const nsCString &clientID,
                                   nsIApplicationCacheNamespace *ns)
{
  nsCString namespaceSpec;
  nsresult rv = ns->GetNamespaceSpec(namespaceSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString data;
  rv = ns->GetData(data);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 itemType;
  rv = ns->GetItemType(&itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("nsOfflineCacheDevice::AddNamespace [cid=%s, ns=%s, data=%s, type=%d]",
       PromiseFlatCString(clientID).get(),
       namespaceSpec.get(), data.get(), itemType));

  AutoResetStatement statement(mStatement_InsertNamespaceEntry);

  rv = statement->BindUTF8StringParameter(0, clientID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindUTF8StringParameter(1, namespaceSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindUTF8StringParameter(2, data);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt32Parameter(3, itemType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsOfflineCacheDevice::RunSimpleQuery(mozIStorageStatement * statement,
                                     PRUint32 resultIndex,
                                     PRUint32 * count,
                                     char *** values)
{
  PRBool hasRows;
  nsresult rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<nsCString> valArray;
  while (hasRows)
  {
    PRUint32 length;
    valArray.AppendElement(
      nsDependentCString(statement->AsSharedUTF8String(resultIndex, &length)));

    rv = statement->ExecuteStep(&hasRows);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *count = valArray.Length();
  char **ret = static_cast<char **>(NS_Alloc(*count * sizeof(char*)));
  if (!ret) return NS_ERROR_OUT_OF_MEMORY;

  for (PRUint32 i = 0; i <  *count; i++) {
    ret[i] = NS_strdup(valArray[i].get());
    if (!ret[i]) {
      NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(i, ret);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *values = ret;

  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheDevice::CreateApplicationCache(const nsACString &group,
                                             nsIApplicationCache **out)
{
  *out = nsnull;

  nsCString clientID;
  
  
  if (!NS_Escape(nsCString(group), clientID, url_Path)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRTime now = PR_Now();

  
  
  clientID.Append(nsPrintfCString(64, "|%016lld|%d",
                                  now / PR_USEC_PER_SEC,
                                  gNextTemporaryClientID++));

  nsCOMPtr<nsIApplicationCache> cache = new nsApplicationCache(this,
                                                               group,
                                                               clientID);
  if (!cache)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(cache);
  if (!weak)
    return NS_ERROR_OUT_OF_MEMORY;

  mCaches.Put(clientID, weak);

  cache.swap(*out);

  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheDevice::GetApplicationCache(const nsACString &clientID,
                                          nsIApplicationCache **out)
{
  *out = nsnull;

  nsCOMPtr<nsIApplicationCache> cache;

  nsWeakPtr weak;
  if (mCaches.Get(clientID, getter_AddRefs(weak)))
    cache = do_QueryReferent(weak);

  if (!cache)
  {
    nsCString group;
    nsresult rv = GetGroupForCache(clientID, group);
    NS_ENSURE_SUCCESS(rv, rv);

    if (group.IsEmpty()) {
      return NS_OK;
    }

    cache = new nsApplicationCache(this, group, clientID);
    weak = do_GetWeakReference(cache);
    if (!weak)
      return NS_ERROR_OUT_OF_MEMORY;

    mCaches.Put(clientID, weak);
  }

  cache.swap(*out);

  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheDevice::GetActiveCache(const nsACString &group,
                                     nsIApplicationCache **out)
{
  *out = nsnull;

  nsCString *clientID;
  if (mActiveCachesByGroup.Get(group, &clientID))
    return GetApplicationCache(*clientID, out);

  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheDevice::DeactivateGroup(const nsACString &group)
{
  nsCString *active = nsnull;

  AutoResetStatement statement(mStatement_DeactivateGroup);
  nsresult rv = statement->BindUTF8StringParameter(0, group);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  if (mActiveCachesByGroup.Get(group, &active))
  {
    mActiveCaches.Remove(*active);
    mActiveCachesByGroup.Remove(group);
    active = nsnull;
  }

  return NS_OK;
}

PRBool
nsOfflineCacheDevice::CanUseCache(nsIURI *keyURI, const nsCString &clientID)
{
  if (mActiveCaches.Contains(clientID)) {
    nsCAutoString groupID;
    nsresult rv = GetGroupForCache(clientID, groupID);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    nsCOMPtr<nsIURI> groupURI;
    rv = NS_NewURI(getter_AddRefs(groupURI), groupID);
    if (NS_SUCCEEDED(rv)) {
      
      
      
      
      
      
      if (NS_SecurityCompareURIs(keyURI, groupURI,
                                 GetStrictFileOriginPolicy()))
        return PR_TRUE;
    }
  }

  return PR_FALSE;
}


NS_IMETHODIMP
nsOfflineCacheDevice::ChooseApplicationCache(const nsACString &key,
                                             nsIApplicationCache **out)
{
  *out = nsnull;

  nsCOMPtr<nsIURI> keyURI;
  nsresult rv = NS_NewURI(getter_AddRefs(keyURI), key);
  NS_ENSURE_SUCCESS(rv, rv);

  
  AutoResetStatement statement(mStatement_FindClient);
  rv = statement->BindUTF8StringParameter(0, key);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasRows;
  rv = statement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  while (hasRows) {
    PRInt32 itemType;
    rv = statement->GetInt32(1, &itemType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!(itemType & nsIApplicationCache::ITEM_FOREIGN)) {
      nsCAutoString clientID;
      rv = statement->GetUTF8String(0, clientID);
      NS_ENSURE_SUCCESS(rv, rv);

      if (CanUseCache(keyURI, clientID)) {
        return GetApplicationCache(clientID, out);
      }
    }

    rv = statement->ExecuteStep(&hasRows);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  

  AutoResetStatement nsstatement(mStatement_FindClientByNamespace);

  rv = nsstatement->BindUTF8StringParameter(0, key);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = nsstatement->ExecuteStep(&hasRows);
  NS_ENSURE_SUCCESS(rv, rv);

  while (hasRows)
  {
    PRInt32 itemType;
    rv = nsstatement->GetInt32(1, &itemType);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!(itemType & nsIApplicationCacheNamespace::NAMESPACE_BYPASS)) {
      nsCAutoString clientID;
      rv = nsstatement->GetUTF8String(0, clientID);
      NS_ENSURE_SUCCESS(rv, rv);

      if (CanUseCache(keyURI, clientID)) {
        return GetApplicationCache(clientID, out);
      }
    }

    rv = nsstatement->ExecuteStep(&hasRows);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsOfflineCacheDevice::CacheOpportunistically(nsIApplicationCache* cache,
                                             const nsACString &key)
{
  NS_ENSURE_ARG_POINTER(cache);

  nsresult rv;

  nsCAutoString clientID;
  rv = cache->GetClientID(clientID);
  NS_ENSURE_SUCCESS(rv, rv);

  return CacheOpportunistically(clientID, key);
}

nsresult
nsOfflineCacheDevice::ActivateCache(const nsCSubstring &group,
                                    const nsCSubstring &clientID)
{
  AutoResetStatement statement(mStatement_ActivateClient);
  nsresult rv = statement->BindUTF8StringParameter(0, group);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(1, clientID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt32Parameter(2, SecondsFromPRTime(PR_Now()));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString *active;
  if (mActiveCachesByGroup.Get(group, &active))
  {
    mActiveCaches.Remove(*active);
    mActiveCachesByGroup.Remove(group);
    active = nsnull;
  }

  if (!clientID.IsEmpty())
  {
    mActiveCaches.Put(clientID);
    mActiveCachesByGroup.Put(group, new nsCString(clientID));
  }

  return NS_OK;
}

PRBool
nsOfflineCacheDevice::IsActiveCache(const nsCSubstring &group,
                                    const nsCSubstring &clientID)
{
  nsCString *active = nsnull;
  return mActiveCachesByGroup.Get(group, &active) && *active == clientID;
}

nsresult
nsOfflineCacheDevice::GetGroupForCache(const nsACString &clientID,
                                       nsCString &out)
{
  out.Assign(clientID);
  out.Truncate(out.FindChar('|'));
  NS_UnescapeURL(out);

  return NS_OK;
}





void
nsOfflineCacheDevice::SetCacheParentDirectory(nsILocalFile *parentDir)
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
  rv = dir->AppendNative(NS_LITERAL_CSTRING("OfflineCache"));
  if (NS_FAILED(rv))
    return;

  mCacheDirectory = do_QueryInterface(dir);
}

void
nsOfflineCacheDevice::SetCapacity(PRUint32 capacity)
{
  mCacheCapacity = capacity * 1024;
}
