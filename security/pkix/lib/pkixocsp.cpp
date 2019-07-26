























#include <limits>

#include "pkix/bind.h"
#include "pkix/pkix.h"
#include "pkixcheck.h"
#include "pkixder.h"

#include "hasht.h"
#include "pk11pub.h"
#include "secder.h"




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
  Context(TrustDomain& trustDomain,
          const CERTCertificate& cert,
          CERTCertificate& issuerCert,
          PRTime time,
          PRTime* thisUpdate,
          PRTime* validThrough)
    : trustDomain(trustDomain)
    , cert(cert)
    , issuerCert(issuerCert)
    , time(time)
    , certStatus(CertStatus::Unknown)
    , thisUpdate(thisUpdate)
    , validThrough(validThrough)
  {
    if (thisUpdate) {
      *thisUpdate = 0;
    }
    if (validThrough) {
      *validThrough = 0;
    }
  }

  TrustDomain& trustDomain;
  const CERTCertificate& cert;
  CERTCertificate& issuerCert;
  const PRTime time;
  CertStatus certStatus;
  PRTime* thisUpdate;
  PRTime* validThrough;

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
                            CERTCertificate& potentialSigner,
                            const CERTCertificate& issuerCert, PRTime time)
{
  Result rv;

  BackCert cert(&potentialSigner, nullptr, BackCert::IncludeCN::No);
  rv = cert.Init();
  if (rv != Success) {
    return rv;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  rv = CheckIssuerIndependentProperties(trustDomain, cert, time,
                                        EndEntityOrCA::MustBeEndEntity, 0,
                                        KeyPurposeId::id_kp_OCSPSigning,
                                        SEC_OID_X509_ANY_POLICY, 0);
  if (rv != Success) {
    return rv;
  }

  
  
  
  if (!SECITEM_ItemsAreEqual(&cert.GetNSSCert()->derIssuer,
                             &issuerCert.derSubject) &&
      CERT_CompareName(&cert.GetNSSCert()->issuer,
                       &issuerCert.subject) != SECEqual) {
    return Fail(RecoverableError, SEC_ERROR_OCSP_RESPONDER_CERT_INVALID);
  }

  

  if (trustDomain.VerifySignedData(&potentialSigner.signatureWrap,
                                   &issuerCert) != SECSuccess) {
    return MapSECStatus(SECFailure);
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
static inline der::Result CheckExtensionsForCriticality(der::Input&);
static inline der::Result CertID(der::Input& input,
                                  const Context& context,
                                   bool& match);
static der::Result MatchKeyHash(const SECItem& issuerKeyHash,
                                const CERTCertificate& issuer,
                                 bool& match);







static CERTCertificate*
GetOCSPSignerCertificate(TrustDomain& trustDomain,
                         ResponderIDType responderIDType,
                         const SECItem& responderIDItem,
                         const SECItem* certs, size_t numCerts,
                         CERTCertificate& issuerCert, PRTime time)
{
  bool isIssuer = true;
  size_t i = 0;
  for (;;) {
    ScopedCERTCertificate potentialSigner;
    if (isIssuer) {
      potentialSigner = CERT_DupCertificate(&issuerCert);
    } else if (i < numCerts) {
      potentialSigner = CERT_NewTempCertificate(
                          CERT_GetDefaultCertDB(),
                          const_cast<SECItem*>(&certs[i]), nullptr,
                          false, false);
      if (!potentialSigner) {
        return nullptr;
      }
      ++i;
    } else {
      PR_SetError(SEC_ERROR_OCSP_INVALID_SIGNING_CERT, 0);
      return nullptr;
    }

    bool match;
    switch (responderIDType) {
      case ResponderIDType::byName:
        
        
        
        
        
        
        
        match = SECITEM_ItemsAreEqual(&responderIDItem,
                                      &potentialSigner->derSubject);
        if (!match) {
          ScopedPLArenaPool arena(PORT_NewArena(DER_DEFAULT_CHUNKSIZE));
          if (!arena) {
            return nullptr;
          }
          CERTName name;
          if (SEC_QuickDERDecodeItem(arena.get(), &name,
                                     SEC_ASN1_GET(CERT_NameTemplate),
                                     &responderIDItem) != SECSuccess) {
            return nullptr;
          }
          match = CERT_CompareName(&name, &potentialSigner->subject) == SECEqual;
        }
        break;

      case ResponderIDType::byKey:
      {
        der::Input responderID;
        if (responderID.Init(responderIDItem.data, responderIDItem.len)
              != der::Success) {
          return nullptr;
        }
        SECItem keyHash;
        if (der::Skip(responderID, der::OCTET_STRING, keyHash) != der::Success) {
          return nullptr;
        }
        if (MatchKeyHash(keyHash, *potentialSigner.get(), match) != der::Success) {
          return nullptr;
        }
        break;
      }

      default:
        PR_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE, 0);
        return nullptr;
    }

    if (match && !isIssuer) {
      Result rv = CheckOCSPResponseSignerCert(trustDomain,
                                              *potentialSigner.get(),
                                              issuerCert, time);
      if (rv == RecoverableError) {
        match = false;
      } else if (rv != Success) {
        return nullptr;
      }
    }

    if (match) {
      return potentialSigner.release();
    }

    isIssuer = false;
  }
}

static SECStatus
VerifySignature(Context& context, ResponderIDType responderIDType,
                const SECItem& responderID, const SECItem* certs,
                size_t numCerts, const CERTSignedData& signedResponseData)
{
  ScopedCERTCertificate signer(
    GetOCSPSignerCertificate(context.trustDomain, responderIDType, responderID,
                             certs, numCerts, context.issuerCert,
                             context.time));
  if (!signer) {
    return SECFailure;
  }

  if (context.trustDomain.VerifySignedData(&signedResponseData, signer.get())
        != SECSuccess) {
    if (PR_GetError() == SEC_ERROR_BAD_SIGNATURE) {
      PR_SetError(SEC_ERROR_OCSP_BAD_SIGNATURE, 0);
    }
    return SECFailure;
  }

  return SECSuccess;
}

static inline void
SetErrorToMalformedResponseOnBadDERError()
{
  if (PR_GetError() == SEC_ERROR_BAD_DER) {
    PR_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE, 0);
  }
}

SECStatus
VerifyEncodedOCSPResponse(TrustDomain& trustDomain,
                          const CERTCertificate* cert,
                          CERTCertificate* issuerCert, PRTime time,
                          const SECItem* encodedResponse,
                          PRTime* thisUpdate,
                          PRTime* validThrough)
{
  PR_ASSERT(cert);
  PR_ASSERT(issuerCert);
  
  PR_ASSERT(encodedResponse);
  if (!cert || !issuerCert || !encodedResponse || !encodedResponse->data) {
    PR_SetError(SEC_ERROR_INVALID_ARGS, 0);
    return SECFailure;
  }

  der::Input input;
  if (input.Init(encodedResponse->data, encodedResponse->len) != der::Success) {
    SetErrorToMalformedResponseOnBadDERError();
    return SECFailure;
  }

  Context context(trustDomain, *cert, *issuerCert, time, thisUpdate,
                  validThrough);

  if (der::Nested(input, der::SEQUENCE,
                  bind(OCSPResponse, _1, ref(context))) != der::Success) {
    SetErrorToMalformedResponseOnBadDERError();
    return SECFailure;
  }

  if (der::End(input) != der::Success) {
    SetErrorToMalformedResponseOnBadDERError();
    return SECFailure;
  }

  switch (context.certStatus) {
    case CertStatus::Good:
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
  der::Input::Mark mark(input.GetMark());

  uint16_t length;
  if (der::ExpectTagAndGetLength(input, der::SEQUENCE, length)
        != der::Success) {
    return der::Failure;
  }

  
  
  
  

  der::Input tbsResponseData;

  if (input.Skip(length, tbsResponseData) != der::Success) {
    return der::Failure;
  }

  CERTSignedData signedData;

  if (input.GetSECItem(siBuffer, mark, signedData.data) != der::Success) {
    return der::Failure;
  }

  if (der::Nested(input, der::SEQUENCE,
                  bind(der::AlgorithmIdentifier, _1,
                       ref(signedData.signatureAlgorithm))) != der::Success) {
    return der::Failure;
  }

  if (der::Skip(input, der::BIT_STRING, signedData.signature) != der::Success) {
    return der::Failure;
  }
  if (signedData.signature.len == 0) {
    return der::Fail(SEC_ERROR_OCSP_BAD_SIGNATURE);
  }
  unsigned int unusedBitsAtEnd = signedData.signature.data[0];
  
  
  
  
  if (unusedBitsAtEnd != 0) {
    return der::Fail(SEC_ERROR_OCSP_BAD_SIGNATURE);
  }
  ++signedData.signature.data;
  --signedData.signature.len;
  signedData.signature.len = (signedData.signature.len << 3); 

  

  SECItem certs[8];
  size_t numCerts = 0;

  if (!input.AtEnd()) {
    
    
    

    
    if (der::ExpectTagAndIgnoreLength(
          input, der::CONSTRUCTED | der::CONTEXT_SPECIFIC | 0)
        != der::Success) {
      return der::Failure;
    }

    
    if (der::ExpectTagAndIgnoreLength(input, der::SEQUENCE) != der::Success) {
      return der::Failure;
    }

    
    while (!input.AtEnd()) {
      if (numCerts == PR_ARRAY_SIZE(certs)) {
        return der::Fail(SEC_ERROR_BAD_DER);
      }

      
      
      der::Input::Mark mark(input.GetMark());
      if (der::Skip(input, der::SEQUENCE) != der::Success) {
        return der::Failure;
      }

      if (input.GetSECItem(siBuffer, mark, certs[numCerts]) != der::Success) {
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
  uint8_t version;
  if (der::OptionalVersion(input, version) != der::Success) {
    return der::Failure;
  }
  if (version != der::v1) {
    
    return der::Fail(SEC_ERROR_BAD_DER);
  }

  
  
  
  SECItem responderID;
  uint16_t responderIDLength;
  ResponderIDType responderIDType
    = input.Peek(static_cast<uint8_t>(ResponderIDType::byName))
    ? ResponderIDType::byName
    : ResponderIDType::byKey;
  if (ExpectTagAndGetLength(input, static_cast<uint8_t>(responderIDType),
                            responderIDLength) != der::Success) {
    return der::Failure;
  }
  
  
  if (input.Skip(responderIDLength, responderID) != der::Success) {
    return der::Failure;
  }

  
  
  
  if (VerifySignature(context, responderIDType, responderID, certs, numCerts,
                      signedResponseData) != SECSuccess) {
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

  if (!input.AtEnd()) {
    if (der::Nested(input, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                    CheckExtensionsForCriticality) != der::Success) {
      return der::Failure;
    }
  }

  return der::Success;
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
    
    
    
    
    if (der::Skip(input, static_cast<uint8_t>(CertStatus::Revoked))
          != der::Success) {
      return der::Failure;
    }
    context.certStatus = CertStatus::Revoked;
  } else if (ExpectTagAndLength(input,
                                static_cast<uint8_t>(CertStatus::Unknown),
                                0) != der::Success) {
    return der::Failure;
  }

  
  
  
  
  
  

  
  
  static const PRTime OLDEST_ACCEPTABLE = INT64_C(10) * ONE_DAY;

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
    if (nextUpdate - thisUpdate <= OLDEST_ACCEPTABLE) {
      notAfter = nextUpdate;
    } else {
      notAfter = thisUpdate + OLDEST_ACCEPTABLE;
    }
  } else {
    
    
    notAfter = thisUpdate + ONE_DAY;
  }

  if (context.time < SLOP) { 
    return der::Fail(SEC_ERROR_INVALID_ARGS);
  }
  if (context.time - SLOP > notAfter) {
    return der::Fail(SEC_ERROR_OCSP_OLD_RESPONSE);
  }

  if (!input.AtEnd()) {
    if (der::Nested(input, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                    CheckExtensionsForCriticality) != der::Success) {
      return der::Failure;
    }
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
  if (der::Nested(input, der::SEQUENCE,
                  bind(der::AlgorithmIdentifier, _1, ref(hashAlgorithm)))
         != der::Success) {
    return der::Failure;
  }

  SECItem issuerNameHash;
  if (der::Skip(input, der::OCTET_STRING, issuerNameHash) != der::Success) {
    return der::Failure;
  }

  SECItem issuerKeyHash;
  if (der::Skip(input, der::OCTET_STRING, issuerKeyHash) != der::Success) {
    return der::Failure;
  }

  SECItem serialNumber;
  if (der::CertificateSerialNumber(input, serialNumber) != der::Success) {
    return der::Failure;
  }

  const CERTCertificate& cert = context.cert;
  const CERTCertificate& issuerCert = context.issuerCert;

  if (!SECITEM_ItemsAreEqual(&serialNumber, &cert.serialNumber)) {
    
    
    
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
  if (HashBuf(cert.derIssuer, hashBuf, sizeof(hashBuf)) != der::Success) {
    return der::Failure;
  }
  if (memcmp(hashBuf, issuerNameHash.data, issuerNameHash.len)) {
    
    input.SkipToEnd();
    return der::Success;
  }

  return MatchKeyHash(issuerKeyHash, issuerCert, match);
}




static der::Result
MatchKeyHash(const SECItem& keyHash, const CERTCertificate& cert,
              bool& match)
{
  if (keyHash.len != SHA1_LENGTH)  {
    return der::Fail(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
  }

  

  
  
  
  SECItem spk = cert.subjectPublicKeyInfo.subjectPublicKey;
  DER_ConvertBitString(&spk);

  static uint8_t hashBuf[SHA1_LENGTH];
  if (HashBuf(spk, hashBuf, sizeof(hashBuf)) != der::Success) {
    return der::Failure;
  }

  match = !memcmp(hashBuf, keyHash.data, keyHash.len);
  return der::Success;
}






static der::Result
CheckExtensionForCriticality(der::Input& input)
{
  uint16_t toSkip;
  if (ExpectTagAndGetLength(input, der::OIDTag, toSkip) != der::Success) {
    return der::Failure;
  }

  
  if (input.Skip(toSkip) != der::Success) {
    return der::Failure;
  }

  
  
  if (input.Peek(der::BOOLEAN)) {
    return der::Fail(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION);
  }

  if (ExpectTagAndGetLength(input, der::OCTET_STRING, toSkip)
        != der::Success) {
    return der::Failure;
  }
  return input.Skip(toSkip);
}


static der::Result
CheckExtensionsForCriticality(der::Input& input)
{
  
  
  
  return der::NestedOf(input, der::SEQUENCE, der::SEQUENCE,
                       der::EmptyAllowed::Yes, CheckExtensionForCriticality);
}
























SECItem*
CreateEncodedOCSPRequest(PLArenaPool* arena,
                         const CERTCertificate* cert,
                         const CERTCertificate* issuerCert)
{
  if (!arena || !cert || !issuerCert) {
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

  
  
  
  
  
  
  if (issuerCert->serialNumber.len > 127u - totalLenWithoutSerialNumberData) {
    PR_SetError(SEC_ERROR_BAD_DATA, 0);
    return nullptr;
  }

  uint8_t totalLen = static_cast<uint8_t>(totalLenWithoutSerialNumberData +
    cert->serialNumber.len);

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
  if (HashBuf(issuerCert->derSubject, d, hashLen) != der::Success) {
    return nullptr;
  }
  d += hashLen;

  
  *d++ = 0x04;
  *d++ = hashLen;
  SECItem key = issuerCert->subjectPublicKeyInfo.subjectPublicKey;
  DER_ConvertBitString(&key);
  if (HashBuf(key, d, hashLen) != der::Success) {
    return nullptr;
  }
  d += hashLen;

  
  *d++ = 0x02; 
  *d++ = static_cast<uint8_t>(cert->serialNumber.len);
  for (size_t i = 0; i < cert->serialNumber.len; ++i) {
    *d++ = cert->serialNumber.data[i];
  }

  PR_ASSERT(d == encodedRequest->data + totalLen);

  return encodedRequest;
}

} } 
