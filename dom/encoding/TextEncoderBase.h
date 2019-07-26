



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

  











  JSObject* Encode(JSContext* aCx,
                   JS::Handle<JSObject*> aObj,
                   const nsAString& aString,
                   const bool aStream,
                   ErrorResult& aRv);

protected:
  JSObject*
  CreateUint8Array(JSContext* aCx, JS::Handle<JSObject*> aObj, 
                   char* aBuf, uint32_t aLen) const
  {
    return Uint8Array::Create(aCx, aObj, aLen,
                              reinterpret_cast<uint8_t*>(aBuf));
  }

private:
  nsCString mEncoding;
  nsCOMPtr<nsIUnicodeEncoder> mEncoder;
};

} 
} 

#endif 
