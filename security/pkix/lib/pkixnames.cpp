




































#include "pkix/bind.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

namespace {











MOZILLA_PKIX_ENUM_CLASS GeneralNameType : uint8_t
{
  dNSName = der::CONTEXT_SPECIFIC | 2,
  iPAddress = der::CONTEXT_SPECIFIC | 7,
};

MOZILLA_PKIX_ENUM_CLASS FallBackToCommonName { No = 0, Yes = 1 };

Result SearchForName(const Input* subjectAltName, Input subject,
                     GeneralNameType referenceIDType,
                     Input referenceID,
                     FallBackToCommonName fallBackToCommonName,
                      bool& foundMatch);
Result SearchWithinRDN(Reader& rdn,
                       GeneralNameType referenceIDType,
                       Input referenceID,
                        bool& foundMatch);
Result SearchWithinAVA(Reader& rdn,
                       GeneralNameType referenceIDType,
                       Input referenceID,
                        bool& foundMatch);

Result MatchPresentedIDWithReferenceID(GeneralNameType referenceIDType,
                                       Input presentedID,
                                       Input referenceID,
                                        bool& foundMatch);

uint8_t LocaleInsensitveToLower(uint8_t a);
bool StartsWithIDNALabel(Input id);

MOZILLA_PKIX_ENUM_CLASS ValidDNSIDMatchType
{
  ReferenceID = 0,
  PresentedID = 1,
};

bool IsValidDNSID(Input hostname, ValidDNSIDMatchType matchType);

} 

bool IsValidReferenceDNSID(Input hostname);
bool IsValidPresentedDNSID(Input hostname);
bool ParseIPv4Address(Input hostname,  uint8_t (&out)[4]);
bool ParseIPv6Address(Input hostname,  uint8_t (&out)[16]);
bool PresentedDNSIDMatchesReferenceDNSID(Input presentedDNSID,
                                         Input referenceDNSID);





Result
CheckCertHostname(Input endEntityCertDER, Input hostname)
{
  BackCert cert(endEntityCertDER, EndEntityOrCA::MustBeEndEntity, nullptr);
  Result rv = cert.Init();
  if (rv != Success) {
    return rv;
  }

  const Input* subjectAltName(cert.GetSubjectAltName());
  Input subject(cert.GetSubject());

  
  
  
  
  
  
  
  
  
  bool found;
  uint8_t ipv6[16];
  uint8_t ipv4[4];
  if (IsValidReferenceDNSID(hostname)) {
    rv = SearchForName(subjectAltName, subject, GeneralNameType::dNSName,
                       hostname, FallBackToCommonName::Yes, found);
  } else if (ParseIPv6Address(hostname, ipv6)) {
    rv = SearchForName(subjectAltName, subject, GeneralNameType::iPAddress,
                       Input(ipv6), FallBackToCommonName::No, found);
  } else if (ParseIPv4Address(hostname, ipv4)) {
    rv = SearchForName(subjectAltName, subject, GeneralNameType::iPAddress,
                       Input(ipv4), FallBackToCommonName::Yes, found);
  } else {
    return Result::ERROR_BAD_CERT_DOMAIN;
  }
  if (rv != Success) {
    return rv;
  }
  if (!found) {
    return Result::ERROR_BAD_CERT_DOMAIN;
  }
  return Success;
}

namespace {

Result
SearchForName( const Input* subjectAltName,
              Input subject,
              GeneralNameType referenceIDType,
              Input referenceID,
              FallBackToCommonName fallBackToCommonName,
               bool& foundMatch)
{
  Result rv;

  foundMatch = false;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool hasAtLeastOneDNSNameOrIPAddressSAN = false;

  if (subjectAltName) {
    Reader altNames;
    rv = der::ExpectTagAndGetValueAtEnd(*subjectAltName, der::SEQUENCE,
                                        altNames);
    if (rv != Success) {
      return rv;
    }

    
    do {
      uint8_t tag;
      Input presentedID;
      rv = der::ReadTagAndGetValue(altNames, tag, presentedID);
      if (rv != Success) {
        return rv;
      }
      if (tag == static_cast<uint8_t>(referenceIDType)) {
        rv = MatchPresentedIDWithReferenceID(referenceIDType, presentedID,
                                             referenceID, foundMatch);
        if (rv != Success) {
          return rv;
        }
        if (foundMatch) {
          return Success;
        }
      }
      if (tag == static_cast<uint8_t>(GeneralNameType::dNSName) ||
          tag == static_cast<uint8_t>(GeneralNameType::iPAddress)) {
        hasAtLeastOneDNSNameOrIPAddressSAN = true;
      }
    } while (!altNames.AtEnd());
  }

  if (hasAtLeastOneDNSNameOrIPAddressSAN ||
      fallBackToCommonName != FallBackToCommonName::Yes) {
    return Success;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  Reader subjectReader(subject);
  return der::NestedOf(subjectReader, der::SEQUENCE, der::SET,
                       der::EmptyAllowed::Yes,
                       bind(SearchWithinRDN, _1, referenceIDType,
                            referenceID, ref(foundMatch)));
}







Result
SearchWithinRDN(Reader& rdn,
                GeneralNameType referenceIDType,
                Input referenceID,
                 bool& foundMatch)
{
  do {
    Result rv = der::Nested(rdn, der::SEQUENCE,
                            bind(SearchWithinAVA, _1, referenceIDType,
                                 referenceID, ref(foundMatch)));
    if (rv != Success) {
      return rv;
    }
  } while (!rdn.AtEnd());

  return Success;
}















Result
SearchWithinAVA(Reader& rdn,
                GeneralNameType referenceIDType,
                Input referenceID,
                 bool& foundMatch)
{
  
  
  
  static const uint8_t id_at_commonName[] = {
    0x55, 0x04, 0x03
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  Reader type;
  Result rv = der::ExpectTagAndGetValue(rdn, der::OIDTag, type);
  if (rv != Success) {
    return rv;
  }

  
  if (!type.MatchRest(id_at_commonName)) {
    rdn.SkipToEnd();
    return Success;
  }

  
  
  
  foundMatch = false;

  uint8_t valueEncodingTag;
  Input presentedID;
  rv = der::ReadTagAndGetValue(rdn, valueEncodingTag, presentedID);
  if (rv != Success) {
    return rv;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (valueEncodingTag != der::PrintableString &&
      valueEncodingTag != der::UTF8String &&
      valueEncodingTag != der::TeletexString) {
    return Success;
  }

  switch (referenceIDType)
  {
    case GeneralNameType::dNSName:
      foundMatch = PresentedDNSIDMatchesReferenceDNSID(presentedID,
                                                       referenceID);
      break;
    case GeneralNameType::iPAddress:
    {
      
      
      assert(referenceID.GetLength() == 4);
      uint8_t ipv4[4];
      foundMatch = ParseIPv4Address(presentedID, ipv4) &&
                   InputsAreEqual(Input(ipv4), referenceID);
      break;
    }
    default:
      return NotReached("unexpected referenceIDType in SearchWithinAVA",
                        Result::FATAL_ERROR_INVALID_ARGS);
  }

  return Success;
}

Result
MatchPresentedIDWithReferenceID(GeneralNameType nameType,
                                Input presentedID,
                                Input referenceID,
                                 bool& foundMatch)
{
  foundMatch = false;

  switch (nameType) {
    case GeneralNameType::dNSName:
      foundMatch = PresentedDNSIDMatchesReferenceDNSID(presentedID,
                                                       referenceID);
      break;
    case GeneralNameType::iPAddress:
      foundMatch = InputsAreEqual(presentedID, referenceID);
      break;
    default:
      return NotReached("Invalid nameType for SearchType::CheckName",
                        Result::FATAL_ERROR_INVALID_ARGS);
  }
  return Success;
}

} 














bool
PresentedDNSIDMatchesReferenceDNSID(Input presentedDNSID, Input referenceDNSID)
{
  if (!IsValidPresentedDNSID(presentedDNSID)) {
    return false;
  }
  if (!IsValidReferenceDNSID(referenceDNSID)) {
    return false;
  }

  Reader presented(presentedDNSID);
  Reader reference(referenceDNSID);
  bool isFirstPresentedByte = true;
  do {
    uint8_t presentedByte;
    Result rv = presented.Read(presentedByte);
    if (rv != Success) {
      return false;
    }
    if (presentedByte == '*') {
      
      
      
      
      
      
      
      do {
        uint8_t referenceByte;
        rv = reference.Read(referenceByte);
        if (rv != Success) {
          return false;
        }
      } while (!reference.Peek('.'));

      
      
      
      
      
      
      
      if (!isFirstPresentedByte && StartsWithIDNALabel(referenceDNSID)) {
        return false;
      }
    } else {
      
      
      if (reference.AtEnd() && presented.AtEnd() && presentedByte == '.') {
        return true;
      }

      uint8_t referenceByte;
      rv = reference.Read(referenceByte);
      if (rv != Success) {
        return false;
      }
      if (LocaleInsensitveToLower(presentedByte) !=
          LocaleInsensitveToLower(referenceByte)) {
        return false;
      }
    }
    isFirstPresentedByte = false;
  } while (!presented.AtEnd());

  
  if (!reference.AtEnd()) {
    uint8_t referenceByte;
    Result rv = reference.Read(referenceByte);
    if (rv != Success) {
      return false;
    }
    if (referenceByte != '.') {
      return false;
    }
    if (!reference.AtEnd()) {
      return false;
    }
  }

  return true;
}

namespace {



inline uint8_t
LocaleInsensitveToLower(uint8_t a)
{
  if (a >= 'A' && a <= 'Z') { 
    return static_cast<uint8_t>(
             static_cast<uint8_t>(a - static_cast<uint8_t>('A')) +
             static_cast<uint8_t>('a'));
  }
  return a;
}

bool
StartsWithIDNALabel(Input id)
{
  static const uint8_t IDN_ALABEL_PREFIX[4] = { 'x', 'n', '-', '-' };
  Reader input(id);
  for (size_t i = 0; i < sizeof(IDN_ALABEL_PREFIX); ++i) {
    uint8_t b;
    if (input.Read(b) != Success) {
      return false;
    }
    if (b != IDN_ALABEL_PREFIX[i]) {
      return false;
    }
  }
  return true;
}

bool
ReadIPv4AddressComponent(Reader& input, bool lastComponent,
                          uint8_t& valueOut)
{
  size_t length = 0;
  unsigned int value = 0; 

  for (;;) {
    if (input.AtEnd() && lastComponent) {
      break;
    }

    uint8_t b;
    if (input.Read(b) != Success) {
      return false;
    }

    if (b >= '0' && b <= '9') {
      if (value == 0 && length > 0) {
        return false; 
      }
      value = (value * 10) + (b - '0');
      if (value > 255) {
        return false; 
      }
      ++length;
    } else if (!lastComponent && b == '.') {
      break;
    } else {
      return false; 
    }
  }

  if (length == 0) {
    return false; 
  }

  valueOut = static_cast<uint8_t>(value);
  return true;
}

} 

bool
ParseIPv4Address(Input hostname,  uint8_t (&out)[4])
{
  Reader input(hostname);
  return ReadIPv4AddressComponent(input, false, out[0]) &&
         ReadIPv4AddressComponent(input, false, out[1]) &&
         ReadIPv4AddressComponent(input, false, out[2]) &&
         ReadIPv4AddressComponent(input, true, out[3]);
}

namespace {

bool
FinishIPv6Address( uint8_t (&address)[16], int numComponents,
                  int contractionIndex)
{
  assert(numComponents >= 0);
  assert(numComponents <= 8);
  assert(contractionIndex >= -1);
  assert(contractionIndex <= 8);
  assert(contractionIndex <= numComponents);
  if (!(numComponents >= 0 &&
        numComponents <= 8 &&
        contractionIndex >= -1 &&
        contractionIndex <= 8 &&
        contractionIndex <= numComponents)) {
    return false;
  }

  if (contractionIndex == -1) {
    
    return numComponents == 8;
  }

  if (numComponents >= 8) {
    return false; 
  }

  
  int componentsToMove = numComponents - contractionIndex;
  memmove(address + (2u * (8 - componentsToMove)),
          address + (2u * contractionIndex),
          componentsToMove * 2u);
  
  memset(address + (2u * contractionIndex), 0u,
         (8u - numComponents) * 2u);

  return true;
}

} 


bool
ParseIPv6Address(Input hostname,  uint8_t (&out)[16])
{
  Reader input(hostname);

  int currentComponentIndex = 0;
  int contractionIndex = -1;

  if (input.Peek(':')) {
    
    
    uint8_t b;
    if (input.Read(b) != Success || b != ':') {
      assert(false);
      return false;
    }
    if (input.Read(b) != Success) {
      return false;
    }
    if (b != ':') {
      return false;
    }
    contractionIndex = 0;
  }

  for (;;) {
    
    
    Reader::Mark startOfComponent(input.GetMark());
    uint16_t componentValue = 0;
    size_t componentLength = 0;
    while (!input.AtEnd() && !input.Peek(':')) {
      uint8_t value;
      uint8_t b;
      if (input.Read(b) != Success) {
        assert(false);
        return false;
      }
      switch (b) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          value = static_cast<uint8_t>(b - static_cast<uint8_t>('0'));
          break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
          value = static_cast<uint8_t>(b - static_cast<uint8_t>('a') +
                                       UINT8_C(10));
          break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          value = static_cast<uint8_t>(b - static_cast<uint8_t>('A') +
                                       UINT8_C(10));
          break;
        case '.':
        {
          
          
          

          if (currentComponentIndex > 6) {
            return false; 
          }

          input.SkipToEnd();
          Input ipv4Component;
          if (input.GetInput(startOfComponent, ipv4Component) != Success) {
            return false;
          }
          uint8_t (*ipv4)[4] =
            reinterpret_cast<uint8_t(*)[4]>(&out[2 * currentComponentIndex]);
          if (!ParseIPv4Address(ipv4Component, *ipv4)) {
            return false;
          }
          assert(input.AtEnd());
          currentComponentIndex += 2;

          return FinishIPv6Address(out, currentComponentIndex,
                                   contractionIndex);
        }
        default:
          return false;
      }
      if (componentLength >= 4) {
        
        return false;
      }
      ++componentLength;
      componentValue = (componentValue * 0x10u) + value;
    }

    if (currentComponentIndex >= 8) {
      return false; 
    }

    if (componentLength == 0) {
      if (input.AtEnd() && currentComponentIndex == contractionIndex) {
        if (contractionIndex == 0) {
          
          return false;
        }
        return FinishIPv6Address(out, currentComponentIndex,
                                 contractionIndex);
      }
      return false;
    }

    out[2 * currentComponentIndex] =
      static_cast<uint8_t>(componentValue / 0x100);
    out[(2 * currentComponentIndex) + 1] =
      static_cast<uint8_t>(componentValue % 0x100);

    ++currentComponentIndex;

    if (input.AtEnd()) {
      return FinishIPv6Address(out, currentComponentIndex,
                               contractionIndex);
    }

    uint8_t b;
    if (input.Read(b) != Success || b != ':') {
      assert(false);
      return false;
    }

    if (input.Peek(':')) {
      
      if (contractionIndex != -1) {
        return false; 
      }
      uint8_t b;
      if (input.Read(b) != Success || b != ':') {
        assert(false);
        return false;
      }
      contractionIndex = currentComponentIndex;
      if (input.AtEnd()) {
        
        return FinishIPv6Address(out, currentComponentIndex,
                                 contractionIndex);
      }
    }
  }
}

bool
IsValidReferenceDNSID(Input hostname)
{
  return IsValidDNSID(hostname, ValidDNSIDMatchType::ReferenceID);
}

bool
IsValidPresentedDNSID(Input hostname)
{
  return IsValidDNSID(hostname, ValidDNSIDMatchType::PresentedID);
}

namespace {

bool
IsValidDNSID(Input hostname, ValidDNSIDMatchType matchType)
{
  if (hostname.GetLength() > 253) {
    return false;
  }

  Reader input(hostname);

  bool allowWildcard = matchType == ValidDNSIDMatchType::PresentedID;
  bool isWildcard = false;
  size_t dotCount = 0;

  size_t labelLength = 0;
  bool labelIsAllNumeric = false;
  bool labelIsWildcard = false;
  bool labelEndsWithHyphen = false;

  do {
    static const size_t MAX_LABEL_LENGTH = 63;

    uint8_t b;
    if (input.Read(b) != Success) {
      return false;
    }
    if (labelIsWildcard) {
      
      
      
      if (b != '.') {
        return false;
      }
    }
    switch (b) {
      case '-':
        if (labelLength == 0) {
          return false; 
        }
        labelIsAllNumeric = false;
        labelEndsWithHyphen = true;
        ++labelLength;
        if (labelLength > MAX_LABEL_LENGTH) {
          return false;
        }
        break;

      
      
      case '0': case '5':
      case '1': case '6':
      case '2': case '7':
      case '3': case '8':
      case '4': case '9':
        if (labelLength == 0) {
          labelIsAllNumeric = true;
        }
        labelEndsWithHyphen = false;
        ++labelLength;
        if (labelLength > MAX_LABEL_LENGTH) {
          return false;
        }
        break;

      
      
      
      case 'a': case 'A': case 'n': case 'N':
      case 'b': case 'B': case 'o': case 'O':
      case 'c': case 'C': case 'p': case 'P':
      case 'd': case 'D': case 'q': case 'Q':
      case 'e': case 'E': case 'r': case 'R':
      case 'f': case 'F': case 's': case 'S':
      case 'g': case 'G': case 't': case 'T':
      case 'h': case 'H': case 'u': case 'U':
      case 'i': case 'I': case 'v': case 'V':
      case 'j': case 'J': case 'w': case 'W':
      case 'k': case 'K': case 'x': case 'X':
      case 'l': case 'L': case 'y': case 'Y':
      case 'm': case 'M': case 'z': case 'Z':
        labelIsAllNumeric = false;
        labelEndsWithHyphen = false;
        ++labelLength;
        if (labelLength > MAX_LABEL_LENGTH) {
          return false;
        }
        break;

      case '*':
        if (!allowWildcard) {
          return false;
        }
        labelIsWildcard = true;
        isWildcard = true;
        labelIsAllNumeric = false;
        labelEndsWithHyphen = false;
        ++labelLength;
        if (labelLength > MAX_LABEL_LENGTH) {
          return false;
        }
        break;

      case '.':
        ++dotCount;
        if (labelLength == 0) {
          return false;
        }
        if (labelEndsWithHyphen) {
          return false; 
        }
        allowWildcard = false; 
        labelIsWildcard = false;
        labelLength = 0;
        break;

      default:
        return false; 
    }
  } while (!input.AtEnd());

  if (labelEndsWithHyphen) {
    return false; 
  }

  if (labelIsAllNumeric) {
    return false; 
  }

  if (isWildcard) {
    
    size_t labelCount = (labelLength == 0) ? dotCount : (dotCount + 1);

    
    
    
    
    
    if (labelCount < 3) {
      return false;
    }
    
    
    
    if (StartsWithIDNALabel(hostname)) {
      return false;
    }

    
    
    
  }

  return true;
}

} 

} } 
