



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

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WorkerDataStoreCursor, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WorkerDataStoreCursor, Release)

NS_IMPL_CYCLE_COLLECTION(WorkerDataStoreCursor, mWorkerStore)

WorkerDataStoreCursor::WorkerDataStoreCursor(WorkerDataStore* aWorkerStore)
  : mWorkerStore(aWorkerStore)
{
  MOZ_ASSERT(!NS_IsMainThread());
}

already_AddRefed<WorkerDataStoreCursor>
WorkerDataStoreCursor::Constructor(GlobalObject& aGlobal, ErrorResult& aRv)
{
  
  
  NS_NOTREACHED("Cannot use the chrome constructor on the worker!");
  return nullptr;
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

  
  
  
  MOZ_ASSERT(mWorkerStore);
  nsRefPtr<WorkerDataStore> workerStore = mWorkerStore;
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
