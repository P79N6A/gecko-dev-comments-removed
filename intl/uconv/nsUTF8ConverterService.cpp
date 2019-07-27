





#include "nsString.h"
#include "nsUTF8ConverterService.h"
#include "nsEscape.h"
#include "nsAutoPtr.h"
#include "nsIUnicodeDecoder.h"
#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;

NS_IMPL_ISUPPORTS(nsUTF8ConverterService, nsIUTF8ConverterService)

static nsresult 
ToUTF8(const nsACString &aString, const char *aCharset,
       bool aAllowSubstitution, nsACString &aResult)
{
  nsresult rv;
  if (!aCharset || !*aCharset)
    return NS_ERROR_INVALID_ARG;

  nsDependentCString label(aCharset);
  nsAutoCString encoding;
  if (!EncodingUtils::FindEncodingForLabelNoReplacement(label, encoding)) {
    return NS_ERROR_UCONV_NOCONV;
  }
  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder =
    EncodingUtils::DecoderForEncoding(encoding);

  if (!aAllowSubstitution)
    unicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);

  int32_t srcLen = aString.Length();
  int32_t dstLen;
  const nsAFlatCString& inStr = PromiseFlatCString(aString);
  rv = unicodeDecoder->GetMaxLength(inStr.get(), srcLen, &dstLen);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoArrayPtr<char16_t> ustr(new char16_t[dstLen]);
  NS_ENSURE_TRUE(ustr, NS_ERROR_OUT_OF_MEMORY);

  rv = unicodeDecoder->Convert(inStr.get(), &srcLen, ustr, &dstLen);
  if (NS_SUCCEEDED(rv)){
    
    CopyUTF16toUTF8(Substring(ustr.get(), ustr + dstLen), aResult);
  }
  return rv;
}

NS_IMETHODIMP  
nsUTF8ConverterService::ConvertStringToUTF8(const nsACString &aString, 
                                            const char *aCharset, 
                                            bool aSkipCheck, 
                                            bool aAllowSubstitution,
                                            uint8_t aOptionalArgc,
                                            nsACString &aUTF8String)
{
  bool allowSubstitution = (aOptionalArgc == 1) ? aAllowSubstitution : true;

  
  
  
  
  if (!aSkipCheck && (IsASCII(aString) || IsUTF8(aString))) {
    aUTF8String = aString;
    return NS_OK;
  }

  aUTF8String.Truncate();

  nsresult rv = ToUTF8(aString, aCharset, allowSubstitution, aUTF8String);

  
  
  
  
  if (aSkipCheck && NS_FAILED(rv) && IsUTF8(aString)) {
    aUTF8String = aString;
    return NS_OK;
  }

  return rv;
}

NS_IMETHODIMP  
nsUTF8ConverterService::ConvertURISpecToUTF8(const nsACString &aSpec, 
                                             const char *aCharset, 
                                             nsACString &aUTF8Spec)
{
  
  
  if (!IsASCII(aSpec)) {
    aUTF8Spec = aSpec;
    return NS_OK;
  }

  aUTF8Spec.Truncate();

  nsAutoCString unescapedSpec; 
  
  
  bool written = NS_UnescapeURL(PromiseFlatCString(aSpec).get(), aSpec.Length(), 
                                  esc_OnlyNonASCII, unescapedSpec);

  if (!written) {
    aUTF8Spec = aSpec;
    return NS_OK;
  }
  
  if (IsASCII(unescapedSpec) || IsUTF8(unescapedSpec)) {
    aUTF8Spec = unescapedSpec;
    return NS_OK;
  }

  return ToUTF8(unescapedSpec, aCharset, true, aUTF8Spec);
}

