



#ifndef mozilla_dom_textencoder_h_
#define mozilla_dom_textencoder_h_

#include "jsapi.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TextEncoderBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/ErrorResult.h"
#include "nsIUnicodeEncoder.h"
#include "nsString.h"

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {

class TextEncoder : public nsISupports, public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TextEncoder)

  
  static already_AddRefed<TextEncoder>
  Constructor(nsISupports* aGlobal,
              const Optional<nsAString>& aEncoding,
              ErrorResult& aRv)
  {
    nsRefPtr<TextEncoder> txtEncoder = new TextEncoder(aGlobal);
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
  WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
  {
    return TextEncoderBinding::Wrap(aCx, aScope, this, aTriedToWrap);
  }

  nsISupports*
  GetParentObject()
  {
    return mGlobal;
  }

  




  void GetEncoding(nsAString& aEncoding);

  










  JSObject* Encode(JSContext* aCx,
                   const nsAString& aString,
                   const TextEncodeOptions& aOptions,
                   ErrorResult& aRv);
private:
  const char* mEncoding;
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;
  nsCOMPtr<nsISupports> mGlobal;

  








  void Init(const Optional<nsAString>& aEncoding,
            ErrorResult& aRv);
};

} 
} 

#endif 
