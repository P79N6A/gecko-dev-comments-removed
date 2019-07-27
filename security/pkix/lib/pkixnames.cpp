



































#include "pkix/bind.h"
#include "pkixcheck.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

namespace {











MOZILLA_PKIX_ENUM_CLASS GeneralNameType : uint8_t
{
  
  
  otherName = der::CONTEXT_SPECIFIC | 0,
  rfc822Name = der::CONTEXT_SPECIFIC | 1,
  dNSName = der::CONTEXT_SPECIFIC | 2,
  x400Address = der::CONTEXT_SPECIFIC | 3,
  directoryName = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 4,
  ediPartyName = der::CONTEXT_SPECIFIC | 5,
  uniformResourceIdentifier = der::CONTEXT_SPECIFIC | 6,
  iPAddress = der::CONTEXT_SPECIFIC | 7,
  registeredID = der::CONTEXT_SPECIFIC | 8,
  
  
  nameConstraints = 0xff
};

inline Result
ReadGeneralName(Reader& reader,
                 GeneralNameType& generalNameType,
                 Input& value)
{
  uint8_t tag;
  Result rv = der::ReadTagAndGetValue(reader, tag, value);
  if (rv != Success) {
    return rv;
  }
  switch (tag) {
    case static_cast<uint8_t>(GeneralNameType::otherName):
      generalNameType = GeneralNameType::otherName;
      break;
    case static_cast<uint8_t>(GeneralNameType::rfc822Name):
      generalNameType = GeneralNameType::rfc822Name;
      break;
    case static_cast<uint8_t>(GeneralNameType::dNSName):
      generalNameType = GeneralNameType::dNSName;
      break;
    case static_cast<uint8_t>(GeneralNameType::x400Address):
      generalNameType = GeneralNameType::x400Address;
      break;
    case static_cast<uint8_t>(GeneralNameType::directoryName):
      generalNameType = GeneralNameType::directoryName;
      break;
    case static_cast<uint8_t>(GeneralNameType::ediPartyName):
      generalNameType = GeneralNameType::ediPartyName;
      break;
    case static_cast<uint8_t>(GeneralNameType::uniformResourceIdentifier):
      generalNameType = GeneralNameType::uniformResourceIdentifier;
      break;
    case static_cast<uint8_t>(GeneralNameType::iPAddress):
      generalNameType = GeneralNameType::iPAddress;
      break;
    case static_cast<uint8_t>(GeneralNameType::registeredID):
      generalNameType = GeneralNameType::registeredID;
      break;
    default:
      return Result::ERROR_BAD_DER;
  }
  return Success;
}

MOZILLA_PKIX_ENUM_CLASS FallBackToCommonName { No = 0, Yes = 1 };

MOZILLA_PKIX_ENUM_CLASS MatchResult
{
  NoNamesOfGivenType = 0,
  Mismatch = 1,
  Match = 2
};

Result SearchNames(const Input* subjectAltName, Input subject,
                   GeneralNameType referenceIDType,
                   Input referenceID,
                   FallBackToCommonName fallBackToCommonName,
                    MatchResult& match);
Result SearchWithinRDN(Reader& rdn,
                       GeneralNameType referenceIDType,
                       Input referenceID,
                        MatchResult& match);
Result SearchWithinAVA(Reader& rdn,
                       GeneralNameType referenceIDType,
                       Input referenceID,
                        MatchResult& match);

Result MatchPresentedIDWithReferenceID(GeneralNameType referenceIDType,
                                       Input presentedID,
                                       Input referenceID,
                                        bool& isMatch);
Result CheckPresentedIDConformsToConstraints(GeneralNameType referenceIDType,
                                             Input presentedID,
                                             Input nameConstraints);

uint8_t LocaleInsensitveToLower(uint8_t a);
bool StartsWithIDNALabel(Input id);

MOZILLA_PKIX_ENUM_CLASS ValidDNSIDMatchType
{
  ReferenceID = 0,
  PresentedID = 1,
  NameConstraint = 2,
};

bool IsValidDNSID(Input hostname, ValidDNSIDMatchType matchType);

bool PresentedDNSIDMatchesReferenceDNSID(
       Input presentedDNSID, ValidDNSIDMatchType referenceDNSIDMatchType,
       Input referenceDNSID);

} 

bool IsValidReferenceDNSID(Input hostname);
bool IsValidPresentedDNSID(Input hostname);
bool ParseIPv4Address(Input hostname,  uint8_t (&out)[4]);
bool ParseIPv6Address(Input hostname,  uint8_t (&out)[16]);

bool PresentedDNSIDMatchesReferenceDNSID(Input presentedDNSID,
                                         Input referenceDNSID)
{
  return PresentedDNSIDMatchesReferenceDNSID(presentedDNSID,
                                             ValidDNSIDMatchType::ReferenceID,
                                             referenceDNSID);
}





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

  
  
  
  
  
  
  
  
  
  MatchResult match;
  uint8_t ipv6[16];
  uint8_t ipv4[4];
  if (IsValidReferenceDNSID(hostname)) {
    rv = SearchNames(subjectAltName, subject, GeneralNameType::dNSName,
                     hostname, FallBackToCommonName::Yes, match);
  } else if (ParseIPv6Address(hostname, ipv6)) {
    rv = SearchNames(subjectAltName, subject, GeneralNameType::iPAddress,
                     Input(ipv6), FallBackToCommonName::No, match);
  } else if (ParseIPv4Address(hostname, ipv4)) {
    rv = SearchNames(subjectAltName, subject, GeneralNameType::iPAddress,
                     Input(ipv4), FallBackToCommonName::Yes, match);
  } else {
    return Result::ERROR_BAD_CERT_DOMAIN;
  }
  if (rv != Success) {
    return rv;
  }
  switch (match) {
    case MatchResult::NoNamesOfGivenType: 
    case MatchResult::Mismatch:
      return Result::ERROR_BAD_CERT_DOMAIN;
    case MatchResult::Match:
      return Success;
    default:
      return NotReached("Invalid match result",
                        Result::FATAL_ERROR_LIBRARY_FAILURE);
  }
}


Result
CheckNameConstraints(Input encodedNameConstraints,
                     const BackCert& firstChild,
                     KeyPurposeId requiredEKUIfPresent)
{
  for (const BackCert* child = &firstChild; child; child = child->childCert) {
    FallBackToCommonName fallBackToCommonName
      = (child->endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
         requiredEKUIfPresent == KeyPurposeId::id_kp_serverAuth)
      ? FallBackToCommonName::Yes
      : FallBackToCommonName::No;

    MatchResult match;
    Result rv = SearchNames(child->GetSubjectAltName(), child->GetSubject(),
                            GeneralNameType::nameConstraints,
                            encodedNameConstraints, fallBackToCommonName,
                            match);
    if (rv != Success) {
      return rv;
    }
    switch (match) {
      case MatchResult::Match: 
      case MatchResult::NoNamesOfGivenType:
        break;
      case MatchResult::Mismatch:
        return Result::ERROR_CERT_NOT_IN_NAME_SPACE;
    }
  }

  return Success;
}

namespace {
















Result
SearchNames( const Input* subjectAltName,
            Input subject,
            GeneralNameType referenceIDType,
            Input referenceID,
            FallBackToCommonName fallBackToCommonName,
             MatchResult& match)
{
  Result rv;

  match = MatchResult::NoNamesOfGivenType;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool hasAtLeastOneDNSNameOrIPAddressSAN = false;

  if (subjectAltName) {
    Reader altNames;
    rv = der::ExpectTagAndGetValueAtEnd(*subjectAltName, der::SEQUENCE,
                                        altNames);
    if (rv != Success) {
      return rv;
    }

    
    do {
      GeneralNameType presentedIDType;
      Input presentedID;
      rv = ReadGeneralName(altNames, presentedIDType, presentedID);
      if (rv != Success) {
        return rv;
      }
      if (referenceIDType == GeneralNameType::nameConstraints) {
        rv = CheckPresentedIDConformsToConstraints(presentedIDType,
                                                   presentedID, referenceID);
        if (rv != Success) {
          return rv;
        }
      } else if (presentedIDType == referenceIDType) {
        bool isMatch;
        rv = MatchPresentedIDWithReferenceID(presentedIDType, presentedID,
                                             referenceID, isMatch);
        if (rv != Success) {
          return rv;
        }
        if (isMatch) {
          match = MatchResult::Match;
          return Success;
        }
        match = MatchResult::Mismatch;
      }
      if (presentedIDType == GeneralNameType::dNSName ||
          presentedIDType == GeneralNameType::iPAddress) {
        hasAtLeastOneDNSNameOrIPAddressSAN = true;
      }
    } while (!altNames.AtEnd());
  }

  if (referenceIDType == GeneralNameType::nameConstraints) {
    rv = CheckPresentedIDConformsToConstraints(GeneralNameType::directoryName,
                                               subject, referenceID);
    if (rv != Success) {
      return rv;
    }
  }

  if (hasAtLeastOneDNSNameOrIPAddressSAN ||
      fallBackToCommonName != FallBackToCommonName::Yes) {
    return Success;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  Reader subjectReader(subject);
  return der::NestedOf(subjectReader, der::SEQUENCE, der::SET,
                       der::EmptyAllowed::Yes,
                       bind(SearchWithinRDN, _1, referenceIDType,
                            referenceID, ref(match)));
}







Result
SearchWithinRDN(Reader& rdn,
                GeneralNameType referenceIDType,
                Input referenceID,
                 MatchResult& match)
{
  do {
    Result rv = der::Nested(rdn, der::SEQUENCE,
                            bind(SearchWithinAVA, _1, referenceIDType,
                                 referenceID, ref(match)));
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
                 MatchResult& match)
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

  
  
  
  match = MatchResult::NoNamesOfGivenType;

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

  if (IsValidPresentedDNSID(presentedID)) {
    if (referenceIDType == GeneralNameType::nameConstraints) {
      rv = CheckPresentedIDConformsToConstraints(GeneralNameType::dNSName,
                                                 presentedID, referenceID);
      if (rv == Success) {
        match = MatchResult::Match;
      } else {
        match = MatchResult::Mismatch;
      }
    } else if (referenceIDType == GeneralNameType::dNSName) {
      bool isMatch;
      rv = MatchPresentedIDWithReferenceID(GeneralNameType::dNSName,
                                           presentedID, referenceID, isMatch);
      match = isMatch ? MatchResult::Match : MatchResult::Mismatch;
    }
  } else {
    uint8_t ipv4[4];
    
    
    
    if (ParseIPv4Address(presentedID, ipv4)) {
      if (referenceIDType == GeneralNameType::nameConstraints) {
        rv = CheckPresentedIDConformsToConstraints(GeneralNameType::iPAddress,
                                                   Input(ipv4), referenceID);
        if (rv == Success) {
          match = MatchResult::Match;
        } else {
          match = MatchResult::Mismatch;
        }
      } else if (referenceIDType == GeneralNameType::iPAddress) {
        bool isMatch;
        rv = MatchPresentedIDWithReferenceID(GeneralNameType::iPAddress,
                                             Input(ipv4), referenceID,
                                             isMatch);
        if (rv != Success) {
          return rv;
        }
        match = isMatch ? MatchResult::Match : MatchResult::Mismatch;
      }
    }
  }

  

  return Success;
}

Result
MatchPresentedIDWithReferenceID(GeneralNameType nameType,
                                Input presentedID,
                                Input referenceID,
                                 bool& foundMatch)
{
  switch (nameType) {
    case GeneralNameType::dNSName:
      foundMatch = PresentedDNSIDMatchesReferenceDNSID(
                     presentedID, ValidDNSIDMatchType::ReferenceID,
                     referenceID);
      return Success;

    case GeneralNameType::iPAddress:
      foundMatch = InputsAreEqual(presentedID, referenceID);
      return Success;

    case GeneralNameType::rfc822Name: 
    case GeneralNameType::directoryName:
      
      

    case GeneralNameType::otherName: 
    case GeneralNameType::x400Address: 
    case GeneralNameType::ediPartyName: 
    case GeneralNameType::uniformResourceIdentifier: 
    case GeneralNameType::registeredID: 
    case GeneralNameType::nameConstraints:
      return NotReached("unexpected nameType for SearchType::Match",
                        Result::FATAL_ERROR_INVALID_ARGS);

    default:
      return NotReached("Invalid nameType for MatchPresentedIDWithReferenceID",
                        Result::FATAL_ERROR_INVALID_ARGS);
  }
}

MOZILLA_PKIX_ENUM_CLASS NameConstraintsSubtrees : uint8_t
{
  permittedSubtrees = der::CONSTRUCTED | der::CONTEXT_SPECIFIC | 0,
  excludedSubtrees  = der::CONSTRUCTED | der::CONTEXT_SPECIFIC | 1
};

Result CheckPresentedIDConformsToNameConstraintsSubtrees(
         GeneralNameType presentedIDType,
         Input presentedID,
         Reader& nameConstraints,
         NameConstraintsSubtrees subtreesType);
Result MatchPresentedIPAddressWithConstraint(Input presentedID,
                                             Input iPAddressConstraint,
                                              bool& foundMatch);
Result MatchPresentedDirectoryNameWithConstraint(
         NameConstraintsSubtrees subtreesType, Input presentedID,
         Input directoryNameConstraint,  bool& matches);

Result
CheckPresentedIDConformsToConstraints(
  GeneralNameType presentedIDType,
  Input presentedID,
  Input encodedNameConstraints)
{
  
  
  
  Reader nameConstraints;
  Result rv = der::ExpectTagAndGetValueAtEnd(encodedNameConstraints,
                                             der::SEQUENCE, nameConstraints);
  if (rv != Success) {
    return rv;
  }

  
  
  
  if (nameConstraints.AtEnd()) {
    return Result::ERROR_BAD_DER;
  }

  rv = CheckPresentedIDConformsToNameConstraintsSubtrees(
         presentedIDType, presentedID, nameConstraints,
         NameConstraintsSubtrees::permittedSubtrees);
  if (rv != Success) {
    return rv;
  }

  rv = CheckPresentedIDConformsToNameConstraintsSubtrees(
         presentedIDType, presentedID, nameConstraints,
         NameConstraintsSubtrees::excludedSubtrees);
  if (rv != Success) {
    return rv;
  }

  return der::End(nameConstraints);
}

Result
CheckPresentedIDConformsToNameConstraintsSubtrees(
  GeneralNameType presentedIDType,
  Input presentedID,
  Reader& nameConstraints,
  NameConstraintsSubtrees subtreesType)
{
  if (!nameConstraints.Peek(static_cast<uint8_t>(subtreesType))) {
    return Success;
  }

  Reader subtrees;
  Result rv = der::ExpectTagAndGetValue(nameConstraints,
                                        static_cast<uint8_t>(subtreesType),
                                        subtrees);
  if (rv != Success) {
    return rv;
  }

  bool hasPermittedSubtreesMatch = false;
  bool hasPermittedSubtreesMismatch = false;

  
  
  
  do {
    
    
    
    
    Reader subtree;
    rv = ExpectTagAndGetValue(subtrees, der::SEQUENCE, subtree);
    if (rv != Success) {
      return rv;
    }
    GeneralNameType nameConstraintType;
    Input base;
    rv = ReadGeneralName(subtree, nameConstraintType, base);
    if (rv != Success) {
      return rv;
    }
    
    
    
    
    
    
    
    rv = der::End(subtree);
    if (rv != Success) {
      return rv;
    }

    if (presentedIDType == nameConstraintType) {
      bool matches;

      switch (presentedIDType) {
        case GeneralNameType::dNSName:
          matches = PresentedDNSIDMatchesReferenceDNSID(
                      presentedID, ValidDNSIDMatchType::NameConstraint, base);
          
          
          if (!matches &&
              !IsValidDNSID(base, ValidDNSIDMatchType::NameConstraint)) {
            return Result::ERROR_CERT_NOT_IN_NAME_SPACE;
          }
          break;

        case GeneralNameType::iPAddress:
          rv = MatchPresentedIPAddressWithConstraint(presentedID, base,
                                                     matches);
          if (rv != Success) {
            return rv;
          }
          break;

        case GeneralNameType::directoryName:
          rv = MatchPresentedDirectoryNameWithConstraint(subtreesType,
                                                         presentedID, base,
                                                         matches);
          if (rv != Success) {
            return rv;
          }
          break;

        case GeneralNameType::rfc822Name:
          return Result::FATAL_ERROR_LIBRARY_FAILURE; 

        
        
        
        
        
        
        
        
        
        
        case GeneralNameType::otherName: 
        case GeneralNameType::x400Address: 
        case GeneralNameType::ediPartyName: 
        case GeneralNameType::uniformResourceIdentifier: 
        case GeneralNameType::registeredID: 
          return Result::ERROR_CERT_NOT_IN_NAME_SPACE;

        case GeneralNameType::nameConstraints: 
        default:
          return NotReached("invalid presentedIDType",
                            Result::FATAL_ERROR_LIBRARY_FAILURE);
      }

      switch (subtreesType) {
        case NameConstraintsSubtrees::permittedSubtrees:
          if (matches) {
            hasPermittedSubtreesMatch = true;
          } else {
            hasPermittedSubtreesMismatch = true;
          }
          break;
        case NameConstraintsSubtrees::excludedSubtrees:
          if (matches) {
            return Result::ERROR_CERT_NOT_IN_NAME_SPACE;
          }
          break;
        default:
          return NotReached("unexpected subtreesType",
                            Result::FATAL_ERROR_INVALID_ARGS);
      }
    }
  } while (!subtrees.AtEnd());

  if (hasPermittedSubtreesMismatch && !hasPermittedSubtreesMatch) {
    
    
    
    return Result::ERROR_CERT_NOT_IN_NAME_SPACE;
  }

  return Success;
}

























































































































bool
PresentedDNSIDMatchesReferenceDNSID(
  Input presentedDNSID,
  ValidDNSIDMatchType referenceDNSIDMatchType,
  Input referenceDNSID)
{
  if (!IsValidPresentedDNSID(presentedDNSID)) {
    return false;
  }

  if (!IsValidDNSID(referenceDNSID, referenceDNSIDMatchType)) {
    return false;
  }

  Reader presented(presentedDNSID);
  Reader reference(referenceDNSID);

  switch (referenceDNSIDMatchType)
  {
    case ValidDNSIDMatchType::ReferenceID:
      break;

    case ValidDNSIDMatchType::NameConstraint:
    {
      if (presentedDNSID.GetLength() > referenceDNSID.GetLength()) {
        if (referenceDNSID.GetLength() == 0) {
          
          return true;
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (reference.Peek('.')) {
          if (presented.Skip(static_cast<Input::size_type>(
                               presentedDNSID.GetLength() -
                                 referenceDNSID.GetLength())) != Success) {
            assert(false);
            return false;
          }
        } else {
          if (presented.Skip(static_cast<Input::size_type>(
                               presentedDNSID.GetLength() -
                                 referenceDNSID.GetLength() - 1)) != Success) {
            assert(false);
            return false;
          }
          uint8_t b;
          if (presented.Read(b) != Success) {
            assert(false);
            return false;
          }
          if (b != '.') {
            return false;
          }
        }
      }
      break;
    }

    case ValidDNSIDMatchType::PresentedID: 
    default:
      assert(false);
      return false;
  }

  
  if (presented.Peek('*')) {
    Result rv = presented.Skip(1);
    if (rv != Success) {
      assert(false);
      return false;
    }
    do {
      uint8_t referenceByte;
      if (reference.Read(referenceByte) != Success) {
        return false;
      }
    } while (!reference.Peek('.'));
  }

  for (;;) {
    uint8_t presentedByte;
    if (presented.Read(presentedByte) != Success) {
      return false;
    }
    uint8_t referenceByte;
    if (reference.Read(referenceByte) != Success) {
      return false;
    }
    if (LocaleInsensitveToLower(presentedByte) !=
        LocaleInsensitveToLower(referenceByte)) {
      return false;
    }
    if (presented.AtEnd()) {
      
      if (presentedByte == '.') {
        return false;
      }
      break;
    }
  }

  
  
  if (!reference.AtEnd()) {
    if (referenceDNSIDMatchType != ValidDNSIDMatchType::NameConstraint) {
      uint8_t referenceByte;
      if (reference.Read(referenceByte) != Success) {
        return false;
      }
      if (referenceByte != '.') {
        return false;
      }
    }
    if (!reference.AtEnd()) {
      return false;
    }
  }

  return true;
}










Result
MatchPresentedIPAddressWithConstraint(Input presentedID,
                                      Input iPAddressConstraint,
                                       bool& foundMatch)
{
  if (presentedID.GetLength() != 4 && presentedID.GetLength() != 16) {
    return Result::ERROR_BAD_DER;
  }
  if (iPAddressConstraint.GetLength() != 8 &&
      iPAddressConstraint.GetLength() != 32) {
    return Result::ERROR_BAD_DER;
  }

  
  if (presentedID.GetLength() * 2 != iPAddressConstraint.GetLength()) {
    foundMatch = false;
    return Success;
  }

  Reader constraint(iPAddressConstraint);
  Reader constraintAddress;
  Result rv = constraint.Skip(iPAddressConstraint.GetLength() / 2u,
                              constraintAddress);
  if (rv != Success) {
    return rv;
  }
  Reader constraintMask;
  rv = constraint.Skip(iPAddressConstraint.GetLength() / 2u, constraintMask);
  if (rv != Success) {
    return rv;
  }
  rv = der::End(constraint);
  if (rv != Success) {
    return rv;
  }

  Reader presented(presentedID);
  do {
    uint8_t presentedByte;
    rv = presented.Read(presentedByte);
    if (rv != Success) {
      return rv;
    }
    uint8_t constraintAddressByte;
    rv = constraintAddress.Read(constraintAddressByte);
    if (rv != Success) {
      return rv;
    }
    uint8_t constraintMaskByte;
    rv = constraintMask.Read(constraintMaskByte);
    if (rv != Success) {
      return rv;
    }
    foundMatch =
      ((presentedByte ^ constraintAddressByte) & constraintMaskByte) == 0;
  } while (foundMatch && !presented.AtEnd());

  return Success;
}




































Result
MatchPresentedDirectoryNameWithConstraint(NameConstraintsSubtrees subtreesType,
                                          Input presentedID,
                                          Input directoryNameConstraint,
                                           bool& matches)
{
  Reader constraintRDNs;
  Result rv = der::ExpectTagAndGetValueAtEnd(directoryNameConstraint,
                                             der::SEQUENCE, constraintRDNs);
  if (rv != Success) {
    return rv;
  }
  Reader presentedRDNs;
  rv = der::ExpectTagAndGetValueAtEnd(presentedID, der::SEQUENCE,
                                      presentedRDNs);
  if (rv != Success) {
    return rv;
  }

  switch (subtreesType) {
    case NameConstraintsSubtrees::permittedSubtrees:
      break; 
    case NameConstraintsSubtrees::excludedSubtrees:
      if (!constraintRDNs.AtEnd() || !presentedRDNs.AtEnd()) {
        return Result::ERROR_CERT_NOT_IN_NAME_SPACE;
      }
      matches = true;
      return Success;
    default:
      return NotReached("invalid subtrees", Result::FATAL_ERROR_INVALID_ARGS);
  }

  for (;;) {
    
    
    if (constraintRDNs.AtEnd()) {
      matches = true;
      return Success;
    }
    if (presentedRDNs.AtEnd()) {
      matches = false;
      return Success;
    }
    Input constraintRDN;
    rv = der::ExpectTagAndGetValue(constraintRDNs, der::SET, constraintRDN);
    if (rv != Success) {
      return rv;
    }
    Input presentedRDN;
    rv = der::ExpectTagAndGetValue(presentedRDNs, der::SET, presentedRDN);
    if (rv != Success) {
      return rv;
    }
    if (!InputsAreEqual(constraintRDN, presentedRDN)) {
      matches = false;
      return Success;
    }
  }
}



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

  if (matchType == ValidDNSIDMatchType::NameConstraint && input.AtEnd()) {
    return true;
  }

  size_t dotCount = 0;
  size_t labelLength = 0;
  bool labelIsAllNumeric = false;
  bool labelEndsWithHyphen = false;

  
  
  
  bool isWildcard = matchType == ValidDNSIDMatchType::PresentedID &&
                    input.Peek('*');
  bool isFirstByte = !isWildcard;
  if (isWildcard) {
    Result rv = input.Skip(1);
    if (rv != Success) {
      assert(false);
      return false;
    }

    uint8_t b;
    rv = input.Read(b);
    if (rv != Success) {
      return false;
    }
    if (b != '.') {
      return false;
    }
    ++dotCount;
  }

  do {
    static const size_t MAX_LABEL_LENGTH = 63;

    uint8_t b;
    if (input.Read(b) != Success) {
      return false;
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

      case '.':
        ++dotCount;
        if (labelLength == 0 &&
            (matchType != ValidDNSIDMatchType::NameConstraint ||
             !isFirstByte)) {
          return false;
        }
        if (labelEndsWithHyphen) {
          return false; 
        }
        labelLength = 0;
        break;

      default:
        return false; 
    }
    isFirstByte = false;
  } while (!input.AtEnd());

  
  
  if (labelLength == 0 && matchType != ValidDNSIDMatchType::ReferenceID) {
    return false;
  }

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
