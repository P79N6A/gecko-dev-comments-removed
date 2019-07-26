





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
#include "nsNetError.h"
#include "nsIUnicodeDecoder.h"


  
static char *DecodeQ(const char *, PRUint32);
static bool Is7bitNonAsciiString(const char *, PRUint32);
static void CopyRawHeader(const char *, PRUint32, const char *, nsACString &);
static nsresult DecodeRFC2047Str(const char *, const char *, bool, nsACString&);



#define IS_7BIT_NON_ASCII_CHARSET(cset)            \
    (!nsCRT::strncasecmp((cset), "ISO-2022", 8) || \
     !nsCRT::strncasecmp((cset), "HZ-GB", 5)    || \
     !nsCRT::strncasecmp((cset), "UTF-7", 5))   

NS_IMPL_ISUPPORTS1(nsMIMEHeaderParamImpl, nsIMIMEHeaderParam)

NS_IMETHODIMP 
nsMIMEHeaderParamImpl::GetParameter(const nsACString& aHeaderVal, 
                                    const char *aParamName,
                                    const nsACString& aFallbackCharset, 
                                    bool aTryLocaleCharset, 
                                    char **aLang, nsAString& aResult)
{
  return DoGetParameter(aHeaderVal, aParamName, RFC_2231_DECODING,
                        aFallbackCharset, aTryLocaleCharset, aLang, aResult);
}

NS_IMETHODIMP 
nsMIMEHeaderParamImpl::GetParameter5987(const nsACString& aHeaderVal, 
                                        const char *aParamName,
                                        const nsACString& aFallbackCharset, 
                                        bool aTryLocaleCharset, 
                                        char **aLang, nsAString& aResult)
{
  return DoGetParameter(aHeaderVal, aParamName, RFC_5987_DECODING,
                        aFallbackCharset, aTryLocaleCharset, aLang, aResult);
}


nsresult 
nsMIMEHeaderParamImpl::DoGetParameter(const nsACString& aHeaderVal, 
                                      const char *aParamName,
                                      ParamDecoding aDecoding,
                                      const nsACString& aFallbackCharset, 
                                      bool aTryLocaleCharset, 
                                      char **aLang, nsAString& aResult)
{
    aResult.Truncate();
    nsresult rv;

    
    
    nsXPIDLCString med;
    nsXPIDLCString charset;
    rv = DoParameterInternal(PromiseFlatCString(aHeaderVal).get(), aParamName, 
                             aDecoding, getter_Copies(charset), aLang, 
                             getter_Copies(med));
    if (NS_FAILED(rv))
        return rv; 

    
    
    
    nsCAutoString str1;
    rv = DecodeParameter(med, charset.get(), nsnull, false, str1);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!aFallbackCharset.IsEmpty())
    {
        nsCAutoString str2;
        nsCOMPtr<nsIUTF8ConverterService> 
          cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
        if (cvtUTF8 &&
            NS_SUCCEEDED(cvtUTF8->ConvertStringToUTF8(str1, 
                PromiseFlatCString(aFallbackCharset).get(), false, true,
                                   str2))) {
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

  for (char *c = src; *c; ++c)
  {
    if (c[0] == '\\' && c[1])
    {
      
      ++c;
    }
    *dst++ = *c;
  }
  *dst = 0;
}




#define MAX_CONTINUATIONS 999



class Continuation {
  public:
    Continuation(const char *aValue, PRUint32 aLength,
                 bool aNeedsPercentDecoding) {
      value = aValue;
      length = aLength;
      needsPercentDecoding = aNeedsPercentDecoding;
    }
    Continuation() {
      
      value = 0L;
      length = 0;
      needsPercentDecoding = false;
    }
    ~Continuation() {}

    const char *value;
    PRUint32 length;
    bool needsPercentDecoding;
};



char *combineContinuations(nsTArray<Continuation>& aArray)
{
  
  if (aArray.Length() == 0)
    return NULL;

  
  PRUint32 length = 0;
  for (PRUint32 i = 0; i < aArray.Length(); i++) {
    length += aArray[i].length;
  }

  
  char *result = (char *) nsMemory::Alloc(length + 1);

  
  if (result) {
    *result = '\0';

    for (PRUint32 i = 0; i < aArray.Length(); i++) {
      Continuation cont = aArray[i];
      if (! cont.value) break;

      char *c = result + strlen(result);
      strncat(result, cont.value, cont.length);
      if (cont.needsPercentDecoding) {
        nsUnescape(c);
      }
    }

    
    if (*result == '\0') {
      nsMemory::Free(result);
      result = NULL;
    }
  } else {
    
    NS_WARNING("Out of memory\n");
  }

  return result;
}


bool addContinuation(nsTArray<Continuation>& aArray, PRUint32 aIndex,
                     const char *aValue, PRUint32 aLength,
                     bool aNeedsPercentDecoding)
{
  if (aIndex < aArray.Length() && aArray[aIndex].value) {
    NS_WARNING("duplicate RC2231 continuation segment #\n");
    return false;
  }

  if (aIndex > MAX_CONTINUATIONS) {
    NS_WARNING("RC2231 continuation segment # exceeds limit\n");
    return false;
  }

  Continuation cont (aValue, aLength, aNeedsPercentDecoding);

  if (aArray.Length() <= aIndex) {
    aArray.SetLength(aIndex + 1);
  }
  aArray[aIndex] = cont;

  return true;
}


PRInt32 parseSegmentNumber(const char *aValue, PRInt32 aLen)
{
  if (aLen < 1) {
    NS_WARNING("segment number missing\n");
    return -1;
  }

  if (aLen > 1 && aValue[0] == '0') {
    NS_WARNING("leading '0' not allowed in segment number\n");
    return -1;
  }

  PRInt32 segmentNumber = 0;

  for (PRInt32 i = 0; i < aLen; i++) {
    if (! (aValue[i] >= '0' && aValue[i] <= '9')) {
      NS_WARNING("invalid characters in segment number\n");
      return -1;
    }

    segmentNumber *= 10;
    segmentNumber += aValue[i] - '0';
    if (segmentNumber > MAX_CONTINUATIONS) {
      NS_WARNING("Segment number exceeds sane size\n");
      return -1;
    }
  }

  return segmentNumber;
}



bool IsValidOctetSequenceForCharset(nsACString& aCharset, const char *aOctets)
{
  nsCOMPtr<nsIUTF8ConverterService> cvtUTF8(do_GetService
    (NS_UTF8CONVERTERSERVICE_CONTRACTID));
  if (!cvtUTF8) {
    NS_WARNING("Can't get UTF8ConverterService\n");
    return false;
  }

  nsCAutoString tmpRaw;
  tmpRaw.Assign(aOctets);
  nsCAutoString tmpDecoded;

  nsresult rv = cvtUTF8->ConvertStringToUTF8(tmpRaw,
                                             PromiseFlatCString(aCharset).get(),
                                             false, false, tmpDecoded);

  if (rv != NS_OK) {
    
    
    NS_WARNING("RFC2231/5987 parameter value does not decode according to specified charset\n");
    return false;
  }

  return true;
}








NS_IMETHODIMP 
nsMIMEHeaderParamImpl::GetParameterInternal(const char *aHeaderValue, 
                                            const char *aParamName,
                                            char **aCharset,
                                            char **aLang,
                                            char **aResult)
{
  return DoParameterInternal(aHeaderValue, aParamName, RFC_2231_DECODING,
                             aCharset, aLang, aResult);
}


nsresult 
nsMIMEHeaderParamImpl::DoParameterInternal(const char *aHeaderValue, 
                                           const char *aParamName,
                                           ParamDecoding aDecoding,
                                           char **aCharset,
                                           char **aLang,
                                           char **aResult)
{

  if (!aHeaderValue ||  !*aHeaderValue || !aResult)
    return NS_ERROR_INVALID_ARG;

  *aResult = nsnull;

  if (aCharset) *aCharset = nsnull;
  if (aLang) *aLang = nsnull;

  nsCAutoString charset;

  bool acceptContinuations = (aDecoding != RFC_5987_DECODING);

  const char *str = aHeaderValue;

  
  for (; *str &&  nsCRT::IsAsciiSpace(*str); ++str)
    ;
  const char *start = str;
  
  
  
  
  if (!aParamName || !*aParamName) 
    {
      for (; *str && *str != ';' && !nsCRT::IsAsciiSpace(*str); ++str)
        ;
      if (str == start)
        return NS_ERROR_FIRST_HEADER_FIELD_COMPONENT_EMPTY;

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

  
  
  
  
  
  
  
  
  
  
  
  
  
  char *caseAResult = NULL;
  char *caseBResult = NULL;
  char *caseCDResult = NULL;

  
  nsTArray<Continuation> segments;


  
  
  nsDependentCSubstring charsetB, charsetCD;

  nsDependentCSubstring lang;

  PRInt32 paramLen = strlen(aParamName);

  while (*str) {
    

    const char *nameStart = str;
    const char *nameEnd = NULL;
    const char *valueStart = str;
    const char *valueEnd = NULL;
    bool isQuotedString = false;

    NS_ASSERTION(!nsCRT::IsAsciiSpace(*str), "should be after whitespace.");

    
    for (; *str && !nsCRT::IsAsciiSpace(*str) && *str != '=' && *str != ';'; str++)
      ;
    nameEnd = str;

    PRInt32 nameLen = nameEnd - nameStart;

    
    while (nsCRT::IsAsciiSpace(*str)) ++str;
    if (!*str) {
      break;
    }
    if (*str++ != '=') {
      
      goto increment_str;
    }
    while (nsCRT::IsAsciiSpace(*str)) ++str;

    if (*str != '"') {
      
      valueStart = str;
      for (valueEnd = str;
           *valueEnd && !nsCRT::IsAsciiSpace (*valueEnd) && *valueEnd != ';';
           valueEnd++)
        ;
      str = valueEnd;
    } else {
      isQuotedString = true;
      
      ++str;
      valueStart = str;
      for (valueEnd = str; *valueEnd; ++valueEnd) {
        if (*valueEnd == '\\')
          ++valueEnd;
        else if (*valueEnd == '"')
          break;
      }
      str = valueEnd;
      
      if (*valueEnd)
        str++;
    }

    
    
    
    if (nameLen == paramLen &&
        !nsCRT::strncasecmp(nameStart, aParamName, paramLen)) {

      if (caseAResult) {
        
        goto increment_str;
      }

      
      
      nsCAutoString tempStr(valueStart, valueEnd - valueStart);
      tempStr.StripChars("\r\n");
      char *res = ToNewCString(tempStr);
      NS_ENSURE_TRUE(res, NS_ERROR_OUT_OF_MEMORY);
      
      if (isQuotedString)
        RemoveQuotedStringEscapes(res);

      caseAResult = res;
      
    }
    
    else if (nameLen > paramLen &&
             !nsCRT::strncasecmp(nameStart, aParamName, paramLen) &&
             *(nameStart + paramLen) == '*') {

      
      const char *cp = nameStart + paramLen + 1; 

      
      bool needExtDecoding = *(nameEnd - 1) == '*';      

      bool caseB = nameLen == paramLen + 1;
      bool caseCStart = (*cp == '0') && needExtDecoding;

      
      PRInt32 segmentNumber = -1;
      if (!caseB) {
        PRInt32 segLen = (nameEnd - cp) - (needExtDecoding ? 1 : 0);
        segmentNumber = parseSegmentNumber(cp, segLen);

        if (segmentNumber == -1) {
          acceptContinuations = false;
          goto increment_str;
        }
      }

      
      
      if (caseB || (caseCStart && acceptContinuations)) {
        
        const char *sQuote1 = PL_strchr(valueStart, 0x27);
        const char *sQuote2 = sQuote1 ? PL_strchr(sQuote1 + 1, 0x27) : nsnull;

        
        
        if (!sQuote1 || !sQuote2) {
          NS_WARNING("Mandatory two single quotes are missing in header parameter\n");
        }

        const char *charsetStart = NULL;
        PRInt32 charsetLength = 0;
        const char *langStart = NULL;
        PRInt32 langLength = 0;
        const char *rawValStart = NULL;
        PRInt32 rawValLength = 0;

        if (sQuote2 && sQuote1) {
          
          rawValStart = sQuote2 + 1;
          rawValLength = valueEnd - rawValStart;

          langStart = sQuote1 + 1;
          langLength = sQuote2 - langStart;

          charsetStart = valueStart;
          charsetLength = sQuote1 - charsetStart;
        }
        else if (sQuote1) {
          
          rawValStart = sQuote1 + 1;
          rawValLength = valueEnd - rawValStart;

          charsetStart = valueStart;
          charsetLength = sQuote1 - valueStart;
        }
        else {
          
          rawValStart = valueStart;
          rawValLength = valueEnd - valueStart;
        }

        if (langLength != 0) {
          lang.Assign(langStart, langLength);
        }

        
        if (caseB) {
          charsetB.Assign(charsetStart, charsetLength);
        } else {
          
          charsetCD.Assign(charsetStart, charsetLength);
        }

        
        if (rawValLength > 0) {
          if (!caseBResult && caseB) {
            
            char *tmpResult = (char *) nsMemory::Clone(rawValStart, rawValLength + 1);
            if (!tmpResult) {
              goto increment_str;
            }
            *(tmpResult + rawValLength) = 0;

            nsUnescape(tmpResult);
            caseBResult = tmpResult;
          } else {
            
            bool added = addContinuation(segments, 0, rawValStart,
                                         rawValLength, needExtDecoding);

            if (!added) {
              
              acceptContinuations = false;
            }
          }
        }
      }  
      
      
      else if (acceptContinuations && segmentNumber != -1) {
        PRUint32 valueLength = valueEnd - valueStart;

        bool added = addContinuation(segments, segmentNumber, valueStart,
                                     valueLength, needExtDecoding);

        if (!added) {
          
          acceptContinuations = false;
        }
      } 
    }

    
    
increment_str:      
    while (nsCRT::IsAsciiSpace(*str)) ++str;
    if (*str == ';') ++str;
    while (nsCRT::IsAsciiSpace(*str)) ++str;
  }

  caseCDResult = combineContinuations(segments);

  if (caseBResult && !charsetB.IsEmpty()) {
    
    
    if (!IsValidOctetSequenceForCharset(charsetB, caseBResult))
      caseBResult = NULL;
  }

  if (caseCDResult && !charsetCD.IsEmpty()) {
    
    
    if (!IsValidOctetSequenceForCharset(charsetCD, caseCDResult))
      caseCDResult = NULL;
  }

  if (caseBResult) {
    
    *aResult = caseBResult;
    caseBResult = NULL;
    charset.Assign(charsetB);
  }
  else if (caseCDResult) {
    
    *aResult = caseCDResult;
    caseCDResult = NULL;
    charset.Assign(charsetCD);
  }
  else if (caseAResult) {
    *aResult = caseAResult;
    caseAResult = NULL;
  }

  
  nsMemory::Free(caseAResult);
  nsMemory::Free(caseBResult);
  nsMemory::Free(caseCDResult);

  
  if (*aResult) {
    
    if (aLang && !lang.IsEmpty()) {
      PRUint32 len = lang.Length();
      *aLang = (char *) nsMemory::Clone(lang.BeginReading(), len + 1);
      if (*aLang) {
        *(*aLang + len) = 0;
      }
   }
    if (aCharset && !charset.IsEmpty()) {
      PRUint32 len = charset.Length();
      *aCharset = (char *) nsMemory::Clone(charset.BeginReading(), len + 1);
      if (*aCharset) {
        *(*aCharset + len) = 0;
      }
    }
  }

  return *aResult ? NS_OK : NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP
nsMIMEHeaderParamImpl::DecodeRFC2047Header(const char* aHeaderVal, 
                                           const char* aDefaultCharset, 
                                           bool aOverrideCharset, 
                                           bool aEatContinuations,
                                           nsACString& aResult)
{
  aResult.Truncate();
  if (!aHeaderVal)
    return NS_ERROR_INVALID_ARG;
  if (!*aHeaderVal)
    return NS_OK;


  
  
  
  if (PL_strstr(aHeaderVal, "=?") || 
      (aDefaultCharset && (!IsUTF8(nsDependentCString(aHeaderVal)) || 
      Is7bitNonAsciiString(aHeaderVal, PL_strlen(aHeaderVal))))) {
    DecodeRFC2047Str(aHeaderVal, aDefaultCharset, aOverrideCharset, aResult);
  } else if (aEatContinuations && 
             (PL_strchr(aHeaderVal, '\n') || PL_strchr(aHeaderVal, '\r'))) {
    aResult = aHeaderVal;
  } else {
    aEatContinuations = false;
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



bool IsRFC5987AttrChar(char aChar)
{
  char c = aChar;

  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         (c == '!' || c == '#' || c == '$' || c == '&' ||
          c == '+' || c == '-' || c == '.' || c == '^' ||
          c == '_' || c == '`' || c == '|' || c == '~');
}


bool IsHexDigit(char aChar)
{
  char c = aChar;

  return (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F') ||
         (c >= '0' && c <= '9');
}



bool PercentDecode(nsACString& aValue)
{
  char *c = (char *) nsMemory::Alloc(aValue.Length() + 1);
  if (!c) {
    return false;
  }

  strcpy(c, PromiseFlatCString(aValue).get());
  nsUnescape(c);
  aValue.Assign(c);
  nsMemory::Free(c);

  return true;
}




NS_IMETHODIMP 
nsMIMEHeaderParamImpl::DecodeRFC5987Param(const nsACString& aParamVal,
                                          nsACString& aLang,
                                          nsAString& aResult)
{
  nsCAutoString charset;
  nsCAutoString language;
  nsCAutoString value;

  PRUint32 delimiters = 0;
  const char *encoded = PromiseFlatCString(aParamVal).get();
  const char *c = encoded;

  while (*c) {
    char tc = *c++;

    if (tc == '\'') {
      
      delimiters++;
    } else if (tc >= 128) {
      
      NS_WARNING("non-US-ASCII character in RFC5987-encoded param");
      return NS_ERROR_INVALID_ARG;
    } else {
      if (delimiters == 0) {
        
        charset.Append(tc);
      } else if (delimiters == 1) {
        
        language.Append(tc);
      } else if (delimiters == 2) {
        if (IsRFC5987AttrChar(tc)) {
          value.Append(tc);
        } else if (tc == '%') {
          if (!IsHexDigit(c[0]) || !IsHexDigit(c[1])) {
            
            NS_WARNING("broken %-escape in RFC5987-encoded param");
            return NS_ERROR_INVALID_ARG;
          }
          value.Append(tc);
          
          value.Append(*c++);
          value.Append(*c++);
        } else {
          
          NS_WARNING("invalid character in RFC5987-encoded param");
          return NS_ERROR_INVALID_ARG;
        }      
      }
    }
  }

  if (delimiters != 2) {
    NS_WARNING("missing delimiters in RFC5987-encoded param");
    return NS_ERROR_INVALID_ARG;
  }

  
  if (!charset.LowerCaseEqualsLiteral("utf-8")) {
    NS_WARNING("unsupported charset in RFC5987-encoded param");
    return NS_ERROR_INVALID_ARG;
  }

  
  if (!PercentDecode(value)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  aLang.Assign(language);

  
  nsresult rv = NS_OK;
  nsCOMPtr<nsIUTF8ConverterService> cvtUTF8 =
    do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString utf8;
  rv = cvtUTF8->ConvertStringToUTF8(value, charset.get(), true, false, utf8);
  NS_ENSURE_SUCCESS(rv, rv);

  CopyUTF8toUTF16(utf8, aResult);
  return NS_OK;
}

NS_IMETHODIMP 
nsMIMEHeaderParamImpl::DecodeParameter(const nsACString& aParamValue,
                                       const char* aCharset,
                                       const char* aDefaultCharset,
                                       bool aOverrideCharset, 
                                       nsACString& aResult)
{
  aResult.Truncate();
  
  
  if (aCharset && *aCharset)
  {
    nsCOMPtr<nsIUTF8ConverterService> cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
    if (cvtUTF8)
      return cvtUTF8->ConvertStringToUTF8(aParamValue, aCharset,
          true, true, aResult);
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
                                    aOverrideCharset, true, decoded);
  
  if (NS_SUCCEEDED(rv) && !decoded.IsEmpty())
    aResult = decoded;
  
  return rv;
}

#define ISHEXCHAR(c) \
        ((0x30 <= PRUint8(c) && PRUint8(c) <= 0x39)  ||  \
         (0x41 <= PRUint8(c) && PRUint8(c) <= 0x46)  ||  \
         (0x61 <= PRUint8(c) && PRUint8(c) <= 0x66))



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





bool Is7bitNonAsciiString(const char *input, PRUint32 len)
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
    if (c & 0x80) return false;
    if (c == 0x1B) return true;
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

  
  
  bool skipCheck = (c == 0x1B || c == '~') && 
                     IS_7BIT_NON_ASCII_CHARSET(aDefaultCharset);

  
  nsCOMPtr<nsIUTF8ConverterService> 
    cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
  nsCAutoString utf8Text;
  if (cvtUTF8 &&
      NS_SUCCEEDED(
      cvtUTF8->ConvertStringToUTF8(Substring(aInput, aInput + aLen), 
      aDefaultCharset, skipCheck, true, utf8Text))) {
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
                          bool aOverrideCharset, nsACString &aResult)
{
  const char *p, *q = nsnull, *r;
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
            charset, IS_7BIT_NON_ASCII_CHARSET(charset), true, utf8Text))) {
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
