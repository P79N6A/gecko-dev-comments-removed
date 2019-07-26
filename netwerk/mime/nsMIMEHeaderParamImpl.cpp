





#include <string.h>
#include "prtypes.h"
#include "prmem.h"
#include "prprf.h"
#include "plstr.h"
#include "plbase64.h"
#include "nsCRT.h"
#include "nsMemory.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsEscape.h"
#include "nsIUTF8ConverterService.h"
#include "nsUConvCID.h"
#include "nsIServiceManager.h"
#include "nsMIMEHeaderParamImpl.h"
#include "nsReadableUtils.h"
#include "nsNativeCharsetUtils.h"
#include "nsError.h"
#include "nsIUnicodeDecoder.h"
#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;


  
static char *DecodeQ(const char *, uint32_t);
static bool Is7bitNonAsciiString(const char *, uint32_t);
static void CopyRawHeader(const char *, uint32_t, const char *, nsACString &);
static nsresult DecodeRFC2047Str(const char *, const char *, bool, nsACString&);
static nsresult internalDecodeParameter(const nsACString&, const char*,
                                        const char*, bool, bool, nsACString&);



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
  return DoGetParameter(aHeaderVal, aParamName, MIME_FIELD_ENCODING,
                        aFallbackCharset, aTryLocaleCharset, aLang, aResult);
}

NS_IMETHODIMP 
nsMIMEHeaderParamImpl::GetParameterHTTP(const nsACString& aHeaderVal, 
                                        const char *aParamName,
                                        const nsACString& aFallbackCharset, 
                                        bool aTryLocaleCharset, 
                                        char **aLang, nsAString& aResult)
{
  return DoGetParameter(aHeaderVal, aParamName, HTTP_FIELD_ENCODING,
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

    
    
    
    nsAutoCString str1;
    rv = internalDecodeParameter(med, charset.get(), nullptr, false,
                                 aDecoding == MIME_FIELD_ENCODING, str1);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!aFallbackCharset.IsEmpty())
    {
        nsAutoCString charset;
        EncodingUtils::FindEncodingForLabel(aFallbackCharset, charset);
        nsAutoCString str2;
        nsCOMPtr<nsIUTF8ConverterService> 
          cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
        if (cvtUTF8 &&
            NS_SUCCEEDED(cvtUTF8->ConvertStringToUTF8(str1, 
                PromiseFlatCString(aFallbackCharset).get(), false,
                                   !charset.EqualsLiteral("UTF-8"),
                                   1, str2))) {
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


bool IsHexDigit(char aChar)
{
  char c = aChar;

  return (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F') ||
         (c >= '0' && c <= '9');
}


bool IsValidPercentEscaped(const char *aValue, int32_t len)
{
  for (int32_t i = 0; i < len; i++) {
    if (aValue[i] == '%') {
      if (!IsHexDigit(aValue[i + 1]) || !IsHexDigit(aValue[i + 2])) {
        return false;
      }
    }
  }
  return true;
}




#define MAX_CONTINUATIONS 999



class Continuation {
  public:
    Continuation(const char *aValue, uint32_t aLength,
                 bool aNeedsPercentDecoding, bool aWasQuotedString) {
      value = aValue;
      length = aLength;
      needsPercentDecoding = aNeedsPercentDecoding;
      wasQuotedString = aWasQuotedString;
    }
    Continuation() {
      
      value = 0L;
      length = 0;
      needsPercentDecoding = false;
      wasQuotedString = false;
    }
    ~Continuation() {}

    const char *value;
    uint32_t length;
    bool needsPercentDecoding;
    bool wasQuotedString;
};



char *combineContinuations(nsTArray<Continuation>& aArray)
{
  
  if (aArray.Length() == 0)
    return NULL;

  
  uint32_t length = 0;
  for (uint32_t i = 0; i < aArray.Length(); i++) {
    length += aArray[i].length;
  }

  
  char *result = (char *) nsMemory::Alloc(length + 1);

  
  if (result) {
    *result = '\0';

    for (uint32_t i = 0; i < aArray.Length(); i++) {
      Continuation cont = aArray[i];
      if (! cont.value) break;

      char *c = result + strlen(result);
      strncat(result, cont.value, cont.length);
      if (cont.needsPercentDecoding) {
        nsUnescape(c);
      }
      if (cont.wasQuotedString) {
        RemoveQuotedStringEscapes(c);
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


bool addContinuation(nsTArray<Continuation>& aArray, uint32_t aIndex,
                     const char *aValue, uint32_t aLength,
                     bool aNeedsPercentDecoding, bool aWasQuotedString)
{
  if (aIndex < aArray.Length() && aArray[aIndex].value) {
    NS_WARNING("duplicate RC2231 continuation segment #\n");
    return false;
  }

  if (aIndex > MAX_CONTINUATIONS) {
    NS_WARNING("RC2231 continuation segment # exceeds limit\n");
    return false;
  }

  if (aNeedsPercentDecoding && aWasQuotedString) {
    NS_WARNING("RC2231 continuation segment can't use percent encoding and quoted string form at the same time\n");
    return false;
  }

  Continuation cont(aValue, aLength, aNeedsPercentDecoding, aWasQuotedString);

  if (aArray.Length() <= aIndex) {
    aArray.SetLength(aIndex + 1);
  }
  aArray[aIndex] = cont;

  return true;
}


int32_t parseSegmentNumber(const char *aValue, int32_t aLen)
{
  if (aLen < 1) {
    NS_WARNING("segment number missing\n");
    return -1;
  }

  if (aLen > 1 && aValue[0] == '0') {
    NS_WARNING("leading '0' not allowed in segment number\n");
    return -1;
  }

  int32_t segmentNumber = 0;

  for (int32_t i = 0; i < aLen; i++) {
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

  nsAutoCString tmpRaw;
  tmpRaw.Assign(aOctets);
  nsAutoCString tmpDecoded;

  nsresult rv = cvtUTF8->ConvertStringToUTF8(tmpRaw,
                                             PromiseFlatCString(aCharset).get(),
                                             false, false, 1, tmpDecoded);

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
  return DoParameterInternal(aHeaderValue, aParamName, MIME_FIELD_ENCODING,
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

  *aResult = nullptr;

  if (aCharset) *aCharset = nullptr;
  if (aLang) *aLang = nullptr;

  nsAutoCString charset;

  
  
  bool acceptContinuations = true;

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

  int32_t paramLen = strlen(aParamName);

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

    int32_t nameLen = nameEnd - nameStart;

    
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
        if (*valueEnd == '\\' && *(valueEnd + 1))
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

      
      
      nsAutoCString tempStr(valueStart, valueEnd - valueStart);
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

      
      int32_t segmentNumber = -1;
      if (!caseB) {
        int32_t segLen = (nameEnd - cp) - (needExtDecoding ? 1 : 0);
        segmentNumber = parseSegmentNumber(cp, segLen);

        if (segmentNumber == -1) {
          acceptContinuations = false;
          goto increment_str;
        }
      }

      
      
      if (caseB || (caseCStart && acceptContinuations)) {
        
        const char *sQuote1 = PL_strchr(valueStart, 0x27);
        const char *sQuote2 = sQuote1 ? PL_strchr(sQuote1 + 1, 0x27) : nullptr;

        
        
        if (!sQuote1 || !sQuote2) {
          NS_WARNING("Mandatory two single quotes are missing in header parameter\n");
        }

        const char *charsetStart = NULL;
        int32_t charsetLength = 0;
        const char *langStart = NULL;
        int32_t langLength = 0;
        const char *rawValStart = NULL;
        int32_t rawValLength = 0;

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
            if (!IsValidPercentEscaped(rawValStart, rawValLength)) {
              goto increment_str;
            }

            
            char *tmpResult = (char *) nsMemory::Clone(rawValStart, rawValLength + 1);
            if (!tmpResult) {
              goto increment_str;
            }
            *(tmpResult + rawValLength) = 0;

            nsUnescape(tmpResult);
            caseBResult = tmpResult;
          } else {
            
            bool added = addContinuation(segments, 0, rawValStart,
                                         rawValLength, needExtDecoding,
                                         isQuotedString);

            if (!added) {
              
              acceptContinuations = false;
            }
          }
        }
      }  
      
      
      else if (acceptContinuations && segmentNumber != -1) {
        uint32_t valueLength = valueEnd - valueStart;

        bool added = addContinuation(segments, segmentNumber, valueStart,
                                     valueLength, needExtDecoding,
                                     isQuotedString);

        if (!added) {
          
          acceptContinuations = false;
        }
      } 
    }

    
    
increment_str:      
    while (nsCRT::IsAsciiSpace(*str)) ++str;
    if (*str == ';') {
      ++str;
    } else {
      
      
      break;
    }
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
      uint32_t len = lang.Length();
      *aLang = (char *) nsMemory::Clone(lang.BeginReading(), len + 1);
      if (*aLang) {
        *(*aLang + len) = 0;
      }
   }
    if (aCharset && !charset.IsEmpty()) {
      uint32_t len = charset.Length();
      *aCharset = (char *) nsMemory::Clone(charset.BeginReading(), len + 1);
      if (*aCharset) {
        *(*aCharset + len) = 0;
      }
    }
  }

  return *aResult ? NS_OK : NS_ERROR_INVALID_ARG;
}

nsresult
internalDecodeRFC2047Header(const char* aHeaderVal, const char* aDefaultCharset,
                            bool aOverrideCharset, bool aEatContinuations,
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
    nsAutoCString temp(aResult);
    temp.ReplaceSubstring("\n\t", " ");
    temp.ReplaceSubstring("\r\t", " ");
    temp.StripChars("\r\n");
    aResult = temp;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMIMEHeaderParamImpl::DecodeRFC2047Header(const char* aHeaderVal, 
                                           const char* aDefaultCharset, 
                                           bool aOverrideCharset, 
                                           bool aEatContinuations,
                                           nsACString& aResult)
{
  return internalDecodeRFC2047Header(aHeaderVal, aDefaultCharset,
                                     aOverrideCharset, aEatContinuations,
                                     aResult);
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
  nsAutoCString charset;
  nsAutoCString language;
  nsAutoCString value;

  uint32_t delimiters = 0;
  const char *encoded = PromiseFlatCString(aParamVal).get();
  const char *c = encoded;

  while (*c) {
    char tc = *c++;

    if (tc == '\'') {
      
      delimiters++;
    } else if (((unsigned char)tc) >= 128) {
      
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

  nsAutoCString utf8;
  rv = cvtUTF8->ConvertStringToUTF8(value, charset.get(), true, false, 1, utf8);
  NS_ENSURE_SUCCESS(rv, rv);

  CopyUTF8toUTF16(utf8, aResult);
  return NS_OK;
}

nsresult 
internalDecodeParameter(const nsACString& aParamValue, const char* aCharset,
                        const char* aDefaultCharset, bool aOverrideCharset,
                        bool aDecode2047, nsACString& aResult)
{
  aResult.Truncate();
  
  
  if (aCharset && *aCharset)
  {
    nsCOMPtr<nsIUTF8ConverterService> cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID));
    if (cvtUTF8)
      return cvtUTF8->ConvertStringToUTF8(aParamValue, aCharset,
          true, true, 1, aResult);
  }

  const nsAFlatCString& param = PromiseFlatCString(aParamValue);
  nsAutoCString unQuoted;
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
  nsresult rv = NS_OK;
  
  if (aDecode2047) {
    nsAutoCString decoded;

    
    rv = internalDecodeRFC2047Header(unQuoted.get(), aDefaultCharset,
                                     aOverrideCharset, true, decoded);

    if (NS_SUCCEEDED(rv) && !decoded.IsEmpty())
      aResult = decoded;
  }
    
  return rv;
}

NS_IMETHODIMP
nsMIMEHeaderParamImpl::DecodeParameter(const nsACString& aParamValue,
                                       const char* aCharset,
                                       const char* aDefaultCharset,
                                       bool aOverrideCharset,
                                       nsACString& aResult)
{
  return internalDecodeParameter(aParamValue, aCharset, aDefaultCharset,
                                 aOverrideCharset, true, aResult);
}

#define ISHEXCHAR(c) \
        ((0x30 <= uint8_t(c) && uint8_t(c) <= 0x39)  ||  \
         (0x41 <= uint8_t(c) && uint8_t(c) <= 0x46)  ||  \
         (0x61 <= uint8_t(c) && uint8_t(c) <= 0x66))



char *DecodeQ(const char *in, uint32_t length)
{
  char *out, *dest = 0;

  out = dest = (char *)PR_Calloc(length + 1, sizeof(char));
  if (dest == nullptr)
    return nullptr;
  while (length > 0) {
    unsigned c = 0;
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
  return nullptr;
}





bool Is7bitNonAsciiString(const char *input, uint32_t len)
{
  int32_t c;

  enum { hz_initial, 
         hz_escaped, 
         hz_seen, 
         hz_notpresent 
  } hz_state;

  hz_state = hz_initial;
  while (len) {
    c = uint8_t(*input++);
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








void CopyRawHeader(const char *aInput, uint32_t aLen, 
                   const char *aDefaultCharset, nsACString &aOutput)
{
  int32_t c;

  
  if (!aDefaultCharset || !*aDefaultCharset) {
    aOutput.Append(aInput, aLen);
    return;
  }

  
  
  while (aLen && (c = uint8_t(*aInput++)) != 0x1B && c != '~' && !(c & 0x80)) {
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
  nsAutoCString utf8Text;
  if (cvtUTF8 &&
      NS_SUCCEEDED(
      cvtUTF8->ConvertStringToUTF8(Substring(aInput, aInput + aLen), 
                                   aDefaultCharset, skipCheck, true, 1,
                                   utf8Text))) {
    aOutput.Append(utf8Text);
  } else { 
    for (uint32_t i = 0; i < aLen; i++) {
      c = uint8_t(*aInput++);
      if (c & 0x80)
        aOutput.Append(REPLACEMENT_CHAR);
      else
        aOutput.Append(char(c));
    }
  }
}

nsresult DecodeQOrBase64Str(const char *aEncoded, size_t aLen, char aQOrBase64,
                            const char *aCharset, nsACString &aResult)
{
  char *decodedText;
  NS_ASSERTION(aQOrBase64 == 'Q' || aQOrBase64 == 'B', "Should be 'Q' or 'B'");
  if(aQOrBase64 == 'Q')
    decodedText = DecodeQ(aEncoded, aLen);
  else if (aQOrBase64 == 'B') {
    decodedText = PL_Base64Decode(aEncoded, aLen, nullptr);
  } else {
    return NS_ERROR_INVALID_ARG;
  }

  if (!decodedText) {
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  nsCOMPtr<nsIUTF8ConverterService>
    cvtUTF8(do_GetService(NS_UTF8CONVERTERSERVICE_CONTRACTID, &rv));
  nsAutoCString utf8Text;
  if (NS_SUCCEEDED(rv)) {
    
    rv = cvtUTF8->ConvertStringToUTF8(nsDependentCString(decodedText),
                                      aCharset,
                                      IS_7BIT_NON_ASCII_CHARSET(aCharset),
                                      true, 1, utf8Text);
  }
  PR_Free(decodedText);
  if (NS_FAILED(rv)) {
    return rv;
  }
  aResult.Append(utf8Text);

  return NS_OK;
}

static const char especials[] = "()<>@,;:\\\"/[]?.=";







nsresult DecodeRFC2047Str(const char *aHeader, const char *aDefaultCharset, 
                          bool aOverrideCharset, nsACString &aResult)
{
  const char *p, *q = nullptr, *r;
  const char *begin; 
  int32_t isLastEncodedWord = 0;
  const char *charsetStart, *charsetEnd;
  nsAutoCString prevCharset, curCharset;
  nsAutoCString encodedText;
  char prevEncoding = '\0', curEncoding;
  nsresult rv;

  begin = aHeader;

  
  
  
  
  
  
  aResult.SetCapacity(3 * strlen(aHeader));

  while ((p = PL_strstr(begin, "=?")) != 0) {
    if (isLastEncodedWord) {
      
      for (q = begin; q < p; ++q) {
        if (!PL_strchr(" \t\r\n", *q)) break;
      }
    }

    if (!isLastEncodedWord || q < p) {
      if (!encodedText.IsEmpty()) {
        rv = DecodeQOrBase64Str(encodedText.get(), encodedText.Length(),
                                prevEncoding, prevCharset.get(), aResult);
        if (NS_FAILED(rv)) {
          aResult.Append(encodedText);
        }
        encodedText.Truncate();
        prevCharset.Truncate();
        prevEncoding = '\0';
      }
      
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

    q++;
    curEncoding = nsCRT::ToUpper(*q);
    if (curEncoding != 'Q' && curEncoding != 'B')
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

    curCharset.Assign(charsetStart, charsetEnd - charsetStart);
    
    
    if ((aOverrideCharset && 0 != nsCRT::strcasecmp(curCharset.get(), "UTF-8"))
    || (aDefaultCharset && 0 == nsCRT::strcasecmp(curCharset.get(), "UNKNOWN-8BIT"))
    ) {
      curCharset = aDefaultCharset;
    }

    const char *R;
    R = r;
    if (curEncoding == 'B') {
      
      
      int32_t n = r - (q + 2);
      R -= (n % 4 == 1 && !PL_strncmp(r - 3, "===", 3)) ? 1 : 0;
    }
    
    if (R[-1] != '='
      && (prevCharset.IsEmpty()
        || (curCharset == prevCharset && curEncoding == prevEncoding))
    ) {
      encodedText.Append(q + 2, R - (q + 2));
      prevCharset = curCharset;
      prevEncoding = curEncoding;

      begin = r + 2;
      isLastEncodedWord = 1;
      continue;
    }

    bool bDecoded; 
    bDecoded = false;
    if (!encodedText.IsEmpty()) {
      if (curCharset == prevCharset && curEncoding == prevEncoding) {
        encodedText.Append(q + 2, R - (q + 2));
        bDecoded = true;
      }
      rv = DecodeQOrBase64Str(encodedText.get(), encodedText.Length(),
                              prevEncoding, prevCharset.get(), aResult);
      if (NS_FAILED(rv)) {
        aResult.Append(encodedText);
      }
      encodedText.Truncate();
      prevCharset.Truncate();
      prevEncoding = '\0';
    }
    if (!bDecoded) {
      rv = DecodeQOrBase64Str(q + 2, R - (q + 2), curEncoding,
                              curCharset.get(), aResult);
      if (NS_FAILED(rv)) {
        aResult.Append(encodedText);
      }
    }

    begin = r + 2;
    isLastEncodedWord = 1;
    continue;

  badsyntax:
    if (!encodedText.IsEmpty()) {
      rv = DecodeQOrBase64Str(encodedText.get(), encodedText.Length(),
                              prevEncoding, prevCharset.get(), aResult);
      if (NS_FAILED(rv)) {
        aResult.Append(encodedText);
      }
      encodedText.Truncate();
      prevCharset.Truncate();
    }
    
    aResult.Append(begin, p - begin);
    begin = p;
    isLastEncodedWord = 0;
  }

  if (!encodedText.IsEmpty()) {
    rv = DecodeQOrBase64Str(encodedText.get(), encodedText.Length(),
                            prevEncoding, prevCharset.get(), aResult);
    if (NS_FAILED(rv)) {
      aResult.Append(encodedText);
    }
  }

  
  CopyRawHeader(begin, strlen(begin), aDefaultCharset, aResult);

  nsAutoCString tempStr(aResult);
  tempStr.ReplaceChar('\t', ' ');
  aResult = tempStr;

  return NS_OK;
}
