



#ifndef mozilla_dom_textencoder_h_
#define mozilla_dom_textencoder_h_

#include "mozilla/dom/TextEncoderBase.h"
#include "mozilla/dom/TextEncoderBinding.h"

namespace mozilla {
namespace dom {

class TextEncoder MOZ_FINAL
  : public nsISupports, public nsWrapperCache, public TextEncoderBase
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TextEncoder)

  
  static already_AddRefed<TextEncoder>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aEncoding,
              ErrorResult& aRv)
  {
    nsRefPtr<TextEncoder> txtEncoder = new TextEncoder(aGlobal.Get());
    txtEncoder->Init(aEncoding, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
    return txtEncoder.forget();
  }

  TextEncoder(nsISupports* aGlobal)
    : mGlobal(aGlobal)
  {
    MOZ_ASSERT(aGlobal);
    SetIsDOMBinding();
  }

  virtual
  ~TextEncoder()
  {}

  virtual JSObject*
  WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap) MOZ_OVERRIDE
  {
    return TextEncoderBinding::Wrap(aCx, aScope, this, aTriedToWrap);
  }

  nsISupports*
  GetParentObject()
  {
    return mGlobal;
  }

  JSObject* Encode(JSContext* aCx,
                   const nsAString& aString,
                   const TextEncodeOptions& aOptions,
                   ErrorResult& aRv) {
    return TextEncoderBase::Encode(aCx, aString, aOptions.mStream, aRv);
  }

protected:
  virtual JSObject*
  CreateUint8Array(JSContext* aCx, char* aBuf, uint32_t aLen) MOZ_OVERRIDE
  {
    return Uint8Array::Create(aCx, this, aLen,
                              reinterpret_cast<uint8_t*>(aBuf));
  }

private:
  nsCOMPtr<nsISupports> mGlobal;
};

} 
} 

#endif 
