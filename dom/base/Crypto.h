


#ifndef mozilla_dom_Crypto_h
#define mozilla_dom_Crypto_h

#include "nsIDOMCrypto.h"
#include "mozilla/dom/SubtleCrypto.h"
#include "nsPIDOMWindow.h"

#include "nsWrapperCache.h"
#include "mozilla/dom/TypedArray.h"
#define NS_DOMCRYPTO_CID \
  {0x929d9320, 0x251e, 0x11d4, { 0x8a, 0x7c, 0x00, 0x60, 0x08, 0xc8, 0x44, 0xc3} }

namespace mozilla {

class ErrorResult;

namespace dom {

class Crypto : public nsIDOMCrypto,
               public nsWrapperCache
{
protected:
  virtual ~Crypto();

public:
  Crypto();

  NS_DECL_NSIDOMCRYPTO

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Crypto)

  void
  GetRandomValues(JSContext* aCx, const ArrayBufferView& aArray,
		  JS::MutableHandle<JSObject*> aRetval,
		  ErrorResult& aRv);

  SubtleCrypto*
  Subtle();

  

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static uint8_t*
  GetRandomValues(uint32_t aLength);

private:
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<SubtleCrypto> mSubtle;
};

} 
} 

#endif 
