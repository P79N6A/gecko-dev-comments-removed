























#include "pkix/bind.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result
BackCert::Init()
{
  Result rv;

  
  
  
  

  Reader tbsCertificate;

  
  
  {
    Reader input(der);
    Reader certificate;
    rv = der::ExpectTagAndGetValue(input, der::SEQUENCE, certificate);
    if (rv != Success) {
      return rv;
    }
    rv = der::End(input);
    if (rv != Success) {
      return rv;
    }
    rv = der::SignedData(certificate, tbsCertificate, signedData);
    if (rv != Success) {
      return rv;
    }
    rv = der::End(certificate);
    if (rv != Success) {
      return rv;
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  rv = der::OptionalVersion(tbsCertificate, version);
  if (rv != Success) {
    return rv;
  }
  rv = der::CertificateSerialNumber(tbsCertificate, serialNumber);
  if (rv != Success) {
    return rv;
  }
  
  
  
  SignatureAlgorithm signature;
  rv = der::SignatureAlgorithmIdentifier(tbsCertificate, signature);
  if (rv != Success) {
    return rv;
  }
  rv = der::ExpectTagAndGetTLV(tbsCertificate, der::SEQUENCE, issuer);
  if (rv != Success) {
    return rv;
  }
  rv = der::ExpectTagAndGetValue(tbsCertificate, der::SEQUENCE, validity);
  if (rv != Success) {
    return rv;
  }
  
  
  
  
  rv = der::ExpectTagAndGetTLV(tbsCertificate, der::SEQUENCE, subject);
  if (rv != Success) {
    return rv;
  }
  
  
  
  
  
  
  
  rv = der::ExpectTagAndGetTLV(tbsCertificate, der::SEQUENCE,
                               subjectPublicKeyInfo);
  if (rv != Success) {
    return rv;
  }

  static const uint8_t CSC = der::CONTEXT_SPECIFIC | der::CONSTRUCTED;

  
  
  if (version != der::Version::v1) {

    
    if (tbsCertificate.Peek(CSC | 1)) {
      rv = der::ExpectTagAndSkipValue(tbsCertificate, CSC | 1);
      if (rv != Success) {
        return rv;
      }
    }

    
    if (tbsCertificate.Peek(CSC | 2)) {
      rv = der::ExpectTagAndSkipValue(tbsCertificate, CSC | 2);
      if (rv != Success) {
        return rv;
      }
    }
  }

  
  
  
  
  if (version == der::Version::v3 || version == der::Version::v4) {
    rv = der::OptionalExtensions(tbsCertificate, CSC | 3,
                                 bind(&BackCert::RememberExtension, this, _1,
                                      _2, _3, _4));
    if (rv != Success) {
      return rv;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (criticalNetscapeCertificateType.GetLength() > 0 &&
        (basicConstraints.GetLength() == 0 || extKeyUsage.GetLength() == 0)) {
      return Result::ERROR_UNKNOWN_CRITICAL_EXTENSION;
    }
  }

  return der::End(tbsCertificate);
}



Result
BackCert::RememberExtension(Reader& extnID, const Input& extnValue,
                            bool critical,  bool& understood)
{
  understood = false;

  
  static const uint8_t id_ce_keyUsage[] = {
    0x55, 0x1d, 0x0f
  };
  
  static const uint8_t id_ce_subjectAltName[] = {
    0x55, 0x1d, 0x11
  };
  
  static const uint8_t id_ce_basicConstraints[] = {
    0x55, 0x1d, 0x13
  };
  
  static const uint8_t id_ce_nameConstraints[] = {
    0x55, 0x1d, 0x1e
  };
  
  static const uint8_t id_ce_certificatePolicies[] = {
    0x55, 0x1d, 0x20
  };
  
  static const uint8_t id_ce_policyConstraints[] = {
    0x55, 0x1d, 0x24
  };
  
  static const uint8_t id_ce_extKeyUsage[] = {
    0x55, 0x1d, 0x25
  };
  
  static const uint8_t id_ce_inhibitAnyPolicy[] = {
    0x55, 0x1d, 0x36
  };
  
  static const uint8_t id_pe_authorityInfoAccess[] = {
    0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01
  };
  
  static const uint8_t Netscape_certificate_type[] = {
    0x60, 0x86, 0x48, 0x01, 0x86, 0xf8, 0x42, 0x01, 0x01
  };

  Input* out = nullptr;

  
  
  
  
  
  
  Input dummyPolicyConstraints;

  
  
  

  if (extnID.MatchRest(id_ce_keyUsage)) {
    out = &keyUsage;
  } else if (extnID.MatchRest(id_ce_subjectAltName)) {
    out = &subjectAltName;
  } else if (extnID.MatchRest(id_ce_basicConstraints)) {
    out = &basicConstraints;
  } else if (extnID.MatchRest(id_ce_nameConstraints)) {
    out = &nameConstraints;
  } else if (extnID.MatchRest(id_ce_certificatePolicies)) {
    out = &certificatePolicies;
  } else if (extnID.MatchRest(id_ce_policyConstraints)) {
    out = &dummyPolicyConstraints;
  } else if (extnID.MatchRest(id_ce_extKeyUsage)) {
    out = &extKeyUsage;
  } else if (extnID.MatchRest(id_ce_inhibitAnyPolicy)) {
    out = &inhibitAnyPolicy;
  } else if (extnID.MatchRest(id_pe_authorityInfoAccess)) {
    out = &authorityInfoAccess;
  } else if (extnID.MatchRest(Netscape_certificate_type) && critical) {
    out = &criticalNetscapeCertificateType;
  }

  if (out) {
    
    
    if (extnValue.GetLength() == 0) {
      return Result::ERROR_EXTENSION_VALUE_INVALID;
    }
    if (out->Init(extnValue) != Success) {
      
      return Result::ERROR_EXTENSION_VALUE_INVALID;
    }
    understood = true;
  }

  return Success;
}

} } 
