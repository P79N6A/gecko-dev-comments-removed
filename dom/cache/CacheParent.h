





#ifndef mozilla_dom_cache_CacheParent_h
#define mozilla_dom_cache_CacheParent_h

#include "mozilla/dom/cache/FetchPut.h"
#include "mozilla/dom/cache/Manager.h"
#include "mozilla/dom/cache/PCacheParent.h"
#include "mozilla/dom/cache/Types.h"

struct nsID;
template <class T> class nsRefPtr;

namespace mozilla {
namespace dom {
namespace cache {

class CacheDBConnection;
class CacheStreamControlParent;
struct SavedResponse;
struct StreamHolder;

class CacheParent MOZ_FINAL : public PCacheParent
                            , public Manager::Listener
                            , public FetchPut::Listener
{
public:
  CacheParent(cache::Manager* aManager, CacheId aCacheId);
  virtual ~CacheParent();

private:
  
  virtual void ActorDestroy(ActorDestroyReason aReason) MOZ_OVERRIDE;
  virtual bool RecvTeardown() MOZ_OVERRIDE;
  virtual bool
  RecvMatch(const RequestId& aRequestId, const PCacheRequest& aRequest,
            const PCacheQueryParams& aParams) MOZ_OVERRIDE;
  virtual bool
  RecvMatchAll(const RequestId& aRequestId, const PCacheRequestOrVoid& aRequest,
               const PCacheQueryParams& aParams) MOZ_OVERRIDE;
  virtual bool
  RecvAddAll(const RequestId& aRequestId,
             nsTArray<PCacheRequest>&& aRequests) MOZ_OVERRIDE;
  virtual bool
  RecvPut(const RequestId& aRequestId,
          const CacheRequestResponse& aPut) MOZ_OVERRIDE;
  virtual bool
  RecvDelete(const RequestId& aRequestId, const PCacheRequest& aRequest,
             const PCacheQueryParams& aParams) MOZ_OVERRIDE;
  virtual bool
  RecvKeys(const RequestId& aRequestId, const PCacheRequestOrVoid& aRequest,
           const PCacheQueryParams& aParams) MOZ_OVERRIDE;

  
  virtual void OnCacheMatch(RequestId aRequestId, nsresult aRv,
                            const SavedResponse* aSavedResponse,
                            StreamList* aStreamList) MOZ_OVERRIDE;
  virtual void OnCacheMatchAll(RequestId aRequestId, nsresult aRv,
                               const nsTArray<SavedResponse>& aSavedResponses,
                               StreamList* aStreamList) MOZ_OVERRIDE;
  virtual void OnCachePutAll(RequestId aRequestId, nsresult aRv) MOZ_OVERRIDE;
  virtual void OnCacheDelete(RequestId aRequestId, nsresult aRv,
                             bool aSuccess) MOZ_OVERRIDE;
  virtual void OnCacheKeys(RequestId aRequestId, nsresult aRv,
                           const nsTArray<SavedRequest>& aSavedRequests,
                           StreamList* aStreamList) MOZ_OVERRIDE;

  
  virtual void OnFetchPut(FetchPut* aFetchPut, RequestId aRequestId,
                          nsresult aRv) MOZ_OVERRIDE;

  already_AddRefed<nsIInputStream>
  DeserializeCacheStream(const PCacheReadStreamOrVoid& aStreamOrVoid);

  nsRefPtr<cache::Manager> mManager;
  const CacheId mCacheId;
  nsTArray<nsRefPtr<FetchPut>> mFetchPutList;
};

} 
} 
} 

#endif 
