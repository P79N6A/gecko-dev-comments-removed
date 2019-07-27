





#include "IDBTransaction.h"

#include "BackgroundChildImpl.h"
#include "IDBDatabase.h"
#include "IDBEvents.h"
#include "IDBObjectStore.h"
#include "IDBRequest.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/DOMStringList.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "nsIAppShell.h"
#include "nsIDOMFile.h"
#include "nsPIDOMWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsTHashtable.h"
#include "nsWidgetsCID.h"
#include "ProfilerHelpers.h"
#include "ReportInternalError.h"


#include "ActorsChild.h"

namespace mozilla {
namespace dom {
namespace indexedDB {

using namespace mozilla::ipc;

namespace {

NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

} 

IDBTransaction::IDBTransaction(IDBDatabase* aDatabase,
                               const nsTArray<nsString>& aObjectStoreNames,
                               Mode aMode)
  : IDBWrapperCache(aDatabase)
  , mDatabase(aDatabase)
  , mObjectStoreNames(aObjectStoreNames)
  , mLoggingSerialNumber(0)
  , mNextObjectStoreId(0)
  , mNextIndexId(0)
  , mAbortCode(NS_OK)
  , mPendingRequestCount(0)
  , mLineNo(0)
  , mReadyState(IDBTransaction::INITIAL)
  , mMode(aMode)
  , mCreating(false)
  , mAbortedByScript(false)
#ifdef DEBUG
  , mSentCommitOrAbort(false)
  , mFiredCompleteOrAbort(false)
#endif
{
  MOZ_ASSERT(aDatabase);
  aDatabase->AssertIsOnOwningThread();

  mBackgroundActor.mNormalBackgroundActor = nullptr;

  BackgroundChildImpl::ThreadLocal* threadLocal =
    BackgroundChildImpl::GetThreadLocalForCurrentThread();
  MOZ_ASSERT(threadLocal);

  ThreadLocal* idbThreadLocal = threadLocal->mIndexedDBThreadLocal;
  MOZ_ASSERT(idbThreadLocal);

  const_cast<int64_t&>(mLoggingSerialNumber) =
    idbThreadLocal->NextTransactionSN(aMode);

#ifdef DEBUG
  if (!aObjectStoreNames.IsEmpty()) {
    nsTArray<nsString> sortedNames(aObjectStoreNames);
    sortedNames.Sort();

    const uint32_t count = sortedNames.Length();
    MOZ_ASSERT(count == aObjectStoreNames.Length());

    
    for (uint32_t index = 0; index < count; index++) {
      MOZ_ASSERT(aObjectStoreNames[index] == sortedNames[index]);
    }

    
    for (uint32_t index = 0; index < count - 1; index++) {
      MOZ_ASSERT(sortedNames[index] != sortedNames[index + 1]);
    }
  }
#endif
}

IDBTransaction::~IDBTransaction()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(!mPendingRequestCount);
  MOZ_ASSERT(!mCreating);
  MOZ_ASSERT(mSentCommitOrAbort);
  MOZ_ASSERT_IF(mMode == VERSION_CHANGE &&
                  mBackgroundActor.mVersionChangeBackgroundActor,
                mFiredCompleteOrAbort);
  MOZ_ASSERT_IF(mMode != VERSION_CHANGE &&
                  mBackgroundActor.mNormalBackgroundActor,
                mFiredCompleteOrAbort);

  mDatabase->UnregisterTransaction(this);

  if (mMode == VERSION_CHANGE) {
    if (auto* actor = mBackgroundActor.mVersionChangeBackgroundActor) {
      actor->SendDeleteMeInternal();

      MOZ_ASSERT(!mBackgroundActor.mVersionChangeBackgroundActor,
                 "SendDeleteMeInternal should have cleared!");
    }
  } else if (auto* actor = mBackgroundActor.mNormalBackgroundActor) {
    actor->SendDeleteMeInternal();

    MOZ_ASSERT(!mBackgroundActor.mNormalBackgroundActor,
               "SendDeleteMeInternal should have cleared!");
  }
}


already_AddRefed<IDBTransaction>
IDBTransaction::CreateVersionChange(
                                IDBDatabase* aDatabase,
                                BackgroundVersionChangeTransactionChild* aActor,
                                IDBOpenDBRequest* aOpenRequest,
                                int64_t aNextObjectStoreId,
                                int64_t aNextIndexId)
{
  MOZ_ASSERT(aDatabase);
  aDatabase->AssertIsOnOwningThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aOpenRequest);
  MOZ_ASSERT(aNextObjectStoreId > 0);
  MOZ_ASSERT(aNextIndexId > 0);

  nsTArray<nsString> emptyObjectStoreNames;

  nsRefPtr<IDBTransaction> transaction =
    new IDBTransaction(aDatabase, emptyObjectStoreNames, VERSION_CHANGE);
  aOpenRequest->GetCallerLocation(transaction->mFilename,
                                  &transaction->mLineNo);

  transaction->SetScriptOwner(aDatabase->GetScriptOwner());
  transaction->mBackgroundActor.mVersionChangeBackgroundActor = aActor;
  transaction->mNextObjectStoreId = aNextObjectStoreId;
  transaction->mNextIndexId = aNextIndexId;

  
  MOZ_ASSERT(NS_IsMainThread(), "This won't work on non-main threads!");

  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (NS_WARN_IF(!appShell) ||
      NS_WARN_IF(NS_FAILED(appShell->RunBeforeNextEvent(transaction)))) {
    return nullptr;
  }

  transaction->mCreating = true;

  aDatabase->RegisterTransaction(transaction);

  return transaction.forget();
}


already_AddRefed<IDBTransaction>
IDBTransaction::Create(IDBDatabase* aDatabase,
                       const nsTArray<nsString>& aObjectStoreNames,
                       Mode aMode)
{
  MOZ_ASSERT(aDatabase);
  aDatabase->AssertIsOnOwningThread();
  MOZ_ASSERT(!aObjectStoreNames.IsEmpty());
  MOZ_ASSERT(aMode == READ_ONLY || aMode == READ_WRITE);

  nsRefPtr<IDBTransaction> transaction =
    new IDBTransaction(aDatabase, aObjectStoreNames, aMode);
  IDBRequest::CaptureCaller(transaction->mFilename, &transaction->mLineNo);

  transaction->SetScriptOwner(aDatabase->GetScriptOwner());

  
  MOZ_ASSERT(NS_IsMainThread(), "This won't work on non-main threads!");

  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (NS_WARN_IF(!appShell) ||
      NS_WARN_IF(NS_FAILED(appShell->RunBeforeNextEvent(transaction)))) {
    return nullptr;
  }

  transaction->mCreating = true;

  aDatabase->RegisterTransaction(transaction);

  return transaction.forget();
}


IDBTransaction*
IDBTransaction::GetCurrent()
{
  using namespace mozilla::ipc;

  MOZ_ASSERT(BackgroundChild::GetForCurrentThread());

  BackgroundChildImpl::ThreadLocal* threadLocal =
    BackgroundChildImpl::GetThreadLocalForCurrentThread();
  MOZ_ASSERT(threadLocal);

  ThreadLocal* idbThreadLocal = threadLocal->mIndexedDBThreadLocal;
  MOZ_ASSERT(idbThreadLocal);

  return idbThreadLocal->GetCurrentTransaction();
}

#ifdef DEBUG

void
IDBTransaction::AssertIsOnOwningThread() const
{
  MOZ_ASSERT(mDatabase);
  mDatabase->AssertIsOnOwningThread();
}

#endif 

void
IDBTransaction::SetBackgroundActor(BackgroundTransactionChild* aBackgroundActor)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aBackgroundActor);
  MOZ_ASSERT(!mBackgroundActor.mNormalBackgroundActor);
  MOZ_ASSERT(mMode != VERSION_CHANGE);

  mBackgroundActor.mNormalBackgroundActor = aBackgroundActor;
}

void
IDBTransaction::StartRequest(BackgroundRequestChild* aBackgroundActor,
                             const RequestParams& aParams)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aBackgroundActor);
  MOZ_ASSERT(aParams.type() != RequestParams::T__None);

  if (mMode == VERSION_CHANGE) {
    MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);

    mBackgroundActor.mVersionChangeBackgroundActor->
      SendPBackgroundIDBRequestConstructor(aBackgroundActor, aParams);
  } else {
    MOZ_ASSERT(mBackgroundActor.mNormalBackgroundActor);

    mBackgroundActor.mNormalBackgroundActor->
      SendPBackgroundIDBRequestConstructor(aBackgroundActor, aParams);
  }
}

void
IDBTransaction::OpenCursor(BackgroundCursorChild* aBackgroundActor,
                           const OpenCursorParams& aParams)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aBackgroundActor);
  MOZ_ASSERT(aParams.type() != OpenCursorParams::T__None);

  if (mMode == VERSION_CHANGE) {
    MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);

    mBackgroundActor.mVersionChangeBackgroundActor->
      SendPBackgroundIDBCursorConstructor(aBackgroundActor, aParams);
  } else {
    MOZ_ASSERT(mBackgroundActor.mNormalBackgroundActor);

    mBackgroundActor.mNormalBackgroundActor->
      SendPBackgroundIDBCursorConstructor(aBackgroundActor, aParams);
  }

  
  OnNewRequest();
}

void
IDBTransaction::RefreshSpec(bool aMayDelete)
{
  AssertIsOnOwningThread();

  for (uint32_t count = mObjectStores.Length(), index = 0;
       index < count;
       index++) {
    mObjectStores[index]->RefreshSpec(aMayDelete);
  }

  for (uint32_t count = mDeletedObjectStores.Length(), index = 0;
       index < count;
       index++) {
    mDeletedObjectStores[index]->RefreshSpec(false);
  }
}

void
IDBTransaction::OnNewRequest()
{
  AssertIsOnOwningThread();

  if (!mPendingRequestCount) {
    MOZ_ASSERT(INITIAL == mReadyState);
    mReadyState = LOADING;
  }

  ++mPendingRequestCount;
}

void
IDBTransaction::OnRequestFinished()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mPendingRequestCount);

  --mPendingRequestCount;

  if (!mPendingRequestCount && !mDatabase->IsInvalidated()) {
    mReadyState = COMMITTING;

    if (NS_SUCCEEDED(mAbortCode)) {
      SendCommit();
    } else {
      SendAbort(mAbortCode);
    }
  }
}

void
IDBTransaction::SendCommit()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_SUCCEEDED(mAbortCode));
  MOZ_ASSERT(IsFinished());
  MOZ_ASSERT(!mSentCommitOrAbort);
  MOZ_ASSERT(!mPendingRequestCount);

  
  
  const uint64_t requestSerialNumber = IDBRequest::NextSerialNumber();

  IDB_LOG_MARK("IndexedDB %s: Child  Transaction[%lld] Request[%llu]: "
                 "All requests complete, committing transaction",
               "IndexedDB %s: C T[%lld] R[%llu]: IDBTransaction commit",
               IDB_LOG_ID_STRING(),
               LoggingSerialNumber(),
               requestSerialNumber);

  if (mMode == VERSION_CHANGE) {
    MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);
    mBackgroundActor.mVersionChangeBackgroundActor->SendCommit();
  } else {
    MOZ_ASSERT(mBackgroundActor.mNormalBackgroundActor);
    mBackgroundActor.mNormalBackgroundActor->SendCommit();
  }

#ifdef DEBUG
  mSentCommitOrAbort = true;
#endif
}

void
IDBTransaction::SendAbort(nsresult aResultCode)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_FAILED(aResultCode));
  MOZ_ASSERT(IsFinished());
  MOZ_ASSERT(!mSentCommitOrAbort);

  
  
  const uint64_t requestSerialNumber = IDBRequest::NextSerialNumber();

  IDB_LOG_MARK("IndexedDB %s: Child  Transaction[%lld] Request[%llu]: "
                 "Aborting transaction with result 0x%x",
               "IndexedDB %s: C T[%lld] R[%llu]: IDBTransaction abort (0x%x)",
               IDB_LOG_ID_STRING(),
               LoggingSerialNumber(),
               requestSerialNumber,
               aResultCode);

  if (mMode == VERSION_CHANGE) {
    MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);
    mBackgroundActor.mVersionChangeBackgroundActor->SendAbort(aResultCode);
  } else {
    MOZ_ASSERT(mBackgroundActor.mNormalBackgroundActor);
    mBackgroundActor.mNormalBackgroundActor->SendAbort(aResultCode);
  }

#ifdef DEBUG
  mSentCommitOrAbort = true;
#endif
}

bool
IDBTransaction::IsOpen() const
{
  AssertIsOnOwningThread();

  
  if (mReadyState == IDBTransaction::INITIAL) {
    return true;
  }

  
  
  
  
  
  if (mReadyState == IDBTransaction::LOADING &&
      (mCreating || GetCurrent() == this)) {
    return true;
  }

  return false;
}

void
IDBTransaction::GetCallerLocation(nsAString& aFilename, uint32_t* aLineNo) const
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aLineNo);

  aFilename = mFilename;
  *aLineNo = mLineNo;
}

already_AddRefed<IDBObjectStore>
IDBTransaction::CreateObjectStore(const ObjectStoreSpec& aSpec)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aSpec.metadata().id());
  MOZ_ASSERT(VERSION_CHANGE == mMode);
  MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);
  MOZ_ASSERT(IsOpen());

#ifdef DEBUG
  {
    const nsString& name = aSpec.metadata().name();

    for (uint32_t count = mObjectStores.Length(), index = 0;
         index < count;
         index++) {
      MOZ_ASSERT(mObjectStores[index]->Name() != name);
    }
  }
#endif

  MOZ_ALWAYS_TRUE(mBackgroundActor.mVersionChangeBackgroundActor->
                    SendCreateObjectStore(aSpec.metadata()));

  nsRefPtr<IDBObjectStore> objectStore = IDBObjectStore::Create(this, aSpec);
  MOZ_ASSERT(objectStore);

  mObjectStores.AppendElement(objectStore);

  return objectStore.forget();
}

void
IDBTransaction::DeleteObjectStore(int64_t aObjectStoreId)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aObjectStoreId);
  MOZ_ASSERT(VERSION_CHANGE == mMode);
  MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);
  MOZ_ASSERT(IsOpen());

  MOZ_ALWAYS_TRUE(mBackgroundActor.mVersionChangeBackgroundActor->
                    SendDeleteObjectStore(aObjectStoreId));

  for (uint32_t count = mObjectStores.Length(), index = 0;
       index < count;
       index++) {
    nsRefPtr<IDBObjectStore>& objectStore = mObjectStores[index];

    if (objectStore->Id() == aObjectStoreId) {
      objectStore->NoteDeletion();

      nsRefPtr<IDBObjectStore>* deletedObjectStore =
        mDeletedObjectStores.AppendElement();
      deletedObjectStore->swap(mObjectStores[index]);

      mObjectStores.RemoveElementAt(index);
      break;
    }
  }
}

void
IDBTransaction::CreateIndex(IDBObjectStore* aObjectStore,
                            const IndexMetadata& aMetadata)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aObjectStore);
  MOZ_ASSERT(aMetadata.id());
  MOZ_ASSERT(VERSION_CHANGE == mMode);
  MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);
  MOZ_ASSERT(IsOpen());

  MOZ_ALWAYS_TRUE(mBackgroundActor.mVersionChangeBackgroundActor->
                    SendCreateIndex(aObjectStore->Id(), aMetadata));
}

void
IDBTransaction::DeleteIndex(IDBObjectStore* aObjectStore,
                            int64_t aIndexId)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aObjectStore);
  MOZ_ASSERT(aIndexId);
  MOZ_ASSERT(VERSION_CHANGE == mMode);
  MOZ_ASSERT(mBackgroundActor.mVersionChangeBackgroundActor);
  MOZ_ASSERT(IsOpen());

  MOZ_ALWAYS_TRUE(mBackgroundActor.mVersionChangeBackgroundActor->
                    SendDeleteIndex(aObjectStore->Id(), aIndexId));
}

void
IDBTransaction::AbortInternal(nsresult aAbortCode,
                              already_AddRefed<DOMError> aError)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_FAILED(aAbortCode));

  nsRefPtr<DOMError> error = aError;

  if (IsFinished()) {
    
    return;
  }

  const bool isVersionChange = mMode == VERSION_CHANGE;
  const bool isInvalidated = mDatabase->IsInvalidated();
  bool needToSendAbort = mReadyState == INITIAL && !isInvalidated;

  if (isInvalidated) {
#ifdef DEBUG
    mSentCommitOrAbort = true;
#endif
    
    
    IDBRequest::NextSerialNumber();
  }

  mAbortCode = aAbortCode;
  mReadyState = DONE;
  mError = error.forget();

  if (isVersionChange) {
    
    
    
    if (!isInvalidated) {
      mDatabase->RevertToPreviousState();
    }

    const nsTArray<ObjectStoreSpec>& specArray =
      mDatabase->Spec()->objectStores();

    if (specArray.IsEmpty()) {
      mObjectStores.Clear();
      mDeletedObjectStores.Clear();
    } else {
      nsTHashtable<nsUint64HashKey> validIds(specArray.Length());

      for (uint32_t specCount = specArray.Length(), specIndex = 0;
           specIndex < specCount;
           specIndex++) {
        const int64_t objectStoreId = specArray[specIndex].metadata().id();
        MOZ_ASSERT(objectStoreId);

        validIds.PutEntry(uint64_t(objectStoreId));
      }

      for (uint32_t objCount = mObjectStores.Length(), objIndex = 0;
            objIndex < objCount;
            ) {
        const int64_t objectStoreId = mObjectStores[objIndex]->Id();
        MOZ_ASSERT(objectStoreId);

        if (validIds.Contains(uint64_t(objectStoreId))) {
          objIndex++;
        } else {
          mObjectStores.RemoveElementAt(objIndex);
          objCount--;
        }
      }

      if (!mDeletedObjectStores.IsEmpty()) {
        for (uint32_t objCount = mDeletedObjectStores.Length(), objIndex = 0;
              objIndex < objCount;
              objIndex++) {
          const int64_t objectStoreId = mDeletedObjectStores[objIndex]->Id();
          MOZ_ASSERT(objectStoreId);

          if (validIds.Contains(uint64_t(objectStoreId))) {
            nsRefPtr<IDBObjectStore>* objectStore =
              mObjectStores.AppendElement();
            objectStore->swap(mDeletedObjectStores[objIndex]);
          }
        }
        mDeletedObjectStores.Clear();
      }
    }
  }

  
  
  if (needToSendAbort) {
    SendAbort(aAbortCode);
  }

  if (isVersionChange) {
    mDatabase->Close();
  }
}

void
IDBTransaction::Abort(IDBRequest* aRequest)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aRequest);

  ErrorResult rv;
  nsRefPtr<DOMError> error = aRequest->GetError(rv);

  AbortInternal(aRequest->GetErrorCode(), error.forget());
}

void
IDBTransaction::Abort(nsresult aErrorCode)
{
  AssertIsOnOwningThread();

  nsRefPtr<DOMError> error = new DOMError(GetOwner(), aErrorCode);
  AbortInternal(aErrorCode, error.forget());
}

void
IDBTransaction::Abort(ErrorResult& aRv)
{
  AssertIsOnOwningThread();

  if (IsFinished()) {
    aRv = NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
    return;
  }

  AbortInternal(NS_ERROR_DOM_INDEXEDDB_ABORT_ERR, nullptr);

  MOZ_ASSERT(!mAbortedByScript);
  mAbortedByScript = true;
}

void
IDBTransaction::FireCompleteOrAbortEvents(nsresult aResult)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(!mFiredCompleteOrAbort);

  mReadyState = DONE;

#ifdef DEBUG
  mFiredCompleteOrAbort = true;
#endif

  nsCOMPtr<nsIDOMEvent> event;
  if (NS_SUCCEEDED(aResult)) {
    event = CreateGenericEvent(this,
                               nsDependentString(kCompleteEventType),
                               eDoesNotBubble,
                               eNotCancelable);
  } else {
    if (!mError && !mAbortedByScript) {
      mError = new DOMError(GetOwner(), aResult);
    }

    event = CreateGenericEvent(this,
                               nsDependentString(kAbortEventType),
                               eDoesBubble,
                               eNotCancelable);
  }

  if (NS_WARN_IF(!event)) {
    return;
  }

  if (NS_SUCCEEDED(mAbortCode)) {
    IDB_LOG_MARK("IndexedDB %s: Child  Transaction[%lld]: "
                   "Firing 'complete' event",
                 "IndexedDB %s: C T[%lld]: IDBTransaction 'complete' event",
                 IDB_LOG_ID_STRING(),
                 mLoggingSerialNumber);
  } else {
    IDB_LOG_MARK("IndexedDB %s: Child  Transaction[%lld]: "
                   "Firing 'abort' event with error 0x%x",
                 "IndexedDB %s: C T[%lld]: IDBTransaction 'abort' event (0x%x)",
                 IDB_LOG_ID_STRING(),
                 mLoggingSerialNumber,
                 mAbortCode);
  }

  bool dummy;
  if (NS_FAILED(DispatchEvent(event, &dummy))) {
    NS_WARNING("DispatchEvent failed!");
  }

  mDatabase->DelayedMaybeExpireFileActors();
}

int64_t
IDBTransaction::NextObjectStoreId()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(VERSION_CHANGE == mMode);

  return mNextObjectStoreId++;
}

int64_t
IDBTransaction::NextIndexId()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(VERSION_CHANGE == mMode);

  return mNextIndexId++;
}

nsPIDOMWindow*
IDBTransaction::GetParentObject() const
{
  AssertIsOnOwningThread();

  return mDatabase->GetParentObject();
}

IDBTransactionMode
IDBTransaction::GetMode(ErrorResult& aRv) const
{
  AssertIsOnOwningThread();

  switch (mMode) {
    case READ_ONLY:
      return IDBTransactionMode::Readonly;

    case READ_WRITE:
      return IDBTransactionMode::Readwrite;

    case VERSION_CHANGE:
      return IDBTransactionMode::Versionchange;

    case MODE_INVALID:
    default:
      MOZ_CRASH("Bad mode!");
  }
}

DOMError*
IDBTransaction::GetError() const
{
  AssertIsOnOwningThread();

  return mError;
}

already_AddRefed<DOMStringList>
IDBTransaction::ObjectStoreNames()
{
  AssertIsOnOwningThread();

  if (mMode == IDBTransaction::VERSION_CHANGE) {
    return mDatabase->ObjectStoreNames();
  }

  nsRefPtr<DOMStringList> list = new DOMStringList();
  list->StringArray() = mObjectStoreNames;
  return list.forget();
}

already_AddRefed<IDBObjectStore>
IDBTransaction::ObjectStore(const nsAString& aName, ErrorResult& aRv)
{
  AssertIsOnOwningThread();

  if (IsFinished()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }

  const ObjectStoreSpec* spec = nullptr;

  if (IDBTransaction::VERSION_CHANGE == mMode ||
      mObjectStoreNames.Contains(aName)) {
    const nsTArray<ObjectStoreSpec>& objectStores =
      mDatabase->Spec()->objectStores();

    for (uint32_t count = objectStores.Length(), index = 0;
         index < count;
         index++) {
      const ObjectStoreSpec& objectStore = objectStores[index];
      if (objectStore.metadata().name() == aName) {
        spec = &objectStore;
        break;
      }
    }
  }

  if (!spec) {
    aRv.Throw(NS_ERROR_DOM_INDEXEDDB_NOT_FOUND_ERR);
    return nullptr;
  }

  const int64_t desiredId = spec->metadata().id();

  nsRefPtr<IDBObjectStore> objectStore;

  for (uint32_t count = mObjectStores.Length(), index = 0;
       index < count;
       index++) {
    nsRefPtr<IDBObjectStore>& existingObjectStore = mObjectStores[index];

    if (existingObjectStore->Id() == desiredId) {
      objectStore = existingObjectStore;
      break;
    }
  }

  if (!objectStore) {
    objectStore = IDBObjectStore::Create(this, *spec);
    MOZ_ASSERT(objectStore);

    mObjectStores.AppendElement(objectStore);
  }

  return objectStore.forget();
}

NS_IMPL_ADDREF_INHERITED(IDBTransaction, IDBWrapperCache)
NS_IMPL_RELEASE_INHERITED(IDBTransaction, IDBWrapperCache)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBTransaction)
  NS_INTERFACE_MAP_ENTRY(nsIRunnable)
NS_INTERFACE_MAP_END_INHERITING(IDBWrapperCache)

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBTransaction)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBTransaction,
                                                  IDBWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDatabase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mError)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mObjectStores)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDeletedObjectStores)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBTransaction, IDBWrapperCache)
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mError)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mObjectStores)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDeletedObjectStores)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

JSObject*
IDBTransaction::WrapObject(JSContext* aCx)
{
  AssertIsOnOwningThread();

  return IDBTransactionBinding::Wrap(aCx, this);
}

nsresult
IDBTransaction::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  AssertIsOnOwningThread();

  aVisitor.mCanHandle = true;
  aVisitor.mParentTarget = mDatabase;
  return NS_OK;
}

NS_IMETHODIMP
IDBTransaction::Run()
{
  AssertIsOnOwningThread();

  
  mCreating = false;

  
  if (mReadyState == IDBTransaction::INITIAL) {
    mReadyState = DONE;

    SendCommit();
  }

  return NS_OK;
}

} 
} 
} 
