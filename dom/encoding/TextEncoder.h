




#ifndef mozilla_dom_textencoder_h_
#define mozilla_dom_textencoder_h_

#include "mozilla/dom/NonRefcountedDOMObject.h"
#include "mozilla/dom/TextEncoderBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "nsIUnicodeEncoder.h"

namespace mozilla {
class ErrorResult;

namespace dom {

class TextEncoder MOZ_FINAL : public NonRefcountedDOMObject
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

  JSObject* WrapObject(JSContext* aCx, bool* aTookOwnership)
  {
    return TextEncoderBinding::Wrap(aCx, this, aTookOwnership);
  }

protected:

  








  void Init(const nsAString& aEncoding, ErrorResult& aRv);

public:
  




  void GetEncoding(nsAString& aEncoding);

  








  void Encode(JSContext* aCx,
              JS::Handle<JSObject*> aObj,
              const nsAString& aString,
              JS::MutableHandle<JSObject*> aRetval,
              ErrorResult& aRv);
private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;
};

} 
} 

#endif 
