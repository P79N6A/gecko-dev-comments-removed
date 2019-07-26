



#include "mozilla/dom/TextEncoder.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsContentUtils.h"
#include "nsICharsetConverterManager.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

void
TextEncoderBase::Init(const nsAString& aEncoding, ErrorResult& aRv)
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

  
  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID);
  if (!ccm) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  ccm->GetUnicodeEncoderRaw(mEncoding.get(), getter_AddRefs(mEncoder));
  if (!mEncoder) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }
}

JSObject*
TextEncoderBase::Encode(JSContext* aCx,
                        const nsAString& aString,
                        const bool aStream,
                        ErrorResult& aRv)
{
  
  int32_t srcLen = aString.Length();
  int32_t maxLen;
  const PRUnichar* data = PromiseFlatString(aString).get();
  nsresult rv = mEncoder->GetMaxLength(data, srcLen, &maxLen);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }
  
  
  static const fallible_t fallible = fallible_t();
  nsAutoArrayPtr<char> buf(new (fallible) char[maxLen + 1]);
  if (!buf) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
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
    outView = CreateUint8Array(aCx, buf, dstLen);
    if (!outView) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }
  }

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
  return outView;
}

void
TextEncoderBase::GetEncoding(nsAString& aEncoding)
{
  CopyASCIItoUTF16(mEncoding, aEncoding);
  nsContentUtils::ASCIIToLower(aEncoding);
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(TextEncoder, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(TextEncoder, Release)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(TextEncoder, mGlobal)

} 
} 
