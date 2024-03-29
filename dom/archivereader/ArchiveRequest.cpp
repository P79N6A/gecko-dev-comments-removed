





#include "ArchiveRequest.h"

#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/ArchiveRequestBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsContentUtils.h"

using namespace mozilla;

USING_ARCHIVEREADER_NAMESPACE




class ArchiveRequestEvent : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  explicit ArchiveRequestEvent(ArchiveRequest* aRequest)
  : mRequest(aRequest)
  {
    MOZ_COUNT_CTOR(ArchiveRequestEvent);
  }

protected:
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
  MOZ_ASSERT(mRequest, "the request is not longer valid");
  mRequest->Run();
  return NS_OK;
}



ArchiveRequest::ArchiveRequest(nsPIDOMWindow* aWindow,
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
ArchiveRequest::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = true;
  aVisitor.mParentTarget = nullptr;
  return NS_OK;
}

 JSObject*
ArchiveRequest::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return ArchiveRequestBinding::Wrap(aCx, this, aGivenProto);
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
ArchiveRequest::ReaderReady(nsTArray<nsRefPtr<File>>& aFileList,
                            nsresult aStatus)
{
  if (NS_FAILED(aStatus)) {
    FireError(aStatus);
    return NS_OK;
  }

  nsresult rv;

  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(GetOwner()))) {
    return NS_ERROR_UNEXPECTED;
  }
  JSContext* cx = jsapi.cx();

  JS::Rooted<JS::Value> result(cx);
  switch (mOperation) {
    case GetFilenames:
      rv = GetFilenamesResult(cx, result.address(), aFileList);
      break;

    case GetFile:
      rv = GetFileResult(cx, &result, aFileList);
      break;

    case GetFiles:
      rv = GetFilesResult(cx, &result, aFileList);
      break;

    default:
      rv = NS_ERROR_UNEXPECTED;
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
                                   nsTArray<nsRefPtr<File>>& aFileList)
{
  JS::Rooted<JSObject*> array(aCx, JS_NewArrayObject(aCx, aFileList.Length()));

  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  JS::Rooted<JSString*> str(aCx);
  for (uint32_t i = 0; i < aFileList.Length(); ++i) {
    nsRefPtr<File> file = aFileList[i];

    nsString filename;
    file->GetName(filename);

    str = JS_NewUCStringCopyZ(aCx, filename.get());
    NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

    if (!JS_DefineElement(aCx, array, i, str, JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }
  }

  if (!JS_FreezeObject(aCx, array)) {
    return NS_ERROR_FAILURE;
  }

  aValue->setObject(*array);
  return NS_OK;
}

nsresult
ArchiveRequest::GetFileResult(JSContext* aCx,
                              JS::MutableHandle<JS::Value> aValue,
                              nsTArray<nsRefPtr<File>>& aFileList)
{
  for (uint32_t i = 0; i < aFileList.Length(); ++i) {
    nsRefPtr<File> file = aFileList[i];

    nsString filename;
    file->GetName(filename);

    if (filename == mFilename) {
      if (!ToJSValue(aCx, file, aValue)) {
        return NS_ERROR_FAILURE;
      }

      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
ArchiveRequest::GetFilesResult(JSContext* aCx,
                               JS::MutableHandle<JS::Value> aValue,
                               nsTArray<nsRefPtr<File>>& aFileList)
{
  JS::Rooted<JSObject*> array(aCx, JS_NewArrayObject(aCx, aFileList.Length()));
  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t i = 0; i < aFileList.Length(); ++i) {
    nsRefPtr<File> file = aFileList[i];

    JS::Rooted<JS::Value> value(aCx);
    if (!ToJSValue(aCx, file, &value)) {
      return NS_ERROR_FAILURE;
    }

    if (!JS_DefineElement(aCx, array, i, value, JSPROP_ENUMERATE)) {
      return NS_ERROR_FAILURE;
    }
  }

  aValue.setObject(*array);
  return NS_OK;
}


already_AddRefed<ArchiveRequest>
ArchiveRequest::Create(nsPIDOMWindow* aOwner,
                       ArchiveReader* aReader)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<ArchiveRequest> request = new ArchiveRequest(aOwner, aReader);

  return request.forget();
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(ArchiveRequest, DOMRequest,
                                   mArchiveReader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ArchiveRequest)
NS_INTERFACE_MAP_END_INHERITING(DOMRequest)

NS_IMPL_ADDREF_INHERITED(ArchiveRequest, DOMRequest)
NS_IMPL_RELEASE_INHERITED(ArchiveRequest, DOMRequest)
