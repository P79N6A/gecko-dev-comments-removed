




































#include "nsString.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharsetConverterManager.h"
#include "nsReadableUtils.h"
#include "nsITextToSubURI.h"
#include "nsIServiceManager.h"
#include "nsUConvDll.h"
#include "nsEscape.h"
#include "prmem.h"
#include "nsTextToSubURI.h"
#include "nsCRT.h"

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

nsTextToSubURI::nsTextToSubURI()
{
}
nsTextToSubURI::~nsTextToSubURI()
{
}

NS_IMPL_ISUPPORTS1(nsTextToSubURI, nsITextToSubURI)

NS_IMETHODIMP  nsTextToSubURI::ConvertAndEscape(
  const char *charset, const PRUnichar *text, char **_retval) 
{
  if(nsnull == _retval)
    return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  nsresult rv = NS_OK;
  
  
  nsICharsetConverterManager *ccm;
  rv = CallGetService(kCharsetConverterManagerCID, &ccm);
  if(NS_SUCCEEDED(rv)) {
     nsIUnicodeEncoder *encoder;
     rv = ccm->GetUnicodeEncoder(charset, &encoder);
     NS_RELEASE(ccm);
     if (NS_SUCCEEDED(rv)) {
       rv = encoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
       if(NS_SUCCEEDED(rv))
       {
          char buf[256];
          char *pBuf = buf;
          PRInt32 ulen = nsCRT::strlen(text);
          PRInt32 outlen = 0;
          if(NS_SUCCEEDED(rv = encoder->GetMaxLength(text, ulen, &outlen))) 
          {
             if(outlen >= 256) {
                pBuf = (char*)NS_Alloc(outlen+1);
             }
             if(nsnull == pBuf) {
                outlen = 255;
                pBuf = buf;
             }
             PRInt32 bufLen = outlen;
             if(NS_SUCCEEDED(rv = encoder->Convert(text,&ulen, pBuf, &outlen))) {
                
                PRInt32 finLen = bufLen - outlen;
                if (finLen > 0) {
                  if (NS_SUCCEEDED(encoder->Finish((char *)(pBuf+outlen), &finLen)))
                    outlen += finLen;
                }
                pBuf[outlen] = '\0';
                *_retval = nsEscape(pBuf, url_XPAlphas);
                if(nsnull == *_retval)
                  rv = NS_ERROR_OUT_OF_MEMORY;
             }
          }
          if(pBuf != buf)
             NS_Free(pBuf);
       }
       NS_RELEASE(encoder);
     }
  }
  
  return rv;
}

NS_IMETHODIMP  nsTextToSubURI::UnEscapeAndConvert(
  const char *charset, const char *text, PRUnichar **_retval) 
{
  if(nsnull == _retval)
    return NS_ERROR_NULL_POINTER;
  if(nsnull == text) {
    
    
    text = "";
  }
  *_retval = nsnull;
  nsresult rv = NS_OK;
  
  
  char *unescaped = NS_strdup(text);
  if (nsnull == unescaped)
    return NS_ERROR_OUT_OF_MEMORY;
  unescaped = nsUnescape(unescaped);
  NS_ASSERTION(unescaped, "nsUnescape returned null");

  
  nsCOMPtr<nsICharsetConverterManager> ccm = 
           do_GetService(kCharsetConverterManagerCID, &rv); 
  if (NS_SUCCEEDED(rv)) {
    nsIUnicodeDecoder *decoder;
    rv = ccm->GetUnicodeDecoder(charset, &decoder);
    if (NS_SUCCEEDED(rv)) {
      PRUnichar *pBuf = nsnull;
      PRInt32 len = strlen(unescaped);
      PRInt32 outlen = 0;
      if (NS_SUCCEEDED(rv = decoder->GetMaxLength(unescaped, len, &outlen))) {
        pBuf = (PRUnichar *) NS_Alloc((outlen+1)*sizeof(PRUnichar*));
        if (nsnull == pBuf)
          rv = NS_ERROR_OUT_OF_MEMORY;
        else {
          if (NS_SUCCEEDED(rv = decoder->Convert(unescaped, &len, pBuf, &outlen))) {
            pBuf[outlen] = 0;
            *_retval = pBuf;
          }
          else
            NS_Free(pBuf);
        }
      }
      NS_RELEASE(decoder);
    }
  }
  NS_Free(unescaped);

  return rv;
}

static PRBool statefulCharset(const char *charset)
{
  if (!nsCRT::strncasecmp(charset, "ISO-2022-", sizeof("ISO-2022-")-1) ||
      !nsCRT::strcasecmp(charset, "UTF-7") ||
      !nsCRT::strcasecmp(charset, "HZ-GB-2312"))
    return PR_TRUE;

  return PR_FALSE;
}

nsresult nsTextToSubURI::convertURItoUnicode(const nsAFlatCString &aCharset,
                                             const nsAFlatCString &aURI, 
                                             PRBool aIRI, 
                                             nsAString &_retval)
{
  nsresult rv = NS_OK;

  
  PRBool isStatefulCharset = statefulCharset(aCharset.get());

  if (!isStatefulCharset && IsASCII(aURI)) {
    CopyASCIItoUTF16(aURI, _retval);
    return rv;
  }

  if (!isStatefulCharset && aIRI) {
    if (IsUTF8(aURI)) {
      CopyUTF8toUTF16(aURI, _retval);
      return rv;
    }
  }

  
  NS_ENSURE_FALSE(aCharset.IsEmpty(), NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsICharsetConverterManager> charsetConverterManager;

  charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;
  rv = charsetConverterManager->GetUnicodeDecoder(aCharset.get(), 
                                                  getter_AddRefs(unicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 srcLen = aURI.Length();
  PRInt32 dstLen;
  rv = unicodeDecoder->GetMaxLength(aURI.get(), srcLen, &dstLen);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar *ustr = (PRUnichar *) NS_Alloc(dstLen * sizeof(PRUnichar));
  NS_ENSURE_TRUE(ustr, NS_ERROR_OUT_OF_MEMORY);

  rv = unicodeDecoder->Convert(aURI.get(), &srcLen, ustr, &dstLen);

  if (NS_SUCCEEDED(rv))
    _retval.Assign(ustr, dstLen);
  
  NS_Free(ustr);

  return rv;
}

NS_IMETHODIMP  nsTextToSubURI::UnEscapeURIForUI(const nsACString & aCharset, 
                                                const nsACString &aURIFragment, 
                                                nsAString &_retval)
{
  nsCAutoString unescapedSpec;
  
  NS_UnescapeURL(PromiseFlatCString(aURIFragment), 
                 esc_SkipControl | esc_AlwaysCopy, unescapedSpec);

  
  
  
  if (convertURItoUnicode(
                PromiseFlatCString(aCharset), unescapedSpec, PR_TRUE, _retval)
      != NS_OK)
    
    CopyUTF8toUTF16(aURIFragment, _retval); 
  return NS_OK;
}

NS_IMETHODIMP  nsTextToSubURI::UnEscapeNonAsciiURI(const nsACString & aCharset, 
                                                   const nsACString &aURIFragment, 
                                                   nsAString &_retval)
{
  nsCAutoString unescapedSpec;
  NS_UnescapeURL(PromiseFlatCString(aURIFragment),
                 esc_AlwaysCopy | esc_OnlyNonASCII, unescapedSpec);

  return convertURItoUnicode(PromiseFlatCString(aCharset), unescapedSpec, PR_TRUE, _retval);
}


