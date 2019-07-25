





































#include "nsString.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharsetConverterManager.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "prmem.h"
#include "nsUTF8ConverterService.h"
#include "nsEscape.h"
#include "nsAutoPtr.h"

NS_IMPL_ISUPPORTS1(nsUTF8ConverterService, nsIUTF8ConverterService)

static nsresult 
ToUTF8(const nsACString &aString, const char *aCharset, nsACString &aResult)
{
  nsresult rv;
  if (!aCharset || !*aCharset)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsICharsetConverterManager> ccm;

  ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;
  rv = ccm->GetUnicodeDecoder(aCharset,
                              getter_AddRefs(unicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 srcLen = aString.Length();
  PRInt32 dstLen;
  const nsAFlatCString& inStr = PromiseFlatCString(aString);
  rv = unicodeDecoder->GetMaxLength(inStr.get(), srcLen, &dstLen);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoArrayPtr<PRUnichar> ustr(new PRUnichar[dstLen]);
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
                                            PRBool aSkipCheck, 
                                            nsACString &aUTF8String)
{
  
  
  
  
  if (!aSkipCheck && (IsASCII(aString) || IsUTF8(aString))) {
    aUTF8String = aString;
    return NS_OK;
  }

  aUTF8String.Truncate();

  nsresult rv = ToUTF8(aString, aCharset, aUTF8String);

  
  
  
  
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

  nsCAutoString unescapedSpec; 
  
  
  PRBool written = NS_UnescapeURL(PromiseFlatCString(aSpec).get(), aSpec.Length(), 
                                  esc_OnlyNonASCII, unescapedSpec);

  if (!written) {
    aUTF8Spec = aSpec;
    return NS_OK;
  }
  
  if (IsASCII(unescapedSpec) || IsUTF8(unescapedSpec)) {
    aUTF8Spec = unescapedSpec;
    return NS_OK;
  }

  return ToUTF8(unescapedSpec, aCharset, aUTF8Spec);
}

