























#include <limits>

#include "pkix/bind.h"
#include "pkix/pkix.h"
#include "pkixcheck.h"
#include "pkixder.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result
CheckTimes(const CERTValidity& validity, PRTime time)
{
  der::Input notBeforeInput;
  if (notBeforeInput.Init(validity.notBefore.data, validity.notBefore.len)
        != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }
  PRTime notBefore;
  if (der::TimeChoice(validity.notBefore.type, notBeforeInput, notBefore)
        != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }
  if (time < notBefore) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }

  der::Input notAfterInput;
  if (notAfterInput.Init(validity.notAfter.data, validity.notAfter.len)
        != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }
  PRTime notAfter;
  if (der::TimeChoice(validity.notAfter.type, notAfterInput, notAfter)
        != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }
  if (time > notAfter) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }

  return Success;
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

  der::Input input;
  if (input.Init(encodedKeyUsage->data, encodedKeyUsage->len) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
  }
  der::Input value;
  if (der::ExpectTagAndGetValue(input, der::BIT_STRING, value) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
  }

  uint8_t numberOfPaddingBits;
  if (value.Read(numberOfPaddingBits) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
  }
  if (numberOfPaddingBits > 7) {
    return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
  }

  uint8_t bits;
  if (value.Read(bits) != der::Success) {
    
    return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (requiredKeyUsageIfPresent != KeyUsage::noParticularKeyUsageRequired) {
    
    if ((bits & KeyUsageToBitMask(requiredKeyUsageIfPresent)) == 0) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
    }
  }

  if (endEntityOrCA != EndEntityOrCA::MustBeCA) {
    
    
    
    
    if ((bits & KeyUsageToBitMask(KeyUsage::keyCertSign)) != 0) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
    }
  }

  
  while (!value.AtEnd()) {
    if (value.Read(bits) != der::Success) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
    }
  }

  
  uint8_t paddingMask = static_cast<uint8_t>((1 << numberOfPaddingBits) - 1);
  if ((bits & paddingMask) != 0) {
    return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
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
                      der::Version version, TrustLevel trustLevel,
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
CheckNameConstraints(const BackCert& cert)
{
  if (!cert.encodedNameConstraints) {
    return Success;
  }

  ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return MapSECStatus(SECFailure);
  }

  
  const CERTNameConstraints* constraints =
    CERT_DecodeNameConstraintsExtension(arena.get(), cert.encodedNameConstraints);
  if (!constraints) {
    return MapSECStatus(SECFailure);
  }

  for (const BackCert* child = cert.childCert; child;
       child = child->childCert) {
    ScopedCERTCertificate
      nssCert(CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                      const_cast<SECItem*>(&child->GetDER()),
                                      nullptr, false, true));
    if (!nssCert) {
      return MapSECStatus(SECFailure);
    }

    bool includeCN = child->includeCN == BackCert::IncludeCN::Yes;
    
    const CERTGeneralName*
      names(CERT_GetConstrainedCertificateNames(nssCert.get(), arena.get(),
                                                includeCN));
    if (!names) {
      return MapSECStatus(SECFailure);
    }

    CERTGeneralName* currentName = const_cast<CERTGeneralName*>(names);
    do {
      if (CERT_CheckNameSpace(arena.get(), constraints, currentName)
            != SECSuccess) {
        
        
        
        
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
                                 KeyUsage requiredKeyUsageIfPresent,
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

  

  

  
  rv = CheckKeyUsage(endEntityOrCA, cert.encodedKeyUsage,
                     requiredKeyUsageIfPresent);
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
                             cert.version, trustLevel, subCACount);
  if (rv != Success) {
    return rv;
  }

  

  
  

  
  rv = CheckExtendedKeyUsage(endEntityOrCA, cert.encodedExtendedKeyUsage,
                             requiredEKUIfPresent);
  if (rv != Success) {
    return rv;
  }

  
  
  

  
  

  
  
  rv = CheckTimes(cert.GetNSSCert()->validity, time);
  if (rv != Success) {
    return rv;
  }

  return Success;
}

} } 
