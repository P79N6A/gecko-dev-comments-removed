





#include "mozilla/dom/cache/Manager.h"

#include "mozilla/AutoRestore.h"
#include "mozilla/Mutex.h"
#include "mozilla/StaticMutex.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/dom/cache/Context.h"
#include "mozilla/dom/cache/DBAction.h"
#include "mozilla/dom/cache/DBSchema.h"
#include "mozilla/dom/cache/FileUtils.h"
#include "mozilla/dom/cache/ManagerId.h"
#include "mozilla/dom/cache/PCacheTypes.h"
#include "mozilla/dom/cache/SavedTypes.h"
#include "mozilla/dom/cache/StreamList.h"
#include "mozilla/dom/cache/Types.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozStorageHelper.h"
#include "nsAutoPtr.h"
#include "nsIInputStream.h"
#include "nsID.h"
#include "nsIFile.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "nsTObserverArray.h"

namespace {

using mozilla::unused;
using mozilla::dom::cache::Action;
using mozilla::dom::cache::DBSchema;
using mozilla::dom::cache::FileUtils;
using mozilla::dom::cache::QuotaInfo;
using mozilla::dom::cache::SyncDBAction;




class SetupAction final : public SyncDBAction
{
public:
  SetupAction()
    : SyncDBAction(DBAction::Create)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    
    
    
    
    

    nsresult rv = FileUtils::BodyCreateDir(aDBDir);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    mozStorageTransaction trans(aConn, false,
                                mozIStorageConnection::TRANSACTION_IMMEDIATE);

    rv = DBSchema::CreateSchema(aConn);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = trans.Commit();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return rv;
  }
};





class DeleteOrphanedBodyAction final : public Action
{
public:
  explicit DeleteOrphanedBodyAction(const nsTArray<nsID>& aDeletedBodyIdList)
    : mDeletedBodyIdList(aDeletedBodyIdList)
  { }

  explicit DeleteOrphanedBodyAction(const nsID& aBodyId)
  {
    mDeletedBodyIdList.AppendElement(aBodyId);
  }

  virtual void
  RunOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo) override
  {
    MOZ_ASSERT(aResolver);
    MOZ_ASSERT(aQuotaInfo.mDir);

    
    

    nsCOMPtr<nsIFile> dbDir;
    nsresult rv = aQuotaInfo.mDir->Clone(getter_AddRefs(dbDir));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      aResolver->Resolve(rv);
      return;
    }

    rv = dbDir->Append(NS_LITERAL_STRING("cache"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      aResolver->Resolve(rv);
      return;
    }

    rv = FileUtils::BodyDeleteFiles(dbDir, mDeletedBodyIdList);
    unused << NS_WARN_IF(NS_FAILED(rv));

    aResolver->Resolve(rv);
  }

private:
  nsTArray<nsID> mDeletedBodyIdList;
};

} 

namespace mozilla {
namespace dom {
namespace cache {





class Manager::Factory
{
public:
  friend class StaticAutoPtr<Manager::Factory>;

  static nsresult
  GetOrCreate(ManagerId* aManagerId, Manager** aManagerOut)
  {
    mozilla::ipc::AssertIsOnBackgroundThread();

    
    
    nsresult rv = MaybeCreateInstance();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    nsRefPtr<Manager> ref = Get(aManagerId);
    if (!ref) {
      
      nsCOMPtr<nsIThread> ioThread;
      rv = NS_NewNamedThread("DOMCacheThread", getter_AddRefs(ioThread));
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

      ref = new Manager(aManagerId, ioThread);

      MOZ_ASSERT(!sFactory->mManagerList.Contains(ref));
      sFactory->mManagerList.AppendElement(ref);
    }

    ref.forget(aManagerOut);

    return NS_OK;
  }

  static already_AddRefed<Manager>
  Get(ManagerId* aManagerId)
  {
    mozilla::ipc::AssertIsOnBackgroundThread();

    nsresult rv = MaybeCreateInstance();
    if (NS_WARN_IF(NS_FAILED(rv))) { return nullptr; }

    ManagerList::ForwardIterator iter(sFactory->mManagerList);
    while (iter.HasMore()) {
      nsRefPtr<Manager> manager = iter.GetNext();
      
      
      
      if (manager->IsValid() && *manager->mManagerId == *aManagerId) {
        return manager.forget();
      }
    }

    return nullptr;
  }

  static void
  Remove(Manager* aManager)
  {
    mozilla::ipc::AssertIsOnBackgroundThread();
    MOZ_ASSERT(aManager);
    MOZ_ASSERT(sFactory);

    MOZ_ALWAYS_TRUE(sFactory->mManagerList.RemoveElement(aManager));

    
    MaybeDestroyInstance();
  }

  static void
  StartShutdownAllOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    StaticMutexAutoLock lock(sMutex);

    sFactoryShutdown = true;

    if (!sBackgroundThread) {
      return;
    }

    
    
    nsCOMPtr<nsIRunnable> runnable = new ShutdownAllRunnable();
    nsresult rv = sBackgroundThread->Dispatch(runnable,
                                              nsIThread::DISPATCH_NORMAL);
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(rv));
  }

  static bool
  IsShutdownAllCompleteOnMainThread()
  {
    MOZ_ASSERT(NS_IsMainThread());
    StaticMutexAutoLock lock(sMutex);
    
    
    return sFactoryShutdown && !sBackgroundThread;
  }

private:
  Factory()
    : mInSyncShutdown(false)
  {
    MOZ_COUNT_CTOR(cache::Manager::Factory);
  }

  ~Factory()
  {
    MOZ_COUNT_DTOR(cache::Manager::Factory);
    MOZ_ASSERT(mManagerList.IsEmpty());
    MOZ_ASSERT(!mInSyncShutdown);
  }

  static nsresult
  MaybeCreateInstance()
  {
    mozilla::ipc::AssertIsOnBackgroundThread();

    if (!sFactory) {
      
      
      
      {
        StaticMutexAutoLock lock(sMutex);

        if (sFactoryShutdown) {
          return NS_ERROR_ILLEGAL_DURING_SHUTDOWN;
        }

        
        
        
        MOZ_ASSERT(!sBackgroundThread);
        sBackgroundThread = NS_GetCurrentThread();
      }

      
      
      
      
      sFactory = new Factory();
    }

    
    
    
    

    return NS_OK;
  }

  static void
  MaybeDestroyInstance()
  {
    mozilla::ipc::AssertIsOnBackgroundThread();
    MOZ_ASSERT(sFactory);

    
    
    
    
    if (!sFactory->mManagerList.IsEmpty() || sFactory->mInSyncShutdown) {
      return;
    }

    
    
    {
      StaticMutexAutoLock lock(sMutex);
      MOZ_ASSERT(sBackgroundThread);
      sBackgroundThread = nullptr;
    }

    sFactory = nullptr;
  }

  static void
  ShutdownAllOnBackgroundThread()
  {
    mozilla::ipc::AssertIsOnBackgroundThread();

    
    
    
    
    if (!sFactory) {
#ifdef DEBUG
      StaticMutexAutoLock lock(sMutex);
      MOZ_ASSERT(!sBackgroundThread);
#endif
      return;
    }

    MOZ_ASSERT(!sFactory->mManagerList.IsEmpty());

    {
      
      
      
      AutoRestore<bool> restore(sFactory->mInSyncShutdown);
      sFactory->mInSyncShutdown = true;

      ManagerList::ForwardIterator iter(sFactory->mManagerList);
      while (iter.HasMore()) {
        nsRefPtr<Manager> manager = iter.GetNext();
        manager->Shutdown();
      }
    }

    MaybeDestroyInstance();
  }

  class ShutdownAllRunnable final : public nsRunnable
  {
  public:
    NS_IMETHOD
    Run() override
    {
      mozilla::ipc::AssertIsOnBackgroundThread();
      ShutdownAllOnBackgroundThread();
      return NS_OK;
    }
  private:
    ~ShutdownAllRunnable() { }
  };

  
  
  
  static StaticAutoPtr<Factory> sFactory;

  
  static StaticMutex sMutex;

  
  
  static bool sFactoryShutdown;

  
  
  
  static StaticRefPtr<nsIThread> sBackgroundThread;

  
  
  
  typedef nsTObserverArray<Manager*> ManagerList;
  ManagerList mManagerList;

  
  
  
  bool mInSyncShutdown;
};


StaticAutoPtr<Manager::Factory> Manager::Factory::sFactory;


StaticMutex Manager::Factory::sMutex;


bool Manager::Factory::sFactoryShutdown = false;


StaticRefPtr<nsIThread> Manager::Factory::sBackgroundThread;






class Manager::BaseAction : public SyncDBAction
{
protected:
  BaseAction(Manager* aManager, ListenerId aListenerId)
    : SyncDBAction(DBAction::Existing)
    , mManager(aManager)
    , mListenerId(aListenerId)
  {
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) = 0;

  virtual void
  CompleteOnInitiatingThread(nsresult aRv) override
  {
    NS_ASSERT_OWNINGTHREAD(Manager::BaseAction);
    Listener* listener = mManager->GetListener(mListenerId);
    if (listener) {
      Complete(listener, aRv);
    }

    
    mManager = nullptr;
  }

  nsRefPtr<Manager> mManager;
  const ListenerId mListenerId;
};





class Manager::DeleteOrphanedCacheAction final : public SyncDBAction
{
public:
  DeleteOrphanedCacheAction(Manager* aManager, CacheId aCacheId)
    : SyncDBAction(DBAction::Existing)
    , mManager(aManager)
    , mCacheId(aCacheId)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    mozStorageTransaction trans(aConn, false,
                                mozIStorageConnection::TRANSACTION_IMMEDIATE);

    nsresult rv = DBSchema::DeleteCache(aConn, mCacheId, mDeletedBodyIdList);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = trans.Commit();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return rv;
  }

  virtual void
  CompleteOnInitiatingThread(nsresult aRv) override
  {
    mManager->NoteOrphanedBodyIdList(mDeletedBodyIdList);

    
    mManager = nullptr;
  }

private:
  nsRefPtr<Manager> mManager;
  const CacheId mCacheId;
  nsTArray<nsID> mDeletedBodyIdList;
};



class Manager::CacheMatchAction final : public Manager::BaseAction
{
public:
  CacheMatchAction(Manager* aManager, ListenerId aListenerId,
                   CacheId aCacheId, const CacheMatchArgs& aArgs,
                   StreamList* aStreamList)
    : BaseAction(aManager, aListenerId)
    , mCacheId(aCacheId)
    , mArgs(aArgs)
    , mStreamList(aStreamList)
    , mFoundResponse(false)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    nsresult rv = DBSchema::CacheMatch(aConn, mCacheId, mArgs.request(),
                                       mArgs.params(), &mFoundResponse,
                                       &mResponse);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    if (!mFoundResponse || !mResponse.mHasBodyId) {
      return rv;
    }

    nsCOMPtr<nsIInputStream> stream;
    rv = FileUtils::BodyOpen(aQuotaInfo, aDBDir, mResponse.mBodyId,
                             getter_AddRefs(stream));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    if (NS_WARN_IF(!stream)) { return NS_ERROR_FILE_NOT_FOUND; }

    mStreamList->Add(mResponse.mBodyId, stream);

    return rv;
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    if (!mFoundResponse) {
      aListener->OnOpComplete(aRv, CacheMatchResult(void_t()));
    } else {
      mStreamList->Activate(mCacheId);
      aListener->OnOpComplete(aRv, CacheMatchResult(void_t()), mResponse,
                              mStreamList);
    }
    mStreamList = nullptr;
  }

  virtual bool MatchesCacheId(CacheId aCacheId) const override
  {
    return aCacheId == mCacheId;
  }

private:
  const CacheId mCacheId;
  const CacheMatchArgs mArgs;
  nsRefPtr<StreamList> mStreamList;
  bool mFoundResponse;
  SavedResponse mResponse;
};



class Manager::CacheMatchAllAction final : public Manager::BaseAction
{
public:
  CacheMatchAllAction(Manager* aManager, ListenerId aListenerId,
                      CacheId aCacheId, const CacheMatchAllArgs& aArgs,
                      StreamList* aStreamList)
    : BaseAction(aManager, aListenerId)
    , mCacheId(aCacheId)
    , mArgs(aArgs)
    , mStreamList(aStreamList)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    nsresult rv = DBSchema::CacheMatchAll(aConn, mCacheId, mArgs.requestOrVoid(),
                                          mArgs.params(), mSavedResponses);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    for (uint32_t i = 0; i < mSavedResponses.Length(); ++i) {
      if (!mSavedResponses[i].mHasBodyId) {
        continue;
      }

      nsCOMPtr<nsIInputStream> stream;
      rv = FileUtils::BodyOpen(aQuotaInfo, aDBDir,
                               mSavedResponses[i].mBodyId,
                               getter_AddRefs(stream));
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
      if (NS_WARN_IF(!stream)) { return NS_ERROR_FILE_NOT_FOUND; }

      mStreamList->Add(mSavedResponses[i].mBodyId, stream);
    }

    return rv;
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    mStreamList->Activate(mCacheId);
    aListener->OnOpComplete(aRv, CacheMatchAllResult(), mSavedResponses,
                            mStreamList);
    mStreamList = nullptr;
  }

  virtual bool MatchesCacheId(CacheId aCacheId) const override
  {
    return aCacheId == mCacheId;
  }

private:
  const CacheId mCacheId;
  const CacheMatchAllArgs mArgs;
  nsRefPtr<StreamList> mStreamList;
  nsTArray<SavedResponse> mSavedResponses;
};






class Manager::CachePutAllAction final : public DBAction
{
public:
  CachePutAllAction(Manager* aManager, ListenerId aListenerId,
                    CacheId aCacheId,
                    const nsTArray<CacheRequestResponse>& aPutList,
                    const nsTArray<nsCOMPtr<nsIInputStream>>& aRequestStreamList,
                    const nsTArray<nsCOMPtr<nsIInputStream>>& aResponseStreamList)
    : DBAction(DBAction::Existing)
    , mManager(aManager)
    , mListenerId(aListenerId)
    , mCacheId(aCacheId)
    , mList(aPutList.Length())
    , mExpectedAsyncCopyCompletions(1)
    , mAsyncResult(NS_OK)
    , mMutex("cache::Manager::CachePutAllAction")
  {
    MOZ_ASSERT(!aPutList.IsEmpty());
    MOZ_ASSERT(aPutList.Length() == aRequestStreamList.Length());
    MOZ_ASSERT(aPutList.Length() == aResponseStreamList.Length());

    for (uint32_t i = 0; i < aPutList.Length(); ++i) {
      Entry* entry = mList.AppendElement();
      entry->mRequest = aPutList[i].request();
      entry->mRequestStream = aRequestStreamList[i];
      entry->mResponse = aPutList[i].response();
      entry->mResponseStream = aResponseStreamList[i];
    }
  }

private:
  ~CachePutAllAction() { }

  virtual void
  RunWithDBOnTarget(Resolver* aResolver, const QuotaInfo& aQuotaInfo,
                    nsIFile* aDBDir, mozIStorageConnection* aConn) override
  {
    MOZ_ASSERT(aResolver);
    MOZ_ASSERT(aDBDir);
    MOZ_ASSERT(aConn);
    MOZ_ASSERT(!mResolver);
    MOZ_ASSERT(!mDBDir);
    MOZ_ASSERT(!mConn);

    MOZ_ASSERT(!mTargetThread);
    mTargetThread = NS_GetCurrentThread();
    MOZ_ASSERT(mTargetThread);

    
    
    
    MOZ_ASSERT(mExpectedAsyncCopyCompletions == 1);

    mResolver = aResolver;
    mDBDir = aDBDir;
    mConn = aConn;

    
    
    
    nsresult rv = NS_OK;
    for (uint32_t i = 0; i < mList.Length(); ++i) {
      rv = StartStreamCopy(aQuotaInfo, mList[i], RequestStream,
                           &mExpectedAsyncCopyCompletions);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        break;
      }

      rv = StartStreamCopy(aQuotaInfo, mList[i], ResponseStream,
                           &mExpectedAsyncCopyCompletions);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        break;
      }
    }


    
    
    
    
    OnAsyncCopyComplete(rv);
  }

  
  
  
  void
  OnAsyncCopyComplete(nsresult aRv)
  {
    MOZ_ASSERT(mTargetThread == NS_GetCurrentThread());
    MOZ_ASSERT(mConn);
    MOZ_ASSERT(mResolver);
    MOZ_ASSERT(mExpectedAsyncCopyCompletions > 0);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (NS_SUCCEEDED(aRv) && IsCanceled()) {
      aRv = NS_ERROR_ABORT;
    }

    
    
    
    if (NS_FAILED(aRv) && NS_SUCCEEDED(mAsyncResult)) {
      CancelAllStreamCopying();
      mAsyncResult = aRv;
    }

    
    
    mExpectedAsyncCopyCompletions -= 1;
    if (mExpectedAsyncCopyCompletions > 0) {
      return;
    }

    
    
    {
      MutexAutoLock lock(mMutex);
      mCopyContextList.Clear();
    }

    
    
    if (NS_FAILED(mAsyncResult)) {
      DoResolve(mAsyncResult);
      return;
    }

    mozStorageTransaction trans(mConn, false,
                                mozIStorageConnection::TRANSACTION_IMMEDIATE);

    nsresult rv = NS_OK;
    for (uint32_t i = 0; i < mList.Length(); ++i) {
      Entry& e = mList[i];
      if (e.mRequestStream) {
        rv = FileUtils::BodyFinalizeWrite(mDBDir, e.mRequestBodyId);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          DoResolve(rv);
          return;
        }
      }
      if (e.mResponseStream) {
        rv = FileUtils::BodyFinalizeWrite(mDBDir, e.mResponseBodyId);
        if (NS_WARN_IF(NS_FAILED(rv))) {
          DoResolve(rv);
          return;
        }
      }

      rv = DBSchema::CachePut(mConn, mCacheId, e.mRequest,
                              e.mRequestStream ? &e.mRequestBodyId : nullptr,
                              e.mResponse,
                              e.mResponseStream ? &e.mResponseBodyId : nullptr,
                              mDeletedBodyIdList);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        DoResolve(rv);
        return;
      }
    }

    rv = trans.Commit();
    unused << NS_WARN_IF(NS_FAILED(rv));

    DoResolve(rv);
  }

  virtual void
  CompleteOnInitiatingThread(nsresult aRv) override
  {
    NS_ASSERT_OWNINGTHREAD(Action);

    for (uint32_t i = 0; i < mList.Length(); ++i) {
      mList[i].mRequestStream = nullptr;
      mList[i].mResponseStream = nullptr;
    }

    mManager->NoteOrphanedBodyIdList(mDeletedBodyIdList);

    Listener* listener = mManager->GetListener(mListenerId);
    mManager = nullptr;
    if (listener) {
      listener->OnOpComplete(aRv, CachePutAllResult());
    }
  }

  virtual void
  CancelOnInitiatingThread() override
  {
    NS_ASSERT_OWNINGTHREAD(Action);
    Action::CancelOnInitiatingThread();
    CancelAllStreamCopying();
  }

  virtual bool MatchesCacheId(CacheId aCacheId) const override
  {
    NS_ASSERT_OWNINGTHREAD(Action);
    return aCacheId == mCacheId;
  }

  struct Entry
  {
    PCacheRequest mRequest;
    nsCOMPtr<nsIInputStream> mRequestStream;
    nsID mRequestBodyId;
    nsCOMPtr<nsISupports> mRequestCopyContext;

    PCacheResponse mResponse;
    nsCOMPtr<nsIInputStream> mResponseStream;
    nsID mResponseBodyId;
    nsCOMPtr<nsISupports> mResponseCopyContext;
  };

  enum StreamId
  {
    RequestStream,
    ResponseStream
  };

  nsresult
  StartStreamCopy(const QuotaInfo& aQuotaInfo, Entry& aEntry,
                  StreamId aStreamId, uint32_t* aCopyCountOut)
  {
    MOZ_ASSERT(mTargetThread == NS_GetCurrentThread());
    MOZ_ASSERT(aCopyCountOut);

    if (IsCanceled()) {
      return NS_ERROR_ABORT;
    }

    nsCOMPtr<nsIInputStream> source;
    nsID* bodyId;

    if (aStreamId == RequestStream) {
      source = aEntry.mRequestStream;
      bodyId = &aEntry.mRequestBodyId;
    } else {
      MOZ_ASSERT(aStreamId == ResponseStream);
      source = aEntry.mResponseStream;
      bodyId = &aEntry.mResponseBodyId;
    }

    if (!source) {
      return NS_OK;
    }

    nsCOMPtr<nsISupports> copyContext;

    nsresult rv = FileUtils::BodyStartWriteStream(aQuotaInfo, mDBDir, source,
                                                  this, AsyncCopyCompleteFunc,
                                                  bodyId,
                                                  getter_AddRefs(copyContext));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    mBodyIdWrittenList.AppendElement(*bodyId);

    if (copyContext) {
      MutexAutoLock lock(mMutex);
      mCopyContextList.AppendElement(copyContext);
    }

    *aCopyCountOut += 1;

    return rv;
  }

  void
  CancelAllStreamCopying()
  {
    
    MutexAutoLock lock(mMutex);
    for (uint32_t i = 0; i < mCopyContextList.Length(); ++i) {
      FileUtils::BodyCancelWrite(mDBDir, mCopyContextList[i]);
    }
    mCopyContextList.Clear();
  }

  static void
  AsyncCopyCompleteFunc(void* aClosure, nsresult aRv)
  {
    
    MOZ_ASSERT(aClosure);
    
    
    CachePutAllAction* action = static_cast<CachePutAllAction*>(aClosure);
    action->CallOnAsyncCopyCompleteOnTargetThread(aRv);
  }

  void
  CallOnAsyncCopyCompleteOnTargetThread(nsresult aRv)
  {
    
    
    
    nsCOMPtr<nsIRunnable> runnable = NS_NewNonOwningRunnableMethodWithArgs<nsresult>(
      this, &CachePutAllAction::OnAsyncCopyComplete, aRv);
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
      mTargetThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL)));
  }

  void
  DoResolve(nsresult aRv)
  {
    MOZ_ASSERT(mTargetThread == NS_GetCurrentThread());

    
#ifdef DEBUG
    {
      MutexAutoLock lock(mMutex);
      MOZ_ASSERT(mCopyContextList.IsEmpty());
    }
#endif

    
    if (NS_FAILED(aRv)) {
      FileUtils::BodyDeleteFiles(mDBDir, mBodyIdWrittenList);
    }

    
    mConn = nullptr;

    
    
    
    mTargetThread = nullptr;

    
    nsRefPtr<Action::Resolver> resolver;
    mResolver.swap(resolver);
    resolver->Resolve(aRv);
  }

  
  nsRefPtr<Manager> mManager;
  const ListenerId mListenerId;

  
  
  const CacheId mCacheId;
  nsTArray<Entry> mList;
  uint32_t mExpectedAsyncCopyCompletions;

  
  nsRefPtr<Resolver> mResolver;
  nsCOMPtr<nsIFile> mDBDir;
  nsCOMPtr<mozIStorageConnection> mConn;
  nsCOMPtr<nsIThread> mTargetThread;
  nsresult mAsyncResult;
  nsTArray<nsID> mBodyIdWrittenList;

  
  
  nsTArray<nsID> mDeletedBodyIdList;

  
  Mutex mMutex;
  nsTArray<nsCOMPtr<nsISupports>> mCopyContextList;
};



class Manager::CacheDeleteAction final : public Manager::BaseAction
{
public:
  CacheDeleteAction(Manager* aManager, ListenerId aListenerId,
                    CacheId aCacheId, const CacheDeleteArgs& aArgs)
    : BaseAction(aManager, aListenerId)
    , mCacheId(aCacheId)
    , mArgs(aArgs)
    , mSuccess(false)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    mozStorageTransaction trans(aConn, false,
                                mozIStorageConnection::TRANSACTION_IMMEDIATE);

    nsresult rv = DBSchema::CacheDelete(aConn, mCacheId, mArgs.request(),
                                        mArgs.params(), mDeletedBodyIdList,
                                        &mSuccess);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = trans.Commit();
    if (NS_WARN_IF(NS_FAILED(rv))) {
      mSuccess = false;
      return rv;
    }

    return rv;
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    mManager->NoteOrphanedBodyIdList(mDeletedBodyIdList);
    aListener->OnOpComplete(aRv, CacheDeleteResult(mSuccess));
  }

  virtual bool MatchesCacheId(CacheId aCacheId) const override
  {
    return aCacheId == mCacheId;
  }

private:
  const CacheId mCacheId;
  const CacheDeleteArgs mArgs;
  bool mSuccess;
  nsTArray<nsID> mDeletedBodyIdList;
};



class Manager::CacheKeysAction final : public Manager::BaseAction
{
public:
  CacheKeysAction(Manager* aManager, ListenerId aListenerId,
                  CacheId aCacheId, const CacheKeysArgs& aArgs,
                  StreamList* aStreamList)
    : BaseAction(aManager, aListenerId)
    , mCacheId(aCacheId)
    , mArgs(aArgs)
    , mStreamList(aStreamList)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    nsresult rv = DBSchema::CacheKeys(aConn, mCacheId, mArgs.requestOrVoid(),
                                      mArgs.params(), mSavedRequests);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    for (uint32_t i = 0; i < mSavedRequests.Length(); ++i) {
      if (!mSavedRequests[i].mHasBodyId) {
        continue;
      }

      nsCOMPtr<nsIInputStream> stream;
      rv = FileUtils::BodyOpen(aQuotaInfo, aDBDir,
                               mSavedRequests[i].mBodyId,
                               getter_AddRefs(stream));
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
      if (NS_WARN_IF(!stream)) { return NS_ERROR_FILE_NOT_FOUND; }

      mStreamList->Add(mSavedRequests[i].mBodyId, stream);
    }

    return rv;
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    mStreamList->Activate(mCacheId);
    aListener->OnOpComplete(aRv, CacheKeysResult(), mSavedRequests,
                            mStreamList);
    mStreamList = nullptr;
  }

  virtual bool MatchesCacheId(CacheId aCacheId) const override
  {
    return aCacheId == mCacheId;
  }

private:
  const CacheId mCacheId;
  const CacheKeysArgs mArgs;
  nsRefPtr<StreamList> mStreamList;
  nsTArray<SavedRequest> mSavedRequests;
};



class Manager::StorageMatchAction final : public Manager::BaseAction
{
public:
  StorageMatchAction(Manager* aManager, ListenerId aListenerId,
                     Namespace aNamespace,
                     const StorageMatchArgs& aArgs,
                     StreamList* aStreamList)
    : BaseAction(aManager, aListenerId)
    , mNamespace(aNamespace)
    , mArgs(aArgs)
    , mStreamList(aStreamList)
    , mFoundResponse(false)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    nsresult rv = DBSchema::StorageMatch(aConn, mNamespace, mArgs.request(),
                                         mArgs.params(), &mFoundResponse,
                                         &mSavedResponse);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    if (!mFoundResponse || !mSavedResponse.mHasBodyId) {
      return rv;
    }

    nsCOMPtr<nsIInputStream> stream;
    rv = FileUtils::BodyOpen(aQuotaInfo, aDBDir, mSavedResponse.mBodyId,
                             getter_AddRefs(stream));
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    if (NS_WARN_IF(!stream)) { return NS_ERROR_FILE_NOT_FOUND; }

    mStreamList->Add(mSavedResponse.mBodyId, stream);

    return rv;
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    if (!mFoundResponse) {
      aListener->OnOpComplete(aRv, StorageMatchResult(void_t()));
    } else {
      mStreamList->Activate(mSavedResponse.mCacheId);
      aListener->OnOpComplete(aRv, StorageMatchResult(void_t()), mSavedResponse,
                              mStreamList);
    }
    mStreamList = nullptr;
  }

private:
  const Namespace mNamespace;
  const StorageMatchArgs mArgs;
  nsRefPtr<StreamList> mStreamList;
  bool mFoundResponse;
  SavedResponse mSavedResponse;
};



class Manager::StorageHasAction final : public Manager::BaseAction
{
public:
  StorageHasAction(Manager* aManager, ListenerId aListenerId,
                   Namespace aNamespace, const StorageHasArgs& aArgs)
    : BaseAction(aManager, aListenerId)
    , mNamespace(aNamespace)
    , mArgs(aArgs)
    , mCacheFound(false)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    CacheId cacheId;
    return DBSchema::StorageGetCacheId(aConn, mNamespace, mArgs.key(),
                                       &mCacheFound, &cacheId);
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    aListener->OnOpComplete(aRv, StorageHasResult(mCacheFound));
  }

private:
  const Namespace mNamespace;
  const StorageHasArgs mArgs;
  bool mCacheFound;
};



class Manager::StorageOpenAction final : public Manager::BaseAction
{
public:
  StorageOpenAction(Manager* aManager, ListenerId aListenerId,
                    Namespace aNamespace, const StorageOpenArgs& aArgs)
    : BaseAction(aManager, aListenerId)
    , mNamespace(aNamespace)
    , mArgs(aArgs)
    , mCacheId(INVALID_CACHE_ID)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    
    mozStorageTransaction trans(aConn, false,
                                mozIStorageConnection::TRANSACTION_IMMEDIATE);

    
    bool cacheFound;
    nsresult rv = DBSchema::StorageGetCacheId(aConn, mNamespace, mArgs.key(),
                                              &cacheFound, &mCacheId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    if (cacheFound) {
      return rv;
    }

    rv = DBSchema::CreateCache(aConn, &mCacheId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = DBSchema::StoragePutCache(aConn, mNamespace, mArgs.key(), mCacheId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = trans.Commit();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return rv;
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    aListener->OnOpComplete(aRv, StorageOpenResult(), mCacheId);
  }

private:
  const Namespace mNamespace;
  const StorageOpenArgs mArgs;
  CacheId mCacheId;
};



class Manager::StorageDeleteAction final : public Manager::BaseAction
{
public:
  StorageDeleteAction(Manager* aManager, ListenerId aListenerId,
                      Namespace aNamespace, const StorageDeleteArgs& aArgs)
    : BaseAction(aManager, aListenerId)
    , mNamespace(aNamespace)
    , mArgs(aArgs)
    , mCacheDeleted(false)
    , mCacheId(INVALID_CACHE_ID)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    mozStorageTransaction trans(aConn, false,
                                mozIStorageConnection::TRANSACTION_IMMEDIATE);

    bool exists;
    nsresult rv = DBSchema::StorageGetCacheId(aConn, mNamespace, mArgs.key(),
                                              &exists, &mCacheId);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    if (!exists) {
      mCacheDeleted = false;
      return NS_OK;
    }

    rv = DBSchema::StorageForgetCache(aConn, mNamespace, mArgs.key());
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    rv = trans.Commit();
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    mCacheDeleted = true;
    return rv;
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    if (mCacheDeleted) {
      
      
      if (!mManager->SetCacheIdOrphanedIfRefed(mCacheId)) {

        
        nsRefPtr<Context> context = mManager->CurrentContext();
        context->CancelForCacheId(mCacheId);
        nsRefPtr<Action> action =
          new DeleteOrphanedCacheAction(mManager, mCacheId);
        context->Dispatch(mManager->mIOThread, action);
      }
    }

    aListener->OnOpComplete(aRv, StorageDeleteResult(mCacheDeleted));
  }

private:
  const Namespace mNamespace;
  const StorageDeleteArgs mArgs;
  bool mCacheDeleted;
  CacheId mCacheId;
};



class Manager::StorageKeysAction final : public Manager::BaseAction
{
public:
  StorageKeysAction(Manager* aManager, ListenerId aListenerId,
                    Namespace aNamespace)
    : BaseAction(aManager, aListenerId)
    , mNamespace(aNamespace)
  { }

  virtual nsresult
  RunSyncWithDBOnTarget(const QuotaInfo& aQuotaInfo, nsIFile* aDBDir,
                        mozIStorageConnection* aConn) override
  {
    return DBSchema::StorageGetKeys(aConn, mNamespace, mKeys);
  }

  virtual void
  Complete(Listener* aListener, nsresult aRv) override
  {
    if (NS_FAILED(aRv)) {
      mKeys.Clear();
    }
    aListener->OnOpComplete(aRv, StorageKeysResult(mKeys));
  }

private:
  const Namespace mNamespace;
  nsTArray<nsString> mKeys;
};




Manager::ListenerId Manager::sNextListenerId = 0;

void
Manager::Listener::OnOpComplete(nsresult aRv, const CacheOpResult& aResult)
{
  OnOpComplete(aRv, aResult, INVALID_CACHE_ID, nsTArray<SavedResponse>(),
               nsTArray<SavedRequest>(), nullptr);
}

void
Manager::Listener::OnOpComplete(nsresult aRv, const CacheOpResult& aResult,
                                CacheId aOpenedCacheId)
{
  OnOpComplete(aRv, aResult, aOpenedCacheId, nsTArray<SavedResponse>(),
               nsTArray<SavedRequest>(), nullptr);
}

void
Manager::Listener::OnOpComplete(nsresult aRv, const CacheOpResult& aResult,
                                const SavedResponse& aSavedResponse,
                                StreamList* aStreamList)
{
  nsAutoTArray<SavedResponse, 1> responseList;
  responseList.AppendElement(aSavedResponse);
  OnOpComplete(aRv, aResult, INVALID_CACHE_ID, responseList,
               nsTArray<SavedRequest>(), aStreamList);
}

void
Manager::Listener::OnOpComplete(nsresult aRv, const CacheOpResult& aResult,
                                const nsTArray<SavedResponse>& aSavedResponseList,
                                StreamList* aStreamList)
{
  OnOpComplete(aRv, aResult, INVALID_CACHE_ID, aSavedResponseList,
               nsTArray<SavedRequest>(), aStreamList);
}

void
Manager::Listener::OnOpComplete(nsresult aRv, const CacheOpResult& aResult,
                                const nsTArray<SavedRequest>& aSavedRequestList,
                                StreamList* aStreamList)
{
  OnOpComplete(aRv, aResult, INVALID_CACHE_ID, nsTArray<SavedResponse>(),
               aSavedRequestList, aStreamList);
}


nsresult
Manager::GetOrCreate(ManagerId* aManagerId, Manager** aManagerOut)
{
  mozilla::ipc::AssertIsOnBackgroundThread();
  return Factory::GetOrCreate(aManagerId, aManagerOut);
}


already_AddRefed<Manager>
Manager::Get(ManagerId* aManagerId)
{
  mozilla::ipc::AssertIsOnBackgroundThread();
  return Factory::Get(aManagerId);
}


void
Manager::ShutdownAllOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  Factory::StartShutdownAllOnMainThread();

  while (!Factory::IsShutdownAllCompleteOnMainThread()) {
    if (!NS_ProcessNextEvent()) {
      NS_WARNING("Something bad happened!");
      break;
    }
  }
}

void
Manager::RemoveListener(Listener* aListener)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  
  
  mListeners.RemoveElement(aListener, ListenerEntryListenerComparator());
  MOZ_ASSERT(!mListeners.Contains(aListener,
                                  ListenerEntryListenerComparator()));
  MaybeAllowContextToClose();
}

void
Manager::RemoveContext(Context* aContext)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  MOZ_ASSERT(mContext);
  MOZ_ASSERT(mContext == aContext);

  
  
  
  MOZ_ASSERT(!mValid);

  mContext = nullptr;

  
  
  if (mShuttingDown) {
    Factory::Remove(this);
  }
}

void
Manager::Invalidate()
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  
  mValid = false;
}

bool
Manager::IsValid() const
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  return mValid;
}

void
Manager::AddRefCacheId(CacheId aCacheId)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  for (uint32_t i = 0; i < mCacheIdRefs.Length(); ++i) {
    if (mCacheIdRefs[i].mCacheId == aCacheId) {
      mCacheIdRefs[i].mCount += 1;
      return;
    }
  }
  CacheIdRefCounter* entry = mCacheIdRefs.AppendElement();
  entry->mCacheId = aCacheId;
  entry->mCount = 1;
  entry->mOrphaned = false;
}

void
Manager::ReleaseCacheId(CacheId aCacheId)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  for (uint32_t i = 0; i < mCacheIdRefs.Length(); ++i) {
    if (mCacheIdRefs[i].mCacheId == aCacheId) {
      DebugOnly<uint32_t> oldRef = mCacheIdRefs[i].mCount;
      mCacheIdRefs[i].mCount -= 1;
      MOZ_ASSERT(mCacheIdRefs[i].mCount < oldRef);
      if (mCacheIdRefs[i].mCount == 0) {
        bool orphaned = mCacheIdRefs[i].mOrphaned;
        mCacheIdRefs.RemoveElementAt(i);
        
        if (orphaned && !mShuttingDown && mValid) {
          nsRefPtr<Context> context = CurrentContext();
          context->CancelForCacheId(aCacheId);
          nsRefPtr<Action> action = new DeleteOrphanedCacheAction(this,
                                                                  aCacheId);
          context->Dispatch(mIOThread, action);
        }
      }
      MaybeAllowContextToClose();
      return;
    }
  }
  MOZ_ASSERT_UNREACHABLE("Attempt to release CacheId that is not referenced!");
}

void
Manager::AddRefBodyId(const nsID& aBodyId)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  for (uint32_t i = 0; i < mBodyIdRefs.Length(); ++i) {
    if (mBodyIdRefs[i].mBodyId == aBodyId) {
      mBodyIdRefs[i].mCount += 1;
      return;
    }
  }
  BodyIdRefCounter* entry = mBodyIdRefs.AppendElement();
  entry->mBodyId = aBodyId;
  entry->mCount = 1;
  entry->mOrphaned = false;
}

void
Manager::ReleaseBodyId(const nsID& aBodyId)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  for (uint32_t i = 0; i < mBodyIdRefs.Length(); ++i) {
    if (mBodyIdRefs[i].mBodyId == aBodyId) {
      DebugOnly<uint32_t> oldRef = mBodyIdRefs[i].mCount;
      mBodyIdRefs[i].mCount -= 1;
      MOZ_ASSERT(mBodyIdRefs[i].mCount < oldRef);
      if (mBodyIdRefs[i].mCount < 1) {
        bool orphaned = mBodyIdRefs[i].mOrphaned;
        mBodyIdRefs.RemoveElementAt(i);
        
        if (orphaned && !mShuttingDown && mValid) {
          nsRefPtr<Action> action = new DeleteOrphanedBodyAction(aBodyId);
          nsRefPtr<Context> context = CurrentContext();
          context->Dispatch(mIOThread, action);
        }
      }
      MaybeAllowContextToClose();
      return;
    }
  }
  MOZ_ASSERT_UNREACHABLE("Attempt to release BodyId that is not referenced!");
}

already_AddRefed<ManagerId>
Manager::GetManagerId() const
{
  nsRefPtr<ManagerId> ref = mManagerId;
  return ref.forget();
}

void
Manager::AddStreamList(StreamList* aStreamList)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  MOZ_ASSERT(aStreamList);
  mStreamLists.AppendElement(aStreamList);
}

void
Manager::RemoveStreamList(StreamList* aStreamList)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  MOZ_ASSERT(aStreamList);
  mStreamLists.RemoveElement(aStreamList);
}

void
Manager::ExecuteCacheOp(Listener* aListener, CacheId aCacheId,
                        const CacheOpArgs& aOpArgs)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  MOZ_ASSERT(aListener);
  MOZ_ASSERT(aOpArgs.type() != CacheOpArgs::TCacheAddAllArgs);
  MOZ_ASSERT(aOpArgs.type() != CacheOpArgs::TCachePutAllArgs);

  if (mShuttingDown || !mValid) {
    aListener->OnOpComplete(NS_ERROR_FAILURE, void_t());
    return;
  }

  nsRefPtr<Context> context = CurrentContext();
  nsRefPtr<StreamList> streamList = new StreamList(this, context);
  ListenerId listenerId = SaveListener(aListener);

  nsRefPtr<Action> action;
  switch(aOpArgs.type()) {
    case CacheOpArgs::TCacheMatchArgs:
      action = new CacheMatchAction(this, listenerId, aCacheId,
                                    aOpArgs.get_CacheMatchArgs(), streamList);
      break;
    case CacheOpArgs::TCacheMatchAllArgs:
      action = new CacheMatchAllAction(this, listenerId, aCacheId,
                                       aOpArgs.get_CacheMatchAllArgs(),
                                       streamList);
      break;
    case CacheOpArgs::TCacheDeleteArgs:
      action = new CacheDeleteAction(this, listenerId, aCacheId,
                                     aOpArgs.get_CacheDeleteArgs());
      break;
    case CacheOpArgs::TCacheKeysArgs:
      action = new CacheKeysAction(this, listenerId, aCacheId,
                                   aOpArgs.get_CacheKeysArgs(), streamList);
      break;
    default:
      MOZ_CRASH("Unknown Cache operation!");
  }

  context->Dispatch(mIOThread, action);
}

void
Manager::ExecuteStorageOp(Listener* aListener, Namespace aNamespace,
                          const CacheOpArgs& aOpArgs)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  MOZ_ASSERT(aListener);

  if (mShuttingDown || !mValid) {
    aListener->OnOpComplete(NS_ERROR_FAILURE, void_t());
    return;
  }

  nsRefPtr<Context> context = CurrentContext();
  nsRefPtr<StreamList> streamList = new StreamList(this, context);
  ListenerId listenerId = SaveListener(aListener);

  nsRefPtr<Action> action;
  switch(aOpArgs.type()) {
    case CacheOpArgs::TStorageMatchArgs:
      action = new StorageMatchAction(this, listenerId, aNamespace,
                                      aOpArgs.get_StorageMatchArgs(),
                                      streamList);
      break;
    case CacheOpArgs::TStorageHasArgs:
      action = new StorageHasAction(this, listenerId, aNamespace,
                                    aOpArgs.get_StorageHasArgs());
      break;
    case CacheOpArgs::TStorageOpenArgs:
      action = new StorageOpenAction(this, listenerId, aNamespace,
                                     aOpArgs.get_StorageOpenArgs());
      break;
    case CacheOpArgs::TStorageDeleteArgs:
      action = new StorageDeleteAction(this, listenerId, aNamespace,
                                       aOpArgs.get_StorageDeleteArgs());
      break;
    case CacheOpArgs::TStorageKeysArgs:
      action = new StorageKeysAction(this, listenerId, aNamespace);
      break;
    default:
      MOZ_CRASH("Unknown CacheStorage operation!");
  }

  context->Dispatch(mIOThread, action);
}

void
Manager::ExecutePutAll(Listener* aListener, CacheId aCacheId,
                       const nsTArray<CacheRequestResponse>& aPutList,
                       const nsTArray<nsCOMPtr<nsIInputStream>>& aRequestStreamList,
                       const nsTArray<nsCOMPtr<nsIInputStream>>& aResponseStreamList)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  MOZ_ASSERT(aListener);

  if (mShuttingDown || !mValid) {
    aListener->OnOpComplete(NS_ERROR_FAILURE, CachePutAllResult());
    return;
  }

  nsRefPtr<Context> context = CurrentContext();
  ListenerId listenerId = SaveListener(aListener);

  nsRefPtr<Action> action = new CachePutAllAction(this, listenerId, aCacheId,
                                                  aPutList, aRequestStreamList,
                                                  aResponseStreamList);

  context->Dispatch(mIOThread, action);
}

Manager::Manager(ManagerId* aManagerId, nsIThread* aIOThread)
  : mManagerId(aManagerId)
  , mIOThread(aIOThread)
  , mContext(nullptr)
  , mShuttingDown(false)
  , mValid(true)
{
  MOZ_ASSERT(mManagerId);
  MOZ_ASSERT(mIOThread);
}

Manager::~Manager()
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  MOZ_ASSERT(!mContext);
  Shutdown();

  nsCOMPtr<nsIThread> ioThread;
  mIOThread.swap(ioThread);

  
  
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(ioThread, &nsIThread::Shutdown);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
}

void
Manager::Shutdown()
{
  NS_ASSERT_OWNINGTHREAD(Manager);

  
  
  
  if (mShuttingDown) {
    return;
  }

  
  
  
  mShuttingDown = true;

  
  
  if (mContext) {
    nsRefPtr<Context> context = mContext;
    context->CancelAll();
    return;
  }

  
  Factory::Remove(this);
}

already_AddRefed<Context>
Manager::CurrentContext()
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  nsRefPtr<Context> ref = mContext;
  if (!ref) {
    MOZ_ASSERT(!mShuttingDown);
    MOZ_ASSERT(mValid);
    nsRefPtr<Action> setupAction = new SetupAction();
    ref = Context::Create(this, setupAction);
    mContext = ref;
  }
  return ref.forget();
}

Manager::ListenerId
Manager::SaveListener(Listener* aListener)
{
  NS_ASSERT_OWNINGTHREAD(Manager);

  
  
  
  ListenerList::index_type index =
    mListeners.IndexOf(aListener, 0, ListenerEntryListenerComparator());
  if (index != ListenerList::NoIndex) {
    return mListeners[index].mId;
  }

  ListenerId id = sNextListenerId;
  sNextListenerId += 1;

  mListeners.AppendElement(ListenerEntry(id, aListener));
  return id;
}

Manager::Listener*
Manager::GetListener(ListenerId aListenerId) const
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  ListenerList::index_type index =
    mListeners.IndexOf(aListenerId, 0, ListenerEntryIdComparator());
  if (index != ListenerList::NoIndex) {
    return mListeners[index].mListener;
  }

  
  
  return nullptr;
}

bool
Manager::SetCacheIdOrphanedIfRefed(CacheId aCacheId)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  for (uint32_t i = 0; i < mCacheIdRefs.Length(); ++i) {
    if (mCacheIdRefs[i].mCacheId == aCacheId) {
      MOZ_ASSERT(mCacheIdRefs[i].mCount > 0);
      MOZ_ASSERT(!mCacheIdRefs[i].mOrphaned);
      mCacheIdRefs[i].mOrphaned = true;
      return true;
    }
  }
  return false;
}



bool
Manager::SetBodyIdOrphanedIfRefed(const nsID& aBodyId)
{
  NS_ASSERT_OWNINGTHREAD(Manager);
  for (uint32_t i = 0; i < mBodyIdRefs.Length(); ++i) {
    if (mBodyIdRefs[i].mBodyId == aBodyId) {
      MOZ_ASSERT(mBodyIdRefs[i].mCount > 0);
      MOZ_ASSERT(!mBodyIdRefs[i].mOrphaned);
      mBodyIdRefs[i].mOrphaned = true;
      return true;
    }
  }
  return false;
}

void
Manager::NoteOrphanedBodyIdList(const nsTArray<nsID>& aDeletedBodyIdList)
{
  NS_ASSERT_OWNINGTHREAD(Manager);

  nsAutoTArray<nsID, 64> deleteNowList;
  deleteNowList.SetCapacity(aDeletedBodyIdList.Length());

  for (uint32_t i = 0; i < aDeletedBodyIdList.Length(); ++i) {
    if (!SetBodyIdOrphanedIfRefed(aDeletedBodyIdList[i])) {
      deleteNowList.AppendElement(aDeletedBodyIdList[i]);
    }
  }

  if (!deleteNowList.IsEmpty()) {
    nsRefPtr<Action> action = new DeleteOrphanedBodyAction(deleteNowList);
    nsRefPtr<Context> context = CurrentContext();
    context->Dispatch(mIOThread, action);
  }
}

void
Manager::MaybeAllowContextToClose()
{
  NS_ASSERT_OWNINGTHREAD(Manager);

  
  
  
  
  
  if (mContext && mListeners.IsEmpty()
               && mCacheIdRefs.IsEmpty()
               && mBodyIdRefs.IsEmpty()) {

    
    
    
    mValid = false;

    mContext->AllowToClose();
  }
}

} 
} 
} 
