























#ifndef mozilla_pkix__pkixder_h
#define mozilla_pkix__pkixder_h













#include "pkix/enumclass.h"
#include "pkix/nullptr.h"

#include "prerror.h"
#include "prlog.h"
#include "secder.h"
#include "secerr.h"
#include "secoidt.h"
#include "stdint.h"

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
  GENERALIZED_TIME = UNIVERSAL | 0x18,
  SEQUENCE = UNIVERSAL | CONSTRUCTED | 0x30,
};

enum Result
{
  Failure = -1,
  Success = 0
};

MOZILLA_PKIX_ENUM_CLASS EmptyAllowed { No = 0, Yes = 1 };

Result Fail(PRErrorCode errorCode);

class Input
{
public:
  Input()
    : input(nullptr)
    , end(nullptr)
  {
  }

  Result Init(const uint8_t* data, size_t len)
  {
    if (input) {
      
      return Fail(SEC_ERROR_INVALID_ARGS);
    }
    if (!data || len > 0xffffu) {
      
      return Fail(SEC_ERROR_BAD_DER);
    }

    
    
    this->input = data;
    this->end = data + len;

    return Success;
  }

  Result Expect(const uint8_t* expected, uint16_t expectedLen)
  {
    if (EnsureLength(expectedLen) != Success) {
      return Failure;
    }
    if (memcmp(input, expected, expectedLen)) {
      return Fail(SEC_ERROR_BAD_DER);
    }
    input += expectedLen;
    return Success;
  }

  bool Peek(uint8_t expectedByte) const
  {
    return input < end && *input == expectedByte;
  }

  Result Read(uint8_t& out)
  {
    if (EnsureLength(1) != Success) {
      return Failure;
    }
    out = *input++;
    return Success;
  }

  Result Read(uint16_t& out)
  {
    if (EnsureLength(2) != Success) {
      return Failure;
    }
    out = *input++;
    out <<= 8u;
    out |= *input++;
    return Success;
  }

  template <uint16_t N>
  bool MatchBytes(const uint8_t (&toMatch)[N])
  {
    if (EnsureLength(N) != Success) {
      return false;
    }
    if (memcmp(input, toMatch, N)) {
      return false;
    }
    input += N;
    return true;
  }

  template <uint16_t N>
  bool MatchTLV(uint8_t tag, uint16_t len, const uint8_t (&value)[N])
  {
    static_assert(N <= 127, "buffer larger than largest length supported");
    if (len > N) {
      PR_NOT_REACHED("overflow prevented dynamically instead of statically");
      return false;
    }
    uint16_t totalLen = 2u + len;
    if (EnsureLength(totalLen) != Success) {
      return false;
    }
    if (*input != tag) {
      return false;
    }
    if (*(input + 1) != len) {
      return false;
    }
    if (memcmp(input + 2, value, len)) {
      return false;
    }
    input += totalLen;
    return true;
  }

  Result Skip(uint16_t len)
  {
    if (EnsureLength(len) != Success) {
      return Failure;
    }
    input += len;
    return Success;
  }

  Result Skip(uint16_t len, Input& skippedInput)
  {
    if (EnsureLength(len) != Success) {
      return Failure;
    }
    if (skippedInput.Init(input, len) != Success) {
      return Failure;
    }
    input += len;
    return Success;
  }

  Result Skip(uint16_t len, SECItem& skippedItem)
  {
    if (EnsureLength(len) != Success) {
      return Failure;
    }
    skippedItem.type = siBuffer;
    skippedItem.data = const_cast<uint8_t*>(input);
    skippedItem.len = len;
    input += len;
    return Success;
  }

  void SkipToEnd()
  {
    input = end;
  }

  Result EnsureLength(uint16_t len)
  {
    if (static_cast<size_t>(end - input) < len) {
      return Fail(SEC_ERROR_BAD_DER);
    }
    return Success;
  }

  bool AtEnd() const { return input == end; }

  class Mark
  {
  private:
    friend class Input;
    Mark(const Input& input, const uint8_t* mark) : input(input), mark(mark) { }
    const Input& input;
    const uint8_t* const mark;
    void operator=(const Mark&) ;
  };

  Mark GetMark() const { return Mark(*this, input); }

  Result GetSECItem(SECItemType type, const Mark& mark,  SECItem& item)
  {
    if (&mark.input != this || mark.mark > input) {
      PR_NOT_REACHED("invalid mark");
      return Fail(SEC_ERROR_INVALID_ARGS);
    }
    item.type = type;
    item.data = const_cast<uint8_t*>(mark.mark);
    item.len = static_cast<decltype(item.len)>(input - mark.mark);
    return Success;
  }

private:
  const uint8_t* input;
  const uint8_t* end;

  Input(const Input&) ;
  void operator=(const Input&) ;
};

inline Result
ExpectTagAndLength(Input& input, uint8_t expectedTag, uint8_t expectedLength)
{
  PR_ASSERT((expectedTag & 0x1F) != 0x1F); 
  PR_ASSERT(expectedLength < 128); 

  uint16_t tagAndLength;
  if (input.Read(tagAndLength) != Success) {
    return Failure;
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
  if (internal::ExpectTagAndGetLength(input, tag, length) != Success) {
    return Failure;
  }
  return input.Skip(length);
}

inline Result
ExpectTagAndGetValue(Input& input, uint8_t tag,  SECItem& value)
{
  uint16_t length;
  if (internal::ExpectTagAndGetLength(input, tag, length) != Success) {
    return Failure;
  }
  return input.Skip(length, value);
}

inline Result
ExpectTagAndGetValue(Input& input, uint8_t tag,  Input& value)
{
  uint16_t length;
  if (internal::ExpectTagAndGetLength(input, tag, length) != Success) {
    return Failure;
  }
  return input.Skip(length, value);
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
  if (ExpectTagAndGetValue(input, tag, nested) != Success) {
    return Failure;
  }
  if (decoder(nested) != Success) {
    return Failure;
  }
  return End(nested);
}

template <typename Decoder>
inline Result
Nested(Input& input, uint8_t outerTag, uint8_t innerTag, Decoder decoder)
{
  
  

  Input nestedInput;
  if (ExpectTagAndGetValue(input, outerTag, nestedInput) != Success) {
    return Failure;
  }
  if (Nested(nestedInput, innerTag, decoder) != Success) {
    return Failure;
  }
  return End(nestedInput);
}


















template <typename Decoder>
inline Result
NestedOf(Input& input, uint8_t outerTag, uint8_t innerTag,
         EmptyAllowed mayBeEmpty, Decoder decoder)
{
  Input inner;
  if (ExpectTagAndGetValue(input, outerTag, inner) != Success) {
    return Failure;
  }

  if (inner.AtEnd()) {
    if (mayBeEmpty != EmptyAllowed::Yes) {
      return Fail(SEC_ERROR_BAD_DER);
    }
    return Success;
  }

  do {
    if (Nested(inner, innerTag, decoder) != Success) {
      return Failure;
    }
  } while (!inner.AtEnd());

  return Success;
}



namespace internal {



template <typename T> inline Result
IntegralValue(Input& input, uint8_t tag, T& value)
{
  
  
  
  if (ExpectTagAndLength(input, tag, 1) != Success) {
    return Failure;
  }
  uint8_t valueByte;
  if (input.Read(valueByte) != Success) {
    return Failure;
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
  if (ExpectTagAndLength(input, BOOLEAN, 1) != Success) {
    return Failure;
  }

  uint8_t intValue;
  if (input.Read(intValue) != Success) {
    return Failure;
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
    if (Boolean(input, value) != Success) {
      return Failure;
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

inline Result
GeneralizedTime(Input& input, PRTime& time)
{
  SECItem encoded;
  if (ExpectTagAndGetValue(input, GENERALIZED_TIME, encoded) != Success) {
    return Failure;
  }
  if (DER_GeneralizedTimeToTime(&time, &encoded) != SECSuccess) {
    return Failure;
  }
  return Success;
}



inline Result
Integer(Input& input,  uint8_t& value)
{
  if (internal::IntegralValue(input, INTEGER, value) != Success) {
    return Failure;
  }
  return Success;
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
  if (Integer(input, parsedValue) != Success) {
    return Failure;
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
  if (ExpectTagAndLength(input, OIDTag, Len) != Success) {
    return Failure;
  }

  return input.Expect(expectedOid, Len);
}






inline Result
AlgorithmIdentifier(Input& input, SECAlgorithmID& algorithmID)
{
  if (ExpectTagAndGetValue(input, OIDTag, algorithmID.algorithm) != Success) {
    return Failure;
  }
  algorithmID.parameters.data = nullptr;
  algorithmID.parameters.len = 0;
  if (input.AtEnd()) {
    return Success;
  }
  return Null(input);
}

inline Result
CertificateSerialNumber(Input& input,  SECItem& value)
{
  
  
  
  
  
  
  
  
  
  

  if (ExpectTagAndGetValue(input, INTEGER, value) != Success) {
    return Failure;
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



enum Version { v1 = 0, v2 = 1, v3 = 2 };




inline Result
OptionalVersion(Input& input,  uint8_t& version)
{
  const uint8_t tag = CONTEXT_SPECIFIC | CONSTRUCTED | 0;
  if (!input.Peek(tag)) {
    version = v1;
    return Success;
  }
  if (ExpectTagAndLength(input, tag, 3) != Success) {
    return Failure;
  }
  if (ExpectTagAndLength(input, INTEGER, 1) != Success) {
    return Failure;
  }
  if (input.Read(version) != Success) {
    return Failure;
  }
  if (version & 0x80) { 
    return Fail(SEC_ERROR_BAD_DER);
  }
  return Success;
}

} } } 

#endif
