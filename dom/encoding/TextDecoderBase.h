



#ifndef mozilla_dom_textdecoderbase_h_
#define mozilla_dom_textdecoderbase_h_

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TypedArray.h"
#include "nsIUnicodeDecoder.h"

namespace mozilla {
class ErrorResult;

namespace dom {

class TextDecoderBase
{
public:
  TextDecoderBase()
    : mFatal(false)
  {}

  virtual
  ~TextDecoderBase()
  {}

  









  void Init(const nsAString& aEncoding, const bool aFatal, ErrorResult& aRv);

  




  void GetEncoding(nsAString& aEncoding);

  















  void Decode(const char* aInput, const int32_t aLength,
              const bool aStream, nsAString& aOutDecodedString,
              ErrorResult& aRv);

private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeDecoder> mDecoder;
  bool mFatal;
};

} 
} 

#endif 
