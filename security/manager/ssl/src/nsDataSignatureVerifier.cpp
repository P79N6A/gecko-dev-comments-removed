



#include "nsDataSignatureVerifier.h"

#include "cms.h"
#include "cryptohi.h"
#include "keyhi.h"
#include "nsCOMPtr.h"
#include "nsNSSComponent.h"
#include "nssb64.h"
#include "pkix/pkixtypes.h"
#include "ScopedNSSTypes.h"
#include "secerr.h"
#include "SharedCertVerifier.h"

using namespace mozilla;
using namespace mozilla::pkix;
using namespace mozilla::psm;

SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)

NS_IMPL_ISUPPORTS(nsDataSignatureVerifier, nsIDataSignatureVerifier)

const SEC_ASN1Template CERT_SignatureDataTemplate[] =
{
    { SEC_ASN1_SEQUENCE,
        0, nullptr, sizeof(CERTSignedData) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
        offsetof(CERTSignedData,signatureAlgorithm),
        SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate), },
    { SEC_ASN1_BIT_STRING,
        offsetof(CERTSignedData,signature), },
    { 0, }
};

NS_IMETHODIMP
nsDataSignatureVerifier::VerifyData(const nsACString & aData,
                                    const nsACString & aSignature,
                                    const nsACString & aPublicKey,
                                    bool *_retval)
{
    
    PLArenaPool *arena;
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
        return NS_ERROR_OUT_OF_MEMORY;

    
    SECItem keyItem;
    PORT_Memset(&keyItem, 0, sizeof(SECItem));
    if (!NSSBase64_DecodeBuffer(arena, &keyItem,
                                nsPromiseFlatCString(aPublicKey).get(),
                                aPublicKey.Length())) {
        PORT_FreeArena(arena, false);
        return NS_ERROR_FAILURE;
    }
    
    
    CERTSubjectPublicKeyInfo *pki = SECKEY_DecodeDERSubjectPublicKeyInfo(&keyItem);
    if (!pki) {
        PORT_FreeArena(arena, false);
        return NS_ERROR_FAILURE;
    }
    SECKEYPublicKey *publicKey = SECKEY_ExtractPublicKey(pki);
    SECKEY_DestroySubjectPublicKeyInfo(pki);
    pki = nullptr;
    
    if (!publicKey) {
        PORT_FreeArena(arena, false);
        return NS_ERROR_FAILURE;
    }
    
    
    SECItem signatureItem;
    PORT_Memset(&signatureItem, 0, sizeof(SECItem));
    if (!NSSBase64_DecodeBuffer(arena, &signatureItem,
                                nsPromiseFlatCString(aSignature).get(),
                                aSignature.Length())) {
        SECKEY_DestroyPublicKey(publicKey);
        PORT_FreeArena(arena, false);
        return NS_ERROR_FAILURE;
    }
    
    
    CERTSignedData sigData;
    PORT_Memset(&sigData, 0, sizeof(CERTSignedData));
    SECStatus ss = SEC_QuickDERDecodeItem(arena, &sigData, 
                                          CERT_SignatureDataTemplate,
                                          &signatureItem);
    if (ss != SECSuccess) {
        SECKEY_DestroyPublicKey(publicKey);
        PORT_FreeArena(arena, false);
        return NS_ERROR_FAILURE;
    }
    
    
    DER_ConvertBitString(&(sigData.signature));
    ss = VFY_VerifyDataWithAlgorithmID((const unsigned char*)nsPromiseFlatCString(aData).get(),
                                       aData.Length(), publicKey,
                                       &(sigData.signature),
                                       &(sigData.signatureAlgorithm),
                                       nullptr, nullptr);
    
    
    SECKEY_DestroyPublicKey(publicKey);
    PORT_FreeArena(arena, false);
    
    *_retval = (ss == SECSuccess);

    return NS_OK;
}

namespace mozilla {

nsresult
VerifyCMSDetachedSignatureIncludingCertificate(
  const SECItem& buffer, const SECItem& detachedDigest,
  nsresult (*verifyCertificate)(CERTCertificate* cert, void* context,
                                void* pinArg),
  void *verifyCertificateContext, void* pinArg)
{
  
  if (NS_WARN_IF(!buffer.data && buffer.len > 0) ||
      NS_WARN_IF(!detachedDigest.data && detachedDigest.len > 0) ||
      (!verifyCertificate) ||
      NS_WARN_IF(!verifyCertificateContext)) {
    return NS_ERROR_INVALID_ARG;
  }

  ScopedNSSCMSMessage
    cmsMsg(NSS_CMSMessage_CreateFromDER(const_cast<SECItem*>(&buffer), nullptr,
                                        nullptr, nullptr, nullptr, nullptr,
                                        nullptr));
  if (!cmsMsg) {
    return NS_ERROR_CMS_VERIFY_ERROR_PROCESSING;
  }

  if (!NSS_CMSMessage_IsSigned(cmsMsg.get())) {
    return NS_ERROR_CMS_VERIFY_NOT_SIGNED;
  }

  NSSCMSContentInfo* cinfo = NSS_CMSMessage_ContentLevel(cmsMsg.get(), 0);
  if (!cinfo) {
    return NS_ERROR_CMS_VERIFY_NO_CONTENT_INFO;
  }

  
  NSSCMSSignedData* signedData =
    reinterpret_cast<NSSCMSSignedData*>(NSS_CMSContentInfo_GetContent(cinfo));
  if (!signedData) {
    return NS_ERROR_CMS_VERIFY_NO_CONTENT_INFO;
  }

  
  if (NSS_CMSSignedData_SetDigestValue(signedData, SEC_OID_SHA1,
                                       const_cast<SECItem*>(&detachedDigest))) {
    return NS_ERROR_CMS_VERIFY_BAD_DIGEST;
  }

  
  
  ScopedCERTCertList certs(CERT_NewCertList());
  if (!certs) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (signedData->rawCerts) {
    for (size_t i = 0; signedData->rawCerts[i]; ++i) {
      ScopedCERTCertificate
        cert(CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                     signedData->rawCerts[i], nullptr, false,
                                     true));
      
      if (cert) {
        if (CERT_AddCertToListTail(certs.get(), cert.get()) == SECSuccess) {
          cert.forget(); 
        } else {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
    }
  }

  
  int numSigners = NSS_CMSSignedData_SignerInfoCount(signedData);
  if (NS_WARN_IF(numSigners != 1)) {
    return NS_ERROR_CMS_VERIFY_ERROR_PROCESSING;
  }
  
  NSSCMSSignerInfo* signer = NSS_CMSSignedData_GetSignerInfo(signedData, 0);
  if (NS_WARN_IF(!signer)) {
    return NS_ERROR_CMS_VERIFY_ERROR_PROCESSING;
  }
  CERTCertificate* signerCert =
    NSS_CMSSignerInfo_GetSigningCertificate(signer, CERT_GetDefaultCertDB());
  if (!signerCert) {
    return NS_ERROR_CMS_VERIFY_ERROR_PROCESSING;
  }

  nsresult rv = verifyCertificate(signerCert, verifyCertificateContext, pinArg);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  SECOidData* contentTypeOidData =
    SECOID_FindOID(&signedData->contentInfo.contentType);
  if (!contentTypeOidData) {
    return NS_ERROR_CMS_VERIFY_ERROR_PROCESSING;
  }

  return MapSECStatus(NSS_CMSSignerInfo_Verify(signer,
                         const_cast<SECItem*>(&detachedDigest),
                         &contentTypeOidData->oid));
}

} 

namespace {

struct VerifyCertificateContext
{
  nsCOMPtr<nsIX509Cert> signingCert;
  ScopedCERTCertList builtChain;
};

static nsresult
VerifyCertificate(CERTCertificate* cert, void* voidContext, void* pinArg)
{
  
  if (NS_WARN_IF(!cert) || NS_WARN_IF(!voidContext)) {
    return NS_ERROR_INVALID_ARG;
  }

  VerifyCertificateContext* context =
    reinterpret_cast<VerifyCertificateContext*>(voidContext);

  nsCOMPtr<nsIX509Cert> xpcomCert(nsNSSCertificate::Create(cert));
  if (!xpcomCert) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  context->signingCert = xpcomCert;

  RefPtr<SharedCertVerifier> certVerifier(GetDefaultCertVerifier());
  NS_ENSURE_TRUE(certVerifier, NS_ERROR_UNEXPECTED);

  return MapSECStatus(certVerifier->VerifyCert(cert,
                                               certificateUsageObjectSigner,
                                               Now(), pinArg,
                                               nullptr, 
                                               0, 
                                               nullptr, 
                                               &context->builtChain));
}

} 

NS_IMETHODIMP
nsDataSignatureVerifier::VerifySignature(const char* aRSABuf,
                                         uint32_t aRSABufLen,
                                         const char* aPlaintext,
                                         uint32_t aPlaintextLen,
                                         int32_t* aErrorCode,
                                         nsIX509Cert** aSigningCert)
{
  if (!aPlaintext || !aSigningCert || !aErrorCode) {
    return NS_ERROR_INVALID_ARG;
  }

  *aErrorCode = VERIFY_ERROR_OTHER;
  *aSigningCert = nullptr;

  nsNSSShutDownPreventionLock locker;

  Digest digest;
  nsresult rv = digest.DigestBuf(SEC_OID_SHA1,
                                 reinterpret_cast<const uint8_t*>(aPlaintext),
                                 aPlaintextLen);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  SECItem buffer = {
    siBuffer,
    reinterpret_cast<uint8_t*>(const_cast<char*>(aRSABuf)),
    aRSABufLen
  };

  VerifyCertificateContext context;
  
  rv = VerifyCMSDetachedSignatureIncludingCertificate(buffer, digest.get(),
                                                      VerifyCertificate,
                                                      &context, nullptr);
  if (NS_SUCCEEDED(rv)) {
    *aErrorCode = VERIFY_OK;
  } else if (NS_ERROR_GET_MODULE(rv) == NS_ERROR_MODULE_SECURITY) {
    if (rv == GetXPCOMFromNSSError(SEC_ERROR_UNKNOWN_ISSUER)) {
      *aErrorCode = VERIFY_ERROR_UNKNOWN_ISSUER;
    } else {
      *aErrorCode = VERIFY_ERROR_OTHER;
    }
    rv = NS_OK;
  }
  if (rv == NS_OK) {
    context.signingCert.forget(aSigningCert);
  }

  return rv;
}
