





#include "mozilla/dom/cache/DBSchema.h"

#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/InternalHeaders.h"
#include "mozilla/dom/cache/CacheTypes.h"
#include "mozilla/dom/cache/SavedTypes.h"
#include "mozilla/dom/cache/Types.h"
#include "mozilla/dom/cache/TypeUtils.h"
#include "mozIStorageConnection.h"
#include "mozIStorageStatement.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsCRT.h"
#include "nsHttp.h"
#include "nsICryptoHash.h"
#include "mozilla/BasePrincipal.h"
#include "mozilla/dom/HeadersBinding.h"
#include "mozilla/dom/RequestBinding.h"
#include "mozilla/dom/ResponseBinding.h"
#include "nsIContentPolicy.h"

namespace mozilla {
namespace dom {
namespace cache {
namespace db {

const int32_t kMaxWipeSchemaVersion = 14;

namespace {

const int32_t kLatestSchemaVersion = 14;
const int32_t kMaxEntriesPerStatement = 255;

const uint32_t kPageSize = 4 * 1024;


const uint32_t kGrowthSize = 32 * 1024;
const uint32_t kGrowthPages = kGrowthSize / kPageSize;
static_assert(kGrowthSize % kPageSize == 0,
              "Growth size must be multiple of page size");


const int32_t kMaxFreePages = kGrowthPages;


const uint32_t kWalAutoCheckpointSize = 512 * 1024;
const uint32_t kWalAutoCheckpointPages = kWalAutoCheckpointSize / kPageSize;
static_assert(kWalAutoCheckpointSize % kPageSize == 0,
              "WAL checkpoint size must be multiple of page size");

} 





static_assert(int(HeadersGuardEnum::None) == 0 &&
              int(HeadersGuardEnum::Request) == 1 &&
              int(HeadersGuardEnum::Request_no_cors) == 2 &&
              int(HeadersGuardEnum::Response) == 3 &&
              int(HeadersGuardEnum::Immutable) == 4 &&
              int(HeadersGuardEnum::EndGuard_) == 5,
              "HeadersGuardEnum values are as expected");
static_assert(int(RequestMode::Same_origin) == 0 &&
              int(RequestMode::No_cors) == 1 &&
              int(RequestMode::Cors) == 2 &&
              int(RequestMode::Cors_with_forced_preflight) == 3 &&
              int(RequestMode::EndGuard_) == 4,
              "RequestMode values are as expected");
static_assert(int(RequestCredentials::Omit) == 0 &&
              int(RequestCredentials::Same_origin) == 1 &&
              int(RequestCredentials::Include) == 2 &&
              int(RequestCredentials::EndGuard_) == 3,
              "RequestCredentials values are as expected");
static_assert(int(RequestCache::Default) == 0 &&
              int(RequestCache::No_store) == 1 &&
              int(RequestCache::Reload) == 2 &&
              int(RequestCache::No_cache) == 3 &&
              int(RequestCache::Force_cache) == 4 &&
              int(RequestCache::Only_if_cached) == 5 &&
              int(RequestCache::EndGuard_) == 6,
              "RequestCache values are as expected");
static_assert(int(ResponseType::Basic) == 0 &&
              int(ResponseType::Cors) == 1 &&
              int(ResponseType::Default) == 2 &&
              int(ResponseType::Error) == 3 &&
              int(ResponseType::Opaque) == 4 &&
              int(ResponseType::EndGuard_) == 5,
              "ResponseType values are as expected");





static_assert(DEFAULT_NAMESPACE == 0 &&
              CHROME_ONLY_NAMESPACE == 1 &&
              NUMBER_OF_NAMESPACES == 2,
              "Namespace values are as expected");





static_assert(nsIContentPolicy::TYPE_INVALID == 0 &&
              nsIContentPolicy::TYPE_OTHER == 1 &&
              nsIContentPolicy::TYPE_SCRIPT == 2 &&
              nsIContentPolicy::TYPE_IMAGE == 3 &&
              nsIContentPolicy::TYPE_STYLESHEET == 4 &&
              nsIContentPolicy::TYPE_OBJECT == 5 &&
              nsIContentPolicy::TYPE_DOCUMENT == 6 &&
              nsIContentPolicy::TYPE_SUBDOCUMENT == 7 &&
              nsIContentPolicy::TYPE_REFRESH == 8 &&
              nsIContentPolicy::TYPE_XBL == 9 &&
              nsIContentPolicy::TYPE_PING == 10 &&
              nsIContentPolicy::TYPE_XMLHTTPREQUEST == 11 &&
              nsIContentPolicy::TYPE_DATAREQUEST == 11 &&
              nsIContentPolicy::TYPE_OBJECT_SUBREQUEST == 12 &&
              nsIContentPolicy::TYPE_DTD == 13 &&
              nsIContentPolicy::TYPE_FONT == 14 &&
              nsIContentPolicy::TYPE_MEDIA == 15 &&
              nsIContentPolicy::TYPE_WEBSOCKET == 16 &&
              nsIContentPolicy::TYPE_CSP_REPORT == 17 &&
              nsIContentPolicy::TYPE_XSLT == 18 &&
              nsIContentPolicy::TYPE_BEACON == 19 &&
              nsIContentPolicy::TYPE_FETCH == 20 &&
              nsIContentPolicy::TYPE_IMAGESET == 21 &&
              nsIContentPolicy::TYPE_WEB_MANIFEST == 22 &&
              nsIContentPolicy::TYPE_INTERNAL_SCRIPT == 23 &&
              nsIContentPolicy::TYPE_INTERNAL_WORKER == 24 &&
              nsIContentPolicy::TYPE_INTERNAL_SHARED_WORKER == 25 &&
              nsIContentPolicy::TYPE_INTERNAL_EMBED == 26 &&
              nsIContentPolicy::TYPE_INTERNAL_OBJECT == 27 &&
              nsIContentPolicy::TYPE_INTERNAL_FRAME == 28 &&
              nsIContentPolicy::TYPE_INTERNAL_IFRAME == 29 &&
              nsIContentPolicy::TYPE_INTERNAL_AUDIO == 30 &&
              nsIContentPolicy::TYPE_INTERNAL_VIDEO == 31 &&
              nsIContentPolicy::TYPE_INTERNAL_TRACK == 32,
              "nsContentPolicytType values are as expected");

namespace {

typedef int32_t EntryId;

struct IdCount
{
  IdCount() : mId(-1), mCount(0) { }
  explicit IdCount(int32_t aId) : mId(aId), mCount(1) { }
  int32_t mId;
  int32_t mCount;
};

static nsresult QueryAll(mozIStorageConnection* aConn, CacheId aCacheId,
                         nsTArray<EntryId>& aEntryIdListOut);
static nsresult QueryCache(mozIStorageConnection* aConn, CacheId aCacheId,
                           const CacheRequest& aRequest,
                           const CacheQueryParams& aParams,
                           nsTArray<EntryId>& aEntryIdListOut,
                           uint32_t aMaxResults = UINT32_MAX);
static nsresult MatchByVaryHeader(mozIStorageConnection* aConn,
                                  const CacheRequest& aRequest,
                                  EntryId entryId, bool* aSuccessOut);
static nsresult DeleteEntries(mozIStorageConnection* aConn,
                              const nsTArray<EntryId>& aEntryIdList,
                              nsTArray<nsID>& aDeletedBodyIdListOut,
                              nsTArray<IdCount>& aDeletedSecurityIdListOut,
                              uint32_t aPos=0, int32_t aLen=-1);
static nsresult InsertSecurityInfo(mozIStorageConnection* aConn,
                                   nsICryptoHash* aCrypto,
                                   const nsACString& aData, int32_t *aIdOut);
static nsresult DeleteSecurityInfo(mozIStorageConnection* aConn, int32_t aId,
                                   int32_t aCount);
static nsresult DeleteSecurityInfoList(mozIStorageConnection* aConn,
                                       const nsTArray<IdCount>& aDeletedStorageIdList);
static nsresult InsertEntry(mozIStorageConnection* aConn, CacheId aCacheId,
                            const CacheRequest& aRequest,
                            const nsID* aRequestBodyId,
                            const CacheResponse& aResponse,
                            const nsID* aResponseBodyId);
static nsresult ReadResponse(mozIStorageConnection* aConn, EntryId aEntryId,
                             SavedResponse* aSavedResponseOut);
static nsresult ReadRequest(mozIStorageConnection* aConn, EntryId aEntryId,
                            SavedRequest* aSavedRequestOut);

static void AppendListParamsToQuery(nsACString& aQuery,
                                    const nsTArray<EntryId>& aEntryIdList,
                                    uint32_t aPos, int32_t aLen);
static nsresult BindListParamsToQuery(mozIStorageStatement* aState,
                                      const nsTArray<EntryId>& aEntryIdList,
                                      uint32_t aPos, int32_t aLen);
static nsresult BindId(mozIStorageStatement* aState, const nsACString& aName,
                       const nsID* aId);
static nsresult ExtractId(mozIStorageStatement* aState, uint32_t aPos,
                          nsID* aIdOut);
static nsresult CreateAndBindKeyStatement(mozIStorageConnection* aConn,
                                          const char* aQueryFormat,
                                          const nsAString& aKey,
                                          mozIStorageStatement** aStateOut);
static nsresult HashCString(nsICryptoHash* aCrypto, const nsACString& aIn,
                            nsACString& aOut);
} 

nsresult
CreateSchema(mozIStorageConnection* aConn)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  int32_t schemaVersion;
  nsresult rv = aConn->GetSchemaVersion(&schemaVersion);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (schemaVersion == kLatestSchemaVersion) {
    
    return rv;
  }

  if (!schemaVersion) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE caches ("
        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT "
      ");"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    
    
    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE security_info ("
        "id INTEGER NOT NULL PRIMARY KEY, "
        "hash BLOB NOT NULL, "  
        "data BLOB NOT NULL, "  
        "refcount INTEGER NOT NULL"
      ");"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    
    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE INDEX security_info_hash_index ON security_info (hash);"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE entries ("
        "id INTEGER NOT NULL PRIMARY KEY, "
        "request_method TEXT NOT NULL, "
        "request_url_no_query TEXT NOT NULL, "
        "request_url_no_query_hash BLOB NOT NULL, " 
        "request_url_query TEXT NOT NULL, "
        "request_url_query_hash BLOB NOT NULL, "    
        "request_referrer TEXT NOT NULL, "
        "request_headers_guard INTEGER NOT NULL, "
        "request_mode INTEGER NOT NULL, "
        "request_credentials INTEGER NOT NULL, "
        "request_contentpolicytype INTEGER NOT NULL, "
        "request_cache INTEGER NOT NULL, "
        "request_body_id TEXT NULL, "
        "response_type INTEGER NOT NULL, "
        "response_url TEXT NOT NULL, "
        "response_status INTEGER NOT NULL, "
        "response_status_text TEXT NOT NULL, "
        "response_headers_guard INTEGER NOT NULL, "
        "response_body_id TEXT NULL, "
        "response_security_info_id INTEGER NULL REFERENCES security_info(id), "
        "response_principal_info TEXT NOT NULL, "
        "response_redirected INTEGER NOT NULL, "
        
        
        "response_redirected_url TEXT NOT NULL, "
        "cache_id INTEGER NOT NULL REFERENCES caches(id) ON DELETE CASCADE"
      ");"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    
    
    
    
    
    
    
    
    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE INDEX entries_request_match_index "
                "ON entries (cache_id, request_url_no_query_hash, "
                            "request_url_query_hash);"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE request_headers ("
        "name TEXT NOT NULL, "
        "value TEXT NOT NULL, "
        "entry_id INTEGER NOT NULL REFERENCES entries(id) ON DELETE CASCADE"
      ");"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE response_headers ("
        "name TEXT NOT NULL, "
        "value TEXT NOT NULL, "
        "entry_id INTEGER NOT NULL REFERENCES entries(id) ON DELETE CASCADE"
      ");"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    
    
    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE INDEX response_headers_name_index "
                "ON response_headers (name);"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    
    
    
    rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE TABLE storage ("
        "namespace INTEGER NOT NULL, "
        "key BLOB NULL, "
        "cache_id INTEGER NOT NULL REFERENCES caches(id), "
        "PRIMARY KEY(namespace, key) "
      ");"
    ));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = aConn->SetSchemaVersion(kLatestSchemaVersion);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = aConn->GetSchemaVersion(&schemaVersion);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  if (schemaVersion != kLatestSchemaVersion) {
    return NS_ERROR_FAILURE;
  }

  return rv;
}

nsresult
InitializeConnection(mozIStorageConnection* aConn)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  
  

  nsPrintfCString pragmas(
    
    "PRAGMA page_size = %u; "
    
    "PRAGMA auto_vacuum = INCREMENTAL; "
    "PRAGMA foreign_keys = ON; ",
    kPageSize
  );

  
  
  
  

  nsresult rv = aConn->ExecuteSimpleSQL(pragmas);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  rv = aConn->SetGrowthIncrement(kGrowthSize, EmptyCString());
  if (rv == NS_ERROR_FILE_TOO_BIG) {
    NS_WARNING("Not enough disk space to set sqlite growth increment.");
    rv = NS_OK;
  }
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  
  nsPrintfCString wal(
    
    "PRAGMA wal_autocheckpoint = %u; "
    
    "PRAGMA journal_size_limit = %u; "
    
    "PRAGMA journal_mode = WAL; ",
    kWalAutoCheckpointPages,
    kWalAutoCheckpointSize
  );

  rv = aConn->ExecuteSimpleSQL(wal);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  
  
#ifdef DEBUG
  nsCOMPtr<mozIStorageStatement> state;
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "PRAGMA auto_vacuum;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t mode;
  rv = state->GetInt32(0, &mode);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  if (NS_WARN_IF(mode != 2)) { return NS_ERROR_UNEXPECTED; }
#endif

  return NS_OK;
}

nsresult
CreateCacheId(mozIStorageConnection* aConn, CacheId* aCacheIdOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aCacheIdOut);

  nsresult rv = aConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT INTO caches DEFAULT VALUES;"
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsCOMPtr<mozIStorageStatement> state;
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT last_insert_rowid()"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  if (NS_WARN_IF(!hasMoreData)) { return NS_ERROR_UNEXPECTED; }

  rv = state->GetInt64(0, aCacheIdOut);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

nsresult
DeleteCacheId(mozIStorageConnection* aConn, CacheId aCacheId,
              nsTArray<nsID>& aDeletedBodyIdListOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  
  
  
  nsAutoTArray<EntryId, 256> matches;
  nsresult rv = QueryAll(aConn, aCacheId, matches);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoTArray<IdCount, 16> deletedSecurityIdList;
  rv = DeleteEntries(aConn, matches, aDeletedBodyIdListOut,
                     deletedSecurityIdList);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = DeleteSecurityInfoList(aConn, deletedSecurityIdList);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  nsCOMPtr<mozIStorageStatement> state;
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM caches WHERE id=:id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt64ByName(NS_LITERAL_CSTRING("id"), aCacheId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

nsresult
IsCacheOrphaned(mozIStorageConnection* aConn, CacheId aCacheId,
                bool* aOrphanedOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aOrphanedOut);

  
  *aOrphanedOut = false;

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT COUNT(*) FROM storage WHERE cache_id=:cache_id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt64ByName(NS_LITERAL_CSTRING("cache_id"), aCacheId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  MOZ_ASSERT(hasMoreData);

  int32_t refCount;
  rv = state->GetInt32(0, &refCount);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  *aOrphanedOut = refCount == 0;

  return rv;
}

nsresult
FindOrphanedCacheIds(mozIStorageConnection* aConn,
                     nsTArray<CacheId>& aOrphanedListOut)
{
  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id FROM caches "
    "WHERE id NOT IN (SELECT cache_id from storage);"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    CacheId cacheId = INVALID_CACHE_ID;
    rv = state->GetInt64(0, &cacheId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    aOrphanedListOut.AppendElement(cacheId);
  }

  return rv;
}

nsresult
GetKnownBodyIds(mozIStorageConnection* aConn, nsTArray<nsID>& aBodyIdListOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT request_body_id, response_body_id FROM entries;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    
    for (uint32_t i = 0; i < 2; ++i) {
      bool isNull = false;

      rv = state->GetIsNull(i, &isNull);
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

      if (!isNull) {
        nsID id;
        rv = ExtractId(state, i, &id);
        if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

        aBodyIdListOut.AppendElement(id);
      }
    }
  }

  return rv;
}

nsresult
CacheMatch(mozIStorageConnection* aConn, CacheId aCacheId,
           const CacheRequest& aRequest,
           const CacheQueryParams& aParams,
           bool* aFoundResponseOut,
           SavedResponse* aSavedResponseOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aFoundResponseOut);
  MOZ_ASSERT(aSavedResponseOut);

  *aFoundResponseOut = false;

  nsAutoTArray<EntryId, 1> matches;
  nsresult rv = QueryCache(aConn, aCacheId, aRequest, aParams, matches, 1);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (matches.IsEmpty()) {
    return rv;
  }

  rv = ReadResponse(aConn, matches[0], aSavedResponseOut);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  aSavedResponseOut->mCacheId = aCacheId;
  *aFoundResponseOut = true;

  return rv;
}

nsresult
CacheMatchAll(mozIStorageConnection* aConn, CacheId aCacheId,
              const CacheRequestOrVoid& aRequestOrVoid,
              const CacheQueryParams& aParams,
              nsTArray<SavedResponse>& aSavedResponsesOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  nsresult rv;

  nsAutoTArray<EntryId, 256> matches;
  if (aRequestOrVoid.type() == CacheRequestOrVoid::Tvoid_t) {
    rv = QueryAll(aConn, aCacheId, matches);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  } else {
    rv = QueryCache(aConn, aCacheId, aRequestOrVoid, aParams, matches);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  
  for (uint32_t i = 0; i < matches.Length(); ++i) {
    SavedResponse savedResponse;
    rv = ReadResponse(aConn, matches[i], &savedResponse);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    savedResponse.mCacheId = aCacheId;
    aSavedResponsesOut.AppendElement(savedResponse);
  }

  return rv;
}

nsresult
CachePut(mozIStorageConnection* aConn, CacheId aCacheId,
         const CacheRequest& aRequest,
         const nsID* aRequestBodyId,
         const CacheResponse& aResponse,
         const nsID* aResponseBodyId,
         nsTArray<nsID>& aDeletedBodyIdListOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  CacheQueryParams params(false, false, false, false,
                           NS_LITERAL_STRING(""));
  nsAutoTArray<EntryId, 256> matches;
  nsresult rv = QueryCache(aConn, aCacheId, aRequest, params, matches);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoTArray<IdCount, 16> deletedSecurityIdList;
  rv = DeleteEntries(aConn, matches, aDeletedBodyIdListOut,
                     deletedSecurityIdList);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = InsertEntry(aConn, aCacheId, aRequest, aRequestBodyId, aResponse,
                   aResponseBodyId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  
  rv = DeleteSecurityInfoList(aConn, deletedSecurityIdList);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

nsresult
CacheDelete(mozIStorageConnection* aConn, CacheId aCacheId,
            const CacheRequest& aRequest,
            const CacheQueryParams& aParams,
            nsTArray<nsID>& aDeletedBodyIdListOut, bool* aSuccessOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aSuccessOut);

  *aSuccessOut = false;

  nsAutoTArray<EntryId, 256> matches;
  nsresult rv = QueryCache(aConn, aCacheId, aRequest, aParams, matches);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (matches.IsEmpty()) {
    return rv;
  }

  nsAutoTArray<IdCount, 16> deletedSecurityIdList;
  rv = DeleteEntries(aConn, matches, aDeletedBodyIdListOut,
                     deletedSecurityIdList);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = DeleteSecurityInfoList(aConn, deletedSecurityIdList);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  *aSuccessOut = true;

  return rv;
}

nsresult
CacheKeys(mozIStorageConnection* aConn, CacheId aCacheId,
          const CacheRequestOrVoid& aRequestOrVoid,
          const CacheQueryParams& aParams,
          nsTArray<SavedRequest>& aSavedRequestsOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  nsresult rv;

  nsAutoTArray<EntryId, 256> matches;
  if (aRequestOrVoid.type() == CacheRequestOrVoid::Tvoid_t) {
    rv = QueryAll(aConn, aCacheId, matches);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  } else {
    rv = QueryCache(aConn, aCacheId, aRequestOrVoid, aParams, matches);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  
  for (uint32_t i = 0; i < matches.Length(); ++i) {
    SavedRequest savedRequest;
    rv = ReadRequest(aConn, matches[i], &savedRequest);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    savedRequest.mCacheId = aCacheId;
    aSavedRequestsOut.AppendElement(savedRequest);
  }

  return rv;
}

nsresult
StorageMatch(mozIStorageConnection* aConn,
             Namespace aNamespace,
             const CacheRequest& aRequest,
             const CacheQueryParams& aParams,
             bool* aFoundResponseOut,
             SavedResponse* aSavedResponseOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aFoundResponseOut);
  MOZ_ASSERT(aSavedResponseOut);

  *aFoundResponseOut = false;

  nsresult rv;

  
  
  if (!aParams.cacheName().EqualsLiteral("")) {
    bool foundCache = false;
    
    CacheId cacheId = INVALID_CACHE_ID;
    rv = StorageGetCacheId(aConn, aNamespace, aParams.cacheName(), &foundCache,
                           &cacheId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    if (!foundCache) { return NS_ERROR_DOM_NOT_FOUND_ERR; }

    rv = CacheMatch(aConn, cacheId, aRequest, aParams, aFoundResponseOut,
                    aSavedResponseOut);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return rv;
  }

  

  nsCOMPtr<mozIStorageStatement> state;
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT cache_id FROM storage WHERE namespace=:namespace ORDER BY rowid;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("namespace"), aNamespace);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoTArray<CacheId, 32> cacheIdList;

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    CacheId cacheId = INVALID_CACHE_ID;
    rv = state->GetInt64(0, &cacheId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    cacheIdList.AppendElement(cacheId);
  }

  
  for (uint32_t i = 0; i < cacheIdList.Length(); ++i) {
    rv = CacheMatch(aConn, cacheIdList[i], aRequest, aParams, aFoundResponseOut,
                    aSavedResponseOut);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    if (*aFoundResponseOut) {
      aSavedResponseOut->mCacheId = cacheIdList[i];
      return rv;
    }
  }

  return NS_OK;
}

nsresult
StorageGetCacheId(mozIStorageConnection* aConn, Namespace aNamespace,
                  const nsAString& aKey, bool* aFoundCacheOut,
                  CacheId* aCacheIdOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aFoundCacheOut);
  MOZ_ASSERT(aCacheIdOut);

  *aFoundCacheOut = false;

  
  
  
  const char* query = "SELECT cache_id FROM storage "
                      "WHERE namespace=:namespace AND %s "
                      "ORDER BY rowid;";

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = CreateAndBindKeyStatement(aConn, query, aKey,
                                          getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("namespace"), aNamespace);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (!hasMoreData) {
    return rv;
  }

  rv = state->GetInt64(0, aCacheIdOut);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  *aFoundCacheOut = true;
  return rv;
}

nsresult
StoragePutCache(mozIStorageConnection* aConn, Namespace aNamespace,
                const nsAString& aKey, CacheId aCacheId)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO storage (namespace, key, cache_id) "
                 "VALUES (:namespace, :key, :cache_id);"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("namespace"), aNamespace);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindStringAsBlobByName(NS_LITERAL_CSTRING("key"), aKey);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt64ByName(NS_LITERAL_CSTRING("cache_id"), aCacheId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

nsresult
StorageForgetCache(mozIStorageConnection* aConn, Namespace aNamespace,
                   const nsAString& aKey)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  
  
  
  const char *query = "DELETE FROM storage WHERE namespace=:namespace AND %s;";

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = CreateAndBindKeyStatement(aConn, query, aKey,
                                          getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("namespace"), aNamespace);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

nsresult
StorageGetKeys(mozIStorageConnection* aConn, Namespace aNamespace,
               nsTArray<nsString>& aKeysOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT key FROM storage WHERE namespace=:namespace ORDER BY rowid;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("namespace"), aNamespace);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    nsAutoString key;
    rv = state->GetBlobAsString(0, key);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    aKeysOut.AppendElement(key);
  }

  return rv;
}

namespace {

nsresult
QueryAll(mozIStorageConnection* aConn, CacheId aCacheId,
         nsTArray<EntryId>& aEntryIdListOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id FROM entries WHERE cache_id=:cache_id ORDER BY id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt64ByName(NS_LITERAL_CSTRING("cache_id"), aCacheId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    EntryId entryId = INT32_MAX;
    rv = state->GetInt32(0, &entryId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    aEntryIdListOut.AppendElement(entryId);
  }

  return rv;
}

nsresult
QueryCache(mozIStorageConnection* aConn, CacheId aCacheId,
           const CacheRequest& aRequest,
           const CacheQueryParams& aParams,
           nsTArray<EntryId>& aEntryIdListOut,
           uint32_t aMaxResults)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aMaxResults > 0);

  if (!aParams.ignoreMethod() && !aRequest.method().LowerCaseEqualsLiteral("get")
                              && !aRequest.method().LowerCaseEqualsLiteral("head"))
  {
    return NS_OK;
  }

  nsAutoCString query(
    "SELECT id, COUNT(response_headers.name) AS vary_count "
    "FROM entries "
    "LEFT OUTER JOIN response_headers ON entries.id=response_headers.entry_id "
                                    "AND response_headers.name='vary' "
    "WHERE entries.cache_id=:cache_id "
      "AND entries.request_url_no_query_hash=:url_no_query_hash "
  );

  if (!aParams.ignoreSearch()) {
    query.AppendLiteral("AND entries.request_url_query_hash=:url_query_hash ");
  }

  query.AppendLiteral("AND entries.request_url_no_query=:url_no_query ");

  if (!aParams.ignoreSearch()) {
    query.AppendLiteral("AND entries.request_url_query=:url_query ");
  }

  query.AppendLiteral("GROUP BY entries.id ORDER BY entries.id;");

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(query, getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt64ByName(NS_LITERAL_CSTRING("cache_id"), aCacheId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsCOMPtr<nsICryptoHash> crypto =
    do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoCString urlWithoutQueryHash;
  rv = HashCString(crypto, aRequest.urlWithoutQuery(), urlWithoutQueryHash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringAsBlobByName(NS_LITERAL_CSTRING("url_no_query_hash"),
                                         urlWithoutQueryHash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (!aParams.ignoreSearch()) {
    nsAutoCString urlQueryHash;
    rv = HashCString(crypto, aRequest.urlQuery(), urlQueryHash);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindUTF8StringAsBlobByName(NS_LITERAL_CSTRING("url_query_hash"),
                                           urlQueryHash);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("url_no_query"),
                                   aRequest.urlWithoutQuery());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (!aParams.ignoreSearch()) {
    rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("url_query"),
                                     aRequest.urlQuery());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    
    EntryId entryId = INT32_MAX;
    rv = state->GetInt32(0, &entryId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    int32_t varyCount;
    rv = state->GetInt32(1, &varyCount);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    if (!aParams.ignoreVary() && varyCount > 0) {
      bool matchedByVary = false;
      rv = MatchByVaryHeader(aConn, aRequest, entryId, &matchedByVary);
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
      if (!matchedByVary) {
        continue;
      }
    }

    aEntryIdListOut.AppendElement(entryId);

    if (aEntryIdListOut.Length() == aMaxResults) {
      return NS_OK;
    }
  }

  return rv;
}

nsresult
MatchByVaryHeader(mozIStorageConnection* aConn,
                  const CacheRequest& aRequest,
                  EntryId entryId, bool* aSuccessOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  *aSuccessOut = false;

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT value FROM response_headers "
    "WHERE name='vary' AND entry_id=:entry_id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("entry_id"), entryId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoTArray<nsCString, 8> varyValues;

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    nsAutoCString value;
    rv = state->GetUTF8String(0, value);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    varyValues.AppendElement(value);
  }
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  MOZ_ASSERT(!varyValues.IsEmpty());

  state->Reset();
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT name, value FROM request_headers "
    "WHERE entry_id=:entry_id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("entry_id"), entryId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsRefPtr<InternalHeaders> cachedHeaders =
    new InternalHeaders(HeadersGuardEnum::None);

  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    nsAutoCString name;
    nsAutoCString value;
    rv = state->GetUTF8String(0, name);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    rv = state->GetUTF8String(1, value);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    ErrorResult errorResult;

    cachedHeaders->Append(name, value, errorResult);
    if (errorResult.Failed()) { return errorResult.StealNSResult(); };
  }
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsRefPtr<InternalHeaders> queryHeaders =
    TypeUtils::ToInternalHeaders(aRequest.headers());

  
  bool varyHeadersMatch = true;

  for (uint32_t i = 0; i < varyValues.Length(); ++i) {
    
    nsAutoCString varyValue(varyValues[i]);
    char* rawBuffer = varyValue.BeginWriting();
    char* token = nsCRT::strtok(rawBuffer, NS_HTTP_HEADER_SEPS, &rawBuffer);
    bool bailOut = false;
    for (; token;
         token = nsCRT::strtok(rawBuffer, NS_HTTP_HEADER_SEPS, &rawBuffer)) {
      nsDependentCString header(token);
      MOZ_ASSERT(!header.EqualsLiteral("*"),
                 "We should have already caught this in "
                 "TypeUtils::ToPCacheResponseWithoutBody()");

      ErrorResult errorResult;
      nsAutoCString queryValue;
      queryHeaders->Get(header, queryValue, errorResult);
      if (errorResult.Failed()) {
        errorResult.SuppressException();
        MOZ_ASSERT(queryValue.IsEmpty());
      }

      nsAutoCString cachedValue;
      cachedHeaders->Get(header, cachedValue, errorResult);
      if (errorResult.Failed()) {
        errorResult.SuppressException();
        MOZ_ASSERT(cachedValue.IsEmpty());
      }

      if (queryValue != cachedValue) {
        varyHeadersMatch = false;
        bailOut = true;
        break;
      }
    }

    if (bailOut) {
      break;
    }
  }

  *aSuccessOut = varyHeadersMatch;
  return rv;
}

nsresult
DeleteEntries(mozIStorageConnection* aConn,
              const nsTArray<EntryId>& aEntryIdList,
              nsTArray<nsID>& aDeletedBodyIdListOut,
              nsTArray<IdCount>& aDeletedSecurityIdListOut,
              uint32_t aPos, int32_t aLen)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  if (aEntryIdList.IsEmpty()) {
    return NS_OK;
  }

  MOZ_ASSERT(aPos < aEntryIdList.Length());

  if (aLen < 0) {
    aLen = aEntryIdList.Length() - aPos;
  }

  
  
  if (aLen > kMaxEntriesPerStatement) {
    uint32_t curPos = aPos;
    int32_t remaining = aLen;
    while (remaining > 0) {
      int32_t max = kMaxEntriesPerStatement;
      int32_t curLen = std::min(max, remaining);
      nsresult rv = DeleteEntries(aConn, aEntryIdList, aDeletedBodyIdListOut,
                                  aDeletedSecurityIdListOut, curPos, curLen);
      if (NS_FAILED(rv)) { return rv; }

      curPos += curLen;
      remaining -= curLen;
    }
    return NS_OK;
  }

  nsCOMPtr<mozIStorageStatement> state;
  nsAutoCString query(
    "SELECT request_body_id, response_body_id, response_security_info_id "
    "FROM entries WHERE id IN ("
  );
  AppendListParamsToQuery(query, aEntryIdList, aPos, aLen);
  query.AppendLiteral(")");

  nsresult rv = aConn->CreateStatement(query, getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = BindListParamsToQuery(state, aEntryIdList, aPos, aLen);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    
    for (uint32_t i = 0; i < 2; ++i) {
      bool isNull = false;

      rv = state->GetIsNull(i, &isNull);
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

      if (!isNull) {
        nsID id;
        rv = ExtractId(state, i, &id);
        if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
        aDeletedBodyIdListOut.AppendElement(id);
      }
    }

    
    bool isNull = false;
    rv = state->GetIsNull(2, &isNull);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    if (!isNull) {
      int32_t securityId = -1;
      rv = state->GetInt32(2, &securityId);
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

      
      
      bool found = false;
      for (uint32_t i = 0; i < aDeletedSecurityIdListOut.Length(); ++i) {
        if (aDeletedSecurityIdListOut[i].mId == securityId) {
          found = true;
          aDeletedSecurityIdListOut[i].mCount += 1;
          break;
        }
      }

      
      if (!found) {
        aDeletedSecurityIdListOut.AppendElement(IdCount(securityId));
      }
    }
  }

  

  query = NS_LITERAL_CSTRING(
    "DELETE FROM entries WHERE id IN ("
  );
  AppendListParamsToQuery(query, aEntryIdList, aPos, aLen);
  query.AppendLiteral(")");

  rv = aConn->CreateStatement(query, getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = BindListParamsToQuery(state, aEntryIdList, aPos, aLen);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

nsresult
InsertSecurityInfo(mozIStorageConnection* aConn, nsICryptoHash* aCrypto,
                   const nsACString& aData, int32_t *aIdOut)
{
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aCrypto);
  MOZ_ASSERT(aIdOut);
  MOZ_ASSERT(!aData.IsEmpty());

  
  
  
  
  nsAutoCString hash;
  nsresult rv = HashCString(aCrypto, aData, hash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  
  
  
  nsCOMPtr<mozIStorageStatement> state;
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    
    
    "SELECT id, refcount FROM security_info WHERE hash=:hash AND data=:data;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringAsBlobByName(NS_LITERAL_CSTRING("hash"), hash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringAsBlobByName(NS_LITERAL_CSTRING("data"), aData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  if (hasMoreData) {
    
    rv = state->GetInt32(0, aIdOut);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    int32_t refcount = -1;
    rv = state->GetInt32(1, &refcount);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    
    refcount += 1;

    rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE security_info SET refcount=:refcount WHERE id=:id;"
    ), getter_AddRefs(state));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindInt32ByName(NS_LITERAL_CSTRING("refcount"), refcount);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindInt32ByName(NS_LITERAL_CSTRING("id"), *aIdOut);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return NS_OK;
  }

  
  
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO security_info (hash, data, refcount) VALUES (:hash, :data, 1);"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringAsBlobByName(NS_LITERAL_CSTRING("hash"), hash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringAsBlobByName(NS_LITERAL_CSTRING("data"), aData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT last_insert_rowid()"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->GetInt32(0, aIdOut);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return NS_OK;
}

nsresult
DeleteSecurityInfo(mozIStorageConnection* aConn, int32_t aId, int32_t aCount)
{
  
  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT refcount FROM security_info WHERE id=:id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("id"), aId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t refcount = -1;
  rv = state->GetInt32(0, &refcount);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  MOZ_ASSERT(refcount >= aCount);

  
  int32_t newCount = refcount - aCount;

  
  
  if (newCount == 0) {
    rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM security_info WHERE id=:id;"
    ), getter_AddRefs(state));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindInt32ByName(NS_LITERAL_CSTRING("id"), aId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return NS_OK;
  }

  
  
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE security_info SET refcount=:refcount WHERE id=:id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("refcount"), newCount);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("id"), aId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return NS_OK;
}

nsresult
DeleteSecurityInfoList(mozIStorageConnection* aConn,
                        const nsTArray<IdCount>& aDeletedStorageIdList)
{
  for (uint32_t i = 0; i < aDeletedStorageIdList.Length(); ++i) {
    nsresult rv = DeleteSecurityInfo(aConn, aDeletedStorageIdList[i].mId,
                                     aDeletedStorageIdList[i].mCount);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  return NS_OK;
}

nsresult
InsertEntry(mozIStorageConnection* aConn, CacheId aCacheId,
            const CacheRequest& aRequest,
            const nsID* aRequestBodyId,
            const CacheResponse& aResponse,
            const nsID* aResponseBodyId)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);

  nsresult rv = NS_OK;

  nsCOMPtr<nsICryptoHash> crypto =
    do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t securityId = -1;
  if (!aResponse.channelInfo().securityInfo().IsEmpty()) {
    rv = InsertSecurityInfo(aConn, crypto,
                            aResponse.channelInfo().securityInfo(),
                            &securityId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  nsCOMPtr<mozIStorageStatement> state;
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO entries ("
      "request_method, "
      "request_url_no_query, "
      "request_url_no_query_hash, "
      "request_url_query, "
      "request_url_query_hash, "
      "request_referrer, "
      "request_headers_guard, "
      "request_mode, "
      "request_credentials, "
      "request_contentpolicytype, "
      "request_cache, "
      "request_body_id, "
      "response_type, "
      "response_url, "
      "response_status, "
      "response_status_text, "
      "response_headers_guard, "
      "response_body_id, "
      "response_security_info_id, "
      "response_principal_info, "
      "response_redirected, "
      "response_redirected_url, "
      "cache_id "
    ") VALUES ("
      ":request_method, "
      ":request_url_no_query, "
      ":request_url_no_query_hash, "
      ":request_url_query, "
      ":request_url_query_hash, "
      ":request_referrer, "
      ":request_headers_guard, "
      ":request_mode, "
      ":request_credentials, "
      ":request_contentpolicytype, "
      ":request_cache, "
      ":request_body_id, "
      ":response_type, "
      ":response_url, "
      ":response_status, "
      ":response_status_text, "
      ":response_headers_guard, "
      ":response_body_id, "
      ":response_security_info_id, "
      ":response_principal_info, "
      ":response_redirected, "
      ":response_redirected_url, "
      ":cache_id "
    ");"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("request_method"),
                                   aRequest.method());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("request_url_no_query"),
                                   aRequest.urlWithoutQuery());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoCString urlWithoutQueryHash;
  rv = HashCString(crypto, aRequest.urlWithoutQuery(), urlWithoutQueryHash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringAsBlobByName(
    NS_LITERAL_CSTRING("request_url_no_query_hash"), urlWithoutQueryHash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("request_url_query"),
                                   aRequest.urlQuery());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoCString urlQueryHash;
  rv = HashCString(crypto, aRequest.urlQuery(), urlQueryHash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringAsBlobByName(
    NS_LITERAL_CSTRING("request_url_query_hash"), urlQueryHash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindStringByName(NS_LITERAL_CSTRING("request_referrer"),
                               aRequest.referrer());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("request_headers_guard"),
    static_cast<int32_t>(aRequest.headersGuard()));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("request_mode"),
                              static_cast<int32_t>(aRequest.mode()));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("request_credentials"),
    static_cast<int32_t>(aRequest.credentials()));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("request_contentpolicytype"),
    static_cast<int32_t>(aRequest.contentPolicyType()));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("request_cache"),
    static_cast<int32_t>(aRequest.requestCache()));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = BindId(state, NS_LITERAL_CSTRING("request_body_id"), aRequestBodyId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("response_type"),
                              static_cast<int32_t>(aResponse.type()));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("response_url"),
                                   aResponse.url());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("response_status"),
                              aResponse.status());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("response_status_text"),
                                   aResponse.statusText());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("response_headers_guard"),
    static_cast<int32_t>(aResponse.headersGuard()));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = BindId(state, NS_LITERAL_CSTRING("response_body_id"), aResponseBodyId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (aResponse.channelInfo().securityInfo().IsEmpty()) {
    rv = state->BindNullByName(NS_LITERAL_CSTRING("response_security_info_id"));
  } else {
    rv = state->BindInt32ByName(NS_LITERAL_CSTRING("response_security_info_id"),
                                securityId);
  }
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoCString serializedInfo;
  
  if (aResponse.principalInfo().type() == mozilla::ipc::OptionalPrincipalInfo::TPrincipalInfo) {
    const mozilla::ipc::PrincipalInfo& principalInfo =
      aResponse.principalInfo().get_PrincipalInfo();
    MOZ_ASSERT(principalInfo.type() == mozilla::ipc::PrincipalInfo::TContentPrincipalInfo);
    const mozilla::ipc::ContentPrincipalInfo& cInfo =
      principalInfo.get_ContentPrincipalInfo();

    serializedInfo.Append(cInfo.spec());

    MOZ_ASSERT(cInfo.appId() != nsIScriptSecurityManager::UNKNOWN_APP_ID);
    OriginAttributes attrs(cInfo.appId(), cInfo.isInBrowserElement());
    nsAutoCString suffix;
    attrs.CreateSuffix(suffix);
    serializedInfo.Append(suffix);
  }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("response_principal_info"),
                                   serializedInfo);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("response_redirected"),
                              aResponse.channelInfo().redirected() ? 1 : 0);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("response_redirected_url"),
                                   aResponse.channelInfo().redirectedURI());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt64ByName(NS_LITERAL_CSTRING("cache_id"), aCacheId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->Execute();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT last_insert_rowid()"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t entryId;
  rv = state->GetInt32(0, &entryId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO request_headers ("
      "name, "
      "value, "
      "entry_id "
    ") VALUES (:name, :value, :entry_id)"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  const nsTArray<HeadersEntry>& requestHeaders = aRequest.headers();
  for (uint32_t i = 0; i < requestHeaders.Length(); ++i) {
    rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("name"),
                                     requestHeaders[i].name());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("value"),
                                     requestHeaders[i].value());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindInt32ByName(NS_LITERAL_CSTRING("entry_id"), entryId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO response_headers ("
      "name, "
      "value, "
      "entry_id "
    ") VALUES (:name, :value, :entry_id)"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  const nsTArray<HeadersEntry>& responseHeaders = aResponse.headers();
  for (uint32_t i = 0; i < responseHeaders.Length(); ++i) {
    rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("name"),
                                     responseHeaders[i].name());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindUTF8StringByName(NS_LITERAL_CSTRING("value"),
                                     responseHeaders[i].value());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->BindInt32ByName(NS_LITERAL_CSTRING("entry_id"), entryId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->Execute();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  return rv;
}

nsresult
ReadResponse(mozIStorageConnection* aConn, EntryId aEntryId,
             SavedResponse* aSavedResponseOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aSavedResponseOut);

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "entries.response_type, "
      "entries.response_url, "
      "entries.response_status, "
      "entries.response_status_text, "
      "entries.response_headers_guard, "
      "entries.response_body_id, "
      "entries.response_principal_info, "
      "entries.response_redirected, "
      "entries.response_redirected_url, "
      "security_info.data "
    "FROM entries "
    "LEFT OUTER JOIN security_info "
    "ON entries.response_security_info_id=security_info.id "
    "WHERE entries.id=:id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("id"), aEntryId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t type;
  rv = state->GetInt32(0, &type);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedResponseOut->mValue.type() = static_cast<ResponseType>(type);

  rv = state->GetUTF8String(1, aSavedResponseOut->mValue.url());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t status;
  rv = state->GetInt32(2, &status);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedResponseOut->mValue.status() = status;

  rv = state->GetUTF8String(3, aSavedResponseOut->mValue.statusText());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t guard;
  rv = state->GetInt32(4, &guard);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedResponseOut->mValue.headersGuard() =
    static_cast<HeadersGuardEnum>(guard);

  bool nullBody = false;
  rv = state->GetIsNull(5, &nullBody);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedResponseOut->mHasBodyId = !nullBody;

  if (aSavedResponseOut->mHasBodyId) {
    rv = ExtractId(state, 5, &aSavedResponseOut->mBodyId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  nsAutoCString serializedInfo;
  rv = state->GetUTF8String(6, serializedInfo);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  aSavedResponseOut->mValue.principalInfo() = void_t();
  if (!serializedInfo.IsEmpty()) {
    nsAutoCString originNoSuffix;
    OriginAttributes attrs;
    fprintf(stderr, "\n%s\n", serializedInfo.get());
    if (!attrs.PopulateFromOrigin(serializedInfo, originNoSuffix)) {
      NS_WARNING("Something went wrong parsing a serialized principal!");
      return NS_ERROR_FAILURE;
    }

    aSavedResponseOut->mValue.principalInfo() =
      mozilla::ipc::ContentPrincipalInfo(attrs.mAppId, attrs.mInBrowser, originNoSuffix);
  }

  int32_t redirected;
  rv = state->GetInt32(7, &redirected);
  aSavedResponseOut->mValue.channelInfo().redirected() = !!redirected;

  rv = state->GetUTF8String(8, aSavedResponseOut->mValue.channelInfo().redirectedURI());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->GetBlobAsUTF8String(9, aSavedResponseOut->mValue.channelInfo().securityInfo());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "name, "
      "value "
    "FROM response_headers "
    "WHERE entry_id=:entry_id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("entry_id"), aEntryId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    HeadersEntry header;

    rv = state->GetUTF8String(0, header.name());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->GetUTF8String(1, header.value());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    aSavedResponseOut->mValue.headers().AppendElement(header);
  }

  return rv;
}

nsresult
ReadRequest(mozIStorageConnection* aConn, EntryId aEntryId,
            SavedRequest* aSavedRequestOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aSavedRequestOut);

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "request_method, "
      "request_url_no_query, "
      "request_url_query, "
      "request_referrer, "
      "request_headers_guard, "
      "request_mode, "
      "request_credentials, "
      "request_contentpolicytype, "
      "request_cache, "
      "request_body_id "
    "FROM entries "
    "WHERE id=:id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("id"), aEntryId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->GetUTF8String(0, aSavedRequestOut->mValue.method());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->GetUTF8String(1, aSavedRequestOut->mValue.urlWithoutQuery());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->GetUTF8String(2, aSavedRequestOut->mValue.urlQuery());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->GetString(3, aSavedRequestOut->mValue.referrer());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t guard;
  rv = state->GetInt32(4, &guard);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedRequestOut->mValue.headersGuard() =
    static_cast<HeadersGuardEnum>(guard);

  int32_t mode;
  rv = state->GetInt32(5, &mode);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedRequestOut->mValue.mode() = static_cast<RequestMode>(mode);

  int32_t credentials;
  rv = state->GetInt32(6, &credentials);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedRequestOut->mValue.credentials() =
    static_cast<RequestCredentials>(credentials);

  int32_t requestContentPolicyType;
  rv = state->GetInt32(7, &requestContentPolicyType);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedRequestOut->mValue.contentPolicyType() =
    static_cast<nsContentPolicyType>(requestContentPolicyType);

  int32_t requestCache;
  rv = state->GetInt32(8, &requestCache);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedRequestOut->mValue.requestCache() =
    static_cast<RequestCache>(requestCache);

  bool nullBody = false;
  rv = state->GetIsNull(9, &nullBody);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  aSavedRequestOut->mHasBodyId = !nullBody;

  if (aSavedRequestOut->mHasBodyId) {
    rv = ExtractId(state, 9, &aSavedRequestOut->mBodyId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "name, "
      "value "
    "FROM request_headers "
    "WHERE entry_id=:entry_id;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = state->BindInt32ByName(NS_LITERAL_CSTRING("entry_id"), aEntryId);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  while (NS_SUCCEEDED(state->ExecuteStep(&hasMoreData)) && hasMoreData) {
    HeadersEntry header;

    rv = state->GetUTF8String(0, header.name());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = state->GetUTF8String(1, header.value());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    aSavedRequestOut->mValue.headers().AppendElement(header);
  }

  return rv;
}

void
AppendListParamsToQuery(nsACString& aQuery,
                        const nsTArray<EntryId>& aEntryIdList,
                        uint32_t aPos, int32_t aLen)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT((aPos + aLen) <= aEntryIdList.Length());
  for (int32_t i = aPos; i < aLen; ++i) {
    if (i == 0) {
      aQuery.AppendLiteral("?");
    } else {
      aQuery.AppendLiteral(",?");
    }
  }
}

nsresult
BindListParamsToQuery(mozIStorageStatement* aState,
                      const nsTArray<EntryId>& aEntryIdList,
                      uint32_t aPos, int32_t aLen)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT((aPos + aLen) <= aEntryIdList.Length());
  for (int32_t i = aPos; i < aLen; ++i) {
    nsresult rv = aState->BindInt32ByIndex(i, aEntryIdList[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
BindId(mozIStorageStatement* aState, const nsACString& aName, const nsID* aId)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aState);
  nsresult rv;

  if (!aId) {
    rv = aState->BindNullByName(aName);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    return rv;
  }

  char idBuf[NSID_LENGTH];
  aId->ToProvidedString(idBuf);
  rv = aState->BindUTF8StringByName(aName, nsAutoCString(idBuf));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return rv;
}

nsresult
ExtractId(mozIStorageStatement* aState, uint32_t aPos, nsID* aIdOut)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aState);
  MOZ_ASSERT(aIdOut);

  nsAutoCString idString;
  nsresult rv = aState->GetUTF8String(aPos, idString);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool success = aIdOut->Parse(idString.get());
  if (NS_WARN_IF(!success)) { return NS_ERROR_UNEXPECTED; }

  return rv;
}

nsresult
CreateAndBindKeyStatement(mozIStorageConnection* aConn,
                          const char* aQueryFormat,
                          const nsAString& aKey,
                          mozIStorageStatement** aStateOut)
{
  MOZ_ASSERT(aConn);
  MOZ_ASSERT(aQueryFormat);
  MOZ_ASSERT(aStateOut);

  
  
  
  
  
  
  const char* constraint = nullptr;
  if (aKey.IsEmpty()) {
    constraint = "key IS NULL";
  } else {
    constraint = "key=:key";
  }

  nsPrintfCString query(aQueryFormat, constraint);

  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(query, getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  if (!aKey.IsEmpty()) {
    rv = state->BindStringAsBlobByName(NS_LITERAL_CSTRING("key"), aKey);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  }

  state.forget(aStateOut);

  return rv;
}

nsresult
HashCString(nsICryptoHash* aCrypto, const nsACString& aIn, nsACString& aOut)
{
  MOZ_ASSERT(aCrypto);

  nsresult rv = aCrypto->Init(nsICryptoHash::SHA1);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  rv = aCrypto->Update(reinterpret_cast<const uint8_t*>(aIn.BeginReading()),
                       aIn.Length());
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  nsAutoCString fullHash;
  rv = aCrypto->Finish(false , fullHash);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  aOut = Substring(fullHash, 0, 8);
  return rv;
}

} 

nsresult
IncrementalVacuum(mozIStorageConnection* aConn)
{
  
  nsCOMPtr<mozIStorageStatement> state;
  nsresult rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "PRAGMA freelist_count;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  bool hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  int32_t freePages = 0;
  rv = state->GetInt32(0, &freePages);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  
  
  
  
  
  
  
  
  
  
  
  if (freePages <= kMaxFreePages) {
    return NS_OK;
  }

  
  
  int32_t pagesToRelease = freePages - kMaxFreePages;

  rv = aConn->ExecuteSimpleSQL(nsPrintfCString(
    "PRAGMA incremental_vacuum(%d);", pagesToRelease
  ));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
#ifdef DEBUG
  rv = aConn->CreateStatement(NS_LITERAL_CSTRING(
    "PRAGMA freelist_count;"
  ), getter_AddRefs(state));
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  hasMoreData = false;
  rv = state->ExecuteStep(&hasMoreData);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  freePages = 0;
  rv = state->GetInt32(0, &freePages);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  MOZ_ASSERT(freePages <= kMaxFreePages);
#endif

  return NS_OK;
}

} 
} 
} 
} 
