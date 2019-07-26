




#ifndef mozilla_dom_workers_textdecoder_h_
#define mozilla_dom_workers_textdecoder_h_

#include "mozilla/dom/TextDecoderBase.h"
#include "mozilla/dom/workers/bindings/DOMBindingBase.h"
#include "mozilla/dom/TextDecoderBinding.h"

BEGIN_WORKERS_NAMESPACE

class TextDecoder MOZ_FINAL : public DOMBindingBase,
                              public TextDecoderBase
{
protected:
  TextDecoder(JSContext* aCx)
  : DOMBindingBase(aCx)
  {}

  virtual
  ~TextDecoder()
  {}

public:
  virtual void
  _trace(JSTracer* aTrc) MOZ_OVERRIDE;

  virtual void
  _finalize(JSFreeOp* aFop) MOZ_OVERRIDE;

  static TextDecoder*
  Constructor(JSContext* aCx, JSObject* aObj,
              const nsAString& aEncoding,
              const TextDecoderOptionsWorkers& aOptions,
              ErrorResult& aRv);

  void
  Decode(const ArrayBufferView* aView,
         const TextDecodeOptionsWorkers& aOptions,
         nsAString& aOutDecodedString,
         ErrorResult& aRv) {
    return TextDecoderBase::Decode(aView, aOptions.mStream,
                                   aOutDecodedString, aRv);
  }
};

END_WORKERS_NAMESPACE

#endif 
