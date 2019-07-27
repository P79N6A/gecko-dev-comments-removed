























#include "cert.h"
#include "pkix/bind.h"
#include "pkix/pkix.h"
#include "pkix/ScopedPtr.h"
#include "pkixcheck.h"
#include "pkixder.h"
#include "pkix/pkixnss.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result
CheckValidity(const SECItem& encodedValidity, PRTime time)
{
  Input validity;
  if (validity.Init(encodedValidity.data, encodedValidity.len) != Success) {
    return Result::ERROR_EXPIRED_CERTIFICATE;
  }
  PRTime notBefore;
  if (der::TimeChoice(validity, notBefore) != Success) {
    return Result::ERROR_EXPIRED_CERTIFICATE;
  }
  if (time < notBefore) {
    return Result::ERROR_EXPIRED_CERTIFICATE;
  }

  PRTime notAfter;
  if (der::TimeChoice(validity, notAfter) != Success) {
    return Result::ERROR_EXPIRED_CERTIFICATE;
  }
  if (time > notAfter) {
    return Result::ERROR_EXPIRED_CERTIFICATE;
  }

  return der::End(validity);
}





inline uint8_t KeyUsageToBitMask(KeyUsage keyUsage)
{
  PR_ASSERT(keyUsage != KeyUsage::noParticularKeyUsageRequired);
  return 0x80u >> static_cast<uint8_t>(keyUsage);
}

Result
CheckKeyUsage(EndEntityOrCA endEntityOrCA, const SECItem* encodedKeyUsage,
              KeyUsage requiredKeyUsageIfPresent)
{
  if (!encodedKeyUsage) {
    
    
    
    
    
    
    
    
    
    
    
    return Success;
  }

  Input input;
  if (input.Init(encodedKeyUsage->data, encodedKeyUsage->len) != Success) {
    return Result::ERROR_INADEQUATE_KEY_USAGE;
  }
  Input value;
  if (der::ExpectTagAndGetValue(input, der::BIT_STRING, value) != Success) {
    return Result::ERROR_INADEQUATE_KEY_USAGE;
  }

  uint8_t numberOfPaddingBits;
  if (value.Read(numberOfPaddingBits) != Success) {
    return Result::ERROR_INADEQUATE_KEY_USAGE;
  }
  if (numberOfPaddingBits > 7) {
    return Result::ERROR_INADEQUATE_KEY_USAGE;
  }

  uint8_t bits;
  if (value.Read(bits) != Success) {
    
    return Result::ERROR_INADEQUATE_KEY_USAGE;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (requiredKeyUsageIfPresent != KeyUsage::noParticularKeyUsageRequired) {
    
    if ((bits & KeyUsageToBitMask(requiredKeyUsageIfPresent)) == 0) {
      return Result::ERROR_INADEQUATE_KEY_USAGE;
    }
  }

  if (endEntityOrCA != EndEntityOrCA::MustBeCA) {
    
    
    
    
    if ((bits & KeyUsageToBitMask(KeyUsage::keyCertSign)) != 0) {
      return Result::ERROR_INADEQUATE_KEY_USAGE;
    }
  }

  
  while (!value.AtEnd()) {
    if (value.Read(bits) != Success) {
      return Result::ERROR_INADEQUATE_KEY_USAGE;
    }
  }

  
  uint8_t paddingMask = static_cast<uint8_t>((1 << numberOfPaddingBits) - 1);
  if ((bits & paddingMask) != 0) {
    return Result::ERROR_INADEQUATE_KEY_USAGE;
  }

  return Success;
}










 const CertPolicyId CertPolicyId::anyPolicy = {
  4, { (40*2)+5, 29, 32, 0 }
};

bool CertPolicyId::IsAnyPolicy() const
{
  return this == &anyPolicy ||
         (numBytes == anyPolicy.numBytes &&
          !memcmp(bytes, anyPolicy.bytes, anyPolicy.numBytes));
}





inline Result
CheckPolicyInformation(Input& input, EndEntityOrCA endEntityOrCA,
                       const CertPolicyId& requiredPolicy,
                        bool& found)
{
  if (input.MatchTLV(der::OIDTag, requiredPolicy.numBytes,
                     requiredPolicy.bytes)) {
    found = true;
  } else if (endEntityOrCA == EndEntityOrCA::MustBeCA &&
             input.MatchTLV(der::OIDTag, CertPolicyId::anyPolicy.numBytes,
                            CertPolicyId::anyPolicy.bytes)) {
    found = true;
  }

  
  
  
  
  
  

  
  input.SkipToEnd();

  return Success;
}


Result
CheckCertificatePolicies(EndEntityOrCA endEntityOrCA,
                         const SECItem* encodedCertificatePolicies,
                         const SECItem* encodedInhibitAnyPolicy,
                         TrustLevel trustLevel,
                         const CertPolicyId& requiredPolicy)
{
  if (requiredPolicy.numBytes == 0 ||
      requiredPolicy.numBytes > sizeof requiredPolicy.bytes) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  
  
  
  if (requiredPolicy.IsAnyPolicy()) {
    return Success;
  }

  
  
  
  if (encodedInhibitAnyPolicy) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }

  
  
  
  
  if (trustLevel == TrustLevel::TrustAnchor &&
      endEntityOrCA == EndEntityOrCA::MustBeCA) {
    return Success;
  }

  if (!encodedCertificatePolicies) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }

  bool found = false;

  Input input;
  if (input.Init(encodedCertificatePolicies->data,
                 encodedCertificatePolicies->len) != Success) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }
  if (der::NestedOf(input, der::SEQUENCE, der::SEQUENCE, der::EmptyAllowed::No,
                    bind(CheckPolicyInformation, _1, endEntityOrCA,
                         requiredPolicy, ref(found))) != Success) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }
  if (der::End(input) != Success) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }
  if (!found) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }

  return Success;
}

static const long UNLIMITED_PATH_LEN = -1; 




static Result
DecodeBasicConstraints(Input& input,  bool& isCA,
                        long& pathLenConstraint)
{
  
  
  
  
  
  
  if (der::OptionalBoolean(input, true, isCA) != Success) {
    return Result::ERROR_EXTENSION_VALUE_INVALID;
  }

  
  
  
  if (der::OptionalInteger(input, UNLIMITED_PATH_LEN, pathLenConstraint)
        != Success) {
    return Result::ERROR_EXTENSION_VALUE_INVALID;
  }

  return Success;
}


Result
CheckBasicConstraints(EndEntityOrCA endEntityOrCA,
                      const SECItem* encodedBasicConstraints,
                      const der::Version version, TrustLevel trustLevel,
                      unsigned int subCACount)
{
  bool isCA = false;
  long pathLenConstraint = UNLIMITED_PATH_LEN;

  if (encodedBasicConstraints) {
    Input input;
    if (input.Init(encodedBasicConstraints->data,
                   encodedBasicConstraints->len) != Success) {
      return Result::ERROR_EXTENSION_VALUE_INVALID;
    }
    if (der::Nested(input, der::SEQUENCE,
                    bind(DecodeBasicConstraints, _1, ref(isCA),
                         ref(pathLenConstraint))) != Success) {
      return Result::ERROR_EXTENSION_VALUE_INVALID;
    }
    if (der::End(input) != Success) {
      return Result::ERROR_EXTENSION_VALUE_INVALID;
    }
  } else {
    
    
    
    
    
    
    
    
    
    if (endEntityOrCA == EndEntityOrCA::MustBeCA &&
        trustLevel == TrustLevel::TrustAnchor && version == der::Version::v1) {
      isCA = true;
    }
  }

  if (endEntityOrCA == EndEntityOrCA::MustBeEndEntity) {
    

    if (isCA) {
      
      
      
      
      
      
      
      
      
      
      return Result::ERROR_CA_CERT_INVALID;
    }

    return Success;
  }

  PORT_Assert(endEntityOrCA == EndEntityOrCA::MustBeCA);

  
  if (!isCA) {
    return Result::ERROR_CA_CERT_INVALID;
  }

  if (pathLenConstraint >= 0 &&
      static_cast<long>(subCACount) > pathLenConstraint) {
    return Result::ERROR_PATH_LEN_CONSTRAINT_INVALID;
  }

  return Success;
}



inline void
PORT_FreeArena_false(PLArenaPool* arena) {
  
  
  return PORT_FreeArena(arena, PR_FALSE);
}



Result
CheckNameConstraints(const SECItem& encodedNameConstraints,
                     const BackCert& firstChild,
                     KeyPurposeId requiredEKUIfPresent)
{
  ScopedPtr<PLArenaPool, PORT_FreeArena_false>
    arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return Result::FATAL_ERROR_NO_MEMORY;
  }

  
  const CERTNameConstraints* constraints =
    CERT_DecodeNameConstraintsExtension(arena.get(), &encodedNameConstraints);
  if (!constraints) {
    return MapPRErrorCodeToResult(PR_GetError());
  }

  for (const BackCert* child = &firstChild; child; child = child->childCert) {
    ScopedPtr<CERTCertificate, CERT_DestroyCertificate>
      nssCert(CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                      const_cast<SECItem*>(&child->GetDER()),
                                      nullptr, false, true));
    if (!nssCert) {
      return MapPRErrorCodeToResult(PR_GetError());
    }

    bool includeCN = child->endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
                     requiredEKUIfPresent == KeyPurposeId::id_kp_serverAuth;
    
    const CERTGeneralName*
      names(CERT_GetConstrainedCertificateNames(nssCert.get(), arena.get(),
                                                includeCN));
    if (!names) {
      return MapPRErrorCodeToResult(PR_GetError());
    }

    CERTGeneralName* currentName = const_cast<CERTGeneralName*>(names);
    do {
      if (CERT_CheckNameSpace(arena.get(), constraints, currentName)
            != SECSuccess) {
        
        
        
        return Result::ERROR_CERT_NOT_IN_NAME_SPACE;
      }
      currentName = CERT_GetNextGeneralName(currentName);
    } while (currentName != names);
  }

  return Success;
}



static Result
MatchEKU(Input& value, KeyPurposeId requiredEKU,
         EndEntityOrCA endEntityOrCA,  bool& found,
          bool& foundOCSPSigning)
{
  
  

  
  
  
  
  
  
  
  
  
  static const uint8_t server[] = { (40*1)+3, 6, 1, 5, 5, 7, 3, 1 };
  static const uint8_t client[] = { (40*1)+3, 6, 1, 5, 5, 7, 3, 2 };
  static const uint8_t code  [] = { (40*1)+3, 6, 1, 5, 5, 7, 3, 3 };
  static const uint8_t email [] = { (40*1)+3, 6, 1, 5, 5, 7, 3, 4 };
  static const uint8_t ocsp  [] = { (40*1)+3, 6, 1, 5, 5, 7, 3, 9 };

  
  
  
  static const uint8_t serverStepUp[] =
    { (40*2)+16, 128+6,72, 1, 128+6,128+120,66, 4, 1 };

  bool match = false;

  if (!found) {
    switch (requiredEKU) {
      case KeyPurposeId::id_kp_serverAuth:
        
        
        
        
        match = value.MatchRest(server) ||
                (endEntityOrCA == EndEntityOrCA::MustBeCA &&
                 value.MatchRest(serverStepUp));
        break;

      case KeyPurposeId::id_kp_clientAuth:
        match = value.MatchRest(client);
        break;

      case KeyPurposeId::id_kp_codeSigning:
        match = value.MatchRest(code);
        break;

      case KeyPurposeId::id_kp_emailProtection:
        match = value.MatchRest(email);
        break;

      case KeyPurposeId::id_kp_OCSPSigning:
        match = value.MatchRest(ocsp);
        break;

      case KeyPurposeId::anyExtendedKeyUsage:
        PR_NOT_REACHED("anyExtendedKeyUsage should start with found==true");
        return Result::FATAL_ERROR_LIBRARY_FAILURE;

      default:
        PR_NOT_REACHED("unrecognized EKU");
        return Result::FATAL_ERROR_LIBRARY_FAILURE;
    }
  }

  if (match) {
    found = true;
    if (requiredEKU == KeyPurposeId::id_kp_OCSPSigning) {
      foundOCSPSigning = true;
    }
  } else if (value.MatchRest(ocsp)) {
    foundOCSPSigning = true;
  }

  value.SkipToEnd(); 

  return Success;
}

Result
CheckExtendedKeyUsage(EndEntityOrCA endEntityOrCA,
                      const SECItem* encodedExtendedKeyUsage,
                      KeyPurposeId requiredEKU)
{
  
  
  
  

  bool foundOCSPSigning = false;

  if (encodedExtendedKeyUsage) {
    bool found = requiredEKU == KeyPurposeId::anyExtendedKeyUsage;

    Input input;
    if (input.Init(encodedExtendedKeyUsage->data,
                   encodedExtendedKeyUsage->len) != Success) {
      return Result::ERROR_INADEQUATE_CERT_TYPE;
    }
    if (der::NestedOf(input, der::SEQUENCE, der::OIDTag, der::EmptyAllowed::No,
                      bind(MatchEKU, _1, requiredEKU, endEntityOrCA,
                           ref(found), ref(foundOCSPSigning)))
          != Success) {
      return Result::ERROR_INADEQUATE_CERT_TYPE;
    }
    if (der::End(input) != Success) {
      return Result::ERROR_INADEQUATE_CERT_TYPE;
    }

    
    
    if (!found) {
      return Result::ERROR_INADEQUATE_CERT_TYPE;
    }
  }

  

  if (endEntityOrCA == EndEntityOrCA::MustBeEndEntity) {
    
    
    
    
    
    
    
    
    
    
    
    
    if (foundOCSPSigning && requiredEKU != KeyPurposeId::id_kp_OCSPSigning) {
      return Result::ERROR_INADEQUATE_CERT_TYPE;
    }
    
    
    
    
    
    
    
    
    
    if (!foundOCSPSigning && requiredEKU == KeyPurposeId::id_kp_OCSPSigning) {
      return Result::ERROR_INADEQUATE_CERT_TYPE;
    }
  }

  return Success;
}

Result
CheckIssuerIndependentProperties(TrustDomain& trustDomain,
                                 const BackCert& cert,
                                 PRTime time,
                                 KeyUsage requiredKeyUsageIfPresent,
                                 KeyPurposeId requiredEKUIfPresent,
                                 const CertPolicyId& requiredPolicy,
                                 unsigned int subCACount,
                 TrustLevel* trustLevelOut)
{
  Result rv;

  const EndEntityOrCA endEntityOrCA = cert.endEntityOrCA;

  TrustLevel trustLevel;
  rv = trustDomain.GetCertTrust(endEntityOrCA, requiredPolicy, cert.GetDER(),
                                &trustLevel);
  if (rv != Success) {
    return rv;
  }
  if (trustLevel == TrustLevel::ActivelyDistrusted) {
    return Result::ERROR_UNTRUSTED_CERT;
  }
  if (trustLevel != TrustLevel::TrustAnchor &&
      trustLevel != TrustLevel::InheritsTrust) {
    
    return Result::FATAL_ERROR_INVALID_STATE;
  }
  if (trustLevelOut) {
    *trustLevelOut = trustLevel;
  }

  

  

  
  rv = CheckKeyUsage(endEntityOrCA, cert.GetKeyUsage(),
                     requiredKeyUsageIfPresent);
  if (rv != Success) {
    return rv;
  }

  
  rv = CheckCertificatePolicies(endEntityOrCA, cert.GetCertificatePolicies(),
                                cert.GetInhibitAnyPolicy(), trustLevel,
                                requiredPolicy);
  if (rv != Success) {
    return rv;
  }

  
  

  
  

  

  
  

  
  rv = CheckBasicConstraints(endEntityOrCA, cert.GetBasicConstraints(),
                             cert.GetVersion(), trustLevel, subCACount);
  if (rv != Success) {
    return rv;
  }

  

  
  

  
  rv = CheckExtendedKeyUsage(endEntityOrCA, cert.GetExtKeyUsage(),
                             requiredEKUIfPresent);
  if (rv != Success) {
    return rv;
  }

  
  
  

  
  

  
  
  rv = CheckValidity(cert.GetValidity(), time);
  if (rv != Success) {
    return rv;
  }

  return Success;
}

} } 
