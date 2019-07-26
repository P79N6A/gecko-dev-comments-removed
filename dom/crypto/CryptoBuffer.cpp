





#include "CryptoBuffer.h"
#include "mozilla/dom/UnionTypes.h"

namespace mozilla {
namespace dom {

uint8_t*
CryptoBuffer::Assign(const uint8_t* aData, uint32_t aLength)
{
  return ReplaceElementsAt(0, Length(), aData, aLength);
}

uint8_t*
CryptoBuffer::Assign(const SECItem* aItem)
{
  MOZ_ASSERT(aItem);
  return Assign(aItem->data, aItem->len);
}

uint8_t*
CryptoBuffer::Assign(const ArrayBuffer& aData)
{
  aData.ComputeLengthAndData();
  return Assign(aData.Data(), aData.Length());
}

uint8_t*
CryptoBuffer::Assign(const ArrayBufferView& aData)
{
  aData.ComputeLengthAndData();
  return Assign(aData.Data(), aData.Length());
}

uint8_t*
CryptoBuffer::Assign(const ArrayBufferViewOrArrayBuffer& aData)
{
  if (aData.IsArrayBufferView()) {
    return Assign(aData.GetAsArrayBufferView());
  } else if (aData.IsArrayBuffer()) {
    return Assign(aData.GetAsArrayBuffer());
  }

  
  MOZ_ASSERT(false);
  SetLength(0);
  return nullptr;
}

uint8_t*
CryptoBuffer::Assign(const OwningArrayBufferViewOrArrayBuffer& aData)
{
  if (aData.IsArrayBufferView()) {
    return Assign(aData.GetAsArrayBufferView());
  } else if (aData.IsArrayBuffer()) {
    return Assign(aData.GetAsArrayBuffer());
  }

  
  MOZ_ASSERT(false);
  SetLength(0);
  return nullptr;
}

SECItem*
CryptoBuffer::ToSECItem()
{
  uint8_t* data = (uint8_t*) moz_malloc(Length());
  if (!data) {
    return nullptr;
  }

  SECItem* item = new SECItem();
  item->type = siBuffer;
  item->data = data;
  item->len = Length();

  memcpy(item->data, Elements(), Length());

  return item;
}




bool
CryptoBuffer::GetBigIntValue(unsigned long& aRetVal)
{
  if (Length() > sizeof(aRetVal)) {
    return false;
  }

  aRetVal = 0;
  for (size_t i=0; i < Length(); ++i) {
    aRetVal = (aRetVal << 8) + ElementAt(i);
  }
  return true;
}

} 
} 
