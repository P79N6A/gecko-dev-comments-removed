





#include "DOMFileRequest.h"

#include "mozilla/dom/FileRequestBinding.h"
#include "LockedFile.h"

USING_FILE_NAMESPACE

DOMFileRequest::DOMFileRequest(nsIDOMWindow* aWindow)
  : FileRequest(aWindow)
{
}

 JSObject*
DOMFileRequest::WrapObject(JSContext* aCx, JSObject* aScope)
{
  return FileRequestBinding::Wrap(aCx, aScope, this);
}

nsIDOMLockedFile*
DOMFileRequest::GetLockedFile() const
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return mLockedFile;
}
