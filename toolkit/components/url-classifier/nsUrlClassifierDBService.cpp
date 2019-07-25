








































#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "mozStorageHelper.h"
#include "mozStorageCID.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsCRT.h"
#include "nsDataHashtable.h"
#include "nsICryptoHash.h"
#include "nsICryptoHMAC.h"
#include "nsIDirectoryService.h"
#include "nsIKeyModule.h"
#include "nsIObserverService.h"
#include "nsIPermissionManager.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefService.h"
#include "nsIProperties.h"
#include "nsIProxyObjectManager.h"
#include "nsToolkitCompsCID.h"
#include "nsIUrlClassifierUtils.h"
#include "nsUrlClassifierDBService.h"
#include "nsUrlClassifierUtils.h"
#include "nsURILoader.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsTArray.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsThreadUtils.h"
#include "nsXPCOMStrings.h"
#include "mozilla/Mutex.h"
#include "prlog.h"
#include "prprf.h"
#include "prnetdb.h"
#include "zlib.h"


#include <sqlite3.h>

using namespace mozilla;





























#if defined(PR_LOGGING)
static const PRLogModuleInfo *gUrlClassifierDbServiceLog = nsnull;
#define LOG(args) PR_LOG(gUrlClassifierDbServiceLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUrlClassifierDbServiceLog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (PR_FALSE)
#endif









#define DATABASE_FILENAME "urlclassifier3.sqlite"





#define IMPLEMENTATION_VERSION 7

#define MAX_HOST_COMPONENTS 5
#define MAX_PATH_COMPONENTS 4


#define MAX_CHUNK_SIZE (1024 * 1024)


#define CHECK_MALWARE_PREF      "browser.safebrowsing.malware.enabled"
#define CHECK_MALWARE_DEFAULT   PR_FALSE

#define CHECK_PHISHING_PREF     "browser.safebrowsing.enabled"
#define CHECK_PHISHING_DEFAULT  PR_FALSE

#define GETHASH_NOISE_PREF      "urlclassifier.gethashnoise"
#define GETHASH_NOISE_DEFAULT   4

#define GETHASH_TABLES_PREF     "urlclassifier.gethashtables"

#define CONFIRM_AGE_PREF        "urlclassifier.confirm-age"
#define CONFIRM_AGE_DEFAULT_SEC (45 * 60)

#define UPDATE_CACHE_SIZE_PREF    "urlclassifier.updatecachemax"
#define UPDATE_CACHE_SIZE_DEFAULT -1


#define CLEAN_HOST_KEYS_SIZE 16
#define CLEAN_FRAGMENTS_SIZE 32




#define UPDATE_WORKING_TIME         "urlclassifier.workingtime"
#define UPDATE_WORKING_TIME_DEFAULT 5



#define UPDATE_DELAY_TIME           "urlclassifier.updatetime"
#define UPDATE_DELAY_TIME_DEFAULT   60

class nsUrlClassifierDBServiceWorker;


static nsUrlClassifierDBService* sUrlClassifierDBService;


static nsIThread* gDbBackgroundThread = nsnull;



static PRBool gShuttingDownThread = PR_FALSE;

static PRInt32 gFreshnessGuarantee = CONFIRM_AGE_DEFAULT_SEC;

static PRInt32 gUpdateCacheSize = UPDATE_CACHE_SIZE_DEFAULT;

static PRInt32 gWorkingTimeThreshold = UPDATE_WORKING_TIME_DEFAULT;
static PRInt32 gDelayTime = UPDATE_DELAY_TIME_DEFAULT;

static void
SplitTables(const nsACString& str, nsTArray<nsCString>& tables)
{
  tables.Clear();

  nsACString::const_iterator begin, iter, end;
  str.BeginReading(begin);
  str.EndReading(end);
  while (begin != end) {
    iter = begin;
    FindCharInReadable(',', iter, end);
    tables.AppendElement(Substring(begin, iter));
    begin = iter;
    if (begin != end)
      begin++;
  }
}







template <PRUint32 S>
struct nsUrlClassifierHash
{
  static const PRUint32 sHashSize = S;
  typedef nsUrlClassifierHash<S> self_type;
  PRUint8 buf[S];

  nsresult FromPlaintext(const nsACString& plainText, nsICryptoHash *hash) {
    
    
    

    nsresult rv = hash->Init(nsICryptoHash::SHA256);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = hash->Update
      (reinterpret_cast<const PRUint8*>(plainText.BeginReading()),
       plainText.Length());
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString hashed;
    rv = hash->Finish(PR_FALSE, hashed);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(hashed.Length() >= sHashSize,
                 "not enough characters in the hash");

    memcpy(buf, hashed.BeginReading(), sHashSize);

    return NS_OK;
  }

  void Assign(const nsACString& str) {
    NS_ASSERTION(str.Length() >= sHashSize,
                 "string must be at least sHashSize characters long");
    memcpy(buf, str.BeginReading(), sHashSize);
  }

  void Clear() {
    memset(buf, 0, sizeof(buf));
  }

  const PRBool operator==(const self_type& hash) const {
    return (memcmp(buf, hash.buf, sizeof(buf)) == 0);
  }
  const PRBool operator!=(const self_type& hash) const {
    return !(*this == hash);
  }
  const PRBool operator<(const self_type& hash) const {
    return memcmp(buf, hash.buf, sizeof(self_type)) < 0;
  }
  const PRBool StartsWith(const nsUrlClassifierHash<PARTIAL_LENGTH>& hash) const {
    NS_ASSERTION(sHashSize >= PARTIAL_LENGTH, "nsUrlClassifierHash must be at least PARTIAL_LENGTH bytes long");
    return memcmp(buf, hash.buf, PARTIAL_LENGTH) == 0;
  }
};

typedef nsUrlClassifierHash<DOMAIN_LENGTH> nsUrlClassifierDomainHash;
typedef nsUrlClassifierHash<PARTIAL_LENGTH> nsUrlClassifierPartialHash;
typedef nsUrlClassifierHash<COMPLETE_LENGTH> nsUrlClassifierCompleteHash;







class nsUrlClassifierEntry
{
public:
  nsUrlClassifierEntry()
    : mId(-1)
    , mHavePartial(PR_FALSE)
    , mHaveComplete(PR_FALSE)
    , mTableId(0)
    , mChunkId(0)
    , mAddChunkId(0)
    {}
  ~nsUrlClassifierEntry() {}

  
  PRBool Match(const nsUrlClassifierCompleteHash &hash);

  
  PRBool SubMatch(const nsUrlClassifierEntry& sub);

  
  void Clear();

  
  void SetHash(const nsUrlClassifierPartialHash &partialHash) {
    mPartialHash = partialHash;
    mHavePartial = PR_TRUE;
  }

  
  void SetHash(const nsUrlClassifierCompleteHash &completeHash) {
    mCompleteHash = completeHash;
    mHaveComplete = PR_TRUE;
  }

  PRBool operator== (const nsUrlClassifierEntry& entry) const {
    return ! (mTableId != entry.mTableId ||
              mChunkId != entry.mChunkId ||
              mHavePartial != entry.mHavePartial ||
              (mHavePartial && mPartialHash != entry.mPartialHash) ||
              mHaveComplete != entry.mHaveComplete ||
              (mHaveComplete && mCompleteHash != entry.mCompleteHash));
  }

  PRBool operator< (const nsUrlClassifierEntry& entry) const {
    return (mTableId < entry.mTableId ||
            mChunkId < entry.mChunkId ||
            mHavePartial && !entry.mHavePartial ||
            (mHavePartial && mPartialHash < entry.mPartialHash) ||
            mHaveComplete && !entry.mHaveComplete ||
            (mHaveComplete && mCompleteHash < entry.mCompleteHash));
  }

  PRInt64 mId;

  nsUrlClassifierDomainHash mKey;

  PRBool mHavePartial;
  nsUrlClassifierPartialHash mPartialHash;

  PRBool mHaveComplete;
  nsUrlClassifierCompleteHash mCompleteHash;

  PRUint32 mTableId;
  PRUint32 mChunkId;
  PRUint32 mAddChunkId;
};

PRBool
nsUrlClassifierEntry::Match(const nsUrlClassifierCompleteHash &hash)
{
  if (mHaveComplete)
    return mCompleteHash == hash;

  if (mHavePartial)
    return hash.StartsWith(mPartialHash);

  return PR_FALSE;
}

PRBool
nsUrlClassifierEntry::SubMatch(const nsUrlClassifierEntry &subEntry)
{
  if ((mTableId != subEntry.mTableId) || (mChunkId != subEntry.mAddChunkId))
    return PR_FALSE;

  if (subEntry.mHaveComplete)
    return mHaveComplete && mCompleteHash == subEntry.mCompleteHash;

  if (subEntry.mHavePartial)
    return mHavePartial && mPartialHash == subEntry.mPartialHash;

  return PR_FALSE;
}

void
nsUrlClassifierEntry::Clear()
{
  mId = -1;
  mHavePartial = PR_FALSE;
  mHaveComplete = PR_FALSE;
}





class nsUrlClassifierLookupResult
{
public:
  nsUrlClassifierLookupResult() : mConfirmed(PR_FALSE), mNoise(PR_FALSE) {
    mLookupFragment.Clear();
  }
  ~nsUrlClassifierLookupResult() {}

  PRBool operator==(const nsUrlClassifierLookupResult &result) const {
    
    return (mLookupFragment == result.mLookupFragment &&
            mConfirmed == result.mConfirmed &&
            mEntry == result.mEntry);
  }

  PRBool operator<(const nsUrlClassifierLookupResult &result) const {
    
    return (mLookupFragment < result.mLookupFragment ||
            mConfirmed < result.mConfirmed ||
            mEntry < result.mEntry);
  }

  
  nsUrlClassifierCompleteHash mLookupFragment;

  
  nsUrlClassifierEntry mEntry;

  
  
  PRPackedBool mConfirmed;

  
  
  PRPackedBool mNoise;

  
  nsCString mTableName;
};





class nsUrlClassifierStore
{
public:
  nsUrlClassifierStore() {}
  virtual ~nsUrlClassifierStore() {}

  
  nsresult Init(nsUrlClassifierDBServiceWorker *worker,
                mozIStorageConnection *connection,
                const nsACString& entriesTableName);
  
  void Close();

  
  virtual PRBool ReadStatement(mozIStorageStatement* statement,
                               nsUrlClassifierEntry& entry);

  
  virtual nsresult BindStatement(const nsUrlClassifierEntry& entry,
                                 mozIStorageStatement* statement);

  
  nsresult ReadEntry(PRInt64 id, nsUrlClassifierEntry& entry, PRBool *exists);

  
  nsresult DeleteEntry(nsUrlClassifierEntry& entry);

  
  nsresult WriteEntry(nsUrlClassifierEntry& entry);

  
  
  nsresult UpdateEntry(nsUrlClassifierEntry& entry);

  
  nsresult Expire(PRUint32 tableId,
                  PRUint32 chunkNum);

  
  
  nsresult ReadNoiseEntries(PRInt64 rowID,
                            PRUint32 numRequested,
                            PRBool before,
                            nsTArray<nsUrlClassifierEntry> &entries);

  
  
  nsresult RandomNumber(PRInt64 *randomNum);

protected:
  nsresult ReadEntries(mozIStorageStatement *statement,
                       nsTArray<nsUrlClassifierEntry>& entries);
  nsUrlClassifierDBServiceWorker *mWorker;
  nsCOMPtr<mozIStorageConnection> mConnection;

  nsCOMPtr<mozIStorageStatement> mLookupWithIDStatement;

  nsCOMPtr<mozIStorageStatement> mInsertStatement;
  nsCOMPtr<mozIStorageStatement> mUpdateStatement;
  nsCOMPtr<mozIStorageStatement> mDeleteStatement;
  nsCOMPtr<mozIStorageStatement> mExpireStatement;

  nsCOMPtr<mozIStorageStatement> mPartialEntriesStatement;
  nsCOMPtr<mozIStorageStatement> mPartialEntriesAfterStatement;
  nsCOMPtr<mozIStorageStatement> mLastPartialEntriesStatement;
  nsCOMPtr<mozIStorageStatement> mPartialEntriesBeforeStatement;

  nsCOMPtr<mozIStorageStatement> mRandomStatement;
};

nsresult
nsUrlClassifierStore::Init(nsUrlClassifierDBServiceWorker *worker,
                           mozIStorageConnection *connection,
                           const nsACString& entriesName)
{
  mWorker = worker;
  mConnection = connection;

  nsresult rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesName +
     NS_LITERAL_CSTRING(" WHERE id=?1"),
     getter_AddRefs(mLookupWithIDStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("DELETE FROM ") + entriesName +
     NS_LITERAL_CSTRING(" WHERE id=?1"),
     getter_AddRefs(mDeleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("DELETE FROM ") + entriesName +
     NS_LITERAL_CSTRING(" WHERE table_id=?1 AND chunk_id=?2"),
     getter_AddRefs(mExpireStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesName +
     NS_LITERAL_CSTRING(" WHERE complete_data ISNULL"
                        " LIMIT ?1"),
     getter_AddRefs(mPartialEntriesStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesName +
     NS_LITERAL_CSTRING(" WHERE id > ?1 AND complete_data ISNULL"
                        " LIMIT ?2"),
     getter_AddRefs(mPartialEntriesAfterStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesName +
     NS_LITERAL_CSTRING(" WHERE complete_data ISNULL"
                        " ORDER BY id DESC LIMIT ?1"),
     getter_AddRefs(mLastPartialEntriesStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesName +
     NS_LITERAL_CSTRING(" WHERE id < ?1 AND complete_data ISNULL"
                        " ORDER BY id DESC LIMIT ?2"),
     getter_AddRefs(mPartialEntriesBeforeStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT abs(random())"),
     getter_AddRefs(mRandomStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsUrlClassifierStore::Close()
{
  mLookupWithIDStatement = nsnull;

  mInsertStatement = nsnull;
  mUpdateStatement = nsnull;
  mDeleteStatement = nsnull;
  mExpireStatement = nsnull;

  mPartialEntriesStatement = nsnull;
  mPartialEntriesAfterStatement = nsnull;
  mPartialEntriesBeforeStatement = nsnull;
  mLastPartialEntriesStatement = nsnull;
  mRandomStatement = nsnull;

  mConnection = nsnull;
}


PRBool
nsUrlClassifierStore::ReadStatement(mozIStorageStatement* statement,
                                    nsUrlClassifierEntry& entry)
{
  entry.mId = statement->AsInt64(0);

  PRUint32 size;
  const PRUint8* blob = statement->AsSharedBlob(1, &size);
  if (!blob || (size != DOMAIN_LENGTH))
    return PR_FALSE;
  memcpy(entry.mKey.buf, blob, DOMAIN_LENGTH);

  blob = statement->AsSharedBlob(2, &size);
  if (!blob || size == 0) {
    entry.mHavePartial = PR_FALSE;
  } else {
    if (size != PARTIAL_LENGTH)
      return PR_FALSE;
    entry.mHavePartial = PR_TRUE;
    memcpy(entry.mPartialHash.buf, blob, PARTIAL_LENGTH);
  }

  blob = statement->AsSharedBlob(3, &size);
  if (!blob || size == 0) {
    entry.mHaveComplete = PR_FALSE;
  } else {
    if (size != COMPLETE_LENGTH)
      return PR_FALSE;
    entry.mHaveComplete = PR_TRUE;
    memcpy(entry.mCompleteHash.buf, blob, COMPLETE_LENGTH);
  }

  
  
  if (!(entry.mHavePartial || entry.mHaveComplete)) {
    entry.SetHash(entry.mKey);
  }

  entry.mChunkId = statement->AsInt32(4);
  entry.mTableId = statement->AsInt32(5);

  return PR_TRUE;
}

nsresult
nsUrlClassifierStore::BindStatement(const nsUrlClassifierEntry &entry,
                                    mozIStorageStatement* statement)
{
  nsresult rv;

  if (entry.mId == -1)
    rv = statement->BindNullByIndex(0);
  else
    rv = statement->BindInt64ByIndex(0, entry.mId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindBlobByIndex(1, entry.mKey.buf, DOMAIN_LENGTH);
  NS_ENSURE_SUCCESS(rv, rv);

  if (entry.mHavePartial) {
    
    
    if (!entry.mHaveComplete && entry.mKey == entry.mPartialHash) {
      rv = statement->BindNullByIndex(2);
    } else {
      rv = statement->BindBlobByIndex(2, entry.mPartialHash.buf,
                                        PARTIAL_LENGTH);
    }
  } else {
    rv = statement->BindNullByIndex(2);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (entry.mHaveComplete) {
    rv = statement->BindBlobByIndex(3, entry.mCompleteHash.buf, COMPLETE_LENGTH);
  } else {
    rv = statement->BindNullByIndex(3);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt32ByIndex(4, entry.mChunkId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindInt32ByIndex(5, entry.mTableId);
  NS_ENSURE_SUCCESS(rv, rv);

  return PR_TRUE;
}

nsresult
nsUrlClassifierStore::ReadEntries(mozIStorageStatement *statement,
                                  nsTArray<nsUrlClassifierEntry>& entries)
{
  PRBool exists;
  nsresult rv = statement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  while (exists) {
    nsUrlClassifierEntry *entry = entries.AppendElement();
    if (!entry) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!ReadStatement(statement, *entry))
      return NS_ERROR_FAILURE;

    statement->ExecuteStep(&exists);
  }

  return NS_OK;
}

nsresult
nsUrlClassifierStore::ReadEntry(PRInt64 id,
                                nsUrlClassifierEntry& entry,
                                PRBool *exists)
{
  entry.Clear();

  mozStorageStatementScoper scoper(mLookupWithIDStatement);

  nsresult rv = mLookupWithIDStatement->BindInt64ByIndex(0, id);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mLookupWithIDStatement->ExecuteStep(exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (*exists) {
    if (ReadStatement(mLookupWithIDStatement, entry))
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierStore::ReadNoiseEntries(PRInt64 rowID,
                                       PRUint32 numRequested,
                                       PRBool before,
                                       nsTArray<nsUrlClassifierEntry> &entries)
{
  if (numRequested == 0) {
    return NS_OK;
  }

  mozIStorageStatement *statement =
    before ? mPartialEntriesBeforeStatement : mPartialEntriesAfterStatement;
  mozStorageStatementScoper scoper(statement);

  nsresult rv = statement->BindInt64ByIndex(0, rowID);
  NS_ENSURE_SUCCESS(rv, rv);

  statement->BindInt32ByIndex(1, numRequested);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = entries.Length();
  rv = ReadEntries(statement, entries);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 numRead = entries.Length() - length;

  if (numRead >= numRequested)
    return NS_OK;

  
  

  mozIStorageStatement *wraparoundStatement =
    before ? mPartialEntriesStatement : mLastPartialEntriesStatement;
  mozStorageStatementScoper wraparoundScoper(wraparoundStatement);

  rv = wraparoundStatement->BindInt32ByIndex(0, numRequested - numRead);
  NS_ENSURE_SUCCESS(rv, rv);

  return ReadEntries(wraparoundStatement, entries);
}

nsresult
nsUrlClassifierStore::RandomNumber(PRInt64 *randomNum)
{
  mozStorageStatementScoper randScoper(mRandomStatement);
  PRBool exists;
  nsresult rv = mRandomStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists)
    return NS_ERROR_NOT_AVAILABLE;

  *randomNum = mRandomStatement->AsInt64(0);

  return NS_OK;
}





class nsUrlClassifierAddStore: public nsUrlClassifierStore
{
public:
  nsUrlClassifierAddStore() {};
  virtual ~nsUrlClassifierAddStore() {};

  nsresult Init(nsUrlClassifierDBServiceWorker *worker,
                mozIStorageConnection *connection,
                const nsACString& entriesTableName);

  void Close();

  
  nsresult ReadAddEntries(const nsUrlClassifierDomainHash& key,
                          PRUint32 tableId,
                          PRUint32 chunkId,
                          nsTArray<nsUrlClassifierEntry>& entry);

  
  nsresult ReadAddEntries(const nsUrlClassifierDomainHash& key,
                          nsTArray<nsUrlClassifierEntry>& entry);

protected:
  nsCOMPtr<mozIStorageStatement> mLookupStatement;
  nsCOMPtr<mozIStorageStatement> mLookupWithChunkStatement;
};

nsresult
nsUrlClassifierAddStore::Init(nsUrlClassifierDBServiceWorker *worker,
                              mozIStorageConnection *connection,
                              const nsACString &entriesTableName)
{
  nsresult rv = nsUrlClassifierStore::Init(worker, connection,
                                           entriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("INSERT OR REPLACE INTO ") + entriesTableName +
     NS_LITERAL_CSTRING(" VALUES (?1, ?2, ?3, ?4, ?5, ?6)"),
     getter_AddRefs(mInsertStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("UPDATE ") + entriesTableName +
     NS_LITERAL_CSTRING(" SET domain=?2, partial_data=?3, "
                        " complete_data=?4, chunk_id=?5, table_id=?6"
                        " WHERE id=?1"),
     getter_AddRefs(mUpdateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesTableName +
     NS_LITERAL_CSTRING(" WHERE domain=?1"),
     getter_AddRefs(mLookupStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesTableName +
     NS_LITERAL_CSTRING(" WHERE domain=?1 AND table_id=?2 AND chunk_id=?3"),
     getter_AddRefs(mLookupWithChunkStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsUrlClassifierAddStore::Close()
{
  nsUrlClassifierStore::Close();

  mLookupStatement = nsnull;
  mLookupWithChunkStatement = nsnull;
}

nsresult
nsUrlClassifierAddStore::ReadAddEntries(const nsUrlClassifierDomainHash& hash,
                                        PRUint32 tableId,
                                        PRUint32 chunkId,
                                        nsTArray<nsUrlClassifierEntry>& entries)
{
  mozStorageStatementScoper scoper(mLookupWithChunkStatement);

  nsresult rv = mLookupWithChunkStatement->BindBlobByIndex
                  (0, hash.buf, DOMAIN_LENGTH);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mLookupWithChunkStatement->BindInt32ByIndex(1, tableId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mLookupWithChunkStatement->BindInt32ByIndex(2, chunkId);
  NS_ENSURE_SUCCESS(rv, rv);

  return ReadEntries(mLookupWithChunkStatement, entries);
}

nsresult
nsUrlClassifierAddStore::ReadAddEntries(const nsUrlClassifierDomainHash& hash,
                                        nsTArray<nsUrlClassifierEntry>& entries)
{
  mozStorageStatementScoper scoper(mLookupStatement);

  nsresult rv = mLookupStatement->BindBlobByIndex
                  (0, hash.buf, DOMAIN_LENGTH);
  NS_ENSURE_SUCCESS(rv, rv);

  return ReadEntries(mLookupStatement, entries);
}





class nsUrlClassifierSubStore : public nsUrlClassifierStore
{
public:
  nsUrlClassifierSubStore() {};
  virtual ~nsUrlClassifierSubStore() {};

  nsresult Init(nsUrlClassifierDBServiceWorker *worker,
                mozIStorageConnection *connection,
                const nsACString& entriesTableName);

  void Close();

  
  virtual PRBool ReadStatement(mozIStorageStatement* statement,
                               nsUrlClassifierEntry& entry);

  
  virtual nsresult BindStatement(const nsUrlClassifierEntry& entry,
                                 mozIStorageStatement* statement);

  
  nsresult ReadSubEntries(PRUint32 tableId, PRUint32 chunkId,
                          nsTArray<nsUrlClassifierEntry> &subEntry);

  
  nsresult ExpireAddChunk(PRUint32 tableId, PRUint32 chunkId);

protected:
  nsCOMPtr<mozIStorageStatement> mLookupWithAddChunkStatement;
  nsCOMPtr<mozIStorageStatement> mExpireAddChunkStatement;
};

nsresult
nsUrlClassifierSubStore::Init(nsUrlClassifierDBServiceWorker *worker,
                              mozIStorageConnection *connection,
                              const nsACString &entriesTableName)
{
  nsresult rv = nsUrlClassifierStore::Init(worker, connection,
                                           entriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("INSERT OR REPLACE INTO ") + entriesTableName +
     NS_LITERAL_CSTRING(" VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)"),
     getter_AddRefs(mInsertStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("UPDATE ") + entriesTableName +
     NS_LITERAL_CSTRING(" SET domain=?2, partial_data=?3, complete_data=?4,"
                        " chunk_id=?5, table_id=?6, add_chunk_id=?7"
                        " WHERE id=?1"),
     getter_AddRefs(mUpdateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("SELECT * FROM ") + entriesTableName +
     NS_LITERAL_CSTRING(" WHERE table_id=?1 AND add_chunk_id=?2"),
     getter_AddRefs(mLookupWithAddChunkStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mConnection->CreateStatement
    (NS_LITERAL_CSTRING("DELETE FROM ") + entriesTableName +
     NS_LITERAL_CSTRING(" WHERE table_id=?1 AND add_chunk_id=?2"),
     getter_AddRefs(mExpireAddChunkStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
nsUrlClassifierSubStore::ReadStatement(mozIStorageStatement* statement,
                                       nsUrlClassifierEntry& entry)
{
  if (!nsUrlClassifierStore::ReadStatement(statement, entry))
    return PR_FALSE;

  entry.mAddChunkId = statement->AsInt32(6);
  return PR_TRUE;
}

nsresult
nsUrlClassifierSubStore::BindStatement(const nsUrlClassifierEntry& entry,
                                       mozIStorageStatement* statement)
{
  nsresult rv = nsUrlClassifierStore::BindStatement(entry, statement);
  NS_ENSURE_SUCCESS(rv, rv);

  return statement->BindInt32ByIndex(6, entry.mAddChunkId);
}

nsresult
nsUrlClassifierSubStore::ReadSubEntries(PRUint32 tableId, PRUint32 addChunkId,
                                        nsTArray<nsUrlClassifierEntry>& entries)
{
  mozStorageStatementScoper scoper(mLookupWithAddChunkStatement);

  nsresult rv = mLookupWithAddChunkStatement->BindInt32ByIndex(0, tableId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mLookupWithAddChunkStatement->BindInt32ByIndex(1, addChunkId);
  NS_ENSURE_SUCCESS(rv, rv);

  return ReadEntries(mLookupWithAddChunkStatement, entries);
}

nsresult
nsUrlClassifierSubStore::ExpireAddChunk(PRUint32 tableId, PRUint32 addChunkId)
{
  mozStorageStatementScoper scoper(mExpireAddChunkStatement);

  nsresult rv = mExpireAddChunkStatement->BindInt32ByIndex(0, tableId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mExpireAddChunkStatement->BindInt32ByIndex(1, addChunkId);
  NS_ENSURE_SUCCESS(rv, rv);

  return mExpireAddChunkStatement->Execute();
}

void
nsUrlClassifierSubStore::Close()
{
  nsUrlClassifierStore::Close();
  mLookupWithAddChunkStatement = nsnull;
  mExpireAddChunkStatement = nsnull;
}



class nsUrlClassifierDBServiceWorker : public nsIUrlClassifierDBServiceWorker
{
public:
  nsUrlClassifierDBServiceWorker();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERDBSERVICE
  NS_DECL_NSIURLCLASSIFIERDBSERVICEWORKER

  
  nsresult Init(PRInt32 gethashNoise);

  
  nsresult QueueLookup(const nsACString& lookupKey,
                       nsIUrlClassifierLookupCallback* callback);

  
  nsresult CheckCleanHost(const nsACString &lookupKey,
                          PRBool *clean);

  
  
  nsresult HandlePendingLookups();

private:
  
  ~nsUrlClassifierDBServiceWorker();

  
  nsUrlClassifierDBServiceWorker(nsUrlClassifierDBServiceWorker&);

  
  nsresult OpenDb();

  
  nsresult MaybeCreateTables(mozIStorageConnection* connection);

  nsresult GetTableName(PRUint32 tableId, nsACString& table);
  nsresult GetTableId(const nsACString& table, PRUint32* tableId);

  
  nsresult InflateChunk(nsACString& chunk);

  
  nsresult GetShaEntries(PRUint32 tableId,
                         PRUint32 chunkType,
                         PRUint32 chunkNum,
                         PRUint32 domainSize,
                         PRUint32 hashSize,
                         nsACString& chunk,
                         nsTArray<nsUrlClassifierEntry>& entries);

  
  nsresult GetChunkEntries(const nsACString& table,
                           PRUint32 tableId,
                           PRUint32 chunkType,
                           PRUint32 chunkNum,
                           PRUint32 hashSize,
                           nsACString& chunk,
                           nsTArray<nsUrlClassifierEntry>& entries);

  
  
  
  PRBool ParseChunkRange(nsACString::const_iterator &begin,
                         const nsACString::const_iterator &end,
                         PRUint32 *first, PRUint32 *last);

  
  nsresult ParseChunkList(const nsACString& chunkStr,
                          nsTArray<PRUint32>& chunks);

  
  nsresult JoinChunkList(nsTArray<PRUint32>& chunks, nsCString& chunkStr);

  
  nsresult GetChunkLists(PRUint32 tableId,
                         nsACString& addChunks,
                         nsACString& subChunks);

  
  nsresult SetChunkLists(PRUint32 tableId,
                         const nsACString& addChunks,
                         const nsACString& subChunks);

  
  
  
  nsresult CacheChunkLists(PRUint32 tableId,
                           PRBool parseAdds,
                           PRBool parseSubs);

  
  void ClearCachedChunkLists();

  
  nsresult FlushChunkLists();

  
  
  PRBool InsertChunkId(nsTArray<PRUint32>& chunks, PRUint32 chunkNum);

  
  
  nsresult AddChunk(PRUint32 tableId, PRUint32 chunkNum,
                    nsTArray<nsUrlClassifierEntry>& entries);

  
  nsresult ExpireAdd(PRUint32 tableId, PRUint32 chunkNum);

  
  nsresult SubChunk(PRUint32 tableId, PRUint32 chunkNum,
                    nsTArray<nsUrlClassifierEntry>& entries);

  
  nsresult ExpireSub(PRUint32 tableId, PRUint32 chunkNum);

  
  nsresult ProcessResponseLines(PRBool* done);
  
  nsresult ProcessChunk(PRBool* done);

  
  nsresult SetupUpdate();

  
  nsresult ApplyUpdate();

  
  void ResetStream();

  
  void ResetUpdate();

  
  void ResetLookupCache();

  
  
  
  nsresult GetLookupFragments(const nsCSubstring& spec,
                              nsTArray<nsCString>& fragments);

  
  PRBool IsCanonicalizedIP(const nsACString& host);

  
  
  
  
  
  nsresult GetKey(const nsACString& spec, nsUrlClassifierDomainHash& hash);

  
  
  
  
  
  nsresult GetHostKeys(const nsACString &spec,
                       nsTArray<nsCString> &hostKeys);


  nsresult CacheEntries(const nsCSubstring& spec);

  
  
  nsresult Check(const nsCSubstring& spec,
                 nsTArray<nsUrlClassifierLookupResult>& results);

  
  nsresult DoLookup(const nsACString& spec, nsIUrlClassifierLookupCallback* c);

  
  nsresult AddNoise(PRInt64 nearID,
                    PRInt32 count,
                    nsTArray<nsUrlClassifierLookupResult>& results);

  nsCOMPtr<nsIFile> mDBFile;

  nsCOMPtr<nsICryptoHash> mCryptoHash;

  
  
  
  nsCOMPtr<mozIStorageConnection> mConnection;

  
  
  nsUrlClassifierAddStore mMainStore;

  
  nsUrlClassifierSubStore mPendingSubStore;

  nsCOMPtr<mozIStorageStatement> mGetChunkListsStatement;
  nsCOMPtr<mozIStorageStatement> mSetChunkListsStatement;

  nsCOMPtr<mozIStorageStatement> mGetTablesStatement;
  nsCOMPtr<mozIStorageStatement> mGetTableIdStatement;
  nsCOMPtr<mozIStorageStatement> mGetTableNameStatement;
  nsCOMPtr<mozIStorageStatement> mInsertTableIdStatement;
  nsCOMPtr<mozIStorageStatement> mGetPageSizeStatement;

  
  nsDataHashtable<nsCStringHashKey, PRInt64> mTableFreshness;

  
  
  nsCString mPendingStreamUpdate;

  PRInt32 mUpdateWait;

  PRBool mResetRequested;
  PRBool mGrewCache;

  enum {
    STATE_LINE,
    STATE_CHUNK
  } mState;

  enum {
    CHUNK_ADD,
    CHUNK_SUB
  } mChunkType;

  PRUint32 mChunkNum;
  PRUint32 mHashSize;
  PRUint32 mChunkLen;

  
  nsTArray<nsCString> mUpdateTables;

  nsCString mUpdateTable;
  PRUint32 mUpdateTableId;

  nsresult mUpdateStatus;

  nsCOMPtr<nsIUrlClassifierUpdateObserver> mUpdateObserver;
  PRBool mInStream;
  PRBool mPrimaryStream;

  PRBool mHaveCachedLists;
  PRUint32 mCachedListsTable;
  nsCAutoString mCachedSubsStr;
  nsCAutoString mCachedAddsStr;

  PRBool mHaveCachedAddChunks;
  nsTArray<PRUint32> mCachedAddChunks;

  PRBool mHaveCachedSubChunks;
  nsTArray<PRUint32> mCachedSubChunks;

  
  nsCString mUpdateClientKey;

  
  nsCString mServerMAC;

  
  
  PRIntervalTime mUpdateStartTime;

  nsCOMPtr<nsICryptoHMAC> mHMAC;
  
  PRInt32 mGethashNoise;

  
  
  nsUrlClassifierFragmentSet mCleanHostKeys;

  
  
  
  Mutex mCleanHostKeysLock;

  
  
  nsUrlClassifierFragmentSet mCleanFragments;

  
  
  nsCString mCachedHostKey;
  nsTArray<nsUrlClassifierEntry> mCachedEntries;

  
  
  Mutex mPendingLookupLock;

  class PendingLookup {
  public:
    nsCString mKey;
    nsCOMPtr<nsIUrlClassifierLookupCallback> mCallback;
  };

  
  nsTArray<PendingLookup> mPendingLookups;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsUrlClassifierDBServiceWorker,
                              nsIUrlClassifierDBServiceWorker,
                              nsIUrlClassifierDBService)

nsUrlClassifierDBServiceWorker::nsUrlClassifierDBServiceWorker()
  : mUpdateWait(0)
  , mResetRequested(PR_FALSE)
  , mGrewCache(PR_FALSE)
  , mState(STATE_LINE)
  , mChunkType(CHUNK_ADD)
  , mChunkNum(0)
  , mHashSize(0)
  , mChunkLen(0)
  , mUpdateTableId(0)
  , mUpdateStatus(NS_OK)
  , mInStream(PR_FALSE)
  , mPrimaryStream(PR_FALSE)
  , mHaveCachedLists(PR_FALSE)
  , mCachedListsTable(PR_UINT32_MAX)
  , mHaveCachedAddChunks(PR_FALSE)
  , mHaveCachedSubChunks(PR_FALSE)
  , mUpdateStartTime(0)
  , mGethashNoise(0)
  , mCleanHostKeysLock("nsUrlClassifierDBServerWorker.mCleanHostKeysLock")
  , mPendingLookupLock("nsUrlClassifierDBServerWorker.mPendingLookupLock")
{
}

nsUrlClassifierDBServiceWorker::~nsUrlClassifierDBServiceWorker()
{
  NS_ASSERTION(!mConnection,
               "Db connection not closed, leaking memory!  Call CloseDb "
               "to close the connection.");
}

nsresult
nsUrlClassifierDBServiceWorker::Init(PRInt32 gethashNoise)
{
  mGethashNoise = gethashNoise;

  

  
  
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                       getter_AddRefs(mDBFile));

  if (NS_FAILED(rv)) {
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(mDBFile));
  }

  if (NS_FAILED(rv)) return NS_ERROR_NOT_AVAILABLE;

  rv = mDBFile->Append(NS_LITERAL_STRING(DATABASE_FILENAME));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mCleanHostKeys.Init(CLEAN_HOST_KEYS_SIZE))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mCleanFragments.Init(CLEAN_FRAGMENTS_SIZE))
    return NS_ERROR_OUT_OF_MEMORY;

  ResetUpdate();

  mTableFreshness.Init();

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::QueueLookup(const nsACString& spec,
                                            nsIUrlClassifierLookupCallback* callback)
{
  MutexAutoLock lock(mPendingLookupLock);

  PendingLookup* lookup = mPendingLookups.AppendElement();
  if (!lookup) return NS_ERROR_OUT_OF_MEMORY;

  lookup->mKey = spec;
  lookup->mCallback = callback;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::CheckCleanHost(const nsACString &spec,
                                               PRBool *clean)
{
  nsAutoTArray<nsCString, 2> lookupHosts;
  nsresult rv = GetHostKeys(spec, lookupHosts);
  NS_ENSURE_SUCCESS(rv, rv);

  MutexAutoLock lock(mCleanHostKeysLock);

  for (PRUint32 i = 0; i < lookupHosts.Length(); i++) {
    if (!mCleanHostKeys.Has(lookupHosts[i])) {
      *clean = PR_FALSE;
      return NS_OK;
    }
  }

  *clean = PR_TRUE;
  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::GetLookupFragments(const nsACString& spec,
                                                   nsTArray<nsCString>& fragments)
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
  nsCAutoString path;
  path.Assign(Substring(iter, end));

  








  nsTArray<nsCString> hosts;
  hosts.AppendElement(host);

  host.BeginReading(begin);
  host.EndReading(end);
  int numComponents = 0;
  while (RFindInReadable(NS_LITERAL_CSTRING("."), begin, end) &&
         numComponents < MAX_HOST_COMPONENTS) {
    
    if (++numComponents >= 2) {
      host.EndReading(iter);
      hosts.AppendElement(Substring(end, iter));
    }
    end = begin;
    host.BeginReading(begin);
  }

  











  nsTArray<nsCString> paths;
  paths.AppendElement(path);

  path.BeginReading(iter);
  path.EndReading(end);
  if (FindCharInReadable('?', iter, end)) {
    path.BeginReading(begin);
    path = Substring(begin, iter);
    paths.AppendElement(path);
  }

  
  paths.AppendElement(EmptyCString());

  numComponents = 1;
  path.BeginReading(begin);
  path.EndReading(end);
  iter = begin;
  while (FindCharInReadable('/', iter, end) &&
         numComponents < MAX_PATH_COMPONENTS) {
    iter++;
    paths.AppendElement(Substring(begin, iter));
    numComponents++;
  }

  for (PRUint32 hostIndex = 0; hostIndex < hosts.Length(); hostIndex++) {
    for (PRUint32 pathIndex = 0; pathIndex < paths.Length(); pathIndex++) {
      nsCString key;
      key.Assign(hosts[hostIndex]);
      key.Append('/');
      key.Append(paths[pathIndex]);
      LOG(("Chking %s", key.get()));

      fragments.AppendElement(key);
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::CacheEntries(const nsACString& spec)
{
  nsAutoTArray<nsCString, 2> lookupHosts;
  nsresult rv = GetHostKeys(spec, lookupHosts);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString hostKey;
  for (PRUint32 i = 0; i < lookupHosts.Length(); i++) {
    hostKey.Append(lookupHosts[i]);
    hostKey.Append("|");
  }

  if (hostKey == mCachedHostKey) {
    
    return NS_OK;
  }

  mCachedEntries.Clear();
  mCachedHostKey.Truncate();

  PRUint32 prevLength = 0;
  for (PRUint32 i = 0; i < lookupHosts.Length(); i++) {
    
    
    
    
    
    {
      MutexAutoLock lock(mCleanHostKeysLock);
      if (mCleanHostKeys.Has(lookupHosts[i]))
        continue;
    }

    
    nsUrlClassifierDomainHash hostKeyHash;
    hostKeyHash.FromPlaintext(lookupHosts[i], mCryptoHash);
    mMainStore.ReadAddEntries(hostKeyHash, mCachedEntries);

    if (mCachedEntries.Length() == prevLength) {
      
      
      
      MutexAutoLock lock(mCleanHostKeysLock);
      mCleanHostKeys.Put(lookupHosts[i]);
    } else {
      prevLength = mCachedEntries.Length();
    }
  }

  mCachedHostKey = hostKey;

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::Check(const nsACString& spec,
                                      nsTArray<nsUrlClassifierLookupResult>& results)
{
  
  nsresult  rv = CacheEntries(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mCachedEntries.Length() == 0) {
    return NS_OK;
  }

  
  nsTArray<nsCString> fragments;
  rv = GetLookupFragments(spec, fragments);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 now = (PR_Now() / PR_USEC_PER_SEC);

  
  for (PRUint32 i = 0; i < fragments.Length(); i++) {
    
    if (mCleanFragments.Has(fragments[i]))
      continue;

    nsUrlClassifierCompleteHash lookupHash;
    lookupHash.FromPlaintext(fragments[i], mCryptoHash);

    PRBool foundMatch = PR_FALSE;
    for (PRUint32 j = 0; j < mCachedEntries.Length(); j++) {
      nsUrlClassifierEntry &entry = mCachedEntries[j];
      if (entry.Match(lookupHash)) {
        
        
        
        
        nsUrlClassifierLookupResult *result = results.AppendElement();
        if (!result)
          return NS_ERROR_OUT_OF_MEMORY;

        result->mLookupFragment = lookupHash;
        result->mEntry = entry;

        
        GetTableName(entry.mTableId, result->mTableName);

        PRBool fresh;
        PRInt64 tableUpdateTime;
        if (mTableFreshness.Get(result->mTableName, &tableUpdateTime)) {
          LOG(("tableUpdateTime: %lld, now: %lld, freshnessGuarantee: %d\n",
               tableUpdateTime, now, gFreshnessGuarantee));
          fresh = ((now - tableUpdateTime) <= gFreshnessGuarantee);
        } else {
          LOG(("No expiration time for this table.\n"));
          fresh = PR_FALSE;
        }

        
        
        result->mConfirmed = entry.mHaveComplete && fresh;

        foundMatch = PR_TRUE;
        LOG(("Found a result.  complete=%d, fresh=%d",
             entry.mHaveComplete, fresh));
      }
    }

    if (!foundMatch) {
      
      
      mCleanFragments.Put(fragments[i]);
    }
  }

  return NS_OK;
}












nsresult
nsUrlClassifierDBServiceWorker::DoLookup(const nsACString& spec,
                                         nsIUrlClassifierLookupCallback* c)
{
  if (gShuttingDownThread) {
    c->LookupComplete(nsnull);
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    c->LookupComplete(nsnull);
    return NS_ERROR_FAILURE;
  }

#if defined(PR_LOGGING)
  PRIntervalTime clockStart = 0;
  if (LOG_ENABLED()) {
    clockStart = PR_IntervalNow();
  }
#endif

  nsAutoPtr<nsTArray<nsUrlClassifierLookupResult> > results;
  results = new nsTArray<nsUrlClassifierLookupResult>();
  if (!results) {
    c->LookupComplete(nsnull);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  Check(spec, *results);

#if defined(PR_LOGGING)
  if (LOG_ENABLED()) {
    PRIntervalTime clockEnd = PR_IntervalNow();
    LOG(("query took %dms\n",
         PR_IntervalToMilliseconds(clockEnd - clockStart)));
  }
#endif

  for (PRUint32 i = 0; i < results->Length(); i++) {
    if (!results->ElementAt(i).mConfirmed) {
      
      AddNoise(results->ElementAt(i).mEntry.mId, mGethashNoise, *results);
      break;
    }
  }

  
  c->LookupComplete(results.forget());

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::HandlePendingLookups()
{
  MutexAutoLock lock(mPendingLookupLock);
  while (mPendingLookups.Length() > 0) {
    PendingLookup lookup = mPendingLookups[0];
    mPendingLookups.RemoveElementAt(0);
    {
      MutexAutoUnlock unlock(mPendingLookupLock);
      DoLookup(lookup.mKey, lookup.mCallback);
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::AddNoise(PRInt64 nearID,
                                         PRInt32 count,
                                         nsTArray<nsUrlClassifierLookupResult>& results)
{
  if (count < 1) {
    return NS_OK;
  }

  PRInt64 randomNum;
  nsresult rv = mMainStore.RandomNumber(&randomNum);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 numBefore = randomNum % count;

  nsTArray<nsUrlClassifierEntry> noiseEntries;
  rv = mMainStore.ReadNoiseEntries(nearID, numBefore, PR_TRUE, noiseEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainStore.ReadNoiseEntries(nearID, count - numBefore, PR_FALSE, noiseEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < noiseEntries.Length(); i++) {
    nsUrlClassifierLookupResult *result = results.AppendElement();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;

    result->mEntry = noiseEntries[i];
    result->mConfirmed = PR_FALSE;
    result->mNoise = PR_TRUE;

    
    GetTableName(noiseEntries[i].mTableId, result->mTableName);
  }

  return NS_OK;
}



NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::Lookup(const nsACString& spec,
                                       nsIUrlClassifierCallback* c)
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

    PRBool haveAdds = PR_FALSE;
    if (!val.IsEmpty()) {
      response.Append("a:");
      response.Append(val);
      haveAdds = PR_TRUE;
    }

    mGetTablesStatement->GetUTF8String(2, val);
    if (!val.IsEmpty()) {
      if (haveAdds)
        response.Append(":");

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

  nsresult rv = mGetTableIdStatement->BindUTF8StringByIndex(0, table);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = mGetTableIdStatement->ExecuteStep(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (exists) {
    *tableId = mGetTableIdStatement->AsInt32(0);
    return NS_OK;
  }

  mozStorageStatementScoper insertScoper(mInsertTableIdStatement);
  rv = mInsertTableIdStatement->BindUTF8StringByIndex(0, table);
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
  nsresult rv = mGetTableNameStatement->BindInt32ByIndex(0, tableId);
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
nsUrlClassifierStore::DeleteEntry(nsUrlClassifierEntry& entry)
{
  if (entry.mId == -1) {
    return NS_OK;
  }

  mozStorageStatementScoper scoper(mDeleteStatement);
  mDeleteStatement->BindInt64ByIndex(0, entry.mId);
  nsresult rv = mDeleteStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  entry.mId = -1;

  return NS_OK;
}

nsresult
nsUrlClassifierStore::WriteEntry(nsUrlClassifierEntry& entry)
{
  if (entry.mId != -1) {
    
    return NS_OK;
  }

  mozStorageStatementScoper scoper(mInsertStatement);

  nsresult rv = BindStatement(entry, mInsertStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mInsertStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 rowId;
  rv = mConnection->GetLastInsertRowID(&rowId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowId > PR_UINT32_MAX) {
    return NS_ERROR_FAILURE;
  }

  entry.mId = rowId;

  return NS_OK;
}

nsresult
nsUrlClassifierStore::UpdateEntry(nsUrlClassifierEntry& entry)
{
  mozStorageStatementScoper scoper(mUpdateStatement);

  NS_ENSURE_ARG(entry.mId != -1);

  nsresult rv = BindStatement(entry, mUpdateStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mUpdateStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
nsUrlClassifierDBServiceWorker::IsCanonicalizedIP(const nsACString& host)
{
  
  
  PRUint32 i1, i2, i3, i4;
  char c;
  if (PR_sscanf(PromiseFlatCString(host).get(), "%u.%u.%u.%u%c",
                &i1, &i2, &i3, &i4, &c) == 4) {
    return (i1 <= 0xFF && i2 <= 0xFF && i3 <= 0xFF && i4 <= 0xFF);
  }

  return PR_FALSE;
}

nsresult
nsUrlClassifierDBServiceWorker::GetKey(const nsACString& spec,
                                       nsUrlClassifierDomainHash& hash)
{
  nsACString::const_iterator begin, end, iter;
  spec.BeginReading(begin);
  spec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
    return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter);

  if (IsCanonicalizedIP(host)) {
    nsCAutoString key;
    key.Assign(host);
    key.Append("/");
    return hash.FromPlaintext(key, mCryptoHash);
  }

  nsTArray<nsCString> hostComponents;
  ParseString(PromiseFlatCString(host), '.', hostComponents);

  if (hostComponents.Length() < 2)
    return NS_ERROR_FAILURE;

  PRInt32 last = PRInt32(hostComponents.Length()) - 1;
  nsCAutoString lookupHost;

  if (hostComponents.Length() > 2) {
    lookupHost.Append(hostComponents[last - 2]);
    lookupHost.Append(".");
  }

  lookupHost.Append(hostComponents[last - 1]);
  lookupHost.Append(".");
  lookupHost.Append(hostComponents[last]);
  lookupHost.Append("/");

  return hash.FromPlaintext(lookupHost, mCryptoHash);
}

nsresult
nsUrlClassifierDBServiceWorker::GetHostKeys(const nsACString &spec,
                                            nsTArray<nsCString> &hostKeys)
{
  nsACString::const_iterator begin, end, iter;
  spec.BeginReading(begin);
  spec.EndReading(end);

  iter = begin;
  if (!FindCharInReadable('/', iter, end)) {
    return NS_OK;
  }

  const nsCSubstring& host = Substring(begin, iter);

  if (IsCanonicalizedIP(host)) {
    nsCString *key = hostKeys.AppendElement();
    if (!key)
      return NS_ERROR_OUT_OF_MEMORY;

    key->Assign(host);
    key->Append("/");
    return NS_OK;
  }

  nsTArray<nsCString> hostComponents;
  ParseString(PromiseFlatCString(host), '.', hostComponents);

  if (hostComponents.Length() < 2) {
    
    return NS_OK;
  }

  
  PRInt32 last = PRInt32(hostComponents.Length()) - 1;
  nsCString *lookupHost = hostKeys.AppendElement();
  if (!lookupHost)
    return NS_ERROR_OUT_OF_MEMORY;

  lookupHost->Assign(hostComponents[last - 1]);
  lookupHost->Append(".");
  lookupHost->Append(hostComponents[last]);
  lookupHost->Append("/");

  
  if (hostComponents.Length() > 2) {
    nsCString *lookupHost2 = hostKeys.AppendElement();
    if (!lookupHost2)
      return NS_ERROR_OUT_OF_MEMORY;
    lookupHost2->Assign(hostComponents[last - 2]);
    lookupHost2->Append(".");
    lookupHost2->Append(*lookupHost);
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::GetShaEntries(PRUint32 tableId,
                                              PRUint32 chunkType,
                                              PRUint32 chunkNum,
                                              PRUint32 domainSize,
                                              PRUint32 fragmentSize,
                                              nsACString& chunk,
                                              nsTArray<nsUrlClassifierEntry>& entries)
{
  PRUint32 start = 0;
  while (start + domainSize + 1 <= chunk.Length()) {
    nsUrlClassifierDomainHash domain;
    domain.Assign(Substring(chunk, start, DOMAIN_LENGTH));
    start += domainSize;

    
    PRUint8 numEntries = static_cast<PRUint8>(chunk[start]);
    start++;

    if (numEntries == 0) {
      
      
      if (domainSize != fragmentSize) {
        NS_WARNING("Received 0-fragment entry where domainSize != fragmentSize");
        return NS_ERROR_FAILURE;
      }

      nsUrlClassifierEntry* entry = entries.AppendElement();
      if (!entry) return NS_ERROR_OUT_OF_MEMORY;

      entry->mKey = domain;
      entry->mTableId = tableId;
      entry->mChunkId = chunkNum;
      entry->SetHash(domain);

      if (chunkType == CHUNK_SUB) {
        if (start + 4 > chunk.Length()) {
          
          NS_WARNING("Received a zero-entry sub chunk without an associated add.");
          return NS_ERROR_FAILURE;
        }
        const nsCSubstring& str = Substring(chunk, start, 4);
        PRUint32 p;
        memcpy(&p, str.BeginReading(), 4);
        entry->mAddChunkId = PR_ntohl(p);
        if (entry->mAddChunkId == 0) {
          NS_WARNING("Received invalid chunk number.");
          return NS_ERROR_FAILURE;
        }
        start += 4;
      }
    } else {
      PRUint32 entrySize = fragmentSize;
      if (chunkType == CHUNK_SUB) {
        entrySize += 4;
      }
      if (start + (numEntries * entrySize) > chunk.Length()) {
        
        NS_WARNING("Received a chunk without enough data");
        return NS_ERROR_FAILURE;
      }

      for (PRUint8 i = 0; i < numEntries; i++) {
        nsUrlClassifierEntry* entry = entries.AppendElement();
        if (!entry) return NS_ERROR_OUT_OF_MEMORY;

        entry->mKey = domain;
        entry->mTableId = tableId;
        entry->mChunkId = chunkNum;

        if (chunkType == CHUNK_SUB) {
          const nsCSubstring& str = Substring(chunk, start, 4);
          PRUint32 p;
          memcpy(&p, str.BeginReading(), 4);
          entry->mAddChunkId = PR_ntohl(p);
          if (entry->mAddChunkId == 0) {
            NS_WARNING("Received invalid chunk number.");
            return NS_ERROR_FAILURE;
          }
          start += 4;
        }

        if (fragmentSize == PARTIAL_LENGTH) {
          nsUrlClassifierPartialHash hash;
          hash.Assign(Substring(chunk, start, PARTIAL_LENGTH));
          entry->SetHash(hash);
        } else if (fragmentSize == COMPLETE_LENGTH) {
          nsUrlClassifierCompleteHash hash;
          hash.Assign(Substring(chunk, start, COMPLETE_LENGTH));
          entry->SetHash(hash);
        } else {
          NS_ASSERTION(PR_FALSE, "Invalid fragment size!");
          return NS_ERROR_FAILURE;
        }

        start += fragmentSize;
      }
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::GetChunkEntries(const nsACString& table,
                                                PRUint32 tableId,
                                                PRUint32 chunkType,
                                                PRUint32 chunkNum,
                                                PRUint32 hashSize,
                                                nsACString& chunk,
                                                nsTArray<nsUrlClassifierEntry>& entries)
{
  nsresult rv;
  if (StringEndsWith(table, NS_LITERAL_CSTRING("-exp"))) {
    
    rv = InflateChunk(chunk);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (StringEndsWith(table, NS_LITERAL_CSTRING("-shavar"))) {
    rv = GetShaEntries(tableId, chunkType, chunkNum, DOMAIN_LENGTH, hashSize,
                       chunk, entries);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    nsTArray<nsCString> lines;
    ParseString(PromiseFlatCString(chunk), '\n', lines);

    
    for (PRInt32 i = 0; i < PRInt32(lines.Length()); i++) {
      nsUrlClassifierEntry *entry = entries.AppendElement();
      if (!entry)
        return NS_ERROR_OUT_OF_MEMORY;

      nsCAutoString entryStr;
      if (chunkType == CHUNK_SUB) {
        nsCString::const_iterator begin, iter, end;
        lines[i].BeginReading(begin);
        lines[i].EndReading(end);
        iter = begin;
        if (!FindCharInReadable(':', iter, end) ||
            PR_sscanf(lines[i].get(), "%d:", &entry->mAddChunkId) != 1) {
          NS_WARNING("Received sub chunk without associated add chunk.");
          return NS_ERROR_FAILURE;
        }
        iter++;
        entryStr = Substring(iter, end);
      } else {
        entryStr = lines[i];
      }

      rv = GetKey(entryStr, entry->mKey);
      NS_ENSURE_SUCCESS(rv, rv);

      entry->mTableId = tableId;
      entry->mChunkId = chunkNum;
      if (hashSize == PARTIAL_LENGTH) {
        nsUrlClassifierPartialHash hash;
        hash.FromPlaintext(entryStr, mCryptoHash);
        entry->SetHash(hash);
      } else if (hashSize == COMPLETE_LENGTH) {
        nsUrlClassifierCompleteHash hash;
        hash.FromPlaintext(entryStr, mCryptoHash);
        entry->SetHash(hash);
      } else {
        NS_ASSERTION(PR_FALSE, "Invalid fragment size!");
        return NS_ERROR_FAILURE;
      }
    }
  }

  return NS_OK;
}

PRBool
nsUrlClassifierDBServiceWorker::ParseChunkRange(nsACString::const_iterator &begin,
                                                const nsACString::const_iterator &end,
                                                PRUint32 *first,
                                                PRUint32 *last)
{
  nsACString::const_iterator iter = begin;
  FindCharInReadable(',', iter, end);

  nsCAutoString element(Substring(begin, iter));
  begin = iter;
  if (begin != end)
    begin++;

  PRUint32 numRead = PR_sscanf(element.get(), "%u-%u", first, last);
  if (numRead == 2) {
    if (*first > *last) {
      PRUint32 tmp = *first;
      *first = *last;
      *last = tmp;
    }
    return PR_TRUE;
  }

  if (numRead == 1) {
    *last = *first;
    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
nsUrlClassifierDBServiceWorker::ParseChunkList(const nsACString& chunkStr,
                                               nsTArray<PRUint32>& chunks)
{
  LOG(("Parsing %s", PromiseFlatCString(chunkStr).get()));

  nsACString::const_iterator begin, end;
  chunkStr.BeginReading(begin);
  chunkStr.EndReading(end);
  while (begin != end) {
    PRUint32 first, last;
    if (ParseChunkRange(begin, end, &first, &last)) {
      for (PRUint32 num = first; num <= last; num++) {
        chunks.AppendElement(num);
      }
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
    while (i < chunks.Length() && (chunks[i] == chunks[i - 1] + 1 || chunks[i] == chunks[i - 1])) {
      last = i++;
    }

    if (last != first) {
      chunkStr.Append('-');
      chunkStr.AppendInt(chunks[last]);
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

  nsresult rv = mGetChunkListsStatement->BindInt32ByIndex(0, tableId);
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

  LOG(("Getting chunks for %d, got %s/%s",
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

  mSetChunkListsStatement->BindUTF8StringByIndex(0, addChunks);
  mSetChunkListsStatement->BindUTF8StringByIndex(1, subChunks);
  mSetChunkListsStatement->BindInt32ByIndex(2, tableId);
  nsresult rv = mSetChunkListsStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::CacheChunkLists(PRUint32 tableId,
                                                PRBool parseAdds,
                                                PRBool parseSubs)
{
  nsresult rv;

  if (mHaveCachedLists && mCachedListsTable != tableId) {
    rv = FlushChunkLists();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mHaveCachedLists) {
    rv = GetChunkLists(tableId, mCachedAddsStr, mCachedSubsStr);
    NS_ENSURE_SUCCESS(rv, rv);

    mHaveCachedLists = PR_TRUE;
    mCachedListsTable = tableId;
  }

  if (parseAdds && !mHaveCachedAddChunks) {
    ParseChunkList(mCachedAddsStr, mCachedAddChunks);
    mHaveCachedAddChunks = PR_TRUE;
  }

  if (parseSubs && !mHaveCachedSubChunks) {
    ParseChunkList(mCachedSubsStr, mCachedSubChunks);
    mHaveCachedSubChunks = PR_TRUE;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::FlushChunkLists()
{
  if (!mHaveCachedLists) {
    return NS_OK;
  }

  if (mHaveCachedAddChunks) {
    JoinChunkList(mCachedAddChunks, mCachedAddsStr);
  }

  if (mHaveCachedSubChunks) {
    JoinChunkList(mCachedSubChunks, mCachedSubsStr);
  }

  nsresult rv = SetChunkLists(mCachedListsTable,
                              mCachedAddsStr, mCachedSubsStr);

  
  ClearCachedChunkLists();

  return rv;
}

void
nsUrlClassifierDBServiceWorker::ClearCachedChunkLists()
{
  mCachedAddsStr.Truncate();
  mCachedSubsStr.Truncate();
  mCachedListsTable = PR_UINT32_MAX;
  mHaveCachedLists = PR_FALSE;

  mCachedAddChunks.Clear();
  mHaveCachedAddChunks = PR_FALSE;

  mCachedSubChunks.Clear();
  mHaveCachedSubChunks = PR_FALSE;
}

PRBool
nsUrlClassifierDBServiceWorker::InsertChunkId(nsTArray<PRUint32> &chunks,
                                              PRUint32 chunkNum)
{
  PRUint32 low = 0, high = chunks.Length();
  while (high > low) {
    PRUint32 mid = (high + low) >> 1;
    if (chunks[mid] == chunkNum)
      return PR_FALSE;
    if (chunks[mid] < chunkNum)
      low = mid + 1;
    else
      high = mid;
  }

  PRUint32 *item = chunks.InsertElementAt(low, chunkNum);
  return (item != nsnull);
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

  nsresult rv = CacheChunkLists(tableId, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!InsertChunkId(mCachedAddChunks, chunkNum)) {
    LOG(("Ignoring duplicate add chunk %d in table %d", chunkNum, tableId));
    return NS_OK;
  }

  LOG(("Adding %d entries to chunk %d in table %d", entries.Length(), chunkNum, tableId));

  nsTArray<PRUint32> entryIDs;

  nsAutoTArray<nsUrlClassifierEntry, 5> subEntries;
  rv = mPendingSubStore.ReadSubEntries(tableId, chunkNum, subEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < entries.Length(); i++) {
    nsUrlClassifierEntry& thisEntry = entries[i];

    HandlePendingLookups();

    PRBool writeEntry = PR_TRUE;
    for (PRUint32 j = 0; j < subEntries.Length(); j++) {
      if (thisEntry.SubMatch(subEntries[j])) {
        subEntries.RemoveElementAt(j);

        writeEntry = PR_FALSE;
        break;
      }
    }

    HandlePendingLookups();

    if (writeEntry) {
      rv = mMainStore.WriteEntry(thisEntry);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = mPendingSubStore.ExpireAddChunk(tableId, chunkNum);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(PR_LOGGING)
  if (LOG_ENABLED()) {
    PRIntervalTime clockEnd = PR_IntervalNow();
    LOG(("adding chunk %d took %dms\n", chunkNum,
         PR_IntervalToMilliseconds(clockEnd - clockStart)));
  }
#endif

  return rv;
}

nsresult
nsUrlClassifierStore::Expire(PRUint32 tableId, PRUint32 chunkNum)
{
  LOG(("Expiring chunk %d\n", chunkNum));

  mozStorageStatementScoper expireScoper(mExpireStatement);

  nsresult rv = mExpireStatement->BindInt32ByIndex(0, tableId);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mExpireStatement->BindInt32ByIndex(1, chunkNum);
  NS_ENSURE_SUCCESS(rv, rv);

  mWorker->HandlePendingLookups();

  rv = mExpireStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ExpireAdd(PRUint32 tableId,
                                          PRUint32 chunkNum)
{
  nsresult rv = CacheChunkLists(tableId, PR_TRUE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  mCachedAddChunks.RemoveElement(chunkNum);

  return mMainStore.Expire(tableId, chunkNum);
}

nsresult
nsUrlClassifierDBServiceWorker::SubChunk(PRUint32 tableId,
                                         PRUint32 chunkNum,
                                         nsTArray<nsUrlClassifierEntry>& entries)
{
  nsresult rv = CacheChunkLists(tableId, PR_TRUE, PR_TRUE);

  if (!InsertChunkId(mCachedSubChunks, chunkNum)) {
    LOG(("Ignoring duplicate sub chunk %d in table %d", chunkNum, tableId));
    return NS_OK;
  }

  LOG(("Subbing %d entries in chunk %d in table %d", entries.Length(), chunkNum, tableId));

  for (PRUint32 i = 0; i < entries.Length(); i++) {
    nsAutoTArray<nsUrlClassifierEntry, 5> existingEntries;
    nsUrlClassifierEntry& thisEntry = entries[i];

    HandlePendingLookups();

    
    PRBool haveAdds = (mCachedAddChunks.BinaryIndexOf(thisEntry.mAddChunkId) !=
                       mCachedAddChunks.NoIndex);

    if (haveAdds) {
      rv = mMainStore.ReadAddEntries(thisEntry.mKey, thisEntry.mTableId,
                                     thisEntry.mAddChunkId, existingEntries);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    for (PRUint32 j = 0; j < existingEntries.Length(); j++) {
      if (existingEntries[j].SubMatch(thisEntry)) {
        rv = mMainStore.DeleteEntry(existingEntries[j]);
        NS_ENSURE_SUCCESS(rv, rv);
        existingEntries.RemoveElementAt(j);
        break;
      }
    }

    if (!haveAdds) {
      
      rv = mPendingSubStore.WriteEntry(thisEntry);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ExpireSub(PRUint32 tableId, PRUint32 chunkNum)
{
  nsresult rv = CacheChunkLists(tableId, PR_FALSE, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  mCachedSubChunks.RemoveElement(chunkNum);

  return mPendingSubStore.Expire(tableId, chunkNum);
}

nsresult
nsUrlClassifierDBServiceWorker::ProcessChunk(PRBool* done)
{
  
  if (mPendingStreamUpdate.Length() < static_cast<PRUint32>(mChunkLen)) {
    *done = PR_TRUE;
    return NS_OK;
  }

  nsCAutoString chunk;
  chunk.Assign(Substring(mPendingStreamUpdate, 0, mChunkLen));
  mPendingStreamUpdate = Substring(mPendingStreamUpdate, mChunkLen);

  LOG(("Handling a chunk sized %d", chunk.Length()));

  nsTArray<nsUrlClassifierEntry> entries;
  nsresult rv = GetChunkEntries(mUpdateTable, mUpdateTableId, mChunkType,
                                mChunkNum, mHashSize, chunk, entries);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mChunkType == CHUNK_ADD) {
    rv = AddChunk(mUpdateTableId, mChunkNum, entries);
  } else {
    rv = SubChunk(mUpdateTableId, mChunkNum, entries);
  }

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

    if (mHMAC && mServerMAC.IsEmpty()) {
      
      
      
      if (StringBeginsWith(line, NS_LITERAL_CSTRING("m:"))) {
        mServerMAC = Substring(line, 2);
        nsUrlClassifierUtils::UnUrlsafeBase64(mServerMAC);

        
        const nsCSubstring &toDigest = Substring(updateString, cur);
        rv = mHMAC->Update(reinterpret_cast<const PRUint8*>(toDigest.BeginReading()),
                           toDigest.Length());
        NS_ENSURE_SUCCESS(rv, rv);
      } else if (line.EqualsLiteral("e:pleaserekey")) {
        mUpdateObserver->RekeyRequested();
      } else {
        LOG(("No MAC specified!"));
        return NS_ERROR_FAILURE;
      }
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("n:"))) {
      if (PR_sscanf(PromiseFlatCString(line).get(), "n:%d",
                    &mUpdateWait) != 1) {
        LOG(("Error parsing n: field: %s", PromiseFlatCString(line).get()));
        mUpdateWait = 0;
      }
    } else if (line.EqualsLiteral("r:pleasereset")) {
      mResetRequested = PR_TRUE;
    } else if (line.EqualsLiteral("e:pleaserekey")) {
      mUpdateObserver->RekeyRequested();
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("i:"))) {
      mUpdateTable.Assign(Substring(line, 2));
      GetTableId(mUpdateTable, &mUpdateTableId);
      LOG(("update table: '%s' (%d)", mUpdateTable.get(), mUpdateTableId));
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("u:"))) {
      if (!mPrimaryStream) {
        LOG(("Forwarded update tried to add its own forwarded update."));
        return NS_ERROR_FAILURE;
      }

      const nsCSubstring& data = Substring(line, 2);
      if (mHMAC) {
        
        nsCSubstring::const_iterator begin, end, sepBegin, sepEnd;
        data.BeginReading(begin);
        sepBegin = begin;

        data.EndReading(end);
        sepEnd = end;

        if (!RFindInReadable(NS_LITERAL_CSTRING(","), sepBegin, sepEnd)) {
          NS_WARNING("No MAC specified for a redirect in a request that expects a MAC");
          return NS_ERROR_FAILURE;
        }

        nsCString serverMAC(Substring(sepEnd, end));
        nsUrlClassifierUtils::UnUrlsafeBase64(serverMAC);
        mUpdateObserver->UpdateUrlRequested(Substring(begin, sepBegin),
                                            mUpdateTable,
                                            serverMAC);
      } else {
        
        mUpdateObserver->UpdateUrlRequested(data, mUpdateTable,
                                            NS_LITERAL_CSTRING(""));
      }
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("a:")) ||
               StringBeginsWith(line, NS_LITERAL_CSTRING("s:"))) {
      mState = STATE_CHUNK;
      char command;
      if (PR_sscanf(PromiseFlatCString(line).get(),
                    "%c:%d:%d:%d", &command, &mChunkNum, &mHashSize, &mChunkLen) != 4) {
        return NS_ERROR_FAILURE;
      }

      if (mChunkLen > MAX_CHUNK_SIZE) {
        return NS_ERROR_FAILURE;
      }

      if (!(mHashSize == PARTIAL_LENGTH || mHashSize == COMPLETE_LENGTH)) {
        NS_WARNING("Invalid hash size specified in update.");
        return NS_ERROR_FAILURE;
      }

      mChunkType = (command == 'a') ? CHUNK_ADD : CHUNK_SUB;

      
      *done = PR_FALSE;
      break;
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("ad:"))) {
      const nsCSubstring &list = Substring(line, 3);
      nsACString::const_iterator begin, end;
      list.BeginReading(begin);
      list.EndReading(end);
      while (begin != end) {
        PRUint32 first, last;
        if (ParseChunkRange(begin, end, &first, &last)) {
          for (PRUint32 num = first; num <= last; num++) {
            rv = ExpireAdd(mUpdateTableId, num);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        } else {
          return NS_ERROR_FAILURE;
        }
      }
    } else if (StringBeginsWith(line, NS_LITERAL_CSTRING("sd:"))) {
      const nsCSubstring &list = Substring(line, 3);
      nsACString::const_iterator begin, end;
      list.BeginReading(begin);
      list.EndReading(end);
      while (begin != end) {
        PRUint32 first, last;
        if (ParseChunkRange(begin, end, &first, &last)) {
          for (PRUint32 num = first; num <= last; num++) {
            rv = ExpireSub(mUpdateTableId, num);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        } else {
          return NS_ERROR_FAILURE;
        }
      }
    } else {
      LOG(("ignoring unknown line: '%s'", PromiseFlatCString(line).get()));
    }
  }

  mPendingStreamUpdate = Substring(updateString, cur);

  return NS_OK;
}

void
nsUrlClassifierDBServiceWorker::ResetStream()
{
  mState = STATE_LINE;
  mChunkNum = 0;
  mHashSize = 0;
  mChunkLen = 0;
  mInStream = PR_FALSE;
  mPrimaryStream = PR_FALSE;
  mUpdateTable.Truncate();
  mPendingStreamUpdate.Truncate();
  mServerMAC.Truncate();
  mHMAC = nsnull;
}

void
nsUrlClassifierDBServiceWorker::ResetUpdate()
{
  mUpdateWait = 0;
  mUpdateStatus = NS_OK;
  mUpdateObserver = nsnull;
  mUpdateClientKey.Truncate();
  mResetRequested = PR_FALSE;
  mUpdateTables.Clear();
}

void
nsUrlClassifierDBServiceWorker::ResetLookupCache()
{
    mCachedHostKey.Truncate();
    mCachedEntries.Clear();

    mCleanFragments.Clear();

    MutexAutoLock lock(mCleanHostKeysLock);
    mCleanHostKeys.Clear();
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::SetHashCompleter(const nsACString &tableName,
                                                 nsIUrlClassifierHashCompleter *completer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::BeginUpdate(nsIUrlClassifierUpdateObserver *observer,
                                            const nsACString &tables,
                                            const nsACString &clientKey)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(!mUpdateObserver);

  nsresult rv = OpenDb();
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to open database");
    return NS_ERROR_FAILURE;
  }

  PRBool transaction;
  rv = mConnection->GetTransactionInProgress(&transaction);
  if (NS_FAILED(rv)) {
    mUpdateStatus = rv;
    return rv;
  }

  if (transaction) {
    NS_WARNING("Transaction already in progress in nsUrlClassifierDBServiceWorker::BeginUpdate.  Cancelling update.");
    mUpdateStatus = NS_ERROR_FAILURE;
    return rv;
  }

  rv = SetupUpdate();
  if (NS_FAILED(rv)) {
    mUpdateStatus = rv;
    return rv;
  }

  mUpdateObserver = observer;

  if (!clientKey.IsEmpty()) {
    rv = nsUrlClassifierUtils::DecodeClientKey(clientKey, mUpdateClientKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  mPrimaryStream = PR_TRUE;

  SplitTables(tables, mUpdateTables);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::BeginStream(const nsACString &table,
                                            const nsACString &serverMAC)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(mUpdateObserver);
  NS_ENSURE_STATE(!mInStream);

  
  
  nsresult rv = SetupUpdate();
  if (NS_FAILED(rv)) {
    mUpdateStatus = rv;
    return rv;
  }

  mInStream = PR_TRUE;

  
  if (!mUpdateClientKey.IsEmpty()) {
    nsCOMPtr<nsIKeyObjectFactory> keyObjectFactory(do_GetService(
        "@mozilla.org/security/keyobjectfactory;1", &rv));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to get nsIKeyObjectFactory service");
      mUpdateStatus = rv;
      return mUpdateStatus;
    }

    nsCOMPtr<nsIKeyObject> keyObject;
    rv = keyObjectFactory->KeyFromString(nsIKeyObject::HMAC, mUpdateClientKey, 
        getter_AddRefs(keyObject));
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create key object, maybe not FIPS compliant?");
      mUpdateStatus = rv;
      return mUpdateStatus;
    }

    mHMAC = do_CreateInstance(NS_CRYPTO_HMAC_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to create nsICryptoHMAC instance");
      mUpdateStatus = rv;
      return mUpdateStatus;
    }

    rv = mHMAC->Init(nsICryptoHMAC::SHA1, keyObject);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to initialize nsICryptoHMAC instance");
      mUpdateStatus = rv;
      return mUpdateStatus;
    }
  }

  mServerMAC = serverMAC;

  if (!table.IsEmpty()) {
    mUpdateTable = table;
    GetTableId(mUpdateTable, &mUpdateTableId);
    LOG(("update table: '%s' (%d)", mUpdateTable.get(), mUpdateTableId));
  }

  return NS_OK;
}
































NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::UpdateStream(const nsACString& chunk)
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(mInStream);

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

  if (mHMAC && !mServerMAC.IsEmpty()) {
    rv = mHMAC->Update(reinterpret_cast<const PRUint8*>(chunk.BeginReading()),
                       chunk.Length());
    if (NS_FAILED(rv)) {
      mUpdateStatus = rv;
      return mUpdateStatus;
    }
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
nsUrlClassifierDBServiceWorker::FinishStream()
{
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(mInStream);
  NS_ENSURE_STATE(mUpdateObserver);

  PRInt32 nextStreamDelay = 0;

  if (NS_SUCCEEDED(mUpdateStatus) && mHMAC) {
    nsCAutoString clientMAC;
    mHMAC->Finish(PR_TRUE, clientMAC);

    if (clientMAC != mServerMAC) {
      NS_WARNING("Invalid update MAC!");
      LOG(("Invalid update MAC: expected %s, got %s",
           mServerMAC.get(), clientMAC.get()));
      mUpdateStatus = NS_ERROR_FAILURE;
    }
    PRIntervalTime updateTime = PR_IntervalNow() - mUpdateStartTime;
    if (PR_IntervalToSeconds(updateTime) >=
        static_cast<PRUint32>(gWorkingTimeThreshold)) {
      
      
      ApplyUpdate();

      nextStreamDelay = gDelayTime * 1000;
    }
  }

  mUpdateObserver->StreamFinished(mUpdateStatus,
                                  static_cast<PRUint32>(nextStreamDelay));

  ResetStream();

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::SetupUpdate()
{
  LOG(("nsUrlClassifierDBServiceWorker::SetupUpdate"));
  PRBool inProgress;
  nsresult rv = mConnection->GetTransactionInProgress(&inProgress);
  if (inProgress) {
    return NS_OK;
  }

  mUpdateStartTime = PR_IntervalNow();

  rv = mConnection->BeginTransaction();
  NS_ENSURE_SUCCESS(rv, rv);

  if (gUpdateCacheSize > 0) {
    PRBool hasResult;
    rv = mGetPageSizeStatement->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(hasResult, "Should always be able to get page size from sqlite");
    PRUint32 pageSize = mGetPageSizeStatement->AsInt32(0);
    PRUint32 cachePages = gUpdateCacheSize / pageSize;
    nsCAutoString cacheSizePragma("PRAGMA cache_size=");
    cacheSizePragma.AppendInt(cachePages);
    rv = mConnection->ExecuteSimpleSQL(cacheSizePragma);
    NS_ENSURE_SUCCESS(rv, rv);
    mGrewCache = PR_TRUE;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierDBServiceWorker::ApplyUpdate()
{
  LOG(("nsUrlClassifierDBServiceWorker::ApplyUpdate"));

  if (mConnection) {
    if (NS_FAILED(mUpdateStatus)) {
      mConnection->RollbackTransaction();
    } else {
      mUpdateStatus = FlushChunkLists();
      if (NS_SUCCEEDED(mUpdateStatus)) {
        mUpdateStatus = mConnection->CommitTransaction();
      }
    }
  }

  if (NS_SUCCEEDED(mUpdateStatus)) {
    
    
    ResetLookupCache();
  }

  if (mGrewCache) {
    
    
    
    mGrewCache = PR_FALSE;
    CloseDb();
    OpenDb();
  }

  mUpdateStartTime = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::FinishUpdate()
{
  LOG(("nsUrlClassifierDBServiceWorker::FinishUpdate()"));
  if (gShuttingDownThread)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_STATE(!mInStream);
  NS_ENSURE_STATE(mUpdateObserver);

  
  
  PRInt32 errcode = SQLITE_OK;
  if (mConnection)
    mConnection->GetLastError(&errcode);

  ApplyUpdate();

  if (NS_SUCCEEDED(mUpdateStatus)) {
    mUpdateObserver->UpdateSuccess(mUpdateWait);
  } else {
    mUpdateObserver->UpdateError(mUpdateStatus);
  }

  
  
  
  PRBool resetDB = (NS_SUCCEEDED(mUpdateStatus) && mResetRequested) ||
                    errcode == SQLITE_CORRUPT;

  if (!resetDB) {
    if (NS_SUCCEEDED(mUpdateStatus)) {
      PRInt64 now = (PR_Now() / PR_USEC_PER_SEC);
      for (PRUint32 i = 0; i < mUpdateTables.Length(); i++) {
        LOG(("Successfully updated %s", mUpdateTables[i].get()));
        mTableFreshness.Put(mUpdateTables[i], now);
      }
    } else {
      for (PRUint32 i = 0; i < mUpdateTables.Length(); i++) {
        LOG(("Failed updating %s", mUpdateTables[i].get()));
        mTableFreshness.Remove(mUpdateTables[i]);
      }
    }
  }

  ResetUpdate();

  if (resetDB) {
    ResetDatabase();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::ResetDatabase()
{
  LOG(("nsUrlClassifierDBServiceWorker::ResetDatabase [%p]", this));
  ClearCachedChunkLists();

  mTableFreshness.Clear();
  ResetLookupCache();

  nsresult rv = CloseDb();
  NS_ENSURE_SUCCESS(rv, rv);

  mDBFile->Remove(PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CancelUpdate()
{
  LOG(("CancelUpdate"));

  if (mUpdateObserver) {
    mUpdateStatus = NS_BINDING_ABORTED;

    ClearCachedChunkLists();
    mConnection->RollbackTransaction();
    mUpdateObserver->UpdateError(mUpdateStatus);

    for (PRUint32 i = 0; i < mUpdateTables.Length(); i++) {
      LOG(("Failed updating %s", mUpdateTables[i].get()));
      mTableFreshness.Remove(mUpdateTables[i]);
    }

    ResetStream();
    ResetUpdate();
  }

  return NS_OK;
}





NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CloseDb()
{
  if (mConnection) {
    mMainStore.Close();
    mPendingSubStore.Close();

    mGetChunkListsStatement = nsnull;
    mSetChunkListsStatement = nsnull;

    mGetTablesStatement = nsnull;
    mGetTableIdStatement = nsnull;
    mGetTableNameStatement = nsnull;
    mInsertTableIdStatement = nsnull;
    mGetPageSizeStatement = nsnull;

    mConnection = nsnull;
    LOG(("urlclassifier db closed\n"));
  }

  mCryptoHash = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBServiceWorker::CacheCompletions(nsTArray<nsUrlClassifierLookupResult> *results)
{
  LOG(("nsUrlClassifierDBServiceWorker::CacheCompletions [%p]", this));

  nsAutoPtr<nsTArray<nsUrlClassifierLookupResult> > resultsPtr(results);

  
  
  
  mozStorageTransaction trans(mConnection, PR_TRUE);

  for (PRUint32 i = 0; i < results->Length(); i++) {
    nsUrlClassifierLookupResult& result = results->ElementAt(i);
    
    
    
    mMainStore.UpdateEntry(result.mEntry);
  }

  
  
  mCachedHostKey.Truncate();
  mCachedEntries.Clear();

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

  PRBool exists;
  rv = mDBFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool newDB = !exists;

  nsCOMPtr<mozIStorageConnection> connection;
  rv = storageService->OpenDatabase(mDBFile, getter_AddRefs(connection));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    rv = mDBFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    newDB = PR_TRUE;

    rv = storageService->OpenDatabase(mDBFile, getter_AddRefs(connection));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (!newDB) {
    PRInt32 databaseVersion;
    rv = connection->GetSchemaVersion(&databaseVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    if (databaseVersion != IMPLEMENTATION_VERSION) {
      LOG(("Incompatible database, removing."));

      rv = connection->Close();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDBFile->Remove(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);

      newDB = PR_TRUE;

      rv = storageService->OpenDatabase(mDBFile, getter_AddRefs(connection));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  connection->SetGrowthIncrement(5 * 1024 * 1024, EmptyCString());
  rv = connection->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA synchronous=OFF"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (newDB) {
    rv = connection->SetSchemaVersion(IMPLEMENTATION_VERSION);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = MaybeCreateTables(connection);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainStore.Init(this, connection,
                       NS_LITERAL_CSTRING("moz_classifier"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPendingSubStore.Init(this, connection,
                             NS_LITERAL_CSTRING("moz_subs"));
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

  rv = connection->CreateStatement
    (NS_LITERAL_CSTRING("PRAGMA page_size"),
     getter_AddRefs(mGetPageSizeStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  mConnection = connection;

  mCryptoHash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

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
                       "  partial_data BLOB,"
                       "  complete_data BLOB,"
                       "  chunk_id INTEGER,"
                       "  table_id INTEGER)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS"
                       " moz_classifier_domain_index"
                       " ON moz_classifier(domain)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS"
                       " moz_classifier_chunk_index"
                       " ON moz_classifier(chunk_id)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_subs"
                       " (id INTEGER PRIMARY KEY,"
                       "  domain BLOB,"
                       "  partial_data BLOB,"
                       "  complete_data BLOB,"
                       "  chunk_id INTEGER,"
                       "  table_id INTEGER,"
                       "  add_chunk_id INTEGER)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS"
                       " moz_subs_addchunk_index"
                       " ON moz_subs(add_chunk_id)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE INDEX IF NOT EXISTS"
                       " moz_subs_chunk_index"
                       " ON moz_subs(chunk_id)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = connection->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("CREATE TABLE IF NOT EXISTS moz_tables"
                       " (id INTEGER PRIMARY KEY,"
                       "  name TEXT,"
                       "  add_chunks TEXT,"
                       "  sub_chunks TEXT);"));
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}








class nsUrlClassifierLookupCallback : public nsIUrlClassifierLookupCallback
                                    , public nsIUrlClassifierHashCompleterCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERLOOKUPCALLBACK
  NS_DECL_NSIURLCLASSIFIERHASHCOMPLETERCALLBACK

  nsUrlClassifierLookupCallback(nsUrlClassifierDBService *dbservice,
                                nsIUrlClassifierCallback *c)
    : mDBService(dbservice)
    , mResults(nsnull)
    , mPendingCompletions(0)
    , mCallback(c)
    {}

private:
  nsresult HandleResults();

  nsRefPtr<nsUrlClassifierDBService> mDBService;
  nsAutoPtr<nsTArray<nsUrlClassifierLookupResult> > mResults;

  
  nsAutoPtr<nsTArray<nsUrlClassifierLookupResult> > mCacheResults;

  PRUint32 mPendingCompletions;
  nsCOMPtr<nsIUrlClassifierCallback> mCallback;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsUrlClassifierLookupCallback,
                              nsIUrlClassifierLookupCallback,
                              nsIUrlClassifierHashCompleterCallback)

NS_IMETHODIMP
nsUrlClassifierLookupCallback::LookupComplete(nsTArray<nsUrlClassifierLookupResult>* results)
{
  NS_ASSERTION(mResults == nsnull,
               "Should only get one set of results per nsUrlClassifierLookupCallback!");

  if (!results) {
    HandleResults();
    return NS_OK;
  }

  mResults = results;
  mResults->Sort();

  
  for (PRUint32 i = 0; i < results->Length(); i++) {
    nsUrlClassifierLookupResult& result = results->ElementAt(i);

    
    if (!result.mConfirmed) {
      nsCOMPtr<nsIUrlClassifierHashCompleter> completer;
      if (mDBService->GetCompleter(result.mTableName,
                                   getter_AddRefs(completer))) {
        nsCAutoString partialHash;
        PRUint8 *buf =
          result.mEntry.mHavePartial ? result.mEntry.mPartialHash.buf
                                     : result.mEntry.mCompleteHash.buf;
        partialHash.Assign(reinterpret_cast<char*>(buf), PARTIAL_LENGTH);

        nsresult rv = completer->Complete(partialHash, this);
        if (NS_SUCCEEDED(rv)) {
          mPendingCompletions++;
        }
      } else {
        
        
        
        
        if (result.mEntry.mHaveComplete
            && (result.mLookupFragment == result.mEntry.mCompleteHash)) {
          result.mConfirmed = PR_TRUE;
        } else {
          NS_WARNING("Partial match in a table without a valid completer, ignoring partial match.");
        }
      }
    }
  }

  if (mPendingCompletions == 0) {
    
    HandleResults();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierLookupCallback::CompletionFinished(nsresult status)
{
  LOG(("nsUrlClassifierLookupCallback::CompletionFinished [%p, %08x]",
       this, status));
  if (NS_FAILED(status)) {
    NS_WARNING("gethash response failed.");
  }

  mPendingCompletions--;
  if (mPendingCompletions == 0) {
    HandleResults();

    if (mCacheResults) {
      
      
      mDBService->CacheCompletions(mCacheResults.forget());
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierLookupCallback::Completion(const nsACString& completeHash,
                                          const nsACString& tableName,
                                          PRUint32 chunkId,
                                          PRBool verified)
{
  LOG(("nsUrlClassifierLookupCallback::Completion [%p, %s, %d, %d]",
       this, PromiseFlatCString(tableName).get(), chunkId, verified));
  nsUrlClassifierCompleteHash hash;
  hash.Assign(completeHash);

  for (PRUint32 i = 0; i < mResults->Length(); i++) {
    nsUrlClassifierLookupResult& result = mResults->ElementAt(i);

    
    if (verified &&
        !result.mEntry.mHaveComplete &&
        hash.StartsWith(result.mEntry.mPartialHash) &&
        result.mTableName == tableName &&
        result.mEntry.mChunkId == chunkId) {
      
      result.mEntry.SetHash(hash);

      if (!mCacheResults) {
        mCacheResults = new nsTArray<nsUrlClassifierLookupResult>();
        if (!mCacheResults)
          return NS_ERROR_OUT_OF_MEMORY;
      }

      mCacheResults->AppendElement(result);
    }

    
    if (result.mLookupFragment == hash) {
      result.mConfirmed = PR_TRUE;

      if (result.mTableName != tableName ||
          result.mEntry.mChunkId != chunkId) {
        
        
        
        
        
        
        
        
        
        result.mTableName = tableName;
        NS_WARNING("Accepting a gethash with an invalid table name or chunk id");
      }
    }
  }

  return NS_OK;
}

nsresult
nsUrlClassifierLookupCallback::HandleResults()
{
  if (!mResults) {
    
    return mCallback->HandleEvent(NS_LITERAL_CSTRING(""));
  }

  
  mResults->Sort();
  PRUint32 lastTableId = 0;
  nsCAutoString tables;
  for (PRUint32 i = 0; i < mResults->Length(); i++) {
    nsUrlClassifierLookupResult& result = mResults->ElementAt(i);
    
    
    
    if (!result.mConfirmed || result.mNoise)
      continue;

    if (tables.Length() > 0) {
      if (lastTableId == result.mEntry.mTableId)
        continue;
      tables.Append(",");
    }

    tables.Append(result.mTableName);
    lastTableId = result.mEntry.mTableId;
  }

  return mCallback->HandleEvent(tables);
}






class nsUrlClassifierClassifyCallback : public nsIUrlClassifierCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERCALLBACK

  nsUrlClassifierClassifyCallback(nsIURIClassifierCallback *c,
                                  PRBool checkMalware,
                                  PRBool checkPhishing)
    : mCallback(c)
    , mCheckMalware(checkMalware)
    , mCheckPhishing(checkPhishing)
    {}

private:
  nsCOMPtr<nsIURIClassifierCallback> mCallback;
  PRPackedBool mCheckMalware;
  PRPackedBool mCheckPhishing;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUrlClassifierClassifyCallback,
                              nsIUrlClassifierCallback)

NS_IMETHODIMP
nsUrlClassifierClassifyCallback::HandleEvent(const nsACString& tables)
{
  
  
  
  nsresult response = NS_OK;

  nsACString::const_iterator begin, end;

  tables.BeginReading(begin);
  tables.EndReading(end);
  if (mCheckMalware &&
      FindInReadable(NS_LITERAL_CSTRING("-malware-"), begin, end)) {
    response = NS_ERROR_MALWARE_URI;
  } else {
    
    tables.BeginReading(begin);

    if (mCheckPhishing &&
        FindInReadable(NS_LITERAL_CSTRING("-phish-"), begin, end)) {
      response = NS_ERROR_PHISHING_URI;
    }
  }

  mCallback->OnClassifyComplete(response);

  return NS_OK;
}





NS_IMPL_THREADSAFE_ISUPPORTS3(nsUrlClassifierDBService,
                              nsIUrlClassifierDBService,
                              nsIURIClassifier,
                              nsIObserver)

 nsUrlClassifierDBService*
nsUrlClassifierDBService::GetInstance(nsresult *result)
{
  *result = NS_OK;
  if (!sUrlClassifierDBService) {
    sUrlClassifierDBService = new nsUrlClassifierDBService();
    if (!sUrlClassifierDBService) {
      *result = NS_ERROR_OUT_OF_MEMORY;
      return nsnull;
    }

    NS_ADDREF(sUrlClassifierDBService);   

    *result = sUrlClassifierDBService->Init();
    if (NS_FAILED(*result)) {
      NS_RELEASE(sUrlClassifierDBService);
      return nsnull;
    }
  } else {
    
    NS_ADDREF(sUrlClassifierDBService);   
  }
  return sUrlClassifierDBService;
}


nsUrlClassifierDBService::nsUrlClassifierDBService()
 : mCheckMalware(CHECK_MALWARE_DEFAULT)
 , mCheckPhishing(CHECK_PHISHING_DEFAULT)
 , mInUpdate(PR_FALSE)
{
}

nsUrlClassifierDBService::~nsUrlClassifierDBService()
{
  sUrlClassifierDBService = nsnull;
}

nsresult
nsUrlClassifierDBService::Init()
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierDbServiceLog)
    gUrlClassifierDbServiceLog = PR_NewLogModule("UrlClassifierDbService");
#endif

  
  nsresult rv;
  nsCOMPtr<mozIStorageService> storageService =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsICryptoHash> hash =
    do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);

  PRInt32 gethashNoise = 0;
  if (prefs) {
    PRBool tmpbool;
    rv = prefs->GetBoolPref(CHECK_MALWARE_PREF, &tmpbool);
    mCheckMalware = NS_SUCCEEDED(rv) ? tmpbool : CHECK_MALWARE_DEFAULT;

    prefs->AddObserver(CHECK_MALWARE_PREF, this, PR_FALSE);

    rv = prefs->GetBoolPref(CHECK_PHISHING_PREF, &tmpbool);
    mCheckPhishing = NS_SUCCEEDED(rv) ? tmpbool : CHECK_PHISHING_DEFAULT;

    prefs->AddObserver(CHECK_PHISHING_PREF, this, PR_FALSE);

    if (NS_FAILED(prefs->GetIntPref(GETHASH_NOISE_PREF, &gethashNoise))) {
      gethashNoise = GETHASH_NOISE_DEFAULT;
    }

    nsXPIDLCString tmpstr;
    if (NS_SUCCEEDED(prefs->GetCharPref(GETHASH_TABLES_PREF, getter_Copies(tmpstr)))) {
      SplitTables(tmpstr, mGethashWhitelist);
    }

    prefs->AddObserver(GETHASH_TABLES_PREF, this, PR_FALSE);

    PRInt32 tmpint;
    rv = prefs->GetIntPref(CONFIRM_AGE_PREF, &tmpint);
    PR_ATOMIC_SET(&gFreshnessGuarantee, NS_SUCCEEDED(rv) ? tmpint : CONFIRM_AGE_DEFAULT_SEC);

    prefs->AddObserver(CONFIRM_AGE_PREF, this, PR_FALSE);

    rv = prefs->GetIntPref(UPDATE_CACHE_SIZE_PREF, &tmpint);
    PR_ATOMIC_SET(&gUpdateCacheSize, NS_SUCCEEDED(rv) ? tmpint : UPDATE_CACHE_SIZE_DEFAULT);

    rv = prefs->GetIntPref(UPDATE_WORKING_TIME, &tmpint);
    PR_ATOMIC_SET(&gWorkingTimeThreshold,
                  NS_SUCCEEDED(rv) ? tmpint : UPDATE_WORKING_TIME_DEFAULT);

    rv = prefs->GetIntPref(UPDATE_DELAY_TIME, &tmpint);
    PR_ATOMIC_SET(&gDelayTime,
                  NS_SUCCEEDED(rv) ? tmpint : UPDATE_DELAY_TIME_DEFAULT);
  }

  
  rv = NS_NewThread(&gDbBackgroundThread);
  if (NS_FAILED(rv))
    return rv;

  mWorker = new nsUrlClassifierDBServiceWorker();
  if (!mWorker)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = mWorker->Init(gethashNoise);
  if (NS_FAILED(rv)) {
    mWorker = nsnull;
    return rv;
  }

  
  rv = NS_GetProxyForObject(gDbBackgroundThread,
                            NS_GET_IID(nsIUrlClassifierDBServiceWorker),
                            mWorker,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(mWorkerProxy));
  NS_ENSURE_SUCCESS(rv, rv);

  mCompleters.Init();

  
  nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, "profile-before-change", PR_FALSE);
  observerService->AddObserver(this, "xpcom-shutdown-threads", PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::Classify(nsIURI *uri,
                                   nsIURIClassifierCallback* c,
                                   PRBool* result)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  if (!(mCheckMalware || mCheckPhishing)) {
    *result = PR_FALSE;
    return NS_OK;
  }

  nsRefPtr<nsUrlClassifierClassifyCallback> callback =
    new nsUrlClassifierClassifyCallback(c, mCheckMalware, mCheckPhishing);
  if (!callback) return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = LookupURI(uri, callback, PR_FALSE, result);
  if (rv == NS_ERROR_MALFORMED_URI) {
    *result = PR_FALSE;
    
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::Lookup(const nsACString& spec,
                                 nsIUrlClassifierCallback* c)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIURI> uri;

  nsresult rv = NS_NewURI(getter_AddRefs(uri), spec);
  NS_ENSURE_SUCCESS(rv, rv);

  uri = NS_GetInnermostURI(uri);
  if (!uri) {
    return NS_ERROR_FAILURE;
  }

  PRBool didLookup;
  return LookupURI(uri, c, PR_TRUE, &didLookup);
}

nsresult
nsUrlClassifierDBService::LookupURI(nsIURI* uri,
                                    nsIUrlClassifierCallback* c,
                                    PRBool forceLookup,
                                    PRBool *didLookup)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  nsCAutoString key;
  
  nsCOMPtr<nsIUrlClassifierUtils> utilsService =
    do_GetService(NS_URLCLASSIFIERUTILS_CONTRACTID);
  nsresult rv = utilsService->GetKeyForURI(uri, key);
  if (NS_FAILED(rv))
    return rv;

  if (forceLookup) {
    *didLookup = PR_TRUE;
  } else {
    
    
    PRBool clean;
    rv = mWorker->CheckCleanHost(key, &clean);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!clean) {
      nsCOMPtr<nsIPermissionManager> permissionManager =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

      if (permissionManager) {
        PRUint32 perm;
        permissionManager->TestPermission(uri, "safe-browsing", &perm);
        clean |= (perm == nsIPermissionManager::ALLOW_ACTION);
      }
    }

    *didLookup = !clean;
    if (clean) {
      return NS_OK;
    }
  }

  
  
  
  nsCOMPtr<nsIUrlClassifierLookupCallback> callback =
    new nsUrlClassifierLookupCallback(this, c);
  if (!callback)
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIUrlClassifierLookupCallback> proxyCallback;
  
  rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsIUrlClassifierLookupCallback),
                            callback,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxyCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mWorker->QueueLookup(key, proxyCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  return mWorkerProxy->Lookup(EmptyCString(), nsnull);
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

  return mWorkerProxy->GetTables(proxyCallback);
}

NS_IMETHODIMP
nsUrlClassifierDBService::SetHashCompleter(const nsACString &tableName,
                                           nsIUrlClassifierHashCompleter *completer)
{
  if (completer) {
    if (!mCompleters.Put(tableName, completer)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    mCompleters.Remove(tableName);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierDBService::BeginUpdate(nsIUrlClassifierUpdateObserver *observer,
                                      const nsACString &updateTables,
                                      const nsACString &clientKey)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  if (mInUpdate)
    return NS_ERROR_NOT_AVAILABLE;

  mInUpdate = PR_TRUE;

  nsresult rv;

  
  nsCOMPtr<nsIUrlClassifierUpdateObserver> proxyObserver;
  rv = NS_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                            NS_GET_IID(nsIUrlClassifierUpdateObserver),
                            observer,
                            NS_PROXY_ASYNC,
                            getter_AddRefs(proxyObserver));
  NS_ENSURE_SUCCESS(rv, rv);

  return mWorkerProxy->BeginUpdate(proxyObserver, updateTables, clientKey);
}

NS_IMETHODIMP
nsUrlClassifierDBService::BeginStream(const nsACString &table,
                                      const nsACString &serverMAC)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->BeginStream(table, serverMAC);
}

NS_IMETHODIMP
nsUrlClassifierDBService::UpdateStream(const nsACString& aUpdateChunk)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->UpdateStream(aUpdateChunk);
}

NS_IMETHODIMP
nsUrlClassifierDBService::FinishStream()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->FinishStream();
}

NS_IMETHODIMP
nsUrlClassifierDBService::FinishUpdate()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  mInUpdate = PR_FALSE;

  return mWorkerProxy->FinishUpdate();
}


NS_IMETHODIMP
nsUrlClassifierDBService::CancelUpdate()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  mInUpdate = PR_FALSE;

  return mWorkerProxy->CancelUpdate();
}

NS_IMETHODIMP
nsUrlClassifierDBService::ResetDatabase()
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->ResetDatabase();
}

nsresult
nsUrlClassifierDBService::CacheCompletions(nsTArray<nsUrlClassifierLookupResult> *results)
{
  NS_ENSURE_TRUE(gDbBackgroundThread, NS_ERROR_NOT_INITIALIZED);

  return mWorkerProxy->CacheCompletions(results);
}

PRBool
nsUrlClassifierDBService::GetCompleter(const nsACString &tableName,
                                       nsIUrlClassifierHashCompleter **completer)
{
  if (mCompleters.Get(tableName, completer)) {
    return PR_TRUE;
  }

  if (!mGethashWhitelist.Contains(tableName)) {
    return PR_FALSE;
  }

  return NS_SUCCEEDED(CallGetService(NS_URLCLASSIFIERHASHCOMPLETER_CONTRACTID,
                                     completer));
}

NS_IMETHODIMP
nsUrlClassifierDBService::Observe(nsISupports *aSubject, const char *aTopic,
                                  const PRUnichar *aData)
{
  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> prefs(do_QueryInterface(aSubject, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    if (NS_LITERAL_STRING(CHECK_MALWARE_PREF).Equals(aData)) {
      PRBool tmpbool;
      rv = prefs->GetBoolPref(CHECK_MALWARE_PREF, &tmpbool);
      mCheckMalware = NS_SUCCEEDED(rv) ? tmpbool : CHECK_MALWARE_DEFAULT;
    } else if (NS_LITERAL_STRING(CHECK_PHISHING_PREF).Equals(aData)) {
      PRBool tmpbool;
      rv = prefs->GetBoolPref(CHECK_PHISHING_PREF, &tmpbool);
      mCheckPhishing = NS_SUCCEEDED(rv) ? tmpbool : CHECK_PHISHING_DEFAULT;
    } else if (NS_LITERAL_STRING(GETHASH_TABLES_PREF).Equals(aData)) {
      mGethashWhitelist.Clear();
      nsXPIDLCString val;
      if (NS_SUCCEEDED(prefs->GetCharPref(GETHASH_TABLES_PREF, getter_Copies(val)))) {
        SplitTables(val, mGethashWhitelist);
      }
    } else if (NS_LITERAL_STRING(CONFIRM_AGE_PREF).Equals(aData)) {
      PRInt32 tmpint;
      rv = prefs->GetIntPref(CONFIRM_AGE_PREF, &tmpint);
      PR_ATOMIC_SET(&gFreshnessGuarantee, NS_SUCCEEDED(rv) ? tmpint : CONFIRM_AGE_DEFAULT_SEC);
    } else if (NS_LITERAL_STRING(UPDATE_CACHE_SIZE_PREF).Equals(aData)) {
      PRInt32 tmpint;
      rv = prefs->GetIntPref(UPDATE_CACHE_SIZE_PREF, &tmpint);
      PR_ATOMIC_SET(&gUpdateCacheSize, NS_SUCCEEDED(rv) ? tmpint : UPDATE_CACHE_SIZE_DEFAULT);
    } else if (NS_LITERAL_STRING(UPDATE_WORKING_TIME).Equals(aData)) {
      PRInt32 tmpint;
      rv = prefs->GetIntPref(UPDATE_WORKING_TIME, &tmpint);
      PR_ATOMIC_SET(&gWorkingTimeThreshold,
                    NS_SUCCEEDED(rv) ? tmpint : UPDATE_WORKING_TIME_DEFAULT);
    } else if (NS_LITERAL_STRING(UPDATE_DELAY_TIME).Equals(aData)) {
      PRInt32 tmpint;
      rv = prefs->GetIntPref(UPDATE_DELAY_TIME, &tmpint);
      PR_ATOMIC_SET(&gDelayTime,
                    NS_SUCCEEDED(rv) ? tmpint : UPDATE_DELAY_TIME_DEFAULT);
    }
  } else if (!strcmp(aTopic, "profile-before-change") ||
             !strcmp(aTopic, "xpcom-shutdown-threads")) {
    Shutdown();
  } else {
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}


nsresult
nsUrlClassifierDBService::Shutdown()
{
  LOG(("shutting down db service\n"));

  if (!gDbBackgroundThread)
    return NS_OK;

  mCompleters.Clear();

  nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    prefs->RemoveObserver(CHECK_MALWARE_PREF, this);
    prefs->RemoveObserver(CHECK_PHISHING_PREF, this);
    prefs->RemoveObserver(GETHASH_TABLES_PREF, this);
    prefs->RemoveObserver(CONFIRM_AGE_PREF, this);
  }

  nsresult rv;
  
  if (mWorker) {
    rv = mWorkerProxy->CancelUpdate();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post cancel update event");
    rv = mWorkerProxy->CloseDb();
    NS_ASSERTION(NS_SUCCEEDED(rv), "failed to post close db event");
  }

  mWorkerProxy = nsnull;

  LOG(("joining background thread"));

  gShuttingDownThread = PR_TRUE;

  nsIThread *backgroundThread = gDbBackgroundThread;
  gDbBackgroundThread = nsnull;
  backgroundThread->Shutdown();
  NS_RELEASE(backgroundThread);

  return NS_OK;
}
