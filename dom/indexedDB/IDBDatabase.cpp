






































#include "IDBDatabase.h"

#include "jscntxt.h"
#include "mozilla/Mutex.h"
#include "mozilla/storage.h"
#include "nsDOMClassInfo.h"
#include "nsEventDispatcher.h"
#include "nsJSUtils.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

#include "AsyncConnectionHelper.h"
#include "CheckQuotaHelper.h"
#include "DatabaseInfo.h"
#include "IDBEvents.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "IDBTransaction.h"
#include "IDBFactory.h"
#include "IndexedDatabaseManager.h"
#include "LazyIdleThread.h"
#include "TransactionThreadPool.h"

USING_INDEXEDDB_NAMESPACE

namespace {

const PRUint32 kDefaultDatabaseTimeoutSeconds = 30;

PRUint32 gDatabaseInstanceCount = 0;
mozilla::Mutex* gPromptHelpersMutex = nsnull;


nsTArray<nsRefPtr<CheckQuotaHelper> >* gPromptHelpers = nsnull;

class SetVersionHelper : public AsyncConnectionHelper
{
public:
  SetVersionHelper(IDBTransaction* aTransaction,
                   IDBRequest* aRequest,
                   const nsAString& aVersion)
  : AsyncConnectionHelper(aTransaction, aRequest), mVersion(aVersion)
  { }

  nsresult DoDatabaseWork(mozIStorageConnection* aConnection);
  nsresult GetSuccessResult(JSContext* aCx,
                            jsval* aVal);

private:
  
  nsString mVersion;
};

class CreateObjectStoreHelper : public AsyncConnectionHelper
{
public:
  CreateObjectStoreHelper(IDBTransaction* aTransaction,
                          IDBObjectStore* aObjectStore)
  : AsyncConnectionHelper(aTransaction, nsnull), mObjectStore(aObjectStore)
  { }

  nsresult DoDatabaseWork(mozIStorageConnection* aConnection);

  nsresult OnSuccess()
  {
    return NS_OK;
  }

  void OnError()
  {
    NS_ASSERTION(mTransaction->IsAborted(), "How else can this fail?!");
  }

  void ReleaseMainThreadObjects()
  {
    mObjectStore = nsnull;
    AsyncConnectionHelper::ReleaseMainThreadObjects();
  }

private:
  nsRefPtr<IDBObjectStore> mObjectStore;
};

class DeleteObjectStoreHelper : public AsyncConnectionHelper
{
public:
  DeleteObjectStoreHelper(IDBTransaction* aTransaction,
                          PRInt64 aObjectStoreId)
  : AsyncConnectionHelper(aTransaction, nsnull), mObjectStoreId(aObjectStoreId)
  { }

  nsresult DoDatabaseWork(mozIStorageConnection* aConnection);

  nsresult OnSuccess()
  {
    return NS_OK;
  }

  void OnError()
  {
    NS_ASSERTION(mTransaction->IsAborted(), "How else can this fail?!");
  }

private:
  
  PRInt64 mObjectStoreId;
};

NS_STACK_CLASS
class AutoFree
{
public:
  AutoFree(void* aPtr) : mPtr(aPtr) { }
  ~AutoFree() { NS_Free(mPtr); }
private:
  void* mPtr;
};

NS_STACK_CLASS
class AutoRemoveObjectStore
{
public:
  AutoRemoveObjectStore(PRUint32 aId, const nsAString& aName)
  : mId(aId), mName(aName)
  { }

  ~AutoRemoveObjectStore()
  {
    if (mId) {
      ObjectStoreInfo::Remove(mId, mName);
    }
  }

  void forget()
  {
    mId = 0;
  }

private:
  PRUint32 mId;
  nsString mName;
};

inline
nsresult
ConvertVariantToStringArray(nsIVariant* aVariant,
                            nsTArray<nsString>& aStringArray)
{
#ifdef DEBUG
  PRUint16 type;
  NS_ASSERTION(NS_SUCCEEDED(aVariant->GetDataType(&type)) &&
               type == nsIDataType::VTYPE_ARRAY, "Bad arg!");
#endif

  PRUint16 valueType;
  nsIID iid;
  PRUint32 valueCount;
  void* rawArray;

  nsresult rv = aVariant->GetAsArray(&valueType, &iid, &valueCount, &rawArray);
  NS_ENSURE_SUCCESS(rv, rv);

  AutoFree af(rawArray);

  
  if (valueType != nsIDataType::VTYPE_WCHAR_STR) {
    switch (valueType) {
      case nsIDataType::VTYPE_ID:
      case nsIDataType::VTYPE_CHAR_STR: {
        char** charArray = reinterpret_cast<char**>(rawArray);
        for (PRUint32 index = 0; index < valueCount; index++) {
          if (charArray[index]) {
            NS_Free(charArray[index]);
          }
        }
      } break;

      case nsIDataType::VTYPE_INTERFACE:
      case nsIDataType::VTYPE_INTERFACE_IS: {
        nsISupports** supportsArray = reinterpret_cast<nsISupports**>(rawArray);
        for (PRUint32 index = 0; index < valueCount; index++) {
          NS_IF_RELEASE(supportsArray[index]);
        }
      } break;

      default: {
        
      }
    }

    return NS_ERROR_ILLEGAL_VALUE;
  }

  PRUnichar** strings = reinterpret_cast<PRUnichar**>(rawArray);

  for (PRUint32 index = 0; index < valueCount; index++) {
    nsString* newString = aStringArray.AppendElement();

    if (!newString) {
      NS_ERROR("Out of memory?");
      for (; index < valueCount; index++) {
        NS_Free(strings[index]);
      }
      return NS_ERROR_OUT_OF_MEMORY;
    }

    newString->Adopt(strings[index], -1);
  }

  return NS_OK;
}

} 


already_AddRefed<IDBDatabase>
IDBDatabase::Create(nsIScriptContext* aScriptContext,
                    nsPIDOMWindow* aOwner,
                    DatabaseInfo* aDatabaseInfo,
                    const nsACString& aASCIIOrigin)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabaseInfo, "Null pointer!");
  NS_ASSERTION(!aASCIIOrigin.IsEmpty(), "Empty origin!");

  nsRefPtr<IDBDatabase> db(new IDBDatabase());

  db->mScriptContext = aScriptContext;
  db->mOwner = aOwner;

  db->mDatabaseId = aDatabaseInfo->id;
  db->mName = aDatabaseInfo->name;
  db->mFilePath = aDatabaseInfo->filePath;
  db->mASCIIOrigin = aASCIIOrigin;

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never be null!");

  if (!mgr->RegisterDatabase(db)) {
    
    return nsnull;
  }

  return db.forget();
}

IDBDatabase::IDBDatabase()
: mDatabaseId(0),
  mInvalidated(0),
  mRegistered(false),
  mClosed(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!gDatabaseInstanceCount++) {
    NS_ASSERTION(!gPromptHelpersMutex, "Should be null!");
    gPromptHelpersMutex = new mozilla::Mutex("IDBDatabase gPromptHelpersMutex");
  }
}

IDBDatabase::~IDBDatabase()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mRegistered) {
    CloseInternal();

    IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
    if (mgr) {
      mgr->UnregisterDatabase(this);
    }
  }

  if (mDatabaseId && !mInvalidated) {
    DatabaseInfo* info;
    if (!DatabaseInfo::Get(mDatabaseId, &info)) {
      NS_ERROR("This should never fail!");
    }

    NS_ASSERTION(info->referenceCount, "Bad reference count!");
    if (--info->referenceCount == 0) {
      DatabaseInfo::Remove(mDatabaseId);
    }
  }

  if (mListenerManager) {
    mListenerManager->Disconnect();
  }

  if (!--gDatabaseInstanceCount) {
    NS_ASSERTION(gPromptHelpersMutex, "Should not be null!");

    delete gPromptHelpers;
    gPromptHelpers = nsnull;

    delete gPromptHelpersMutex;
    gPromptHelpersMutex = nsnull;
  }
}

bool
IDBDatabase::IsQuotaDisabled()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(gPromptHelpersMutex, "This should never be null!");

  MutexAutoLock lock(*gPromptHelpersMutex);

  if (!gPromptHelpers) {
    gPromptHelpers = new nsAutoTArray<nsRefPtr<CheckQuotaHelper>, 10>();
  }

  CheckQuotaHelper* foundHelper = nsnull;

  PRUint32 count = gPromptHelpers->Length();
  for (PRUint32 index = 0; index < count; index++) {
    nsRefPtr<CheckQuotaHelper>& helper = gPromptHelpers->ElementAt(index);
    if (helper->WindowSerial() == Owner()->GetSerial()) {
      foundHelper = helper;
      break;
    }
  }

  if (!foundHelper) {
    nsRefPtr<CheckQuotaHelper>* newHelper = gPromptHelpers->AppendElement();
    if (!newHelper) {
      NS_WARNING("Out of memory!");
      return false;
    }
    *newHelper = new CheckQuotaHelper(this, *gPromptHelpersMutex);
    foundHelper = *newHelper;

    {
      
      MutexAutoUnlock unlock(*gPromptHelpersMutex);

      nsresult rv = NS_DispatchToMainThread(foundHelper, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, false);
    }
  }

  return foundHelper->PromptAndReturnQuotaIsDisabled();
}

void
IDBDatabase::Invalidate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(gPromptHelpersMutex, "This should never be null!");

  
  Close();

  
  {
    MutexAutoLock lock(*gPromptHelpersMutex);

    if (gPromptHelpers) {
      PRUint32 count = gPromptHelpers->Length();
      for (PRUint32 index = 0; index < count; index++) {
        nsRefPtr<CheckQuotaHelper>& helper = gPromptHelpers->ElementAt(index);
        if (helper->WindowSerial() == Owner()->GetSerial()) {
          helper->Cancel();
          break;
        }
      }
    }
  }

  if (!PR_ATOMIC_SET(&mInvalidated, 1)) {
    DatabaseInfo* info;
    if (!DatabaseInfo::Get(mDatabaseId, &info)) {
      NS_ERROR("This should never fail!");
    }

    NS_ASSERTION(info->referenceCount, "Bad reference count!");
    if (--info->referenceCount == 0) {
      DatabaseInfo::Remove(mDatabaseId);
    }
  }
}

bool
IDBDatabase::IsInvalidated()
{
  return !!mInvalidated;
}

void
IDBDatabase::CloseInternal()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mClosed) {
    IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
    if (mgr) {
      mgr->OnDatabaseClosed(this);
    }
    mClosed = true;
  }
}

bool
IDBDatabase::IsClosed()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return mClosed;
}

void
IDBDatabase::OnUnlink()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mOwner, "Should have been cleared already!");

  
  
  Close();

  
  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  if (mgr) {
    mgr->UnregisterDatabase(this);

    
    mRegistered = false;
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBDatabase)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBDatabase,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnVersionChangeListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBDatabase,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnVersionChangeListener)

  
  tmp->OnUnlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBDatabase)
  NS_INTERFACE_MAP_ENTRY(nsIIDBDatabase)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBDatabase)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(IDBDatabase, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(IDBDatabase, nsDOMEventTargetHelper)

DOMCI_DATA(IDBDatabase, IDBDatabase)

NS_IMETHODIMP
IDBDatabase::GetName(nsAString& aName)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::GetVersion(nsAString& aVersion)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  DatabaseInfo* info;
  if (!DatabaseInfo::Get(mDatabaseId, &info)) {
    NS_ERROR("This should never fail!");
  }
  aVersion.Assign(info->version);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::GetObjectStoreNames(nsIDOMDOMStringList** aObjectStores)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  DatabaseInfo* info;
  if (!DatabaseInfo::Get(mDatabaseId, &info)) {
    NS_ERROR("This should never fail!");
  }

  nsAutoTArray<nsString, 10> objectStoreNames;
  if (!info->GetObjectStoreNames(objectStoreNames)) {
    NS_WARNING("Couldn't get names!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  nsRefPtr<nsDOMStringList> list(new nsDOMStringList());
  PRUint32 count = objectStoreNames.Length();
  for (PRUint32 index = 0; index < count; index++) {
    NS_ENSURE_TRUE(list->Add(objectStoreNames[index]),
                   NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
  }

  list.forget(aObjectStores);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::CreateObjectStore(const nsAString& aName,
                               const jsval& aOptions,
                               JSContext* aCx,
                               nsIIDBObjectStore** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (aName.IsEmpty()) {
    
    return NS_ERROR_DOM_INDEXEDDB_NON_TRANSIENT_ERR;
  }

  IDBTransaction* transaction = AsyncConnectionHelper::GetCurrentTransaction();

  if (!transaction ||
      transaction->Mode() != nsIIDBTransaction::VERSION_CHANGE) {
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  DatabaseInfo* databaseInfo;
  if (!DatabaseInfo::Get(mDatabaseId, &databaseInfo)) {
    NS_ERROR("This should never fail!");
  }

  if (databaseInfo->ContainsStoreName(aName)) {
    return NS_ERROR_DOM_INDEXEDDB_CONSTRAINT_ERR;
  }

  nsString keyPath;
  bool autoIncrement = false;

  if (!JSVAL_IS_VOID(aOptions) && !JSVAL_IS_NULL(aOptions)) {
    if (JSVAL_IS_PRIMITIVE(aOptions)) {
      
      return NS_ERROR_DOM_INDEXEDDB_NON_TRANSIENT_ERR;
    }

    NS_ASSERTION(JSVAL_IS_OBJECT(aOptions), "Huh?!");
    JSObject* options = JSVAL_TO_OBJECT(aOptions);

    js::AutoIdArray ids(aCx, JS_Enumerate(aCx, options));
    if (!ids) {
      return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    }

    for (size_t index = 0; index < ids.length(); index++) {
      jsid id = ids[index];

      if (id != nsDOMClassInfo::sKeyPath_id &&
          id != nsDOMClassInfo::sAutoIncrement_id) {
        
        return NS_ERROR_DOM_INDEXEDDB_NON_TRANSIENT_ERR;
      }

      jsval val;
      if (!JS_GetPropertyById(aCx, options, id, &val)) {
        NS_WARNING("JS_GetPropertyById failed!");
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }

      if (id == nsDOMClassInfo::sKeyPath_id) {
        JSString* str = JS_ValueToString(aCx, val);
        if (!str) {
          NS_WARNING("JS_ValueToString failed!");
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }
        nsDependentJSString dependentKeyPath;
        if (!dependentKeyPath.init(aCx, str)) {
          NS_WARNING("Initializing keyPath failed!");
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }
        keyPath = dependentKeyPath;
      }
      else if (id == nsDOMClassInfo::sAutoIncrement_id) {
        JSBool boolVal;
        if (!JS_ValueToBoolean(aCx, val, &boolVal)) {
          NS_WARNING("JS_ValueToBoolean failed!");
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }
        autoIncrement = !!boolVal;
      }
      else {
        NS_NOTREACHED("Shouldn't be able to get here!");
      }
    }
  }

  nsAutoPtr<ObjectStoreInfo> newInfo(new ObjectStoreInfo());

  newInfo->name = aName;
  newInfo->id = databaseInfo->nextObjectStoreId++;
  newInfo->keyPath = keyPath;
  newInfo->autoIncrement = autoIncrement;
  newInfo->databaseId = mDatabaseId;

  if (!ObjectStoreInfo::Put(newInfo)) {
    NS_WARNING("Put failed!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }
  ObjectStoreInfo* objectStoreInfo = newInfo.forget();

  
  AutoRemoveObjectStore autoRemove(mDatabaseId, aName);

  nsRefPtr<IDBObjectStore> objectStore =
    transaction->GetOrCreateObjectStore(aName, objectStoreInfo);
  NS_ENSURE_TRUE(objectStore, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsRefPtr<CreateObjectStoreHelper> helper =
    new CreateObjectStoreHelper(transaction, objectStore);

  nsresult rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  autoRemove.forget();

  objectStore.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::DeleteObjectStore(const nsAString& aName)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  IDBTransaction* transaction = AsyncConnectionHelper::GetCurrentTransaction();

  if (!transaction ||
      transaction->Mode() != nsIIDBTransaction::VERSION_CHANGE) {
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  ObjectStoreInfo* objectStoreInfo;
  if (!ObjectStoreInfo::Get(mDatabaseId, aName, &objectStoreInfo)) {
    return NS_ERROR_DOM_INDEXEDDB_NOT_FOUND_ERR;
  }

  nsRefPtr<DeleteObjectStoreHelper> helper =
    new DeleteObjectStoreHelper(transaction, objectStoreInfo->id);
  nsresult rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  ObjectStoreInfo::Remove(mDatabaseId, aName);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::SetVersion(const nsAString& aVersion,
                        JSContext* aCx,
                        nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mClosed) {
    
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  DatabaseInfo* info;
  if (!DatabaseInfo::Get(mDatabaseId, &info)) {
    NS_ERROR("This should never fail!");
  }

  
  nsTArray<nsString> storesToOpen;
  nsRefPtr<IDBTransaction> transaction =
    IDBTransaction::Create(this, storesToOpen, IDBTransaction::VERSION_CHANGE,
                           kDefaultDatabaseTimeoutSeconds, true);
  NS_ENSURE_TRUE(transaction, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsRefPtr<IDBVersionChangeRequest> request =
    IDBVersionChangeRequest::Create(static_cast<nsIDOMEventTarget*>(this),
                                    ScriptContext(), Owner(), transaction);
  NS_ENSURE_TRUE(request, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  nsRefPtr<SetVersionHelper> helper =
    new SetVersionHelper(transaction, request, aVersion);

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never be null!");

  nsresult rv = mgr->SetDatabaseVersion(this, request, aVersion, helper);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::Transaction(nsIVariant* aStoreNames,
                         PRUint16 aMode,
                         PRUint32 aTimeout,
                         JSContext* aCx,
                         PRUint8 aOptionalArgCount,
                         nsIIDBTransaction** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (IndexedDatabaseManager::IsShuttingDown()) {
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }

  if (mClosed) {
    return NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR;
  }

  if (aOptionalArgCount) {
    if (aMode != nsIIDBTransaction::READ_WRITE &&
        aMode != nsIIDBTransaction::READ_ONLY) {
      return NS_ERROR_DOM_INDEXEDDB_NON_TRANSIENT_ERR;
    }
  }
  else {
    aMode = nsIIDBTransaction::READ_ONLY;
  }

  if (aOptionalArgCount <= 1) {
    aTimeout = kDefaultDatabaseTimeoutSeconds;
  }

  PRUint16 type;
  nsresult rv = aStoreNames->GetDataType(&type);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  DatabaseInfo* info;
  if (!DatabaseInfo::Get(mDatabaseId, &info)) {
    NS_ERROR("This should never fail!");
  }

  nsTArray<nsString> storesToOpen;

  switch (type) {
    case nsIDataType::VTYPE_VOID:
    case nsIDataType::VTYPE_EMPTY:
    case nsIDataType::VTYPE_EMPTY_ARRAY: {
      
      if (!info->GetObjectStoreNames(storesToOpen)) {
        NS_WARNING("Out of memory?");
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    } break;

    case nsIDataType::VTYPE_WSTRING_SIZE_IS: {
      
      nsString name;
      rv = aStoreNames->GetAsAString(name);
      NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      if (!info->ContainsStoreName(name)) {
        return NS_ERROR_DOM_INDEXEDDB_NOT_FOUND_ERR;
      }

      if (!storesToOpen.AppendElement(name)) {
        NS_WARNING("Out of memory?");
        return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
      }
    } break;

    case nsIDataType::VTYPE_ARRAY: {
      nsTArray<nsString> names;
      rv = ConvertVariantToStringArray(aStoreNames, names);
      NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      PRUint32 nameCount = names.Length();
      for (PRUint32 nameIndex = 0; nameIndex < nameCount; nameIndex++) {
        nsString& name = names[nameIndex];

        if (!info->ContainsStoreName(name)) {
          return NS_ERROR_DOM_INDEXEDDB_NOT_FOUND_ERR;
        }

        if (!storesToOpen.AppendElement(name)) {
          NS_WARNING("Out of memory?");
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }
      }
      NS_ASSERTION(nameCount == storesToOpen.Length(), "Should have bailed!");
    } break;

    case nsIDataType::VTYPE_INTERFACE:
    case nsIDataType::VTYPE_INTERFACE_IS: {
      nsCOMPtr<nsISupports> supports;
      nsID *iid;
      rv = aStoreNames->GetAsInterface(&iid, getter_AddRefs(supports));
      NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      NS_Free(iid);

      nsCOMPtr<nsIDOMDOMStringList> stringList(do_QueryInterface(supports));
      if (!stringList) {
        
        return NS_ERROR_DOM_INDEXEDDB_NON_TRANSIENT_ERR;
      }

      PRUint32 stringCount;
      rv = stringList->GetLength(&stringCount);
      NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

      for (PRUint32 stringIndex = 0; stringIndex < stringCount; stringIndex++) {
        nsString name;
        rv = stringList->Item(stringIndex, name);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

        if (!info->ContainsStoreName(name)) {
          return NS_ERROR_DOM_INDEXEDDB_NOT_FOUND_ERR;
        }

        if (!storesToOpen.AppendElement(name)) {
          NS_WARNING("Out of memory?");
          return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
        }
      }
    } break;

    default:
      return NS_ERROR_DOM_INDEXEDDB_NON_TRANSIENT_ERR;
  }

  nsRefPtr<IDBTransaction> transaction =
    IDBTransaction::Create(this, storesToOpen, aMode,
                           kDefaultDatabaseTimeoutSeconds);
  NS_ENSURE_TRUE(transaction, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  transaction.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  CloseInternal();

  NS_ASSERTION(mClosed, "Should have set the closed flag!");
  return NS_OK;
}

NS_IMETHODIMP
IDBDatabase::SetOnerror(nsIDOMEventListener* aErrorListener)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ERROR_EVT_STR),
                                mOnErrorListener, aErrorListener);
}

NS_IMETHODIMP
IDBDatabase::GetOnerror(nsIDOMEventListener** aErrorListener)
{
  return GetInnerEventListener(mOnErrorListener, aErrorListener);
}

NS_IMETHODIMP
IDBDatabase::SetOnversionchange(nsIDOMEventListener* aVersionChangeListener)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(VERSIONCHANGE_EVT_STR),
                                mOnVersionChangeListener,
                                aVersionChangeListener);
}

NS_IMETHODIMP
IDBDatabase::GetOnversionchange(nsIDOMEventListener** aVersionChangeListener)
{
  return GetInnerEventListener(mOnVersionChangeListener,
                               aVersionChangeListener);
}

nsresult
IDBDatabase::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  NS_ENSURE_TRUE(aVisitor.mDOMEvent, NS_ERROR_UNEXPECTED);

  if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault) {
    nsString type;
    nsresult rv = aVisitor.mDOMEvent->GetType(type);
    NS_ENSURE_SUCCESS(rv, rv);

    if (type.EqualsLiteral(ERROR_EVT_STR)) {
      nsRefPtr<nsDOMEvent> duplicateEvent = CreateGenericEvent(type);
      NS_ENSURE_STATE(duplicateEvent);

      nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mOwner));
      NS_ASSERTION(target, "How can this happen?!");

      PRBool dummy;
      rv = target->DispatchEvent(duplicateEvent, &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
SetVersionHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_PRECONDITION(aConnection, "Passing a null connection!");

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE database "
    "SET version = :version"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("version"), mVersion);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  if (NS_FAILED(stmt->Execute())) {
    return NS_ERROR_DOM_INDEXEDDB_CONSTRAINT_ERR;
  }

  return NS_OK;
}

nsresult
SetVersionHelper::GetSuccessResult(JSContext* aCx,
                                   jsval* aVal)
{
  DatabaseInfo* info;
  if (!DatabaseInfo::Get(mDatabase->Id(), &info)) {
    NS_ERROR("This should never fail!");
    return NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
  }
  info->version = mVersion;

  nsresult rv = WrapNative(aCx, NS_ISUPPORTS_CAST(nsIDOMEventTarget*,
                                                  mTransaction),
                           aVal);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
CreateObjectStoreHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  nsCOMPtr<mozIStorageStatement> stmt =
    mTransaction->GetCachedStatement(NS_LITERAL_CSTRING(
    "INSERT INTO object_store (id, name, key_path, auto_increment) "
    "VALUES (:id, :name, :key_path, :auto_increment)"
  ));
  NS_ENSURE_TRUE(stmt, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"),
                                       mObjectStore->Id());
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), mObjectStore->Name());
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("key_path"),
                              mObjectStore->KeyPath());
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("auto_increment"),
                             mObjectStore->IsAutoIncrement() ? 1 : 0);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  return NS_OK;
}

nsresult
DeleteObjectStoreHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  nsCOMPtr<mozIStorageStatement> stmt =
    mTransaction->GetCachedStatement(NS_LITERAL_CSTRING(
    "DELETE FROM object_store "
    "WHERE id = :id "
  ));
  NS_ENSURE_TRUE(stmt, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("id"), mObjectStoreId);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  return NS_OK;
}
