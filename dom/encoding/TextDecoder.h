





#ifndef mozilla_dom_textdecoder_h_
#define mozilla_dom_textdecoder_h_

#include "mozilla/dom/NonRefcountedDOMObject.h"
#include "mozilla/dom/TextDecoderBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "nsIUnicodeDecoder.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class ArrayBufferViewOrArrayBuffer;

class TextDecoder final
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

  bool WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto, JS::MutableHandle<JSObject*> aReflector)
  {
    return TextDecoderBinding::Wrap(aCx, this, aGivenProto, aReflector);
  }

  







  void Init(const nsAString& aLabel, const bool aFatal, ErrorResult& aRv);

  







  void InitWithEncoding(const nsACString& aEncoding, const bool aFatal);

  




  void GetEncoding(nsAString& aEncoding);

  















  void Decode(const char* aInput, const int32_t aLength,
              const bool aStream, nsAString& aOutDecodedString,
              ErrorResult& aRv);

  void Decode(const Optional<ArrayBufferViewOrArrayBuffer>& aBuffer,
              const TextDecodeOptions& aOptions,
              nsAString& aOutDecodedString,
              ErrorResult& aRv);

  bool Fatal() const {
    return mFatal;
  }

private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  bool mFatal;
};

} 
} 

#endif 
