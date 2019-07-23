




































#include "nsSMILParserUtils.h"
#include "nsISMILAttr.h"
#include "nsSMILValue.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTypes.h"
#include "nsSMILRepeatCount.h"
#include "nsString.h"
#include "prdtoa.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "prlong.h"




namespace {

const PRUint32 MSEC_PER_SEC  = 1000;
const PRUint32 MSEC_PER_MIN  = 1000 * 60;
const PRUint32 MSEC_PER_HOUR = 1000 * 60 * 60;



inline PRBool
IsSpace(const PRUnichar c)
{
  return (c == 0x9 || c == 0xA || c == 0xD || c == 0x20);
}

template<class T>
inline void
SkipWsp(T& aStart, T aEnd)
{
  while (aStart != aEnd && IsSpace(*aStart)) {
    ++aStart;
  }
}

double
GetFloat(const char*& aStart, const char* aEnd, nsresult* aErrorCode)
{
  char *floatEnd;
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

} 




nsresult
nsSMILParserUtils::ParseKeySplines(const nsAString& aSpec,
                                   nsTArray<double>& aSplineArray)
{
  nsresult rv = NS_OK;

  NS_ConvertUTF16toUTF8 spec(aSpec);
  const char* start = spec.BeginReading();
  const char* end = spec.EndReading();

  SkipWsp(start, end);

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

    SkipWsp(start, end);
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

    SkipWsp(start, end);
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

  SkipWsp(start, end);

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

    SkipWsp(start, end);
    if (start == end)
      break;

    if (*start++ != ';') {
      rv = NS_ERROR_FAILURE;
      break;
    }

    SkipWsp(start, end);
  }

  return rv;
}

nsresult
nsSMILParserUtils::ParseValues(const nsAString& aSpec,
                               const nsISMILAnimationElement* aSrcElement,
                               const nsISMILAttr& aAttribute,
                               nsTArray<nsSMILValue>& aValuesArray)
{
  nsresult rv = NS_ERROR_FAILURE;

  const PRUnichar* start = aSpec.BeginReading();
  const PRUnichar* end = aSpec.EndReading();
  const PRUnichar* substrEnd = nsnull;
  const PRUnichar* next = nsnull;

  while (start != end) {
    rv = NS_ERROR_FAILURE;

    SkipWsp(start, end);

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

    nsSMILValue newValue;
    rv = aAttribute.ValueFromString(Substring(start, substrEnd),
                                    aSrcElement, newValue);
    if (NS_FAILED(rv))
      break;

    if (!aValuesArray.AppendElement(newValue)) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      break;
    }

    rv = NS_OK;
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

  SkipWsp(start, end);

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

    
    SkipWsp(start, end);
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

  
  
  SkipWsp(start, end);
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

  
  SkipWsp(start, end);

  
  if (start != end && *start == '-') {
    ++start;
    
    if (start != end && NS_IS_DIGIT(*start)) {
      absValLocation = start.get() - start.start();
    }
  }
  return absValLocation;
}
