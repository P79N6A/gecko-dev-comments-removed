



#include "mozilla/dom/TextDecoder.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsContentUtils.h"
#include "nsICharsetConverterManager.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {

static const PRUnichar kReplacementChar = static_cast<PRUnichar>(0xFFFD);

void
TextDecoderBase::Init(const nsAString& aEncoding, const bool aFatal,
                      ErrorResult& aRv)
{
  nsAutoString label(aEncoding);
  EncodingUtils::TrimSpaceCharacters(label);

  
  
  if (!EncodingUtils::FindEncodingForLabel(label, mEncoding)) {
    aRv.ThrowTypeError(MSG_ENCODING_NOT_SUPPORTED, &label);
    return;
  }

  
  
  
  mFatal = aFatal;

  
  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID);
  if (!ccm) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  ccm->GetUnicodeDecoderRaw(mEncoding.get(), getter_AddRefs(mDecoder));
  if (!mDecoder) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  if (mFatal) {
    mDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);
  }
}

void
TextDecoderBase::Decode(const ArrayBufferView* aView,
                        const bool aStream,
                        nsAString& aOutDecodedString,
                        ErrorResult& aRv)
{
  const char* data;
  int32_t length;
  
  if (!aView) {
    data = EmptyCString().BeginReading();
    length = EmptyCString().Length();
  } else {
    data = reinterpret_cast<const char*>(aView->Data());
    length = aView->Length();
  }

  aOutDecodedString.Truncate();

  
  int32_t outLen;
  nsresult rv = mDecoder->GetMaxLength(data, length, &outLen);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return;
  }
  
  
  static const fallible_t fallible = fallible_t();
  nsAutoArrayPtr<PRUnichar> buf(new (fallible) PRUnichar[outLen + 1]);
  if (!buf) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  rv = mDecoder->Convert(data, &length, buf, &outLen);
  MOZ_ASSERT(mFatal || rv != NS_ERROR_ILLEGAL_INPUT);
  buf[outLen] = 0;
  aOutDecodedString.Append(buf, outLen);

  
  
  if (!aStream) {
    mDecoder->Reset();
    if (rv == NS_OK_UDEC_MOREINPUT) {
      if (mFatal) {
        aRv.Throw(NS_ERROR_DOM_ENCODING_DECODE_ERR);
      } else {
        
        
        aOutDecodedString.Append(kReplacementChar);
      }
    }
  }

  if (NS_FAILED(rv)) {
    aRv.Throw(NS_ERROR_DOM_ENCODING_DECODE_ERR);
  }
}

void
TextDecoderBase::GetEncoding(nsAString& aEncoding)
{
  CopyASCIItoUTF16(mEncoding, aEncoding);
  nsContentUtils::ASCIIToLower(aEncoding);
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(TextDecoder)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TextDecoder)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TextDecoder)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(TextDecoder, mGlobal)

} 
} 
