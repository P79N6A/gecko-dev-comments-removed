



#ifndef mozilla_dom_textdecoder_h_
#define mozilla_dom_textdecoder_h_

#include "jsapi.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TextDecoderBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/ErrorResult.h"
#include "nsIUnicodeDecoder.h"
#include "nsString.h"

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {

class TextDecoder : public nsISupports, public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TextDecoder)

  
  static already_AddRefed<TextDecoder>
  Constructor(nsISupports* aGlobal,
              const nsAString& aEncoding,
              const TextDecoderOptions& aFatal,
              ErrorResult& aRv)
  {
    nsRefPtr<TextDecoder> txtDecoder = new TextDecoder(aGlobal);
    txtDecoder->Init(aEncoding, aFatal, aRv);
    if (aRv.Failed()) {
      return nullptr;
    }
    return txtDecoder.forget();
  }

  TextDecoder(nsISupports* aGlobal)
    : mGlobal(aGlobal), mFatal(false)
  {
    MOZ_ASSERT(aGlobal);
    SetIsDOMBinding();
  }

  virtual
  ~TextDecoder()
  {}

  virtual JSObject*
  WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap)
  {
    return TextDecoderBinding::Wrap(aCx, aScope, this, aTriedToWrap);
  }

  nsISupports*
  GetParentObject()
  {
    return mGlobal;
  }

  




  void GetEncoding(nsAString& aEncoding);

  















  void Decode(const ArrayBufferView* aView,
              const TextDecodeOptions& aOptions,
              nsAString& aOutDecodedString,
              ErrorResult& aRv);

private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  nsCOMPtr<nsISupports> mGlobal;
  bool mFatal;

  









  void Init(const nsAString& aEncoding,
            const TextDecoderOptions& aFatal,
            ErrorResult& aRv);
};

} 
} 

#endif 
