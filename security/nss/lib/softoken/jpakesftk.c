


































#include "seccomon.h"
#include "secerr.h"
#include "blapi.h"
#include "pkcs11i.h"
#include "softoken.h"

static CK_RV
jpake_mapStatus(SECStatus rv, CK_RV invalidArgsMapping) {
    int err;
    if (rv == SECSuccess)
        return CKR_OK;
    err = PORT_GetError();
    switch (err) {
        

        case SEC_ERROR_INVALID_ARGS:  return invalidArgsMapping;
        case SEC_ERROR_BAD_SIGNATURE: return CKR_SIGNATURE_INVALID;
        case SEC_ERROR_NO_MEMORY:     return CKR_HOST_MEMORY;
    }
    return CKR_FUNCTION_FAILED;
}



static CK_RV
jpake_Sign(PLArenaPool * arena, const PQGParams * pqg, HASH_HashType hashType,
           const SECItem * signerID, const SECItem * x,
           CK_NSS_JPAKEPublicValue * out)
{
    SECItem gx, gv, r;
    CK_RV crv;

    PORT_Assert(arena != NULL);
    
    gx.data = NULL;
    gv.data = NULL;
    r.data = NULL;
    crv = jpake_mapStatus(JPAKE_Sign(arena, pqg, hashType, signerID, x, NULL,
                                     NULL, &gx, &gv, &r),
                          CKR_MECHANISM_PARAM_INVALID);
    if (crv == CKR_OK) {
        if ((out->pGX != NULL && out->ulGXLen >= gx.len) ||
            (out->pGV != NULL && out->ulGVLen >= gv.len) ||
            (out->pR  != NULL && out->ulRLen >= r.len)) {
            PORT_Memcpy(out->pGX, gx.data, gx.len); 
            PORT_Memcpy(out->pGV, gv.data, gv.len); 
            PORT_Memcpy(out->pR, r.data, r.len);
            out->ulGXLen = gx.len;
            out->ulGVLen = gv.len;
            out->ulRLen = r.len;
        } else {
            crv = CKR_MECHANISM_PARAM_INVALID;
        }
    } 
    return crv;
}

static CK_RV
jpake_Verify(PLArenaPool * arena, const PQGParams * pqg,
             HASH_HashType hashType, const SECItem * signerID,
             const CK_BYTE * peerIDData, CK_ULONG peerIDLen,
             const CK_NSS_JPAKEPublicValue * publicValueIn)
{
    SECItem peerID, gx, gv, r;
    peerID.data = (unsigned char *) peerIDData; peerID.len = peerIDLen;
    gx.data = publicValueIn->pGX; gx.len = publicValueIn->ulGXLen;
    gv.data = publicValueIn->pGV; gv.len = publicValueIn->ulGVLen;
    r.data = publicValueIn->pR;   r.len = publicValueIn->ulRLen;
    return jpake_mapStatus(JPAKE_Verify(arena, pqg, hashType, signerID, &peerID,
                                        &gx, &gv, &r),
                           CKR_MECHANISM_PARAM_INVALID);
}

#define NUM_ELEM(x) (sizeof (x) / sizeof (x)[0])





static CK_RV
jpake_enforceKeyType(SFTKObject * key, CK_KEY_TYPE keyType) {
    CK_RV crv;
    SFTKAttribute * keyTypeAttr = sftk_FindAttribute(key, CKA_KEY_TYPE);
    if (keyTypeAttr != NULL) {
        crv = *(CK_KEY_TYPE *)keyTypeAttr->attrib.pValue == keyType
            ? CKR_OK
            : CKR_TEMPLATE_INCONSISTENT;
        sftk_FreeAttribute(keyTypeAttr);
    } else {
        crv = sftk_forceAttribute(key, CKA_KEY_TYPE, &keyType, sizeof keyType);
    }
    return crv;
}

static CK_RV
jpake_MultipleSecItem2Attribute(SFTKObject * key, const SFTKItemTemplate * attrs,
                                size_t attrsCount)
{
    size_t i;
    
    for (i = 0; i < attrsCount; ++i) {
        CK_RV crv = sftk_forceAttribute(key, attrs[i].type, attrs[i].item->data,
                                        attrs[i].item->len);
        if (crv != CKR_OK)
            return crv;
    }
    return CKR_OK;
}

CK_RV
jpake_Round1(HASH_HashType hashType, CK_NSS_JPAKERound1Params * params,
             SFTKObject * key)
{
    CK_RV crv;
    PQGParams pqg;
    PLArenaPool * arena;
    SECItem signerID;
    SFTKItemTemplate templateAttrs[] = {
        { CKA_PRIME, &pqg.prime },
        { CKA_SUBPRIME, &pqg.subPrime },
        { CKA_BASE, &pqg.base },
        { CKA_NSS_JPAKE_SIGNERID, &signerID }
    };
    SECItem x2, gx1, gx2;
    const SFTKItemTemplate generatedAttrs[] = {
        { CKA_NSS_JPAKE_X2,  &x2  },
        { CKA_NSS_JPAKE_GX1, &gx1 },
        { CKA_NSS_JPAKE_GX2, &gx2 },
    };
    SECItem x1;

    PORT_Assert(params != NULL);
    PORT_Assert(key != NULL);

    arena = PORT_NewArena(NSS_SOFTOKEN_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
        crv = CKR_HOST_MEMORY;

    crv = sftk_MultipleAttribute2SecItem(arena, key, templateAttrs,
                                         NUM_ELEM(templateAttrs));

    if (crv == CKR_OK && (signerID.data == NULL || signerID.len == 0))
        crv = CKR_TEMPLATE_INCOMPLETE;

    
    if (crv == CKR_OK) {
        x1.data = NULL;
        crv = jpake_mapStatus(DSA_NewRandom(arena, &pqg.subPrime, &x1),
                              CKR_TEMPLATE_INCONSISTENT);
    }
    if (crv == CKR_OK)
        crv = jpake_Sign(arena, &pqg, hashType, &signerID, &x1, &params->gx1);

    
    if (crv == CKR_OK) {
        x2.data = NULL;
        crv = jpake_mapStatus(DSA_NewRandom(arena, &pqg.subPrime, &x2),
                              CKR_TEMPLATE_INCONSISTENT);
    }
    if (crv == CKR_OK)
        crv = jpake_Sign(arena, &pqg, hashType, &signerID, &x2, &params->gx2);

    
    if (crv == CKR_OK) {
        gx1.data = params->gx1.pGX;
        gx1.len = params->gx1.ulGXLen;
        gx2.data = params->gx2.pGX;
        gx2.len = params->gx2.ulGXLen;
        crv = jpake_MultipleSecItem2Attribute(key, generatedAttrs, 
                                              NUM_ELEM(generatedAttrs));
    }

    PORT_FreeArena(arena, PR_TRUE);
    return crv;
}

CK_RV
jpake_Round2(HASH_HashType hashType, CK_NSS_JPAKERound2Params * params,
             SFTKObject * sourceKey, SFTKObject * key)
{
    CK_RV crv;
    PLArenaPool * arena;
    PQGParams pqg;
    SECItem signerID, x2, gx1, gx2;
    SFTKItemTemplate sourceAttrs[] = { 
        { CKA_PRIME, &pqg.prime },
        { CKA_SUBPRIME, &pqg.subPrime },
        { CKA_BASE, &pqg.base },
        { CKA_NSS_JPAKE_SIGNERID, &signerID },
        { CKA_NSS_JPAKE_X2,  &x2 },
        { CKA_NSS_JPAKE_GX1, &gx1 },
        { CKA_NSS_JPAKE_GX2, &gx2 },
    };
    SECItem x2s, gx3, gx4;
    const SFTKItemTemplate copiedAndGeneratedAttrs[] = {
        { CKA_NSS_JPAKE_SIGNERID, &signerID },
        { CKA_PRIME, &pqg.prime },
        { CKA_SUBPRIME, &pqg.subPrime },
        { CKA_NSS_JPAKE_X2,  &x2  },
        { CKA_NSS_JPAKE_X2S, &x2s },
        { CKA_NSS_JPAKE_GX1, &gx1 },
        { CKA_NSS_JPAKE_GX2, &gx2 },
        { CKA_NSS_JPAKE_GX3, &gx3 },
        { CKA_NSS_JPAKE_GX4, &gx4 }
    };
    SECItem peerID;

    PORT_Assert(params != NULL);
    PORT_Assert(sourceKey != NULL);
    PORT_Assert(key != NULL);

    arena = PORT_NewArena(NSS_SOFTOKEN_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
        crv = CKR_HOST_MEMORY;

    

    crv = sftk_MultipleAttribute2SecItem(arena, sourceKey, sourceAttrs,
                                         NUM_ELEM(sourceAttrs));

    
    if (crv == CKR_OK)
        crv = sftk_Attribute2SecItem(arena, &peerID, key,
                                     CKA_NSS_JPAKE_PEERID);
    if (crv == CKR_OK && (peerID.data == NULL || peerID.len == 0))
        crv = CKR_TEMPLATE_INCOMPLETE;
    if (crv == CKR_OK && SECITEM_CompareItem(&signerID, &peerID) == SECEqual)
        crv = CKR_TEMPLATE_INCONSISTENT;

    
    if (crv == CKR_OK)
        crv = jpake_Verify(arena, &pqg, hashType, &signerID,
                           peerID.data, peerID.len, &params->gx3);
    if (crv == CKR_OK)
        crv = jpake_Verify(arena, &pqg, hashType, &signerID,
                           peerID.data, peerID.len, &params->gx4);

    
    if (crv == CKR_OK) {
        SECItem s;
        s.data = params->pSharedKey;
        s.len = params->ulSharedKeyLen;
        gx3.data = params->gx3.pGX;
        gx3.len = params->gx3.ulGXLen;
        gx4.data = params->gx4.pGX;
        gx4.len = params->gx4.ulGXLen;
        pqg.base.data = NULL;
        x2s.data = NULL;
        crv = jpake_mapStatus(JPAKE_Round2(arena, &pqg.prime, &pqg.subPrime,
                                           &gx1, &gx3, &gx4, &pqg.base, 
                                           &x2, &s, &x2s),
                              CKR_MECHANISM_PARAM_INVALID);
    }

    
    if (crv == CKR_OK)
        crv = jpake_Sign(arena, &pqg, hashType, &signerID, &x2s, &params->A);

    

    if (crv == CKR_OK)
        crv = sftk_forceAttribute(key, CKA_PRIME, pqg.prime.data,
                                  pqg.prime.len);
    if (crv == CKR_OK)
        crv = sftk_forceAttribute(key, CKA_SUBPRIME, pqg.subPrime.data,
                                  pqg.subPrime.len);
    if (crv == CKR_OK) {
        crv = jpake_MultipleSecItem2Attribute(key, copiedAndGeneratedAttrs,
                                              NUM_ELEM(copiedAndGeneratedAttrs));
    }

    if (crv == CKR_OK)
        crv = jpake_enforceKeyType(key, CKK_NSS_JPAKE_ROUND2);

    PORT_FreeArena(arena, PR_TRUE);
    return crv;
}

CK_RV
jpake_Final(HASH_HashType hashType, const CK_NSS_JPAKEFinalParams * param,
            SFTKObject * sourceKey, SFTKObject * key)
{
    PLArenaPool * arena;
    SECItem K;
    PQGParams pqg;
    CK_RV crv;
    SECItem peerID, signerID, x2s, x2, gx1, gx2, gx3, gx4;
    SFTKItemTemplate sourceAttrs[] = {
        { CKA_NSS_JPAKE_PEERID, &peerID },
        { CKA_NSS_JPAKE_SIGNERID, &signerID },
        { CKA_PRIME, &pqg.prime },
        { CKA_SUBPRIME, &pqg.subPrime },
        { CKA_NSS_JPAKE_X2,  &x2  },
        { CKA_NSS_JPAKE_X2S, &x2s },
        { CKA_NSS_JPAKE_GX1, &gx1 },
        { CKA_NSS_JPAKE_GX2, &gx2 },
        { CKA_NSS_JPAKE_GX3, &gx3 },
        { CKA_NSS_JPAKE_GX4, &gx4 }
    };

    PORT_Assert(param != NULL);
    PORT_Assert(sourceKey != NULL);
    PORT_Assert(key != NULL);

    arena = PORT_NewArena(NSS_SOFTOKEN_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
        crv = CKR_HOST_MEMORY;
    
    

    crv = sftk_MultipleAttribute2SecItem(arena, sourceKey, sourceAttrs,
                                         NUM_ELEM(sourceAttrs));

    
    if (crv == CKR_OK) {
        pqg.base.data = NULL;
        crv = jpake_mapStatus(JPAKE_Round2(arena, &pqg.prime, &pqg.subPrime,
                                           &gx1, &gx2, &gx3, &pqg.base,
                                           NULL, NULL, NULL),
                              CKR_MECHANISM_PARAM_INVALID);
    }

    
    if (crv == CKR_OK)
        crv = jpake_Verify(arena, &pqg, hashType, &signerID,
                           peerID.data, peerID.len, &param->B);
    if (crv == CKR_OK) {
        SECItem B;
        B.data = param->B.pGX;
        B.len = param->B.ulGXLen;
        K.data = NULL;
        crv = jpake_mapStatus(JPAKE_Final(arena, &pqg.prime, &pqg.subPrime,
                                          &x2, &gx4, &x2s, &B, &K),
                              CKR_MECHANISM_PARAM_INVALID);
    }

    
    if (crv == CKR_OK)
        crv = sftk_forceAttribute(key, CKA_VALUE, K.data, K.len);

    if (crv == CKR_OK)
        crv = jpake_enforceKeyType(key, CKK_GENERIC_SECRET);

    PORT_FreeArena(arena, PR_TRUE);
    return crv;
}
