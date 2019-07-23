




































#include "nsSMILParserUtils.h"
#include "nsISMILAttr.h"
#include "nsSMILValue.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimeValueSpecParams.h"
#include "nsSMILTypes.h"
#include "nsSMILRepeatCount.h"
#include "nsContentUtils.h"
#include "nsString.h"
#include "prdtoa.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "prlong.h"




namespace {

const PRUint32 MSEC_PER_SEC  = 1000;
const PRUint32 MSEC_PER_MIN  = 1000 * 60;
const PRUint32 MSEC_PER_HOUR = 1000 * 60 * 60;
const PRInt32  DECIMAL_BASE  = 10;

#define ACCESSKEY_PREFIX NS_LITERAL_STRING("accesskey(")
#define REPEAT_PREFIX    NS_LITERAL_STRING("repeat(")
#define WALLCLOCK_PREFIX NS_LITERAL_STRING("wallclock(")



inline PRBool
IsSpace(const PRUnichar c)
{
  return (c == 0x9 || c == 0xA || c == 0xD || c == 0x20);
}

template<class T>
inline void
SkipBeginWsp(T& aStart, T aEnd)
{
  while (aStart != aEnd && IsSpace(*aStart)) {
    ++aStart;
  }
}

inline void
SkipBeginEndWsp(const PRUnichar*& aStart, const PRUnichar*& aEnd)
{
  SkipBeginWsp(aStart, aEnd);
  while (aEnd != aStart && IsSpace(*(aEnd - 1))) {
    --aEnd;
  }
}

double
GetFloat(const char*& aStart, const char* aEnd, nsresult* aErrorCode)
{
  char* floatEnd;
  double value = PR_strtod(aStart, &floatEnd);

  nsresult rv;

  if (floatEnd == aStart || floatEnd > aEnd) {
    rv = NS_ERROR_FAILURE;
  } else {
    aStart = floatEnd;
    rv = NS_OK;
  }

  if (aErrorCode) {
    *aErrorCode = rv;
  }

  return value;
}

size_t
GetUnsignedInt(const nsAString& aStr, PRUint32& aResult)
{
  NS_ConvertUTF16toUTF8 cstr(aStr);
  const char* str = cstr.get();

  char* rest;
  PRInt32 value = strtol(str, &rest, DECIMAL_BASE);

  if (rest == str || value < 0)
    return 0;

  aResult = static_cast<PRUint32>(value);
  return rest - str;
}

PRBool
GetUnsignedIntAndEndParen(const nsAString& aStr, PRUint32& aResult)
{
  size_t intLen = GetUnsignedInt(aStr, aResult);

  const PRUnichar* start = aStr.BeginReading();
  const PRUnichar* end = aStr.EndReading();

  
  if (intLen == 0 || start + intLen + 1 != end || *(start + intLen) != ')')
    return PR_FALSE;

  return PR_TRUE;
}

inline PRBool
ConsumeSubstring(const char*& aStart, const char* aEnd, const char* aSubstring)
{
  size_t substrLen = PL_strlen(aSubstring);

  if (static_cast<size_t>(aEnd - aStart) < substrLen)
    return PR_FALSE;

  PRBool result = PR_FALSE;

  if (PL_strstr(aStart, aSubstring) == aStart) {
    aStart += substrLen;
    result = PR_TRUE;
  }

  return result;
}

PRBool
ParseClockComponent(const char*& aStart,
                    const char* aEnd,
                    double& aResult,
                    PRBool& aIsReal,
                    PRBool& aCouldBeMin,
                    PRBool& aCouldBeSec)
{
  nsresult rv;
  const char* begin = aStart;
  double value = GetFloat(aStart, aEnd, &rv);

  
  if (NS_FAILED(rv))
    return PR_FALSE;

  
  size_t len = aStart - begin;
  PRBool isExp = (PL_strnpbrk(begin, "eE", len) != nsnull);
  if (isExp)
    return PR_FALSE;

  
  if (*(aStart - 1) == '.')
    return PR_FALSE;

  
  aResult = value;

  
  
  aIsReal = (PL_strnchr(begin, '.', len) != nsnull);
  aCouldBeMin = (value < 60.0 && (len == 2));
  aCouldBeSec = (value < 60.0 ||
      (value == 60.0 && begin[0] == '5')); 
  aCouldBeSec &= (len >= 2 &&
      (begin[2] == '\0' || begin[2] == '.' || IsSpace(begin[2])));

  return PR_TRUE;
}

PRBool
ParseMetricMultiplicand(const char*& aStart,
                        const char* aEnd,
                        PRInt32& multiplicand)
{
  PRBool result = PR_FALSE;

  size_t len = aEnd - aStart;
  const char* cur = aStart;

  if (len) {
    switch (*cur++)
    {
      case 'h':
        multiplicand = MSEC_PER_HOUR;
        result = PR_TRUE;
        break;
      case 'm':
        if (len >= 2) {
          if (*cur == 's') {
            ++cur;
            multiplicand = 1;
            result = PR_TRUE;
          } else if (len >= 3 && *cur++ == 'i' && *cur++ == 'n') {
            multiplicand = MSEC_PER_MIN;
            result = PR_TRUE;
          }
        }
        break;
      case 's':
        multiplicand = MSEC_PER_SEC;
        result = PR_TRUE;
        break;
    }
  }

  if (result) {
    aStart = cur;
  }

  return result;
}

nsresult
ParseOptionalOffset(const nsAString& aSpec, nsSMILTimeValueSpecParams& aResult)
{
  if (aSpec.IsEmpty()) {
    aResult.mOffset.SetMillis(0);
    return NS_OK;
  }

  if (aSpec.First() != '+' && aSpec.First() != '-')
    return NS_ERROR_FAILURE;

  return nsSMILParserUtils::ParseClockValue(aSpec, &aResult.mOffset, PR_TRUE);
}

nsresult
ParseAccessKey(const nsAString& aSpec, nsSMILTimeValueSpecParams& aResult)
{
  NS_ABORT_IF_FALSE(StringBeginsWith(aSpec, ACCESSKEY_PREFIX),
      "Calling ParseAccessKey on non-accesskey-type spec");

  nsSMILTimeValueSpecParams result;
  result.mType = nsSMILTimeValueSpecParams::ACCESSKEY;

  const PRUnichar* start = aSpec.BeginReading() + ACCESSKEY_PREFIX.Length();
  const PRUnichar* end = aSpec.EndReading();

  
  if (end - start < 2)
    return NS_ERROR_FAILURE;

  PRUint32 c = *start++;

  
  if (NS_IS_HIGH_SURROGATE(c)) {
    if (end - start < 2) 
      return NS_ERROR_FAILURE;
    PRUint32 lo = *start++;
    if (!NS_IS_LOW_SURROGATE(lo))
      return NS_ERROR_FAILURE;
    c = SURROGATE_TO_UCS4(c, lo);
  
  } else if (NS_IS_LOW_SURROGATE(c) || c == 0xFFFE || c == 0xFFFF) {
    return NS_ERROR_FAILURE;
  }

  result.mRepeatIterationOrAccessKey = c;

  if (*start++ != ')')
    return NS_ERROR_FAILURE;

  SkipBeginWsp(start, end);

  nsresult rv = ParseOptionalOffset(Substring(start, end), result);
  if (NS_FAILED(rv))
    return rv;

  aResult = result;

  return NS_OK;
}

const PRUnichar*
GetTokenEnd(const nsAString& aStr, PRBool aBreakOnDot)
{
  const PRUnichar* start;
  const PRUnichar* tokenEnd;
  start = tokenEnd = aStr.BeginReading();
  const PRUnichar* const end = aStr.EndReading();
  PRBool escape = PR_FALSE;
  while (tokenEnd != end) {
    PRUnichar c = *tokenEnd;
    if (IsSpace(c) ||
       (!escape && (c == '+' || c == '-' || (aBreakOnDot && c == '.')))) {
      break;
    }
    escape = (!escape && c == '\\');
    ++tokenEnd;
  }
  return tokenEnd;
}

void
Unescape(nsAString& aStr)
{
  const PRUnichar* read = aStr.BeginReading();
  const PRUnichar* const end = aStr.EndReading();
  PRUnichar* write = aStr.BeginWriting();
  PRBool escape = PR_FALSE;

  while (read != end) {
    NS_ABORT_IF_FALSE(write <= read, "Writing past where we've read");
    if (!escape && *read == '\\') {
      escape = PR_TRUE;
      ++read;
    } else {
      *write++ = *read++;
      escape = PR_FALSE;
    }
  }

  aStr.SetLength(write - aStr.BeginReading());
}

nsresult
ParseElementBaseTimeValueSpec(const nsAString& aSpec,
                              nsSMILTimeValueSpecParams& aResult)
{
  nsSMILTimeValueSpecParams result;

  
  
  
  
  
  
  
  
  

  const PRUnichar* tokenStart = aSpec.BeginReading();
  const PRUnichar* tokenEnd = GetTokenEnd(aSpec, PR_TRUE);
  nsAutoString token(Substring(tokenStart, tokenEnd));
  Unescape(token);

  if (token.IsEmpty())
    return NS_ERROR_FAILURE;

  
  if (NS_FAILED(nsContentUtils::CheckQName(token, PR_FALSE)))
    return NS_ERROR_FAILURE;

  
  if (tokenEnd != aSpec.EndReading() && *tokenEnd == '.') {
    result.mDependentElemID = do_GetAtom(token);

    tokenStart = ++tokenEnd;
    tokenEnd = GetTokenEnd(Substring(tokenStart, aSpec.EndReading()), PR_FALSE);

    
    
    const nsAString& rawToken2 = Substring(tokenStart, tokenEnd);

    
    if (rawToken2.Equals(NS_LITERAL_STRING("begin"))) {
      result.mType = nsSMILTimeValueSpecParams::SYNCBASE;
      result.mSyncBegin = PR_TRUE;
    
    } else if (rawToken2.Equals(NS_LITERAL_STRING("end"))) {
      result.mType = nsSMILTimeValueSpecParams::SYNCBASE;
      result.mSyncBegin = PR_FALSE;
    
    } else if (StringBeginsWith(rawToken2, REPEAT_PREFIX)) {
      result.mType = nsSMILTimeValueSpecParams::REPEAT;
      if (!GetUnsignedIntAndEndParen(
            Substring(tokenStart + REPEAT_PREFIX.Length(), tokenEnd),
            result.mRepeatIterationOrAccessKey))
        return NS_ERROR_FAILURE;
    
    } else {
      nsAutoString token2(rawToken2);
      Unescape(token2);
      result.mType = nsSMILTimeValueSpecParams::EVENT;
      if (token2.IsEmpty() ||
          NS_FAILED(nsContentUtils::CheckQName(token2, PR_FALSE)))
        return NS_ERROR_FAILURE;
      result.mEventSymbol = do_GetAtom(token2);
    }
  } else {
    
    result.mType = nsSMILTimeValueSpecParams::EVENT;
    result.mEventSymbol = do_GetAtom(token);
  }

  
  
  const PRUnichar* specEnd = aSpec.EndReading();
  SkipBeginWsp(tokenEnd, specEnd);

  nsresult rv = ParseOptionalOffset(Substring(tokenEnd, specEnd), result);
  if (NS_SUCCEEDED(rv)) {
    aResult = result;
  }

  return rv;
}

} 




nsresult
nsSMILParserUtils::ParseKeySplines(const nsAString& aSpec,
                                   nsTArray<double>& aSplineArray)
{
  nsresult rv = NS_OK;

  NS_ConvertUTF16toUTF8 spec(aSpec);
  const char* start = spec.BeginReading();
  const char* end = spec.EndReading();

  SkipBeginWsp(start, end);

  int i = 0;

  while (start != end)
  {
    double value = GetFloat(start, end, &rv);
    if (NS_FAILED(rv))
      break;

    if (value > 1.0 || value < 0.0) {
      rv = NS_ERROR_FAILURE;
      break;
    }

    if (!aSplineArray.AppendElement(value)) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      break;
    }

    ++i;

    SkipBeginWsp(start, end);
    if (start == end)
      break;

    if (i % 4) {
      if (*start == ',') {
        ++start;
      }
    } else {
      if (*start != ';') {
        rv = NS_ERROR_FAILURE;
        break;
      }
      ++start;
    }

    SkipBeginWsp(start, end);
  }

  if (i % 4) {
    rv = NS_ERROR_FAILURE; 
  }

  return rv;
}

nsresult
nsSMILParserUtils::ParseKeyTimes(const nsAString& aSpec,
                                 nsTArray<double>& aTimeArray)
{
  nsresult rv = NS_OK;

  NS_ConvertUTF16toUTF8 spec(aSpec);
  const char* start = spec.BeginReading();
  const char* end = spec.EndReading();

  SkipBeginWsp(start, end);

  double previousValue = -1.0;

  while (start != end) {
    double value = GetFloat(start, end, &rv);
    if (NS_FAILED(rv))
      break;

    if (value > 1.0 || value < 0.0 || value < previousValue) {
      rv = NS_ERROR_FAILURE;
      break;
    }

    if (!aTimeArray.AppendElement(value)) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      break;
    }
    previousValue = value;

    SkipBeginWsp(start, end);
    if (start == end)
      break;

    if (*start++ != ';') {
      rv = NS_ERROR_FAILURE;
      break;
    }

    SkipBeginWsp(start, end);
  }

  return rv;
}


class SMILValueParser : public nsSMILParserUtils::GenericValueParser
{
public:
  SMILValueParser(const nsISMILAnimationElement* aSrcElement,
                  const nsISMILAttr* aSMILAttr,
                  nsTArray<nsSMILValue>* aValuesArray,
                  PRBool* aCanCache) :
    mSrcElement(aSrcElement),
    mSMILAttr(aSMILAttr),
    mValuesArray(aValuesArray),
    mCanCache(aCanCache)
  {}

  virtual nsresult Parse(const nsAString& aValueStr) {
    nsSMILValue newValue;
    PRBool tmpCanCache;
    nsresult rv = mSMILAttr->ValueFromString(aValueStr, mSrcElement,
                                             newValue, tmpCanCache);
    if (NS_FAILED(rv))
      return rv;

    if (!mValuesArray->AppendElement(newValue)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!tmpCanCache) {
      *mCanCache = PR_FALSE;
    }
    return NS_OK;
  }
protected:
  const nsISMILAnimationElement* mSrcElement;
  const nsISMILAttr* mSMILAttr;
  nsTArray<nsSMILValue>* mValuesArray;
  PRBool* mCanCache;
};

nsresult
nsSMILParserUtils::ParseValues(const nsAString& aSpec,
                               const nsISMILAnimationElement* aSrcElement,
                               const nsISMILAttr& aAttribute,
                               nsTArray<nsSMILValue>& aValuesArray,
                               PRBool& aCanCache)
{
  
  aCanCache = PR_TRUE;
  SMILValueParser valueParser(aSrcElement, &aAttribute,
                              &aValuesArray, &aCanCache);
  return ParseValuesGeneric(aSpec, valueParser);
}

nsresult
nsSMILParserUtils::ParseValuesGeneric(const nsAString& aSpec,
                                      GenericValueParser& aParser)
{
  nsresult rv = NS_ERROR_FAILURE;

  const PRUnichar* start = aSpec.BeginReading();
  const PRUnichar* end = aSpec.EndReading();
  const PRUnichar* substrEnd = nsnull;
  const PRUnichar* next = nsnull;

  while (start != end) {
    rv = NS_ERROR_FAILURE;

    SkipBeginWsp(start, end);

    if (start == end || *start == ';')
      break;

    substrEnd = start;

    while (substrEnd != end && *substrEnd != ';') {
      ++substrEnd;
    }

    next = substrEnd;
    if (*substrEnd == ';') {
      ++next;
      if (next == end)
        break;
    }

    while (substrEnd != start && NS_IS_SPACE(*(substrEnd-1)))
      --substrEnd;

    rv = aParser.Parse(Substring(start, substrEnd));
    if (NS_FAILED(rv))
      break;

    start = next;
  }

  return rv;
}

nsresult
nsSMILParserUtils::ParseRepeatCount(const nsAString& aSpec,
                                    nsSMILRepeatCount& aResult)
{
  nsresult rv = NS_OK;

  NS_ConvertUTF16toUTF8 spec(aSpec);
  const char* start = spec.BeginReading();
  const char* end = spec.EndReading();

  SkipBeginWsp(start, end);

  if (start != end)
  {
    if (ConsumeSubstring(start, end, "indefinite")) {
      aResult.SetIndefinite();
    } else {
      double value = GetFloat(start, end, &rv);

      if (NS_SUCCEEDED(rv))
      {
        
        if (value <= 0.0) {
          rv = NS_ERROR_FAILURE;
        } else {
          aResult = value;
        }
      }
    }

    
    SkipBeginWsp(start, end);
    if (start != end) {
      rv = NS_ERROR_FAILURE;
    }
  } else {
    
    rv = NS_ERROR_FAILURE;
  }

  if (NS_FAILED(rv)) {
    aResult.Unset();
  }

  return rv;
}

nsresult
nsSMILParserUtils::ParseTimeValueSpecParams(const nsAString& aSpec,
                                            nsSMILTimeValueSpecParams& aResult)
{
  nsresult rv = NS_ERROR_FAILURE;

  const PRUnichar* start = aSpec.BeginReading();
  const PRUnichar* end = aSpec.EndReading();

  SkipBeginEndWsp(start, end);
  if (start == end)
    return rv;

  const nsAString &spec = Substring(start, end);

  
  if (*start == '+' || *start == '-' || NS_IsAsciiDigit(*start)) {
    rv = ParseClockValue(spec, &aResult.mOffset, PR_TRUE);
    if (NS_SUCCEEDED(rv)) {
      aResult.mType = nsSMILTimeValueSpecParams::OFFSET;
    }
  }

  
  else if (spec.Equals(NS_LITERAL_STRING("indefinite"))) {
    aResult.mType = nsSMILTimeValueSpecParams::INDEFINITE;
    rv = NS_OK;
  }

  
  else if (StringBeginsWith(spec, WALLCLOCK_PREFIX)) {
    rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  
  else if (StringBeginsWith(spec, ACCESSKEY_PREFIX)) {
    rv = ParseAccessKey(spec, aResult);
  }

  
  else {
    rv = ParseElementBaseTimeValueSpec(spec, aResult);
  }

  return rv;
}

nsresult
nsSMILParserUtils::ParseClockValue(const nsAString& aSpec,
                                   nsSMILTimeValue* aResult,
                                   PRUint32 aFlags,   
                                   PRBool* aIsMedia)  
{
  nsSMILTime offset = 0L;
  double component = 0.0;

  PRInt8 sign = 0;
  PRUint8 colonCount = 0;

  PRBool started = PR_FALSE;
  PRBool isValid = PR_TRUE;

  PRInt32 metricMultiplicand = MSEC_PER_SEC;

  PRBool numIsReal = PR_FALSE;
  PRBool prevNumCouldBeMin = PR_FALSE;
  PRBool numCouldBeMin = PR_FALSE;
  PRBool numCouldBeSec = PR_FALSE;
  PRBool isIndefinite = PR_FALSE;

  if (aIsMedia) {
    *aIsMedia = PR_FALSE;
  }

  NS_ConvertUTF16toUTF8 spec(aSpec);
  const char* start = spec.BeginReading();
  const char* end = spec.EndReading();

  while (start != end) {
    if (IsSpace(*start)) {
      if (started) {
        ++start;
        break;
      }
      
      ++start;

    } else if ((aFlags & kClockValueAllowSign)
               && (*start == '+' || *start == '-')) {
      if (sign != 0) {
        
        isValid = PR_FALSE;
        break;
      }

      if (started) {
        
        isValid = PR_FALSE;
        break;
      }

      sign = (*start == '+') ? 1 : -1;
      ++start;
    
    } else if (NS_IS_DIGIT(*start)) {
      prevNumCouldBeMin = numCouldBeMin;

      if (!ParseClockComponent(start, end, component, numIsReal, numCouldBeMin,
                               numCouldBeSec)) {
        isValid = PR_FALSE;
        break;
      }

      started = PR_TRUE;
    } else if (*start == ':') {
      ++colonCount;

      
      if (numIsReal) {
        isValid = PR_FALSE;
        break;
      }

      
      if (!started) {
        isValid = PR_FALSE;
        break;
      }

      
      if (colonCount > 2) {
        isValid = PR_FALSE;
        break;
      }

      
      offset = offset * 60 + PRInt64(component);

      component = 0.0;
      ++start;
    } else if (NS_IS_ALPHA(*start)) {
      if (colonCount > 0) {
        isValid = PR_FALSE;
        break;
      }

      if ((aFlags & kClockValueAllowIndefinite)
          && ConsumeSubstring(start, end, "indefinite")) {
        
        
        
        isIndefinite = PR_TRUE;
        if (aResult) {
          aResult->SetIndefinite();
        }
      } else if (aIsMedia && ConsumeSubstring(start, end, "media")) {
        *aIsMedia = PR_TRUE;
      } else if (!ParseMetricMultiplicand(start, end, metricMultiplicand)) {
        isValid = PR_FALSE;
        break;
      }

      
      break;
    } else {
      isValid = PR_FALSE;
      break;
    }
  }

  if (!started) {
    isValid = PR_FALSE;
  }

  
  
  SkipBeginWsp(start, end);
  if (start != end) {
    isValid = PR_FALSE;
  }

  
  if (isIndefinite || (aIsMedia && *aIsMedia))
    return NS_OK;

  
  
  
  
  if (colonCount > 0 && (!prevNumCouldBeMin || !numCouldBeSec)) {
    isValid = PR_FALSE;
  }

  if (isValid) {
    
    if (colonCount > 0) {
      offset = offset * 60 * 1000;
      component *= 1000;
      
      component = (component >= 0) ? component + 0.5 : component - 0.5;
      offset += PRInt64(component);
    } else {
      component *= metricMultiplicand;
      
      component = (component >= 0) ? component + 0.5 : component - 0.5;
      offset = PRInt64(component);
    }

    if (aResult) {
      nsSMILTime millis = offset;

      if (sign == -1) {
        millis = -offset;
      }

      aResult->SetMillis(millis);
    }
  }

  return (isValid) ? NS_OK : NS_ERROR_FAILURE;
}

PRInt32
nsSMILParserUtils::CheckForNegativeNumber(const nsAString& aStr)
{
  PRInt32 absValLocation = -1;

  nsAString::const_iterator start, end;
  aStr.BeginReading(start);
  aStr.EndReading(end);

  
  SkipBeginWsp(start, end);

  
  if (start != end && *start == '-') {
    ++start;
    
    if (start != end && NS_IS_DIGIT(*start)) {
      absValLocation = start.get() - start.start();
    }
  }
  return absValLocation;
}
