










































#include <string.h>
#include "prtypes.h"
#include "prmem.h"
#include "prprf.h"
#include "plstr.h"
#include "plbase64.h"
#include "nsCRT.h"
#include "nsMemory.h"
#include "nsCOMPtr.h"
#include "nsEscape.h"
#include "nsIUTF8ConverterService.h"
#include "nsUConvCID.h"
#include "nsIServiceManager.h"
#include "nsMIMEHeaderParamImpl.h"
#include "nsReadableUtils.h"
#include "nsNativeCharsetUtils.h"


  
static char *DecodeQ(const char *, PRUint32);
static PRBool Is7bitNonAsciiString(const char *, PRUint32);
static void CopyRawHeader(const char *, PRUint32, const char *, nsACString &);
static nsresult DecodeRFC2047Str(const char *, const char *, PRBool, nsACString&);



#define IS_7BIT_NON_ASCII_CHARSET(cset)            \
    (!nsCRT::strncasecmp((cset), "ISO-2022", 8) || \
     !nsCRT::strncasecmp((cset), "HZ-GB", 5)    || \
     !nsCRT::strncasecmp((cset), "UTF-7", 5))   

NS_IMPL_ISUPPORTS1(nsMIMEHeaderParamImpl, nsIMIMEHeaderParam)


NS_IMETHODIMP 
nsMIMEHeaderParamImpl::GetParameter(const nsACString& aHeaderVal, 
                                    const char *aParamName,
                                    const nsACString& aFallbackCharset, 
                                    PRBool aTryLocaleCharset, 
                                    char **aLang, nsAString& aResult)
{
    aResult.Truncate();
    nsresult rv;

    
    
    nsXPIDLCString med;
    nsXPIDLCString charset;
    rv = GetParameterInternal(PromiseFlatCString(aHeaderVal).get(), aParamName, 
                              getter_Copies(charset), aLang, getter_Copies(med));
    if (NS_FAILED(rv))
        return rv; 

    
    
    
    nsCAutoString str1;
    rv = DecodeParameter(med, charset.get(), nsnull, PR_FALSE, str1);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!aFallbackCharset.IsEmpty())
    {
        nsCAutoString str2;
        nsCOMPtr<nsIUTF8ConverterService> 
          cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
        if (cvtUTF8 &&
            NS_SUCCEEDED(cvtUTF8->ConvertStringToUTF8(str1, 
                PromiseFlatCString(aFallbackCharset).get(), PR_FALSE, str2))) {
          CopyUTF8toUTF16(str2, aResult);
          return NS_OK;
        }
    }

    if (IsUTF8(str1)) {
      CopyUTF8toUTF16(str1, aResult);
      return NS_OK;
    }

    if (aTryLocaleCharset && !NS_IsNativeUTF8()) 
      return NS_CopyNativeToUnicode(str1, aResult);

    CopyASCIItoUTF16(str1, aResult);
    return NS_OK;
}



void RemoveQuotedStringEscapes(char *src)
{
  char *dst = src;

  for (char *c = src; *c; c += 1)
  {
    if (c[0] == '\\' && c[1])
    {
      
      ++c;
    }
    *dst++ = *c;
  }
  *dst = 0;
}








NS_IMETHODIMP 
nsMIMEHeaderParamImpl::GetParameterInternal(const char *aHeaderValue, 
                                            const char *aParamName,
                                            char **aCharset,
                                            char **aLang,
                                            char **aResult)
{
  if (!aHeaderValue ||  !*aHeaderValue || !aResult)
    return NS_ERROR_INVALID_ARG;

  *aResult = nsnull;

  if (aCharset) *aCharset = nsnull;
  if (aLang) *aLang = nsnull;

  const char *str = aHeaderValue;

  
  for (; *str &&  nsCRT::IsAsciiSpace(*str); ++str)
    ;
  const char *start = str;
  
  
  
  
  if (!aParamName || !*aParamName) 
    {
      for (; *str && *str != ';' && !nsCRT::IsAsciiSpace(*str); ++str)
        ;
      if (str == start)
        return NS_ERROR_UNEXPECTED;
      *aResult = (char *) nsMemory::Clone(start, (str - start) + 1);
      NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);
      (*aResult)[str - start] = '\0';  
      return NS_OK;
    }

  
  for (; *str && *str != ';' && *str != ','; ++str)
    ;
  if (*str)
    str++;
  
  for (; *str && nsCRT::IsAsciiSpace(*str); ++str)
    ;

  
  
  
  
  
  if (!*str)
    str = start;

  
  
  
  
  
  
  
  

  PRInt32 paramLen = strlen(aParamName);

  while (*str) {
    const char *tokenStart = str;
    const char *tokenEnd = 0;
    const char *valueStart = str;
    const char *valueEnd = 0;

    NS_ASSERTION(!nsCRT::IsAsciiSpace(*str), "should be after whitespace.");

    
    for (; *str && !nsCRT::IsAsciiSpace(*str) && *str != '=' && *str != ';'; str++)
      ;
    tokenEnd = str;

    
    while (nsCRT::IsAsciiSpace(*str)) ++str;
    if (*str == '=') ++str;
    while (nsCRT::IsAsciiSpace(*str)) ++str;

    PRBool needUnquote = PR_FALSE;
    
    if (*str != '"')
    {
      
      valueStart = str;
      for (valueEnd = str;
           *valueEnd && !nsCRT::IsAsciiSpace (*valueEnd) && *valueEnd != ';';
           valueEnd++)
        ;
      str = valueEnd;
    }
    else
    {
      
      needUnquote = PR_TRUE;
      
      ++str;
      valueStart = str;
      for (valueEnd = str; *valueEnd; ++valueEnd)
      {
        if (*valueEnd == '\\')
          ++valueEnd;
        else if (*valueEnd == '"')
          break;
      }
      str = valueEnd + 1;
    }

    
    
    
    if (tokenEnd - tokenStart == paramLen &&
        !nsCRT::strncasecmp(tokenStart, aParamName, paramLen))
    {
      
      
      nsCAutoString tempStr(valueStart, valueEnd - valueStart);
      tempStr.StripChars("\r\n");
      char *res = ToNewCString(tempStr);
      NS_ENSURE_TRUE(*res, NS_ERROR_OUT_OF_MEMORY);
      
      if (needUnquote)
        RemoveQuotedStringEscapes(res);
            
      *aResult = res;
      
      
    }
    
    else if (tokenEnd - tokenStart > paramLen &&
             !nsCRT::strncasecmp(tokenStart, aParamName, paramLen) &&
             *(tokenStart + paramLen) == '*')
    {
      const char *cp = tokenStart + paramLen + 1; 
      PRBool needUnescape = *(tokenEnd - 1) == '*';
      
      
      if ((*cp == '0' && needUnescape) || (tokenEnd - tokenStart == paramLen + 1))
      {
        
        const char *sQuote1 = PL_strchr(valueStart, 0x27);
        const char *sQuote2 = (char *) (sQuote1 ? PL_strchr(sQuote1 + 1, 0x27) : nsnull);

        
        
        if (!sQuote1 || !sQuote2)
          NS_WARNING("Mandatory two single quotes are missing in header parameter\n");
        if (aCharset && sQuote1 > valueStart && sQuote1 < valueEnd)
        {
          *aCharset = (char *) nsMemory::Clone(valueStart, sQuote1 - valueStart + 1);
          if (*aCharset) 
            *(*aCharset + (sQuote1 - valueStart)) = 0;
        }
        if (aLang && sQuote1 && sQuote2 && sQuote2 > sQuote1 + 1 &&
            sQuote2 < valueEnd)
        {
          *aLang = (char *) nsMemory::Clone(sQuote1 + 1, sQuote2 - (sQuote1 + 1) + 1);
          if (*aLang) 
            *(*aLang + (sQuote2 - (sQuote1 + 1))) = 0;
        }

        
        
        if (sQuote1)
        {
          if(!sQuote2)
            sQuote2 = sQuote1;
        }
        else
          sQuote2 = valueStart - 1;

        if (sQuote2 && sQuote2 + 1 < valueEnd)
        {
          if (*aResult)
          {
            
            
            nsMemory::Free(*aResult);
          }
          *aResult = (char *) nsMemory::Alloc(valueEnd - (sQuote2 + 1) + 1);
          if (*aResult)
          {
            memcpy(*aResult, sQuote2 + 1, valueEnd - (sQuote2 + 1));
            *(*aResult + (valueEnd - (sQuote2 + 1))) = 0;
            if (needUnescape)
            {
              nsUnescape(*aResult);
              if (tokenEnd - tokenStart == paramLen + 1)
                
                return NS_OK; 
            }
          }
        }
      }  
      
      
      else if (nsCRT::IsAsciiDigit(PRUnichar(*cp)))
      {
        PRInt32 len = 0;
        if (*aResult) 
        {
          len = strlen(*aResult);
          char *ns = (char *) nsMemory::Realloc(*aResult, len + (valueEnd - valueStart) + 1);
          if (!ns)
          {
            nsMemory::Free(*aResult);
          }
          *aResult = ns;
        }
        else if (*cp == '0') 
        {
          *aResult = (char *) nsMemory::Alloc(valueEnd - valueStart + 1);
        }
        
        if (*aResult)
        {
          
          memcpy(*aResult + len, valueStart, valueEnd - valueStart);
          *(*aResult + len + (valueEnd - valueStart)) = 0;
          if (needUnescape)
            nsUnescape(*aResult + len);
        }
        else 
          return NS_ERROR_OUT_OF_MEMORY;
      } 
    }

    
    
      
    while (nsCRT::IsAsciiSpace(*str)) ++str;
    if (*str == ';') ++str;
    while (nsCRT::IsAsciiSpace(*str)) ++str;
  }

  if (*aResult) 
    return NS_OK;
  else
    return NS_ERROR_INVALID_ARG; 
}


NS_IMETHODIMP
nsMIMEHeaderParamImpl::DecodeRFC2047Header(const char* aHeaderVal, 
                                           const char* aDefaultCharset, 
                                           PRBool aOverrideCharset, 
                                           PRBool aEatContinuations,
                                           nsACString& aResult)
{
  aResult.Truncate();
  if (!aHeaderVal)
    return NS_ERROR_INVALID_ARG;
  if (!*aHeaderVal)
    return NS_OK;


  
  
  
  if (PL_strstr(aHeaderVal, "=?") || 
      aDefaultCharset && (!IsUTF8(nsDependentCString(aHeaderVal)) || 
      Is7bitNonAsciiString(aHeaderVal, PL_strlen(aHeaderVal)))) {
    DecodeRFC2047Str(aHeaderVal, aDefaultCharset, aOverrideCharset, aResult);
  } else if (aEatContinuations && 
             (PL_strchr(aHeaderVal, '\n') || PL_strchr(aHeaderVal, '\r'))) {
    aResult = aHeaderVal;
  } else {
    aEatContinuations = PR_FALSE;
    aResult = aHeaderVal;
  }

  if (aEatContinuations) {
    nsCAutoString temp(aResult);
    temp.ReplaceSubstring("\n\t", " ");
    temp.ReplaceSubstring("\r\t", " ");
    temp.StripChars("\r\n");
    aResult = temp;
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsMIMEHeaderParamImpl::DecodeParameter(const nsACString& aParamValue,
                                       const char* aCharset,
                                       const char* aDefaultCharset,
                                       PRBool aOverrideCharset, 
                                       nsACString& aResult)
{
  aResult.Truncate();
  
  
  if (aCharset && *aCharset)
  {
    nsCOMPtr<nsIUTF8ConverterService> cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
    if (cvtUTF8)
      
      return cvtUTF8->ConvertStringToUTF8(aParamValue, aCharset,
          IS_7BIT_NON_ASCII_CHARSET(aCharset), aResult);
  }

  const nsAFlatCString& param = PromiseFlatCString(aParamValue);
  nsCAutoString unQuoted;
  nsACString::const_iterator s, e;
  param.BeginReading(s);
  param.EndReading(e);

  
  for ( ; s != e; ++s) {
    if ((*s == '\\')) {
      if (++s == e) {
        --s; 
      }
      else if (*s != nsCRT::CR && *s != nsCRT::LF && *s != '"' && *s != '\\') {
        --s; 
      }
      
    }
    unQuoted.Append(*s);
  }

  aResult = unQuoted;
  
  nsCAutoString decoded;

  
  nsresult rv = DecodeRFC2047Header(unQuoted.get(), aDefaultCharset, 
                                    aOverrideCharset, PR_TRUE, decoded);
  
  if (NS_SUCCEEDED(rv) && !decoded.IsEmpty())
    aResult = decoded;
  
  return rv;
}

#define ISHEXCHAR(c) \
        (0x30 <= PRUint8(c) && PRUint8(c) <= 0x39  ||  \
         0x41 <= PRUint8(c) && PRUint8(c) <= 0x46  ||  \
         0x61 <= PRUint8(c) && PRUint8(c) <= 0x66)



char *DecodeQ(const char *in, PRUint32 length)
{
  char *out, *dest = 0;

  out = dest = (char *)PR_Calloc(length + 1, sizeof(char));
  if (dest == nsnull)
    return nsnull;
  while (length > 0) {
    PRUintn c = 0;
    switch (*in) {
    case '=':
      
      if (length < 3 || !ISHEXCHAR(in[1]) || !ISHEXCHAR(in[2]))
        goto badsyntax;
      PR_sscanf(in + 1, "%2X", &c);
      *out++ = (char) c;
      in += 3;
      length -= 3;
      break;

    case '_':
      *out++ = ' ';
      in++;
      length--;
      break;

    default:
      if (*in & 0x80) goto badsyntax;
      *out++ = *in++;
      length--;
    }
  }
  *out++ = '\0';

  for (out = dest; *out ; ++out) {
    if (*out == '\t')
      *out = ' ';
  }

  return dest;

 badsyntax:
  PR_Free(dest);
  return nsnull;
}





PRBool Is7bitNonAsciiString(const char *input, PRUint32 len)
{
  PRInt32 c;

  enum { hz_initial, 
         hz_escaped, 
         hz_seen, 
         hz_notpresent 
  } hz_state;

  hz_state = hz_initial;
  while (len) {
    c = PRUint8(*input++);
    len--;
    if (c & 0x80) return PR_FALSE;
    if (c == 0x1B) return PR_TRUE;
    if (c == '~') {
      switch (hz_state) {
      case hz_initial:
      case hz_seen:
        if (*input == '{') {
          hz_state = hz_escaped;
        } else if (*input == '~') {
          
          hz_state = hz_seen;
          input++;
          len--;
        } else {
          hz_state = hz_notpresent;
        }
        break;

      case hz_escaped:
        if (*input == '}') hz_state = hz_seen;
        break;
      default:
        break;
      }
    }
  }
  return hz_state == hz_seen;
}

#define REPLACEMENT_CHAR "\357\277\275" // EF BF BD (UTF-8 encoding of U+FFFD)








void CopyRawHeader(const char *aInput, PRUint32 aLen, 
                   const char *aDefaultCharset, nsACString &aOutput)
{
  PRInt32 c;

  
  if (!aDefaultCharset || !*aDefaultCharset) {
    aOutput.Append(aInput, aLen);
    return;
  }

  
  
  while (aLen && (c = PRUint8(*aInput++)) != 0x1B && c != '~' && !(c & 0x80)) {
    aOutput.Append(char(c));
    aLen--;
  }
  if (!aLen) {
    return;
  }
  aInput--;

  
  
  PRBool skipCheck = (c == 0x1B || c == '~') && 
                     IS_7BIT_NON_ASCII_CHARSET(aDefaultCharset);

  
  nsCOMPtr<nsIUTF8ConverterService> 
    cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
  nsCAutoString utf8Text;
  if (cvtUTF8 &&
      NS_SUCCEEDED(
      cvtUTF8->ConvertStringToUTF8(Substring(aInput, aInput + aLen), 
      aDefaultCharset, skipCheck, utf8Text))) {
    aOutput.Append(utf8Text);
  } else { 
    for (PRUint32 i = 0; i < aLen; i++) {
      c = PRUint8(*aInput++);
      if (c & 0x80)
        aOutput.Append(REPLACEMENT_CHAR);
      else
        aOutput.Append(char(c));
    }
  }
}

static const char especials[] = "()<>@,;:\\\"/[]?.=";







nsresult DecodeRFC2047Str(const char *aHeader, const char *aDefaultCharset, 
                          PRBool aOverrideCharset, nsACString &aResult)
{
  const char *p, *q, *r;
  char *decodedText;
  const char *begin; 
  PRInt32 isLastEncodedWord = 0;
  const char *charsetStart, *charsetEnd;
  char charset[80];

  
  charset[0] = '\0';

  begin = aHeader;

  
  
  
  
  
  
  aResult.SetCapacity(3 * strlen(aHeader));

  while ((p = PL_strstr(begin, "=?")) != 0) {
    if (isLastEncodedWord) {
      
      for (q = begin; q < p; ++q) {
        if (!PL_strchr(" \t\r\n", *q)) break;
      }
    }

    if (!isLastEncodedWord || q < p) {
      
      CopyRawHeader(begin, p - begin, aDefaultCharset, aResult);
      begin = p;
    }

    p += 2;

    
    charsetStart = p;
    charsetEnd = 0;
    for (q = p; *q != '?'; q++) {
      if (*q <= ' ' || PL_strchr(especials, *q)) {
        goto badsyntax;
      }

      
      if (!charsetEnd && *q == '*') {
        charsetEnd = q; 
      }
    }
    if (!charsetEnd) {
      charsetEnd = q;
    }

    
    if (PRUint32(charsetEnd - charsetStart) >= sizeof(charset)) 
      goto badsyntax;
    
    memcpy(charset, charsetStart, charsetEnd - charsetStart);
    charset[charsetEnd - charsetStart] = 0;

    q++;
    if (*q != 'Q' && *q != 'q' && *q != 'B' && *q != 'b')
      goto badsyntax;

    if (q[1] != '?')
      goto badsyntax;

    r = q;
    for (r = q + 2; *r != '?'; r++) {
      if (*r < ' ') goto badsyntax;
    }
    if (r[1] != '=')
        goto badsyntax;
    else if (r == q + 2) {
        
        begin = r + 2;
        isLastEncodedWord = 1;
        continue;
    }

    if(*q == 'Q' || *q == 'q')
      decodedText = DecodeQ(q + 2, r - (q + 2));
    else {
      
      
      PRInt32 n = r - (q + 2);
      n -= (n % 4 == 1 && !PL_strncmp(r - 3, "===", 3)) ? 1 : 0;
      decodedText = PL_Base64Decode(q + 2, n, nsnull);
    }

    if (decodedText == nsnull)
      goto badsyntax;

    
    
    if ((aOverrideCharset && 0 != nsCRT::strcasecmp(charset, "UTF-8")) ||
        (aDefaultCharset && 0 == nsCRT::strcasecmp(charset, "UNKNOWN-8BIT"))) {
      PL_strncpy(charset, aDefaultCharset, sizeof(charset) - 1);
      charset[sizeof(charset) - 1] = '\0';
    }

    {
      nsCOMPtr<nsIUTF8ConverterService> 
        cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
      nsCAutoString utf8Text;
      
      if (cvtUTF8 &&
          NS_SUCCEEDED(
            cvtUTF8->ConvertStringToUTF8(nsDependentCString(decodedText),
            charset, IS_7BIT_NON_ASCII_CHARSET(charset), utf8Text))) {
        aResult.Append(utf8Text);
      } else {
        aResult.Append(REPLACEMENT_CHAR);
      }
    }
    PR_Free(decodedText);
    begin = r + 2;
    isLastEncodedWord = 1;
    continue;

  badsyntax:
    
    aResult.Append(begin, p - begin);
    begin = p;
    isLastEncodedWord = 0;
  }

  
  CopyRawHeader(begin, strlen(begin), aDefaultCharset, aResult);

  nsCAutoString tempStr(aResult);
  tempStr.ReplaceChar('\t', ' ');
  aResult = tempStr;

  return NS_OK;
}

