





#ifndef mozilla_dom_CryptoBuffer_h
#define mozilla_dom_CryptoBuffer_h

#include "nsTArray.h"
#include "seccomon.h"
#include "mozilla/dom/TypedArray.h"

namespace mozilla {
namespace dom {

class ArrayBufferViewOrArrayBuffer;
class OwningArrayBufferViewOrArrayBuffer;

class CryptoBuffer : public FallibleTArray<uint8_t>
{
public:
  uint8_t* Assign(const uint8_t* aData, uint32_t aLength);
  uint8_t* Assign(const SECItem* aItem);
  uint8_t* Assign(const ArrayBuffer& aData);
  uint8_t* Assign(const ArrayBufferView& aData);
  uint8_t* Assign(const ArrayBufferViewOrArrayBuffer& aData);
  uint8_t* Assign(const OwningArrayBufferViewOrArrayBuffer& aData);

  template<typename T,
         JSObject* UnboxArray(JSObject*, uint32_t*, T**)>
  uint8_t* Assign(const TypedArray_base<T, UnboxArray>& aData)
  {
    return Assign(aData.Data(), aData.Length());
  }


  SECItem* ToSECItem();

  bool GetBigIntValue(unsigned long& aRetVal);
};

} 
} 

#endif 
