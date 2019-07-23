



































#include "p12t.h"
#include "p12.h"
#include "plarena.h"
#include "secitem.h"
#include "secoid.h"
#include "seccomon.h"
#include "secport.h"
#include "cert.h"
#include "secpkcs7.h"
#include "secasn1.h"
#include "secerr.h"
#include "pk11func.h"
#include "p12plcy.h"
#include "p12local.h"
#include "prcpucfg.h"























#define PK12_OUTPUT_BUFFER_SIZE  8192

struct sec_pkcs12OutputBufferStr {
    SEC_PKCS7EncoderContext * p7eCx;
    PK11Context             * hmacCx;
    unsigned int              numBytes;
    unsigned int              bufBytes;
             char             buf[PK12_OUTPUT_BUFFER_SIZE];
};
typedef struct sec_pkcs12OutputBufferStr sec_pkcs12OutputBuffer;









struct SEC_PKCS12SafeInfoStr {
    PRArenaPool *arena;

    
    SECItem pwitem;
    SECOidTag algorithm;
    PK11SymKey *encryptionKey;

    



    unsigned int itemCount;

    
    SEC_PKCS7ContentInfo *cinfo;

    sec_PKCS12SafeContents *safe;
};




struct SEC_PKCS12ExportContextStr {
    PRArenaPool *arena;
    PK11SlotInfo *slot;
    void *wincx;

    
    PRBool integrityEnabled;
    PRBool	pwdIntegrity;
    union {
	struct sec_PKCS12PasswordModeInfo pwdInfo;
	struct sec_PKCS12PublicKeyModeInfo pubkeyInfo;
    } integrityInfo; 

    
    
    SECKEYGetPasswordKey pwfn;
    void *pwfnarg;

    
    SEC_PKCS12SafeInfo **safeInfos;
    unsigned int safeInfoCount;

    
    sec_PKCS12AuthenticatedSafe authSafe;

    
    CERTCertificate **certList;
};




struct sec_pkcs12_encoder_output {
    SEC_PKCS12EncoderOutputCallback outputfn;
    void *outputarg;
};

struct sec_pkcs12_hmac_and_output_info {
    void *arg;
    struct sec_pkcs12_encoder_output output;
};




typedef struct sec_PKCS12EncoderContextStr {
    PRArenaPool *arena;
    SEC_PKCS12ExportContext *p12exp;
    PK11SymKey *encryptionKey;

    


    SEC_ASN1EncoderContext *outerA1ecx;
    union {
	struct sec_pkcs12_hmac_and_output_info hmacAndOutputInfo;
	struct sec_pkcs12_encoder_output       encOutput;
    } output;

    
    sec_PKCS12PFXItem        pfx;
    sec_PKCS12MacData        mac;

    
    SEC_PKCS7ContentInfo    *aSafeCinfo;
    SEC_PKCS7EncoderContext *middleP7ecx;
    SEC_ASN1EncoderContext  *middleA1ecx;
    unsigned int             currentSafe;

    
    PK11Context             *hmacCx;

    
    sec_pkcs12OutputBuffer  middleBuf;
    sec_pkcs12OutputBuffer  innerBuf;

} sec_PKCS12EncoderContext;














SEC_PKCS12ExportContext *
SEC_PKCS12CreateExportContext(SECKEYGetPasswordKey pwfn, void *pwfnarg,  
			      PK11SlotInfo *slot, void *wincx)
{
    PRArenaPool *arena = NULL;
    SEC_PKCS12ExportContext *p12ctxt = NULL;

    
    arena = PORT_NewArena(4096);
    if(!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    p12ctxt = (SEC_PKCS12ExportContext *)PORT_ArenaZAlloc(arena, 
					sizeof(SEC_PKCS12ExportContext));
    if(!p12ctxt) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    p12ctxt->pwfn = pwfn;
    p12ctxt->pwfnarg = pwfnarg;

    p12ctxt->integrityEnabled = PR_FALSE;
    p12ctxt->arena = arena;
    p12ctxt->wincx = wincx;
    p12ctxt->slot = (slot) ? PK11_ReferenceSlot(slot) : PK11_GetInternalSlot();

    return p12ctxt;

loser:
    if(arena) {
	PORT_FreeArena(arena, PR_TRUE);
    }

    return NULL;
}













SECStatus
SEC_PKCS12AddPasswordIntegrity(SEC_PKCS12ExportContext *p12ctxt,
			       SECItem *pwitem, SECOidTag integAlg) 
{			       
    if(!p12ctxt || p12ctxt->integrityEnabled) {
	return SECFailure;
    }
   
    
    p12ctxt->pwdIntegrity = PR_TRUE;
    p12ctxt->integrityInfo.pwdInfo.password = 
        (SECItem*)PORT_ArenaZAlloc(p12ctxt->arena, sizeof(SECItem));
    if(!p12ctxt->integrityInfo.pwdInfo.password) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    if(SECITEM_CopyItem(p12ctxt->arena, 
			p12ctxt->integrityInfo.pwdInfo.password, pwitem)
		!= SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    p12ctxt->integrityInfo.pwdInfo.algorithm = integAlg;
    p12ctxt->integrityEnabled = PR_TRUE;

    return SECSuccess;
}












SECStatus
SEC_PKCS12AddPublicKeyIntegrity(SEC_PKCS12ExportContext *p12ctxt,
				CERTCertificate *cert, CERTCertDBHandle *certDb,
				SECOidTag algorithm, int keySize)
{
    if(!p12ctxt) {
	return SECFailure;
    }
    
    p12ctxt->integrityInfo.pubkeyInfo.cert = cert;
    p12ctxt->integrityInfo.pubkeyInfo.certDb = certDb;
    p12ctxt->integrityInfo.pubkeyInfo.algorithm = algorithm;
    p12ctxt->integrityInfo.pubkeyInfo.keySize = keySize;
    p12ctxt->integrityEnabled = PR_TRUE;

    return SECSuccess;
}












static SECStatus
sec_pkcs12_append_safe_info(SEC_PKCS12ExportContext *p12ctxt, SEC_PKCS12SafeInfo *info)
{
    void *mark = NULL, *dummy1 = NULL, *dummy2 = NULL;

    if(!p12ctxt || !info) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);

    
    if(!p12ctxt->safeInfoCount) {
	p12ctxt->safeInfos = (SEC_PKCS12SafeInfo **)PORT_ArenaZAlloc(p12ctxt->arena, 
					      2 * sizeof(SEC_PKCS12SafeInfo *));
	dummy1 = p12ctxt->safeInfos;
	p12ctxt->authSafe.encodedSafes = (SECItem **)PORT_ArenaZAlloc(p12ctxt->arena, 
					2 * sizeof(SECItem *));
	dummy2 = p12ctxt->authSafe.encodedSafes;
    } else {
	dummy1 = PORT_ArenaGrow(p12ctxt->arena, p12ctxt->safeInfos, 
			       (p12ctxt->safeInfoCount + 1) * sizeof(SEC_PKCS12SafeInfo *),
			       (p12ctxt->safeInfoCount + 2) * sizeof(SEC_PKCS12SafeInfo *));
	p12ctxt->safeInfos = (SEC_PKCS12SafeInfo **)dummy1;
	dummy2 = PORT_ArenaGrow(p12ctxt->arena, p12ctxt->authSafe.encodedSafes, 
			       (p12ctxt->authSafe.safeCount + 1) * sizeof(SECItem *),
			       (p12ctxt->authSafe.safeCount + 2) * sizeof(SECItem *));
	p12ctxt->authSafe.encodedSafes = (SECItem**)dummy2;
    }
    if(!dummy1 || !dummy2) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    p12ctxt->safeInfos[p12ctxt->safeInfoCount] = info;
    p12ctxt->safeInfos[++p12ctxt->safeInfoCount] = NULL;
    p12ctxt->authSafe.encodedSafes[p12ctxt->authSafe.safeCount] = 
        (SECItem*)PORT_ArenaZAlloc(p12ctxt->arena, sizeof(SECItem));
    if(!p12ctxt->authSafe.encodedSafes[p12ctxt->authSafe.safeCount]) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    p12ctxt->authSafe.encodedSafes[++p12ctxt->authSafe.safeCount] = NULL;

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(p12ctxt->arena, mark);
    return SECFailure;
}








SEC_PKCS12SafeInfo *
SEC_PKCS12CreatePasswordPrivSafe(SEC_PKCS12ExportContext *p12ctxt, 
				 SECItem *pwitem, SECOidTag privAlg)
{
    SEC_PKCS12SafeInfo *safeInfo = NULL;
    void *mark = NULL;
    PK11SlotInfo *slot = NULL;
    SECAlgorithmID *algId;
    SECItem uniPwitem = {siBuffer, NULL, 0};

    if(!p12ctxt) {
	return NULL;
    }

    
    mark = PORT_ArenaMark(p12ctxt->arena);
    safeInfo = (SEC_PKCS12SafeInfo *)PORT_ArenaZAlloc(p12ctxt->arena, 
    						sizeof(SEC_PKCS12SafeInfo));
    if(!safeInfo) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_ArenaRelease(p12ctxt->arena, mark);
	return NULL;
    }

    safeInfo->itemCount = 0;

    
    safeInfo->cinfo = SEC_PKCS7CreateEncryptedData(privAlg, 0, p12ctxt->pwfn, 
    						   p12ctxt->pwfnarg);
    if(!safeInfo->cinfo) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    safeInfo->arena = p12ctxt->arena;

     
    if(!sec_pkcs12_convert_item_to_unicode(NULL, &uniPwitem, pwitem,
					       PR_TRUE, PR_TRUE, PR_TRUE)) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    if(SECITEM_CopyItem(p12ctxt->arena, &safeInfo->pwitem, &uniPwitem) != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    slot = PK11_ReferenceSlot(p12ctxt->slot);
    if(!slot) {
	slot = PK11_GetInternalKeySlot();
	if(!slot) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
    }

    algId = SEC_PKCS7GetEncryptionAlgorithm(safeInfo->cinfo);
    safeInfo->encryptionKey = PK11_PBEKeyGen(slot, algId, &uniPwitem, 
					     PR_FALSE, p12ctxt->wincx);
    if(!safeInfo->encryptionKey) {
	goto loser;
    }

    safeInfo->arena = p12ctxt->arena;
    safeInfo->safe = NULL;
    if(sec_pkcs12_append_safe_info(p12ctxt, safeInfo) != SECSuccess) {
	goto loser;
    }

    if(uniPwitem.data) {
	SECITEM_ZfreeItem(&uniPwitem, PR_FALSE);
    }
    PORT_ArenaUnmark(p12ctxt->arena, mark);

    if (slot) {
	PK11_FreeSlot(slot);
    }
    return safeInfo;

loser:
    if (slot) {
	PK11_FreeSlot(slot);
    }
    if(safeInfo->cinfo) {
	SEC_PKCS7DestroyContentInfo(safeInfo->cinfo);
    }

    if(uniPwitem.data) {
	SECITEM_ZfreeItem(&uniPwitem, PR_FALSE);
    }

    PORT_ArenaRelease(p12ctxt->arena, mark);
    return NULL;
}






SEC_PKCS12SafeInfo *
SEC_PKCS12CreateUnencryptedSafe(SEC_PKCS12ExportContext *p12ctxt)
{
    SEC_PKCS12SafeInfo *safeInfo = NULL;
    void *mark = NULL;

    if(!p12ctxt) {
	return NULL;
    }

    
    mark = PORT_ArenaMark(p12ctxt->arena);
    safeInfo = (SEC_PKCS12SafeInfo *)PORT_ArenaZAlloc(p12ctxt->arena, 
    					      sizeof(SEC_PKCS12SafeInfo));
    if(!safeInfo) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    safeInfo->itemCount = 0;

    
    safeInfo->cinfo = SEC_PKCS7CreateData();
    if(!safeInfo->cinfo) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    if(sec_pkcs12_append_safe_info(p12ctxt, safeInfo) != SECSuccess) {
	goto loser;
    }

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return safeInfo;

loser:
    if(safeInfo->cinfo) {
	SEC_PKCS7DestroyContentInfo(safeInfo->cinfo);
    }

    PORT_ArenaRelease(p12ctxt->arena, mark);
    return NULL;
}











SEC_PKCS12SafeInfo *
SEC_PKCS12CreatePubKeyEncryptedSafe(SEC_PKCS12ExportContext *p12ctxt,
				    CERTCertDBHandle *certDb,
				    CERTCertificate *signer,
				    CERTCertificate **recipients,
				    SECOidTag algorithm, int keysize) 
{
    SEC_PKCS12SafeInfo *safeInfo = NULL;
    void *mark = NULL;

    if(!p12ctxt || !signer || !recipients || !(*recipients)) {
	return NULL;
    }

    
    mark = PORT_ArenaMark(p12ctxt->arena);
    safeInfo = (SEC_PKCS12SafeInfo *)PORT_ArenaZAlloc(p12ctxt->arena, 
    						      sizeof(SEC_PKCS12SafeInfo));
    if(!safeInfo) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    safeInfo->itemCount = 0;
    safeInfo->arena = p12ctxt->arena;

    


    safeInfo->cinfo = SEC_PKCS7CreateEnvelopedData(signer, certUsageEmailSigner,
					certDb, algorithm, keysize, 
					p12ctxt->pwfn, p12ctxt->pwfnarg);
    if(!safeInfo->cinfo) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    if(recipients) {
	unsigned int i = 0;
	while(recipients[i] != NULL) {
	    SECStatus rv = SEC_PKCS7AddRecipient(safeInfo->cinfo, recipients[i],
					       certUsageEmailRecipient, certDb);
	    if(rv != SECSuccess) {
		goto loser;
	    }
	    i++;
	}
    }

    if(sec_pkcs12_append_safe_info(p12ctxt, safeInfo) != SECSuccess) {
	goto loser;
    }

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return safeInfo;

loser:
    if(safeInfo->cinfo) {
	SEC_PKCS7DestroyContentInfo(safeInfo->cinfo);
	safeInfo->cinfo = NULL;
    }

    PORT_ArenaRelease(p12ctxt->arena, mark);
    return NULL;
} 






sec_PKCS12SafeContents *
sec_PKCS12CreateSafeContents(PRArenaPool *arena)
{
    sec_PKCS12SafeContents *safeContents;

    if(arena == NULL) {
	return NULL; 
    }

    
    safeContents = (sec_PKCS12SafeContents *)PORT_ArenaZAlloc(arena,
					    sizeof(sec_PKCS12SafeContents));
    if(!safeContents) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    safeContents->safeBags = NULL;
    safeContents->arena = arena;
    safeContents->bagCount = 0;

    return safeContents;

loser:
    return NULL;
}   



SECStatus
sec_pkcs12_append_bag_to_safe_contents(PRArenaPool *arena, 
				       sec_PKCS12SafeContents *safeContents,
				       sec_PKCS12SafeBag *safeBag)
{
    void *mark = NULL, *dummy = NULL;

    if(!arena || !safeBag || !safeContents) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(arena);
    if(!mark) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }

    
    if(!safeContents->safeBags) {
	safeContents->safeBags = (sec_PKCS12SafeBag **)PORT_ArenaZAlloc(arena, 
						(2 * sizeof(sec_PKCS12SafeBag *)));
	dummy = safeContents->safeBags;
	safeContents->bagCount = 0;
    } else {
	dummy = PORT_ArenaGrow(arena, safeContents->safeBags, 
			(safeContents->bagCount + 1) * sizeof(sec_PKCS12SafeBag *),
			(safeContents->bagCount + 2) * sizeof(sec_PKCS12SafeBag *));
	safeContents->safeBags = (sec_PKCS12SafeBag **)dummy;
    }

    if(!dummy) {
	PORT_ArenaRelease(arena, mark);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }

    
    safeContents->safeBags[safeContents->bagCount++] = safeBag;
    safeContents->safeBags[safeContents->bagCount] = NULL;

    PORT_ArenaUnmark(arena, mark);

    return SECSuccess;
}



SECStatus
sec_pkcs12_append_bag(SEC_PKCS12ExportContext *p12ctxt, 
		      SEC_PKCS12SafeInfo *safeInfo, sec_PKCS12SafeBag *safeBag)
{
    sec_PKCS12SafeContents *dest;
    SECStatus rv = SECFailure;

    if(!p12ctxt || !safeBag || !safeInfo) {
	return SECFailure;
    }

    if(!safeInfo->safe) {
	safeInfo->safe = sec_PKCS12CreateSafeContents(p12ctxt->arena);
	if(!safeInfo->safe) {
	    return SECFailure;
	}
    }

    dest = safeInfo->safe;
    rv = sec_pkcs12_append_bag_to_safe_contents(p12ctxt->arena, dest, safeBag);
    if(rv == SECSuccess) {
	safeInfo->itemCount++;
    }
    
    return rv;
} 





sec_PKCS12SafeBag *
sec_PKCS12CreateSafeBag(SEC_PKCS12ExportContext *p12ctxt, SECOidTag bagType, 
			void *bagData)
{
    sec_PKCS12SafeBag *safeBag;
    PRBool setName = PR_TRUE;
    void *mark = NULL;
    SECStatus rv = SECSuccess;
    SECOidData *oidData = NULL;

    if(!p12ctxt) {
	return NULL;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);
    if(!mark) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    safeBag = (sec_PKCS12SafeBag *)PORT_ArenaZAlloc(p12ctxt->arena, 
    						    sizeof(sec_PKCS12SafeBag));
    if(!safeBag) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    
    switch(bagType) {
	case SEC_OID_PKCS12_V1_KEY_BAG_ID:
	    safeBag->safeBagContent.pkcs8KeyBag =
	        (SECKEYPrivateKeyInfo *)bagData;
	    break;
	case SEC_OID_PKCS12_V1_CERT_BAG_ID:
	    safeBag->safeBagContent.certBag = (sec_PKCS12CertBag *)bagData;
	    break;
	case SEC_OID_PKCS12_V1_CRL_BAG_ID:
	    safeBag->safeBagContent.crlBag = (sec_PKCS12CRLBag *)bagData;
	    break;
	case SEC_OID_PKCS12_V1_SECRET_BAG_ID:
	    safeBag->safeBagContent.secretBag = (sec_PKCS12SecretBag *)bagData;
	    break;
	case SEC_OID_PKCS12_V1_PKCS8_SHROUDED_KEY_BAG_ID:
	    safeBag->safeBagContent.pkcs8ShroudedKeyBag = 
	        (SECKEYEncryptedPrivateKeyInfo *)bagData;
	    break;
	case SEC_OID_PKCS12_V1_SAFE_CONTENTS_BAG_ID:
	    safeBag->safeBagContent.safeContents = 
	        (sec_PKCS12SafeContents *)bagData;
	    setName = PR_FALSE;
	    break;
	default:
	    goto loser;
    }

    oidData = SECOID_FindOIDByTag(bagType);
    if(oidData) {
	rv = SECITEM_CopyItem(p12ctxt->arena, &safeBag->safeBagType, &oidData->oid);
	if(rv != SECSuccess) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
    } else {
	goto loser;
    }
    
    safeBag->arena = p12ctxt->arena;
    PORT_ArenaUnmark(p12ctxt->arena, mark);

    return safeBag;

loser:
    if(mark) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
    }

    return NULL;
}




sec_PKCS12CertBag *
sec_PKCS12NewCertBag(PRArenaPool *arena, SECOidTag certType)
{
    sec_PKCS12CertBag *certBag = NULL;
    SECOidData *bagType = NULL;
    SECStatus rv;
    void *mark = NULL;

    if(!arena) {
	return NULL;
    }

    mark = PORT_ArenaMark(arena);
    certBag = (sec_PKCS12CertBag *)PORT_ArenaZAlloc(arena, 
    						    sizeof(sec_PKCS12CertBag));
    if(!certBag) {
	PORT_ArenaRelease(arena, mark);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    bagType = SECOID_FindOIDByTag(certType);
    if(!bagType) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    rv = SECITEM_CopyItem(arena, &certBag->bagID, &bagType->oid);
    if(rv != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
	
    PORT_ArenaUnmark(arena, mark);
    return certBag;

loser:
    PORT_ArenaRelease(arena, mark);
    return NULL;
}




sec_PKCS12CRLBag *
sec_PKCS12NewCRLBag(PRArenaPool *arena, SECOidTag crlType)
{
    sec_PKCS12CRLBag *crlBag = NULL;
    SECOidData *bagType = NULL;
    SECStatus rv;
    void *mark = NULL;

    if(!arena) {
	return NULL;
    }

    mark = PORT_ArenaMark(arena);
    crlBag = (sec_PKCS12CRLBag *)PORT_ArenaZAlloc(arena, 
    						  sizeof(sec_PKCS12CRLBag));
    if(!crlBag) {
	PORT_ArenaRelease(arena, mark);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    bagType = SECOID_FindOIDByTag(crlType);
    if(!bagType) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    rv = SECITEM_CopyItem(arena, &crlBag->bagID, &bagType->oid);
    if(rv != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
	
    PORT_ArenaUnmark(arena, mark);
    return crlBag;

loser:
    PORT_ArenaRelease(arena, mark);
    return NULL;
}










SECStatus
sec_PKCS12AddAttributeToBag(SEC_PKCS12ExportContext *p12ctxt, 
			    sec_PKCS12SafeBag *safeBag, SECOidTag attrType,
			    SECItem *attrData)
{
    sec_PKCS12Attribute *attribute;
    void *mark = NULL, *dummy = NULL;
    SECOidData *oiddata = NULL;
    SECItem unicodeName = { siBuffer, NULL, 0};
    void *src = NULL;
    unsigned int nItems = 0;
    SECStatus rv;

    if(!safeBag || !p12ctxt) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(safeBag->arena);

    
    attribute = (sec_PKCS12Attribute *)PORT_ArenaZAlloc(safeBag->arena, 
    						sizeof(sec_PKCS12Attribute));
    if(!attribute) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    oiddata = SECOID_FindOIDByTag(attrType);
    if(!oiddata) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    if(SECITEM_CopyItem(p12ctxt->arena, &attribute->attrType, &oiddata->oid) !=
    		SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    nItems = 1;
    switch(attrType) {
	case SEC_OID_PKCS9_LOCAL_KEY_ID:
	    {
		src = attrData;
		break;
	    }
	case SEC_OID_PKCS9_FRIENDLY_NAME:
	    {
		if(!sec_pkcs12_convert_item_to_unicode(p12ctxt->arena, 
					&unicodeName, attrData, PR_FALSE, 
					PR_FALSE, PR_TRUE)) {
		    goto loser;
		}
		src = &unicodeName;
		break;
	    }
	default:
	    goto loser;
    }

    
    attribute->attrValue = (SECItem **)PORT_ArenaZAlloc(p12ctxt->arena, 
    					    ((nItems + 1) * sizeof(SECItem *)));
    if(!attribute->attrValue) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    


    attribute->attrValue[0] = (SECItem *)PORT_ArenaZAlloc(p12ctxt->arena, 
    							  sizeof(SECItem));
    if(!attribute->attrValue[0]) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }
    attribute->attrValue[1] = NULL;

    rv = SECITEM_CopyItem(p12ctxt->arena, attribute->attrValue[0], 
			  (SECItem*)src);
    if(rv != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    if(safeBag->nAttribs) {
	dummy = PORT_ArenaGrow(p12ctxt->arena, safeBag->attribs, 
			((safeBag->nAttribs + 1) * sizeof(sec_PKCS12Attribute *)),
			((safeBag->nAttribs + 2) * sizeof(sec_PKCS12Attribute *)));
	safeBag->attribs = (sec_PKCS12Attribute **)dummy;
    } else {
	safeBag->attribs = (sec_PKCS12Attribute **)PORT_ArenaZAlloc(p12ctxt->arena, 
						2 * sizeof(sec_PKCS12Attribute *));
	dummy = safeBag->attribs;
    }
    if(!dummy) {
	goto loser;
    }

    safeBag->attribs[safeBag->nAttribs] = attribute;
    safeBag->attribs[++safeBag->nAttribs] = NULL;

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return SECSuccess;

loser:
    if(mark) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
    }

    return SECFailure;
}













SECStatus
SEC_PKCS12AddCert(SEC_PKCS12ExportContext *p12ctxt, SEC_PKCS12SafeInfo *safe, 
		  void *nestedDest, CERTCertificate *cert, 
		  CERTCertDBHandle *certDb, SECItem *keyId,
		  PRBool includeCertChain)
{
    sec_PKCS12CertBag *certBag;
    sec_PKCS12SafeBag *safeBag;
    void *mark;
    SECStatus rv;
    SECItem nick = {siBuffer, NULL,0};

    if(!p12ctxt || !cert) {
	return SECFailure;
    }
    mark = PORT_ArenaMark(p12ctxt->arena);

    
    certBag = sec_PKCS12NewCertBag(p12ctxt->arena, 
    				   SEC_OID_PKCS9_X509_CERT);
    if(!certBag) {
	goto loser;
    }

    if(SECITEM_CopyItem(p12ctxt->arena, &certBag->value.x509Cert, 
    			&cert->derCert) != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    


    if(includeCertChain) {
	CERTCertificateList *certList = CERT_CertChainFromCert(cert,
							       certUsageSSLClient,
							       PR_TRUE);
	unsigned int count = 0;
	if(!certList) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}

	
	for(count = 0; count < (unsigned int)certList->len; count++) {
	    if(SECITEM_CompareItem(&certList->certs[count], &cert->derCert)
	    			!= SECEqual) {
	    	CERTCertificate *tempCert;

		
		









		tempCert = CERT_FindCertByDERCert(CERT_GetDefaultCertDB(),
		                                  &certList->certs[count]);
	    	if(!tempCert) {
		    CERT_DestroyCertificateList(certList);
		    goto loser;
		}

		
	    	if(SEC_PKCS12AddCert(p12ctxt, safe, nestedDest, tempCert,
				 certDb, NULL, PR_FALSE) != SECSuccess) {
		    CERT_DestroyCertificate(tempCert);
		    CERT_DestroyCertificateList(certList);
		    goto loser;
		}
		CERT_DestroyCertificate(tempCert);
	    }
	}
	CERT_DestroyCertificateList(certList);
    }

    


    if(cert->nickname) {
        if (cert->slot && !PK11_IsInternal(cert->slot)) {
	  





	    char *delimit;
	    
	    delimit = PORT_Strchr(cert->nickname,':');
	    if (delimit == NULL) {
	        nick.data = (unsigned char *)cert->nickname;
		nick.len = PORT_Strlen(cert->nickname);
	    } else {
	        delimit++;
	        nick.data = (unsigned char *)PORT_ArenaStrdup(p12ctxt->arena,
							      delimit);
		nick.len = PORT_Strlen(delimit);
	    }
	} else {
	    nick.data = (unsigned char *)cert->nickname;
	    nick.len = PORT_Strlen(cert->nickname);
	}
    }

    safeBag = sec_PKCS12CreateSafeBag(p12ctxt, SEC_OID_PKCS12_V1_CERT_BAG_ID, 
    				      certBag);
    if(!safeBag) {
	goto loser;
    }

    
    if(nick.data) {
	if(sec_PKCS12AddAttributeToBag(p12ctxt, safeBag, 
				       SEC_OID_PKCS9_FRIENDLY_NAME, &nick) 
				       != SECSuccess) {
	    goto loser;
	}
    }
	   
    if(keyId) {
	if(sec_PKCS12AddAttributeToBag(p12ctxt, safeBag, SEC_OID_PKCS9_LOCAL_KEY_ID,
				       keyId) != SECSuccess) {
	    goto loser;
	}
    }

    
    if(nestedDest) {
	rv = sec_pkcs12_append_bag_to_safe_contents(p12ctxt->arena, 
					  (sec_PKCS12SafeContents*)nestedDest, 
					   safeBag);
    } else {
	rv = sec_pkcs12_append_bag(p12ctxt, safe, safeBag);
    }

    if(rv != SECSuccess) {
	goto loser;
    }

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return SECSuccess;

loser:
    if(mark) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
    }

    return SECFailure;
}

















SECStatus
SEC_PKCS12AddKeyForCert(SEC_PKCS12ExportContext *p12ctxt, SEC_PKCS12SafeInfo *safe, 
			void *nestedDest, CERTCertificate *cert,
			PRBool shroudKey, SECOidTag algorithm, SECItem *pwitem,
			SECItem *keyId, SECItem *nickName)
{
    void *mark;
    void *keyItem;
    SECOidTag keyType;
    SECStatus rv = SECFailure;
    SECItem nickname = {siBuffer,NULL,0}, uniPwitem = {siBuffer, NULL, 0};
    sec_PKCS12SafeBag *returnBag;

    if(!p12ctxt || !cert || !safe) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);

    

	   
    if(!shroudKey) {

	
	SECKEYPrivateKeyInfo *pki = PK11_ExportPrivateKeyInfo(cert, 
							      p12ctxt->wincx);
	if(!pki) {
	    PORT_ArenaRelease(p12ctxt->arena, mark);
	    PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY);
	    return SECFailure;
	}   
	keyItem = PORT_ArenaZAlloc(p12ctxt->arena, sizeof(SECKEYPrivateKeyInfo));
	if(!keyItem) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	rv = SECKEY_CopyPrivateKeyInfo(p12ctxt->arena, 
				       (SECKEYPrivateKeyInfo *)keyItem, pki);
	keyType = SEC_OID_PKCS12_V1_KEY_BAG_ID;
	SECKEY_DestroyPrivateKeyInfo(pki, PR_TRUE);
    } else {

	
	SECKEYEncryptedPrivateKeyInfo *epki = NULL;
	PK11SlotInfo *slot = NULL;

	if(!sec_pkcs12_convert_item_to_unicode(p12ctxt->arena, &uniPwitem,
				 pwitem, PR_TRUE, PR_TRUE, PR_TRUE)) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}

	
	if(PK11_IsInternal(p12ctxt->slot)) {
	    slot = PK11_GetInternalKeySlot();
	} else {
	    slot = PK11_ReferenceSlot(p12ctxt->slot);
	}

	epki = PK11_ExportEncryptedPrivateKeyInfo(slot, algorithm, 
						  &uniPwitem, cert, 1, 
						  p12ctxt->wincx);
	PK11_FreeSlot(slot);
	if(!epki) {
	    PORT_SetError(SEC_ERROR_PKCS12_UNABLE_TO_EXPORT_KEY);
	    goto loser;
	}   
	
	keyItem = PORT_ArenaZAlloc(p12ctxt->arena, 
				  sizeof(SECKEYEncryptedPrivateKeyInfo));
	if(!keyItem) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}
	rv = SECKEY_CopyEncryptedPrivateKeyInfo(p12ctxt->arena, 
					(SECKEYEncryptedPrivateKeyInfo *)keyItem,
					epki);
	keyType = SEC_OID_PKCS12_V1_PKCS8_SHROUDED_KEY_BAG_ID;
	SECKEY_DestroyEncryptedPrivateKeyInfo(epki, PR_TRUE);
    }

    if(rv != SECSuccess) {
	goto loser;
    }
	
    

					  
    if(!nickName) {
	if(cert->nickname) {
	    nickname.data = (unsigned char *)cert->nickname;
	    nickname.len = PORT_Strlen(cert->nickname);
	    nickName = &nickname;
	}
    }

    
    returnBag = sec_PKCS12CreateSafeBag(p12ctxt, keyType, keyItem);
    if(!returnBag) {
	rv = SECFailure;
	goto loser;
    }

    if(nickName) {
	if(sec_PKCS12AddAttributeToBag(p12ctxt, returnBag, 
				       SEC_OID_PKCS9_FRIENDLY_NAME, nickName) 
				       != SECSuccess) {
	    goto loser;
	}
    }
	   
    if(keyId) {
	if(sec_PKCS12AddAttributeToBag(p12ctxt, returnBag, SEC_OID_PKCS9_LOCAL_KEY_ID,
				       keyId) != SECSuccess) {
	    goto loser;
	}
    }

    if(nestedDest) {
	rv = sec_pkcs12_append_bag_to_safe_contents(p12ctxt->arena,
					  (sec_PKCS12SafeContents*)nestedDest, 
					  returnBag);
    } else {
	rv = sec_pkcs12_append_bag(p12ctxt, safe, returnBag);
    }

loser:

    if (rv != SECSuccess) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
    } else {
	PORT_ArenaUnmark(p12ctxt->arena, mark);
    }

    return rv;
}














SECStatus
SEC_PKCS12AddCertOrChainAndKey(SEC_PKCS12ExportContext *p12ctxt, 
			       void *certSafe, void *certNestedDest, 
			       CERTCertificate *cert, CERTCertDBHandle *certDb,
			       void *keySafe, void *keyNestedDest, 
			       PRBool shroudKey, SECItem *pwitem, 
			       SECOidTag algorithm, PRBool includeCertChain)
{
    SECStatus rv = SECFailure;
    SGNDigestInfo *digest = NULL;
    void *mark = NULL;

    if(!p12ctxt || !certSafe || !keySafe || !cert) {
	return SECFailure;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);

    
    digest = sec_pkcs12_compute_thumbprint(&cert->derCert);
    if(!digest) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
	return SECFailure;
    }

    
    rv = SEC_PKCS12AddCert(p12ctxt, (SEC_PKCS12SafeInfo*)certSafe, 
			   (SEC_PKCS12SafeInfo*)certNestedDest, cert, certDb,
    			   &digest->digest, includeCertChain);
    if(rv != SECSuccess) {
	goto loser;
    }

    
    rv = SEC_PKCS12AddKeyForCert(p12ctxt, (SEC_PKCS12SafeInfo*)keySafe, 
				 keyNestedDest, cert, 
    				 shroudKey, algorithm, pwitem, 
    				 &digest->digest, NULL );
    if(rv != SECSuccess) {
	goto loser;
    }

    SGN_DestroyDigestInfo(digest);

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return SECSuccess;

loser:
    SGN_DestroyDigestInfo(digest);
    PORT_ArenaRelease(p12ctxt->arena, mark);
    
    return SECFailure; 
}


SECStatus
SEC_PKCS12AddCertAndKey(SEC_PKCS12ExportContext *p12ctxt, 
			void *certSafe, void *certNestedDest, 
			CERTCertificate *cert, CERTCertDBHandle *certDb,
			void *keySafe, void *keyNestedDest, 
			PRBool shroudKey, SECItem *pwItem, SECOidTag algorithm)
{
    return SEC_PKCS12AddCertOrChainAndKey(p12ctxt, certSafe, certNestedDest,
    		cert, certDb, keySafe, keyNestedDest, shroudKey, pwItem, 
		algorithm, PR_TRUE);
}










void *
SEC_PKCS12CreateNestedSafeContents(SEC_PKCS12ExportContext *p12ctxt,
				   void *baseSafe, void *nestedDest)
{
    sec_PKCS12SafeContents *newSafe;
    sec_PKCS12SafeBag *safeContentsBag;
    void *mark;
    SECStatus rv;

    if(!p12ctxt || !baseSafe) {
	return NULL;
    }

    mark = PORT_ArenaMark(p12ctxt->arena);

    newSafe = sec_PKCS12CreateSafeContents(p12ctxt->arena);
    if(!newSafe) {
	PORT_ArenaRelease(p12ctxt->arena, mark);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    
    safeContentsBag = sec_PKCS12CreateSafeBag(p12ctxt, 
					SEC_OID_PKCS12_V1_SAFE_CONTENTS_BAG_ID,
					newSafe);
    if(!safeContentsBag) {
	goto loser;
    }

    
    if(nestedDest) {
	rv = sec_pkcs12_append_bag_to_safe_contents(p12ctxt->arena, 
					   (sec_PKCS12SafeContents*)nestedDest,
					   safeContentsBag);
    } else {
	rv = sec_pkcs12_append_bag(p12ctxt, (SEC_PKCS12SafeInfo*)baseSafe, 
				   safeContentsBag);
    }
    if(rv != SECSuccess) {
	goto loser;
    }

    PORT_ArenaUnmark(p12ctxt->arena, mark);
    return newSafe;

loser:
    PORT_ArenaRelease(p12ctxt->arena, mark);
    return NULL;
}









sec_PKCS12EncoderContext *
sec_pkcs12_encoder_start_context(SEC_PKCS12ExportContext *p12exp)
{
    sec_PKCS12EncoderContext *p12enc = NULL;
    unsigned int i, nonEmptyCnt;
    SECStatus rv;
    SECItem ignore = {0};
    void *mark;

    if(!p12exp || !p12exp->safeInfos) {
	return NULL;
    }

    
    i = nonEmptyCnt = 0;
    while(p12exp->safeInfos[i]) {
	if(p12exp->safeInfos[i]->itemCount) {
	    nonEmptyCnt++;
	}
	i++;
    }
    if(nonEmptyCnt == 0) {
	return NULL;
    }
    p12exp->authSafe.encodedSafes[nonEmptyCnt] = NULL;

    
    mark = PORT_ArenaMark(p12exp->arena);
    p12enc = PORT_ArenaZNew(p12exp->arena, sec_PKCS12EncoderContext);
    if(!p12enc) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    p12enc->arena = p12exp->arena;
    p12enc->p12exp = p12exp;

    
    PORT_Memset(&p12enc->pfx, 0, sizeof(sec_PKCS12PFXItem));
    if(!SEC_ASN1EncodeInteger(p12exp->arena, &(p12enc->pfx.version), 
    			      SEC_PKCS12_VERSION) ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    	goto loser;
    }

    




    if(p12exp->integrityEnabled && !p12exp->pwdIntegrity) {
	SECStatus rv;

	
	p12enc->aSafeCinfo = SEC_PKCS7CreateSignedData(
				p12exp->integrityInfo.pubkeyInfo.cert,
				certUsageEmailSigner,
				p12exp->integrityInfo.pubkeyInfo.certDb,
				p12exp->integrityInfo.pubkeyInfo.algorithm,
				NULL,
				p12exp->pwfn,
				p12exp->pwfnarg);
	if(!p12enc->aSafeCinfo) {
	    goto loser;
	}
	if(SEC_PKCS7IncludeCertChain(p12enc->aSafeCinfo,NULL) != SECSuccess) {
	    goto loser;
	}
	rv = SEC_PKCS7AddSigningTime(p12enc->aSafeCinfo);
	PORT_Assert(rv == SECSuccess);
    } else {
	p12enc->aSafeCinfo = SEC_PKCS7CreateData();

	
	if(p12exp->integrityEnabled) {
	    SECItem  pwd = {siBuffer,NULL, 0};
	    SECItem *salt = sec_pkcs12_generate_salt();
	    PK11SymKey *symKey;
	    SECItem *params;
	    CK_MECHANISM_TYPE integrityMechType;
	    CK_MECHANISM_TYPE hmacMechType;

	    
	    PORT_Memset(&p12enc->mac, 0, sizeof(sec_PKCS12MacData));

	    if(!salt) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    }
	    if(SECITEM_CopyItem(p12exp->arena, &(p12enc->mac.macSalt), salt) 
			!= SECSuccess) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    }   

	    
	    if(!sec_pkcs12_convert_item_to_unicode(NULL, &pwd, 
			p12exp->integrityInfo.pwdInfo.password, PR_TRUE, 
			PR_TRUE, PR_TRUE)) {
		goto loser;
	    }
	    




	    params = PK11_CreatePBEParams(salt, &pwd, 1);
	    SECITEM_ZfreeItem(salt, PR_TRUE);
	    SECITEM_ZfreeItem(&pwd, PR_FALSE);

	    
	    switch (p12exp->integrityInfo.pwdInfo.algorithm) {
	    case SEC_OID_SHA1:
		integrityMechType = CKM_PBA_SHA1_WITH_SHA1_HMAC; break;
	    case SEC_OID_MD5:
		integrityMechType = CKM_NETSCAPE_PBE_MD5_HMAC_KEY_GEN;  break;
	    case SEC_OID_MD2:
		integrityMechType = CKM_NETSCAPE_PBE_MD2_HMAC_KEY_GEN;  break;
	    default:
		goto loser;
	    }

	    
	    symKey = PK11_KeyGen(NULL, integrityMechType, params, 20, NULL);
	    PK11_DestroyPBEParams(params);
	    if(!symKey) {
		goto loser;
	    }

	    
	    
	    hmacMechType=  sec_pkcs12_algtag_to_mech( 
	                              p12exp->integrityInfo.pwdInfo.algorithm);

	    p12enc->hmacCx = PK11_CreateContextBySymKey( hmacMechType,
						 CKA_SIGN, symKey, &ignore);

	    PK11_FreeSymKey(symKey);
	    if(!p12enc->hmacCx) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    }
	    rv = PK11_DigestBegin(p12enc->hmacCx);
	    if (rv != SECSuccess)
		goto loser;
	}
    }

    if(!p12enc->aSafeCinfo) {
	goto loser;
    }

    PORT_ArenaUnmark(p12exp->arena, mark);

    return p12enc;

loser:
    if(p12enc) {
	if(p12enc->aSafeCinfo) {
	    SEC_PKCS7DestroyContentInfo(p12enc->aSafeCinfo);
	}
	if(p12enc->hmacCx) {
	    PK11_DestroyContext(p12enc->hmacCx, PR_TRUE);
	}
    }
    if (p12exp->arena != NULL)
	PORT_ArenaRelease(p12exp->arena, mark);

    return NULL;
}





static void
sec_P12A1OutputCB_Outer(void *arg, const char *buf, unsigned long len,
		       int depth, SEC_ASN1EncodingPart data_kind)
{
    struct sec_pkcs12_encoder_output *output;

    output = (struct sec_pkcs12_encoder_output*)arg;
    (* output->outputfn)(output->outputarg, buf, len);
}





static void
sec_P12A1OutputCB_HmacP7Update(void *arg, const char *buf,
			       unsigned long        len, 
			       int                  depth,
			       SEC_ASN1EncodingPart data_kind)
{
    sec_pkcs12OutputBuffer *  bufcx = (sec_pkcs12OutputBuffer *)arg;

    if(!buf || !len) 
	return;

    if (bufcx->hmacCx) {
	PK11_DigestOp(bufcx->hmacCx, (unsigned char *)buf, len);
    }

    
    if (bufcx->numBytes > 0) {
	int toCopy;
	if (len + bufcx->numBytes <= bufcx->bufBytes) {
	    memcpy(bufcx->buf + bufcx->numBytes, buf, len);
	    bufcx->numBytes += len;
	    if (bufcx->numBytes < bufcx->bufBytes) 
	    	return;
	    SEC_PKCS7EncoderUpdate(bufcx->p7eCx, bufcx->buf, bufcx->bufBytes);
	    bufcx->numBytes = 0;
	    return;
	} 
	toCopy = bufcx->bufBytes - bufcx->numBytes;
	memcpy(bufcx->buf + bufcx->numBytes, buf, toCopy);
	SEC_PKCS7EncoderUpdate(bufcx->p7eCx, bufcx->buf, bufcx->bufBytes);
	bufcx->numBytes = 0;
	len -= toCopy;
	buf += toCopy;
    } 
    
    if (len >= bufcx->bufBytes) {
	
	SEC_PKCS7EncoderUpdate(bufcx->p7eCx, buf, len);
    } else {
	
	memcpy(bufcx->buf, buf, len);
	bufcx->numBytes = len;
    }
}

void
sec_FlushPkcs12OutputBuffer( sec_pkcs12OutputBuffer *  bufcx)
{
    if (bufcx->numBytes > 0) {
	SEC_PKCS7EncoderUpdate(bufcx->p7eCx, bufcx->buf, bufcx->numBytes);
	bufcx->numBytes = 0;
    }
}




static void
sec_P12P7OutputCB_CallA1Update(void *arg, const char *buf, unsigned long len)
{
    SEC_ASN1EncoderContext *cx = (SEC_ASN1EncoderContext*)arg;

    if (!buf || !len) 
    	return;

    SEC_ASN1EncoderUpdate(cx, buf, len);
}





static SECStatus 
sec_pkcs12_encoder_asafe_process(sec_PKCS12EncoderContext *p12ecx)
{
    SEC_PKCS7EncoderContext *innerP7ecx;
    SEC_PKCS7ContentInfo    *cinfo;
    PK11SymKey              *bulkKey      = NULL;
    SEC_ASN1EncoderContext  *innerA1ecx   = NULL;
    SECStatus                rv           = SECSuccess;

    if(p12ecx->currentSafe < p12ecx->p12exp->authSafe.safeCount) {
	SEC_PKCS12SafeInfo *safeInfo;
	SECOidTag cinfoType;

	safeInfo = p12ecx->p12exp->safeInfos[p12ecx->currentSafe];

	
	if(safeInfo->itemCount == 0) {
	    return SECSuccess;
	}

	cinfo = safeInfo->cinfo;
	cinfoType = SEC_PKCS7ContentType(cinfo);

	
	switch(cinfoType) {
	    case SEC_OID_PKCS7_DATA:
	    case SEC_OID_PKCS7_ENVELOPED_DATA:
		break;
	    case SEC_OID_PKCS7_ENCRYPTED_DATA:
		bulkKey = safeInfo->encryptionKey;
		PK11_SetSymKeyUserData(bulkKey, &safeInfo->pwitem, NULL);
		break;
	    default:
		return SECFailure;

	}

	
	innerP7ecx = SEC_PKCS7EncoderStart(cinfo, 
				  sec_P12P7OutputCB_CallA1Update,
				  p12ecx->middleA1ecx, bulkKey);
	if(!innerP7ecx) {
	    goto loser;
	}

	
	p12ecx->innerBuf.p7eCx    = innerP7ecx;
	p12ecx->innerBuf.hmacCx   = NULL;
	p12ecx->innerBuf.numBytes = 0;
	p12ecx->innerBuf.bufBytes = sizeof p12ecx->innerBuf.buf;

	innerA1ecx = SEC_ASN1EncoderStart(safeInfo->safe, 
	                           sec_PKCS12SafeContentsTemplate,
				   sec_P12A1OutputCB_HmacP7Update, 
				   &p12ecx->innerBuf);
	if(!innerA1ecx) {
	    goto loser;
	}   
	rv = SEC_ASN1EncoderUpdate(innerA1ecx, NULL, 0);
	SEC_ASN1EncoderFinish(innerA1ecx);
	sec_FlushPkcs12OutputBuffer( &p12ecx->innerBuf);
	innerA1ecx = NULL;
	if(rv != SECSuccess) {
	    goto loser;
	}


	
	rv = SEC_PKCS7EncoderFinish(innerP7ecx, p12ecx->p12exp->pwfn, 
				    p12ecx->p12exp->pwfnarg);
    }
    memset(&p12ecx->innerBuf, 0, sizeof p12ecx->innerBuf);
    return SECSuccess;

loser:
    if(innerP7ecx) {
	SEC_PKCS7EncoderFinish(innerP7ecx, p12ecx->p12exp->pwfn, 
			       p12ecx->p12exp->pwfnarg);
    }

    if(innerA1ecx) {
	SEC_ASN1EncoderFinish(innerA1ecx);
    }
    memset(&p12ecx->innerBuf, 0, sizeof p12ecx->innerBuf);
    return SECFailure;
}




static SECStatus
sec_Pkcs12FinishMac(sec_PKCS12EncoderContext *p12ecx)
{
    SECItem hmac = { siBuffer, NULL, 0 };
    SECStatus rv;
    SGNDigestInfo *di = NULL;
    void *dummy;

    if(!p12ecx) {
	return SECFailure;
    }

    
    if(!p12ecx->p12exp->integrityEnabled) {
	return SECSuccess;
    }

    if(!p12ecx->p12exp->pwdIntegrity) {
	return SECSuccess;
    }

    
    hmac.data = (unsigned char *)PORT_ZAlloc(SHA1_LENGTH);
    if(!hmac.data) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }

    rv = PK11_DigestFinal(p12ecx->hmacCx, hmac.data, &hmac.len, SHA1_LENGTH);

    if(rv != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    di = SGN_CreateDigestInfo(p12ecx->p12exp->integrityInfo.pwdInfo.algorithm,
    			      hmac.data, hmac.len);
    if(!di) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	rv = SECFailure;
	goto loser;
    }

    rv = SGN_CopyDigestInfo(p12ecx->arena, &p12ecx->mac.safeMac, di);
    if(rv != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    
    dummy = SEC_ASN1EncodeItem(p12ecx->arena, &p12ecx->pfx.encodedMacData, 
    			    &p12ecx->mac, sec_PKCS12MacDataTemplate);
    if(!dummy) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	rv = SECFailure;
    }

loser:
    if(di) {
	SGN_DestroyDigestInfo(di);
    }
    if(hmac.data) {
	SECITEM_ZfreeItem(&hmac, PR_FALSE);
    }
    PK11_DestroyContext(p12ecx->hmacCx, PR_TRUE);
    p12ecx->hmacCx = NULL;

    return rv;
}






static void
sec_pkcs12_encoder_pfx_notify(void *arg, PRBool before, void *dest, int real_depth)
{
    sec_PKCS12EncoderContext *p12ecx;

    if(!before) {
	return;
    }

    
    p12ecx = (sec_PKCS12EncoderContext*)arg;
    if(dest != &p12ecx->pfx.encodedAuthSafe) {
	return;
    }

    SEC_ASN1EncoderSetTakeFromBuf(p12ecx->outerA1ecx);
    SEC_ASN1EncoderSetStreaming(p12ecx->outerA1ecx);
    SEC_ASN1EncoderClearNotifyProc(p12ecx->outerA1ecx);
}










SECStatus
SEC_PKCS12Encode(SEC_PKCS12ExportContext *p12exp, 
		 SEC_PKCS12EncoderOutputCallback output, void *outputarg)
{
    sec_PKCS12EncoderContext *p12enc;
    struct sec_pkcs12_encoder_output outInfo;
    SECStatus rv;

    if(!p12exp || !output) {
	return SECFailure;
    }

    
    p12enc = sec_pkcs12_encoder_start_context(p12exp);
    if(!p12enc) {
	return SECFailure;
    }

    outInfo.outputfn = output;
    outInfo.outputarg = outputarg;

    
    p12enc->outerA1ecx = SEC_ASN1EncoderStart(&p12enc->pfx, 
                                       sec_PKCS12PFXItemTemplate,
				       sec_P12A1OutputCB_Outer, 
				       &outInfo);
    if(!p12enc->outerA1ecx) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	rv = SECFailure;
	goto loser;
    }
    SEC_ASN1EncoderSetStreaming(p12enc->outerA1ecx);
    SEC_ASN1EncoderSetNotifyProc(p12enc->outerA1ecx, 
                                 sec_pkcs12_encoder_pfx_notify, p12enc);
    rv = SEC_ASN1EncoderUpdate(p12enc->outerA1ecx, NULL, 0);
    if(rv != SECSuccess) {
	rv = SECFailure;
	goto loser;
    }

    
    p12enc->middleP7ecx = SEC_PKCS7EncoderStart(p12enc->aSafeCinfo, 
				       sec_P12P7OutputCB_CallA1Update,
				       p12enc->outerA1ecx, NULL);
    if(!p12enc->middleP7ecx) {
	rv = SECFailure;
	goto loser;
    }

    
    p12enc->middleBuf.p7eCx    = p12enc->middleP7ecx;
    p12enc->middleBuf.hmacCx   = NULL;
    p12enc->middleBuf.numBytes = 0;
    p12enc->middleBuf.bufBytes = sizeof p12enc->middleBuf.buf;

    
    if(p12enc->p12exp->integrityEnabled && 
       p12enc->p12exp->pwdIntegrity) {
	p12enc->middleBuf.hmacCx = p12enc->hmacCx;
    }
    p12enc->middleA1ecx = SEC_ASN1EncoderStart(&p12enc->p12exp->authSafe,
			    sec_PKCS12AuthenticatedSafeTemplate,
			    sec_P12A1OutputCB_HmacP7Update,
			    &p12enc->middleBuf);
    if(!p12enc->middleA1ecx) {
	rv = SECFailure;
	goto loser;
    }
    SEC_ASN1EncoderSetStreaming(p12enc->middleA1ecx);
    SEC_ASN1EncoderSetTakeFromBuf(p12enc->middleA1ecx); 
	
    			 
    while(p12enc->currentSafe != p12enc->p12exp->safeInfoCount) {
	sec_pkcs12_encoder_asafe_process(p12enc);
	p12enc->currentSafe++;
    }
    SEC_ASN1EncoderClearTakeFromBuf(p12enc->middleA1ecx);
    SEC_ASN1EncoderClearStreaming(p12enc->middleA1ecx);
    SEC_ASN1EncoderUpdate(p12enc->middleA1ecx, NULL, 0);
    SEC_ASN1EncoderFinish(p12enc->middleA1ecx);

    sec_FlushPkcs12OutputBuffer( &p12enc->middleBuf);

    
    rv = SEC_PKCS7EncoderFinish(p12enc->middleP7ecx, p12exp->pwfn, 
    				p12exp->pwfnarg);
    if(rv != SECSuccess) {
	goto loser;
    }

    SEC_ASN1EncoderClearTakeFromBuf(p12enc->outerA1ecx);
    SEC_ASN1EncoderClearStreaming(p12enc->outerA1ecx);

    
    rv = sec_Pkcs12FinishMac(p12enc);
    if(rv != SECSuccess) {
	goto loser;
    }
   
     
    rv = SEC_ASN1EncoderUpdate(p12enc->outerA1ecx, NULL, 0);

    SEC_ASN1EncoderFinish(p12enc->outerA1ecx);

loser:
    return rv;
}

void
SEC_PKCS12DestroyExportContext(SEC_PKCS12ExportContext *p12ecx)
{
    int i = 0;

    if(!p12ecx) {
	return;
    }

    if(p12ecx->safeInfos) {
	i = 0;
	while(p12ecx->safeInfos[i] != NULL) {
	    if(p12ecx->safeInfos[i]->encryptionKey) {
		PK11_FreeSymKey(p12ecx->safeInfos[i]->encryptionKey);
	    }
	    if(p12ecx->safeInfos[i]->cinfo) {
		SEC_PKCS7DestroyContentInfo(p12ecx->safeInfos[i]->cinfo);
	    }
	    i++;
	}
    }

    PK11_FreeSlot(p12ecx->slot);

    PORT_FreeArena(p12ecx->arena, PR_TRUE);
}

