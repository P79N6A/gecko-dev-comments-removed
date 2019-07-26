
















#include <limits>

#include "pkix/pkix.h"
#include "pkixcheck.h"
#include "pkixder.h"
#include "pkixutil.h"
#include "secder.h"

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
              bool isTrustAnchor,
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

  if (endEntityOrCA == MustBeCA) {
   
   
   
   if ((allowedKeyUsages & KU_KEY_CERT_SIGN) == 0) {
      return Fail(RecoverableError, SEC_ERROR_INADEQUATE_KEY_USAGE);
    }
  } else {
    
    
    
    
    
    
    
    
    
  }

  return Success;
}





Result
CheckCertificatePolicies(BackCert& cert, EndEntityOrCA endEntityOrCA,
                         bool isTrustAnchor, SECOidTag requiredPolicy)
{
  if (requiredPolicy == SEC_OID_X509_ANY_POLICY) {
    return Success;
  }

  
  
  if (requiredPolicy == SEC_OID_UNKNOWN) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return FatalError;
  }

  
  
  
  if (cert.encodedInhibitAnyPolicy) {
    PR_SetError(SEC_ERROR_POLICY_VALIDATION_FAILED, 0);
    return RecoverableError;
  }

  
  
  
  
  if (isTrustAnchor && endEntityOrCA == MustBeCA) {
    return Success;
  }

  if (!cert.encodedCertificatePolicies) {
    PR_SetError(SEC_ERROR_POLICY_VALIDATION_FAILED, 0);
    return RecoverableError;
  }

  ScopedPtr<CERTCertificatePolicies, CERT_DestroyCertificatePoliciesExtension>
    policies(CERT_DecodeCertificatePoliciesExtension(
                cert.encodedCertificatePolicies));
  if (!policies) {
    return MapSECStatus(SECFailure);
  }

  for (const CERTPolicyInfo* const* policyInfos = policies->policyInfos;
       *policyInfos; ++policyInfos) {
    if ((*policyInfos)->oid == requiredPolicy) {
      return Success;
    }
    
    if (endEntityOrCA == MustBeCA &&
        (*policyInfos)->oid == SEC_OID_X509_ANY_POLICY) {
      return Success;
    }
  }

  PR_SetError(SEC_ERROR_POLICY_VALIDATION_FAILED, 0);
  return RecoverableError;
}




der::Result
DecodeBasicConstraints(const SECItem* encodedBasicConstraints,
                       CERTBasicConstraints& basicConstraints)
{
  PR_ASSERT(encodedBasicConstraints);
  if (!encodedBasicConstraints) {
    return der::Fail(SEC_ERROR_INVALID_ARGS);
  }

  basicConstraints.isCA = false;
  basicConstraints.pathLenConstraint = 0;

  der::Input input;
  if (input.Init(encodedBasicConstraints->data, encodedBasicConstraints->len)
        != der::Success) {
    return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
  }

  if (der::ExpectTagAndIgnoreLength(input, der::SEQUENCE) != der::Success) {
    return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
  }

  bool isCA = false;
  
  
  
  
  
  
  if (der::OptionalBoolean(input, true, isCA) != der::Success) {
    return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
  }
  basicConstraints.isCA = isCA;

  if (input.Peek(der::INTEGER)) {
    SECItem pathLenConstraintEncoded;
    if (der::Integer(input, pathLenConstraintEncoded) != der::Success) {
      return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
    }
    long pathLenConstraint = DER_GetInteger(&pathLenConstraintEncoded);
    if (pathLenConstraint >= std::numeric_limits<int>::max() ||
        pathLenConstraint < 0) {
      return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
    }
    basicConstraints.pathLenConstraint = static_cast<int>(pathLenConstraint);
    
    
    
  } else if (basicConstraints.isCA) {
    
    basicConstraints.pathLenConstraint = CERT_UNLIMITED_PATH_CONSTRAINT;
  }

  if (der::End(input) != der::Success) {
    return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
  }
  return der::Success;
}


Result
CheckBasicConstraints(const BackCert& cert,
                      EndEntityOrCA endEntityOrCA,
                      bool isTrustAnchor,
                      unsigned int subCACount)
{
  CERTBasicConstraints basicConstraints;
  if (cert.encodedBasicConstraints) {
    if (DecodeBasicConstraints(cert.encodedBasicConstraints,
                               basicConstraints) != der::Success) {
      return RecoverableError;
    }
  } else {
    
    basicConstraints.isCA = false;
    basicConstraints.pathLenConstraint = 0;

    
    
    
    
    
    
    
    
    
    if (endEntityOrCA == MustBeCA && isTrustAnchor) {
      const CERTCertificate* nssCert = cert.GetNSSCert();
      
      
      
      if (!nssCert->version.data && !nssCert->version.len) {
        basicConstraints.isCA = true;
        basicConstraints.pathLenConstraint = CERT_UNLIMITED_PATH_CONSTRAINT;
      }
    }
  }

  if (endEntityOrCA == MustBeEndEntity) {
    

    if (basicConstraints.isCA) {
      
      
      
      
      
      
      
      
      
      
      return Fail(RecoverableError, SEC_ERROR_CA_CERT_INVALID);
    }

    return Success;
  }

  PORT_Assert(endEntityOrCA == MustBeCA);

  
  if (!basicConstraints.isCA) {
    return Fail(RecoverableError, SEC_ERROR_CA_CERT_INVALID);
  }

  if (basicConstraints.pathLenConstraint >= 0) {
    if (subCACount >
           static_cast<unsigned int>(basicConstraints.pathLenConstraint)) {
      return Fail(RecoverableError, SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID);
    }
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
                                          cnOptions == IncludeCN);
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
        
        
        
        
        PR_SetError(SEC_ERROR_CERT_NOT_IN_NAME_SPACE, 0);
        return RecoverableError;
      }
      currentName = CERT_GetNextGeneralName(currentName);
    } while (currentName != names);
  }

  return Success;
}



Result
CheckExtendedKeyUsage(EndEntityOrCA endEntityOrCA, const SECItem* encodedEKUs,
                      SECOidTag requiredEKU)
{
  
  
  

  
  
  
  

  bool foundOCSPSigning = false;

  if (encodedEKUs) {
    ScopedPtr<CERTOidSequence, CERT_DestroyOidSequence>
      seq(CERT_DecodeOidSequence(encodedEKUs));
    if (!seq) {
      PR_SetError(SEC_ERROR_INADEQUATE_CERT_TYPE, 0);
      return RecoverableError;
    }

    bool found = false;

    
    for (const SECItem* const* oids = seq->oids; oids && *oids; ++oids) {
      SECOidTag oidTag = SECOID_FindOIDTag(*oids);
      if (requiredEKU != SEC_OID_UNKNOWN && oidTag == requiredEKU) {
        found = true;
      } else {
        
        
        
        
        if (endEntityOrCA == MustBeCA &&
            requiredEKU == SEC_OID_EXT_KEY_USAGE_SERVER_AUTH &&
            oidTag == SEC_OID_NS_KEY_USAGE_GOVT_APPROVED) {
          found = true;
        }
      }
      if (oidTag == SEC_OID_OCSP_RESPONDER) {
        foundOCSPSigning = true;
      }
    }

    
    
    if (!found) {
      PR_SetError(SEC_ERROR_INADEQUATE_CERT_TYPE, 0);
      return RecoverableError;
    }
  }

  

  if (foundOCSPSigning) {
    
    
    
    
    
    if (requiredEKU != SEC_OID_OCSP_RESPONDER) {
      PR_SetError(SEC_ERROR_INADEQUATE_CERT_TYPE, 0);
      return RecoverableError;
    }
  } else if (requiredEKU == SEC_OID_OCSP_RESPONDER &&
             endEntityOrCA == MustBeEndEntity) {
    
    
    
    
    
    
    
    
    
    PR_SetError(SEC_ERROR_INADEQUATE_CERT_TYPE, 0);
    return RecoverableError;
  }

  return Success;
}

Result
CheckIssuerIndependentProperties(TrustDomain& trustDomain,
                                 BackCert& cert,
                                 PRTime time,
                                 EndEntityOrCA endEntityOrCA,
                                 KeyUsages requiredKeyUsagesIfPresent,
                                 SECOidTag requiredEKUIfPresent,
                                 SECOidTag requiredPolicy,
                                 unsigned int subCACount,
                 TrustDomain::TrustLevel* trustLevelOut)
{
  Result rv;

  TrustDomain::TrustLevel trustLevel;
  rv = MapSECStatus(trustDomain.GetCertTrust(endEntityOrCA,
                                             requiredPolicy,
                                             cert.GetNSSCert(),
                                             &trustLevel));
  if (rv != Success) {
    return rv;
  }
  if (trustLevel == TrustDomain::ActivelyDistrusted) {
    PORT_SetError(SEC_ERROR_UNTRUSTED_CERT);
    return RecoverableError;
  }
  if (trustLevel != TrustDomain::TrustAnchor &&
      trustLevel != TrustDomain::InheritsTrust) {
    
    PORT_SetError(PR_INVALID_STATE_ERROR);
    return FatalError;
  }
  if (trustLevelOut) {
    *trustLevelOut = trustLevel;
  }

  bool isTrustAnchor = endEntityOrCA == MustBeCA &&
                       trustLevel == TrustDomain::TrustAnchor;

  PLArenaPool* arena = cert.GetArena();
  if (!arena) {
    return FatalError;
  }

  

  

  
  rv = CheckKeyUsage(endEntityOrCA, isTrustAnchor, cert.encodedKeyUsage,
                     requiredKeyUsagesIfPresent, arena);
  if (rv != Success) {
    return rv;
  }

  
  rv = CheckCertificatePolicies(cert, endEntityOrCA, isTrustAnchor,
                                requiredPolicy);
  if (rv != Success) {
    return rv;
  }

  
  

  
  

  

  
  

  
  rv = CheckBasicConstraints(cert, endEntityOrCA, isTrustAnchor, subCACount);
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
