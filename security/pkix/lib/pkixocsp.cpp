























#include <limits>

#include "hasht.h"
#include "pk11pub.h"
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

static der::Result
HashBuf(const SECItem& item,  uint8_t *hashBuf, size_t hashBufLen)
{
  if (hashBufLen != SHA1_LENGTH) {
    PR_NOT_REACHED("invalid hash length");
    return der::Fail(SEC_ERROR_INVALID_ARGS);
  }
  if (item.len >
      static_cast<decltype(item.len)>(std::numeric_limits<int32_t>::max())) {
    PR_NOT_REACHED("large OCSP responses should have already been rejected");
    return der::Fail(SEC_ERROR_INVALID_ARGS);
  }
  if (PK11_HashBuf(SEC_OID_SHA1, hashBuf, item.data,
                   static_cast<int32_t>(item.len)) != SECSuccess) {
    return der::Fail(PR_GetError());
  }
  return der::Success;
}



static Result
CheckOCSPResponseSignerCert(TrustDomain& trustDomain,
                            BackCert& potentialSigner,
                            const SECItem& issuerSubject,
                            const SECItem& issuerSubjectPublicKeyInfo,
                            PRTime time)
{
  Result rv;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  rv = CheckIssuerIndependentProperties(trustDomain, potentialSigner, time,
                                        EndEntityOrCA::MustBeEndEntity,
                                        KeyUsage::noParticularKeyUsageRequired,
                                        KeyPurposeId::id_kp_OCSPSigning,
                                        CertPolicyId::anyPolicy, 0);
  if (rv != Success) {
    return rv;
  }

  
  
  
  
  
  if (!SECITEM_ItemsAreEqual(&potentialSigner.GetIssuer(), &issuerSubject)) {
    return Fail(RecoverableError, SEC_ERROR_OCSP_RESPONDER_CERT_INVALID);
  }

  
  SECStatus srv = trustDomain.VerifySignedData(potentialSigner.GetSignedData(),
                                               issuerSubjectPublicKeyInfo);
  if (srv != SECSuccess) {
    return MapSECStatus(srv);
  }

  
  
  

  return Success;
}

MOZILLA_PKIX_ENUM_CLASS ResponderIDType : uint8_t
{
  byName = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
  byKey = der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 2
};

static inline der::Result OCSPResponse(der::Input&, Context&);
static inline der::Result ResponseBytes(der::Input&, Context&);
static inline der::Result BasicResponse(der::Input&, Context&);
static inline der::Result ResponseData(
                              der::Input& tbsResponseData, Context& context,
                              const CERTSignedData& signedResponseData,
                               SECItem* certs, size_t numCerts);
static inline der::Result SingleResponse(der::Input& input,
                                          Context& context);
static der::Result ExtensionNotUnderstood(der::Input& extnID,
                                          const SECItem& extnValue,
                                           bool& understood);
static inline der::Result CertID(der::Input& input,
                                  const Context& context,
                                   bool& match);
static Result MatchKeyHash(const SECItem& issuerKeyHash,
                           const SECItem& issuerSubjectPublicKeyInfo,
                            bool& match);
static Result KeyHash(const SECItem& subjectPublicKeyInfo,
                       uint8_t* hashBuf, size_t hashBufSize);


static Result
MatchResponderID(ResponderIDType responderIDType,
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
      der::Input responderID;
      if (responderID.Init(responderIDItem.data, responderIDItem.len)
            != der::Success) {
        return RecoverableError;
      }
      SECItem keyHash;
      if (der::ExpectTagAndGetValue(responderID, der::OCTET_STRING, keyHash)
            != der::Success) {
        return RecoverableError;
      }
      return MatchKeyHash(keyHash, potentialSignerSubjectPublicKeyInfo, match);
    }

    default:
      return Fail(RecoverableError, SEC_ERROR_OCSP_MALFORMED_RESPONSE);
  }
}

static Result
VerifyOCSPSignedData(TrustDomain& trustDomain,
                     const CERTSignedData& signedResponseData,
                     const SECItem& spki)
{
  SECStatus srv = trustDomain.VerifySignedData(signedResponseData, spki);
  if (srv != SECSuccess) {
    if (PR_GetError() == SEC_ERROR_BAD_SIGNATURE) {
      PR_SetError(SEC_ERROR_OCSP_BAD_SIGNATURE, 0);
    }
  }
  return MapSECStatus(srv);
}







static Result
VerifySignature(Context& context, ResponderIDType responderIDType,
                const SECItem& responderID, const SECItem* certs,
                size_t numCerts, const CERTSignedData& signedResponseData)
{
  bool match;
  Result rv = MatchResponderID(responderIDType, responderID,
                               context.certID.issuer,
                               context.certID.issuerSubjectPublicKeyInfo,
                               match);
  if (rv != Success) {
    return rv;
  }
  if (match) {
    return VerifyOCSPSignedData(context.trustDomain, signedResponseData,
                                context.certID.issuerSubjectPublicKeyInfo);
  }

  for (size_t i = 0; i < numCerts; ++i) {
    BackCert cert(certs[i], nullptr, BackCert::IncludeCN::No);
    rv = cert.Init();
    if (rv != Success) {
      return rv;
    }
    rv = MatchResponderID(responderIDType, responderID,
                          cert.GetSubject(), cert.GetSubjectPublicKeyInfo(),
                          match);
    if (rv == FatalError) {
      return rv;
    }
    if (rv == RecoverableError) {
      continue;
    }

    if (match) {
      rv = CheckOCSPResponseSignerCert(context.trustDomain, cert,
                                       context.certID.issuer,
                                       context.certID.issuerSubjectPublicKeyInfo,
                                       context.time);
      if (rv == FatalError) {
        return rv;
      }
      if (rv == RecoverableError) {
        continue;
      }

      return VerifyOCSPSignedData(context.trustDomain, signedResponseData,
                                  cert.GetSubjectPublicKeyInfo());
    }
  }

  return Fail(RecoverableError, SEC_ERROR_OCSP_INVALID_SIGNING_CERT);
}

static inline void
SetErrorToMalformedResponseOnBadDERError()
{
  if (PR_GetError() == SEC_ERROR_BAD_DER) {
    PR_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE, 0);
  }
}

SECStatus
VerifyEncodedOCSPResponse(TrustDomain& trustDomain, const struct CertID& certID,
                          PRTime time, uint16_t maxOCSPLifetimeInDays,
                          const SECItem& encodedResponse,
                           bool& expired,
                           PRTime* thisUpdate,
                           PRTime* validThrough)
{
  
  expired = false;

  der::Input input;
  if (input.Init(encodedResponse.data, encodedResponse.len) != der::Success) {
    SetErrorToMalformedResponseOnBadDERError();
    return SECFailure;
  }
  Context context(trustDomain, certID, time, maxOCSPLifetimeInDays,
                  thisUpdate, validThrough);

  if (der::Nested(input, der::SEQUENCE,
                  bind(OCSPResponse, _1, ref(context))) != der::Success) {
    SetErrorToMalformedResponseOnBadDERError();
    return SECFailure;
  }

  if (der::End(input) != der::Success) {
    SetErrorToMalformedResponseOnBadDERError();
    return SECFailure;
  }

  expired = context.expired;

  switch (context.certStatus) {
    case CertStatus::Good:
      if (expired) {
        PR_SetError(SEC_ERROR_OCSP_OLD_RESPONSE, 0);
        return SECFailure;
      }
      return SECSuccess;
    case CertStatus::Revoked:
      PR_SetError(SEC_ERROR_REVOKED_CERTIFICATE, 0);
      return SECFailure;
    case CertStatus::Unknown:
      PR_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT, 0);
      return SECFailure;
  }

  PR_NOT_REACHED("unknown CertStatus");
  PR_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT, 0);
  return SECFailure;
}





static inline der::Result
OCSPResponse(der::Input& input, Context& context)
{
  
  
  
  
  
  
  
  
  
  uint8_t responseStatus;

  if (der::Enumerated(input, responseStatus) != der::Success) {
    return der::Failure;
  }
  switch (responseStatus) {
    case 0: break; 
    case 1: return der::Fail(SEC_ERROR_OCSP_MALFORMED_REQUEST);
    case 2: return der::Fail(SEC_ERROR_OCSP_SERVER_ERROR);
    case 3: return der::Fail(SEC_ERROR_OCSP_TRY_SERVER_LATER);
    case 5: return der::Fail(SEC_ERROR_OCSP_REQUEST_NEEDS_SIG);
    case 6: return der::Fail(SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST);
    default: return der::Fail(SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS);
  }

  return der::Nested(input, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                     der::SEQUENCE, bind(ResponseBytes, _1, ref(context)));
}




static inline der::Result
ResponseBytes(der::Input& input, Context& context)
{
  static const uint8_t id_pkix_ocsp_basic[] = {
    0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };

  if (der::OID(input, id_pkix_ocsp_basic) != der::Success) {
    return der::Failure;
  }

  return der::Nested(input, der::OCTET_STRING, der::SEQUENCE,
                     bind(BasicResponse, _1, ref(context)));
}






der::Result
BasicResponse(der::Input& input, Context& context)
{
  der::Input tbsResponseData;
  CERTSignedData signedData;
  if (der::SignedData(input, tbsResponseData, signedData) != der::Success) {
    if (PR_GetError() == SEC_ERROR_BAD_SIGNATURE) {
      PR_SetError(SEC_ERROR_OCSP_BAD_SIGNATURE, 0);
    }
    return der::Failure;
  }

  

  SECItem certs[8];
  size_t numCerts = 0;

  if (!input.AtEnd()) {
    
    
    

    
    if (der::ExpectTagAndSkipLength(
          input, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0)
        != der::Success) {
      return der::Failure;
    }

    
    if (der::ExpectTagAndSkipLength(input, der::SEQUENCE) != der::Success) {
      return der::Failure;
    }

    
    while (!input.AtEnd()) {
      if (numCerts == PR_ARRAY_SIZE(certs)) {
        return der::Fail(SEC_ERROR_BAD_DER);
      }

      if (der::ExpectTagAndGetTLV(input, der::SEQUENCE, certs[numCerts])
            != der::Success) {
        return der::Failure;
      }
      ++numCerts;
    }
  }

  return ResponseData(tbsResponseData, context, signedData, certs, numCerts);
}







static inline der::Result
ResponseData(der::Input& input, Context& context,
             const CERTSignedData& signedResponseData,
              SECItem* certs, size_t numCerts)
{
  der::Version version;
  if (der::OptionalVersion(input, version) != der::Success) {
    return der::Failure;
  }
  if (version != der::Version::v1) {
    
    return der::Fail(SEC_ERROR_BAD_DER);
  }

  
  
  
  SECItem responderID;
  ResponderIDType responderIDType
    = input.Peek(static_cast<uint8_t>(ResponderIDType::byName))
    ? ResponderIDType::byName
    : ResponderIDType::byKey;
  if (ExpectTagAndGetValue(input, static_cast<uint8_t>(responderIDType),
                           responderID) != der::Success) {
    return der::Failure;
  }

  
  
  
  if (VerifySignature(context, responderIDType, responderID, certs, numCerts,
                      signedResponseData) != Success) {
    return der::Failure;
  }

  
  PRTime producedAt;
  if (der::GeneralizedTime(input, producedAt) != der::Success) {
    return der::Failure;
  }

  
  
  
  if (der::NestedOf(input, der::SEQUENCE, der::SEQUENCE,
                    der::EmptyAllowed::No,
                    bind(SingleResponse, _1, ref(context))) != der::Success) {
    return der::Failure;
  }

  return der::OptionalExtensions(input,
                                 der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                                 ExtensionNotUnderstood);
}










static inline der::Result
SingleResponse(der::Input& input, Context& context)
{
  bool match = false;
  if (der::Nested(input, der::SEQUENCE,
                  bind(CertID, _1, cref(context), ref(match)))
        != der::Success) {
    return der::Failure;
  }

  if (!match) {
    
    
    
    
    input.SkipToEnd();
    return der::Success;
  }

  
  
  
  
  
  
  
  
  
  
  if (input.Peek(static_cast<uint8_t>(CertStatus::Good))) {
    if (ExpectTagAndLength(input, static_cast<uint8_t>(CertStatus::Good), 0)
          != der::Success) {
      return der::Failure;
    }
    if (context.certStatus != CertStatus::Revoked) {
      context.certStatus = CertStatus::Good;
    }
  } else if (input.Peek(static_cast<uint8_t>(CertStatus::Revoked))) {
    
    
    
    
    if (der::ExpectTagAndSkipValue(input,
                                   static_cast<uint8_t>(CertStatus::Revoked))
          != der::Success) {
      return der::Failure;
    }
    context.certStatus = CertStatus::Revoked;
  } else if (ExpectTagAndLength(input,
                                static_cast<uint8_t>(CertStatus::Unknown),
                                0) != der::Success) {
    return der::Failure;
  }

  
  
  
  
  
  

  const PRTime maxLifetime =
    context.maxLifetimeInDays * ONE_DAY;

  PRTime thisUpdate;
  if (der::GeneralizedTime(input, thisUpdate) != der::Success) {
    return der::Failure;
  }

  if (thisUpdate > context.time + SLOP) {
    return der::Fail(SEC_ERROR_OCSP_FUTURE_RESPONSE);
  }

  PRTime notAfter;
  static const uint8_t NEXT_UPDATE_TAG =
    der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0;
  if (input.Peek(NEXT_UPDATE_TAG)) {
    PRTime nextUpdate;
    if (der::Nested(input, NEXT_UPDATE_TAG,
                    bind(der::GeneralizedTime, _1, ref(nextUpdate)))
          != der::Success) {
      return der::Failure;
    }

    if (nextUpdate < thisUpdate) {
      return der::Fail(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
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
    return der::Fail(SEC_ERROR_INVALID_ARGS);
  }

  if (context.time - SLOP > notAfter) {
    context.expired = true;
  }

  if (der::OptionalExtensions(input,
                              der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                              ExtensionNotUnderstood)
        != der::Success) {
    return der::Failure;
  }

  if (context.thisUpdate) {
    *context.thisUpdate = thisUpdate;
  }
  if (context.validThrough) {
    *context.validThrough = notAfter;
  }

  return der::Success;
}






static inline der::Result
CertID(der::Input& input, const Context& context,  bool& match)
{
  match = false;

  SECAlgorithmID hashAlgorithm;
  if (der::AlgorithmIdentifier(input, hashAlgorithm) != der::Success) {
    return der::Failure;
  }

  SECItem issuerNameHash;
  if (der::ExpectTagAndGetValue(input, der::OCTET_STRING, issuerNameHash)
        != der::Success) {
    return der::Failure;
  }

  SECItem issuerKeyHash;
  if (der::ExpectTagAndGetValue(input, der::OCTET_STRING, issuerKeyHash)
        != der::Success) {
    return der::Failure;
  }

  SECItem serialNumber;
  if (der::CertificateSerialNumber(input, serialNumber) != der::Success) {
    return der::Failure;
  }

  if (!SECITEM_ItemsAreEqual(&serialNumber, &context.certID.serialNumber)) {
    
    
    
    input.SkipToEnd();
    return der::Success;
  }

  

  SECOidTag hashAlg = SECOID_GetAlgorithmTag(&hashAlgorithm);
  if (hashAlg != SEC_OID_SHA1) {
    
    input.SkipToEnd();
    return der::Success;
  }

  if (issuerNameHash.len != SHA1_LENGTH) {
    return der::Fail(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
  }

  
  
  
  uint8_t hashBuf[SHA1_LENGTH];
  if (HashBuf(context.certID.issuer, hashBuf, sizeof(hashBuf))
        != der::Success) {
    return der::Failure;
  }
  if (memcmp(hashBuf, issuerNameHash.data, issuerNameHash.len)) {
    
    input.SkipToEnd();
    return der::Success;
  }

  if (MatchKeyHash(issuerKeyHash, context.certID.issuerSubjectPublicKeyInfo,
                   match) != Success) {
    return der::Failure;
  }

  return der::Success;
}











static Result
MatchKeyHash(const SECItem& keyHash, const SECItem& subjectPublicKeyInfo,
              bool& match)
{
  if (keyHash.len != SHA1_LENGTH)  {
    return Fail(RecoverableError, SEC_ERROR_OCSP_MALFORMED_RESPONSE);
  }
  static uint8_t hashBuf[SHA1_LENGTH];
  Result rv = KeyHash(subjectPublicKeyInfo, hashBuf, sizeof hashBuf);
  if (rv != Success) {
    return rv;
  }
  match = !memcmp(hashBuf, keyHash.data, keyHash.len);
  return Success;
}


Result
KeyHash(const SECItem& subjectPublicKeyInfo,  uint8_t* hashBuf,
        size_t hashBufSize)
{
  if (!hashBuf || hashBufSize != SHA1_LENGTH) {
    return Fail(FatalError, SEC_ERROR_LIBRARY_FAILURE);
  }

  
  
  
  
  

  der::Input spki;

  {
    
    
    der::Input input;
    if (input.Init(subjectPublicKeyInfo.data, subjectPublicKeyInfo.len)
          != der::Success) {
      return MapSECStatus(SECFailure);
    }

    if (der::ExpectTagAndGetValue(input, der::SEQUENCE, spki) != der::Success) {
      return MapSECStatus(SECFailure);
    }
    if (der::End(input) != der::Success) {
      return MapSECStatus(SECFailure);
    }
  }

  
  if (der::ExpectTagAndSkipValue(spki, der::SEQUENCE) != der::Success) {
    return MapSECStatus(SECFailure);
  }

  SECItem subjectPublicKey;
  if (der::ExpectTagAndGetValue(spki, der::BIT_STRING, subjectPublicKey)
        != der::Success) {
    return MapSECStatus(SECFailure);
  }

  if (der::End(spki) != der::Success) {
    return MapSECStatus(SECFailure);
  }

  
  if (subjectPublicKey.len == 0 || subjectPublicKey.data[0] != 0) {
    return Fail(RecoverableError, SEC_ERROR_BAD_DER);
  }
  ++subjectPublicKey.data;
  --subjectPublicKey.len;

  if (HashBuf(subjectPublicKey, hashBuf, hashBufSize) != der::Success) {
    return MapSECStatus(SECFailure);
  }
  return Success;
}

der::Result
ExtensionNotUnderstood(der::Input& , const SECItem& ,
                        bool& understood)
{
  understood = false;
  return der::Success;
}
























SECItem*
CreateEncodedOCSPRequest(PLArenaPool* arena, const struct CertID& certID)
{
  if (!arena) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return nullptr;
  }

  

  
  
  
  
  
  
  
  

  

  
  
  
  static const uint8_t hashAlgorithm[11] = {
    0x30, 0x09,                               
    0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 
    0x05, 0x00,                               
  };
  static const uint8_t hashLen = SHA1_LENGTH;

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

  
  
  
  
  
  
  if (certID.serialNumber.len > 127u - totalLenWithoutSerialNumberData) {
    PR_SetError(SEC_ERROR_BAD_DATA, 0);
    return nullptr;
  }

  uint8_t totalLen = static_cast<uint8_t>(totalLenWithoutSerialNumberData +
    certID.serialNumber.len);

  SECItem* encodedRequest = SECITEM_AllocItem(arena, nullptr, totalLen);
  if (!encodedRequest) {
    return nullptr;
  }

  uint8_t* d = encodedRequest->data;
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
  if (HashBuf(certID.issuer, d, hashLen) != der::Success) {
    return nullptr;
  }
  d += hashLen;

  
  *d++ = 0x04;
  *d++ = hashLen;
  if (KeyHash(certID.issuerSubjectPublicKeyInfo, d, hashLen) != Success) {
    return nullptr;
  }
  d += hashLen;

  
  *d++ = 0x02; 
  *d++ = static_cast<uint8_t>(certID.serialNumber.len);
  for (size_t i = 0; i < certID.serialNumber.len; ++i) {
    *d++ = certID.serialNumber.data[i];
  }

  PR_ASSERT(d == encodedRequest->data + totalLen);

  return encodedRequest;
}

} } 
