























#ifndef mozilla_pkix__pkixder_h
#define mozilla_pkix__pkixder_h













#include "pkix/enumclass.h"
#include "pkix/Input.h"
#include "pkix/pkixtypes.h"
#include "prtime.h"
#include "secoidt.h"

typedef struct CERTSignedDataStr CERTSignedData;

namespace mozilla { namespace pkix { namespace der {

enum Class
{
   UNIVERSAL = 0 << 6,

   CONTEXT_SPECIFIC = 2 << 6,

};

enum Constructed
{
  CONSTRUCTED = 1 << 5
};

enum Tag
{
  BOOLEAN = UNIVERSAL | 0x01,
  INTEGER = UNIVERSAL | 0x02,
  BIT_STRING = UNIVERSAL | 0x03,
  OCTET_STRING = UNIVERSAL | 0x04,
  NULLTag = UNIVERSAL | 0x05,
  OIDTag = UNIVERSAL | 0x06,
  ENUMERATED = UNIVERSAL | 0x0a,
  SEQUENCE = UNIVERSAL | CONSTRUCTED | 0x10, 
  UTCTime = UNIVERSAL | 0x17,
  GENERALIZED_TIME = UNIVERSAL | 0x18,
};

MOZILLA_PKIX_ENUM_CLASS EmptyAllowed { No = 0, Yes = 1 };

inline Result
ExpectTagAndLength(Input& input, uint8_t expectedTag, uint8_t expectedLength)
{
  PR_ASSERT((expectedTag & 0x1F) != 0x1F); 
  PR_ASSERT(expectedLength < 128); 

  uint16_t tagAndLength;
  Result rv = input.Read(tagAndLength);
  if (rv != Success) {
    return rv;
  }

  uint16_t expectedTagAndLength = static_cast<uint16_t>(expectedTag << 8);
  expectedTagAndLength |= expectedLength;

  if (tagAndLength != expectedTagAndLength) {
    return Fail(SEC_ERROR_BAD_DER);
  }

  return Success;
}

namespace internal {

Result
ExpectTagAndGetLength(Input& input, uint8_t expectedTag, uint16_t& length);

} 

inline Result
ExpectTagAndSkipLength(Input& input, uint8_t expectedTag)
{
  uint16_t ignored;
  return internal::ExpectTagAndGetLength(input, expectedTag, ignored);
}

inline Result
ExpectTagAndSkipValue(Input& input, uint8_t tag)
{
  uint16_t length;
  Result rv = internal::ExpectTagAndGetLength(input, tag, length);
  if (rv != Success) {
    return rv;
  }
  return input.Skip(length);
}

inline Result
ExpectTagAndGetValue(Input& input, uint8_t tag,  SECItem& value)
{
  uint16_t length;
  Result rv = internal::ExpectTagAndGetLength(input, tag, length);
  if (rv != Success) {
    return rv;
  }
  return input.Skip(length, value);
}

inline Result
ExpectTagAndGetValue(Input& input, uint8_t tag,  Input& value)
{
  uint16_t length;
  Result rv = internal::ExpectTagAndGetLength(input, tag, length);
  if (rv != Success) {
    return rv;
  }
  return input.Skip(length, value);
}



inline Result
ExpectTagAndGetTLV(Input& input, uint8_t tag,  SECItem& tlv)
{
  Input::Mark mark(input.GetMark());
  uint16_t length;
  Result rv = internal::ExpectTagAndGetLength(input, tag, length);
  if (rv != Success) {
    return rv;
  }
  rv = input.Skip(length);
  if (rv != Success) {
    return rv;
  }
  return input.GetSECItem(siBuffer, mark, tlv);
}

inline Result
End(Input& input)
{
  if (!input.AtEnd()) {
    return Fail(SEC_ERROR_BAD_DER);
  }

  return Success;
}

template <typename Decoder>
inline Result
Nested(Input& input, uint8_t tag, Decoder decoder)
{
  Input nested;
  Result rv = ExpectTagAndGetValue(input, tag, nested);
  if (rv != Success) {
    return rv;
  }
  rv = decoder(nested);
  if (rv != Success) {
    return rv;
  }
  return End(nested);
}

template <typename Decoder>
inline Result
Nested(Input& input, uint8_t outerTag, uint8_t innerTag, Decoder decoder)
{
  
  

  Input nestedInput;
  Result rv = ExpectTagAndGetValue(input, outerTag, nestedInput);
  if (rv != Success) {
    return rv;
  }
  rv = Nested(nestedInput, innerTag, decoder);
  if (rv != Success) {
    return rv;
  }
  return End(nestedInput);
}


















template <typename Decoder>
inline Result
NestedOf(Input& input, uint8_t outerTag, uint8_t innerTag,
         EmptyAllowed mayBeEmpty, Decoder decoder)
{
  Input inner;
  Result rv = ExpectTagAndGetValue(input, outerTag, inner);
  if (rv != Success) {
    return rv;
  }

  if (inner.AtEnd()) {
    if (mayBeEmpty != EmptyAllowed::Yes) {
      return Fail(SEC_ERROR_BAD_DER);
    }
    return Success;
  }

  do {
    rv = Nested(inner, innerTag, decoder);
    if (rv != Success) {
      return rv;
    }
  } while (!inner.AtEnd());

  return Success;
}



namespace internal {



template <typename T> inline Result
IntegralValue(Input& input, uint8_t tag, T& value)
{
  
  
  
  Result rv = ExpectTagAndLength(input, tag, 1);
  if (rv != Success) {
    return rv;
  }
  uint8_t valueByte;
  rv = input.Read(valueByte);
  if (rv != Success) {
    return rv;
  }
  if (valueByte & 0x80) { 
    return Fail(SEC_ERROR_BAD_DER);
  }
  value = valueByte;
  return Success;
}

} 

inline Result
Boolean(Input& input,  bool& value)
{
  Result rv = ExpectTagAndLength(input, BOOLEAN, 1);
  if (rv != Success) {
    return rv;
  }

  uint8_t intValue;
  rv = input.Read(intValue);
  if (rv != Success) {
    return rv;
  }
  switch (intValue) {
    case 0: value = false; return Success;
    case 0xFF: value = true; return Success;
    default:
      return Fail(SEC_ERROR_BAD_DER);
  }
}





inline Result
OptionalBoolean(Input& input, bool allowInvalidExplicitEncoding,
                 bool& value)
{
  value = false;
  if (input.Peek(BOOLEAN)) {
    Result rv = Boolean(input, value);
    if (rv != Success) {
      return rv;
    }
    if (!allowInvalidExplicitEncoding && !value) {
      return Fail(SEC_ERROR_BAD_DER);
    }
  }
  return Success;
}



inline Result
Enumerated(Input& input, uint8_t& value)
{
  return internal::IntegralValue(input, ENUMERATED | 0, value);
}

namespace internal {







Result TimeChoice(Input& input, uint8_t tag,  PRTime& time);

} 




inline Result
GeneralizedTime(Input& input,  PRTime& time)
{
  return internal::TimeChoice(input, GENERALIZED_TIME, time);
}




inline Result
TimeChoice(Input& input,  PRTime& time)
{
  uint8_t expectedTag = input.Peek(UTCTime) ? UTCTime : GENERALIZED_TIME;
  return internal::TimeChoice(input, expectedTag, time);
}



inline Result
Integer(Input& input,  uint8_t& value)
{
  return internal::IntegralValue(input, INTEGER, value);
}





inline Result
OptionalInteger(Input& input, long defaultValue,  long& value)
{
  
  
  if (defaultValue != -1) {
    return Fail(SEC_ERROR_INVALID_ARGS);
  }

  if (!input.Peek(INTEGER)) {
    value = defaultValue;
    return Success;
  }

  uint8_t parsedValue;
  Result rv = Integer(input, parsedValue);
  if (rv != Success) {
    return rv;
  }
  value = parsedValue;
  return Success;
}

inline Result
Null(Input& input)
{
  return ExpectTagAndLength(input, NULLTag, 0);
}

template <uint8_t Len>
Result
OID(Input& input, const uint8_t (&expectedOid)[Len])
{
  Result rv = ExpectTagAndLength(input, OIDTag, Len);
  if (rv != Success) {
    return rv;
  }

  return input.Expect(expectedOid, Len);
}



inline Result
CertificateSerialNumber(Input& input,  SECItem& value)
{
  
  
  
  
  
  
  
  
  
  

  Result rv = ExpectTagAndGetValue(input, INTEGER, value);
  if (rv != Success) {
    return rv;
  }

  if (value.len == 0) {
    return Fail(SEC_ERROR_BAD_DER);
  }

  
  
  
  
  
  if (value.len > 1) {
    if ((value.data[0] == 0x00 && (value.data[1] & 0x80) == 0) ||
        (value.data[0] == 0xff && (value.data[1] & 0x80) != 0)) {
      return Fail(SEC_ERROR_BAD_DER);
    }
  }

  return Success;
}



MOZILLA_PKIX_ENUM_CLASS Version { v1 = 0, v2 = 1, v3 = 2 };




inline Result
OptionalVersion(Input& input,  Version& version)
{
  static const uint8_t TAG = CONTEXT_SPECIFIC | CONSTRUCTED | 0;
  if (!input.Peek(TAG)) {
    version = Version::v1;
    return Success;
  }
  Input value;
  Result rv = ExpectTagAndGetValue(input, TAG, value);
  if (rv != Success) {
    return rv;
  }
  uint8_t integerValue;
  rv = Integer(value, integerValue);
  if (rv != Success) {
    return rv;
  }
  rv = End(value);
  if (rv != Success) {
    return rv;
  }
  switch (integerValue) {
    case static_cast<uint8_t>(Version::v3): version = Version::v3; break;
    case static_cast<uint8_t>(Version::v2): version = Version::v2; break;
    
    
    case static_cast<uint8_t>(Version::v1): version = Version::v1; break;
    default:
      return Fail(SEC_ERROR_BAD_DER);
  }
  return Success;
}

template <typename ExtensionHandler>
inline Result
OptionalExtensions(Input& input, uint8_t tag, ExtensionHandler extensionHandler)
{
  if (!input.Peek(tag)) {
    return Success;
  }

  Result rv;

  Input extensions;
  {
    Input tagged;
    rv = ExpectTagAndGetValue(input, tag, tagged);
    if (rv != Success) {
      return rv;
    }
    rv = ExpectTagAndGetValue(tagged, SEQUENCE, extensions);
    if (rv != Success) {
      return rv;
    }
    rv = End(tagged);
    if (rv != Success) {
      return rv;
    }
  }

  
  
  
  
  
  while (!extensions.AtEnd()) {
    Input extension;
    rv = ExpectTagAndGetValue(extensions, SEQUENCE, extension);
    if (rv != Success) {
      return rv;
    }

    
    
    
    
    
    Input extnID;
    rv = ExpectTagAndGetValue(extension, OIDTag, extnID);
    if (rv != Success) {
      return rv;
    }
    bool critical;
    rv = OptionalBoolean(extension, false, critical);
    if (rv != Success) {
      return rv;
    }
    SECItem extnValue;
    rv = ExpectTagAndGetValue(extension, OCTET_STRING, extnValue);
    if (rv != Success) {
      return rv;
    }
    rv = End(extension);
    if (rv != Success) {
      return rv;
    }

    bool understood = false;
    rv = extensionHandler(extnID, extnValue, understood);
    if (rv != Success) {
      return rv;
    }
    if (critical && !understood) {
      return Fail(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION);
    }
  }

  return Success;
}

Result DigestAlgorithmIdentifier(Input& input,
                                  DigestAlgorithm& algorithm);

Result SignatureAlgorithmIdentifier(Input& input,
                                     SignatureAlgorithm& algorithm);

















Result SignedData(Input& input,  Input& tbs,
                   SignedDataWithSignature& signedDataWithSignature);

} } } 

#endif
