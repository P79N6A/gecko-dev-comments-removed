























#include <limits>

#include "pkix/bind.h"
#include "pkix/pkix.h"
#include "pkix/ScopedPtr.h"
#include "pkixcheck.h"
#include "pkixder.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result
CheckValidity(const SECItem& encodedValidity, PRTime time)
{
  der::Input validity;
  if (validity.Init(encodedValidity.data, encodedValidity.len)
        != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }
  PRTime notBefore;
  if (der::TimeChoice(validity, notBefore) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }
  if (time < notBefore) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }

  PRTime notAfter;
  if (der::TimeChoice(validity, notAfter) != der::Success) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }
  if (time > notAfter) {
    return Fail(RecoverableError, SEC_ERROR_EXPIRED_CERTIFICATE);
  }

  if (der::End(validity) != der::Success) {
    return MapSECStatus(SECFailure);
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



inline void
PORT_FreeArena_false(PLArenaPool* arena) {
  
  
  return PORT_FreeArena(arena, PR_FALSE);
}

Result
CheckNameConstraints(const BackCert& cert, KeyPurposeId requiredEKUIfPresent)
{
  
  
  static const char constraintFranceGov[] =
                                     "\x30\x5D" 
                                     "\xA0\x5B" 
                                     "\x30\x05" 
                                     "\x82\x03" 
                                     ".fr"
                                     "\x30\x05\x82\x03" 
                                     ".gp"
                                     "\x30\x05\x82\x03"
                                     ".gf"
                                     "\x30\x05\x82\x03"
                                     ".mq"
                                     "\x30\x05\x82\x03"
                                     ".re"
                                     "\x30\x05\x82\x03"
                                     ".yt"
                                     "\x30\x05\x82\x03"
                                     ".pm"
                                     "\x30\x05\x82\x03"
                                     ".bl"
                                     "\x30\x05\x82\x03"
                                     ".mf"
                                     "\x30\x05\x82\x03"
                                     ".wf"
                                     "\x30\x05\x82\x03"
                                     ".pf"
                                     "\x30\x05\x82\x03"
                                     ".nc"
                                     "\x30\x05\x82\x03"
                                     ".tf";

  


  static const char rawANSSISubject[] =
                                 "\x30\x81\x85\x31\x0B\x30\x09\x06\x03\x55\x04"
                                 "\x06\x13\x02\x46\x52\x31\x0F\x30\x0D\x06\x03"
                                 "\x55\x04\x08\x13\x06\x46\x72\x61\x6E\x63\x65"
                                 "\x31\x0E\x30\x0C\x06\x03\x55\x04\x07\x13\x05"
                                 "\x50\x61\x72\x69\x73\x31\x10\x30\x0E\x06\x03"
                                 "\x55\x04\x0A\x13\x07\x50\x4D\x2F\x53\x47\x44"
                                 "\x4E\x31\x0E\x30\x0C\x06\x03\x55\x04\x0B\x13"
                                 "\x05\x44\x43\x53\x53\x49\x31\x0E\x30\x0C\x06"
                                 "\x03\x55\x04\x03\x13\x05\x49\x47\x43\x2F\x41"
                                 "\x31\x23\x30\x21\x06\x09\x2A\x86\x48\x86\xF7"
                                 "\x0D\x01\x09\x01\x16\x14\x69\x67\x63\x61\x40"
                                 "\x73\x67\x64\x6E\x2E\x70\x6D\x2E\x67\x6F\x75"
                                 "\x76\x2E\x66\x72";

  const SECItem ANSSI_SUBJECT = {
    siBuffer,
    reinterpret_cast<uint8_t *>(const_cast<char *>(rawANSSISubject)),
    sizeof(rawANSSISubject) - 1
  };

  const SECItem PERMIT_FRANCE_GOV_NC = {
    siBuffer,
    reinterpret_cast<uint8_t *>(const_cast<char *>(constraintFranceGov)),
    sizeof(constraintFranceGov) - 1
  };

  const SECItem* nameConstraintsToUse = cert.GetNameConstraints();

  if (!nameConstraintsToUse) {
    if (SECITEM_ItemsAreEqual(&cert.GetSubject(), &ANSSI_SUBJECT)) {
      nameConstraintsToUse = &PERMIT_FRANCE_GOV_NC;
    } else {
      return Success;
    }
   }

  ScopedPtr<PLArenaPool, PORT_FreeArena_false>
    arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
  if (!arena) {
    return MapSECStatus(SECFailure);
  }

  
  const CERTNameConstraints* constraints =
    CERT_DecodeNameConstraintsExtension(arena.get(), nameConstraintsToUse);
  if (!constraints) {
    return MapSECStatus(SECFailure);
  }

  for (const BackCert* child = cert.childCert; child;
       child = child->childCert) {
    ScopedPtr<CERTCertificate, CERT_DestroyCertificate>
      nssCert(CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                      const_cast<SECItem*>(&child->GetDER()),
                                      nullptr, false, true));
    if (!nssCert) {
      return MapSECStatus(SECFailure);
    }

    bool includeCN = child->endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
                     requiredEKUIfPresent == KeyPurposeId::id_kp_serverAuth;
    
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
