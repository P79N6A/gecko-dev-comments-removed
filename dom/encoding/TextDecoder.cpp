




#include "mozilla/dom/TextDecoder.h"
#include "mozilla/dom/EncodingUtils.h"
#include "mozilla/dom/UnionTypes.h"
#include "nsContentUtils.h"
#include <stdint.h>

namespace mozilla {
namespace dom {

static const char16_t kReplacementChar = static_cast<char16_t>(0xFFFD);

void
TextDecoder::Init(const nsAString& aLabel, const bool aFatal,
                  ErrorResult& aRv)
{
  nsAutoCString encoding;
  
  
  
  if (!EncodingUtils::FindEncodingForLabelNoReplacement(aLabel, encoding)) {
    nsAutoString label(aLabel);
    EncodingUtils::TrimSpaceCharacters(label);
    aRv.ThrowRangeError(MSG_ENCODING_NOT_SUPPORTED, &label);
    return;
  }
  InitWithEncoding(encoding, aFatal);
}

void
TextDecoder::InitWithEncoding(const nsACString& aEncoding, const bool aFatal)
{
  mEncoding = aEncoding;
  
  
  
  mFatal = aFatal;

  
  mDecoder = EncodingUtils::DecoderForEncoding(mEncoding);

  if (mFatal) {
    mDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);
  }
}

void
TextDecoder::Decode(const char* aInput, const int32_t aLength,
                    const bool aStream, nsAString& aOutDecodedString,
                    ErrorResult& aRv)
{
  aOutDecodedString.Truncate();

  
  int32_t outLen;
  nsresult rv = mDecoder->GetMaxLength(aInput, aLength, &outLen);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }
  
  
  nsAutoArrayPtr<char16_t> buf(new (fallible) char16_t[outLen + 1]);
  if (!buf) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  int32_t length = aLength;
  rv = mDecoder->Convert(aInput, &length, buf, &outLen);
  MOZ_ASSERT(mFatal || rv != NS_ERROR_ILLEGAL_INPUT);
  buf[outLen] = 0;
  aOutDecodedString.Append(buf, outLen);

  
  
  if (!aStream) {
    mDecoder->Reset();
    if (rv == NS_OK_UDEC_MOREINPUT) {
      if (mFatal) {
        aRv.ThrowTypeError(MSG_DOM_DECODING_FAILED);
      } else {
        
        
        aOutDecodedString.Append(kReplacementChar);
      }
    }
  }

  if (NS_FAILED(rv)) {
    aRv.ThrowTypeError(MSG_DOM_DECODING_FAILED);
  }
}

void
TextDecoder::Decode(const Optional<ArrayBufferViewOrArrayBuffer>& aBuffer,
                    const TextDecodeOptions& aOptions,
                    nsAString& aOutDecodedString,
                    ErrorResult& aRv)
{
  if (!aBuffer.WasPassed()) {
    Decode(nullptr, 0, aOptions.mStream, aOutDecodedString, aRv);
    return;
  }
  const ArrayBufferViewOrArrayBuffer& buf = aBuffer.Value();
  uint8_t* data;
  uint32_t length;
  if (buf.IsArrayBufferView()) {
    buf.GetAsArrayBufferView().ComputeLengthAndData();
    data = buf.GetAsArrayBufferView().Data();
    length = buf.GetAsArrayBufferView().Length();
  } else {
    MOZ_ASSERT(buf.IsArrayBuffer());
    buf.GetAsArrayBuffer().ComputeLengthAndData();
    data = buf.GetAsArrayBuffer().Data();
    length = buf.GetAsArrayBuffer().Length();
  }
  
  
  
  if (length > INT32_MAX) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }
  Decode(reinterpret_cast<char*>(data), length, aOptions.mStream,
         aOutDecodedString, aRv);
}

void
TextDecoder::GetEncoding(nsAString& aEncoding)
{
  CopyASCIItoUTF16(mEncoding, aEncoding);
  nsContentUtils::ASCIIToLower(aEncoding);
}

} 
} 
