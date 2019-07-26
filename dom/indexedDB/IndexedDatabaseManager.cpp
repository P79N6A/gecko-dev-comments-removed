





#include "IndexedDatabaseManager.h"

#include "nsIConsoleService.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIFile.h"
#include "nsIFileStorage.h"
#include "nsIScriptError.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/quota/QuotaManager.h"
#include "mozilla/dom/quota/Utilities.h"
#include "mozilla/dom/TabContext.h"
#include "mozilla/storage.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsThreadUtils.h"

#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IDBKeyRange.h"
#include "IDBRequest.h"

USING_INDEXEDDB_NAMESPACE
using namespace mozilla::dom;
USING_QUOTA_NAMESPACE

static NS_DEFINE_CID(kDOMSOF_CID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

namespace {

mozilla::StaticRefPtr<IndexedDatabaseManager> gInstance;

int32_t gInitialized = 0;
int32_t gClosed = 0;

class AsyncDeleteFileRunnable MOZ_FINAL : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteFileRunnable(FileManager* aFileManager, int64_t aFileId);

private:
  nsRefPtr<FileManager> mFileManager;
  int64_t mFileId;
};

PLDHashOperator
InvalidateAndRemoveFileManagers(
                           const nsACString& aKey,
                           nsAutoPtr<nsTArray<nsRefPtr<FileManager> > >& aValue,
                           void* aUserArg)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");

  const nsACString* pattern =
    static_cast<const nsACString*>(aUserArg);

  if (!pattern || PatternMatchesOrigin(*pattern, aKey)) {
    for (uint32_t i = 0; i < aValue->Length(); i++) {
      nsRefPtr<FileManager>& fileManager = aValue->ElementAt(i);
      fileManager->Invalidate();
    }
    return PL_DHASH_REMOVE;
  }

  return PL_DHASH_NEXT;
}

} 

IndexedDatabaseManager::IndexedDatabaseManager()
: mFileMutex("IndexedDatabaseManager.mFileMutex")
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  mFileManagers.Init();
}

IndexedDatabaseManager::~IndexedDatabaseManager()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

bool IndexedDatabaseManager::sIsMainProcess = false;


IndexedDatabaseManager*
IndexedDatabaseManager::GetOrCreate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (IsClosed()) {
    NS_ERROR("Calling GetOrCreate() after shutdown!");
    return nullptr;
  }

  if (!gInstance) {
    sIsMainProcess = XRE_GetProcessType() == GeckoProcessType_Default;

    nsRefPtr<IndexedDatabaseManager> instance(new IndexedDatabaseManager());

    nsresult rv = instance->Init();
    NS_ENSURE_SUCCESS(rv, nullptr);

    if (PR_ATOMIC_SET(&gInitialized, 1)) {
      NS_ERROR("Initialized more than once?!");
    }

    gInstance = instance;

    ClearOnShutdown(&gInstance);
  }

  return gInstance;
}


IndexedDatabaseManager*
IndexedDatabaseManager::Get()
{
  
  return gInstance;
}


IndexedDatabaseManager*
IndexedDatabaseManager::FactoryCreate()
{
  
  
  IndexedDatabaseManager* mgr = GetOrCreate();
  NS_IF_ADDREF(mgr);
  return mgr;
}

nsresult
IndexedDatabaseManager::Init()
{
  
  NS_ENSURE_TRUE(QuotaManager::GetOrCreate(), NS_ERROR_FAILURE);

  
  nsCOMPtr<mozIStorageService> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(ss, NS_ERROR_FAILURE);

  return NS_OK;
}

void
IndexedDatabaseManager::Destroy()
{
  
  
  if (!!gInitialized && PR_ATOMIC_SET(&gClosed, 1)) {
    NS_ERROR("Shutdown more than once?!");
  }

  delete this;
}


nsresult
IndexedDatabaseManager::FireWindowOnError(nsPIDOMWindow* aOwner,
                                          nsEventChainPostVisitor& aVisitor)
{
  NS_ENSURE_TRUE(aVisitor.mDOMEvent, NS_ERROR_UNEXPECTED);
  if (!aOwner) {
    return NS_OK;
  }

  if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault) {
    return NS_OK;
  }

  nsString type;
  nsresult rv = aVisitor.mDOMEvent->GetType(type);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!type.EqualsLiteral(ERROR_EVT_STR)) {
    return NS_OK;
  }

  nsCOMPtr<EventTarget> eventTarget =
    aVisitor.mDOMEvent->InternalDOMEvent()->GetTarget();

  nsCOMPtr<nsIIDBRequest> strongRequest = do_QueryInterface(eventTarget);
  IDBRequest* request = static_cast<IDBRequest*>(strongRequest.get());
  NS_ENSURE_TRUE(request, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMDOMError> error;
  rv = request->GetError(getter_AddRefs(error));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString errorName;
  if (error) {
    rv = error->GetName(errorName);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsScriptErrorEvent event(true, NS_LOAD_ERROR);
  request->FillScriptErrorEvent(&event);
  NS_ABORT_IF_FALSE(event.fileName,
                    "FillScriptErrorEvent should give us a non-null string "
                    "for our error's fileName");

  event.errorMsg = errorName.get();

  nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aOwner));
  NS_ASSERTION(sgo, "How can this happen?!");

  nsEventStatus status = nsEventStatus_eIgnore;
  if (NS_FAILED(sgo->HandleScriptError(&event, &status))) {
    NS_WARNING("Failed to dispatch script error event");
    status = nsEventStatus_eIgnore;
  }

  bool preventDefaultCalled = status == nsEventStatus_eConsumeNoDefault;
  if (preventDefaultCalled) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIScriptError> scriptError =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (NS_FAILED(scriptError->InitWithWindowID(errorName,
                                              nsDependentString(event.fileName),
                                              EmptyString(), event.lineNr,
                                              0, 0,
                                              "IndexedDB",
                                              aOwner->WindowID()))) {
    NS_WARNING("Failed to init script error!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIConsoleService> consoleService =
    do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return consoleService->LogMessage(scriptError);
}


bool
IndexedDatabaseManager::TabContextMayAccessOrigin(const TabContext& aContext,
                                                  const nsACString& aOrigin)
{
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");

  
  
  
  nsAutoCString pattern;
  QuotaManager::GetOriginPatternStringMaybeIgnoreBrowser(
                                                aContext.OwnOrContainingAppId(),
                                                aContext.IsBrowserElement(),
                                                pattern);

  return PatternMatchesOrigin(pattern, aOrigin);
}


bool
IndexedDatabaseManager::IsClosed()
{
  return !!gClosed;
}

#ifdef DEBUG

bool
IndexedDatabaseManager::IsMainProcess()
{
  NS_ASSERTION(gInstance,
               "IsMainProcess() called before indexedDB has been initialized!");
  NS_ASSERTION((XRE_GetProcessType() == GeckoProcessType_Default) ==
               sIsMainProcess, "XRE_GetProcessType changed its tune!");
  return sIsMainProcess;
}
#endif

already_AddRefed<FileManager>
IndexedDatabaseManager::GetFileManager(const nsACString& aOrigin,
                                       const nsAString& aDatabaseName)
{
  nsTArray<nsRefPtr<FileManager> >* array;
  if (!mFileManagers.Get(aOrigin, &array)) {
    return nullptr;
  }

  for (uint32_t i = 0; i < array->Length(); i++) {
    nsRefPtr<FileManager>& fileManager = array->ElementAt(i);

    if (fileManager->DatabaseName().Equals(aDatabaseName)) {
      nsRefPtr<FileManager> result = fileManager;
      return result.forget();
    }
  }

  return nullptr;
}

void
IndexedDatabaseManager::AddFileManager(FileManager* aFileManager)
{
  NS_ASSERTION(aFileManager, "Null file manager!");

  nsTArray<nsRefPtr<FileManager> >* array;
  if (!mFileManagers.Get(aFileManager->Origin(), &array)) {
    array = new nsTArray<nsRefPtr<FileManager> >();
    mFileManagers.Put(aFileManager->Origin(), array);
  }

  array->AppendElement(aFileManager);
}

void
IndexedDatabaseManager::InvalidateAllFileManagers()
{
  mFileManagers.Enumerate(InvalidateAndRemoveFileManagers, nullptr);
}

void
IndexedDatabaseManager::InvalidateFileManagersForPattern(
                                                     const nsACString& aPattern)
{
  NS_ASSERTION(!aPattern.IsEmpty(), "Empty pattern!");
  mFileManagers.Enumerate(InvalidateAndRemoveFileManagers,
                          const_cast<nsACString*>(&aPattern));
}

void
IndexedDatabaseManager::InvalidateFileManager(const nsACString& aOrigin,
                                              const nsAString& aDatabaseName)
{
  nsTArray<nsRefPtr<FileManager> >* array;
  if (!mFileManagers.Get(aOrigin, &array)) {
    return;
  }

  for (uint32_t i = 0; i < array->Length(); i++) {
    nsRefPtr<FileManager> fileManager = array->ElementAt(i);
    if (fileManager->DatabaseName().Equals(aDatabaseName)) {
      fileManager->Invalidate();
      array->RemoveElementAt(i);

      if (array->IsEmpty()) {
        mFileManagers.Remove(aOrigin);
      }

      break;
    }
  }
}

nsresult
IndexedDatabaseManager::AsyncDeleteFile(FileManager* aFileManager,
                                        int64_t aFileId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aFileManager);

  QuotaManager* quotaManager = QuotaManager::Get();
  NS_ASSERTION(quotaManager, "Shouldn't be null!");

  
  
  if (quotaManager->IsClearOriginPending(aFileManager->Origin())) {
    return NS_OK;
  }

  nsRefPtr<AsyncDeleteFileRunnable> runnable =
    new AsyncDeleteFileRunnable(aFileManager, aFileId);

  nsresult rv =
    quotaManager->IOThread()->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_ADDREF(IndexedDatabaseManager)
NS_IMPL_RELEASE_WITH_DESTROY(IndexedDatabaseManager, Destroy())
NS_IMPL_QUERY_INTERFACE1(IndexedDatabaseManager, nsIIndexedDatabaseManager)

NS_IMETHODIMP
IndexedDatabaseManager::InitWindowless(const jsval& aObj, JSContext* aCx)
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_ARG(!JSVAL_IS_PRIMITIVE(aObj));

  JS::Rooted<JSObject*> obj(aCx, JSVAL_TO_OBJECT(aObj));

  JSBool hasIndexedDB;
  if (!JS_HasProperty(aCx, obj, "indexedDB", &hasIndexedDB)) {
    return NS_ERROR_FAILURE;
  }

  if (hasIndexedDB) {
    NS_WARNING("Passed object already has an 'indexedDB' property!");
    return NS_ERROR_FAILURE;
  }

  
  
  
  nsCOMPtr<nsIDOMScriptObjectFactory> sof(do_GetService(kDOMSOF_CID));

  JSObject* global = JS_GetGlobalForObject(aCx, obj);
  NS_ASSERTION(global, "What?! No global!");

  nsRefPtr<IDBFactory> factory;
  nsresult rv =
    IDBFactory::Create(aCx, global, nullptr, getter_AddRefs(factory));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);

  NS_ASSERTION(factory, "This should never fail for chrome!");

  jsval indexedDBVal;
  rv = nsContentUtils::WrapNative(aCx, obj, factory, &indexedDBVal);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!JS_DefineProperty(aCx, obj, "indexedDB", indexedDBVal, nullptr,
                         nullptr, JSPROP_ENUMERATE)) {
    return NS_ERROR_FAILURE;
  }

  JSObject* keyrangeObj = JS_NewObject(aCx, nullptr, nullptr, nullptr);
  NS_ENSURE_TRUE(keyrangeObj, NS_ERROR_OUT_OF_MEMORY);

  if (!IDBKeyRange::DefineConstructors(aCx, keyrangeObj)) {
    return NS_ERROR_FAILURE;
  }

  if (!JS_DefineProperty(aCx, obj, "IDBKeyRange", OBJECT_TO_JSVAL(keyrangeObj),
                         nullptr, nullptr, JSPROP_ENUMERATE)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

AsyncDeleteFileRunnable::AsyncDeleteFileRunnable(FileManager* aFileManager,
                                                 int64_t aFileId)
: mFileManager(aFileManager), mFileId(aFileId)
{
}

NS_IMPL_THREADSAFE_ISUPPORTS1(AsyncDeleteFileRunnable,
                              nsIRunnable)

NS_IMETHODIMP
AsyncDeleteFileRunnable::Run()
{
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIFile> directory = mFileManager->GetDirectory();
  NS_ENSURE_TRUE(directory, NS_ERROR_FAILURE);

  nsCOMPtr<nsIFile> file = mFileManager->GetFileForId(directory, mFileId);
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  nsresult rv;
  int64_t fileSize;

  if (mFileManager->Privilege() != Chrome) {
    rv = file->GetFileSize(&fileSize);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  }

  rv = file->Remove(false);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  if (mFileManager->Privilege() != Chrome) {
    QuotaManager* quotaManager = QuotaManager::Get();
    NS_ASSERTION(quotaManager, "Shouldn't be null!");

    quotaManager->DecreaseUsageForOrigin(mFileManager->Origin(), fileSize);
  }

  directory = mFileManager->GetJournalDirectory();
  NS_ENSURE_TRUE(directory, NS_ERROR_FAILURE);

  file = mFileManager->GetFileForId(directory, mFileId);
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  rv = file->Remove(false);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
