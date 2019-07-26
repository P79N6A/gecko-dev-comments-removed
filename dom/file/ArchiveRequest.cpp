





#include "ArchiveRequest.h"

#include "mozilla/dom/ArchiveRequestBinding.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsEventDispatcher.h"

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
  MOZ_ASSERT(aReader);

  MOZ_COUNT_CTOR(ArchiveRequest);

  
  nsRefPtr<ArchiveRequestEvent> event = new ArchiveRequestEvent(this);
  NS_DispatchToCurrentThread(event);
}

ArchiveRequest::~ArchiveRequest()
{
  MOZ_COUNT_DTOR(ArchiveRequest);
}

nsresult
ArchiveRequest::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = true;
  aVisitor.mParentTarget = nullptr;
  return NS_OK;
}

 JSObject*
ArchiveRequest::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return ArchiveRequestBinding::Wrap(aCx, aScope, this);
}

ArchiveReader*
ArchiveRequest::Reader() const
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  return mArchiveReader;
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

  nsresult rv;

  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  NS_ENSURE_STATE(sc);

  AutoPushJSContext cx(sc->GetNativeContext());
  NS_ASSERTION(cx, "Failed to get a context!");

  JS::Rooted<JSObject*> global(cx, sc->GetWindowProxy());
  NS_ASSERTION(global, "Failed to get global object!");

  JSAutoCompartment ac(cx, global);

  JS::Rooted<JS::Value> result(cx);
  switch (mOperation) {
    case GetFilenames:
      rv = GetFilenamesResult(cx, result.address(), aFileList);
      break;

    case GetFile:
      rv = GetFileResult(cx, result.address(), aFileList);
      break;

      case GetFiles:
        rv = GetFilesResult(cx, result.address(), aFileList);
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
  JS::Rooted<JSObject*> array(aCx, JS_NewArrayObject(aCx, aFileList.Length(), nullptr));
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

    JS::Rooted<JS::Value> item(aCx, STRING_TO_JSVAL(str));

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
      JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));
      return nsContentUtils::WrapNative(aCx, global, file,
                                        &NS_GET_IID(nsIDOMFile), aValue);
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
ArchiveRequest::GetFilesResult(JSContext* aCx,
                               JS::Value* aValue,
                               nsTArray<nsCOMPtr<nsIDOMFile> >& aFileList)
{
  JS::Rooted<JSObject*> array(aCx, JS_NewArrayObject(aCx, aFileList.Length(), nullptr));
  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t i = 0; i < aFileList.Length(); ++i) {
    nsCOMPtr<nsIDOMFile> file = aFileList[i];

    JS::Rooted<JS::Value> value(aCx);
    JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));
    nsresult rv = nsContentUtils::WrapNative(aCx, global, file,
                                             &NS_GET_IID(nsIDOMFile),
                                             value.address());
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
NS_INTERFACE_MAP_END_INHERITING(DOMRequest)

NS_IMPL_ADDREF_INHERITED(ArchiveRequest, DOMRequest)
NS_IMPL_RELEASE_INHERITED(ArchiveRequest, DOMRequest)
