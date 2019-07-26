




#include "TextEncoder.h"
#include "DOMBindingInlines.h"

USING_WORKERS_NAMESPACE
using mozilla::ErrorResult;

void
TextEncoder::_trace(JSTracer* aTrc)
{
  DOMBindingBase::_trace(aTrc);
}

void
TextEncoder::_finalize(JSFreeOp* aFop)
{
  DOMBindingBase::_finalize(aFop);
}


TextEncoder*
TextEncoder::Constructor(JSContext* aCx, JSObject* aObj,
                         const nsAString& aEncoding,
                         ErrorResult& aRv)
{
  nsRefPtr<TextEncoder> txtEncoder = new TextEncoder(aCx);
  txtEncoder->Init(aEncoding, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  if (!Wrap(aCx, aObj, txtEncoder)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return txtEncoder;
}
