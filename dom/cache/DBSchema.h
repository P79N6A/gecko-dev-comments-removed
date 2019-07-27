





#ifndef mozilla_dom_cache_DBSchema_h
#define mozilla_dom_cache_DBSchema_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/cache/Types.h"
#include "nsError.h"
#include "nsString.h"
#include "nsTArrayForwardDeclare.h"

class mozIStorageConnection;
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

namespace db {

nsresult
CreateSchema(mozIStorageConnection* aConn);

nsresult
InitializeConnection(mozIStorageConnection* aConn);

nsresult
CreateCache(mozIStorageConnection* aConn, CacheId* aCacheIdOut);


nsresult
DeleteCache(mozIStorageConnection* aConn, CacheId aCacheId,
            nsTArray<nsID>& aDeletedBodyIdListOut);


nsresult
IsCacheOrphaned(mozIStorageConnection* aConn, CacheId aCacheId,
                bool* aOrphanedOut);

nsresult
CacheMatch(mozIStorageConnection* aConn, CacheId aCacheId,
           const CacheRequest& aRequest, const CacheQueryParams& aParams,
           bool* aFoundResponseOut, SavedResponse* aSavedResponseOut);

nsresult
CacheMatchAll(mozIStorageConnection* aConn, CacheId aCacheId,
              const CacheRequestOrVoid& aRequestOrVoid,
              const CacheQueryParams& aParams,
              nsTArray<SavedResponse>& aSavedResponsesOut);

nsresult
CachePut(mozIStorageConnection* aConn, CacheId aCacheId,
         const CacheRequest& aRequest,
         const nsID* aRequestBodyId,
         const CacheResponse& aResponse,
         const nsID* aResponseBodyId,
         nsTArray<nsID>& aDeletedBodyIdListOut);

nsresult
CacheDelete(mozIStorageConnection* aConn, CacheId aCacheId,
            const CacheRequest& aRequest,
            const CacheQueryParams& aParams,
            nsTArray<nsID>& aDeletedBodyIdListOut,
            bool* aSuccessOut);

nsresult
CacheKeys(mozIStorageConnection* aConn, CacheId aCacheId,
          const CacheRequestOrVoid& aRequestOrVoid,
          const CacheQueryParams& aParams,
          nsTArray<SavedRequest>& aSavedRequestsOut);

nsresult
StorageMatch(mozIStorageConnection* aConn,
             Namespace aNamespace,
             const CacheRequest& aRequest,
             const CacheQueryParams& aParams,
             bool* aFoundResponseOut,
             SavedResponse* aSavedResponseOut);

nsresult
StorageGetCacheId(mozIStorageConnection* aConn, Namespace aNamespace,
                  const nsAString& aKey, bool* aFoundCacheOut,
                  CacheId* aCacheIdOut);

nsresult
StoragePutCache(mozIStorageConnection* aConn, Namespace aNamespace,
                const nsAString& aKey, CacheId aCacheId);

nsresult
StorageForgetCache(mozIStorageConnection* aConn, Namespace aNamespace,
                   const nsAString& aKey);

nsresult
StorageGetKeys(mozIStorageConnection* aConn, Namespace aNamespace,
               nsTArray<nsString>& aKeysOut);


extern const int32_t kMaxWipeSchemaVersion;

} 
} 
} 
} 

#endif 
