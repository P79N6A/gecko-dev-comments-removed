





#include "DOMFileRequest.h"

#include "mozilla/dom/FileRequestBinding.h"
#include "LockedFile.h"

USING_FILE_NAMESPACE

DOMFileRequest::DOMFileRequest(nsPIDOMWindow* aWindow)
  : FileRequest(aWindow)
{
}


already_AddRefed<DOMFileRequest>
DOMFileRequest::Create(nsPIDOMWindow* aOwner, LockedFile* aLockedFile)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<DOMFileRequest> request = new DOMFileRequest(aOwner);
  request->mLockedFile = aLockedFile;

  return request.forget();
}

 JSObject*
DOMFileRequest::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return FileRequestBinding::Wrap(aCx, aScope, this);
}

nsIDOMLockedFile*
DOMFileRequest::GetLockedFile() const
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return mLockedFile;
}
