





#ifndef mozilla_dom_cache_DBSchema_h
#define mozilla_dom_cache_DBSchema_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/cache/Types.h"
#include "nsError.h"
#include "nsString.h"
#include "nsTArrayForwardDeclare.h"

class mozIStorageConnection;
class mozIStorageStatement;
struct nsID;

namespace mozilla {
namespace dom {
namespace cache {

class CacheQueryParams;
class CacheRequest;
class CacheRequestOrVoid;
class CacheResponse;
struct SavedRequest;
struct SavedResponse;


class DBSchema final
{
public:
  static nsresult CreateSchema(mozIStorageConnection* aConn);
  static nsresult InitializeConnection(mozIStorageConnection* aConn);

  static nsresult CreateCache(mozIStorageConnection* aConn,
                              CacheId* aCacheIdOut);
  
  static nsresult DeleteCache(mozIStorageConnection* aConn, CacheId aCacheId,
                              nsTArray<nsID>& aDeletedBodyIdListOut);

  
  static nsresult IsCacheOrphaned(mozIStorageConnection* aConn,
                                  CacheId aCacheId, bool* aOrphanedOut);

  static nsresult CacheMatch(mozIStorageConnection* aConn, CacheId aCacheId,
                             const CacheRequest& aRequest,
                             const CacheQueryParams& aParams,
                             bool* aFoundResponseOut,
                             SavedResponse* aSavedResponseOut);
  static nsresult CacheMatchAll(mozIStorageConnection* aConn, CacheId aCacheId,
                                const CacheRequestOrVoid& aRequestOrVoid,
                                const CacheQueryParams& aParams,
                                nsTArray<SavedResponse>& aSavedResponsesOut);
  static nsresult CachePut(mozIStorageConnection* aConn, CacheId aCacheId,
                           const CacheRequest& aRequest,
                           const nsID* aRequestBodyId,
                           const CacheResponse& aResponse,
                           const nsID* aResponseBodyId,
                           nsTArray<nsID>& aDeletedBodyIdListOut);
  static nsresult CacheDelete(mozIStorageConnection* aConn, CacheId aCacheId,
                              const CacheRequest& aRequest,
                              const CacheQueryParams& aParams,
                              nsTArray<nsID>& aDeletedBodyIdListOut,
                              bool* aSuccessOut);
  static nsresult CacheKeys(mozIStorageConnection* aConn, CacheId aCacheId,
                            const CacheRequestOrVoid& aRequestOrVoid,
                            const CacheQueryParams& aParams,
                            nsTArray<SavedRequest>& aSavedRequestsOut);

  static nsresult StorageMatch(mozIStorageConnection* aConn,
                               Namespace aNamespace,
                               const CacheRequest& aRequest,
                               const CacheQueryParams& aParams,
                               bool* aFoundResponseOut,
                               SavedResponse* aSavedResponseOut);
  static nsresult StorageGetCacheId(mozIStorageConnection* aConn,
                                    Namespace aNamespace, const nsAString& aKey,
                                    bool* aFoundCacheOut, CacheId* aCacheIdOut);
  static nsresult StoragePutCache(mozIStorageConnection* aConn,
                                  Namespace aNamespace, const nsAString& aKey,
                                  CacheId aCacheId);
  static nsresult StorageForgetCache(mozIStorageConnection* aConn,
                                     Namespace aNamespace,
                                     const nsAString& aKey);
  static nsresult StorageGetKeys(mozIStorageConnection* aConn,
                                 Namespace aNamespace,
                                 nsTArray<nsString>& aKeysOut);

  
  static const int32_t kMaxWipeSchemaVersion;

private:
  typedef int32_t EntryId;

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
                                uint32_t aPos=0, int32_t aLen=-1);
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
  static nsresult BindId(mozIStorageStatement* aState, uint32_t aPos,
                         const nsID* aId);
  static nsresult ExtractId(mozIStorageStatement* aState, uint32_t aPos,
                            nsID* aIdOut);

  DBSchema() = delete;
  ~DBSchema() = delete;

  static const int32_t kLatestSchemaVersion;
  static const int32_t kMaxEntriesPerStatement;
};

} 
} 
} 

#endif 
