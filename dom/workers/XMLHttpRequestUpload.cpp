




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
XMLHttpRequestUpload::_Trace(JSTracer* aTrc)
{
  if (mXHR) {
    JS_CALL_OBJECT_TRACER(aTrc, mXHR->GetJSObject(), "mXHR");
  }
  XMLHttpRequestEventTarget::_Trace(aTrc);
}

void
XMLHttpRequestUpload::_Finalize(JSContext* aCx)
{
  XMLHttpRequestEventTarget::_Finalize(aCx);
}
