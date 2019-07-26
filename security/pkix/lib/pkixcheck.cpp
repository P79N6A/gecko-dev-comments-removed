























#include <limits>

#include "pkix/bind.h"
#include "pkix/pkix.h"
#include "pkixcheck.h"
#include "pkixder.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result
CheckTimes(const CERTCertificate* cert, PRTime time)
{
  PR_ASSERT(cert);

  SECCertTimeValidity validity = CERT_CheckCertValidTimes(cert, time, false);
  if (validity != secCertTimeValid) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }

  return Success;
}



Result
CheckKeyUsage(EndEntityOrCA endEntityOrCA,
              const SECItem* encodedKeyUsage,
              KeyUsages requiredKeyUsagesIfPresent,
              PLArenaPool* arena)
{
  if (!encodedKeyUsage) {
    
    
    
    
    
    
    
    
    
    
    
    return Success;
  }

  SECItem tmpItem;
  Result rv = MapSECStatus(SEC_QuickDERDecodeItem(arena, &tmpItem,
                              SEC_ASN1_GET(SEC_BitStringTemplate),
                              encodedKeyUsage));
  if (rv != Success) {
    return rv;
  }

  

  KeyUsages allowedKeyUsages = tmpItem.data[0];
  if ((allowedKeyUsages & requiredKeyUsagesIfPresent)
        != requiredKeyUsagesIfPresent) {
    return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
  }

  if (endEntityOrCA == EndEntityOrCA::MustBeCA) {
   
   
   
   if ((allowedKeyUsages & KU_KEY_CERT_SIGN) == 0) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
    }
  } else {
    
    
    
    
    
    
    
    
    
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





inline der::Result
CheckPolicyInformation(der::Input& input, EndEntityOrCA endEntityOrCA,
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

  return der::Success;
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
    return Fail(FatalError, SEC_ERROR_INVALID_ARGS);
  }

  
  
  
  if (requiredPolicy.IsAnyPolicy()) {
    return Success;
  }

  
  
  
  if (encodedInhibitAnyPolicy) {
    return Fail(RecoverableError, SEC_ERROR_POLICY_VALIDATION_FAILED);
  }

  
  
  
  
  if (trustLevel == TrustLevel::TrustAnchor &&
      endEntityOrCA == EndEntityOrCA::MustBeCA) {
    return Success;
  }

  if (!encodedCertificatePolicies) {
    return Fail(RecoverableError, SEC_ERROR_POLICY_VALIDATION_FAILED);
  }

  bool found = false;

  der::Input input;
  if (input.Init(encodedCertificatePolicies->data,
                 encodedCertificatePolicies->len) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_POLICY_VALIDATION_FAILED);
  }
  if (der::NestedOf(input, der::SEQUENCE, der::SEQUENCE, der::EmptyAllowed::No,
                    bind(CheckPolicyInformation, _1, endEntityOrCA,
                         requiredPolicy, ref(found))) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_POLICY_VALIDATION_FAILED);
  }
  if (der::End(input) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_POLICY_VALIDATION_FAILED);
  }
  if (!found) {
    return Fail(RecoverableError, SEC_ERROR_POLICY_VALIDATION_FAILED);
  }

  return Success;
}

static const long UNLIMITED_PATH_LEN = -1; 




static der::Result
DecodeBasicConstraints(der::Input& input,  bool& isCA,
                        long& pathLenConstraint)
{
  
  
  
  
  
  
  if (der::OptionalBoolean(input, true, isCA) != der::Success) {
    return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
  }

  
  
  
  if (OptionalInteger(input, UNLIMITED_PATH_LEN, pathLenConstraint)
        != der::Success) {
    return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
  }

  return der::Success;
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
    der::Input input;
    if (input.Init(encodedBasicConstraints->data,
                   encodedBasicConstraints->len) != der::Success) {
      return Fail(RecoverableError, SEC_ERROR_EXTENSION_VALUE_INVALID);
    }
    if (der::Nested(input, der::SEQUENCE,
                    bind(DecodeBasicConstraints, _1, ref(isCA),
                         ref(pathLenConstraint))) != der::Success) {
      return Fail(RecoverableError, SEC_ERROR_EXTENSION_VALUE_INVALID);
    }
    if (der::End(input) != der::Success) {
      return Fail(RecoverableError, SEC_ERROR_EXTENSION_VALUE_INVALID);
    }
  } else {
    
    
    
    
    
    
    
    
    
    if (endEntityOrCA == EndEntityOrCA::MustBeCA &&
        trustLevel == TrustLevel::TrustAnchor && version == der::Version::v1) {
      isCA = true;
    }
  }

  if (endEntityOrCA == EndEntityOrCA::MustBeEndEntity) {
    

    if (isCA) {
      
      
      
      
      
      
      
      
      
      
      return Fail(RecoverableError, SEC_ERROR_CA_CERT_INVALID);
    }

    return Success;
  }

  PORT_Assert(endEntityOrCA == EndEntityOrCA::MustBeCA);

  
  if (!isCA) {
    return Fail(RecoverableError, SEC_ERROR_CA_CERT_INVALID);
  }

  if (pathLenConstraint >= 0 &&
      static_cast<long>(subCACount) > pathLenConstraint) {
    return Fail(RecoverableError, SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID);
  }

  return Success;
}

Result
BackCert::GetConstrainedNames( const CERTGeneralName** result)
{
  if (!constrainedNames) {
    if (!GetArena()) {
      return FatalError;
    }

    constrainedNames =
      CERT_GetConstrainedCertificateNames(nssCert, arena.get(),
                                          includeCN == IncludeCN::Yes);
    if (!constrainedNames) {
      return MapSECStatus(SECFailure);
    }
  }

  *result = constrainedNames;
  return Success;
}


Result
CheckNameConstraints(BackCert& cert)
{
  if (!cert.encodedNameConstraints) {
    return Success;
  }

  PLArenaPool* arena = cert.GetArena();
  if (!arena) {
    return FatalError;
  }

  
  const CERTNameConstraints* constraints =
    CERT_DecodeNameConstraintsExtension(arena, cert.encodedNameConstraints);
  if (!constraints) {
    return MapSECStatus(SECFailure);
  }

  for (BackCert* prev = cert.childCert; prev; prev = prev->childCert) {
    const CERTGeneralName* names = nullptr;
    Result rv = prev->GetConstrainedNames(&names);
    if (rv != Success) {
      return rv;
    }
    PORT_Assert(names);
    CERTGeneralName* currentName = const_cast<CERTGeneralName*>(names);
    do {
      if (CERT_CheckNameSpace(arena, constraints, currentName) != SECSuccess) {
        
        
        
        
        return Fail(RecoverableError, SEC_ERROR_CERT_NOT_IN_NAME_SPACE);
      }
      currentName = CERT_GetNextGeneralName(currentName);
    } while (currentName != names);
  }

  return Success;
}



static der::Result
MatchEKU(der::Input& value, KeyPurposeId requiredEKU,
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
        return der::Fail(SEC_ERROR_LIBRARY_FAILURE);

      default:
        PR_NOT_REACHED("unrecognized EKU");
        return der::Fail(SEC_ERROR_LIBRARY_FAILURE);
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

  return der::Success;
}

Result
CheckExtendedKeyUsage(EndEntityOrCA endEntityOrCA,
                      const SECItem* encodedExtendedKeyUsage,
                      KeyPurposeId requiredEKU)
{
  
  
  
  

  bool foundOCSPSigning = false;

  if (encodedExtendedKeyUsage) {
    bool found = requiredEKU == KeyPurposeId::anyExtendedKeyUsage;

    der::Input input;
    if (input.Init(encodedExtendedKeyUsage->data,
                   encodedExtendedKeyUsage->len) != der::Success) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_CERT_TYPE);
    }
    if (der::NestedOf(input, der::SEQUENCE, der::OIDTag, der::EmptyAllowed::No,
                      bind(MatchEKU, _1, requiredEKU, endEntityOrCA,
                           ref(found), ref(foundOCSPSigning)))
          != der::Success) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_CERT_TYPE);
    }
    if (der::End(input) != der::Success) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_CERT_TYPE);
    }

    
    
    if (!found) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_CERT_TYPE);
    }
  }

  

  if (endEntityOrCA == EndEntityOrCA::MustBeEndEntity) {
    
    
    
    
    
    
    
    
    
    
    
    
    if (foundOCSPSigning && requiredEKU != KeyPurposeId::id_kp_OCSPSigning) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_CERT_TYPE);
    }
    
    
    
    
    
    
    
    
    
    if (!foundOCSPSigning && requiredEKU == KeyPurposeId::id_kp_OCSPSigning) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_CERT_TYPE);
    }
  }

  return Success;
}

Result
CheckIssuerIndependentProperties(TrustDomain& trustDomain,
                                 BackCert& cert,
                                 PRTime time,
                                 EndEntityOrCA endEntityOrCA,
                                 KeyUsages requiredKeyUsagesIfPresent,
                                 KeyPurposeId requiredEKUIfPresent,
                                 const CertPolicyId& requiredPolicy,
                                 unsigned int subCACount,
                 TrustLevel* trustLevelOut)
{
  Result rv;

  TrustLevel trustLevel;
  rv = MapSECStatus(trustDomain.GetCertTrust(endEntityOrCA, requiredPolicy,
                                             cert.GetDER(), &trustLevel));
  if (rv != Success) {
    return rv;
  }
  if (trustLevel == TrustLevel::ActivelyDistrusted) {
    return Fail(RecoverableError, SEC_ERROR_UNTRUSTED_CERT);
  }
  if (trustLevel != TrustLevel::TrustAnchor &&
      trustLevel != TrustLevel::InheritsTrust) {
    
    PORT_SetError(PR_INVALID_STATE_ERROR);
    return FatalError;
  }
  if (trustLevelOut) {
    *trustLevelOut = trustLevel;
  }

  
  
  
  
  der::Version version = (!cert.GetNSSCert()->version.data &&
                          !cert.GetNSSCert()->version.len) ? der::Version::v1
                                                           : der::Version::v3;

  PLArenaPool* arena = cert.GetArena();
  if (!arena) {
    return FatalError;
  }

  

  

  
  rv = CheckKeyUsage(endEntityOrCA, cert.encodedKeyUsage,
                     requiredKeyUsagesIfPresent, arena);
  if (rv != Success) {
    return rv;
  }

  
  rv = CheckCertificatePolicies(endEntityOrCA, cert.encodedCertificatePolicies,
                                cert.encodedInhibitAnyPolicy, trustLevel,
                                requiredPolicy);
  if (rv != Success) {
    return rv;
  }

  
  

  
  

  

  
  

  
  rv = CheckBasicConstraints(endEntityOrCA, cert.encodedBasicConstraints,
                             version, trustLevel, subCACount);
  if (rv != Success) {
    return rv;
  }

  

  
  

  
  rv = CheckExtendedKeyUsage(endEntityOrCA, cert.encodedExtendedKeyUsage,
                             requiredEKUIfPresent);
  if (rv != Success) {
    return rv;
  }

  
  
  

  
  

  
  
  rv = CheckTimes(cert.GetNSSCert(), time);
  if (rv != Success) {
    return rv;
  }

  return Success;
}

} } 
