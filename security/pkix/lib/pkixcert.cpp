























#include "pkix/bind.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {

Result
BackCert::Init()
{
  
  
  
  

  der::Input tbsCertificate;

  
  
  {
    der::Input input;
    if (input.Init(der.data, der.len) != der::Success) {
      return MapSECStatus(SECFailure);
    }
    der::Input certificate;
    if (der::ExpectTagAndGetValue(input, der::SEQUENCE, certificate)
          != der::Success) {
      return MapSECStatus(SECFailure);
    }
    if (der::End(input) != der::Success) {
      return MapSECStatus(SECFailure);
    }
    if (der::SignedData(certificate, tbsCertificate, signedData)
          != der::Success) {
      return MapSECStatus(SECFailure);
    }
    if (der::End(certificate) != der::Success) {
      return MapSECStatus(SECFailure);
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (der::OptionalVersion(tbsCertificate, version) != der::Success) {
    return MapSECStatus(SECFailure);
  }
  if (der::CertificateSerialNumber(tbsCertificate, serialNumber)
        != der::Success) {
    return MapSECStatus(SECFailure);
  }
  
  
  
  SECAlgorithmID signature;
  if (der::AlgorithmIdentifier(tbsCertificate, signature) != der::Success) {
    return MapSECStatus(SECFailure);
  }
  if (der::ExpectTagAndGetTLV(tbsCertificate, der::SEQUENCE, issuer)
        != der::Success) {
    return MapSECStatus(SECFailure);
  }
  if (der::ExpectTagAndGetValue(tbsCertificate, der::SEQUENCE, validity)
        != der::Success) {
    return MapSECStatus(SECFailure);
  }
  
  
  
  
  if (der::ExpectTagAndGetTLV(tbsCertificate, der::SEQUENCE, subject)
        != der::Success) {
    return MapSECStatus(SECFailure);
  }
  
  
  
  
  
  
  
  if (der::ExpectTagAndGetTLV(tbsCertificate, der::SEQUENCE,
                              subjectPublicKeyInfo) != der::Success) {
    return MapSECStatus(SECFailure);
  }

  static const uint8_t CSC = der::CONTEXT_SPECIFIC | der::CONSTRUCTED;

  
  
  if (version != der::Version::v1) {

    
    if (tbsCertificate.Peek(CSC | 1)) {
      if (der::ExpectTagAndSkipValue(tbsCertificate, CSC | 1) != der::Success) {
        return MapSECStatus(SECFailure);
      }
    }

    
    if (tbsCertificate.Peek(CSC | 2)) {
      if (der::ExpectTagAndSkipValue(tbsCertificate, CSC | 2) != der::Success) {
        return MapSECStatus(SECFailure);
      }
    }
  }

  
  if (version == der::Version::v3) {
    if (der::OptionalExtensions(tbsCertificate, CSC | 3,
                                bind(&BackCert::RememberExtension, this, _1, _2,
                                     _3)) != der::Success) {
      return MapSECStatus(SECFailure);
    }
  }

  if (der::End(tbsCertificate) != der::Success) {
    return MapSECStatus(SECFailure);
  }

  return Success;
}

der::Result
BackCert::RememberExtension(der::Input& extnID, const SECItem& extnValue,
                             bool& understood)
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

  SECItem* out = nullptr;

  
  
  
  
  
  
  SECItem dummyPolicyConstraints = { siBuffer, nullptr, 0 };

  
  
  

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
  }

  if (out) {
    
    
    if (extnValue.len == 0) {
      return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
    }
    if (out->len != 0) {
      
      return der::Fail(SEC_ERROR_EXTENSION_VALUE_INVALID);
    }
    *out = extnValue;
    understood = true;
  }

  return der::Success;
}

} } 
