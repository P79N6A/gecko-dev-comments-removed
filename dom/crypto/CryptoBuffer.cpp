





#include "CryptoBuffer.h"
#include "secitem.h"
#include "mozilla/Base64.h"
#include "mozilla/dom/UnionTypes.h"

namespace mozilla {
namespace dom {

uint8_t*
CryptoBuffer::Assign(const CryptoBuffer& aData)
{
  
  
  return ReplaceElementsAt(0, Length(), aData.Elements(), aData.Length());
}

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





nsresult
CryptoBuffer::FromJwkBase64(const nsString& aBase64)
{
  NS_ConvertUTF16toUTF8 temp(aBase64);
  temp.StripWhitespace();

  
  if (temp.Length() % 4 == 3) {
    temp.AppendLiteral("=");
  } else if (temp.Length() % 4 == 2) {
    temp.AppendLiteral("==");
  } if (temp.Length() % 4 == 1) {
    return NS_ERROR_FAILURE; 
  }

  
  temp.ReplaceChar('-', '+');
  temp.ReplaceChar('_', '/');

  
  nsCString binaryData;
  nsresult rv = Base64Decode(temp, binaryData);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!Assign((const uint8_t*) binaryData.BeginReading(),
              binaryData.Length())) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
CryptoBuffer::ToJwkBase64(nsString& aBase64)
{
  
  if (Length() == 0) {
    aBase64.Truncate();
    return NS_OK;
  }

  
  nsCString base64;
  nsDependentCSubstring binaryData((const char*) Elements(),
                                   (const char*) (Elements() + Length()));
  nsresult rv = Base64Encode(binaryData, base64);
  NS_ENSURE_SUCCESS(rv, rv);

  
  base64.Trim("=");

  
  base64.ReplaceChar('+', '-');
  base64.ReplaceChar('/', '_');
  if (base64.FindCharInSet("+/", 0) != kNotFound) {
    return NS_ERROR_FAILURE;
  }

  CopyASCIItoUTF16(base64, aBase64);
  return NS_OK;
}

SECItem*
CryptoBuffer::ToSECItem() const
{
  uint8_t* data = (uint8_t*) moz_malloc(Length());
  if (!data) {
    return nullptr;
  }

  SECItem* item = ::SECITEM_AllocItem(nullptr, nullptr, 0);
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
