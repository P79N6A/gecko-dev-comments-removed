























#include "pkixder.h"
#include "pkix/bind.h"
#include "cert.h"

namespace mozilla { namespace pkix { namespace der {


Result
Fail(PRErrorCode errorCode)
{
  PR_SetError(errorCode, 0);
  return Failure;
}

namespace internal {


Result
ExpectTagAndGetLength(Input& input, uint8_t expectedTag, uint16_t& length)
{
  PR_ASSERT((expectedTag & 0x1F) != 0x1F); 

  uint8_t tag;
  if (input.Read(tag) != Success) {
    return Failure;
  }

  if (tag != expectedTag) {
    return Fail(SEC_ERROR_BAD_DER);
  }

  
  
  
  
  uint8_t length1;
  if (input.Read(length1) != Success) {
    return Failure;
  }
  if (!(length1 & 0x80)) {
    length = length1;
  } else if (length1 == 0x81) {
    uint8_t length2;
    if (input.Read(length2) != Success) {
      return Failure;
    }
    if (length2 < 128) {
      
      return Fail(SEC_ERROR_BAD_DER);
    }
    length = length2;
  } else if (length1 == 0x82) {
    if (input.Read(length) != Success) {
      return Failure;
    }
    if (length < 256) {
      
      return Fail(SEC_ERROR_BAD_DER);
    }
  } else {
    
    return Fail(SEC_ERROR_BAD_DER);
  }

  
  return input.EnsureLength(length);
}

} 

Result
SignedData(Input& input,  Input& tbs,  CERTSignedData& signedData)
{
  Input::Mark mark(input.GetMark());

  if (ExpectTagAndGetValue(input, SEQUENCE, tbs) != Success) {
    return Failure;
  }

  if (input.GetSECItem(siBuffer, mark, signedData.data) != Success) {
    return Failure;
  }

  if (AlgorithmIdentifier(input, signedData.signatureAlgorithm) != Success) {
    return Failure;
  }

  if (ExpectTagAndGetValue(input, BIT_STRING, signedData.signature)
        != Success) {
    return Failure;
  }
  if (signedData.signature.len == 0) {
    return Fail(SEC_ERROR_BAD_SIGNATURE);
  }
  unsigned int unusedBitsAtEnd = signedData.signature.data[0];
  
  
  
  
  
  
  if (unusedBitsAtEnd != 0) {
    return Fail(SEC_ERROR_BAD_SIGNATURE);
  }
  ++signedData.signature.data;
  --signedData.signature.len;
  signedData.signature.len = (signedData.signature.len << 3); 

  return Success;
}

static inline Result
ReadDigit(Input& input,  int& value)
{
  uint8_t b;
  if (input.Read(b) != Success) {
    return Fail(SEC_ERROR_INVALID_TIME);
  }
  if (b < '0' || b > '9') {
    return Fail(SEC_ERROR_INVALID_TIME);
  }
  value = b - '0';
  return Success;
}

static inline Result
ReadTwoDigits(Input& input, int minValue, int maxValue,  int& value)
{
  int hi;
  if (ReadDigit(input, hi) != Success) {
    return Failure;
  }
  int lo;
  if (ReadDigit(input, lo) != Success) {
    return Failure;
  }
  value = (hi * 10) + lo;
  if (value < minValue || value > maxValue) {
    return Fail(SEC_ERROR_INVALID_TIME);
  }
  return Success;
}

inline int
daysBeforeYear(int year)
{
  return (365 * (year - 1))
       + ((year - 1) / 4)    
       - ((year - 1) / 100)  
       + ((year - 1) / 400); 
}






Result
TimeChoice(SECItemType type, Input& input,  PRTime& time)
{
  int days;

  int yearHi;
  int yearLo;
  if (type == siGeneralizedTime) {
    if (ReadTwoDigits(input, 0, 99, yearHi) != Success) {
      return Failure;
    }
    if (ReadTwoDigits(input, 0, 99, yearLo) != Success) {
      return Failure;
    }
  } else if (type == siUTCTime) {
    if (ReadTwoDigits(input, 0, 99, yearLo) != Success) {
      return Failure;
    }
    yearHi = yearLo >= 50 ? 19 : 20;
  } else {
    PR_NOT_REACHED("invalid tag given to TimeChoice");
    return Fail(SEC_ERROR_INVALID_TIME);
  }
  int year = (yearHi * 100) + yearLo;
  if (year < 1970) {
    
    return Fail(SEC_ERROR_INVALID_TIME); 
  }
  if (year > 1970) {
    
    
    days = daysBeforeYear(year) - daysBeforeYear(1970);
    
    
  } else {
    days = 0;
  }

  int month;
  if (ReadTwoDigits(input, 1, 12, month) != Success) {
    return Failure;
  }
  int daysInMonth;
  static const int jan = 31;
  const int feb = ((year % 4 == 0) &&
                   ((year % 100 != 0) || (year % 400 == 0)))
                ? 29
                : 28;
  static const int mar = 31;
  static const int apr = 30;
  static const int may = 31;
  static const int jun = 30;
  static const int jul = 31;
  static const int aug = 31;
  static const int sep = 30;
  static const int oct = 31;
  static const int nov = 30;
  static const int dec = 31;
  switch (month) {
    case 1:  daysInMonth = jan; break;
    case 2:  daysInMonth = feb; days += jan; break;
    case 3:  daysInMonth = mar; days += jan + feb; break;
    case 4:  daysInMonth = apr; days += jan + feb + mar; break;
    case 5:  daysInMonth = may; days += jan + feb + mar + apr; break;
    case 6:  daysInMonth = jun; days += jan + feb + mar + apr + may; break;
    case 7:  daysInMonth = jul; days += jan + feb + mar + apr + may + jun;
             break;
    case 8:  daysInMonth = aug; days += jan + feb + mar + apr + may + jun +
                                        jul;
             break;
    case 9:  daysInMonth = sep; days += jan + feb + mar + apr + may + jun +
                                        jul + aug;
             break;
    case 10: daysInMonth = oct; days += jan + feb + mar + apr + may + jun +
                                        jul + aug + sep;
             break;
    case 11: daysInMonth = nov; days += jan + feb + mar + apr + may + jun +
                                        jul + aug + sep + oct;
             break;
    case 12: daysInMonth = dec; days += jan + feb + mar + apr + may + jun +
                                        jul + aug + sep + oct + nov;
             break;
    default:
      PR_NOT_REACHED("month already bounds-checked by ReadTwoDigits");
      return Fail(PR_INVALID_STATE_ERROR);
  }

  int dayOfMonth;
  if (ReadTwoDigits(input, 1, daysInMonth, dayOfMonth) != Success) {
    return Failure;
  }
  days += dayOfMonth - 1;

  int hours;
  if (ReadTwoDigits(input, 0, 23, hours) != Success) {
    return Failure;
  }
  int minutes;
  if (ReadTwoDigits(input, 0, 59, minutes) != Success) {
    return Failure;
  }
  int seconds;
  if (ReadTwoDigits(input, 0, 59, seconds) != Success) {
    return Failure;
  }

  uint8_t b;
  if (input.Read(b) != Success) {
    return Fail(SEC_ERROR_INVALID_TIME);
  }
  if (b != 'Z') {
    return Fail(SEC_ERROR_INVALID_TIME); 
  }
  if (End(input) != Success) {
    return Fail(SEC_ERROR_INVALID_TIME);
  }

  int64_t totalSeconds = (static_cast<int64_t>(days) * 24 * 60 * 60) +
                         (static_cast<int64_t>(hours)     * 60 * 60) +
                         (static_cast<int64_t>(minutes)        * 60) +
                         seconds;

  time = totalSeconds * PR_USEC_PER_SEC;
  return Success;
}

} } } 
