























#include <limits>

#include "pkix/bind.h"
#include "pkix/pkix.h"
#include "pkixcheck.h"
#include "pkixder.h"

namespace mozilla { namespace pkix {

static const PRTime ONE_DAY
  = INT64_C(24) * INT64_C(60) * INT64_C(60) * PR_USEC_PER_SEC;
static const PRTime SLOP = ONE_DAY;


MOZILLA_PKIX_ENUM_CLASS CertStatus : uint8_t {
  Good = der::CONTEXT_SPECIFIC | 0,
  Revoked = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
  Unknown = der::CONTEXT_SPECIFIC | 2
};

class Context
{
public:
  Context(TrustDomain& trustDomain, const CertID& certID, PRTime time,
          uint16_t maxLifetimeInDays,  PRTime* thisUpdate,
           PRTime* validThrough)
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
      *thisUpdate = 0;
    }
    if (validThrough) {
      *validThrough = 0;
    }
  }

  TrustDomain& trustDomain;
  const CertID& certID;
  const PRTime time;
  const uint16_t maxLifetimeInDays;
  CertStatus certStatus;
  PRTime* thisUpdate;
  PRTime* validThrough;
  bool expired;

private:
  Context(const Context&); 
  void operator=(const Context&); 
};



static Result
CheckOCSPResponseSignerCert(TrustDomain& trustDomain,
                            BackCert& potentialSigner,
                            const SECItem& issuerSubject,
                            const SECItem& issuerSubjectPublicKeyInfo,
                            PRTime time)
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

  
  
  
  
  
  if (!SECITEM_ItemsAreEqual(&potentialSigner.GetIssuer(), &issuerSubject)) {
    return Result::ERROR_OCSP_RESPONDER_CERT_INVALID;
  }

  
  rv = trustDomain.VerifySignedData(potentialSigner.GetSignedData(),
                                    issuerSubjectPublicKeyInfo);

  
  
  

  return rv;
}

MOZILLA_PKIX_ENUM_CLASS ResponderIDType : uint8_t
{
  byName = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
  byKey = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 2
};

static inline Result OCSPResponse(Input&, Context&);
static inline Result ResponseBytes(Input&, Context&);
static inline Result BasicResponse(Input&, Context&);
static inline Result ResponseData(
                       Input& tbsResponseData,
                       Context& context,
                       const SignedDataWithSignature& signedResponseData,
                       const DERArray& certs);
static inline Result SingleResponse(Input& input, Context& context);
static Result ExtensionNotUnderstood(Input& extnID,
                                     const SECItem& extnValue,
                                      bool& understood);
static inline Result CertID(Input& input,
                            const Context& context,
                             bool& match);
static Result MatchKeyHash(TrustDomain& trustDomain,
                           const SECItem& issuerKeyHash,
                           const SECItem& issuerSubjectPublicKeyInfo,
                            bool& match);
static Result KeyHash(TrustDomain& trustDomain,
                      const SECItem& subjectPublicKeyInfo,
                       uint8_t* hashBuf, size_t hashBufSize);

static Result
MatchResponderID(TrustDomain& trustDomain,
                 ResponderIDType responderIDType,
                 const SECItem& responderIDItem,
                 const SECItem& potentialSignerSubject,
                 const SECItem& potentialSignerSubjectPublicKeyInfo,
                  bool& match)
{
  match = false;

  switch (responderIDType) {
    case ResponderIDType::byName:
      
      
      match = SECITEM_ItemsAreEqual(&responderIDItem, &potentialSignerSubject);
      return Success;

    case ResponderIDType::byKey:
    {
      Input responderID;
      Result rv = responderID.Init(responderIDItem.data, responderIDItem.len);
      if (rv != Success) {
        return rv;
      }
      SECItem keyHash;
      rv = der::ExpectTagAndGetValue(responderID, der::OCTET_STRING, keyHash);
      if (rv != Success) {
        return rv;
      }
      return MatchKeyHash(trustDomain, keyHash,
                          potentialSignerSubjectPublicKeyInfo, match);
    }

    default:
      return Result::ERROR_OCSP_MALFORMED_RESPONSE;
  }
}

static Result
VerifyOCSPSignedData(TrustDomain& trustDomain,
                     const SignedDataWithSignature& signedResponseData,
                     const SECItem& spki)
{
  Result rv = trustDomain.VerifySignedData(signedResponseData, spki);
  if (rv == Result::ERROR_BAD_SIGNATURE) {
    rv = Result::ERROR_OCSP_BAD_SIGNATURE;
  }
  return rv;
}







static Result
VerifySignature(Context& context, ResponderIDType responderIDType,
                const SECItem& responderID, const DERArray& certs,
                const SignedDataWithSignature& signedResponseData)
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
                          PRTime time, uint16_t maxOCSPLifetimeInDays,
                          const SECItem& encodedResponse,
                           bool& expired,
                           PRTime* thisUpdate,
                           PRTime* validThrough)
{
  
  expired = false;

  Input input;
  Result rv = input.Init(encodedResponse.data, encodedResponse.len);
  if (rv != Success) {
    return MapBadDERToMalformedOCSPResponse(rv);
  }

  Context context(trustDomain, certID, time, maxOCSPLifetimeInDays,
                  thisUpdate, validThrough);

  rv = der::Nested(input, der::SEQUENCE, bind(OCSPResponse, _1, ref(context)));
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
  }

  PR_NOT_REACHED("unknown CertStatus");
  return Result::ERROR_OCSP_UNKNOWN_CERT;
}





static inline Result
OCSPResponse(Input& input, Context& context)
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
                     der::SEQUENCE, bind(ResponseBytes, _1, ref(context)));
}




static inline Result
ResponseBytes(Input& input, Context& context)
{
  static const uint8_t id_pkix_ocsp_basic[] = {
    0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };

  Result rv = der::OID(input, id_pkix_ocsp_basic);
  if (rv != Success) {
    return rv;
  }

  return der::Nested(input, der::OCTET_STRING, der::SEQUENCE,
                     bind(BasicResponse, _1, ref(context)));
}






Result
BasicResponse(Input& input, Context& context)
{
  Input tbsResponseData;
  SignedDataWithSignature signedData;
  Result rv = der::SignedData(input, tbsResponseData, signedData);
  if (rv != Success) {
    if (rv == Result::ERROR_BAD_SIGNATURE) {
      return Result::ERROR_OCSP_BAD_SIGNATURE;
    }
    return rv;
  }

  

  NonOwningDERArray certs;
  if (!input.AtEnd()) {
    
    
    

    
    Input wrapped;
    rv = der::ExpectTagAndGetValue(
          input, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0, wrapped);
    if (rv != Success) {
      return rv;
    }
    rv = der::End(input);
    if (rv != Success) {
      return rv;
    }

    
    Input certsSequence;
    rv = der::ExpectTagAndGetValue(wrapped, der::SEQUENCE, certsSequence);
    if (rv != Success) {
      return rv;
    }
    rv = der::End(wrapped);
    if (rv != Success) {
      return rv;
    }

    
    while (!certsSequence.AtEnd()) {
      SECItem cert;
      rv = der::ExpectTagAndGetTLV(certsSequence, der::SEQUENCE, cert);
      if (rv != Success) {
        return rv;
      }
      rv = certs.Append(cert);
      if (rv != Success) {
        return rv;
      }
    }
  }

  return ResponseData(tbsResponseData, context, signedData, certs);
}







static inline Result
ResponseData(Input& input, Context& context,
             const SignedDataWithSignature& signedResponseData,
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

  
  
  
  SECItem responderID;
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

  
  PRTime producedAt;
  rv = der::GeneralizedTime(input, producedAt);
  if (rv != Success) {
    return rv;
  }

  
  
  
  rv = der::NestedOf(input, der::SEQUENCE, der::SEQUENCE,
                     der::EmptyAllowed::No,
                     bind(SingleResponse, _1, ref(context)));
  if (rv != Success) {
    return rv;
  }

  return der::OptionalExtensions(input,
                                 der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                                 ExtensionNotUnderstood);
}










static inline Result
SingleResponse(Input& input, Context& context)
{
  bool match = false;
  Result rv = der::Nested(input, der::SEQUENCE,
                          bind(CertID, _1, cref(context), ref(match)));
  if (rv != Success) {
    return rv;
  }

  if (!match) {
    
    
    
    
    input.SkipToEnd();
    return Success;
  }

  
  
  
  
  
  
  
  
  
  
  if (input.Peek(static_cast<uint8_t>(CertStatus::Good))) {
    rv = der::ExpectTagAndLength(input, static_cast<uint8_t>(CertStatus::Good),
                                 0);
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
    rv = der::ExpectTagAndLength(input,
                                 static_cast<uint8_t>(CertStatus::Unknown), 0);
    if (rv != Success) {
      return rv;
    }
  }

  
  
  
  
  
  

  const PRTime maxLifetime =
    context.maxLifetimeInDays * ONE_DAY;

  PRTime thisUpdate;
  rv = der::GeneralizedTime(input, thisUpdate);
  if (rv != Success) {
    return rv;
  }

  if (thisUpdate > context.time + SLOP) {
    return Result::ERROR_OCSP_FUTURE_RESPONSE;
  }

  PRTime notAfter;
  static const uint8_t NEXT_UPDATE_TAG =
    der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0;
  if (input.Peek(NEXT_UPDATE_TAG)) {
    PRTime nextUpdate;
    rv = der::Nested(input, NEXT_UPDATE_TAG,
                    bind(der::GeneralizedTime, _1, ref(nextUpdate)));
    if (rv != Success) {
      return rv;
    }

    if (nextUpdate < thisUpdate) {
      return Result::ERROR_OCSP_MALFORMED_RESPONSE;
    }
    if (nextUpdate - thisUpdate <= maxLifetime) {
      notAfter = nextUpdate;
    } else {
      notAfter = thisUpdate + maxLifetime;
    }
  } else {
    
    
    notAfter = thisUpdate + ONE_DAY;
  }

  if (context.time < SLOP) { 
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  if (context.time - SLOP > notAfter) {
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
    *context.validThrough = notAfter;
  }

  return Success;
}






static inline Result
CertID(Input& input, const Context& context,  bool& match)
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

  SECItem issuerNameHash;
  rv = der::ExpectTagAndGetValue(input, der::OCTET_STRING, issuerNameHash);
  if (rv != Success) {
    return rv;
  }

  SECItem issuerKeyHash;
  rv = der::ExpectTagAndGetValue(input, der::OCTET_STRING, issuerKeyHash);
  if (rv != Success) {
    return rv;
  }

  SECItem serialNumber;
  rv = der::CertificateSerialNumber(input, serialNumber);
  if (rv != Success) {
    return rv;
  }

  if (!SECITEM_ItemsAreEqual(&serialNumber, &context.certID.serialNumber)) {
    
    
    
    input.SkipToEnd();
    return Success;
  }

  

  if (hashAlgorithm != DigestAlgorithm::sha1) {
    
    input.SkipToEnd();
    return Success;
  }

  if (issuerNameHash.len != TrustDomain::DIGEST_LENGTH) {
    return Result::ERROR_OCSP_MALFORMED_RESPONSE;
  }

  
  
  
  uint8_t hashBuf[TrustDomain::DIGEST_LENGTH];
  rv = context.trustDomain.DigestBuf(context.certID.issuer, hashBuf,
                                     sizeof(hashBuf));
  if (rv != Success) {
    return rv;
  }
  if (memcmp(hashBuf, issuerNameHash.data, issuerNameHash.len)) {
    
    input.SkipToEnd();
    return Success;
  }

  return MatchKeyHash(context.trustDomain, issuerKeyHash,
                      context.certID.issuerSubjectPublicKeyInfo, match);
}











static Result
MatchKeyHash(TrustDomain& trustDomain, const SECItem& keyHash,
             const SECItem& subjectPublicKeyInfo,  bool& match)
{
  if (keyHash.len != TrustDomain::DIGEST_LENGTH)  {
    return Result::ERROR_OCSP_MALFORMED_RESPONSE;
  }
  static uint8_t hashBuf[TrustDomain::DIGEST_LENGTH];
  Result rv = KeyHash(trustDomain, subjectPublicKeyInfo, hashBuf,
                      sizeof hashBuf);
  if (rv != Success) {
    return rv;
  }
  match = !memcmp(hashBuf, keyHash.data, keyHash.len);
  return Success;
}


Result
KeyHash(TrustDomain& trustDomain, const SECItem& subjectPublicKeyInfo,
         uint8_t* hashBuf, size_t hashBufSize)
{
  if (!hashBuf || hashBufSize != TrustDomain::DIGEST_LENGTH) {
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  
  
  
  
  

  Input spki;
  Result rv;

  {
    
    
    Input input;
    rv = input.Init(subjectPublicKeyInfo.data, subjectPublicKeyInfo.len);
    if (rv != Success) {
      return rv;
    }
    rv = der::ExpectTagAndGetValue(input, der::SEQUENCE, spki);
    if (rv != Success) {
      return rv;
    }
    rv = der::End(input);
    if (rv != Success) {
      return rv;
    }
  }

  
  rv = der::ExpectTagAndSkipValue(spki, der::SEQUENCE);
  if (rv != Success) {
    return rv;
  }

  SECItem subjectPublicKey;
  rv = der::ExpectTagAndGetValue(spki, der::BIT_STRING, subjectPublicKey);
  if (rv != Success) {
    return rv;
  }

  rv = der::End(spki);
  if (rv != Success) {
    return rv;
  }

  
  if (subjectPublicKey.len == 0 || subjectPublicKey.data[0] != 0) {
    return Result::ERROR_BAD_DER;
  }
  ++subjectPublicKey.data;
  --subjectPublicKey.len;

  return trustDomain.DigestBuf(subjectPublicKey, hashBuf, hashBufSize);
}

Result
ExtensionNotUnderstood(Input& , const SECItem& ,
                        bool& understood)
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
  static const uint8_t hashLen = TrustDomain::DIGEST_LENGTH;

  static const unsigned int totalLenWithoutSerialNumberData
    = 2                             
    + 2                             
    + 2                             
    + 2                             
    + 2                             
    + PR_ARRAY_SIZE(hashAlgorithm)  
    + 2 + hashLen                   
    + 2 + hashLen                   
    + 2;                            

  
  
  
  
  
  
  static_assert(totalLenWithoutSerialNumberData < OCSP_REQUEST_MAX_LENGTH,
                "totalLenWithoutSerialNumberData too big");
  if (certID.serialNumber.len >
        OCSP_REQUEST_MAX_LENGTH - totalLenWithoutSerialNumberData) {
    return Result::ERROR_BAD_DER;
  }

  outLen = totalLenWithoutSerialNumberData + certID.serialNumber.len;

  uint8_t totalLen = static_cast<uint8_t>(outLen);

  uint8_t* d = out;
  *d++ = 0x30; *d++ = totalLen - 2u;  
  *d++ = 0x30; *d++ = totalLen - 4u;  
  *d++ = 0x30; *d++ = totalLen - 6u;  
  *d++ = 0x30; *d++ = totalLen - 8u;  
  *d++ = 0x30; *d++ = totalLen - 10u; 

  
  for (size_t i = 0; i < PR_ARRAY_SIZE(hashAlgorithm); ++i) {
    *d++ = hashAlgorithm[i];
  }

  
  *d++ = 0x04;
  *d++ = hashLen;
  Result rv = trustDomain.DigestBuf(certID.issuer, d, hashLen);
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
  *d++ = static_cast<uint8_t>(certID.serialNumber.len);
  for (size_t i = 0; i < certID.serialNumber.len; ++i) {
    *d++ = certID.serialNumber.data[i];
  }

  PR_ASSERT(d == out + totalLen);

  return Success;
}

} } 
