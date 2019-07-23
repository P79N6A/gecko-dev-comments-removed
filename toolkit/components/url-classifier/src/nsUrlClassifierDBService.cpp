







































#include "nsCOMPtr.h"
#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozStorageHelper.h"
#include "mozStorageCID.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsAutoLock.h"
#include "nsCRT.h"
#include "nsICryptoHash.h"
#include "nsIDirectoryService.h"
#include "nsIObserverService.h"
#include "nsIProperties.h"
#include "nsIProxyObjectManager.h"
#include "nsToolkitCompsCID.h"
#include "nsIUrlClassifierUtils.h"
#include "nsUrlClassifierDBService.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsVoidArray.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsXPCOMStrings.h"
#include "blapi.h"
#include "prlog.h"
#include "prlock.h"
#include "prprf.h"
#include "zlib.h"

































#if defined(PR_LOGGING)
static const PRLogModuleInfo *gUrlClassifierDbServiceLog = nsnull;
#define LOG(args) PR_LOG(gUrlClassifierDbServiceLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUrlClassifierDbServiceLog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (PR_FALSE)
#endif


#define DATABASE_FILENAME "urlclassifier3.sqlite"

#define MAX_HOST_COMPONENTS 5
#define MAX_PATH_COMPONENTS 4


#define MAX_CHUNK_SIZE (1024 * 1024)

#define KEY_LENGTH 16


static nsUrlClassifierDBService* sUrlClassifierDBService;


static nsIThread* gDbBackgroundThread = nsnull;



static PRBool gShuttingDownThread = PR_FALSE;






struct nsUrlClassifierHash
{
  PRUint8 buf[KEY_LENGTH];

  nsresult FromPlaintext(const nsACString& plainText);
  void Assign(const nsACString& str);

  const PRBool operator==(const nsUrlClassifierHash& hash) const {
    return (memcmp(buf, hash.buf, sizeof(buf)) == 0);
  }
};

nsresult
nsUrlClassifierHash::FromPlaintext(const nsACString& plainText)
{
  
  
  

  unsigned char sha256buf[SHA256_LENGTH];
  if (SHA256_HashBuf(sha256buf,
                     reinterpret_cast<const PRUint8*>(plainText.BeginReading()),
                     plainText.Length()) != SECSuccess) {
    return NS_ERROR_FAILURE;
  }

  memcpy(buf, sha256buf, KEY_LENGTH);

  return NS_OK;
}

void
nsUrlClassifierHash::Assign(const nsACString& str)
{
  NS_ASSERTION(str.Length() >= KEY_LENGTH,
               "string must be at least KEY_LENGTH characters long");
  memcpy(buf, str.BeginReading(), KEY_LENGTH);
}






class nsUrlClassifierEntry
{
public:
  nsUrlClassifierEntry() : mId(0) {}
  ~nsUrlClassifierEntry() {}

  
  PRBool ReadStatement(mozIStorageStatement* statement);

  
  nsresult BindStatement(mozIStorageStatement* statement);

  
  PRBool AddFragment(const nsUrlClassifierHash& hash, PRUint32 chunkNum);

  
  PRBool Merge(const nsUrlClassifierEntry& entry);

  
  PRBool SubtractFragments(const nsUrlClassifierEntry& entry);

  
  PRBool SubtractChunk(PRUint32 chunkNum);

  
  PRBool HasFragment(const nsUrlClassifierHash& hash);

  
  void Clear();

  PRBool IsEmpty() { return mFragments.Length() == 0; }

  nsUrlClassifierHash mKey;
  PRUint32 mId;
  PRUint32 mTableId;

private:
  
  PRBool AddFragments(const PRUint8* blob, PRUint32 blobLength);

  
  struct Fragment {
    nsUrlClassifierHash hash;
    PRUint32 chunkNum;

    PRInt32 Diff(const Fragment& fragment) const {
      PRInt32 cmp = memcmp(hash.buf, fragment.hash.buf, sizeof(hash.buf));
      if (cmp != 0) return cmp;
      return chunkNum - fragment.chunkNum;
    }

    PRBool operator==(const Fragment& fragment) const {
      return (Diff(fragment) == 0);
    }

    PRBool operator<(const Fragment& fragment) const {
      return (Diff(fragment) < 0);
    }
  };

  nsTArray<Fragment> mFragments;
};

PRBool
nsUrlClassifierEntry::ReadStatement(mozIStorageStatement* statement)
{
  mId = statement->AsInt32(0);

  PRUint32 size;
  const PRUint8* blob = statement->AsSharedBlob(1, &size);
  if (!blob || (size != KEY_LENGTH))
    return PR_FALSE;
  memcpy(mKey.buf, blob, KEY_LENGTH);

  blob = statement->AsSharedBlob(2, &size);
  if (!AddFragments(blob, size))
    return PR_FALSE;

  mTableId = statement->AsInt32(3);

  return PR_TRUE;
}

nsresult
nsUrlClassifierEntry::BindStatement(mozIStorageStatement* statement)
{
  nsresult rv;

  if (mId == 0)
    rv = statement->BindNullParameter(0);
  else
    rv = statement->BindInt32Parameter(0, mId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindBlobParameter(1, mKey.buf, KEY_LENGTH);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = statement->BindBlobParameter
    (2, reinterpret_cast<PRUint8*>(mFragments.Elements()),
       mFragments.Length() * sizeof(Fragment));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt32Parameter(3, mTableId);
  NS_ENSURE_SUCCESS(rv, rv);

  return PR_TRUE;
}

PRBool
nsUrlClassifierEntry::AddFragment(const nsUrlClassifierHash& hash,
                                  PRUint32 chunkNum)
{
  Fragment* fragment = mFragments.AppendElement();
  if (!fragment)
    return PR_FALSE;

  fragment->hash = hash;
  fragment->chunkNum = chunkNum;

  return PR_TRUE;
}

PRBool
nsUrlClassifierEntry::AddFragments(const PRUint8* blob, PRUint32 blobLength)
{
  NS_ASSERTION(blobLength % sizeof(Fragment) == 0,
               "Fragment blob not the right length");
  Fragment* fragment = mFragments.AppendElements
    (reinterpret_cast<const Fragment*>(blob), blobLength / sizeof(Fragment));
  return (fragment != nsnull);
}

PRBool
nsUrlClassifierEntry::Merge(const nsUrlClassifierEntry& entry)
{
  Fragment* fragment = mFragments.AppendElements(entry.mFragments);
  return (fragment != nsnull);
}

PRBool
nsUrlClassifierEntry::SubtractFragments(const nsUrlClassifierEntry& entry)
{
  for (PRUint32 i = 0; i < entry.mFragments.Length(); i++) {
    for (PRUint32 j = 0; j < mFragments.Length(); j++) {
      if (mFragments[j].hash == entry.mFragments[i].hash) {
        mFragments.RemoveElementAt(j);
        break;
      }
    }
  }

  return PR_TRUE;
}

PRBool
nsUrlClassifierEntry::SubtractChunk(PRUint32 chunkNum)
{
  PRUint32 i = 0;
  while (i < mFragments.Length()) {
    if (mFragments[i].chunkNum == chunkNum)
      mFragments.RemoveElementAt(i);
    else
      i++;
  }

  return PR_TRUE;
}

PRBool
nsUrlClassifierEntry::HasFragment(const nsUrlClassifierHash& hash)
{
  for (PRUint32 i = 0; i < mFragments.Length(); i++) {
    const Fragment& fragment = mFragments[i];
    if (fragment.hash == hash)
      return PR_TRUE;
  }

  return PR_FALSE;
}

void
nsUrlClassifierEntry::Clear()
{
  mId = 0;
  mFragments.Clear();
}



class nsUrlClassifierDBServiceWorker : public nsIUrlClassifierDBServiceWorker
{
public:
  nsUrlClassifierDBServiceWorker();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIURLCLASSIFIERDBSERVICEWORKER

  
  nsresult Init();

  
  nsresult QueueLookup(const nsACString& lookupKey,
                       nsIUrlClassifierCallback* callback);

private:
  
  ~nsUrlClassifierDBServiceWorker();

  
  nsUrlClassifierDBServiceWorker(nsUrlClassifierDBServiceWorker&);

  
  nsresult OpenDb();

  
  nsresult MaybeCreateTables(mozIStorageConnection* connection);

  nsresult GetTableName(PRUint32 tableId, nsACString& table);
  nsresult GetTableId(const nsACString& table, PRUint32* tableId);

  
  nsresult ReadEntry(const nsUrlClassifierHash& key,
                     PRUint32 tableId,
                     nsUrlClassifierEntry& entry);

  
  nsresult ReadEntry(PRUint32 id, nsUrlClassifierEntry& entry);

  
  nsresult DeleteEntry(nsUrlClassifierEntry& entry);

  
  nsresult WriteEntry(nsUrlClassifierEntry& entry);

  
  nsresult InflateChunk(nsACString& chunk);

  
  nsresult GetChunkEntries(const nsACString& table,
                           PRUint32 tableId,
                           PRUint32 chunkNum,
                           nsACString& chunk,
                           nsTArray<nsUrlClassifierEntry>& entries);

  
  nsresult ParseChunkList(const nsACString& chunkStr,
                          nsTArray<PRUint32>& chunks);

  
  nsresult JoinChunkList(nsTArray<PRUint32>& chunks, nsCString& chunkStr);

  
  nsresult GetChunkLists(PRUint32 tableId,
                         nsACString& addChunks,
                         nsACString& subChunks);

  
  nsresult SetChunkLists(PRUint32 tableId,
                         const nsACString& addChunks,
                         const nsACString& subChunks);

  
  
  nsresult AddChunk(PRUint32 tableId, PRUint32 chunkNum,
                    nsTArray<nsUrlClassifierEntry>& entries);

  
  nsresult ExpireAdd(PRUint32 tableId, PRUint32 chunkNum);

  
  nsresult SubChunk(PRUint32 tableId, PRUint32 chunkNum,
                    nsTArray<nsUrlClassifierEntry>& entries);

  
  nsresult ExpireSub(PRUint32 tableId, PRUint32 chunkNum);

  
  nsresult ProcessResponseLines(PRBool* done);
  
  nsresult ProcessChunk(PRBool* done);

  
  void ResetUpdate();

  
  
  
  nsresult GetLookupFragments(const nsCSubstring& spec,
                              nsTArray<nsUrlClassifierHash>& fragments);

  
  
  
  
  
  nsresult GetKey(const nsACString& spec, nsUrlClassifierHash& hash);

  
  
  nsresult CheckKey(const nsCSubstring& spec,
                    const nsUrlClassifierHash& key,
                    nsTArray<PRUint32>& tables);

  
  nsresult DoLookup(const nsACString& spec, nsIUrlClassifierCallback* c);

  
  
  nsresult HandlePendingLookups();

  nsCOMPtr<nsIFile> mDBFile;

  
  
  
  nsCOMPtr<mozIStorageConnection> mConnection;

  nsCOMPtr<mozIStorageStatement> mLookupStatement;
  nsCOMPtr<mozIStorageStatement> mLookupWithTableStatement;
  nsCOMPtr<mozIStorageStatement> mLookupWithIDStatement;

  nsCOMPtr<mozIStorageStatement> mUpdateStatement;
  nsCOMPtr<mozIStorageStatement> mDeleteStatement;

  nsCOMPtr<mozIStorageStatement> mAddChunkEntriesStatement;
  nsCOMPtr<mozIStorageStatement> mGetChunkEntriesStatement;
  nsCOMPtr<mozIStorageStatement> mDeleteChunkEntriesStatement;

  nsCOMPtr<mozIStorageStatement> mGetChunkListsStatement;
  nsCOMPtr<mozIStorageStatement> mSetChunkListsStatement;

  nsCOMPtr<mozIStorageStatement> mGetTablesStatement;
  nsCOMPtr<mozIStorageStatement> mGetTableIdStatement;
  nsCOMPtr<mozIStorageStatement> mGetTableNameStatement;
  nsCOMPtr<mozIStorageStatement> mInsertTableIdStatement;

  
  
  nsCString mPendingStreamUpdate;

  PRInt32 mUpdateWait;

  enum {
    STATE_LINE,
    STATE_CHUNK,
  } mState;

  enum {
    CHUNK_ADD,
    CHUNK_SUB,
  } mChunkType;

  PRUint32 mChunkNum;
  PRUint32 mChunkLen;

  nsCString mUpdateTable;
  PRUint32 mUpdateTableId;

  nsresult mUpdateStatus;

  
  
  PRLock* mPendingLookupLock;

  class PendingLookup {
  public:
    nsCString mKey;
    nsCOMPtr<nsIUrlClassifierCallback> mCallback;
  };

  
  nsTArray<PendingLookup> mPendingLookups;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUrlClassifierDBServiceWorker,
                              nsIUrlClassifierDBServiceWorker)

nsUrlClassifierDBServiceWorker::nsUrlClassifierDBServiceWorker()
  : mUpdateStatus(NS_OK)
  , mPendingLookupLock(nsnull)
{
}

nsUrlClassifierDBServiceWorker::~nsUrlClassifierDBServiceWorker()
{
  NS_ASSERTION(!mConnection,
               "Db connection not closed, leaking memory!  Call CloseDb "
               "to close the connection.");
  if (mPendingLookupLock)
    PR_DestroyLock(mPendingLookupLock);
}

nsresult
nsUrlClassifierDBServiceWorker::Init()
{
  

  
  
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                       getter_AddRefs(mDBFile));
  if (NS_FAILED(rv)) return rv;

  rv = mDBFile->Append(NS_LITERAL_STRING(DATABASE_FILENAME));
  NS_ENSURE_SUCCESS(rv, rv);

  mPendingLookupLock = PR_NewLock();
  if (!mPendingLookupLock)
    return NS_ERROR_OUT_OF_MEMORY;

  ResetUpdate();

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::QueueLookup(const nsACString& spec,
                                            nsIUrlClassifierCallback* callback)
{
  nsAutoLock lock(mPendingLookupLock);

  PendingLookup* lookup = mPendingLookups.AppendElement();
  if (!lookup) return NS_ERROR_OUT_OF_MEMORY;

  lookup->mKey = spec;
  lookup->mCallback = callback;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::GetLookupFragments(const nsACString& spec,
                                                   nsTArray<nsUrlClassifierHash>& fragments)
{
  fragments.Clear();

  nsACString::const_iterator begin, end, iter;
  spec.BeginReading(begin);
  spec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
    return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter++);
  const nsCSubstring& path = Substring(iter, end);

  








  nsCStringArray hosts;
  hosts.AppendCString(host);

  host.BeginReading(begin);
  host.EndReading(end);
  int numComponents = 0;
  while (RFindInReadable(NS_LITERAL_CSTRING("."), begin, end) &&
         numComponents < MAX_HOST_COMPONENTS) {
    
    if (++numComponents >= 2) {
      host.EndReading(iter);
      hosts.AppendCString(Substring(end, iter));
    }
    end = begin;
    host.BeginReading(begin);
  }

  










  nsCStringArray paths;
  paths.AppendCString(path);

  numComponents = 0;
  path.BeginReading(begin);
  path.EndReading(end);
  iter = begin;
  while (FindCharInReadable('/', iter, end) &&
         numComponents < MAX_PATH_COMPONENTS) {
    iter++;
    paths.AppendCString(Substring(begin, iter));
    numComponents++;
  }

  


  nsCAutoString key;
  key.Assign(spec);
  key.Append('$');
  LOG(("Chking %s", key.get()));

  nsUrlClassifierHash* hash = fragments.AppendElement();
  if (!hash) return NS_ERROR_OUT_OF_MEMORY;
  hash->FromPlaintext(key);

  for (int hostIndex = 0; hostIndex < hosts.Count(); hostIndex++) {
    for (int pathIndex = 0; pathIndex < paths.Count(); pathIndex++) {
      key.Assign(*hosts[hostIndex]);
      key.Append('/');
      key.Append(*paths[pathIndex]);
      LOG(("Chking %s", key.get()));

      hash = fragments.AppendElement();
      if (!hash) return NS_ERROR_OUT_OF_MEMORY;
      hash->FromPlaintext(key);
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::CheckKey(const nsACString& spec,
                                         const nsUrlClassifierHash& hash,
                                         nsTArray<PRUint32>& tables)
{
  mozStorageStatementScoper lookupScoper(mLookupStatement);

  nsresult rv = mLookupStatement->BindBlobParameter
    (0, hash.buf, KEY_LENGTH);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<nsUrlClassifierHash> fragments;
  PRBool haveFragments = PR_FALSE;

  PRBool exists;
  rv = mLookupStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  while (exists) {
    if (!haveFragments) {
      rv = GetLookupFragments(spec, fragments);
      NS_ENSURE_SUCCESS(rv, rv);
      haveFragments = PR_TRUE;
    }

    nsUrlClassifierEntry entry;
    if (!entry.ReadStatement(mLookupStatement))
      return NS_ERROR_FAILURE;

    for (PRUint32 i = 0; i < fragments.Length(); i++) {
      if (entry.HasFragment(fragments[i])) {
        tables.AppendElement(entry.mTableId);
        break;
      }
    }

    rv = mLookupStatement->ExecuteStep(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}












nsresult
nsUrlClassifierDBServiceWorker::DoLookup(const nsACString& spec,
                                         nsIUrlClassifierCallback* c)
{
  if (gShuttingDownThread) {
    c->HandleEvent(EmptyCString());
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    c->HandleEvent(EmptyCString());
    return NS_ERROR_FAILURE;
  }

#if defined(PR_LOGGING)
  PRIntervalTime clockStart = 0;
  if (LOG_ENABLED()) {
    clockStart = PR_IntervalNow();
  }
#endif

  nsACString::const_iterator begin, end, iter;
  spec.BeginReading(begin);
  spec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
    return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter++);
  nsCStringArray hostComponents;
  hostComponents.ParseString(PromiseFlatCString(host).get(), ".");

  if (hostComponents.Count() < 2) {
    
    c->HandleEvent(EmptyCString());
    return NS_OK;
  }

  
  PRInt32 last = hostComponents.Count() - 1;
  nsCAutoString lookupHost;
  lookupHost.Assign(*hostComponents[last - 1]);
  lookupHost.Append(".");
  lookupHost.Append(*hostComponents[last]);
  lookupHost.Append("/");
  nsUrlClassifierHash hash;
  hash.FromPlaintext(lookupHost);

  
  
  nsTArray<PRUint32> resultTables;
  CheckKey(spec, hash, resultTables);

  
  if (hostComponents.Count() > 2) {
    nsCAutoString lookupHost2;
    lookupHost2.Assign(*hostComponents[last - 2]);
    lookupHost2.Append(".");
    lookupHost2.Append(lookupHost);
    hash.FromPlaintext(lookupHost2);

    CheckKey(spec, hash, resultTables);
  }

  nsCAutoString result;
  for (PRUint32 i = 0; i < resultTables.Length(); i++) {
    nsCAutoString tableName;
    GetTableName(resultTables[i], tableName);

    
    
    if (!result.IsEmpty()) {
      result.Append(',');
    }
    result.Append(tableName);
  }

#if defined(PR_LOGGING)
  if (LOG_ENABLED()) {
    PRIntervalTime clockEnd = PR_IntervalNow();
    LOG(("query took %dms\n",
         PR_IntervalToMilliseconds(clockEnd - clockStart)));
  }
#endif

  c->HandleEvent(result);

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::HandlePendingLookups()
{
  nsAutoLock lock(mPendingLookupLock);
  while (mPendingLookups.Length() > 0) {
    PendingLookup lookup = mPendingLookups[0];
    mPendingLookups.RemoveElementAt(0);
    lock.unlock();

    DoLookup(lookup.mKey, lookup.mCallback);

    lock.lock();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Lookup(const nsACString& spec,
                                       nsIUrlClassifierCallback* c,
                                       PRBool needsProxy)
{
  return HandlePendingLookups();
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::GetTables(nsIUrlClassifierCallback* c)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  mozStorageStatementScoper scoper(mGetTablesStatement);

  nsCAutoString response;
  PRBool hasMore;
  while (NS_SUCCEEDED(rv = mGetTablesStatement->ExecuteStep(&hasMore)) &&
         hasMore) {
    nsCAutoString val;
    mGetTablesStatement->GetUTF8String(0, val);

    if (val.IsEmpty()) {
      continue;
    }

    response.Append(val);
    response.Append(';');

    mGetTablesStatement->GetUTF8String(1, val);

    if (!val.IsEmpty()) {
      response.Append("a:");
      response.Append(val);
    }

    mGetTablesStatement->GetUTF8String(2, val);
    if (!val.IsEmpty()) {
      response.Append("s:");
      response.Append(val);
    }

    response.Append('\n');
  }

  if (NS_FAILED(rv)) {
    response.Truncate();
  }

  c->HandleEvent(response);

  return rv;
}

nsresult
nsUrlClassifierDBServiceWorker::GetTableId(const nsACString& table,
                                           PRUint32* tableId)
{
  mozStorageStatementScoper findScoper(mGetTableIdStatement);

  nsresult rv = mGetTableIdStatement->BindUTF8StringParameter(0, table);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = mGetTableIdStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (exists) {
    *tableId = mGetTableIdStatement->AsInt32(0);
    return NS_OK;
  }

  mozStorageStatementScoper insertScoper(mInsertTableIdStatement);
  rv = mInsertTableIdStatement->BindUTF8StringParameter(0, table);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mInsertTableIdStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 rowId;
  rv = mConnection->GetLastInsertRowID(&rowId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowId > PR_UINT32_MAX)
    return NS_ERROR_FAILURE;

  *tableId = rowId;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::GetTableName(PRUint32 tableId,
                                             nsACString& tableName)
{
  mozStorageStatementScoper findScoper(mGetTableNameStatement);
  nsresult rv = mGetTableNameStatement->BindInt32Parameter(0, tableId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = mGetTableNameStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) return NS_ERROR_FAILURE;

  return mGetTableNameStatement->GetUTF8String(0, tableName);
}

nsresult
nsUrlClassifierDBServiceWorker::InflateChunk(nsACString& chunk)
{
  nsCAutoString inflated;
  char buf[4096];

  const nsPromiseFlatCString& flat = PromiseFlatCString(chunk);

  z_stream stream;
  memset(&stream, 0, sizeof(stream));
  stream.next_in = (Bytef*)flat.get();
  stream.avail_in = flat.Length();

  if (inflateInit(&stream) != Z_OK) {
    return NS_ERROR_FAILURE;
  }

  int code;
  do {
    stream.next_out = (Bytef*)buf;
    stream.avail_out = sizeof(buf);

    code = inflate(&stream, Z_NO_FLUSH);
    PRUint32 numRead = sizeof(buf) - stream.avail_out;

    if (code == Z_OK || code == Z_STREAM_END) {
      inflated.Append(buf, numRead);
    }
  } while (code == Z_OK);

  inflateEnd(&stream);

  if (code != Z_STREAM_END) {
    return NS_ERROR_FAILURE;
  }

  chunk = inflated;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ReadEntry(const nsUrlClassifierHash& hash,
                                          PRUint32 tableId,
                                          nsUrlClassifierEntry& entry)
{
  entry.Clear();

  mozStorageStatementScoper scoper(mLookupWithTableStatement);

  nsresult rv = mLookupWithTableStatement->BindBlobParameter
                  (0, hash.buf, KEY_LENGTH);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mLookupWithTableStatement->BindInt32Parameter(1, tableId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = mLookupWithTableStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    if (!entry.ReadStatement(mLookupWithTableStatement))
      return NS_ERROR_FAILURE;
  } else {
    
    entry.mKey = hash;
    entry.mTableId = tableId;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ReadEntry(PRUint32 id,
                                          nsUrlClassifierEntry& entry)
{
  entry.Clear();
  entry.mId = id;

  mozStorageStatementScoper scoper(mLookupWithIDStatement);

  nsresult rv = mLookupWithIDStatement->BindInt32Parameter(0, id);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mLookupWithIDStatement->BindInt32Parameter(0, id);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = mLookupWithIDStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    if (!entry.ReadStatement(mLookupWithIDStatement))
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::DeleteEntry(nsUrlClassifierEntry& entry)
{
  if (entry.mId == 0) {
    return NS_OK;
  }

  mozStorageStatementScoper scoper(mDeleteStatement);
  mDeleteStatement->BindInt32Parameter(0, entry.mId);
  nsresult rv = mDeleteStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  entry.mId = 0;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::WriteEntry(nsUrlClassifierEntry& entry)
{
  mozStorageStatementScoper scoper(mUpdateStatement);

  if (entry.IsEmpty()) {
    return DeleteEntry(entry);
  }

  PRBool newEntry = (entry.mId == 0);

  nsresult rv = entry.BindStatement(mUpdateStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mUpdateStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  if (newEntry) {
    PRInt64 rowId;
    rv = mConnection->GetLastInsertRowID(&rowId);
    NS_ENSURE_SUCCESS(rv, rv);

    if (rowId > PR_UINT32_MAX) {
      return NS_ERROR_FAILURE;
    }

    entry.mId = rowId;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::GetKey(const nsACString& spec,
                                       nsUrlClassifierHash& hash)
{
  nsACString::const_iterator begin, end, iter;
  spec.BeginReading(begin);
  spec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
    return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter++);
  nsCStringArray hostComponents;
  hostComponents.ParseString(PromiseFlatCString(host).get(), ".");

  if (hostComponents.Count() < 2)
    return NS_ERROR_FAILURE;

  PRInt32 last = hostComponents.Count() - 1;
  nsCAutoString lookupHost;

  if (hostComponents.Count() > 2) {
    lookupHost.Append(*hostComponents[last - 2]);
    lookupHost.Append(".");
  }

  lookupHost.Append(*hostComponents[last - 1]);
  lookupHost.Append(".");
  lookupHost.Append(*hostComponents[last]);
  lookupHost.Append("/");

  return hash.FromPlaintext(lookupHost);
}

nsresult
nsUrlClassifierDBServiceWorker::GetChunkEntries(const nsACString& table,
                                                PRUint32 tableId,
                                                PRUint32 chunkNum,
                                                nsACString& chunk,
                                                nsTArray<nsUrlClassifierEntry>& entries)
{
  nsresult rv;
  if (StringEndsWith(table, NS_LITERAL_CSTRING("-exp"))) {
    
    rv = InflateChunk(chunk);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (StringEndsWith(table, NS_LITERAL_CSTRING("-sha128"))) {
    PRUint32 start = 0;
    while (start + KEY_LENGTH + 1 <= chunk.Length()) {
      nsUrlClassifierEntry* entry = entries.AppendElement();
      if (!entry) return NS_ERROR_OUT_OF_MEMORY;

      
      entry->mKey.Assign(Substring(chunk, start, KEY_LENGTH));

      start += KEY_LENGTH;
      
      PRUint8 numEntries = static_cast<PRUint8>(chunk[start]);
      start++;

      if (numEntries == 0) {
        
        
        entry->AddFragment(entry->mKey, chunkNum);
      } else {
        if (start + (numEntries * KEY_LENGTH) >= chunk.Length()) {
          
          return NS_ERROR_FAILURE;
        }

        for (PRUint8 i = 0; i < numEntries; i++) {
          nsUrlClassifierHash hash;
          hash.Assign(Substring(chunk, start, KEY_LENGTH));
          entry->AddFragment(hash, chunkNum);
          start += KEY_LENGTH;
        }
      }
    }
  } else {
    nsCStringArray lines;
    lines.ParseString(PromiseFlatCString(chunk).get(), "\n");

    
    for (PRInt32 i = 0; i < lines.Count(); i++) {
      nsUrlClassifierEntry* entry = entries.AppendElement();
      if (!entry) return NS_ERROR_OUT_OF_MEMORY;

      rv = GetKey(*lines[i], entry->mKey);
      NS_ENSURE_SUCCESS(rv, rv);

      entry->mTableId = tableId;
      nsUrlClassifierHash hash;
      hash.FromPlaintext(*lines[i]);
      entry->AddFragment(hash, mChunkNum);
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ParseChunkList(const nsACString& chunkStr,
                                               nsTArray<PRUint32>& chunks)
{
  LOG(("Parsing %s", PromiseFlatCString(chunkStr).get()));

  nsCStringArray elements;
  elements.ParseString(PromiseFlatCString(chunkStr).get() , ",");

  for (PRInt32 i = 0; i < elements.Count(); i++) {
    nsCString& element = *elements[i];

    PRUint32 first;
    PRUint32 last;
    if (PR_sscanf(element.get(), "%u-%u", &first, &last) == 2) {
      if (first > last) {
        PRUint32 tmp = first;
        first = last;
        last = tmp;
      }
      for (PRUint32 num = first; num <= last; num++) {
        chunks.AppendElement(num);
      }
    } else if (PR_sscanf(element.get(), "%u", &first) == 1) {
      chunks.AppendElement(first);
    }
  }

  LOG(("Got %d elements.", chunks.Length()));

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::JoinChunkList(nsTArray<PRUint32>& chunks,
                                              nsCString& chunkStr)
{
  chunkStr.Truncate();
  chunks.Sort();

  PRUint32 i = 0;
  while (i < chunks.Length()) {
    if (i != 0) {
      chunkStr.Append(',');
    }
    chunkStr.AppendInt(chunks[i]);

    PRUint32 first = i;
    PRUint32 last = first;
    i++;
    while (i < chunks.Length() && chunks[i] == chunks[i - 1] + 1) {
      last = chunks[i++];
    }

    if (last != first) {
      chunkStr.Append('-');
      chunkStr.AppendInt(last);
    }
  }

  return NS_OK;
}


nsresult
nsUrlClassifierDBServiceWorker::GetChunkLists(PRUint32 tableId,
                                              nsACString& addChunks,
                                              nsACString& subChunks)
{
  addChunks.Truncate();
  subChunks.Truncate();

  mozStorageStatementScoper scoper(mGetChunkListsStatement);

  nsresult rv = mGetChunkListsStatement->BindInt32Parameter(0, tableId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = mGetChunkListsStatement->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasMore) {
    LOG(("Getting chunks for %d, found nothing", tableId));
    return NS_OK;
  }

  rv = mGetChunkListsStatement->GetUTF8String(0, addChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mGetChunkListsStatement->GetUTF8String(1, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("Getting chunks for %d, got %s %s",
       tableId,
       PromiseFlatCString(addChunks).get(),
       PromiseFlatCString(subChunks).get()));

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::SetChunkLists(PRUint32 tableId,
                                              const nsACString& addChunks,
                                              const nsACString& subChunks)
{
  mozStorageStatementScoper scoper(mSetChunkListsStatement);

  mSetChunkListsStatement->BindUTF8StringParameter(0, addChunks);
  mSetChunkListsStatement->BindUTF8StringParameter(1, subChunks);
  mSetChunkListsStatement->BindInt32Parameter(2, tableId);
  nsresult rv = mSetChunkListsStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::AddChunk(PRUint32 tableId,
                                         PRUint32 chunkNum,
                                         nsTArray<nsUrlClassifierEntry>& entries)
{
#if defined(PR_LOGGING)
  PRIntervalTime clockStart = 0;
  if (LOG_ENABLED()) {
    clockStart = PR_IntervalNow();
  }
#endif

  LOG(("Adding %d entries to chunk %d", entries.Length(), chunkNum));

  mozStorageTransaction transaction(mConnection, PR_FALSE);

  nsCAutoString addChunks;
  nsCAutoString subChunks;

  HandlePendingLookups();

  nsresult rv = GetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<PRUint32> adds;
  ParseChunkList(addChunks, adds);
  adds.AppendElement(chunkNum);
  JoinChunkList(adds, addChunks);
  rv = SetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<PRUint32> entryIDs;

  for (PRUint32 i = 0; i < entries.Length(); i++) {
    nsUrlClassifierEntry& thisEntry = entries[i];

    HandlePendingLookups();

    nsUrlClassifierEntry existingEntry;
    rv = ReadEntry(thisEntry.mKey, tableId, existingEntry);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!existingEntry.Merge(thisEntry))
      return NS_ERROR_FAILURE;

    HandlePendingLookups();

    rv = WriteEntry(existingEntry);
    NS_ENSURE_SUCCESS(rv, rv);

    entryIDs.AppendElement(existingEntry.mId);
  }

  mozStorageStatementScoper scoper(mAddChunkEntriesStatement);
  rv = mAddChunkEntriesStatement->BindInt32Parameter(0, chunkNum);
  NS_ENSURE_SUCCESS(rv, rv);

  mAddChunkEntriesStatement->BindInt32Parameter(1, tableId);
  NS_ENSURE_SUCCESS(rv, rv);

  mAddChunkEntriesStatement->BindBlobParameter
    (2,
     reinterpret_cast<PRUint8*>(entryIDs.Elements()),
     entryIDs.Length() * sizeof(PRUint32));

  HandlePendingLookups();

  rv = mAddChunkEntriesStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  HandlePendingLookups();

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(PR_LOGGING)
  if (LOG_ENABLED()) {
    PRIntervalTime clockEnd = PR_IntervalNow();
    printf("adding chunk %d took %dms\n", chunkNum,
           PR_IntervalToMilliseconds(clockEnd - clockStart));
  }
#endif

  return rv;
}

nsresult
nsUrlClassifierDBServiceWorker::ExpireAdd(PRUint32 tableId,
                                          PRUint32 chunkNum)
{
  mozStorageTransaction transaction(mConnection, PR_FALSE);

  LOG(("Expiring chunk %d\n", chunkNum));

  nsCAutoString addChunks;
  nsCAutoString subChunks;

  HandlePendingLookups();

  nsresult rv = GetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<PRUint32> adds;
  ParseChunkList(addChunks, adds);
  adds.RemoveElement(chunkNum);
  JoinChunkList(adds, addChunks);
  rv = SetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageStatementScoper getChunkEntriesScoper(mGetChunkEntriesStatement);

  rv = mGetChunkEntriesStatement->BindInt32Parameter(0, chunkNum);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mGetChunkEntriesStatement->BindInt32Parameter(1, tableId);
  NS_ENSURE_SUCCESS(rv, rv);

  HandlePendingLookups();

  PRBool exists;
  rv = mGetChunkEntriesStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  while (exists) {
    PRUint32 size;
    const PRUint8* blob = mGetChunkEntriesStatement->AsSharedBlob(0, &size);
    if (blob) {
      const PRUint32* entries = reinterpret_cast<const PRUint32*>(blob);
      for (PRUint32 i = 0; i < (size / sizeof(PRUint32)); i++) {
        HandlePendingLookups();

        nsUrlClassifierEntry entry;
        rv = ReadEntry(entries[i], entry);
        NS_ENSURE_SUCCESS(rv, rv);

        entry.SubtractChunk(chunkNum);

        HandlePendingLookups();

        rv = WriteEntry(entry);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    HandlePendingLookups();
    rv = mGetChunkEntriesStatement->ExecuteStep(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  HandlePendingLookups();

  mozStorageStatementScoper removeScoper(mDeleteChunkEntriesStatement);
  mDeleteChunkEntriesStatement->BindInt32Parameter(0, tableId);
  mDeleteChunkEntriesStatement->BindInt32Parameter(1, chunkNum);
  rv = mDeleteChunkEntriesStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  HandlePendingLookups();

  return transaction.Commit();
}

nsresult
nsUrlClassifierDBServiceWorker::SubChunk(PRUint32 tableId,
                                         PRUint32 chunkNum,
                                         nsTArray<nsUrlClassifierEntry>& entries)
{
  mozStorageTransaction transaction(mConnection, PR_FALSE);

  nsCAutoString addChunks;
  nsCAutoString subChunks;

  HandlePendingLookups();

  nsresult rv = GetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<PRUint32> subs;
  ParseChunkList(subChunks, subs);
  subs.AppendElement(chunkNum);
  JoinChunkList(subs, subChunks);
  rv = SetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < entries.Length(); i++) {
    nsUrlClassifierEntry& thisEntry = entries[i];

    HandlePendingLookups();

    nsUrlClassifierEntry existingEntry;
    rv = ReadEntry(thisEntry.mKey, tableId, existingEntry);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!existingEntry.SubtractFragments(thisEntry))
      return NS_ERROR_FAILURE;

    HandlePendingLookups();

    rv = WriteEntry(existingEntry);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  HandlePendingLookups();

  return transaction.Commit();
}

nsresult
nsUrlClassifierDBServiceWorker::ExpireSub(PRUint32 tableId, PRUint32 chunkNum)
{
  mozStorageTransaction transaction(mConnection, PR_FALSE);

  nsCAutoString addChunks;
  nsCAutoString subChunks;

  HandlePendingLookups();

  nsresult rv = GetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<PRUint32> subs;
  ParseChunkList(subChunks, subs);
  subs.RemoveElement(chunkNum);
  JoinChunkList(subs, subChunks);
  rv = SetChunkLists(tableId, addChunks, subChunks);
  NS_ENSURE_SUCCESS(rv, rv);

  HandlePendingLookups();

  return transaction.Commit();
}

nsresult
nsUrlClassifierDBServiceWorker::ProcessChunk(PRBool* done)
{
  
  if (mPendingStreamUpdate.Length() <= static_cast<PRUint32>(mChunkLen)) {
    *done = PR_TRUE;
    return NS_OK;
  }

  if (mPendingStreamUpdate[mChunkLen] != '\n') {
    LOG(("Didn't get a terminating newline after the chunk, failing the update"));
    return NS_ERROR_FAILURE;
  }

  nsCAutoString chunk;
  chunk.Assign(Substring(mPendingStreamUpdate, 0, mChunkLen));
  mPendingStreamUpdate = Substring(mPendingStreamUpdate, mChunkLen);

  LOG(("Handling a chunk sized %d", chunk.Length()));

  nsTArray<nsUrlClassifierEntry> entries;
  GetChunkEntries(mUpdateTable, mUpdateTableId, mChunkNum, chunk, entries);

  nsresult rv;

  if (mChunkType == CHUNK_ADD) {
    rv = AddChunk(mUpdateTableId, mChunkNum, entries);
  } else {
    rv = SubChunk(mUpdateTableId, mChunkNum, entries);
  }

  
  mPendingStreamUpdate = Substring(mPendingStreamUpdate, 1);

  mState = STATE_LINE;
  *done = PR_FALSE;

  return rv;
}

nsresult
nsUrlClassifierDBServiceWorker::ProcessResponseLines(PRBool* done)
{
  PRUint32 cur = 0;
  PRInt32 next;

  nsresult rv;
  
  *done = PR_TRUE;

  nsACString& updateString = mPendingStreamUpdate;

  while(cur < updateString.Length() &&
        (next = updateString.FindChar('\n', cur)) != kNotFound) {
    const nsCSubstring& line = Substring(updateString, cur, next - cur);
    cur = next + 1;

    LOG(("Processing %s\n", PromiseFlatCString(line).get()));

    if (StringBeginsWith(line, NS_LITERAL_CSTRING("n:"))) {
      if (PR_sscanf(PromiseFlatCString(line).get(), "n:%d",
                    &mUpdateWait) != 1) {
        LOG(("Error parsing n: field: %s", PromiseFlatCString(line).get()));
        mUpdateWait = 0;
      }
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("k:"))) {
      
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("i:"))) {
      const nsCSubstring& data = Substring(line, 2);
      PRInt32 comma;
      if ((comma = data.FindChar(',')) == kNotFound) {
        mUpdateTable = data;
      } else {
        mUpdateTable = Substring(data, 0, comma);
        
      }
      GetTableId(mUpdateTable, &mUpdateTableId);
      LOG(("update table: '%s' (%d)", mUpdateTable.get(), mUpdateTableId));
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("a:")) ||
               StringBeginsWith(line, NS_LITERAL_CSTRING("s:"))) {
      mState = STATE_CHUNK;
      char command;
      if (PR_sscanf(PromiseFlatCString(line).get(),
                    "%c:%d:%d", &command, &mChunkNum, &mChunkLen) != 3 ||
          mChunkLen > MAX_CHUNK_SIZE) {
        return NS_ERROR_FAILURE;
      }
      mChunkType = (command == 'a') ? CHUNK_ADD : CHUNK_SUB;

      
      *done = PR_FALSE;
      break;
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("ad:"))) {
      PRUint32 chunkNum;
      if (PR_sscanf(PromiseFlatCString(line).get(), "ad:%u", &chunkNum) != 1) {
        return NS_ERROR_FAILURE;
      }
      rv = ExpireAdd(mUpdateTableId, chunkNum);
      NS_ENSURE_SUCCESS(rv, rv);
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("sd:"))) {
      PRUint32 chunkNum;
      if (PR_sscanf(PromiseFlatCString(line).get(), "ad:%u", &chunkNum) != 1) {
        return NS_ERROR_FAILURE;
      }
      rv = ExpireSub(mUpdateTableId, chunkNum);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      LOG(("ignoring unknown line: '%s'", PromiseFlatCString(line).get()));
    }
  }

  mPendingStreamUpdate = Substring(updateString, cur);

  return NS_OK;
}

void
nsUrlClassifierDBServiceWorker::ResetUpdate()
{
  mUpdateWait = 0;
  mState = STATE_LINE;
  mChunkNum = 0;
  mChunkLen = 0;
  mUpdateStatus = NS_OK;

  mUpdateTable.Truncate();
  mPendingStreamUpdate.Truncate();
}
































NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Update(const nsACString& chunk)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  HandlePendingLookups();

  LOG(("Update from Stream."));
  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  
  if (NS_FAILED(mUpdateStatus)) {
    return mUpdateStatus;
  }

  LOG(("Got %s\n", PromiseFlatCString(chunk).get()));

  mPendingStreamUpdate.Append(chunk);

  PRBool done = PR_FALSE;
  while (!done) {
    if (mState == STATE_CHUNK) {
      rv = ProcessChunk(&done);
    } else {
      rv = ProcessResponseLines(&done);
    }
    if (NS_FAILED(rv)) {
      mUpdateStatus = rv;
      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Finish(nsIUrlClassifierCallback* aSuccessCallback,
                                       nsIUrlClassifierCallback* aErrorCallback)
{
  nsCAutoString arg;
  if (NS_SUCCEEDED(mUpdateStatus)) {
    arg.AppendInt(mUpdateWait);
    aSuccessCallback->HandleEvent(arg);
  } else {
    arg.AppendInt(mUpdateStatus);
    aErrorCallback->HandleEvent(arg);
  }

  ResetUpdate();

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CancelStream()
{
  LOG(("CancelStream"));

  ResetUpdate();

  return NS_OK;
}





NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CloseDb()
{
  if (mConnection) {
    mConnection = nsnull;
    LOG(("urlclassifier db closed\n"));
  }
  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::OpenDb()
{
  
  if (mConnection)
    return NS_OK;

  LOG(("Opening db\n"));

  nsresult rv;
  
  nsCOMPtr<mozIStorageService> storageService =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageConnection> connection;
  rv = storageService->OpenDatabase(mDBFile, getter_AddRefs(connection));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = mDBFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = storageService->OpenDatabase(mDBFile, getter_AddRefs(connection));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA synchronous=OFF"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA page_size=4096"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA default_page_size=4096"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = MaybeCreateTables(connection);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM moz_classifier"
                        " WHERE domain=?1"),
     getter_AddRefs(mLookupStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM moz_classifier"
                        " WHERE domain=?1 AND table_id=?2"),
     getter_AddRefs(mLookupWithTableStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM moz_classifier"
                        " WHERE id=?1"),
     getter_AddRefs(mLookupWithIDStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("INSERT OR REPLACE INTO moz_classifier"
                        " VALUES (?1, ?2, ?3, ?4)"),
     getter_AddRefs(mUpdateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
         (NS_LITERAL_CSTRING("DELETE FROM moz_classifier"
                             " WHERE id=?1"),
          getter_AddRefs(mDeleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("INSERT OR REPLACE INTO moz_chunks VALUES (?1, ?2, ?3)"),
     getter_AddRefs(mAddChunkEntriesStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT entries FROM moz_chunks"
                        " WHERE chunk_id = ?1 AND table_id = ?2"),
     getter_AddRefs(mGetChunkEntriesStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("DELETE FROM moz_chunks WHERE table_id=?1 AND chunk_id=?2"),
     getter_AddRefs(mDeleteChunkEntriesStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
         (NS_LITERAL_CSTRING("SELECT add_chunks, sub_chunks FROM moz_tables"
                             " WHERE id=?1"),
          getter_AddRefs(mGetChunkListsStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
         (NS_LITERAL_CSTRING("UPDATE moz_tables"
                             " SET add_chunks=?1, sub_chunks=?2"
                             " WHERE id=?3"),
          getter_AddRefs(mSetChunkListsStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
         (NS_LITERAL_CSTRING("SELECT name, add_chunks, sub_chunks"
                             " FROM moz_tables"),
          getter_AddRefs(mGetTablesStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT id FROM moz_tables"
                        " WHERE name = ?1"),
     getter_AddRefs(mGetTableIdStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT name FROM moz_tables"
                        " WHERE id = ?1"),
     getter_AddRefs(mGetTableNameStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("INSERT INTO moz_tables(id, name, add_chunks, sub_chunks)"
                        " VALUES (null, ?1, null, null)"),
     getter_AddRefs(mInsertTableIdStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  mConnection = connection;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::MaybeCreateTables(mozIStorageConnection* connection)
{
  LOG(("MaybeCreateTables\n"));

  nsresult rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_classifier"
                       " (id INTEGER PRIMARY KEY,"
                       "  domain BLOB,"
                       "  data BLOB,"
                       "  table_id INTEGER)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE UNIQUE INDEX IF NOT EXISTS"
                       " moz_classifier_domain_index"
                       " ON moz_classifier(domain, table_id)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_tables"
                       " (id INTEGER PRIMARY KEY,"
                       "  name TEXT,"
                       "  add_chunks TEXT,"
                       "  sub_chunks TEXT);"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_chunks"
                       " (chunk_id INTEGER,"
                       "  table_id INTEGER,"
                       "  entries BLOB)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS moz_chunks_id"
                       " ON moz_chunks(chunk_id)"));
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}




NS_IMPL_THREADSAFE_ISUPPORTS2(nsUrlClassifierDBService,
                              nsIUrlClassifierDBService,
                              nsIObserver)

 nsUrlClassifierDBService*
nsUrlClassifierDBService::GetInstance()
{
  if (!sUrlClassifierDBService) {
    sUrlClassifierDBService = new nsUrlClassifierDBService();
    if (!sUrlClassifierDBService)
      return nsnull;

    NS_ADDREF(sUrlClassifierDBService);   

    if (NS_FAILED(sUrlClassifierDBService->Init())) {
      NS_RELEASE(sUrlClassifierDBService);
      return nsnull;
    }
  } else {
    
    NS_ADDREF(sUrlClassifierDBService);   
  }
  return sUrlClassifierDBService;
}


nsUrlClassifierDBService::nsUrlClassifierDBService()
{
}

nsUrlClassifierDBService::~nsUrlClassifierDBService()
{
  sUrlClassifierDBService = nsnull;
}

nsresult
nsUrlClassifierDBService::Init()
{
  NS_ASSERTION(sizeof(nsUrlClassifierHash) == KEY_LENGTH,
               "nsUrlClassifierHash must be KEY_LENGTH bytes long!");

#if defined(PR_LOGGING)
  if (!gUrlClassifierDbServiceLog)
    gUrlClassifierDbServiceLog = PR_NewLogModule("UrlClassifierDbService");
#endif

  
  nsresult rv;
  nsCOMPtr<mozIStorageService> storageService =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = NS_NewThread(&gDbBackgroundThread);
  if (NS_FAILED(rv))
    return rv;

  mWorker = new nsUrlClassifierDBServiceWorker();
  if (!mWorker)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = mWorker->Init();
  if (NS_FAILED(rv)) {
    mWorker = nsnull;
    return rv;
  }

  
  nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, "profile-before-change", PR_FALSE);
  observerService->AddObserver(this, "xpcom-shutdown-threads", PR_FALSE);

  return NS_OK;
}

nsresult
nsUrlClassifierDBService::Lookup(const nsACString& spec,
                                 nsIUrlClassifierCallback* c,
                                 PRBool needsProxy)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), spec);
  NS_ENSURE_SUCCESS(rv, rv);

  uri = NS_GetInnermostURI(uri);
  if (!uri) {
    return NS_ERROR_FAILURE;
  }

  nsCAutoString key;
  
  nsCOMPtr<nsIUrlClassifierUtils> utilsService =
    do_GetService(NS_URLCLASSIFIERUTILS_CONTRACTID);
  rv = utilsService->GetKeyForURI(uri, key);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUrlClassifierCallback> proxyCallback;
  if (needsProxy) {
    
    rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                              NS_GET_IID(nsIUrlClassifierCallback),
                              c,
                              NS_PROXY_ASYNC,
                              getter_AddRefs(proxyCallback));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    proxyCallback = c;
  }

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mWorker->QueueLookup(key, proxyCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->Lookup(EmptyCString(), nsnull, PR_FALSE);
}

NS_IMETHODIMP
nsUrlClassifierDBService::GetTables(nsIUrlClassifierCallback* c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  
  nsCOMPtr<nsIUrlClassifierCallback> proxyCallback;
  rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsIUrlClassifierCallback),
                            c,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxyCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->GetTables(proxyCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::Update(const nsACString& aUpdateChunk)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->Update(aUpdateChunk);
}

NS_IMETHODIMP
nsUrlClassifierDBService::Finish(nsIUrlClassifierCallback* aSuccessCallback,
                                 nsIUrlClassifierCallback* aErrorCallback)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  
  nsCOMPtr<nsIUrlClassifierCallback> proxySuccessCallback;
  if (aSuccessCallback) {
    rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                              NS_GET_IID(nsIUrlClassifierCallback),
                              aSuccessCallback,
                              NS_PROXY_ASYNC,
                              getter_AddRefs(proxySuccessCallback));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIUrlClassifierCallback> proxyErrorCallback;
  if (aErrorCallback) {
    rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                              NS_GET_IID(nsIUrlClassifierCallback),
                              aErrorCallback,
                              NS_PROXY_ASYNC,
                              getter_AddRefs(proxyErrorCallback));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->Finish(proxySuccessCallback, proxyErrorCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::CancelStream()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  
  nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  return proxy->CancelStream();
}

NS_IMETHODIMP
nsUrlClassifierDBService::Observe(nsISupports *aSubject, const char *aTopic,
                                  const PRUnichar *aData)
{
  NS_ASSERTION(strcmp(aTopic, "profile-before-change") == 0 ||
               strcmp(aTopic, "xpcom-shutdown-threads") == 0,
               "Unexpected observer topic");

  Shutdown();

  return NS_OK;
}


nsresult
nsUrlClassifierDBService::Shutdown()
{
  LOG(("shutting down db service\n"));

  if (!gDbBackgroundThread)
    return NS_OK;

  nsresult rv;
  
  if (mWorker) {
    nsCOMPtr<nsIUrlClassifierDBServiceWorker> proxy;
    rv = NS_GetProxyForObject(gDbBackgroundThread,
                              NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                              mWorker,
                              NS_PROXY_ASYNC,
                              getter_AddRefs(proxy));
    if (NS_SUCCEEDED(rv)) {
      rv = proxy->CloseDb();
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post close db event");
    }
  }
  LOG(("joining background thread"));

  gShuttingDownThread = PR_TRUE;
  gDbBackgroundThread->Shutdown();
  NS_RELEASE(gDbBackgroundThread);

  return NS_OK;
}
