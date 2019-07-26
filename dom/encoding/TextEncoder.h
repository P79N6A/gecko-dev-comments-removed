



#ifndef mozilla_dom_textencoder_h_
#define mozilla_dom_textencoder_h_

#include "mozilla/dom/NonRefcountedDOMObject.h"
#include "mozilla/dom/TextEncoderBase.h"
#include "mozilla/dom/TextEncoderBinding.h"

namespace mozilla {
namespace dom {

class TextEncoder MOZ_FINAL
  : public NonRefcountedDOMObject, public TextEncoderBase
{
public:
  

  static TextEncoder*
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aEncoding,
              ErrorResult& aRv)
  {
    nsAutoPtr<TextEncoder> txtEncoder(new TextEncoder());
    txtEncoder->Init(aEncoding, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
    return txtEncoder.forget();
  }

  TextEncoder()
  {
  }

  virtual
  ~TextEncoder()
  {}

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope,
                       bool* aTookOwnership)
  {
    return TextEncoderBinding::Wrap(aCx, aScope, this, aTookOwnership);
  }

  nsISupports*
  GetParentObject()
  {
    return nullptr;
  }

  JSObject* Encode(JSContext* aCx,
                   JS::Handle<JSObject*> aObj,
                   const nsAString& aString,
                   const TextEncodeOptions& aOptions,
                   ErrorResult& aRv) {
    return TextEncoderBase::Encode(aCx, aObj, aString, aOptions.mStream, aRv);
  }

private:
};

} 
} 

#endif
