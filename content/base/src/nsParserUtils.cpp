








































#include "nsParserUtils.h"
#include "jsapi.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsContentUtils.h"
#include "nsIParserService.h"
#include "nsParserConstants.h"

#define SKIP_WHITESPACE(iter, end_iter, end_res)                 \
  while ((iter) != (end_iter) && nsCRT::IsAsciiSpace(*(iter))) { \
    ++(iter);                                                    \
  }                                                              \
  if ((iter) == (end_iter)) {                                    \
    return (end_res);                                            \
  }

#define SKIP_ATTR_NAME(iter, end_iter)                            \
  while ((iter) != (end_iter) && !nsCRT::IsAsciiSpace(*(iter)) && \
         *(iter) != '=') {                                        \
    ++(iter);                                                     \
  }

bool
nsParserUtils::GetQuotedAttributeValue(const nsString& aSource, nsIAtom *aName,
                                       nsAString& aValue)
{
  aValue.Truncate();

  const PRUnichar *start = aSource.get();
  const PRUnichar *end = start + aSource.Length();
  const PRUnichar *iter;
  
  while (start != end) {
    SKIP_WHITESPACE(start, end, false)
    iter = start;
    SKIP_ATTR_NAME(iter, end)

    if (start == iter) {
      return false;
    }

    
    const nsDependentSubstring & attrName = Substring(start, iter);

    
    start = iter;
    SKIP_WHITESPACE(start, end, false)
    if (*start != '=') {
      
      
      return false;
    }
    
    
    ++start;
    SKIP_WHITESPACE(start, end, false)
    PRUnichar q = *start;
    if (q != kQuote && q != kApostrophe) {
      
      return false;
    }
    
    ++start;  
    iter = start;

    while (iter != end && *iter != q) {
      ++iter;
    }

    if (iter == end) {
      
      return false;
    }

    
    
    
    if (aName->Equals(attrName)) {
      nsIParserService* parserService = nsContentUtils::GetParserService();
      NS_ENSURE_TRUE(parserService, false);

      
      
      
      const PRUnichar *chunkEnd = start;
      while (chunkEnd != iter) {
        if (*chunkEnd == kLessThan) {
          aValue.Truncate();

          return false;
        }

        if (*chunkEnd == kAmpersand) {
          aValue.Append(start, chunkEnd - start);

          
          ++chunkEnd;

          const PRUnichar *afterEntity;
          PRUnichar result[2];
          PRUint32 count =
            parserService->DecodeEntity(chunkEnd, iter, &afterEntity, result);
          if (count == 0) {
            aValue.Truncate();

            return false;
          }

          aValue.Append(result, count);

          
          start = chunkEnd = afterEntity;
        }
        else {
          ++chunkEnd;
        }
      }

      
      aValue.Append(start, iter - start);

      return true;
    }

    
    
    start = iter + 1;
  }

  return false;
}



bool
nsParserUtils::IsJavaScriptLanguage(const nsString& aName, PRUint32 *aFlags)
{
  JSVersion version = JSVERSION_UNKNOWN;

  if (aName.LowerCaseEqualsLiteral("javascript") ||
      aName.LowerCaseEqualsLiteral("livescript") ||
      aName.LowerCaseEqualsLiteral("mocha")) {
    version = JSVERSION_DEFAULT;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.0")) {
    version = JSVERSION_1_0;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.1")) {
    version = JSVERSION_1_1;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.2")) {
    version = JSVERSION_1_2;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.3")) {
    version = JSVERSION_1_3;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.4")) {
    version = JSVERSION_1_4;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.5")) {
    version = JSVERSION_1_5;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.6")) {
    version = JSVERSION_1_6;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.7")) {
    version = JSVERSION_1_7;
  }
  else if (aName.LowerCaseEqualsLiteral("javascript1.8")) {
    version = JSVERSION_1_8;
  }
  if (version == JSVERSION_UNKNOWN)
    return false;
  *aFlags = version;
  return true;
}

void
nsParserUtils::SplitMimeType(const nsAString& aValue, nsString& aType,
                             nsString& aParams)
{
  aType.Truncate();
  aParams.Truncate();
  PRInt32 semiIndex = aValue.FindChar(PRUnichar(';'));
  if (-1 != semiIndex) {
    aType = Substring(aValue, 0, semiIndex);
    aParams = Substring(aValue, semiIndex + 1,
                       aValue.Length() - (semiIndex + 1));
    aParams.StripWhitespace();
  }
  else {
    aType = aValue;
  }
  aType.StripWhitespace();
}
