





#include "ArchiveRequest.h"

#include "nsContentUtils.h"
#include "nsLayoutStatics.h"
#include "nsEventDispatcher.h"
#include "nsDOMClassInfoID.h"

USING_FILE_NAMESPACE




class ArchiveRequestEvent : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  ArchiveRequestEvent(ArchiveRequest* request)
  : mRequest(request)
  {
    MOZ_COUNT_CTOR(ArchiveRequestEvent);
  }

  ~ArchiveRequestEvent()
  {
    MOZ_COUNT_DTOR(ArchiveRequestEvent);
  }

private: 
  nsRefPtr<ArchiveRequest> mRequest;
};

NS_IMETHODIMP
ArchiveRequestEvent::Run()
{
  NS_ABORT_IF_FALSE(mRequest, "the request is not longer valid");
  mRequest->Run();
  return NS_OK;
}



ArchiveRequest::ArchiveRequest(nsIDOMWindow* aWindow,
                               ArchiveReader* aReader)
: DOMRequest(aWindow),
  mArchiveReader(aReader)
{
  MOZ_COUNT_CTOR(ArchiveRequest);
  nsLayoutStatics::AddRef();

  
  nsRefPtr<ArchiveRequestEvent> event = new ArchiveRequestEvent(this);
  NS_DispatchToCurrentThread(event);
}

ArchiveRequest::~ArchiveRequest()
{
  MOZ_COUNT_DTOR(ArchiveRequest);
  nsLayoutStatics::Release();
}

nsresult
ArchiveRequest::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = true;
  aVisitor.mParentTarget = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
ArchiveRequest::GetReader(nsIDOMArchiveReader** aArchiveReader)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIDOMArchiveReader> archiveReader(mArchiveReader);
  archiveReader.forget(aArchiveReader);
  return NS_OK;
}


void
ArchiveRequest::Run()
{
  
  
  nsresult rv = mArchiveReader->RegisterRequest(this);
  if (NS_FAILED(rv)) {
    FireError(rv);
  }
}

void
ArchiveRequest::OpGetFilenames()
{
  mOperation = GetFilenames;
}

void
ArchiveRequest::OpGetFile(const nsAString& aFilename)
{
  mOperation = GetFile;
  mFilename = aFilename;
}

void
ArchiveRequest::OpGetFiles()
{
  mOperation = GetFiles;
}

nsresult
ArchiveRequest::ReaderReady(nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList,
                            nsresult aStatus)
{
  if (NS_FAILED(aStatus)) {
    FireError(aStatus);
    return NS_OK;
  }

  JS::Value result;
  nsresult rv;

  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  NS_ENSURE_STATE(sc);

  AutoPushJSContext cx(sc->GetNativeContext());
  NS_ASSERTION(cx, "Failed to get a context!");

  JSObject* global = sc->GetNativeGlobal();
  NS_ASSERTION(global, "Failed to get global object!");

  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, global);

  switch (mOperation) {
    case GetFilenames:
      rv = GetFilenamesResult(cx, &result, aFileList);
      break;

    case GetFile:
      rv = GetFileResult(cx, &result, aFileList);
      break;

      case GetFiles:
        rv = GetFilesResult(cx, &result, aFileList);
        break;
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Get*Result failed!");
  }

  if (NS_SUCCEEDED(rv)) {
    FireSuccess(result);
  }
  else {
    FireError(rv);
  }

  return NS_OK;
}

nsresult
ArchiveRequest::GetFilenamesResult(JSContext* aCx,
                                   JS::Value* aValue,
                                   nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList)
{
  JSObject* array = JS_NewArrayObject(aCx, aFileList.Length(), nullptr);
  nsresult rv;

  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t i = 0; i < aFileList.Length(); ++i) {
    nsCOMPtr<nsIDOMFile> file = aFileList[i];

    nsString filename;
    rv = file->GetName(filename);
    NS_ENSURE_SUCCESS(rv, rv);

    JSString* str = JS_NewUCStringCopyZ(aCx, filename.get());
    NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

    JS::Value item = STRING_TO_JSVAL(str);

    if (NS_FAILED(rv) || !JS_SetElement(aCx, array, i, &item)) {
      return NS_ERROR_FAILURE;
    }
  }

  if (!JS_FreezeObject(aCx, array)) {
    return NS_ERROR_FAILURE;
  }

  *aValue = OBJECT_TO_JSVAL(array);
  return NS_OK;
}

nsresult
ArchiveRequest::GetFileResult(JSContext* aCx,
                              JS::Value* aValue,
                              nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList)
{
  for (uint32_t i = 0; i < aFileList.Length(); ++i) {
    nsCOMPtr<nsIDOMFile> file = aFileList[i];

    nsString filename;
    nsresult rv = file->GetName(filename);
    NS_ENSURE_SUCCESS(rv, rv);

    if (filename == mFilename) {
      nsresult rv = nsContentUtils::WrapNative(
                      aCx, JS_GetGlobalForScopeChain(aCx),
                      file, &NS_GET_IID(nsIDOMFile), aValue);
      return rv;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
ArchiveRequest::GetFilesResult(JSContext* aCx,
                               JS::Value* aValue,
                               nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList)
{
  JSObject* array = JS_NewArrayObject(aCx, aFileList.Length(), nullptr);
  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t i = 0; i < aFileList.Length(); ++i) {
    nsCOMPtr<nsIDOMFile> file = aFileList[i];

    JS::Value value;
    nsresult rv = nsContentUtils::WrapNative(aCx, JS_GetGlobalForScopeChain(aCx),
                                             file, &NS_GET_IID(nsIDOMFile), &value);
    if (NS_FAILED(rv) || !JS_SetElement(aCx, array, i, &value)) {
      return NS_ERROR_FAILURE;
    }
  }

  aValue->setObject(*array);
  return NS_OK;
}


already_AddRefed<ArchiveRequest>
ArchiveRequest::Create(nsIDOMWindow* aOwner,
                       ArchiveReader* aReader)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<ArchiveRequest> request = new ArchiveRequest(aOwner, aReader);

  return request.forget();
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(ArchiveRequest, DOMRequest,
                                     mArchiveReader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ArchiveRequest)
  NS_INTERFACE_MAP_ENTRY(nsIDOMArchiveRequest)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ArchiveRequest)
NS_INTERFACE_MAP_END_INHERITING(DOMRequest)

NS_IMPL_ADDREF_INHERITED(ArchiveRequest, DOMRequest)
NS_IMPL_RELEASE_INHERITED(ArchiveRequest, DOMRequest)

DOMCI_DATA(ArchiveRequest, ArchiveRequest)
