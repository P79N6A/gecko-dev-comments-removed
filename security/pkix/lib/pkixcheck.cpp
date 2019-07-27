























#include "pkixcheck.h"

#include "pkixder.h"
#include "pkixutil.h"

namespace mozilla { namespace pkix {




Result
CheckSignatureAlgorithm(Input signatureAlgorithmValue, Input signatureValue)
{
  
  der::PublicKeyAlgorithm publicKeyAlg;
  DigestAlgorithm digestAlg;
  Reader signatureAlgorithmReader(signatureAlgorithmValue);
  Result rv = der::SignatureAlgorithmIdentifierValue(signatureAlgorithmReader,
                                                     publicKeyAlg, digestAlg);
  if (rv != Success) {
    return rv;
  }
  rv = der::End(signatureAlgorithmReader);
  if (rv != Success) {
    return rv;
  }

  
  der::PublicKeyAlgorithm signedPublicKeyAlg;
  DigestAlgorithm signedDigestAlg;
  Reader signedSignatureAlgorithmReader(signatureValue);
  rv = der::SignatureAlgorithmIdentifierValue(signedSignatureAlgorithmReader,
                                              signedPublicKeyAlg,
                                              signedDigestAlg);
  if (rv != Success) {
    return rv;
  }
  rv = der::End(signedSignatureAlgorithmReader);
  if (rv != Success) {
    return rv;
  }

  
  
  
  
  
  
  
  
  
  
  if (publicKeyAlg != signedPublicKeyAlg || digestAlg != signedDigestAlg) {
    return Result::ERROR_SIGNATURE_ALGORITHM_MISMATCH;
  }

  return Success;
}



Result
CheckValidity(Input encodedValidity, Time time)
{
  Reader validity(encodedValidity);
  Time notBefore(Time::uninitialized);
  if (der::TimeChoice(validity, notBefore) != Success) {
    return Result::ERROR_INVALID_DER_TIME;
  }

  Time notAfter(Time::uninitialized);
  if (der::TimeChoice(validity, notAfter) != Success) {
    return Result::ERROR_INVALID_DER_TIME;
  }

  if (der::End(validity) != Success) {
    return Result::ERROR_INVALID_DER_TIME;
  }

  if (notBefore > notAfter) {
    return Result::ERROR_INVALID_DER_TIME;
  }

  if (time < notBefore) {
    return Result::ERROR_NOT_YET_VALID_CERTIFICATE;
  }

  if (time > notAfter) {
    return Result::ERROR_EXPIRED_CERTIFICATE;
  }

  return Success;
}



Result
CheckSubjectPublicKeyInfo(Reader& input, TrustDomain& trustDomain,
                          EndEntityOrCA endEntityOrCA)
{
  
  
  
  
  
  
  
  
  
  
  

  Reader algorithm;
  Input subjectPublicKey;
  Result rv = der::ExpectTagAndGetValue(input, der::SEQUENCE, algorithm);
  if (rv != Success) {
    return rv;
  }
  rv = der::BitStringWithNoUnusedBits(input, subjectPublicKey);
  if (rv != Success) {
    return rv;
  }
  rv = der::End(input);
  if (rv != Success) {
    return rv;
  }

  Reader subjectPublicKeyReader(subjectPublicKey);

  Reader algorithmOID;
  rv = der::ExpectTagAndGetValue(algorithm, der::OIDTag, algorithmOID);
  if (rv != Success) {
    return rv;
  }

  
  
  static const uint8_t rsaEncryption[] = {
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01
  };

  
  
  static const uint8_t id_ecPublicKey[] = {
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01
  };

  if (algorithmOID.MatchRest(id_ecPublicKey)) {
    
    
    
    

    Reader namedCurveOIDValue;
    rv = der::ExpectTagAndGetValue(algorithm, der::OIDTag,
                                   namedCurveOIDValue);
    if (rv != Success) {
      return rv;
    }

    
    
    static const uint8_t secp256r1[] = {
      0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07
    };

    
    
    static const uint8_t secp384r1[] = {
      0x2b, 0x81, 0x04, 0x00, 0x22
    };

    
    
    static const uint8_t secp521r1[] = {
      0x2b, 0x81, 0x04, 0x00, 0x23
    };

    
    
    NamedCurve curve;
    unsigned int bits;
    if (namedCurveOIDValue.MatchRest(secp256r1)) {
      curve = NamedCurve::secp256r1;
      bits = 256;
    } else if (namedCurveOIDValue.MatchRest(secp384r1)) {
      curve = NamedCurve::secp384r1;
      bits = 384;
    } else if (namedCurveOIDValue.MatchRest(secp521r1)) {
      curve = NamedCurve::secp521r1;
      bits = 521;
    } else {
      return Result::ERROR_UNSUPPORTED_ELLIPTIC_CURVE;
    }

    rv = trustDomain.CheckECDSACurveIsAcceptable(endEntityOrCA, curve);
    if (rv != Success) {
      return rv;
    }

    
    
    uint8_t compressedOrUncompressed;
    rv = subjectPublicKeyReader.Read(compressedOrUncompressed);
    if (rv != Success) {
      return rv;
    }
    if (compressedOrUncompressed != 0x04) {
      return Result::ERROR_UNSUPPORTED_EC_POINT_FORM;
    }

    
    
    Input point;
    rv = subjectPublicKeyReader.SkipToEnd(point);
    if (rv != Success) {
      return rv;
    }
    if (point.GetLength() != ((bits + 7) / 8u) * 2u) {
      return Result::ERROR_BAD_DER;
    }

    
    
    
  } else if (algorithmOID.MatchRest(rsaEncryption)) {
    
    
    rv = der::ExpectTagAndEmptyValue(algorithm, der::NULLTag);
    if (rv != Success) {
      return rv;
    }

    
    
    
    rv = der::Nested(subjectPublicKeyReader, der::SEQUENCE,
                     [&trustDomain, endEntityOrCA](Reader& r) {
      Input modulus;
      Input::size_type modulusSignificantBytes;
      Result rv = der::PositiveInteger(r, modulus, &modulusSignificantBytes);
      if (rv != Success) {
        return rv;
      }
      
      rv = trustDomain.CheckRSAPublicKeyModulusSizeInBits(
             endEntityOrCA, modulusSignificantBytes * 8u);
      if (rv != Success) {
        return rv;
      }

      
      
      Input exponent;
      return der::PositiveInteger(r, exponent);
    });
    if (rv != Success) {
      return rv;
    }
  } else {
    return Result::ERROR_UNSUPPORTED_KEYALG;
  }

  rv = der::End(algorithm);
  if (rv != Success) {
    return rv;
  }
  rv = der::End(subjectPublicKeyReader);
  if (rv != Success) {
    return rv;
  }

  return Success;
}





inline uint8_t KeyUsageToBitMask(KeyUsage keyUsage)
{
  assert(keyUsage != KeyUsage::noParticularKeyUsageRequired);
  return 0x80u >> static_cast<uint8_t>(keyUsage);
}

Result
CheckKeyUsage(EndEntityOrCA endEntityOrCA, const Input* encodedKeyUsage,
              KeyUsage requiredKeyUsageIfPresent)
{
  if (!encodedKeyUsage) {
    
    
    
    
    
    
    
    
    
    
    
    return Success;
  }

  Reader input(*encodedKeyUsage);
  Reader value;
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

  
  
  
  
  
  
  
  
  if (requiredKeyUsageIfPresent == KeyUsage::keyCertSign &&
      endEntityOrCA != EndEntityOrCA::MustBeCA) {
    return Result::ERROR_INADEQUATE_KEY_USAGE;
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








static const uint8_t anyPolicy[] = {
  0x55, 0x1d, 0x20, 0x00
};

 const CertPolicyId CertPolicyId::anyPolicy = {
  4, { 0x55, 0x1d, 0x20, 0x00 }
};

bool
CertPolicyId::IsAnyPolicy() const {
  if (this == &CertPolicyId::anyPolicy) {
    return true;
  }
  return numBytes == sizeof(::mozilla::pkix::anyPolicy) &&
         !memcmp(bytes, ::mozilla::pkix::anyPolicy,
                 sizeof(::mozilla::pkix::anyPolicy));
}


Result
CheckCertificatePolicies(EndEntityOrCA endEntityOrCA,
                         const Input* encodedCertificatePolicies,
                         const Input* encodedInhibitAnyPolicy,
                         TrustLevel trustLevel,
                         const CertPolicyId& requiredPolicy)
{
  if (requiredPolicy.numBytes == 0 ||
      requiredPolicy.numBytes > sizeof requiredPolicy.bytes) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  bool requiredPolicyFound = requiredPolicy.IsAnyPolicy();
  if (requiredPolicyFound) {
    return Success;
  }

  
  
  if (!requiredPolicyFound && encodedInhibitAnyPolicy) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }

  
  
  
  
  if (trustLevel == TrustLevel::TrustAnchor &&
      endEntityOrCA == EndEntityOrCA::MustBeCA) {
    requiredPolicyFound = true;
  }

  Input requiredPolicyDER;
  if (requiredPolicyDER.Init(requiredPolicy.bytes, requiredPolicy.numBytes)
        != Success) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  if (encodedCertificatePolicies) {
    Reader extension(*encodedCertificatePolicies);
    Reader certificatePolicies;
    Result rv = der::ExpectTagAndGetValue(extension, der::SEQUENCE,
                                          certificatePolicies);
    if (rv != Success) {
      return Result::ERROR_POLICY_VALIDATION_FAILED;
    }
    if (!extension.AtEnd()) {
      return Result::ERROR_POLICY_VALIDATION_FAILED;
    }

    do {
      
      
      
      
      Reader policyInformation;
      rv = der::ExpectTagAndGetValue(certificatePolicies, der::SEQUENCE,
                                     policyInformation);
      if (rv != Success) {
        return Result::ERROR_POLICY_VALIDATION_FAILED;
      }

      Reader policyIdentifier;
      rv = der::ExpectTagAndGetValue(policyInformation, der::OIDTag,
                                     policyIdentifier);
      if (rv != Success) {
        return rv;
      }

      if (policyIdentifier.MatchRest(requiredPolicyDER)) {
        requiredPolicyFound = true;
      } else if (endEntityOrCA == EndEntityOrCA::MustBeCA &&
                 policyIdentifier.MatchRest(anyPolicy)) {
        requiredPolicyFound = true;
      }

      
      
      
      
      
      
    } while (!requiredPolicyFound && !certificatePolicies.AtEnd());
  }

  if (!requiredPolicyFound) {
    return Result::ERROR_POLICY_VALIDATION_FAILED;
  }

  return Success;
}

static const long UNLIMITED_PATH_LEN = -1; 






Result
CheckBasicConstraints(EndEntityOrCA endEntityOrCA,
                      const Input* encodedBasicConstraints,
                      const der::Version version, TrustLevel trustLevel,
                      unsigned int subCACount)
{
  bool isCA = false;
  long pathLenConstraint = UNLIMITED_PATH_LEN;

  if (encodedBasicConstraints) {
    Reader input(*encodedBasicConstraints);
    Result rv = der::Nested(input, der::SEQUENCE,
                            [&isCA, &pathLenConstraint](Reader& r) {
      Result rv = der::OptionalBoolean(r, isCA);
      if (rv != Success) {
        return rv;
      }
      
      
      
      
      return der::OptionalInteger(r, UNLIMITED_PATH_LEN, pathLenConstraint);
    });
    if (rv != Success) {
      return Result::ERROR_EXTENSION_VALUE_INVALID;
    }
    if (der::End(input) != Success) {
      return Result::ERROR_EXTENSION_VALUE_INVALID;
    }
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (endEntityOrCA == EndEntityOrCA::MustBeCA && version == der::Version::v1) {
      if (trustLevel == TrustLevel::TrustAnchor) {
        isCA = true;
      } else {
        return Result::ERROR_V1_CERT_USED_AS_CA;
      }
    }
  }

  if (endEntityOrCA == EndEntityOrCA::MustBeEndEntity) {
    

    if (isCA) {
      
      
      
      return Result::ERROR_CA_CERT_USED_AS_END_ENTITY;
    }

    return Success;
  }

  assert(endEntityOrCA == EndEntityOrCA::MustBeCA);

  
  if (!isCA) {
    return Result::ERROR_CA_CERT_INVALID;
  }

  if (pathLenConstraint >= 0 &&
      static_cast<long>(subCACount) > pathLenConstraint) {
    return Result::ERROR_PATH_LEN_CONSTRAINT_INVALID;
  }

  return Success;
}



static Result
MatchEKU(Reader& value, KeyPurposeId requiredEKU,
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
        return NotReached("anyExtendedKeyUsage should start with found==true",
                          Result::FATAL_ERROR_LIBRARY_FAILURE);
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
                      const Input* encodedExtendedKeyUsage,
                      KeyPurposeId requiredEKU)
{
  
  
  
  

  bool foundOCSPSigning = false;

  if (encodedExtendedKeyUsage) {
    bool found = requiredEKU == KeyPurposeId::anyExtendedKeyUsage;

    Reader input(*encodedExtendedKeyUsage);
    Result rv = der::NestedOf(input, der::SEQUENCE, der::OIDTag,
                              der::EmptyAllowed::No, [&](Reader& r) {
      return MatchEKU(r, requiredEKU, endEntityOrCA, found, foundOCSPSigning);
    });
    if (rv != Success) {
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
                                 Time time,
                                 KeyUsage requiredKeyUsageIfPresent,
                                 KeyPurposeId requiredEKUIfPresent,
                                 const CertPolicyId& requiredPolicy,
                                 unsigned int subCACount,
                                  TrustLevel& trustLevel)
{
  Result rv;

  const EndEntityOrCA endEntityOrCA = cert.endEntityOrCA;

  
  
  
  rv = trustDomain.GetCertTrust(endEntityOrCA, requiredPolicy, cert.GetDER(),
                                trustLevel);
  if (rv != Success) {
    return rv;
  }

  if (trustLevel == TrustLevel::TrustAnchor &&
      endEntityOrCA == EndEntityOrCA::MustBeEndEntity &&
      requiredEKUIfPresent == KeyPurposeId::id_kp_OCSPSigning) {
    
    
    
    
    trustLevel = TrustLevel::InheritsTrust;
  }

  switch (trustLevel) {
    case TrustLevel::InheritsTrust:
      rv = CheckSignatureAlgorithm(cert.GetSignedData().algorithm,
                                   cert.GetSignature());
      if (rv != Success) {
        return rv;
      }
      break;

    case TrustLevel::TrustAnchor:
      
      
      
      
      break;

    case TrustLevel::ActivelyDistrusted:
      return Result::ERROR_UNTRUSTED_CERT;
  }

  
  
  
  Reader spki(cert.GetSubjectPublicKeyInfo());
  rv = der::Nested(spki, der::SEQUENCE, [&](Reader& r) {
    return CheckSubjectPublicKeyInfo(r, trustDomain, endEntityOrCA);
  });
  if (rv != Success) {
    return rv;
  }
  rv = der::End(spki);
  if (rv != Success) {
    return rv;
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
