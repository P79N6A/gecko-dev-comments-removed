



#ifndef mozilla_dom_textdecoder_h_
#define mozilla_dom_textdecoder_h_

#include "mozilla/dom/TextDecoderBase.h"
#include "mozilla/dom/TextDecoderBinding.h"

namespace mozilla {
namespace dom {

class TextDecoder MOZ_FINAL
  : public NonRefcountedDOMObject, public TextDecoderBase
{
public:
  
  static TextDecoder*
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aEncoding,
              const TextDecoderOptions& aOptions,
              ErrorResult& aRv)
  {
    nsAutoPtr<TextDecoder> txtDecoder(new TextDecoder());
    txtDecoder->Init(aEncoding, aOptions.mFatal, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
    return txtDecoder.forget();
  }

  TextDecoder()
  {
  }

  virtual
  ~TextDecoder()
  {}

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope,
                       bool* aTookOwnership)
  {
    return TextDecoderBinding::Wrap(aCx, aScope, this, aTookOwnership);
  }

  nsISupports*
  GetParentObject()
  {
    return nullptr;
  }

  void Decode(nsAString& aOutDecodedString,
              ErrorResult& aRv) {
    TextDecoderBase::Decode(nullptr, 0, false,
                            aOutDecodedString, aRv);
  }

  void Decode(const ArrayBufferView& aView,
              const TextDecodeOptions& aOptions,
              nsAString& aOutDecodedString,
              ErrorResult& aRv) {
    TextDecoderBase::Decode(reinterpret_cast<char*>(aView.Data()),
                            aView.Length(), aOptions.mStream,
                            aOutDecodedString, aRv);
  }

private:
};

} 
} 

#endif
