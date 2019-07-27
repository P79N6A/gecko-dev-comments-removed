





#ifndef mozilla_dom_cache_Manager_h
#define mozilla_dom_cache_Manager_h

#include "mozilla/dom/cache/CacheInitData.h"
#include "mozilla/dom/cache/PCacheStreamControlParent.h"
#include "mozilla/dom/cache/Types.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIInputStream;
class nsIThread;

namespace mozilla {
namespace dom {
namespace cache {

class CacheRequestResponse;
class Context;
class ManagerId;
class PCacheQueryParams;
class PCacheRequest;
class PCacheRequestOrVoid;
struct SavedRequest;
struct SavedResponse;
class StreamList;






























class Manager final
{
public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  class Listener
  {
  public:
    virtual void OnCacheMatch(RequestId aRequestId, nsresult aRv,
                              const SavedResponse* aResponse,
                              StreamList* aStreamList) { }
    virtual void OnCacheMatchAll(RequestId aRequestId, nsresult aRv,
                                 const nsTArray<SavedResponse>& aSavedResponses,
                                 StreamList* aStreamList) { }
    virtual void OnCachePutAll(RequestId aRequestId, nsresult aRv) { }
    virtual void OnCacheDelete(RequestId aRequestId, nsresult aRv,
                               bool aSuccess) { }
    virtual void OnCacheKeys(RequestId aRequestId, nsresult aRv,
                             const nsTArray<SavedRequest>& aSavedRequests,
                             StreamList* aStreamList) { }

    virtual void OnStorageMatch(RequestId aRequestId, nsresult aRv,
                                const SavedResponse* aResponse,
                                StreamList* aStreamList) { }
    virtual void OnStorageHas(RequestId aRequestId, nsresult aRv,
                              bool aCacheFound) { }
    virtual void OnStorageOpen(RequestId aRequestId, nsresult aRv,
                               CacheId aCacheId) { }
    virtual void OnStorageDelete(RequestId aRequestId, nsresult aRv,
                                 bool aCacheDeleted) { }
    virtual void OnStorageKeys(RequestId aRequestId, nsresult aRv,
                               const nsTArray<nsString>& aKeys) { }

  protected:
    ~Listener() { }
  };

  static nsresult GetOrCreate(ManagerId* aManagerId, Manager** aManagerOut);
  static already_AddRefed<Manager> Get(ManagerId* aManagerId);

  
  static void ShutdownAllOnMainThread();

  
  void RemoveListener(Listener* aListener);

  
  void RemoveContext(Context* aContext);

  
  
  void Invalidate();
  bool IsValid() const;

  
  
  
  
  
  
  void AddRefCacheId(CacheId aCacheId);
  void ReleaseCacheId(CacheId aCacheId);
  void AddRefBodyId(const nsID& aBodyId);
  void ReleaseBodyId(const nsID& aBodyId);

  already_AddRefed<ManagerId> GetManagerId() const;

  
  
  void AddStreamList(StreamList* aStreamList);
  void RemoveStreamList(StreamList* aStreamList);

  
  void CacheMatch(Listener* aListener, RequestId aRequestId, CacheId aCacheId,
                  const PCacheRequest& aRequest,
                  const PCacheQueryParams& aParams);
  void CacheMatchAll(Listener* aListener, RequestId aRequestId,
                     CacheId aCacheId, const PCacheRequestOrVoid& aRequestOrVoid,
                     const PCacheQueryParams& aParams);
  void CachePutAll(Listener* aListener, RequestId aRequestId, CacheId aCacheId,
                   const nsTArray<CacheRequestResponse>& aPutList,
                   const nsTArray<nsCOMPtr<nsIInputStream>>& aRequestStreamList,
                   const nsTArray<nsCOMPtr<nsIInputStream>>& aResponseStreamList);
  void CacheDelete(Listener* aListener, RequestId aRequestId,
                   CacheId aCacheId, const PCacheRequest& aRequest,
                   const PCacheQueryParams& aParams);
  void CacheKeys(Listener* aListener, RequestId aRequestId,
                 CacheId aCacheId, const PCacheRequestOrVoid& aRequestOrVoid,
                 const PCacheQueryParams& aParams);

  void StorageMatch(Listener* aListener, RequestId aRequestId,
                    Namespace aNamespace, const PCacheRequest& aRequest,
                    const PCacheQueryParams& aParams);
  void StorageHas(Listener* aListener, RequestId aRequestId,
                  Namespace aNamespace, const nsAString& aKey);
  void StorageOpen(Listener* aListener, RequestId aRequestId,
                   Namespace aNamespace, const nsAString& aKey);
  void StorageDelete(Listener* aListener, RequestId aRequestId,
                     Namespace aNamespace, const nsAString& aKey);
  void StorageKeys(Listener* aListener, RequestId aRequestId,
                   Namespace aNamespace);

private:
  class Factory;
  class BaseAction;
  class DeleteOrphanedCacheAction;

  class CacheMatchAction;
  class CacheMatchAllAction;
  class CachePutAllAction;
  class CacheDeleteAction;
  class CacheKeysAction;

  class StorageMatchAction;
  class StorageHasAction;
  class StorageOpenAction;
  class StorageDeleteAction;
  class StorageKeysAction;

  typedef uint64_t ListenerId;

  Manager(ManagerId* aManagerId, nsIThread* aIOThread);
  ~Manager();
  void Shutdown();
  already_AddRefed<Context> CurrentContext();

  ListenerId SaveListener(Listener* aListener);
  Listener* GetListener(ListenerId aListenerId) const;

  bool SetCacheIdOrphanedIfRefed(CacheId aCacheId);
  bool SetBodyIdOrphanedIfRefed(const nsID& aBodyId);
  void NoteOrphanedBodyIdList(const nsTArray<nsID>& aDeletedBodyIdList);

  void MaybeAllowContextToClose();

  nsRefPtr<ManagerId> mManagerId;
  nsCOMPtr<nsIThread> mIOThread;

  
  Context* MOZ_NON_OWNING_REF mContext;

  
  struct ListenerEntry
  {
    ListenerEntry()
      : mId(UINT64_MAX)
      , mListener(nullptr)
    {
    }

    ListenerEntry(ListenerId aId, Listener* aListener)
      : mId(aId)
      , mListener(aListener)
    {
    }

    ListenerId mId;
    Listener* mListener;
  };

  class ListenerEntryIdComparator
  {
  public:
    bool Equals(const ListenerEntry& aA, const ListenerId& aB) const
    {
      return aA.mId == aB;
    }
  };

  class ListenerEntryListenerComparator
  {
  public:
    bool Equals(const ListenerEntry& aA, const Listener* aB) const
    {
      return aA.mListener == aB;
    }
  };

  typedef nsTArray<ListenerEntry> ListenerList;
  ListenerList mListeners;
  static ListenerId sNextListenerId;

  
  nsTArray<StreamList*> mStreamLists;

  bool mShuttingDown;
  bool mValid;

  struct CacheIdRefCounter
  {
    CacheId mCacheId;
    MozRefCountType mCount;
    bool mOrphaned;
  };
  nsTArray<CacheIdRefCounter> mCacheIdRefs;

  struct BodyIdRefCounter
  {
    nsID mBodyId;
    MozRefCountType mCount;
    bool mOrphaned;
  };
  nsTArray<BodyIdRefCounter> mBodyIdRefs;

public:
  NS_INLINE_DECL_REFCOUNTING(cache::Manager)
};

} 
} 
} 

#endif
