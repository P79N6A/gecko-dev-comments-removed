























#ifndef mozilla_pkix_pkixder_h
#define mozilla_pkix_pkixder_h













#include "pkix/Input.h"
#include "pkix/pkixtypes.h"

namespace mozilla { namespace pkix { namespace der {

enum Class : uint8_t
{
   UNIVERSAL = 0 << 6,

   CONTEXT_SPECIFIC = 2 << 6,

};

enum Constructed
{
  CONSTRUCTED = 1 << 5
};

enum Tag : uint8_t
{
  BOOLEAN = UNIVERSAL | 0x01,
  INTEGER = UNIVERSAL | 0x02,
  BIT_STRING = UNIVERSAL | 0x03,
  OCTET_STRING = UNIVERSAL | 0x04,
  NULLTag = UNIVERSAL | 0x05,
  OIDTag = UNIVERSAL | 0x06,
  ENUMERATED = UNIVERSAL | 0x0a,
  UTF8String = UNIVERSAL | 0x0c,
  SEQUENCE = UNIVERSAL | CONSTRUCTED | 0x10, 
  SET = UNIVERSAL | CONSTRUCTED | 0x11, 
  PrintableString = UNIVERSAL | 0x13,
  TeletexString = UNIVERSAL | 0x14,
  IA5String = UNIVERSAL | 0x16,
  UTCTime = UNIVERSAL | 0x17,
  GENERALIZED_TIME = UNIVERSAL | 0x18,
};

enum class EmptyAllowed { No = 0, Yes = 1 };

Result ReadTagAndGetValue(Reader& input,  uint8_t& tag,
                           Input& value);
Result End(Reader& input);

inline Result
ExpectTagAndGetValue(Reader& input, uint8_t tag,  Input& value)
{
  uint8_t actualTag;
  Result rv = ReadTagAndGetValue(input, actualTag, value);
  if (rv != Success) {
    return rv;
  }
  if (tag != actualTag) {
    return Result::ERROR_BAD_DER;
  }
  return Success;
}

inline Result
ExpectTagAndGetValue(Reader& input, uint8_t tag,  Reader& value)
{
  Input valueInput;
  Result rv = ExpectTagAndGetValue(input, tag, valueInput);
  if (rv != Success) {
    return rv;
  }
  return value.Init(valueInput);
}

inline Result
ExpectTagAndEmptyValue(Reader& input, uint8_t tag)
{
  Reader value;
  Result rv = ExpectTagAndGetValue(input, tag, value);
  if (rv != Success) {
    return rv;
  }
  return End(value);
}

inline Result
ExpectTagAndSkipValue(Reader& input, uint8_t tag)
{
  Input ignoredValue;
  return ExpectTagAndGetValue(input, tag, ignoredValue);
}



inline Result
ExpectTagAndGetTLV(Reader& input, uint8_t tag,  Input& tlv)
{
  Reader::Mark mark(input.GetMark());
  Result rv = ExpectTagAndSkipValue(input, tag);
  if (rv != Success) {
    return rv;
  }
  return input.GetInput(mark, tlv);
}

inline Result
End(Reader& input)
{
  if (!input.AtEnd()) {
    return Result::ERROR_BAD_DER;
  }

  return Success;
}

template <typename Decoder>
inline Result
Nested(Reader& input, uint8_t tag, Decoder decoder)
{
  Reader nested;
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
Nested(Reader& input, uint8_t outerTag, uint8_t innerTag, Decoder decoder)
{
  Reader nestedInput;
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
NestedOf(Reader& input, uint8_t outerTag, uint8_t innerTag,
         EmptyAllowed mayBeEmpty, Decoder decoder)
{
  Reader inner;
  Result rv = ExpectTagAndGetValue(input, outerTag, inner);
  if (rv != Success) {
    return rv;
  }

  if (inner.AtEnd()) {
    if (mayBeEmpty != EmptyAllowed::Yes) {
      return Result::ERROR_BAD_DER;
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





inline Result
ExpectTagAndGetValueAtEnd(Reader& outer, uint8_t expectedTag,
                           Reader& inner)
{
  Result rv = der::ExpectTagAndGetValue(outer, expectedTag, inner);
  if (rv != Success) {
    return rv;
  }
  return der::End(outer);
}


inline Result
ExpectTagAndGetValueAtEnd(Input outer, uint8_t expectedTag,
                           Reader& inner)
{
  Reader outerReader(outer);
  return ExpectTagAndGetValueAtEnd(outerReader, expectedTag, inner);
}



namespace internal {

enum class IntegralValueRestriction
{
  NoRestriction,
  MustBePositive,
  MustBe0To127,
};

Result IntegralBytes(Reader& input, uint8_t tag,
                     IntegralValueRestriction valueRestriction,
              Input& value,
     Input::size_type* significantBytes = nullptr);



Result IntegralValue(Reader& input, uint8_t tag,  uint8_t& value);

} 

Result
BitStringWithNoUnusedBits(Reader& input,  Input& value);

inline Result
Boolean(Reader& input,  bool& value)
{
  Reader valueReader;
  Result rv = ExpectTagAndGetValue(input, BOOLEAN, valueReader);
  if (rv != Success) {
    return rv;
  }

  uint8_t intValue;
  rv = valueReader.Read(intValue);
  if (rv != Success) {
    return rv;
  }
  rv = End(valueReader);
  if (rv != Success) {
    return rv;
  }
  switch (intValue) {
    case 0: value = false; return Success;
    case 0xFF: value = true; return Success;
    default:
      return Result::ERROR_BAD_DER;
  }
}







inline Result
OptionalBoolean(Reader& input,  bool& value)
{
  value = false;
  if (input.Peek(BOOLEAN)) {
    Result rv = Boolean(input, value);
    if (rv != Success) {
      return rv;
    }
  }
  return Success;
}



inline Result
Enumerated(Reader& input, uint8_t& value)
{
  return internal::IntegralValue(input, ENUMERATED | 0, value);
}

namespace internal {







Result TimeChoice(Reader& input, uint8_t tag,  Time& time);

} 




inline Result
GeneralizedTime(Reader& input,  Time& time)
{
  return internal::TimeChoice(input, GENERALIZED_TIME, time);
}




inline Result
TimeChoice(Reader& input,  Time& time)
{
  uint8_t expectedTag = input.Peek(UTCTime) ? UTCTime : GENERALIZED_TIME;
  return internal::TimeChoice(input, expectedTag, time);
}





inline Result
PositiveInteger(Reader& input,  Input& value,
                 Input::size_type* significantBytes = nullptr)
{
  return internal::IntegralBytes(
           input, INTEGER, internal::IntegralValueRestriction::MustBePositive,
           value, significantBytes);
}



inline Result
Integer(Reader& input,  uint8_t& value)
{
  return internal::IntegralValue(input, INTEGER, value);
}





inline Result
OptionalInteger(Reader& input, long defaultValue,  long& value)
{
  
  
  if (defaultValue != -1) {
    return Result::FATAL_ERROR_INVALID_ARGS;
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
Null(Reader& input)
{
  return ExpectTagAndEmptyValue(input, NULLTag);
}

template <uint8_t Len>
Result
OID(Reader& input, const uint8_t (&expectedOid)[Len])
{
  Reader value;
  Result rv = ExpectTagAndGetValue(input, OIDTag, value);
  if (rv != Success) {
    return rv;
  }
  if (!value.MatchRest(expectedOid)) {
    return Result::ERROR_BAD_DER;
  }
  return Success;
}



inline Result
CertificateSerialNumber(Reader& input,  Input& value)
{
  
  
  
  
  
  
  
  
  
  
  return internal::IntegralBytes(
           input, INTEGER, internal::IntegralValueRestriction::NoRestriction,
           value);
}



enum class Version { v1 = 0, v2 = 1, v3 = 2, v4 = 3 };





Result OptionalVersion(Reader& input,  Version& version);

template <typename ExtensionHandler>
inline Result
OptionalExtensions(Reader& input, uint8_t tag,
                   ExtensionHandler extensionHandler)
{
  if (!input.Peek(tag)) {
    return Success;
  }

  Result rv;

  Reader extensions;
  {
    Reader tagged;
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
    Reader extension;
    rv = ExpectTagAndGetValue(extensions, SEQUENCE, extension);
    if (rv != Success) {
      return rv;
    }

    
    
    
    
    
    Reader extnID;
    rv = ExpectTagAndGetValue(extension, OIDTag, extnID);
    if (rv != Success) {
      return rv;
    }
    bool critical;
    rv = OptionalBoolean(extension, critical);
    if (rv != Success) {
      return rv;
    }
    Input extnValue;
    rv = ExpectTagAndGetValue(extension, OCTET_STRING, extnValue);
    if (rv != Success) {
      return rv;
    }
    rv = End(extension);
    if (rv != Success) {
      return rv;
    }

    bool understood = false;
    rv = extensionHandler(extnID, extnValue, critical, understood);
    if (rv != Success) {
      return rv;
    }
    if (critical && !understood) {
      return Result::ERROR_UNKNOWN_CRITICAL_EXTENSION;
    }
  }

  return Success;
}

Result DigestAlgorithmIdentifier(Reader& input,
                                  DigestAlgorithm& algorithm);

enum class PublicKeyAlgorithm
{
  RSA_PKCS1,
  ECDSA,
};

Result SignatureAlgorithmIdentifierValue(
         Reader& input,
          PublicKeyAlgorithm& publicKeyAlgorithm,
          DigestAlgorithm& digestAlgorithm);

struct SignedDataWithSignature final
{
public:
  Input data;
  Input algorithm;
  Input signature;

  void operator=(const SignedDataWithSignature&) = delete;
};



















Result SignedData(Reader& input,  Reader& tbs,
                   SignedDataWithSignature& signedDataWithSignature);

} } } 

#endif
