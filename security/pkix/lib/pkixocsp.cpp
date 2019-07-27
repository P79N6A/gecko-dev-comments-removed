























#include <limits>

#include "pkix/pkix.h"
#include "pkixcheck.h"
#include "pkixutil.h"

namespace {

const size_t SHA1_DIGEST_LENGTH = 160 / 8;

} 

namespace mozilla { namespace pkix {


enum class CertStatus : uint8_t {
  Good = der::CONTEXT_SPECIFIC | 0,
  Revoked = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
  Unknown = der::CONTEXT_SPECIFIC | 2
};

class Context final
{
public:
  Context(TrustDomain& trustDomain, const CertID& certID, Time time,
          uint16_t maxLifetimeInDays,  Time* thisUpdate,
           Time* validThrough)
    : trustDomain(trustDomain)
    , certID(certID)
    , time(time)
    , maxLifetimeInDays(maxLifetimeInDays)
    , certStatus(CertStatus::Unknown)
    , thisUpdate(thisUpdate)
    , validThrough(validThrough)
    , expired(false)
  {
    if (thisUpdate) {
      *thisUpdate = TimeFromElapsedSecondsAD(0);
    }
    if (validThrough) {
      *validThrough = TimeFromElapsedSecondsAD(0);
    }
  }

  TrustDomain& trustDomain;
  const CertID& certID;
  const Time time;
  const uint16_t maxLifetimeInDays;
  CertStatus certStatus;
  Time* thisUpdate;
  Time* validThrough;
  bool expired;

  Context(const Context&) = delete;
  void operator=(const Context&) = delete;
};



static Result
CheckOCSPResponseSignerCert(TrustDomain& trustDomain,
                            BackCert& potentialSigner,
                            Input issuerSubject,
                            Input issuerSubjectPublicKeyInfo,
                            Time time)
{
  Result rv;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  TrustLevel unusedTrustLevel;
  rv = CheckIssuerIndependentProperties(trustDomain, potentialSigner, time,
                                        KeyUsage::noParticularKeyUsageRequired,
                                        KeyPurposeId::id_kp_OCSPSigning,
                                        CertPolicyId::anyPolicy, 0,
                                        unusedTrustLevel);
  if (rv != Success) {
    return rv;
  }

  
  
  
  
  
  if (!InputsAreEqual(potentialSigner.GetIssuer(), issuerSubject)) {
    return Result::ERROR_OCSP_RESPONDER_CERT_INVALID;
  }

  

  rv = VerifySignedData(trustDomain, potentialSigner.GetSignedData(),
                        issuerSubjectPublicKeyInfo);

  
  
  

  return rv;
}

enum class ResponderIDType : uint8_t
{
  byName = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
  byKey = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 2
};

static inline Result OCSPResponse(Reader&, Context&);
static inline Result ResponseBytes(Reader&, Context&);
static inline Result BasicResponse(Reader&, Context&);
static inline Result ResponseData(
                       Reader& tbsResponseData,
                       Context& context,
                       const der::SignedDataWithSignature& signedResponseData,
                       const DERArray& certs);
static inline Result SingleResponse(Reader& input, Context& context);
static Result ExtensionNotUnderstood(Reader& extnID, Input extnValue,
                                     bool critical,  bool& understood);
static inline Result CertID(Reader& input,
                            const Context& context,
                             bool& match);
static Result MatchKeyHash(TrustDomain& trustDomain,
                           Input issuerKeyHash,
                           Input issuerSubjectPublicKeyInfo,
                            bool& match);
static Result KeyHash(TrustDomain& trustDomain,
                      Input subjectPublicKeyInfo,
                       uint8_t* hashBuf, size_t hashBufSize);

static Result
MatchResponderID(TrustDomain& trustDomain,
                 ResponderIDType responderIDType,
                 Input responderID,
                 Input potentialSignerSubject,
                 Input potentialSignerSubjectPublicKeyInfo,
                  bool& match)
{
  match = false;

  switch (responderIDType) {
    case ResponderIDType::byName:
      
      
      match = InputsAreEqual(responderID, potentialSignerSubject);
      return Success;

    case ResponderIDType::byKey:
    {
      Reader input(responderID);
      Input keyHash;
      Result rv = der::ExpectTagAndGetValue(input, der::OCTET_STRING, keyHash);
      if (rv != Success) {
        return rv;
      }
      return MatchKeyHash(trustDomain, keyHash,
                          potentialSignerSubjectPublicKeyInfo, match);
    }

    MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
  }
}

static Result
VerifyOCSPSignedData(TrustDomain& trustDomain,
                     const der::SignedDataWithSignature& signedResponseData,
                     Input spki)
{
  Result rv = VerifySignedData(trustDomain, signedResponseData, spki);
  if (rv == Result::ERROR_BAD_SIGNATURE) {
    rv = Result::ERROR_OCSP_BAD_SIGNATURE;
  }
  return rv;
}







static Result
VerifySignature(Context& context, ResponderIDType responderIDType,
                Input responderID, const DERArray& certs,
                const der::SignedDataWithSignature& signedResponseData)
{
  bool match;
  Result rv = MatchResponderID(context.trustDomain, responderIDType,
                               responderID, context.certID.issuer,
                               context.certID.issuerSubjectPublicKeyInfo,
                               match);
  if (rv != Success) {
    return rv;
  }
  if (match) {
    return VerifyOCSPSignedData(context.trustDomain, signedResponseData,
                                context.certID.issuerSubjectPublicKeyInfo);
  }

  size_t numCerts = certs.GetLength();
  for (size_t i = 0; i < numCerts; ++i) {
    BackCert cert(*certs.GetDER(i), EndEntityOrCA::MustBeEndEntity, nullptr);
    rv = cert.Init();
    if (rv != Success) {
      return rv;
    }
    rv = MatchResponderID(context.trustDomain, responderIDType, responderID,
                          cert.GetSubject(), cert.GetSubjectPublicKeyInfo(),
                          match);
    if (rv != Success) {
      if (IsFatalError(rv)) {
        return rv;
      }
      continue;
    }

    if (match) {
      rv = CheckOCSPResponseSignerCert(context.trustDomain, cert,
                                       context.certID.issuer,
                                       context.certID.issuerSubjectPublicKeyInfo,
                                       context.time);
      if (rv != Success) {
        if (IsFatalError(rv)) {
          return rv;
        }
        continue;
      }

      return VerifyOCSPSignedData(context.trustDomain, signedResponseData,
                                  cert.GetSubjectPublicKeyInfo());
    }
  }

  return Result::ERROR_OCSP_INVALID_SIGNING_CERT;
}

static inline Result
MapBadDERToMalformedOCSPResponse(Result rv)
{
  if (rv == Result::ERROR_BAD_DER) {
    return Result::ERROR_OCSP_MALFORMED_RESPONSE;
  }
  return rv;
}

Result
VerifyEncodedOCSPResponse(TrustDomain& trustDomain, const struct CertID& certID,
                          Time time, uint16_t maxOCSPLifetimeInDays,
                          Input encodedResponse,
                           bool& expired,
                           Time* thisUpdate,
                           Time* validThrough)
{
  
  expired = false;

  Context context(trustDomain, certID, time, maxOCSPLifetimeInDays,
                  thisUpdate, validThrough);

  Reader input(encodedResponse);
  Result rv = der::Nested(input, der::SEQUENCE, [&context](Reader& r) {
    return OCSPResponse(r, context);
  });
  if (rv != Success) {
    return MapBadDERToMalformedOCSPResponse(rv);
  }
  rv = der::End(input);
  if (rv != Success) {
    return MapBadDERToMalformedOCSPResponse(rv);
  }

  expired = context.expired;

  switch (context.certStatus) {
    case CertStatus::Good:
      if (expired) {
        return Result::ERROR_OCSP_OLD_RESPONSE;
      }
      return Success;
    case CertStatus::Revoked:
      return Result::ERROR_REVOKED_CERTIFICATE;
    case CertStatus::Unknown:
      return Result::ERROR_OCSP_UNKNOWN_CERT;
     MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
  }
}





static inline Result
OCSPResponse(Reader& input, Context& context)
{
  
  
  
  
  
  
  
  
  
  uint8_t responseStatus;

  Result rv = der::Enumerated(input, responseStatus);
  if (rv != Success) {
    return rv;
  }
  switch (responseStatus) {
    case 0: break; 
    case 1: return Result::ERROR_OCSP_MALFORMED_REQUEST;
    case 2: return Result::ERROR_OCSP_SERVER_ERROR;
    case 3: return Result::ERROR_OCSP_TRY_SERVER_LATER;
    case 5: return Result::ERROR_OCSP_REQUEST_NEEDS_SIG;
    case 6: return Result::ERROR_OCSP_UNAUTHORIZED_REQUEST;
    default: return Result::ERROR_OCSP_UNKNOWN_RESPONSE_STATUS;
  }

  return der::Nested(input, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                     der::SEQUENCE, [&context](Reader& r) {
    return ResponseBytes(r, context);
  });
}




static inline Result
ResponseBytes(Reader& input, Context& context)
{
  static const uint8_t id_pkix_ocsp_basic[] = {
    0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };

  Result rv = der::OID(input, id_pkix_ocsp_basic);
  if (rv != Success) {
    return rv;
  }

  return der::Nested(input, der::OCTET_STRING, der::SEQUENCE,
                     [&context](Reader& r) {
    return BasicResponse(r, context);
  });
}






Result
BasicResponse(Reader& input, Context& context)
{
  Reader tbsResponseData;
  der::SignedDataWithSignature signedData;
  Result rv = der::SignedData(input, tbsResponseData, signedData);
  if (rv != Success) {
    if (rv == Result::ERROR_BAD_SIGNATURE) {
      return Result::ERROR_OCSP_BAD_SIGNATURE;
    }
    return rv;
  }

  
  NonOwningDERArray certs;
  if (!input.AtEnd()) {
    rv = der::Nested(input, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                     der::SEQUENCE, [&certs](Reader& certsDER) -> Result {
      while (!certsDER.AtEnd()) {
        Input cert;
        Result rv = der::ExpectTagAndGetTLV(certsDER, der::SEQUENCE, cert);
        if (rv != Success) {
          return rv;
        }
        rv = certs.Append(cert);
        if (rv != Success) {
          return Result::ERROR_BAD_DER; 
        }
      }
      return Success;
    });
    if (rv != Success) {
      return rv;
    }
  }

  return ResponseData(tbsResponseData, context, signedData, certs);
}







static inline Result
ResponseData(Reader& input, Context& context,
             const der::SignedDataWithSignature& signedResponseData,
             const DERArray& certs)
{
  der::Version version;
  Result rv = der::OptionalVersion(input, version);
  if (rv != Success) {
    return rv;
  }
  if (version != der::Version::v1) {
    
    return Result::ERROR_BAD_DER;
  }

  
  
  
  Input responderID;
  ResponderIDType responderIDType
    = input.Peek(static_cast<uint8_t>(ResponderIDType::byName))
    ? ResponderIDType::byName
    : ResponderIDType::byKey;
  rv = der::ExpectTagAndGetValue(input, static_cast<uint8_t>(responderIDType),
                                 responderID);
  if (rv != Success) {
    return rv;
  }

  
  
  
  rv = VerifySignature(context, responderIDType, responderID, certs,
                       signedResponseData);
  if (rv != Success) {
    return rv;
  }

  
  Time producedAt(Time::uninitialized);
  rv = der::GeneralizedTime(input, producedAt);
  if (rv != Success) {
    return rv;
  }

  
  
  
  rv = der::NestedOf(input, der::SEQUENCE, der::SEQUENCE,
                     der::EmptyAllowed::No, [&context](Reader& r) {
    return SingleResponse(r, context);
  });
  if (rv != Success) {
    return rv;
  }

  return der::OptionalExtensions(input,
                                 der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                                 ExtensionNotUnderstood);
}










static inline Result
SingleResponse(Reader& input, Context& context)
{
  bool match = false;
  Result rv = der::Nested(input, der::SEQUENCE, [&context, &match](Reader& r) {
    return CertID(r, context, match);
  });
  if (rv != Success) {
    return rv;
  }

  if (!match) {
    
    
    
    
    input.SkipToEnd();
    return Success;
  }

  
  
  
  
  
  
  
  
  
  
  if (input.Peek(static_cast<uint8_t>(CertStatus::Good))) {
    rv = der::ExpectTagAndEmptyValue(input,
                                     static_cast<uint8_t>(CertStatus::Good));
    if (rv != Success) {
      return rv;
    }
    if (context.certStatus != CertStatus::Revoked) {
      context.certStatus = CertStatus::Good;
    }
  } else if (input.Peek(static_cast<uint8_t>(CertStatus::Revoked))) {
    
    
    
    
    rv = der::ExpectTagAndSkipValue(input,
                                    static_cast<uint8_t>(CertStatus::Revoked));
    if (rv != Success) {
      return rv;
    }
    context.certStatus = CertStatus::Revoked;
  } else {
    rv = der::ExpectTagAndEmptyValue(input,
                                     static_cast<uint8_t>(CertStatus::Unknown));
    if (rv != Success) {
      return rv;
    }
  }

  
  
  
  
  
  

  Time thisUpdate(Time::uninitialized);
  rv = der::GeneralizedTime(input, thisUpdate);
  if (rv != Success) {
    return rv;
  }

  static const uint64_t SLOP_SECONDS = Time::ONE_DAY_IN_SECONDS;

  Time timePlusSlop(context.time);
  rv = timePlusSlop.AddSeconds(SLOP_SECONDS);
  if (rv != Success) {
    return rv;
  }
  if (thisUpdate > timePlusSlop) {
    return Result::ERROR_OCSP_FUTURE_RESPONSE;
  }

  Time notAfter(Time::uninitialized);
  static const uint8_t NEXT_UPDATE_TAG =
    der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0;
  if (input.Peek(NEXT_UPDATE_TAG)) {
    Time nextUpdate(Time::uninitialized);
    rv = der::Nested(input, NEXT_UPDATE_TAG, [&nextUpdate](Reader& r) {
      return der::GeneralizedTime(r, nextUpdate);
    });
    if (rv != Success) {
      return rv;
    }

    if (nextUpdate < thisUpdate) {
      return Result::ERROR_OCSP_MALFORMED_RESPONSE;
    }
    notAfter = thisUpdate;
    if (notAfter.AddSeconds(context.maxLifetimeInDays *
                            Time::ONE_DAY_IN_SECONDS) != Success) {
      
      
      return Result::ERROR_OCSP_FUTURE_RESPONSE;
    }
    if (nextUpdate <= notAfter) {
      notAfter = nextUpdate;
    }
  } else {
    
    
    notAfter = thisUpdate;
    if (notAfter.AddSeconds(Time::ONE_DAY_IN_SECONDS) != Success) {
      
      
      return Result::ERROR_OCSP_FUTURE_RESPONSE;
    }
  }

  
  Time notAfterPlusSlop(notAfter);
  rv = notAfterPlusSlop.AddSeconds(SLOP_SECONDS);
  if (rv != Success) {
    
    
    return Result::ERROR_OCSP_FUTURE_RESPONSE;
  }
  if (context.time > notAfterPlusSlop) {
    context.expired = true;
  }

  rv = der::OptionalExtensions(input,
                               der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                               ExtensionNotUnderstood);
  if (rv != Success) {
    return rv;
  }

  if (context.thisUpdate) {
    *context.thisUpdate = thisUpdate;
  }
  if (context.validThrough) {
    *context.validThrough = notAfterPlusSlop;
  }

  return Success;
}






static inline Result
CertID(Reader& input, const Context& context,  bool& match)
{
  match = false;

  DigestAlgorithm hashAlgorithm;
  Result rv = der::DigestAlgorithmIdentifier(input, hashAlgorithm);
  if (rv != Success) {
    if (rv == Result::ERROR_INVALID_ALGORITHM) {
      
      input.SkipToEnd();
      return Success;
    }
    return rv;
  }

  Input issuerNameHash;
  rv = der::ExpectTagAndGetValue(input, der::OCTET_STRING, issuerNameHash);
  if (rv != Success) {
    return rv;
  }

  Input issuerKeyHash;
  rv = der::ExpectTagAndGetValue(input, der::OCTET_STRING, issuerKeyHash);
  if (rv != Success) {
    return rv;
  }

  Input serialNumber;
  rv = der::CertificateSerialNumber(input, serialNumber);
  if (rv != Success) {
    return rv;
  }

  if (!InputsAreEqual(serialNumber, context.certID.serialNumber)) {
    
    
    
    input.SkipToEnd();
    return Success;
  }

  

  if (hashAlgorithm != DigestAlgorithm::sha1) {
    
    input.SkipToEnd();
    return Success;
  }

  if (issuerNameHash.GetLength() != SHA1_DIGEST_LENGTH) {
    return Result::ERROR_OCSP_MALFORMED_RESPONSE;
  }

  
  
  
  uint8_t hashBuf[SHA1_DIGEST_LENGTH];
  rv = context.trustDomain.DigestBuf(context.certID.issuer,
                                     DigestAlgorithm::sha1, hashBuf,
                                     sizeof(hashBuf));
  if (rv != Success) {
    return rv;
  }
  Input computed(hashBuf);
  if (!InputsAreEqual(computed, issuerNameHash)) {
    
    input.SkipToEnd();
    return Success;
  }

  return MatchKeyHash(context.trustDomain, issuerKeyHash,
                      context.certID.issuerSubjectPublicKeyInfo, match);
}











static Result
MatchKeyHash(TrustDomain& trustDomain, Input keyHash,
             const Input subjectPublicKeyInfo,  bool& match)
{
  if (keyHash.GetLength() != SHA1_DIGEST_LENGTH)  {
    return Result::ERROR_OCSP_MALFORMED_RESPONSE;
  }
  static uint8_t hashBuf[SHA1_DIGEST_LENGTH];
  Result rv = KeyHash(trustDomain, subjectPublicKeyInfo, hashBuf,
                      sizeof hashBuf);
  if (rv != Success) {
    return rv;
  }
  Input computed(hashBuf);
  match = InputsAreEqual(computed, keyHash);
  return Success;
}


Result
KeyHash(TrustDomain& trustDomain, const Input subjectPublicKeyInfo,
         uint8_t* hashBuf, size_t hashBufSize)
{
  if (!hashBuf || hashBufSize != SHA1_DIGEST_LENGTH) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  
  
  
  
  

  Reader spki;
  Result rv = der::ExpectTagAndGetValueAtEnd(subjectPublicKeyInfo,
                                             der::SEQUENCE, spki);
  if (rv != Success) {
    return rv;
  }

  
  rv = der::ExpectTagAndSkipValue(spki, der::SEQUENCE);
  if (rv != Success) {
    return rv;
  }

  Input subjectPublicKey;
  rv = der::BitStringWithNoUnusedBits(spki, subjectPublicKey);
  if (rv != Success) {
    return rv;
  }
  rv = der::End(spki);
  if (rv != Success) {
    return rv;
  }

  return trustDomain.DigestBuf(subjectPublicKey, DigestAlgorithm::sha1,
                               hashBuf, hashBufSize);
}

Result
ExtensionNotUnderstood(Reader& , Input ,
                       bool ,  bool& understood)
{
  understood = false;
  return Success;
}
























Result
CreateEncodedOCSPRequest(TrustDomain& trustDomain, const struct CertID& certID,
                          uint8_t (&out)[OCSP_REQUEST_MAX_LENGTH],
                          size_t& outLen)
{
  

  
  
  
  
  
  
  
  

  

  
  
  
  static const uint8_t hashAlgorithm[11] = {
    0x30, 0x09,                               
    0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 
    0x05, 0x00,                               
  };
  static const uint8_t hashLen = 160 / 8;

  static const unsigned int totalLenWithoutSerialNumberData
    = 2                             
    + 2                             
    + 2                             
    + 2                             
    + 2                             
    + sizeof(hashAlgorithm)         
    + 2 + hashLen                   
    + 2 + hashLen                   
    + 2;                            

  
  
  
  
  
  
  static_assert(totalLenWithoutSerialNumberData < OCSP_REQUEST_MAX_LENGTH,
                "totalLenWithoutSerialNumberData too big");
  if (certID.serialNumber.GetLength() >
        OCSP_REQUEST_MAX_LENGTH - totalLenWithoutSerialNumberData) {
    return Result::ERROR_BAD_DER;
  }

  outLen = totalLenWithoutSerialNumberData + certID.serialNumber.GetLength();

  uint8_t totalLen = static_cast<uint8_t>(outLen);

  uint8_t* d = out;
  *d++ = 0x30; *d++ = totalLen - 2u;  
  *d++ = 0x30; *d++ = totalLen - 4u;  
  *d++ = 0x30; *d++ = totalLen - 6u;  
  *d++ = 0x30; *d++ = totalLen - 8u;  
  *d++ = 0x30; *d++ = totalLen - 10u; 

  
  for (size_t i = 0; i < sizeof(hashAlgorithm); ++i) {
    *d++ = hashAlgorithm[i];
  }

  
  *d++ = 0x04;
  *d++ = hashLen;
  Result rv = trustDomain.DigestBuf(certID.issuer, DigestAlgorithm::sha1, d,
                                    hashLen);
  if (rv != Success) {
    return rv;
  }
  d += hashLen;

  
  *d++ = 0x04;
  *d++ = hashLen;
  rv = KeyHash(trustDomain, certID.issuerSubjectPublicKeyInfo, d, hashLen);
  if (rv != Success) {
    return rv;
  }
  d += hashLen;

  
  *d++ = 0x02; 
  *d++ = static_cast<uint8_t>(certID.serialNumber.GetLength());
  Reader serialNumber(certID.serialNumber);
  do {
    rv = serialNumber.Read(*d);
    if (rv != Success) {
      return rv;
    }
    ++d;
  } while (!serialNumber.AtEnd());

  assert(d == out + totalLen);

  return Success;
}

} } 
