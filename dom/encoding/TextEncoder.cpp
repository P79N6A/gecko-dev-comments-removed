



#include "mozilla/dom/TextEncoder.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

void
TextEncoder::Init(const nsAString& aEncoding, ErrorResult& aRv)
{
  nsAutoString label(aEncoding);
  EncodingUtils::TrimSpaceCharacters(label);

  
  
  
  if (!EncodingUtils::FindEncodingForLabel(label, mEncoding)) {
    aRv.ThrowTypeError(MSG_ENCODING_NOT_SUPPORTED, &label);
    return;
  }

  if (!mEncoding.EqualsLiteral("UTF-8") &&
      !mEncoding.EqualsLiteral("UTF-16LE") &&
      !mEncoding.EqualsLiteral("UTF-16BE")) {
    aRv.ThrowTypeError(MSG_DOM_ENCODING_NOT_UTF);
    return;
  }

  
  mEncoder = EncodingUtils::EncoderForEncoding(mEncoding);
}

void
TextEncoder::Encode(JSContext* aCx,
                    JS::Handle<JSObject*> aObj,
                    const nsAString& aString,
                    const bool aStream,
		    JS::MutableHandle<JSObject*> aRetval,
                    ErrorResult& aRv)
{
  
  int32_t srcLen = aString.Length();
  int32_t maxLen;
  const char16_t* data = PromiseFlatString(aString).get();
  nsresult rv = mEncoder->GetMaxLength(data, srcLen, &maxLen);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }
  
  
  static const fallible_t fallible = fallible_t();
  nsAutoArrayPtr<char> buf(new (fallible) char[maxLen + 1]);
  if (!buf) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  int32_t dstLen = maxLen;
  rv = mEncoder->Convert(data, &srcLen, buf, &dstLen);

  
  
  if (!aStream) {
    int32_t finishLen = maxLen - dstLen;
    rv = mEncoder->Finish(buf + dstLen, &finishLen);
    if (NS_SUCCEEDED(rv)) {
      dstLen += finishLen;
    }
  }

  JSObject* outView = nullptr;
  if (NS_SUCCEEDED(rv)) {
    buf[dstLen] = '\0';
    JSAutoCompartment ac(aCx, aObj);
    outView = Uint8Array::Create(aCx, dstLen,
                                 reinterpret_cast<uint8_t*>(buf.get()));
    if (!outView) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return;
    }
  }

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
  aRetval.set(outView);
}

void
TextEncoder::GetEncoding(nsAString& aEncoding)
{
  CopyASCIItoUTF16(mEncoding, aEncoding);
  nsContentUtils::ASCIIToLower(aEncoding);
}

} 
} 
