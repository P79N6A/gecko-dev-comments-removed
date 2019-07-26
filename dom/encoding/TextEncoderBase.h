



#ifndef mozilla_dom_textencoderbase_h_
#define mozilla_dom_textencoderbase_h_

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TypedArray.h"
#include "nsIUnicodeEncoder.h"

namespace mozilla {
class ErrorResult;

namespace dom {

class TextEncoderBase
{
protected:
  TextEncoderBase()
  {}

  virtual
  ~TextEncoderBase()
  {}

  








  void Init(const nsAString& aEncoding, ErrorResult& aRv);

public:
  




  void GetEncoding(nsAString& aEncoding);

  










  JSObject* Encode(JSContext* aCx, const nsAString& aString,
                   const bool aStream, ErrorResult& aRv);

protected:
  virtual JSObject*
  CreateUint8Array(JSContext* aCx, char* aBuf, uint32_t aLen) = 0;

private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;
};

} 
} 

#endif 
