




#include "XMLHttpRequestUpload.h"

#include "XMLHttpRequest.h"

#include "DOMBindingInlines.h"

USING_WORKERS_NAMESPACE


XMLHttpRequestUpload*
XMLHttpRequestUpload::Create(JSContext* aCx, XMLHttpRequest* aXHR)
{
  nsRefPtr<XMLHttpRequestUpload> upload = new XMLHttpRequestUpload(aCx, aXHR);
  return Wrap(aCx, NULL, upload) ? upload : NULL;
}

void
XMLHttpRequestUpload::_trace(JSTracer* aTrc)
{
  if (mXHR) {
    JS_CallObjectTracer(aTrc, mXHR->GetJSObject(), "mXHR");
  }
  XMLHttpRequestEventTarget::_trace(aTrc);
}

void
XMLHttpRequestUpload::_finalize(JSFreeOp* aFop)
{
  XMLHttpRequestEventTarget::_finalize(aFop);
}
