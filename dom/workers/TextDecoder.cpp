




#include "TextDecoder.h"
#include "DOMBindingInlines.h"

USING_WORKERS_NAMESPACE
using mozilla::ErrorResult;
using mozilla::dom::TextDecoderOptionsWorkers;

void
TextDecoder::_trace(JSTracer* aTrc)
{
  DOMBindingBase::_trace(aTrc);
}

void
TextDecoder::_finalize(JSFreeOp* aFop)
{
  DOMBindingBase::_finalize(aFop);
}


TextDecoder*
TextDecoder::Constructor(JSContext* aCx, JSObject* aObj,
                         const nsAString& aEncoding,
                         const TextDecoderOptionsWorkers& aOptions,
                         ErrorResult& aRv)
{
  nsRefPtr<TextDecoder> txtDecoder = new TextDecoder(aCx);
  txtDecoder->Init(aEncoding, aOptions.mFatal, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  if (!Wrap(aCx, aObj, txtDecoder)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return txtDecoder;
}
