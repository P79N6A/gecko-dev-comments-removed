



#ifndef mozilla_dom_textdecoder_h_
#define mozilla_dom_textdecoder_h_

#include "mozilla/dom/NonRefcountedDOMObject.h"
#include "mozilla/dom/TextDecoderBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "nsIUnicodeDecoder.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class TextDecoder MOZ_FINAL
  : public NonRefcountedDOMObject
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
    : mFatal(false)
  {
    MOZ_COUNT_CTOR(TextDecoder);
  }

  ~TextDecoder()
  {
    MOZ_COUNT_DTOR(TextDecoder);
  }

  JSObject* WrapObject(JSContext* aCx, bool* aTookOwnership)
  {
    return TextDecoderBinding::Wrap(aCx, this, aTookOwnership);
  }

  







  void Init(const nsAString& aLabel, const bool aFatal, ErrorResult& aRv);

  







  void InitWithEncoding(const nsACString& aEncoding, const bool aFatal);

  




  void GetEncoding(nsAString& aEncoding);

  















  void Decode(const char* aInput, const int32_t aLength,
              const bool aStream, nsAString& aOutDecodedString,
              ErrorResult& aRv);

  void Decode(nsAString& aOutDecodedString,
              ErrorResult& aRv) {
    Decode(nullptr, 0, false, aOutDecodedString, aRv);
  }

  void Decode(const ArrayBufferView& aView,
              const TextDecodeOptions& aOptions,
              nsAString& aOutDecodedString,
              ErrorResult& aRv) {
    Decode(reinterpret_cast<char*>(aView.Data()), aView.Length(),
           aOptions.mStream, aOutDecodedString, aRv);
  }

private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  bool mFatal;
};

} 
} 

#endif 
