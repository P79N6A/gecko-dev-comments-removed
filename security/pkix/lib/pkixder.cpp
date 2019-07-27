























#include "pkixder.h"

#include "pkix/bind.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix { namespace der {

namespace internal {


Result
ExpectTagAndGetLength(Reader& input, uint8_t expectedTag, uint16_t& length)
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
OptionalNull(Reader& input)
{
  if (input.Peek(NULLTag)) {
    return Null(input);
  }
  return Success;
}

namespace {

Result
DigestAlgorithmOIDValue(Reader& algorithmID,
                         DigestAlgorithm& algorithm)
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
SignatureAlgorithmOIDValue(Reader& algorithmID,
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

  
  
  
  
  
  static const uint8_t sha1WithRSASignature[] = {
    0x2b, 0x0e, 0x03, 0x02, 0x1d
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
  } else if (algorithmID.MatchRest(sha1WithRSASignature)) {
    
    algorithm = SignatureAlgorithm::rsa_pkcs1_with_sha1;
  } else {
    
    return Result::ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED;
  }

  return Success;
}

template <typename OidValueParser, typename Algorithm>
Result
AlgorithmIdentifier(OidValueParser oidValueParser, Reader& input,
                     Algorithm& algorithm)
{
  Reader value;
  Result rv = ExpectTagAndGetValue(input, SEQUENCE, value);
  if (rv != Success) {
    return rv;
  }

  Reader algorithmID;
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
SignatureAlgorithmIdentifier(Reader& input,
                              SignatureAlgorithm& algorithm)
{
  return AlgorithmIdentifier(SignatureAlgorithmOIDValue, input, algorithm);
}

Result
DigestAlgorithmIdentifier(Reader& input,  DigestAlgorithm& algorithm)
{
  return AlgorithmIdentifier(DigestAlgorithmOIDValue, input, algorithm);
}

Result
SignedData(Reader& input,  Reader& tbs,
            SignedDataWithSignature& signedData)
{
  Reader::Mark mark(input.GetMark());

  Result rv;
  rv = ExpectTagAndGetValue(input, SEQUENCE, tbs);
  if (rv != Success) {
    return rv;
  }

  rv = input.GetInput(mark, signedData.data);
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
BitStringWithNoUnusedBits(Reader& input,  Input& value)
{
  Reader valueWithUnusedBits;
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
  Reader::Mark mark(valueWithUnusedBits.GetMark());
  valueWithUnusedBits.SkipToEnd();
  return valueWithUnusedBits.GetInput(mark, value);
}

static inline Result
ReadDigit(Reader& input,  unsigned int& value)
{
  uint8_t b;
  if (input.Read(b) != Success) {
    return Result::ERROR_INVALID_TIME;
  }
  if (b < '0' || b > '9') {
    return Result::ERROR_INVALID_TIME;
  }
  value = static_cast<unsigned int>(b - static_cast<uint8_t>('0'));
  return Success;
}

static inline Result
ReadTwoDigits(Reader& input, unsigned int minValue, unsigned int maxValue,
               unsigned int& value)
{
  unsigned int hi;
  Result rv = ReadDigit(input, hi);
  if (rv != Success) {
    return rv;
  }
  unsigned int lo;
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

namespace internal {






Result
TimeChoice(Reader& tagged, uint8_t expectedTag,  Time& time)
{
  unsigned int days;

  Reader input;
  Result rv = ExpectTagAndGetValue(tagged, expectedTag, input);
  if (rv != Success) {
    return rv;
  }

  unsigned int yearHi;
  unsigned int yearLo;
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
    yearHi = yearLo >= 50u ? 19u : 20u;
  } else {
    PR_NOT_REACHED("invalid tag given to TimeChoice");
    return Result::ERROR_INVALID_TIME;
  }
  unsigned int year = (yearHi * 100u) + yearLo;
  if (year < 1970u) {
    
    return Result::ERROR_INVALID_TIME;
  }
  days = DaysBeforeYear(year);

  unsigned int month;
  rv = ReadTwoDigits(input, 1u, 12u, month);
  if (rv != Success) {
    return rv;
  }
  unsigned int daysInMonth;
  static const unsigned int jan = 31u;
  const unsigned int feb = ((year % 4u == 0u) &&
                           ((year % 100u != 0u) || (year % 400u == 0u)))
                         ? 29u
                         : 28u;
  static const unsigned int mar = 31u;
  static const unsigned int apr = 30u;
  static const unsigned int may = 31u;
  static const unsigned int jun = 30u;
  static const unsigned int jul = 31u;
  static const unsigned int aug = 31u;
  static const unsigned int sep = 30u;
  static const unsigned int oct = 31u;
  static const unsigned int nov = 30u;
  static const unsigned int dec = 31u;
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

  unsigned int dayOfMonth;
  rv = ReadTwoDigits(input, 1u, daysInMonth, dayOfMonth);
  if (rv != Success) {
    return rv;
  }
  days += dayOfMonth - 1;

  unsigned int hours;
  rv = ReadTwoDigits(input, 0u, 23u, hours);
  if (rv != Success) {
    return rv;
  }
  unsigned int minutes;
  rv = ReadTwoDigits(input, 0u, 59u, minutes);
  if (rv != Success) {
    return rv;
  }
  unsigned int seconds;
  rv = ReadTwoDigits(input, 0u, 59u, seconds);
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

  uint64_t totalSeconds = (static_cast<uint64_t>(days) * 24u * 60u * 60u) +
                          (static_cast<uint64_t>(hours)      * 60u * 60u) +
                          (static_cast<uint64_t>(minutes)          * 60u) +
                          seconds;

  time = TimeFromElapsedSecondsAD(totalSeconds);
  return Success;
}

} 

} } } 
