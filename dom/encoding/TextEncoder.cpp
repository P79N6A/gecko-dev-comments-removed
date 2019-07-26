



#include "mozilla/dom/TextEncoder.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsContentUtils.h"
#include "nsICharsetConverterManager.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

void
TextEncoder::Init(const Optional<nsAString>& aEncoding,
                  ErrorResult& aRv)
{
  
  
  nsAutoString label;
  if (!aEncoding.WasPassed()) {
    label.AssignLiteral("utf-8");
  } else {
    label.Assign(aEncoding.Value());
    EncodingUtils::TrimSpaceCharacters(label);
  }

  
  if (!EncodingUtils::FindEncodingForLabel(label, mEncoding)) {
    
    
    aRv.Throw(NS_ERROR_DOM_ENCODING_NOT_SUPPORTED_ERR);
    return;
  }

  
  
  
  if (PL_strcasecmp(mEncoding, "utf-8") &&
      PL_strcasecmp(mEncoding, "utf-16le") &&
      PL_strcasecmp(mEncoding, "utf-16be")) {
    aRv.Throw(NS_ERROR_DOM_ENCODING_NOT_UTF_ERR);
    return;
  }

  
  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID);
  if (!ccm) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  ccm->GetUnicodeEncoder(mEncoding, getter_AddRefs(mEncoder));
  if (!mEncoder) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }
}

JSObject*
TextEncoder::Encode(JSContext* aCx,
                    const nsAString& aString,
                    const TextEncodeOptions& aOptions,
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

  
  
  if (!aOptions.stream) {
    int32_t finishLen = maxLen - dstLen;
    rv = mEncoder->Finish(buf + dstLen, &finishLen);
    if (NS_SUCCEEDED(rv)) {
      dstLen += finishLen;
    }
  }

  JSObject* outView = nullptr;
  if (NS_SUCCEEDED(rv)) {
    buf[dstLen] = '\0';
    outView = Uint8Array::Create(aCx, this, dstLen,
                                 reinterpret_cast<uint8_t*>(buf.get()));
  }

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
  return outView;
}

void
TextEncoder::GetEncoding(nsAString& aEncoding)
{
  
  
  
  
  
  if (!strcmp(mEncoding, "utf-16le")) {
    aEncoding.AssignLiteral("utf-16");
    return;
  }
  aEncoding.AssignASCII(mEncoding);
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(TextEncoder)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TextEncoder)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TextEncoder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(TextEncoder, mGlobal)

} 
} 
