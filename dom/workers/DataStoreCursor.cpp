



#include "DataStore.h"
#include "DataStoreCursor.h"

#include "mozilla/dom/DataStore.h"
#include "mozilla/dom/DataStoreCursor.h"
#include "mozilla/dom/DataStoreBinding.h"
#include "mozilla/dom/DataStoreImplBinding.h"

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/ErrorResult.h"

#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"

BEGIN_WORKERS_NAMESPACE

already_AddRefed<WorkerDataStoreCursor>
WorkerDataStoreCursor::Constructor(GlobalObject& aGlobal, ErrorResult& aRv)
{
  MOZ_ASSERT(!NS_IsMainThread());
  nsRefPtr<WorkerDataStoreCursor> workerCursor = new WorkerDataStoreCursor();
  return workerCursor.forget();
}

JSObject*
WorkerDataStoreCursor::WrapObject(JSContext* aCx)
{
  return DataStoreCursorBinding_workers::Wrap(aCx, this);
}


class DataStoreCursorRunnable : public WorkerMainThreadRunnable
{
protected:
  nsMainThreadPtrHandle<DataStoreCursor> mBackingCursor;

public:
  DataStoreCursorRunnable(WorkerPrivate* aWorkerPrivate,
                          const nsMainThreadPtrHandle<DataStoreCursor>& aBackingCursor)
    : WorkerMainThreadRunnable(aWorkerPrivate)
    , mBackingCursor(aBackingCursor)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }
};



class DataStoreCursorGetStoreRunnable MOZ_FINAL : public DataStoreCursorRunnable
{
  WorkerDataStore* mWorkerStore;
  ErrorResult& mRv;
  nsRefPtr<DataStoreChangeEventProxy> mEventProxy;

public:
  DataStoreCursorGetStoreRunnable(WorkerPrivate* aWorkerPrivate,
                                  const nsMainThreadPtrHandle<DataStoreCursor>& aBackingCursor,
                                  WorkerDataStore* aWorkerStore,
                                  ErrorResult& aRv)
    : DataStoreCursorRunnable(aWorkerPrivate, aBackingCursor)
    , mWorkerStore(aWorkerStore)
    , mRv(aRv)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();

    
    mEventProxy = new DataStoreChangeEventProxy(aWorkerPrivate, mWorkerStore);
  }

protected:
  virtual bool
  MainThreadRun() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    nsRefPtr<DataStore> store = mBackingCursor->GetStore(mRv);

    
    if (NS_FAILED(store->AddEventListener(NS_LITERAL_STRING("change"),
                                          mEventProxy,
                                          false,
                                          false,
                                          2))) {
      NS_WARNING("Failed to add event listener!");
      return false;
    }

    
    nsMainThreadPtrHandle<DataStore> backingStore =
      new nsMainThreadPtrHolder<DataStore>(store);
    mWorkerStore->SetBackingDataStore(backingStore);

    return true;
  }
};



class DataStoreCursorNextRunnable MOZ_FINAL : public DataStoreCursorRunnable
{
  nsRefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
  ErrorResult& mRv;

public:
  DataStoreCursorNextRunnable(WorkerPrivate* aWorkerPrivate,
                              const nsMainThreadPtrHandle<DataStoreCursor>& aBackingCursor,
                              Promise* aWorkerPromise,
                              ErrorResult& aRv)
    : DataStoreCursorRunnable(aWorkerPrivate, aBackingCursor)
    , mRv(aRv)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();

    mPromiseWorkerProxy =
      new PromiseWorkerProxy(aWorkerPrivate, aWorkerPromise);
  }

protected:
  virtual bool
  MainThreadRun() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    nsRefPtr<Promise> promise = mBackingCursor->Next(mRv);
    promise->AppendNativeHandler(mPromiseWorkerProxy);
    return true;
  }
};



class DataStoreCursorCloseRunnable MOZ_FINAL : public DataStoreCursorRunnable
{
  ErrorResult& mRv;

public:
  DataStoreCursorCloseRunnable(WorkerPrivate* aWorkerPrivate,
                               const nsMainThreadPtrHandle<DataStoreCursor>& aBackingCursor,
                               ErrorResult& aRv)
    : DataStoreCursorRunnable(aWorkerPrivate, aBackingCursor)
    , mRv(aRv)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

protected:
  virtual bool
  MainThreadRun() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    mBackingCursor->Close(mRv);
    return true;
  }
};

already_AddRefed<WorkerDataStore>
WorkerDataStoreCursor::GetStore(JSContext* aCx, ErrorResult& aRv)
{
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  
  
  nsRefPtr<WorkerDataStore> workerStore =
    new WorkerDataStore(workerPrivate->GlobalScope());

  nsRefPtr<DataStoreCursorGetStoreRunnable> runnable =
    new DataStoreCursorGetStoreRunnable(workerPrivate,
                                        mBackingCursor,
                                        workerStore,
                                        aRv);
  runnable->Dispatch(aCx);

  return workerStore.forget();
}

already_AddRefed<Promise>
WorkerDataStoreCursor::Next(JSContext* aCx, ErrorResult& aRv)
{
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  nsRefPtr<Promise> promise = new Promise(workerPrivate->GlobalScope());

  nsRefPtr<DataStoreCursorNextRunnable> runnable =
    new DataStoreCursorNextRunnable(workerPrivate,
                                    mBackingCursor,
                                    promise,
                                    aRv);
  runnable->Dispatch(aCx);

  return promise.forget();
}

void
WorkerDataStoreCursor::Close(JSContext* aCx, ErrorResult& aRv)
{
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  nsRefPtr<DataStoreCursorCloseRunnable> runnable =
    new DataStoreCursorCloseRunnable(workerPrivate, mBackingCursor, aRv);
  runnable->Dispatch(aCx);
}

void
WorkerDataStoreCursor::SetDataStoreCursorImpl(DataStoreCursorImpl& aCursor)
{
  NS_NOTREACHED("We don't use this for the WorkerDataStoreCursor!");
}

void
WorkerDataStoreCursor::SetBackingDataStoreCursor(
  const nsMainThreadPtrHandle<DataStoreCursor>& aBackingCursor)
{
  mBackingCursor = aBackingCursor;
}

END_WORKERS_NAMESPACE
