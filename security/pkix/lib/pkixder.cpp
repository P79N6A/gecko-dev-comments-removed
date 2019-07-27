























#include "pkixder.h"
#include "pkix/bind.h"
#include "cert.h"

namespace mozilla { namespace pkix { namespace der {

namespace internal {


Result
ExpectTagAndGetLength(Input& input, uint8_t expectedTag, uint16_t& length)
{
  PR_ASSERT((expectedTag & 0x1F) != 0x1F); 

  uint8_t tag;
  Result rv;
  rv = input.Read(tag);
  if (rv != Success) {
    return rv;
  }

  if (tag != expectedTag) {
    return Result::ERROR_BAD_DER;
  }

  
  
  
  
  uint8_t length1;
  rv = input.Read(length1);
  if (rv != Success) {
    return rv;
  }
  if (!(length1 & 0x80)) {
    length = length1;
  } else if (length1 == 0x81) {
    uint8_t length2;
    rv = input.Read(length2);
    if (rv != Success) {
      return rv;
    }
    if (length2 < 128) {
      
      return Result::ERROR_BAD_DER;
    }
    length = length2;
  } else if (length1 == 0x82) {
    rv = input.Read(length);
    if (rv != Success) {
      return rv;
    }
    if (length < 256) {
      
      return Result::ERROR_BAD_DER;
    }
  } else {
    
    return Result::ERROR_BAD_DER;
  }

  
  return input.EnsureLength(length);
}

} 

static Result
OptionalNull(Input& input)
{
  if (input.Peek(NULLTag)) {
    return Null(input);
  }
  return Success;
}

namespace {

Result
DigestAlgorithmOIDValue(Input& algorithmID,  DigestAlgorithm& algorithm)
{
  
  
  static const uint8_t id_sha1[] = {
    0x2b, 0x0e, 0x03, 0x02, 0x1a
  };
  
  static const uint8_t id_sha256[] = {
    0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01
  };
  
  static const uint8_t id_sha384[] = {
    0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02
  };
  
  static const uint8_t id_sha512[] = {
    0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03
  };

  
  
  if (algorithmID.MatchRest(id_sha1)) {
    algorithm = DigestAlgorithm::sha1;
  } else if (algorithmID.MatchRest(id_sha256)) {
    algorithm = DigestAlgorithm::sha256;
  } else if (algorithmID.MatchRest(id_sha384)) {
    algorithm = DigestAlgorithm::sha384;
  } else if (algorithmID.MatchRest(id_sha512)) {
    algorithm = DigestAlgorithm::sha512;
  } else {
    return Result::ERROR_INVALID_ALGORITHM;
  }

  return Success;
}

Result
SignatureAlgorithmOIDValue(Input& algorithmID,
                            SignatureAlgorithm& algorithm)
{
  
  
  static const uint8_t id_dsa_with_sha256[] = {
    0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x03, 0x02
  };

  
  
  static const uint8_t ecdsa_with_SHA256[] = {
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02
  };
  
  static const uint8_t ecdsa_with_SHA384[] = {
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x03
  };
  
  static const uint8_t ecdsa_with_SHA512[] = {
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x04
  };

  
  
  static const uint8_t sha256WithRSAEncryption[] = {
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b
  };
  
  static const uint8_t sha384WithRSAEncryption[] = {
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0c
  };
  
  static const uint8_t sha512WithRSAEncryption[] = {
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0d
  };

  
  
  static const uint8_t sha_1WithRSAEncryption[] = {
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05
  };

  
  
  static const uint8_t id_dsa_with_sha1[] = {
    0x2a, 0x86, 0x48, 0xce, 0x38, 0x04, 0x03
  };

  
  
  static const uint8_t ecdsa_with_SHA1[] = {
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x01
  };

  
  
  
  
  
  
  
  

  
  
  if (algorithmID.MatchRest(sha256WithRSAEncryption)) {
    algorithm = SignatureAlgorithm::rsa_pkcs1_with_sha256;
  } else if (algorithmID.MatchRest(ecdsa_with_SHA256)) {
    algorithm = SignatureAlgorithm::ecdsa_with_sha256;
  } else if (algorithmID.MatchRest(sha_1WithRSAEncryption)) {
    algorithm = SignatureAlgorithm::rsa_pkcs1_with_sha1;
  } else if (algorithmID.MatchRest(ecdsa_with_SHA1)) {
    algorithm = SignatureAlgorithm::ecdsa_with_sha1;
  } else if (algorithmID.MatchRest(ecdsa_with_SHA384)) {
    algorithm = SignatureAlgorithm::ecdsa_with_sha384;
  } else if (algorithmID.MatchRest(ecdsa_with_SHA512)) {
    algorithm = SignatureAlgorithm::ecdsa_with_sha512;
  } else if (algorithmID.MatchRest(sha384WithRSAEncryption)) {
    algorithm = SignatureAlgorithm::rsa_pkcs1_with_sha384;
  } else if (algorithmID.MatchRest(sha512WithRSAEncryption)) {
    algorithm = SignatureAlgorithm::rsa_pkcs1_with_sha512;
  } else if (algorithmID.MatchRest(id_dsa_with_sha1)) {
    algorithm = SignatureAlgorithm::dsa_with_sha1;
  } else if (algorithmID.MatchRest(id_dsa_with_sha256)) {
    algorithm = SignatureAlgorithm::dsa_with_sha256;
  } else {
    
    return Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED;
  }

  return Success;
}

template <typename OidValueParser, typename Algorithm>
Result
AlgorithmIdentifier(OidValueParser oidValueParser, Input& input,
                     Algorithm& algorithm)
{
  Input value;
  Result rv = ExpectTagAndGetValue(input, SEQUENCE, value);
  if (rv != Success) {
    return rv;
  }

  Input algorithmID;
  rv = ExpectTagAndGetValue(value, der::OIDTag, algorithmID);
  if (rv != Success) {
    return rv;
  }
  rv = oidValueParser(algorithmID, algorithm);
  if (rv != Success) {
    return rv;
  }

  rv = OptionalNull(value);
  if (rv != Success) {
    return rv;
  }

  return End(value);
}

} 

Result
SignatureAlgorithmIdentifier(Input& input,
                              SignatureAlgorithm& algorithm)
{
  return AlgorithmIdentifier(SignatureAlgorithmOIDValue, input, algorithm);
}

Result
DigestAlgorithmIdentifier(Input& input,  DigestAlgorithm& algorithm)
{
  return AlgorithmIdentifier(DigestAlgorithmOIDValue, input, algorithm);
}

Result
SignedData(Input& input,  Input& tbs,
            SignedDataWithSignature& signedData)
{
  Input::Mark mark(input.GetMark());

  Result rv;
  rv = ExpectTagAndGetValue(input, SEQUENCE, tbs);
  if (rv != Success) {
    return rv;
  }

  rv = input.GetInputBuffer(mark, signedData.data);
  if (rv != Success) {
    return rv;
  }

  rv = SignatureAlgorithmIdentifier(input, signedData.algorithm);
  if (rv != Success) {
    return rv;
  }

  rv = BitStringWithNoUnusedBits(input, signedData.signature);
  if (rv == Result::ERROR_BAD_DER) {
    rv = Result::ERROR_BAD_SIGNATURE;
  }
  return rv;
}

Result
BitStringWithNoUnusedBits(Input& input,  InputBuffer& value)
{
  Input valueWithUnusedBits;
  Result rv = ExpectTagAndGetValue(input, BIT_STRING, valueWithUnusedBits);
  if (rv != Success) {
    return rv;
  }

  uint8_t unusedBitsAtEnd;
  if (valueWithUnusedBits.Read(unusedBitsAtEnd) != Success) {
    return Result::ERROR_BAD_DER;
  }
  
  
  
  
  
  
  if (unusedBitsAtEnd != 0) {
    return Result::ERROR_BAD_DER;
  }
  Input::Mark mark(valueWithUnusedBits.GetMark());
  valueWithUnusedBits.SkipToEnd();
  return valueWithUnusedBits.GetInputBuffer(mark, value);
}

static inline Result
ReadDigit(Input& input,  int& value)
{
  uint8_t b;
  if (input.Read(b) != Success) {
    return Result::ERROR_INVALID_TIME;
  }
  if (b < '0' || b > '9') {
    return Result::ERROR_INVALID_TIME;
  }
  value = b - '0';
  return Success;
}

static inline Result
ReadTwoDigits(Input& input, int minValue, int maxValue,  int& value)
{
  int hi;
  Result rv = ReadDigit(input, hi);
  if (rv != Success) {
    return rv;
  }
  int lo;
  rv = ReadDigit(input, lo);
  if (rv != Success) {
    return rv;
  }
  value = (hi * 10) + lo;
  if (value < minValue || value > maxValue) {
    return Result::ERROR_INVALID_TIME;
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

namespace internal {






Result
TimeChoice(Input& tagged, uint8_t expectedTag,  PRTime& time)
{
  int days;

  Input input;
  Result rv = ExpectTagAndGetValue(tagged, expectedTag, input);
  if (rv != Success) {
    return rv;
  }

  int yearHi;
  int yearLo;
  if (expectedTag == GENERALIZED_TIME) {
    rv = ReadTwoDigits(input, 0, 99, yearHi);
    if (rv != Success) {
      return rv;
    }
    rv = ReadTwoDigits(input, 0, 99, yearLo);
    if (rv != Success) {
      return rv;
    }
  } else if (expectedTag == UTCTime) {
    rv = ReadTwoDigits(input, 0, 99, yearLo);
    if (rv != Success) {
      return rv;
    }
    yearHi = yearLo >= 50 ? 19 : 20;
  } else {
    PR_NOT_REACHED("invalid tag given to TimeChoice");
    return Result::ERROR_INVALID_TIME;
  }
  int year = (yearHi * 100) + yearLo;
  if (year < 1970) {
    
    return Result::ERROR_INVALID_TIME;
  }
  if (year > 1970) {
    
    
    days = daysBeforeYear(year) - daysBeforeYear(1970);
    
    
  } else {
    days = 0;
  }

  int month;
  rv = ReadTwoDigits(input, 1, 12, month);
  if (rv != Success) {
    return rv;
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
      return Result::FATAL_ERROR_INVALID_STATE;
  }

  int dayOfMonth;
  rv = ReadTwoDigits(input, 1, daysInMonth, dayOfMonth);
  if (rv != Success) {
    return rv;
  }
  days += dayOfMonth - 1;

  int hours;
  rv = ReadTwoDigits(input, 0, 23, hours);
  if (rv != Success) {
    return rv;
  }
  int minutes;
  rv = ReadTwoDigits(input, 0, 59, minutes);
  if (rv != Success) {
    return rv;
  }
  int seconds;
  rv = ReadTwoDigits(input, 0, 59, seconds);
  if (rv != Success) {
    return rv;
  }

  uint8_t b;
  if (input.Read(b) != Success) {
    return Result::ERROR_INVALID_TIME;
  }
  if (b != 'Z') {
    return Result::ERROR_INVALID_TIME;
  }
  if (End(input) != Success) {
    return Result::ERROR_INVALID_TIME;
  }

  int64_t totalSeconds = (static_cast<int64_t>(days) * 24 * 60 * 60) +
                         (static_cast<int64_t>(hours)     * 60 * 60) +
                         (static_cast<int64_t>(minutes)        * 60) +
                         seconds;

  time = totalSeconds * PR_USEC_PER_SEC;
  return Success;
}

} 

} } } 
