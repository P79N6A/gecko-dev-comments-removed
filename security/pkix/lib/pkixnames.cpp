



































#include "pkixcheck.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

namespace {











enum class GeneralNameType : uint8_t
{
  
  
  
  otherName = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
  rfc822Name = der::CONTEXT_SPECIFIC | 1,
  dNSName = der::CONTEXT_SPECIFIC | 2,
  x400Address = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 3,
  directoryName = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 4,
  ediPartyName = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 5,
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

enum class FallBackToSearchWithinSubject { No = 0, Yes = 1 };

enum class MatchResult
{
  NoNamesOfGivenType = 0,
  Mismatch = 1,
  Match = 2
};

Result SearchNames(const Input* subjectAltName, Input subject,
                   GeneralNameType referenceIDType,
                   Input referenceID,
                   FallBackToSearchWithinSubject fallBackToCommonName,
                    MatchResult& match);
Result SearchWithinRDN(Reader& rdn,
                       GeneralNameType referenceIDType,
                       Input referenceID,
                       FallBackToSearchWithinSubject fallBackToEmailAddress,
                       FallBackToSearchWithinSubject fallBackToCommonName,
                        MatchResult& match);
Result MatchAVA(Input type,
                uint8_t valueEncodingTag,
                Input presentedID,
                GeneralNameType referenceIDType,
                Input referenceID,
                FallBackToSearchWithinSubject fallBackToEmailAddress,
                FallBackToSearchWithinSubject fallBackToCommonName,
                 MatchResult& match);
Result ReadAVA(Reader& rdn,
                Input& type,
                uint8_t& valueTag,
                Input& value);
void MatchSubjectPresentedIDWithReferenceID(GeneralNameType presentedIDType,
                                            Input presentedID,
                                            GeneralNameType referenceIDType,
                                            Input referenceID,
                                             MatchResult& match);

Result MatchPresentedIDWithReferenceID(GeneralNameType presentedIDType,
                                       Input presentedID,
                                       GeneralNameType referenceIDType,
                                       Input referenceID,
                                        MatchResult& matchResult);
Result CheckPresentedIDConformsToConstraints(GeneralNameType referenceIDType,
                                             Input presentedID,
                                             Input nameConstraints);

uint8_t LocaleInsensitveToLower(uint8_t a);
bool StartsWithIDNALabel(Input id);

enum class IDRole
{
  ReferenceID = 0,
  PresentedID = 1,
  NameConstraint = 2,
};

enum class AllowWildcards { No = 0, Yes = 1 };






enum class AllowDotlessSubdomainMatches { No = 0, Yes = 1 };

bool IsValidDNSID(Input hostname, IDRole idRole,
                  AllowWildcards allowWildcards);

Result MatchPresentedDNSIDWithReferenceDNSID(
         Input presentedDNSID,
         AllowWildcards allowWildcards,
         AllowDotlessSubdomainMatches allowDotlessSubdomainMatches,
         IDRole referenceDNSIDRole,
         Input referenceDNSID,
          bool& matches);

Result MatchPresentedRFC822NameWithReferenceRFC822Name(
         Input presentedRFC822Name, IDRole referenceRFC822NameRole,
         Input referenceRFC822Name,  bool& matches);

} 

bool IsValidReferenceDNSID(Input hostname);
bool IsValidPresentedDNSID(Input hostname);
bool ParseIPv4Address(Input hostname,  uint8_t (&out)[4]);
bool ParseIPv6Address(Input hostname,  uint8_t (&out)[16]);


Result
MatchPresentedDNSIDWithReferenceDNSID(Input presentedDNSID,
                                      Input referenceDNSID,
                                       bool& matches)
{
  return MatchPresentedDNSIDWithReferenceDNSID(
           presentedDNSID, AllowWildcards::Yes,
           AllowDotlessSubdomainMatches::Yes, IDRole::ReferenceID,
           referenceDNSID, matches);
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
                     hostname, FallBackToSearchWithinSubject::Yes, match);
  } else if (ParseIPv6Address(hostname, ipv6)) {
    rv = SearchNames(subjectAltName, subject, GeneralNameType::iPAddress,
                     Input(ipv6), FallBackToSearchWithinSubject::No, match);
  } else if (ParseIPv4Address(hostname, ipv4)) {
    rv = SearchNames(subjectAltName, subject, GeneralNameType::iPAddress,
                     Input(ipv4), FallBackToSearchWithinSubject::Yes, match);
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
    MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
  }
}


Result
CheckNameConstraints(Input encodedNameConstraints,
                     const BackCert& firstChild,
                     KeyPurposeId requiredEKUIfPresent)
{
  for (const BackCert* child = &firstChild; child; child = child->childCert) {
    FallBackToSearchWithinSubject fallBackToCommonName
      = (child->endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
         requiredEKUIfPresent == KeyPurposeId::id_kp_serverAuth)
      ? FallBackToSearchWithinSubject::Yes
      : FallBackToSearchWithinSubject::No;

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
            FallBackToSearchWithinSubject fallBackToCommonName,
             MatchResult& match)
{
  Result rv;

  match = MatchResult::NoNamesOfGivenType;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (subjectAltName) {
    Reader altNames;
    rv = der::ExpectTagAndGetValueAtEnd(*subjectAltName, der::SEQUENCE,
                                        altNames);
    if (rv != Success) {
      return rv;
    }

    
    
    
    while (!altNames.AtEnd()) {
      GeneralNameType presentedIDType;
      Input presentedID;
      rv = ReadGeneralName(altNames, presentedIDType, presentedID);
      if (rv != Success) {
        return rv;
      }

      rv = MatchPresentedIDWithReferenceID(presentedIDType, presentedID,
                                           referenceIDType, referenceID,
                                           match);
      if (rv != Success) {
        return rv;
      }
      if (referenceIDType != GeneralNameType::nameConstraints &&
          match == MatchResult::Match) {
        return Success;
      }
      if (presentedIDType == GeneralNameType::dNSName ||
          presentedIDType == GeneralNameType::iPAddress) {
        fallBackToCommonName = FallBackToSearchWithinSubject::No;
      }
    }
  }

  if (referenceIDType == GeneralNameType::nameConstraints) {
    rv = CheckPresentedIDConformsToConstraints(GeneralNameType::directoryName,
                                               subject, referenceID);
    if (rv != Success) {
      return rv;
    }
  }

  FallBackToSearchWithinSubject fallBackToEmailAddress;
  if (!subjectAltName &&
      (referenceIDType == GeneralNameType::rfc822Name ||
       referenceIDType == GeneralNameType::nameConstraints)) {
    fallBackToEmailAddress = FallBackToSearchWithinSubject::Yes;
  } else {
    fallBackToEmailAddress = FallBackToSearchWithinSubject::No;
  }

  
  
  if (fallBackToEmailAddress == FallBackToSearchWithinSubject::No &&
      fallBackToCommonName == FallBackToSearchWithinSubject::No) {
    return Success;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  Reader subjectReader(subject);
  return der::NestedOf(subjectReader, der::SEQUENCE, der::SET,
                       der::EmptyAllowed::Yes, [&](Reader& r) {
    return SearchWithinRDN(r, referenceIDType, referenceID,
                          fallBackToEmailAddress, fallBackToCommonName, match);
  });
}







Result
SearchWithinRDN(Reader& rdn,
                GeneralNameType referenceIDType,
                Input referenceID,
                FallBackToSearchWithinSubject fallBackToEmailAddress,
                FallBackToSearchWithinSubject fallBackToCommonName,
                 MatchResult& match)
{
  do {
    Input type;
    uint8_t valueTag;
    Input value;
    Result rv = ReadAVA(rdn, type, valueTag, value);
    if (rv != Success) {
      return rv;
    }
    rv = MatchAVA(type, valueTag, value, referenceIDType, referenceID,
                  fallBackToEmailAddress, fallBackToCommonName, match);
    if (rv != Success) {
      return rv;
    }
  } while (!rdn.AtEnd());

  return Success;
}















Result
MatchAVA(Input type, uint8_t valueEncodingTag, Input presentedID,
         GeneralNameType referenceIDType,
         Input referenceID,
         FallBackToSearchWithinSubject fallBackToEmailAddress,
         FallBackToSearchWithinSubject fallBackToCommonName,
          MatchResult& match)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static const uint8_t id_at_commonName[] = {
    0x55, 0x04, 0x03
  };
  if (fallBackToCommonName == FallBackToSearchWithinSubject::Yes &&
      InputsAreEqual(type, Input(id_at_commonName))) {
    
    
    
    match = MatchResult::NoNamesOfGivenType;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (valueEncodingTag != der::PrintableString &&
        valueEncodingTag != der::UTF8String &&
        valueEncodingTag != der::TeletexString) {
      return Success;
    }

    if (IsValidPresentedDNSID(presentedID)) {
      MatchSubjectPresentedIDWithReferenceID(GeneralNameType::dNSName,
                                             presentedID, referenceIDType,
                                             referenceID, match);
    } else {
      
      
      
      
      uint8_t ipv4[4];
      if (ParseIPv4Address(presentedID, ipv4)) {
        MatchSubjectPresentedIDWithReferenceID(GeneralNameType::iPAddress,
                                               Input(ipv4), referenceIDType,
                                               referenceID, match);
      }
    }

    
    
    

    return Success;
  }

  
  
  
  
  
  
  
  
  static const uint8_t id_emailAddress[] = {
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01
  };
  if (fallBackToEmailAddress == FallBackToSearchWithinSubject::Yes &&
      InputsAreEqual(type, Input(id_emailAddress))) {
    if (referenceIDType == GeneralNameType::rfc822Name &&
        match == MatchResult::Match) {
      
      return Success;
    }
    if (valueEncodingTag != der::IA5String) {
      return Result::ERROR_BAD_DER;
    }
    return MatchPresentedIDWithReferenceID(GeneralNameType::rfc822Name,
                                           presentedID, referenceIDType,
                                           referenceID, match);
  }

  return Success;
}

void
MatchSubjectPresentedIDWithReferenceID(GeneralNameType presentedIDType,
                                       Input presentedID,
                                       GeneralNameType referenceIDType,
                                       Input referenceID,
                                        MatchResult& match)
{
  Result rv = MatchPresentedIDWithReferenceID(presentedIDType, presentedID,
                                              referenceIDType, referenceID,
                                              match);
  if (rv != Success) {
    match = MatchResult::Mismatch;
  }
}

Result
MatchPresentedIDWithReferenceID(GeneralNameType presentedIDType,
                                Input presentedID,
                                GeneralNameType referenceIDType,
                                Input referenceID,
                                 MatchResult& matchResult)
{
  if (referenceIDType == GeneralNameType::nameConstraints) {
    
    
    return CheckPresentedIDConformsToConstraints(presentedIDType, presentedID,
                                                 referenceID);
  }

  if (presentedIDType != referenceIDType) {
    matchResult = MatchResult::Mismatch;
    return Success;
  }

  Result rv;
  bool foundMatch;

  switch (referenceIDType) {
    case GeneralNameType::dNSName:
      rv = MatchPresentedDNSIDWithReferenceDNSID(
             presentedID, AllowWildcards::Yes,
             AllowDotlessSubdomainMatches::Yes, IDRole::ReferenceID,
             referenceID, foundMatch);
      break;

    case GeneralNameType::iPAddress:
      foundMatch = InputsAreEqual(presentedID, referenceID);
      rv = Success;
      break;

    case GeneralNameType::rfc822Name:
      rv = MatchPresentedRFC822NameWithReferenceRFC822Name(
             presentedID, IDRole::ReferenceID, referenceID, foundMatch);
      break;

    case GeneralNameType::directoryName:
      
      

    case GeneralNameType::otherName: 
    case GeneralNameType::x400Address: 
    case GeneralNameType::ediPartyName: 
    case GeneralNameType::uniformResourceIdentifier: 
    case GeneralNameType::registeredID: 
    case GeneralNameType::nameConstraints:
      return NotReached("unexpected nameType for SearchType::Match",
                        Result::FATAL_ERROR_INVALID_ARGS);

    MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
 }

  if (rv != Success) {
    return rv;
  }
  matchResult = foundMatch ? MatchResult::Match : MatchResult::Mismatch;
  return Success;
}

enum class NameConstraintsSubtrees : uint8_t
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
          rv = MatchPresentedDNSIDWithReferenceDNSID(
                 presentedID, AllowWildcards::Yes,
                 AllowDotlessSubdomainMatches::Yes, IDRole::NameConstraint,
                 base, matches);
          if (rv != Success) {
            return rv;
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
          rv = MatchPresentedRFC822NameWithReferenceRFC822Name(
                 presentedID, IDRole::NameConstraint, base, matches);
          if (rv != Success) {
            return rv;
          }
          break;

        
        
        
        
        
        
        
        
        
        
        case GeneralNameType::otherName: 
        case GeneralNameType::x400Address: 
        case GeneralNameType::ediPartyName: 
        case GeneralNameType::uniformResourceIdentifier: 
        case GeneralNameType::registeredID: 
          return Result::ERROR_CERT_NOT_IN_NAME_SPACE;

        case GeneralNameType::nameConstraints:
          return NotReached("invalid presentedIDType",
                            Result::FATAL_ERROR_LIBRARY_FAILURE);

        MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
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
      }
    }
  } while (!subtrees.AtEnd());

  if (hasPermittedSubtreesMismatch && !hasPermittedSubtreesMatch) {
    
    
    
    return Result::ERROR_CERT_NOT_IN_NAME_SPACE;
  }

  return Success;
}

























































































































Result
MatchPresentedDNSIDWithReferenceDNSID(
  Input presentedDNSID,
  AllowWildcards allowWildcards,
  AllowDotlessSubdomainMatches allowDotlessSubdomainMatches,
  IDRole referenceDNSIDRole,
  Input referenceDNSID,
   bool& matches)
{
  if (!IsValidDNSID(presentedDNSID, IDRole::PresentedID, allowWildcards)) {
    return Result::ERROR_BAD_DER;
  }

  if (!IsValidDNSID(referenceDNSID, referenceDNSIDRole, AllowWildcards::No)) {
    return Result::ERROR_BAD_DER;
  }

  Reader presented(presentedDNSID);
  Reader reference(referenceDNSID);

  switch (referenceDNSIDRole)
  {
    case IDRole::ReferenceID:
      break;

    case IDRole::NameConstraint:
    {
      if (presentedDNSID.GetLength() > referenceDNSID.GetLength()) {
        if (referenceDNSID.GetLength() == 0) {
          
          matches = true;
          return Success;
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (reference.Peek('.')) {
          if (presented.Skip(static_cast<Input::size_type>(
                               presentedDNSID.GetLength() -
                                 referenceDNSID.GetLength())) != Success) {
            return NotReached("skipping subdomain failed",
                              Result::FATAL_ERROR_LIBRARY_FAILURE);
          }
        } else if (allowDotlessSubdomainMatches ==
                   AllowDotlessSubdomainMatches::Yes) {
          if (presented.Skip(static_cast<Input::size_type>(
                               presentedDNSID.GetLength() -
                                 referenceDNSID.GetLength() - 1)) != Success) {
            return NotReached("skipping subdomains failed",
                              Result::FATAL_ERROR_LIBRARY_FAILURE);
          }
          uint8_t b;
          if (presented.Read(b) != Success) {
            return NotReached("reading from presentedDNSID failed",
                              Result::FATAL_ERROR_LIBRARY_FAILURE);
          }
          if (b != '.') {
            matches = false;
            return Success;
          }
        }
      }
      break;
    }

    case IDRole::PresentedID: 
      return NotReached("IDRole::PresentedID is not a valid referenceDNSIDRole",
                        Result::FATAL_ERROR_INVALID_ARGS);
  }

  
  if (presented.Peek('*')) {
    if (presented.Skip(1) != Success) {
      return NotReached("Skipping '*' failed",
                        Result::FATAL_ERROR_LIBRARY_FAILURE);
    }
    do {
      
      if (reference.AtEnd()) {
        matches = false;
        return Success;
      }
      uint8_t referenceByte;
      if (reference.Read(referenceByte) != Success) {
        return NotReached("invalid reference ID",
                          Result::FATAL_ERROR_INVALID_ARGS);
      }
    } while (!reference.Peek('.'));
  }

  for (;;) {
    uint8_t presentedByte;
    if (presented.Read(presentedByte) != Success) {
      matches = false;
      return Success;
    }
    uint8_t referenceByte;
    if (reference.Read(referenceByte) != Success) {
      matches = false;
      return Success;
    }
    if (LocaleInsensitveToLower(presentedByte) !=
        LocaleInsensitveToLower(referenceByte)) {
      matches = false;
      return Success;
    }
    if (presented.AtEnd()) {
      
      if (presentedByte == '.') {
        return Result::ERROR_BAD_DER;
      }
      break;
    }
  }

  
  
  if (!reference.AtEnd()) {
    if (referenceDNSIDRole != IDRole::NameConstraint) {
      uint8_t referenceByte;
      if (reference.Read(referenceByte) != Success) {
        return NotReached("read failed but not at end",
                          Result::FATAL_ERROR_LIBRARY_FAILURE);
      }
      if (referenceByte != '.') {
        matches = false;
        return Success;
      }
    }
    if (!reference.AtEnd()) {
      matches = false;
      return Success;
    }
  }

  matches = true;
  return Success;
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
ReadAVA(Reader& rdn,
         Input& type,
         uint8_t& valueTag,
         Input& value)
{
  return der::Nested(rdn, der::SEQUENCE, [&](Reader& ava) -> Result {
    Result rv = der::ExpectTagAndGetValue(ava, der::OIDTag, type);
    if (rv != Success) {
      return rv;
    }
    rv = der::ReadTagAndGetValue(ava, valueTag, value);
    if (rv != Success) {
      return rv;
    }
    return Success;
  });
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
    Reader constraintRDN;
    rv = der::ExpectTagAndGetValue(constraintRDNs, der::SET, constraintRDN);
    if (rv != Success) {
      return rv;
    }
    Reader presentedRDN;
    rv = der::ExpectTagAndGetValue(presentedRDNs, der::SET, presentedRDN);
    if (rv != Success) {
      return rv;
    }
    while (!constraintRDN.AtEnd() && !presentedRDN.AtEnd()) {
      Input constraintType;
      uint8_t constraintValueTag;
      Input constraintValue;
      rv = ReadAVA(constraintRDN, constraintType, constraintValueTag,
                   constraintValue);
      if (rv != Success) {
        return rv;
      }
      Input presentedType;
      uint8_t presentedValueTag;
      Input presentedValue;
      rv = ReadAVA(presentedRDN, presentedType, presentedValueTag,
                   presentedValue);
      if (rv != Success) {
        return rv;
      }
      
      
      bool avasMatch =
        InputsAreEqual(constraintType, presentedType) &&
        InputsAreEqual(constraintValue, presentedValue) &&
        (constraintValueTag == presentedValueTag ||
         (constraintValueTag == der::Tag::UTF8String &&
          presentedValueTag == der::Tag::PrintableString) ||
         (constraintValueTag == der::Tag::PrintableString &&
          presentedValueTag == der::Tag::UTF8String));
      if (!avasMatch) {
        matches = false;
        return Success;
      }
    }
    if (!constraintRDN.AtEnd() || !presentedRDN.AtEnd()) {
      matches = false;
      return Success;
    }
  }
}


























bool
IsValidRFC822Name(Input input)
{
  Reader reader(input);

  
  bool startOfAtom = true;
  for (;;) {
    uint8_t presentedByte;
    if (reader.Read(presentedByte) != Success) {
      return false;
    }
    switch (presentedByte) {
      
      case 'A': case 'a': case 'N': case 'n': case '0': case '!': case '#':
      case 'B': case 'b': case 'O': case 'o': case '1': case '$': case '%':
      case 'C': case 'c': case 'P': case 'p': case '2': case '&': case '\'':
      case 'D': case 'd': case 'Q': case 'q': case '3': case '*': case '+':
      case 'E': case 'e': case 'R': case 'r': case '4': case '-': case '/':
      case 'F': case 'f': case 'S': case 's': case '5': case '=': case '?':
      case 'G': case 'g': case 'T': case 't': case '6': case '^': case '_':
      case 'H': case 'h': case 'U': case 'u': case '7': case '`': case '{':
      case 'I': case 'i': case 'V': case 'v': case '8': case '|': case '}':
      case 'J': case 'j': case 'W': case 'w': case '9': case '~':
      case 'K': case 'k': case 'X': case 'x':
      case 'L': case 'l': case 'Y': case 'y':
      case 'M': case 'm': case 'Z': case 'z':
        startOfAtom = false;
        break;

      case '.':
        if (startOfAtom) {
          return false;
        }
        startOfAtom = true;
        break;

      case '@':
      {
        if (startOfAtom) {
          return false;
        }
        Input domain;
        if (reader.SkipToEnd(domain) != Success) {
          return false;
        }
        return IsValidDNSID(domain, IDRole::PresentedID, AllowWildcards::No);
      }

      default:
        return false;
    }
  }
}

Result
MatchPresentedRFC822NameWithReferenceRFC822Name(Input presentedRFC822Name,
                                                IDRole referenceRFC822NameRole,
                                                Input referenceRFC822Name,
                                                 bool& matches)
{
  if (!IsValidRFC822Name(presentedRFC822Name)) {
    return Result::ERROR_BAD_DER;
  }
  Reader presented(presentedRFC822Name);

  switch (referenceRFC822NameRole)
  {
    case IDRole::PresentedID:
      return Result::FATAL_ERROR_INVALID_ARGS;

    case IDRole::ReferenceID:
      break;

    case IDRole::NameConstraint:
    {
      if (InputContains(referenceRFC822Name, '@')) {
        
        break;
      }

      

      
      for (;;) {
        uint8_t presentedByte;
        if (presented.Read(presentedByte) != Success) {
          return Result::FATAL_ERROR_LIBRARY_FAILURE;
        }
        if (presentedByte == '@') {
          break;
        }
      }

      Input presentedDNSID;
      if (presented.SkipToEnd(presentedDNSID) != Success) {
        return Result::FATAL_ERROR_LIBRARY_FAILURE;
      }

      return MatchPresentedDNSIDWithReferenceDNSID(
               presentedDNSID, AllowWildcards::No,
               AllowDotlessSubdomainMatches::No, IDRole::NameConstraint,
               referenceRFC822Name, matches);
    }
  }

  if (!IsValidRFC822Name(referenceRFC822Name)) {
    return Result::ERROR_BAD_DER;
  }

  Reader reference(referenceRFC822Name);

  for (;;) {
    uint8_t presentedByte;
    if (presented.Read(presentedByte) != Success) {
      matches = reference.AtEnd();
      return Success;
    }
    uint8_t referenceByte;
    if (reference.Read(referenceByte) != Success) {
      matches = false;
      return Success;
    }
    if (LocaleInsensitveToLower(presentedByte) !=
        LocaleInsensitveToLower(referenceByte)) {
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

  
  size_t componentsToMove = static_cast<size_t>(numComponents -
                                                contractionIndex);
  memmove(address + (2u * static_cast<size_t>(8 - componentsToMove)),
          address + (2u * static_cast<size_t>(contractionIndex)),
          componentsToMove * 2u);
  
  memset(address + (2u * static_cast<size_t>(contractionIndex)), 0u,
         (8u - static_cast<size_t>(numComponents)) * 2u);

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
  return IsValidDNSID(hostname, IDRole::ReferenceID, AllowWildcards::No);
}

bool
IsValidPresentedDNSID(Input hostname)
{
  return IsValidDNSID(hostname, IDRole::PresentedID, AllowWildcards::Yes);
}

namespace {





bool
IsValidDNSID(Input hostname, IDRole idRole, AllowWildcards allowWildcards)
{
  if (hostname.GetLength() > 253) {
    return false;
  }

  Reader input(hostname);

  if (idRole == IDRole::NameConstraint && input.AtEnd()) {
    return true;
  }

  size_t dotCount = 0;
  size_t labelLength = 0;
  bool labelIsAllNumeric = false;
  bool labelEndsWithHyphen = false;

  
  
  
  bool isWildcard = allowWildcards == AllowWildcards::Yes && input.Peek('*');
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
      
      
      case '_':
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
            (idRole != IDRole::NameConstraint || !isFirstByte)) {
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

  
  
  if (labelLength == 0 && idRole != IDRole::ReferenceID) {
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
