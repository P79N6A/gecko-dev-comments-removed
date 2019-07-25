































#include "nsDataSignatureVerifier.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#include "seccomon.h"
#include "nssb64.h"
#include "certt.h"
#include "keyhi.h"
#include "cryptohi.h"

SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)

NS_IMPL_ISUPPORTS1(nsDataSignatureVerifier, nsIDataSignatureVerifier)

const SEC_ASN1Template CERT_SignatureDataTemplate[] =
{
    { SEC_ASN1_SEQUENCE,
        0, NULL, sizeof(CERTSignedData) },
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
    
    PRArenaPool *arena;
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
    pki = nsnull;
    
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
                                       NULL, NULL);
    
    
    SECKEY_DestroyPublicKey(publicKey);
    PORT_FreeArena(arena, false);
    
    *_retval = (ss == SECSuccess);

    return NS_OK;
}
