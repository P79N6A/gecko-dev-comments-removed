




#include "XMLHttpRequestUpload.h"

#include "XMLHttpRequest.h"

#include "mozilla/dom/XMLHttpRequestUploadBinding.h"

USING_WORKERS_NAMESPACE

XMLHttpRequestUpload::XMLHttpRequestUpload(XMLHttpRequest* aXHR)
: mXHR(aXHR)
{
  SetIsDOMBinding();
}

XMLHttpRequestUpload::~XMLHttpRequestUpload()
{
}

NS_IMPL_ADDREF_INHERITED(XMLHttpRequestUpload, nsXHREventTarget)
NS_IMPL_RELEASE_INHERITED(XMLHttpRequestUpload, nsXHREventTarget)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(XMLHttpRequestUpload)
NS_INTERFACE_MAP_END_INHERITING(nsXHREventTarget)

NS_IMPL_CYCLE_COLLECTION_INHERITED(XMLHttpRequestUpload, nsXHREventTarget,
                                   mXHR)

JSObject*
XMLHttpRequestUpload::WrapObject(JSContext* aCx)
{
  return XMLHttpRequestUploadBinding_workers::Wrap(aCx, this);
}


already_AddRefed<XMLHttpRequestUpload>
XMLHttpRequestUpload::Create(XMLHttpRequest* aXHR)
{
  nsRefPtr<XMLHttpRequestUpload> upload = new XMLHttpRequestUpload(aXHR);
  return upload.forget();
}
