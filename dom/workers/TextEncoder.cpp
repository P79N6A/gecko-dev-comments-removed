




#include "TextEncoder.h"
#include "DOMBindingInlines.h"

USING_WORKERS_NAMESPACE
using mozilla::ErrorResult;
using mozilla::dom::WorkerGlobalObject;

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
TextEncoder::Constructor(const WorkerGlobalObject& aGlobal,
                         const nsAString& aEncoding,
                         ErrorResult& aRv)
{
  nsRefPtr<TextEncoder> txtEncoder = new TextEncoder(aGlobal.GetContext());
  txtEncoder->Init(aEncoding, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  if (!Wrap(aGlobal.GetContext(), aGlobal.Get(), txtEncoder)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return txtEncoder;
}
