








































 
#include "cert.h"
#include "certi.h"
#include "secder.h"
#include "secasn1.h"
#include "secoid.h"
#include "certdb.h"
#include "certxutl.h"
#include "prtime.h"
#include "secerr.h"
#include "pk11func.h"
#include "dev.h"
#include "dev3hack.h"
#include "nssbase.h"
#if defined(DPC_RWLOCK) || defined(GLOBAL_RWLOCK)
#include "nssrwlk.h"
#endif
#include "pk11priv.h"

const SEC_ASN1Template SEC_CERTExtensionTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCertExtension) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(CERTCertExtension,id) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_BOOLEAN,		
	  offsetof(CERTCertExtension,critical), },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(CERTCertExtension,value) },
    { 0, }
};

static const SEC_ASN1Template SEC_CERTExtensionsTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0,  SEC_CERTExtensionTemplate}
};







const SEC_ASN1Template CERT_IssuerAndSNTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTIssuerAndSN) },
    { SEC_ASN1_SAVE,
	  offsetof(CERTIssuerAndSN,derIssuer) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTIssuerAndSN,issuer),
	  CERT_NameTemplate },
    { SEC_ASN1_INTEGER,
	  offsetof(CERTIssuerAndSN,serialNumber) },
    { 0 }
};

static const SEC_ASN1Template cert_KrlEntryTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCrlEntry) },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(CERTCrlEntry,serialNumber) },
    { SEC_ASN1_UTC_TIME,
	  offsetof(CERTCrlEntry,revocationDate) },
    { 0 }
};

SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)
SEC_ASN1_MKSUB(CERT_TimeChoiceTemplate)

static const SEC_ASN1Template cert_KrlTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCrl) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCrl,signatureAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_SAVE,
	  offsetof(CERTCrl,derName) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCrl,name),
	  CERT_NameTemplate },
    { SEC_ASN1_UTC_TIME,
	  offsetof(CERTCrl,lastUpdate) },
    { SEC_ASN1_UTC_TIME,
	  offsetof(CERTCrl,nextUpdate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_SEQUENCE_OF,
	  offsetof(CERTCrl,entries),
	  cert_KrlEntryTemplate },
    { 0 }
};

static const SEC_ASN1Template cert_SignedKrlTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTSignedCrl) },
    { SEC_ASN1_SAVE,
	  offsetof(CERTSignedCrl,signatureWrap.data) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTSignedCrl,crl),
	  cert_KrlTemplate },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTSignedCrl,signatureWrap.signatureAlgorithm),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_BIT_STRING,
	  offsetof(CERTSignedCrl,signatureWrap.signature) },
    { 0 }
};

static const SEC_ASN1Template cert_CrlKeyTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCrlKey) },
    { SEC_ASN1_INTEGER | SEC_ASN1_OPTIONAL, offsetof(CERTCrlKey,dummy) },
    { SEC_ASN1_SKIP },
    { SEC_ASN1_ANY, offsetof(CERTCrlKey,derName) },
    { SEC_ASN1_SKIP_REST },
    { 0 }
};

static const SEC_ASN1Template cert_CrlEntryTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCrlEntry) },
    { SEC_ASN1_INTEGER,
	  offsetof(CERTCrlEntry,serialNumber) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCrlEntry,revocationDate),
          SEC_ASN1_SUB(CERT_TimeChoiceTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_SEQUENCE_OF,
	  offsetof(CERTCrlEntry, extensions),
	  SEC_CERTExtensionTemplate},
    { 0 }
};

const SEC_ASN1Template CERT_CrlTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCrl) },
    { SEC_ASN1_INTEGER | SEC_ASN1_OPTIONAL, offsetof (CERTCrl, version) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCrl,signatureAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate)},
    { SEC_ASN1_SAVE,
	  offsetof(CERTCrl,derName) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCrl,name),
	  CERT_NameTemplate },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCrl,lastUpdate),
          SEC_ASN1_SUB(CERT_TimeChoiceTemplate) },
    { SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL | SEC_ASN1_XTRN,
	  offsetof(CERTCrl,nextUpdate),
          SEC_ASN1_SUB(CERT_TimeChoiceTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_SEQUENCE_OF,
	  offsetof(CERTCrl,entries),
	  cert_CrlEntryTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
	  SEC_ASN1_EXPLICIT | 0,
	  offsetof(CERTCrl,extensions),
	  SEC_CERTExtensionsTemplate},
    { 0 }
};

const SEC_ASN1Template CERT_CrlTemplateNoEntries[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCrl) },
    { SEC_ASN1_INTEGER | SEC_ASN1_OPTIONAL, offsetof (CERTCrl, version) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCrl,signatureAlg),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_SAVE,
	  offsetof(CERTCrl,derName) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCrl,name),
	  CERT_NameTemplate },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCrl,lastUpdate),
          SEC_ASN1_SUB(CERT_TimeChoiceTemplate) },
    { SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL | SEC_ASN1_XTRN,
	  offsetof(CERTCrl,nextUpdate),
          SEC_ASN1_SUB(CERT_TimeChoiceTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_SEQUENCE_OF |
      SEC_ASN1_SKIP }, 
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
	  SEC_ASN1_EXPLICIT | 0,
	  offsetof(CERTCrl,extensions),
	  SEC_CERTExtensionsTemplate },
    { 0 }
};

const SEC_ASN1Template CERT_CrlTemplateEntriesOnly[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCrl) },
    { SEC_ASN1_SKIP | SEC_ASN1_INTEGER | SEC_ASN1_OPTIONAL },
    { SEC_ASN1_SKIP },
    { SEC_ASN1_SKIP },
    { SEC_ASN1_SKIP | SEC_ASN1_INLINE | SEC_ASN1_XTRN,
        offsetof(CERTCrl,lastUpdate),
        SEC_ASN1_SUB(CERT_TimeChoiceTemplate) },
    { SEC_ASN1_SKIP | SEC_ASN1_INLINE | SEC_ASN1_OPTIONAL | SEC_ASN1_XTRN,
        offsetof(CERTCrl,nextUpdate),
        SEC_ASN1_SUB(CERT_TimeChoiceTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_SEQUENCE_OF,
	  offsetof(CERTCrl,entries),
	  cert_CrlEntryTemplate }, 
    { SEC_ASN1_SKIP_REST },
    { 0 }
};

const SEC_ASN1Template CERT_SignedCrlTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTSignedCrl) },
    { SEC_ASN1_SAVE,
	  offsetof(CERTSignedCrl,signatureWrap.data) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTSignedCrl,crl),
	  CERT_CrlTemplate },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN ,
	  offsetof(CERTSignedCrl,signatureWrap.signatureAlgorithm),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_BIT_STRING,
	  offsetof(CERTSignedCrl,signatureWrap.signature) },
    { 0 }
};

static const SEC_ASN1Template cert_SignedCrlTemplateNoEntries[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTSignedCrl) },
    { SEC_ASN1_SAVE,
	  offsetof(CERTSignedCrl,signatureWrap.data) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTSignedCrl,crl),
	  CERT_CrlTemplateNoEntries },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTSignedCrl,signatureWrap.signatureAlgorithm),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_BIT_STRING,
	  offsetof(CERTSignedCrl,signatureWrap.signature) },
    { 0 }
};

const SEC_ASN1Template CERT_SetOfSignedCrlTemplate[] = {
    { SEC_ASN1_SET_OF, 0, CERT_SignedCrlTemplate },
};


int cert_get_crl_version(CERTCrl * crl)
{
    
    int version = SEC_CRL_VERSION_1;
    if (crl && crl->version.data != 0) {
	version = (int)DER_GetUInteger (&crl->version);
    }
    return version;
}



SECStatus cert_check_crl_entries (CERTCrl *crl)
{
    CERTCrlEntry **entries;
    CERTCrlEntry *entry;
    PRBool hasCriticalExten = PR_FALSE;
    SECStatus rv = SECSuccess;

    if (!crl) {
        return SECFailure;
    }

    if (crl->entries == NULL) {
        
        return (SECSuccess);
    }

    


    entries = crl->entries;
    while (*entries) {
	entry = *entries;
	if (entry->extensions) {
	    



            if (hasCriticalExten == PR_FALSE) {
                hasCriticalExten = cert_HasCriticalExtension (entry->extensions);
                if (hasCriticalExten) {
                    if (cert_get_crl_version(crl) != SEC_CRL_VERSION_2) { 
                        
                        PORT_SetError(SEC_ERROR_CRL_V1_CRITICAL_EXTENSION);
                        rv = SECFailure;
                        break;
                    }
                }
            }

	    



	    if (cert_HasUnknownCriticalExten (entry->extensions) == PR_TRUE) {
		PORT_SetError (SEC_ERROR_CRL_UNKNOWN_CRITICAL_EXTENSION);
		rv = SECFailure;
		break;
	    }
	}
	++entries;
    }
    return(rv);
}






SECStatus cert_check_crl_version (CERTCrl *crl)
{
    PRBool hasCriticalExten = PR_FALSE;
    int version = cert_get_crl_version(crl);
	
    if (version > SEC_CRL_VERSION_2) {
	PORT_SetError (SEC_ERROR_CRL_INVALID_VERSION);
	return (SECFailure);
    }

    


    if (crl->extensions) {
	hasCriticalExten = cert_HasCriticalExtension (crl->extensions);
	if (hasCriticalExten) {
            if (version != SEC_CRL_VERSION_2) {
                
                PORT_SetError(SEC_ERROR_CRL_V1_CRITICAL_EXTENSION);
                return (SECFailure);
            }
	    
	    if (cert_HasUnknownCriticalExten (crl->extensions) == PR_TRUE) {
		PORT_SetError (SEC_ERROR_CRL_UNKNOWN_CRITICAL_EXTENSION);
		return (SECFailure);
	    }
	}
    }

    return (SECSuccess);
}





SECStatus
CERT_KeyFromDERCrl(PRArenaPool *arena, SECItem *derCrl, SECItem *key)
{
    SECStatus rv;
    CERTSignedData sd;
    CERTCrlKey crlkey;
    PRArenaPool* myArena;

    if (!arena) {
        
        myArena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    } else {
        myArena = arena;
    }
    PORT_Memset (&sd, 0, sizeof (sd));
    rv = SEC_QuickDERDecodeItem (myArena, &sd, CERT_SignedDataTemplate, derCrl);
    if (SECSuccess == rv) {
        PORT_Memset (&crlkey, 0, sizeof (crlkey));
        rv = SEC_QuickDERDecodeItem(myArena, &crlkey, cert_CrlKeyTemplate, &sd.data);
    }

    

    if (SECSuccess == rv) {
        rv = SECITEM_CopyItem(arena, key, &crlkey.derName);
    }

    if (myArena != arena) {
        PORT_FreeArena(myArena, PR_FALSE);
    }

    return rv;
}

#define GetOpaqueCRLFields(x) ((OpaqueCRLFields*)x->opaque)

SECStatus CERT_CompleteCRLDecodeEntries(CERTSignedCrl* crl)
{
    SECStatus rv = SECSuccess;
    SECItem* crldata = NULL;
    OpaqueCRLFields* extended = NULL;

    if ( (!crl) ||
         (!(extended = (OpaqueCRLFields*) crl->opaque)) ||
         (PR_TRUE == extended->decodingError) ) {
        rv = SECFailure;
    } else {
        if (PR_FALSE == extended->partial) {
            
            return SECSuccess;
        }
        if (PR_TRUE == extended->badEntries) {
            
            return SECFailure;
        }
        crldata = &crl->signatureWrap.data;
        if (!crldata) {
            rv = SECFailure;
        }
    }

    if (SECSuccess == rv) {
        rv = SEC_QuickDERDecodeItem(crl->arena,
            &crl->crl,
            CERT_CrlTemplateEntriesOnly,
            crldata);
        if (SECSuccess == rv) {
            extended->partial = PR_FALSE; 

        } else {
            extended->decodingError = PR_TRUE;
            extended->badEntries = PR_TRUE;
            


        }
        rv = cert_check_crl_entries(&crl->crl);
        if (rv != SECSuccess) {
            extended->badExtensions = PR_TRUE;
        }
    }
    return rv;
}





CERTSignedCrl *
CERT_DecodeDERCrlWithFlags(PRArenaPool *narena, SECItem *derSignedCrl,
                          int type, PRInt32 options)
{
    PRArenaPool *arena;
    CERTSignedCrl *crl;
    SECStatus rv;
    OpaqueCRLFields* extended = NULL;
    const SEC_ASN1Template* crlTemplate = CERT_SignedCrlTemplate;
    PRInt32 testOptions = options;

    PORT_Assert(derSignedCrl);
    if (!derSignedCrl) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }

    



    testOptions &= (CRL_DECODE_ADOPT_HEAP_DER | CRL_DECODE_DONT_COPY_DER);
    PORT_Assert(testOptions != CRL_DECODE_ADOPT_HEAP_DER);
    if (testOptions == CRL_DECODE_ADOPT_HEAP_DER) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }

    
    if (narena == NULL) {
    	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if ( !arena ) {
	    return NULL;
	}
    } else {
	arena = narena;
    }

    
    crl = (CERTSignedCrl *)PORT_ArenaZAlloc(arena, sizeof(CERTSignedCrl));
    if ( !crl ) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto loser;
    }

    crl->arena = arena;

    
    crl->opaque = (void*)PORT_ArenaZAlloc(arena, sizeof(OpaqueCRLFields));
    if ( !crl->opaque ) {
	goto loser;
    }
    extended = (OpaqueCRLFields*) crl->opaque;
    if (options & CRL_DECODE_ADOPT_HEAP_DER) {
        extended->heapDER = PR_TRUE;
    }
    if (options & CRL_DECODE_DONT_COPY_DER) {
        crl->derCrl = derSignedCrl; 


    } else {
        crl->derCrl = (SECItem *)PORT_ArenaZAlloc(arena,sizeof(SECItem));
        if (crl->derCrl == NULL) {
            goto loser;
        }
        rv = SECITEM_CopyItem(arena, crl->derCrl, derSignedCrl);
        if (rv != SECSuccess) {
            goto loser;
        }
    }

    
    crl->crl.arena = arena;
    if (options & CRL_DECODE_SKIP_ENTRIES) {
        crlTemplate = cert_SignedCrlTemplateNoEntries;
        extended->partial = PR_TRUE;
    }

    
    switch (type) {
    case SEC_CRL_TYPE:
        rv = SEC_QuickDERDecodeItem(arena, crl, crlTemplate, crl->derCrl);
        if (rv != SECSuccess) {
            extended->badDER = PR_TRUE;
            break;
        }
        
        rv =  cert_check_crl_version (&crl->crl);
        if (rv != SECSuccess) {
            extended->badExtensions = PR_TRUE;
            break;
        }

        if (PR_TRUE == extended->partial) {
            
            break;
        }

        rv = cert_check_crl_entries(&crl->crl);
        if (rv != SECSuccess) {
            extended->badExtensions = PR_TRUE;
        }

        break;

    case SEC_KRL_TYPE:
	rv = SEC_QuickDERDecodeItem
	     (arena, crl, cert_SignedKrlTemplate, derSignedCrl);
	break;
    default:
	rv = SECFailure;
	break;
    }

    if (rv != SECSuccess) {
	goto loser;
    }

    crl->referenceCount = 1;
    
    return(crl);
    
loser:
    if (options & CRL_DECODE_KEEP_BAD_CRL) {
        if (extended) {
            extended->decodingError = PR_TRUE;
        }
        if (crl) {
            crl->referenceCount = 1;
            return(crl);
        }
    }

    if ((narena == NULL) && arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(0);
}




CERTSignedCrl *
CERT_DecodeDERCrl(PRArenaPool *narena, SECItem *derSignedCrl, int type)
{
    return CERT_DecodeDERCrlWithFlags(narena, derSignedCrl, type,
                                      CRL_DECODE_DEFAULT_OPTIONS);
}

















static SECStatus
SEC_FindCrlByKeyOnSlot(PK11SlotInfo *slot, SECItem *crlKey, int type,
                       CERTSignedCrl** decoded, PRInt32 decodeoptions)
{
    SECStatus rv = SECSuccess;
    CERTSignedCrl *crl = NULL;
    SECItem *derCrl = NULL;
    CK_OBJECT_HANDLE crlHandle = 0;
    char *url = NULL;

    PORT_Assert(decoded);
    if (!decoded) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    derCrl = PK11_FindCrlByName(&slot, &crlHandle, crlKey, type, &url);
    if (derCrl == NULL) {
	

	int nsserror = PORT_GetError();
	if (nsserror != SEC_ERROR_CRL_NOT_FOUND) {
	    rv = SECFailure;
	}
	goto loser;
    }
    PORT_Assert(crlHandle != CK_INVALID_HANDLE);
    
    
    


    decodeoptions |= (CRL_DECODE_ADOPT_HEAP_DER | CRL_DECODE_DONT_COPY_DER);

    crl = CERT_DecodeDERCrlWithFlags(NULL, derCrl, type, decodeoptions);
    if (crl) {
        crl->slot = slot;
        slot = NULL; 
	derCrl = NULL; 
        crl->pkcs11ID = crlHandle;
        if (url) {
            crl->url = PORT_ArenaStrdup(crl->arena,url);
        }
    } else {
        rv = SECFailure;
    }
    
    if (url) {
	PORT_Free(url);
    }

    if (slot) {
	PK11_FreeSlot(slot);
    }

loser:
    if (derCrl) {
	SECITEM_FreeItem(derCrl, PR_TRUE);
    }

    *decoded = crl;

    return rv;
}


CERTSignedCrl *
crl_storeCRL (PK11SlotInfo *slot,char *url,
                  CERTSignedCrl *newCrl, SECItem *derCrl, int type)
{
    CERTSignedCrl *oldCrl = NULL, *crl = NULL;
    PRBool deleteOldCrl = PR_FALSE;
    CK_OBJECT_HANDLE crlHandle = CK_INVALID_HANDLE;
    SECStatus rv;

    PORT_Assert(newCrl);
    PORT_Assert(derCrl);

    

    rv = SEC_FindCrlByKeyOnSlot(slot, &newCrl->crl.derName, type,
                                &oldCrl, CRL_DECODE_SKIP_ENTRIES);
    



    if (oldCrl != NULL) {
	
	if (SECITEM_CompareItem(newCrl->derCrl, oldCrl->derCrl) 
						== SECEqual) {
	    crl = newCrl;
	    crl->slot = PK11_ReferenceSlot(slot);
	    crl->pkcs11ID = oldCrl->pkcs11ID;
	    if (oldCrl->url && !url)
	        url = oldCrl->url;
	    if (url)
		crl->url = PORT_ArenaStrdup(crl->arena, url);
	    goto done;
	}
        if (!SEC_CrlIsNewer(&newCrl->crl,&oldCrl->crl)) {

            if (type == SEC_CRL_TYPE) {
                PORT_SetError(SEC_ERROR_OLD_CRL);
            } else {
                PORT_SetError(SEC_ERROR_OLD_KRL);
            }

            goto done;
        }

        if ((SECITEM_CompareItem(&newCrl->crl.derName,
                &oldCrl->crl.derName) != SECEqual) &&
            (type == SEC_KRL_TYPE) ) {

            PORT_SetError(SEC_ERROR_CKL_CONFLICT);
            goto done;
        }

        
        if (oldCrl->url && !url) {
	    url = oldCrl->url;
        }

        
        
	deleteOldCrl = PR_TRUE;
    }

    
    CERT_CRLCacheRefreshIssuer(NULL, &newCrl->crl.derName);
    
    crlHandle = PK11_PutCrl(slot, derCrl, &newCrl->crl.derName, url, type);
    if (crlHandle != CK_INVALID_HANDLE) {
	crl = newCrl;
	crl->slot = PK11_ReferenceSlot(slot);
	crl->pkcs11ID = crlHandle;
	if (url) {
	    crl->url = PORT_ArenaStrdup(crl->arena,url);
	}
    }

done:
    if (oldCrl) {
	if (deleteOldCrl && crlHandle != CK_INVALID_HANDLE) {
	    SEC_DeletePermCRL(oldCrl);
	}
	SEC_DestroyCrl(oldCrl);
    }

    return crl;
}








CERTSignedCrl *
SEC_NewCrl(CERTCertDBHandle *handle, char *url, SECItem *derCrl, int type)
{
    CERTSignedCrl* retCrl = NULL;
    PK11SlotInfo* slot = PK11_GetInternalKeySlot();
    retCrl = PK11_ImportCRL(slot, derCrl, url, type, NULL,
        CRL_IMPORT_BYPASS_CHECKS, NULL, CRL_DECODE_DEFAULT_OPTIONS);
    PK11_FreeSlot(slot);

    return retCrl;
}
    
CERTSignedCrl *
SEC_FindCrlByDERCert(CERTCertDBHandle *handle, SECItem *derCrl, int type)
{
    PRArenaPool *arena;
    SECItem crlKey;
    SECStatus rv;
    CERTSignedCrl *crl = NULL;
    
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return(NULL);
    }
    
    
    rv = CERT_KeyFromDERCrl(arena, derCrl, &crlKey);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    
    crl = SEC_FindCrlByName(handle, &crlKey, type);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(crl);
}

CERTSignedCrl* SEC_DupCrl(CERTSignedCrl* acrl)
{
    if (acrl)
    {
        PR_AtomicIncrement(&acrl->referenceCount);
        return acrl;
    }
    return NULL;
}

SECStatus
SEC_DestroyCrl(CERTSignedCrl *crl)
{
    if (crl) {
	if (PR_AtomicDecrement(&crl->referenceCount) < 1) {
	    if (crl->slot) {
		PK11_FreeSlot(crl->slot);
	    }
            if (GetOpaqueCRLFields(crl) &&
                PR_TRUE == GetOpaqueCRLFields(crl)->heapDER) {
                SECITEM_FreeItem(crl->derCrl, PR_TRUE);
            }
            if (crl->arena) {
                PORT_FreeArena(crl->arena, PR_FALSE);
            }
	}
        return SECSuccess;
    } else {
        return SECFailure;
    }
}

SECStatus
SEC_LookupCrls(CERTCertDBHandle *handle, CERTCrlHeadNode **nodes, int type)
{
    CERTCrlHeadNode *head;
    PRArenaPool *arena = NULL;
    SECStatus rv;

    *nodes = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return SECFailure;
    }

    
    head = (CERTCrlHeadNode *)PORT_ArenaAlloc(arena, sizeof(CERTCrlHeadNode));
    head->arena = arena;
    head->first = NULL;
    head->last = NULL;
    head->dbhandle = handle;

    
    *nodes = head;

    rv = PK11_LookupCrls(head, type, NULL);
    
    if (rv != SECSuccess) {
	if ( arena ) {
	    PORT_FreeArena(arena, PR_FALSE);
	    *nodes = NULL;
	}
    }

    return rv;
}




SEC_ASN1_CHOOSER_IMPLEMENT(CERT_IssuerAndSNTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(CERT_CrlTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(CERT_SignedCrlTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(CERT_SetOfSignedCrlTemplate)




static SECStatus CachedCrl_Create(CachedCrl** returned, CERTSignedCrl* crl,
                           CRLOrigin origin);

static SECStatus CachedCrl_Destroy(CachedCrl* crl);


static SECStatus CachedCrl_Populate(CachedCrl* crlobject);


static SECStatus CachedCrl_Depopulate(CachedCrl* crl);




static SECStatus CachedCrl_Compare(CachedCrl* a, CachedCrl* b, PRBool* isDupe,
                                PRBool* isUpdated);


static SECStatus DPCache_Create(CRLDPCache** returned, CERTCertificate* issuer,
                         const SECItem* subject, SECItem* dp);


static SECStatus DPCache_Destroy(CRLDPCache* cache);



static SECStatus DPCache_AddCRL(CRLDPCache* cache, CachedCrl* crl,
                                PRBool* added);


static SECStatus DPCache_FetchFromTokens(CRLDPCache* cache, PRTime vfdate,
                                         void* wincx);



static SECStatus DPCache_GetUpToDate(CRLDPCache* cache, CERTCertificate* issuer,
                         PRBool readlocked, PRTime vfdate, void* wincx);


static PRBool DPCache_HasTokenCRLs(CRLDPCache* cache);


static SECStatus DPCache_RemoveCRL(CRLDPCache* cache, PRUint32 offset);


static SECStatus DPCache_SelectCRL(CRLDPCache* cache);


static SECStatus IssuerCache_Create(CRLIssuerCache** returned,
                             CERTCertificate* issuer,
                             const SECItem* subject, const SECItem* dp);


SECStatus IssuerCache_Destroy(CRLIssuerCache* cache);


static SECStatus IssuerCache_AddDP(CRLIssuerCache* cache,
                                   CERTCertificate* issuer,
                                   const SECItem* subject,
                                   const SECItem* dp, CRLDPCache** newdpc);


static CRLDPCache* IssuerCache_GetDPCache(CRLIssuerCache* cache,
                                          const SECItem* dp);






static void * PR_CALLBACK
PreAllocTable(void *pool, PRSize size)
{
    PreAllocator* alloc = (PreAllocator*)pool;
    PORT_Assert(alloc);
    if (!alloc)
    {
        
        return NULL;
    }
    if (size > (alloc->len - alloc->used))
    {
        
        alloc->extra += size;
        return PORT_ArenaAlloc(alloc->arena, size);
    }
    
    alloc->used += size;
    return (char*) alloc->data + alloc->used - size;
}



static void PR_CALLBACK
PreFreeTable(void *pool, void *item)
{
}


static PLHashEntry * PR_CALLBACK
PreAllocEntry(void *pool, const void *key)
{
    return PreAllocTable(pool, sizeof(PLHashEntry));
}



static void PR_CALLBACK
PreFreeEntry(void *pool, PLHashEntry *he, PRUintn flag)
{
}


static PLHashAllocOps preAllocOps =
{
    PreAllocTable, PreFreeTable,
    PreAllocEntry, PreFreeEntry
};


void PreAllocator_Destroy(PreAllocator* PreAllocator)
{
    if (!PreAllocator)
    {
        return;
    }
    if (PreAllocator->arena)
    {
        PORT_FreeArena(PreAllocator->arena, PR_TRUE);
    }
}


PreAllocator* PreAllocator_Create(PRSize size)
{
    PRArenaPool* arena = NULL;
    PreAllocator* prebuffer = NULL;
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
    {
        return NULL;
    }
    prebuffer = (PreAllocator*)PORT_ArenaZAlloc(arena,
                                                sizeof(PreAllocator));
    if (!prebuffer)
    {
        PORT_FreeArena(arena, PR_TRUE);
        return NULL;
    }
    prebuffer->arena = arena;

    if (size)
    {
        prebuffer->len = size;
        prebuffer->data = PORT_ArenaAlloc(arena, size);
        if (!prebuffer->data)
        {
            PORT_FreeArena(arena, PR_TRUE);
            return NULL;
        }
    }
    return prebuffer;
}


static NamedCRLCache namedCRLCache = { NULL, NULL };


static CRLCache crlcache = { NULL, NULL };


static PRBool crlcache_initialized = PR_FALSE;

PRTime CRLCache_Empty_TokenFetch_Interval = 60 * 1000000; 



PRTime CRLCache_TokenRefetch_Interval = 600 * 1000000 ; 



PRTime CRLCache_ExistenceCheck_Interval = 60 * 1000000; 



SECStatus InitCRLCache(void)
{
    if (PR_FALSE == crlcache_initialized)
    {
        PORT_Assert(NULL == crlcache.lock);
        PORT_Assert(NULL == crlcache.issuers);
        PORT_Assert(NULL == namedCRLCache.lock);
        PORT_Assert(NULL == namedCRLCache.entries);
        if (crlcache.lock || crlcache.issuers || namedCRLCache.lock ||
            namedCRLCache.entries)
        {
            
            PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
            return SECFailure;
        }
#ifdef GLOBAL_RWLOCK
        crlcache.lock = NSSRWLock_New(NSS_RWLOCK_RANK_NONE, NULL);
#else
        crlcache.lock = PR_NewLock();
#endif
        namedCRLCache.lock = PR_NewLock();
        crlcache.issuers = PL_NewHashTable(0, SECITEM_Hash, SECITEM_HashCompare,
                                  PL_CompareValues, NULL, NULL);
        namedCRLCache.entries = PL_NewHashTable(0, SECITEM_Hash, SECITEM_HashCompare,
                                  PL_CompareValues, NULL, NULL);
        if (!crlcache.lock || !namedCRLCache.lock || !crlcache.issuers ||
            !namedCRLCache.entries)
        {
            if (crlcache.lock)
            {
#ifdef GLOBAL_RWLOCK
                NSSRWLock_Destroy(crlcache.lock);
#else
                PR_DestroyLock(crlcache.lock);
#endif
                crlcache.lock = NULL;
            }
            if (namedCRLCache.lock)
            {
                PR_DestroyLock(namedCRLCache.lock);
                namedCRLCache.lock = NULL;
            }
            if (crlcache.issuers)
            {
                PL_HashTableDestroy(crlcache.issuers);
                crlcache.issuers = NULL;
            }
            if (namedCRLCache.entries)
            {
                PL_HashTableDestroy(namedCRLCache.entries);
                namedCRLCache.entries = NULL;
            }

            return SECFailure;
        }
        crlcache_initialized = PR_TRUE;
        return SECSuccess;
    }
    else
    {
        PORT_Assert(crlcache.lock);
        PORT_Assert(crlcache.issuers);
        if ( (NULL == crlcache.lock) || (NULL == crlcache.issuers) )
        {
            
            return SECFailure;
        }
        else
        {
            
            return SECSuccess;
        }
    }
}


static SECStatus DPCache_Destroy(CRLDPCache* cache)
{
    PRUint32 i = 0;
    PORT_Assert(cache);
    if (!cache)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    if (cache->lock)
    {
#ifdef DPC_RWLOCK
        NSSRWLock_Destroy(cache->lock);
#else
        PR_DestroyLock(cache->lock);
#endif
    }
    else
    {
        PORT_Assert(0);
        return SECFailure;
    }
    
    for (i=0;i<cache->ncrls;i++)
    {
        if (!cache->crls || !cache->crls[i] ||
            SECSuccess != CachedCrl_Destroy(cache->crls[i]))
        {
            return SECFailure;
        }
    }
    
    if (cache->crls)
    {
	PORT_Free(cache->crls);
    }
    
    if (cache->issuer)
    {
        CERT_DestroyCertificate(cache->issuer);
    }
    
    if (cache->subject)
    {
        SECITEM_FreeItem(cache->subject, PR_TRUE);
    }
    
    if (cache->distributionPoint)
    {
        SECITEM_FreeItem(cache->distributionPoint, PR_TRUE);
    }
    PORT_Free(cache);
    return SECSuccess;
}


SECStatus IssuerCache_Destroy(CRLIssuerCache* cache)
{
    PORT_Assert(cache);
    if (!cache)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
#ifdef XCRL
    if (cache->lock)
    {
        NSSRWLock_Destroy(cache->lock);
    }
    else
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    if (cache->issuer)
    {
        CERT_DestroyCertificate(cache->issuer);
    }
#endif
    
    if (cache->subject)
    {
        SECITEM_FreeItem(cache->subject, PR_TRUE);
    }
    if (SECSuccess != DPCache_Destroy(cache->dpp))
    {
        PORT_Assert(0);
        return SECFailure;
    }
    PORT_Free(cache);
    return SECSuccess;
}


static SECStatus NamedCRLCacheEntry_Create(NamedCRLCacheEntry** returned)
{
    NamedCRLCacheEntry* entry = NULL;
    if (!returned)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    *returned = NULL;
    entry = (NamedCRLCacheEntry*) PORT_ZAlloc(sizeof(NamedCRLCacheEntry));
    if (!entry)
    {
        return SECFailure;
    }
    *returned = entry;
    return SECSuccess;
}


static SECStatus NamedCRLCacheEntry_Destroy(NamedCRLCacheEntry* entry)
{
    if (!entry)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    if (entry->crl)
    {
        
        SECITEM_ZfreeItem(entry->crl, PR_TRUE);
    }
    if (entry->canonicalizedName)
    {
        SECITEM_FreeItem(entry->canonicalizedName, PR_TRUE);
    }
    PORT_Free(entry);
    return SECSuccess;
}


static PRIntn PR_CALLBACK FreeIssuer(PLHashEntry *he, PRIntn i, void *arg)
{
    CRLIssuerCache* issuer = NULL;
    SECStatus* rv = (SECStatus*) arg;

    PORT_Assert(he);
    if (!he)
    {
        return HT_ENUMERATE_NEXT;
    }
    issuer = (CRLIssuerCache*) he->value;
    PORT_Assert(issuer);
    if (issuer)
    {
        if (SECSuccess != IssuerCache_Destroy(issuer))
        {
            PORT_Assert(rv);
            if (rv)
            {
                *rv = SECFailure;
            }
            return HT_ENUMERATE_NEXT;
        }
    }
    return HT_ENUMERATE_NEXT;
}


static PRIntn PR_CALLBACK FreeNamedEntries(PLHashEntry *he, PRIntn i, void *arg)
{
    NamedCRLCacheEntry* entry = NULL;
    SECStatus* rv = (SECStatus*) arg;

    PORT_Assert(he);
    if (!he)
    {
        return HT_ENUMERATE_NEXT;
    }
    entry = (NamedCRLCacheEntry*) he->value;
    PORT_Assert(entry);
    if (entry)
    {
        if (SECSuccess != NamedCRLCacheEntry_Destroy(entry))
        {
            PORT_Assert(rv);
            if (rv)
            {
                *rv = SECFailure;
            }
            return HT_ENUMERATE_NEXT;
        }
    }
    return HT_ENUMERATE_NEXT;
}






SECStatus ShutdownCRLCache(void)
{
    SECStatus rv = SECSuccess;
    if (PR_FALSE == crlcache_initialized &&
        !crlcache.lock && !crlcache.issuers)
    {
        
        return SECSuccess;
    }
    if (PR_TRUE == crlcache_initialized &&
        (!crlcache.lock || !crlcache.issuers || !namedCRLCache.lock ||
         !namedCRLCache.entries))
    {
        
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    
    
    PL_HashTableEnumerateEntries(crlcache.issuers, &FreeIssuer, &rv);
    
    PL_HashTableDestroy(crlcache.issuers);
    crlcache.issuers = NULL;
    
#ifdef GLOBAL_RWLOCK
    NSSRWLock_Destroy(crlcache.lock);
#else
    PR_DestroyLock(crlcache.lock);
#endif
    crlcache.lock = NULL;

    

    
    PL_HashTableEnumerateEntries(namedCRLCache.entries, &FreeNamedEntries, &rv);
    
    PL_HashTableDestroy(namedCRLCache.entries);
    namedCRLCache.entries = NULL;
    
    PR_DestroyLock(namedCRLCache.lock);
    namedCRLCache.lock = NULL;

    crlcache_initialized = PR_FALSE;
    return rv;
}



static SECStatus DPCache_AddCRL(CRLDPCache* cache, CachedCrl* newcrl,
                                PRBool* added)
{
    CachedCrl** newcrls = NULL;
    PRUint32 i = 0;
    PORT_Assert(cache);
    PORT_Assert(newcrl);
    PORT_Assert(added);
    if (!cache || !newcrl || !added)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }

    *added = PR_FALSE;
    
    for (i=0;i<cache->ncrls;i++)
    {
        CachedCrl* existing = NULL;
        SECStatus rv = SECSuccess;
        PRBool dupe = PR_FALSE, updated = PR_FALSE;
        if (!cache->crls)
        {
            PORT_Assert(0);
            return SECFailure;
        }
        existing = cache->crls[i];
        if (!existing)
        {
            PORT_Assert(0);
            return SECFailure;
        }
        rv = CachedCrl_Compare(existing, newcrl, &dupe, &updated);
        if (SECSuccess != rv)
        {
            PORT_Assert(0);
            PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
            return SECFailure;
        }
        if (PR_TRUE == dupe)
        {
            
            PORT_SetError(SEC_ERROR_CRL_ALREADY_EXISTS);
            return SECSuccess;
        }
        if (PR_TRUE == updated)
        {
            

            if (SECSuccess != DPCache_RemoveCRL(cache, i))
            {
                PORT_Assert(0);
                PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
                return PR_FALSE;
            }
        }
    }

    newcrls = (CachedCrl**)PORT_Realloc(cache->crls,
        (cache->ncrls+1)*sizeof(CachedCrl*));
    if (!newcrls)
    {
        return SECFailure;
    }
    cache->crls = newcrls;
    cache->ncrls++;
    cache->crls[cache->ncrls-1] = newcrl;
    *added = PR_TRUE;
    return SECSuccess;
}


static SECStatus DPCache_RemoveCRL(CRLDPCache* cache, PRUint32 offset)
{
    CachedCrl* acrl = NULL;
    PORT_Assert(cache);
    if (!cache || (!cache->crls) || (!(offset<cache->ncrls)) )
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    acrl = cache->crls[offset];
    PORT_Assert(acrl);
    if (!acrl)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    cache->crls[offset] = cache->crls[cache->ncrls-1];
    cache->crls[cache->ncrls-1] = NULL;
    cache->ncrls--;
    if (cache->selected == acrl) {
        cache->selected = NULL;
    }
    if (SECSuccess != CachedCrl_Destroy(acrl))
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    return SECSuccess;
}









static PRBool TokenCRLStillExists(CERTSignedCrl* crl)
{
    NSSItem newsubject;
    SECItem subject;
    CK_ULONG crl_class;
    PRStatus status;
    PK11SlotInfo* slot = NULL;
    nssCryptokiObject instance;
    NSSArena* arena;
    PRBool xstatus = PR_TRUE;
    SECItem* oldSubject = NULL;

    PORT_Assert(crl);
    if (!crl)
    {
        return PR_FALSE;
    }
    slot = crl->slot;
    PORT_Assert(crl->slot);
    if (!slot)
    {
        return PR_FALSE;
    }
    oldSubject = &crl->crl.derName;
    PORT_Assert(oldSubject);
    if (!oldSubject)
    {
        return PR_FALSE;
    }

    


    
    instance.handle = crl->pkcs11ID;
    PORT_Assert(instance.handle);
    if (!instance.handle)
    {
        return PR_FALSE;
    }
    instance.token = PK11Slot_GetNSSToken(slot);
    PORT_Assert(instance.token);
    if (!instance.token)
    {
        return PR_FALSE;
    }
    instance.isTokenObject = PR_TRUE;
    instance.label = NULL;

    arena = NSSArena_Create();
    PORT_Assert(arena);
    if (!arena)
    {
        return PR_FALSE;
    }

    status = nssCryptokiCRL_GetAttributes(&instance,
                                          NULL,  
                                          arena,
                                          NULL,
                                          &newsubject,  
                                          &crl_class,   
                                          NULL,
                                          NULL);
    if (PR_SUCCESS == status)
    {
        subject.data = newsubject.data;
        subject.len = newsubject.size;
        if (SECITEM_CompareItem(oldSubject, &subject) != SECEqual)
        {
            xstatus = PR_FALSE;
        }
        if (CKO_NETSCAPE_CRL != crl_class)
        {
            xstatus = PR_FALSE;
        }
    }
    else
    {
        xstatus = PR_FALSE;
    }
    NSSArena_Destroy(arena);
    return xstatus;
}


static SECStatus CERT_VerifyCRL(
    CERTSignedCrl* crlobject,
    CERTCertificate* issuer,
    PRTime vfdate,
    void* wincx)
{
    return CERT_VerifySignedData(&crlobject->signatureWrap,
                                 issuer, vfdate, wincx);
}


static SECStatus CachedCrl_Verify(CRLDPCache* cache, CachedCrl* crlobject,
                          PRTime vfdate, void* wincx)
{
    











    if (!cache || !crlobject)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    if (PR_TRUE == GetOpaqueCRLFields(crlobject->crl)->decodingError)
    {
        crlobject->sigChecked = PR_TRUE; 

        PORT_SetError(SEC_ERROR_BAD_DER);
        return SECSuccess;
    }
    else
    {
        SECStatus signstatus = SECFailure;
        if (cache->issuer)
        {
            signstatus = CERT_VerifyCRL(crlobject->crl, cache->issuer, vfdate,
                                        wincx);
        }
        if (SECSuccess != signstatus)
        {
            if (!cache->issuer)
            {
                




            } else
            {
                crlobject->sigChecked = PR_TRUE;
            }
            PORT_SetError(SEC_ERROR_CRL_BAD_SIGNATURE);
            return SECSuccess;
        } else
        {
            crlobject->sigChecked = PR_TRUE;
            crlobject->sigValid = PR_TRUE;
        }
    }
    
    return SECSuccess;
}


static SECStatus DPCache_FetchFromTokens(CRLDPCache* cache, PRTime vfdate,
                                         void* wincx)
{
    SECStatus rv = SECSuccess;
    CERTCrlHeadNode head;
    if (!cache)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    
    memset(&head, 0, sizeof(head));
    head.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    rv = pk11_RetrieveCrls(&head, cache->subject, wincx);

    


    if (SECFailure == rv)
    {
        
        cache->invalid |= CRL_CACHE_LAST_FETCH_FAILED;
    } else
    {
        
        cache->invalid &= (~CRL_CACHE_LAST_FETCH_FAILED);
    }

    
    if (SECSuccess == rv)
    {
        CERTCrlNode* crlNode = NULL;

        for (crlNode = head.first; crlNode ; crlNode = crlNode->next)
        {
            CachedCrl* returned = NULL;
            CERTSignedCrl* crlobject = crlNode->crl;
            if (!crlobject)
            {
                PORT_Assert(0);
                continue;
            }
            rv = CachedCrl_Create(&returned, crlobject, CRL_OriginToken);
            if (SECSuccess == rv)
            {
                PRBool added = PR_FALSE;
                rv = DPCache_AddCRL(cache, returned, &added);
                if (PR_TRUE != added)
                {
                    rv = CachedCrl_Destroy(returned);
                    returned = NULL;
                }
                else if (vfdate)
                {
                    rv = CachedCrl_Verify(cache, returned, vfdate, wincx);
                }
            }
            else
            {
                

                cache->invalid |= CRL_CACHE_LAST_FETCH_FAILED;
            }
            if (SECFailure == rv)
            {
                break;
            }
        }
    }

    if (head.arena)
    {
        CERTCrlNode* crlNode = NULL;
        

        for (crlNode = head.first; crlNode ; crlNode = crlNode->next)
        {
            if (crlNode->crl)
            {
                SEC_DestroyCrl(crlNode->crl); 


            }
        }
        PORT_FreeArena(head.arena, PR_FALSE); 
    }

    return rv;
}

static SECStatus CachedCrl_GetEntry(CachedCrl* crl, SECItem* sn,
                                    CERTCrlEntry** returned)
{
    CERTCrlEntry* acrlEntry;
     
    PORT_Assert(crl);
    PORT_Assert(crl->entries);
    PORT_Assert(sn);
    PORT_Assert(returned);
    if (!crl || !sn || !returned || !crl->entries)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    acrlEntry = PL_HashTableLookup(crl->entries, (void*)sn);
    if (acrlEntry)
    {
        *returned = acrlEntry;
    }
    else
    {
        *returned = NULL;
    }
    return SECSuccess;
}


dpcacheStatus DPCache_Lookup(CRLDPCache* cache, SECItem* sn,
                         CERTCrlEntry** returned)
{
    SECStatus rv;
    if (!cache || !sn || !returned)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        
        return dpcacheCallerError;
    }
    *returned = NULL;
    if (0 != cache->invalid)
    {
        

        PORT_SetError(SEC_ERROR_CRL_INVALID);
        return dpcacheInvalidCacheError;
    }
    if (!cache->selected)
    {
        

        return dpcacheEmpty;
    }
    rv = CachedCrl_GetEntry(cache->selected, sn, returned);
    if (SECSuccess != rv)
    {
        return dpcacheLookupError;
    }
    else
    {
        if (*returned)
        {
            return dpcacheFoundEntry;
        }
        else
        {
            return dpcacheNoEntry;
        }
    }
}

#if defined(DPC_RWLOCK)

#define DPCache_LockWrite() \
{ \
    if (readlocked) \
    { \
        NSSRWLock_UnlockRead(cache->lock); \
    } \
    NSSRWLock_LockWrite(cache->lock); \
}

#define DPCache_UnlockWrite() \
{ \
    if (readlocked) \
    { \
        NSSRWLock_LockRead(cache->lock); \
    } \
    NSSRWLock_UnlockWrite(cache->lock); \
}

#else




#define DPCache_LockWrite() \
{ \
}

#define DPCache_UnlockWrite() \
{ \
}

#endif




static SECStatus DPCache_GetUpToDate(CRLDPCache* cache, CERTCertificate*
                                     issuer, PRBool readlocked, PRTime vfdate,
                                     void* wincx)
{
    


    SECStatus rv = SECSuccess;
    PRUint32 i = 0;
    PRBool forcedrefresh = PR_FALSE;
    PRBool dirty = PR_FALSE; 

    PRBool hastokenCRLs = PR_FALSE;
    PRTime now = 0;
    PRTime lastfetch = 0;
    PRBool mustunlock = PR_FALSE;

    if (!cache)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }

    









    forcedrefresh = cache->refresh;
    lastfetch = cache->lastfetch;
    if (PR_TRUE != forcedrefresh && 
        (!(cache->invalid & CRL_CACHE_LAST_FETCH_FAILED)))
    {
        now = PR_Now();
        hastokenCRLs = DPCache_HasTokenCRLs(cache);
    }
    if ( (0 == lastfetch) ||

         (PR_TRUE == forcedrefresh) ||

         (cache->invalid & CRL_CACHE_LAST_FETCH_FAILED) ||

         ( (PR_FALSE == hastokenCRLs) &&
           ( (now - cache->lastfetch > CRLCache_Empty_TokenFetch_Interval) ||
             (now < cache->lastfetch)) ) ||

         ( (PR_TRUE == hastokenCRLs) &&
           ((now - cache->lastfetch > CRLCache_TokenRefetch_Interval) ||
            (now < cache->lastfetch)) ) )
    {
        

        DPCache_LockWrite();
        
        if (lastfetch == cache->lastfetch)
        {
            
            rv = DPCache_FetchFromTokens(cache, vfdate, wincx);
            if (PR_TRUE == cache->refresh)
            {
                cache->refresh = PR_FALSE; 
            }
            dirty = PR_TRUE;
            cache->lastfetch = PR_Now();
        }
        DPCache_UnlockWrite();
    }

    



    if (( PR_TRUE != dirty) && (!now) )
    {
        now = PR_Now();
    }
    if ( (PR_TRUE == dirty) ||
         ( (now - cache->lastcheck > CRLCache_ExistenceCheck_Interval) ||
           (now < cache->lastcheck)) )
    {
        PRTime lastcheck = cache->lastcheck;
        mustunlock = PR_FALSE;
        
        for (i = 0; (i < cache->ncrls) ; i++)
        {
            CachedCrl* savcrl = cache->crls[i];
            if ( (!savcrl) || (savcrl && CRL_OriginToken != savcrl->origin))
            {
                
                continue;
            }
            if ((PR_TRUE != TokenCRLStillExists(savcrl->crl)))
            {
                
                
                if (PR_TRUE != mustunlock)
                {
                    DPCache_LockWrite();
                    mustunlock = PR_TRUE;
                }
                

                if (lastcheck == cache->lastcheck)
                {
                    
                    DPCache_RemoveCRL(cache, i);
                    dirty = PR_TRUE;
                }
                

            }
        }
        if (PR_TRUE == mustunlock)
        {
            cache->lastcheck = PR_Now();
            DPCache_UnlockWrite();
            mustunlock = PR_FALSE;
        }
    }

    
    if (issuer && (NULL == cache->issuer) &&
        (SECSuccess == CERT_CheckCertUsage(issuer, KU_CRL_SIGN)))
    {
        
        DPCache_LockWrite();
        if (!cache->issuer)
        {
            dirty = PR_TRUE;
            cache->issuer = CERT_DupCertificate(issuer);    
        }
        DPCache_UnlockWrite();
    }

    





    if (cache->issuer && vfdate )
    {
	mustunlock = PR_FALSE;
        
        for (i = 0; i < cache->ncrls ; i++)
        {
            CachedCrl* savcrl = cache->crls[i];
            if (!savcrl)
            {
                continue;
            }
            if (PR_TRUE != savcrl->sigChecked)
            {
                if (!mustunlock)
                {
                    DPCache_LockWrite();
                    mustunlock = PR_TRUE;
                }
                



                if ( (i<cache->ncrls) && (savcrl == cache->crls[i]) &&
                     (PR_TRUE != savcrl->sigChecked) )
                {
                    
                    CachedCrl_Verify(cache, savcrl, vfdate, wincx);
                    dirty = PR_TRUE;
                }
                

            }
            if (mustunlock && !dirty)
            {
                DPCache_UnlockWrite();
                mustunlock = PR_FALSE;
            }
        }
    }

    if (dirty || cache->mustchoose)
    {
        

	if (!mustunlock)
	{
	    DPCache_LockWrite();
	    mustunlock = PR_TRUE;
	}
        DPCache_SelectCRL(cache);
        cache->mustchoose = PR_FALSE;
    }
    if (mustunlock)
	DPCache_UnlockWrite();

    return rv;
}


static int SortCRLsByThisUpdate(const void* arg1, const void* arg2)
{
    PRTime timea, timeb;
    SECStatus rv = SECSuccess;
    CachedCrl* a, *b;

    a = *(CachedCrl**) arg1;
    b = *(CachedCrl**) arg2;

    if (!a || !b)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        rv = SECFailure;
    }

    if (SECSuccess == rv)
    {
        rv = DER_DecodeTimeChoice(&timea, &a->crl->crl.lastUpdate);
    }                       
    if (SECSuccess == rv)
    {
        rv = DER_DecodeTimeChoice(&timeb, &b->crl->crl.lastUpdate);
    }
    if (SECSuccess == rv)
    {
        if (timea > timeb)
        {
            return 1; 
        }
        if (timea < timeb )
        {
            return -1; 
        }
    }

    
    PORT_Assert(a != b); 
    return a>b?1:-1;
}








static int SortImperfectCRLs(const void* arg1, const void* arg2)
{
    CachedCrl* a, *b;

    a = *(CachedCrl**) arg1;
    b = *(CachedCrl**) arg2;

    if (!a || !b)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        PORT_Assert(0);
    }
    else
    {
        PRBool aDecoded = PR_FALSE, bDecoded = PR_FALSE;
        if ( (PR_TRUE == a->sigValid) && (PR_TRUE == b->sigValid) )
        {
            
            return SortCRLsByThisUpdate(arg1, arg2);
        }
        if (PR_TRUE == a->sigValid)
        {
            return 1; 
        }
        if (PR_TRUE == b->sigValid)
        {
            return -1; 
        }
        aDecoded = GetOpaqueCRLFields(a->crl)->decodingError;
        bDecoded = GetOpaqueCRLFields(b->crl)->decodingError;
        
        if ( (PR_FALSE == aDecoded) && (PR_FALSE == bDecoded) )
        {
            
            return SortCRLsByThisUpdate(arg1, arg2);
        }
        if (PR_FALSE == aDecoded)
        {
            return 1; 
        }
        if (PR_FALSE == bDecoded)
        {
            return -1; 
        }
        
    }
    
    PORT_Assert(a != b); 
    return a>b?1:-1;
}



static SECStatus DPCache_SelectCRL(CRLDPCache* cache)
{
    PRUint32 i;
    PRBool valid = PR_TRUE;
    CachedCrl* selected = NULL;

    PORT_Assert(cache);
    if (!cache)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    

    for (i = 0 ; i<cache->ncrls; i++)
    {
        if (!cache->crls[i] || !cache->crls[i]->sigChecked ||
            !cache->crls[i]->sigValid)
        {
            valid = PR_FALSE;
            break;
        }
    }
    if (PR_TRUE == valid)
    {
        
        cache->invalid &= (~CRL_CACHE_INVALID_CRLS);
    } else
    {
        
        cache->invalid |= CRL_CACHE_INVALID_CRLS;
    }

    if (cache->invalid)
    {
        
        if (cache->selected)
        {
            cache->selected = NULL;
        }
        
        qsort(cache->crls, cache->ncrls, sizeof(CachedCrl*),
              SortImperfectCRLs);
        return SECSuccess;
    }
    
    qsort(cache->crls, cache->ncrls, sizeof(CachedCrl*),
          SortCRLsByThisUpdate);

    if (cache->ncrls)
    {
        
        selected = cache->crls[cache->ncrls-1];
    
        
        if (SECSuccess != CachedCrl_Populate(selected))
        {
            return SECFailure;
        }
    }

    cache->selected = selected;

    return SECSuccess;
}


static SECStatus DPCache_Create(CRLDPCache** returned, CERTCertificate* issuer,
                         const SECItem* subject, SECItem* dp)
{
    CRLDPCache* cache = NULL;
    PORT_Assert(returned);
    
    if (!returned || !subject)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    *returned = NULL;
    cache = PORT_ZAlloc(sizeof(CRLDPCache));
    if (!cache)
    {
        return SECFailure;
    }
#ifdef DPC_RWLOCK
    cache->lock = NSSRWLock_New(NSS_RWLOCK_RANK_NONE, NULL);
#else
    cache->lock = PR_NewLock();
#endif
    if (!cache->lock)
    {
	PORT_Free(cache);
        return SECFailure;
    }
    if (issuer)
    {
        cache->issuer = CERT_DupCertificate(issuer);
    }
    cache->distributionPoint = SECITEM_DupItem(dp);
    cache->subject = SECITEM_DupItem(subject);
    cache->lastfetch = 0;
    cache->lastcheck = 0;
    *returned = cache;
    return SECSuccess;
}


static SECStatus IssuerCache_Create(CRLIssuerCache** returned,
                             CERTCertificate* issuer,
                             const SECItem* subject, const SECItem* dp)
{
    SECStatus rv = SECSuccess;
    CRLIssuerCache* cache = NULL;
    PORT_Assert(returned);
    PORT_Assert(subject);
    
    if (!returned || !subject)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    *returned = NULL;
    cache = (CRLIssuerCache*) PORT_ZAlloc(sizeof(CRLIssuerCache));
    if (!cache)
    {
        return SECFailure;
    }
    cache->subject = SECITEM_DupItem(subject);
#ifdef XCRL
    cache->lock = NSSRWLock_New(NSS_RWLOCK_RANK_NONE, NULL);
    if (!cache->lock)
    {
        rv = SECFailure;
    }
    if (SECSuccess == rv && issuer)
    {
        cache->issuer = CERT_DupCertificate(issuer);
        if (!cache->issuer)
        {
            rv = SECFailure;
        }
    }
#endif
    if (SECSuccess != rv)
    {
        PORT_Assert(SECSuccess == IssuerCache_Destroy(cache));
        return SECFailure;
    }
    *returned = cache;
    return SECSuccess;
}


static SECStatus IssuerCache_AddDP(CRLIssuerCache* cache,
                                   CERTCertificate* issuer,
                                   const SECItem* subject,
                                   const SECItem* dp,
                                   CRLDPCache** newdpc)
{
    
    if (!cache || !subject || !newdpc)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    if (!dp)
    {
        
        SECStatus rv = DPCache_Create(&cache->dpp, issuer, subject, NULL);
        if (SECSuccess == rv)
        {
            *newdpc = cache->dpp;
            return SECSuccess;
        }
    }
    else
    {
        
        PORT_Assert(dp);
        

    }
    return SECFailure;
}


static SECStatus CRLCache_AddIssuer(CRLIssuerCache* issuer)
{    
    PORT_Assert(issuer);
    PORT_Assert(crlcache.issuers);
    if (!issuer || !crlcache.issuers)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    if (NULL == PL_HashTableAdd(crlcache.issuers, (void*) issuer->subject,
                                (void*) issuer))
    {
        return SECFailure;
    }
    return SECSuccess;
}


static SECStatus CRLCache_GetIssuerCache(CRLCache* cache,
                                         const SECItem* subject,
                                         CRLIssuerCache** returned)
{
    
    SECStatus rv = SECSuccess;
    PORT_Assert(cache);
    PORT_Assert(subject);
    PORT_Assert(returned);
    PORT_Assert(crlcache.issuers);
    if (!cache || !subject || !returned || !crlcache.issuers)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        rv = SECFailure;
    }

    if (SECSuccess == rv)
    {
        *returned = (CRLIssuerCache*) PL_HashTableLookup(crlcache.issuers,
                                                         (void*) subject);
    }

    return rv;
}


static CERTSignedCrl* GetBestCRL(CRLDPCache* cache, PRBool entries)
{
    CachedCrl* acrl = NULL;

    PORT_Assert(cache);
    if (!cache)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return NULL;
    }

    if (0 == cache->ncrls)
    {
        
        PORT_SetError(SEC_ERROR_CRL_NOT_FOUND);
        return NULL;
    }    

    
    if (cache->selected)
    {
        return SEC_DupCrl(cache->selected->crl);
    }

    
    acrl = cache->crls[cache->ncrls-1];

    if (acrl && (PR_FALSE == GetOpaqueCRLFields(acrl->crl)->decodingError) )
    {
        SECStatus rv = SECSuccess;
        if (PR_TRUE == entries)
        {
            rv = CERT_CompleteCRLDecodeEntries(acrl->crl);
        }
        if (SECSuccess == rv)
        {
            return SEC_DupCrl(acrl->crl);
        }
    }

    PORT_SetError(SEC_ERROR_CRL_NOT_FOUND);
    return NULL;
}


static CRLDPCache* IssuerCache_GetDPCache(CRLIssuerCache* cache, const SECItem* dp)
{
    CRLDPCache* dpp = NULL;
    PORT_Assert(cache);
    


    PORT_Assert(NULL == dp);
    if (!cache || dp)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return NULL;
    }
#ifdef XCRL
    NSSRWLock_LockRead(cache->lock);
#endif
    dpp = cache->dpp;
#ifdef XCRL
    NSSRWLock_UnlockRead(cache->lock);
#endif
    return dpp;
}




SECStatus AcquireDPCache(CERTCertificate* issuer, const SECItem* subject,
                         const SECItem* dp, PRTime t, void* wincx,
                         CRLDPCache** dpcache, PRBool* writeLocked)
{
    SECStatus rv = SECSuccess;
    CRLIssuerCache* issuercache = NULL;
#ifdef GLOBAL_RWLOCK
    PRBool globalwrite = PR_FALSE;
#endif
    PORT_Assert(crlcache.lock);
    if (!crlcache.lock)
    {
        
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
#ifdef GLOBAL_RWLOCK
    NSSRWLock_LockRead(crlcache.lock);
#else
    PR_Lock(crlcache.lock);
#endif
    rv = CRLCache_GetIssuerCache(&crlcache, subject, &issuercache);
    if (SECSuccess != rv)
    {
#ifdef GLOBAL_RWLOCK
        NSSRWLock_UnlockRead(crlcache.lock);
#else
        PR_Unlock(crlcache.lock);
#endif
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    if (!issuercache)
    {
        


        
        rv = IssuerCache_Create(&issuercache, issuer, subject, dp);
        if (SECSuccess == rv && !issuercache)
        {
            PORT_Assert(issuercache);
            rv = SECFailure;
        }

        if (SECSuccess == rv)
        {
            

            rv = IssuerCache_AddDP(issuercache, issuer, subject, dp, dpcache);
        }

        if (SECSuccess == rv)
        {
            

            *writeLocked = PR_TRUE;
#ifdef DPC_RWLOCK
            NSSRWLock_LockWrite((*dpcache)->lock);
#else
            PR_Lock((*dpcache)->lock);
#endif
        }
        
        if (SECSuccess == rv)
        {
            

#ifdef GLOBAL_RWLOCK
            CRLIssuerCache* existing = NULL;
            NSSRWLock_UnlockRead(crlcache.lock);
            

            NSSRWLock_LockWrite(crlcache.lock);
            globalwrite = PR_TRUE;
            rv = CRLCache_GetIssuerCache(&crlcache, subject, &existing);
            if (!existing)
            {
#endif
                rv = CRLCache_AddIssuer(issuercache);
                if (SECSuccess != rv)
                {
                    
                    rv = SECFailure;
                }
#ifdef GLOBAL_RWLOCK
            }
            else
            {
                
                IssuerCache_Destroy(issuercache); 
                issuercache = existing; 
                *dpcache = IssuerCache_GetDPCache(issuercache, dp);
            }
#endif
        }

        

#ifdef GLOBAL_RWLOCK
        if (PR_TRUE == globalwrite)
        {
            NSSRWLock_UnlockWrite(crlcache.lock);
            globalwrite = PR_FALSE;
        }
        else
        {
            NSSRWLock_UnlockRead(crlcache.lock);
        }
#else
        PR_Unlock(crlcache.lock);
#endif

        
        if (SECSuccess != rv && issuercache)
        {
            if (PR_TRUE == *writeLocked)
            {
#ifdef DPC_RWLOCK
                NSSRWLock_UnlockWrite((*dpcache)->lock);
#else
                PR_Unlock((*dpcache)->lock);
#endif
            }
            IssuerCache_Destroy(issuercache);
            issuercache = NULL;
        }

        if (SECSuccess != rv)
        {
            return SECFailure;
        }
    } else
    {
#ifdef GLOBAL_RWLOCK
        NSSRWLock_UnlockRead(crlcache.lock);
#else
        PR_Unlock(crlcache.lock);
#endif
        *dpcache = IssuerCache_GetDPCache(issuercache, dp);
    }
    
    
    if (PR_FALSE == *writeLocked)
    {
#ifdef DPC_RWLOCK
        NSSRWLock_LockRead((*dpcache)->lock);
#else
        PR_Lock((*dpcache)->lock);
#endif
    }
    
    if (SECSuccess == rv)
    {
        
        PORT_Assert(*dpcache);
        if (*dpcache)
        {
            
            rv = DPCache_GetUpToDate(*dpcache, issuer, PR_FALSE == *writeLocked,
                                     t, wincx);
        }
        else
        {
            rv = SECFailure;
        }
    }
    return rv;
}


void ReleaseDPCache(CRLDPCache* dpcache, PRBool writeLocked)
{
    if (!dpcache)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return;
    }
#ifdef DPC_RWLOCK
    if (PR_TRUE == writeLocked)
    {
        NSSRWLock_UnlockWrite(dpcache->lock);
    }
    else
    {
        NSSRWLock_UnlockRead(dpcache->lock);
    }
#else
    PR_Unlock(dpcache->lock);
#endif
}

SECStatus
cert_CheckCertRevocationStatus(CERTCertificate* cert, CERTCertificate* issuer,
                               const SECItem* dp, PRTime t, void *wincx,
                               CERTRevocationStatus *revStatus,
                               CERTCRLEntryReasonCode *revReason)
{
    PRBool lockedwrite = PR_FALSE;
    SECStatus rv = SECSuccess;
    CRLDPCache* dpcache = NULL;
    CERTRevocationStatus status = certRevocationStatusRevoked;
    CERTCRLEntryReasonCode reason = crlEntryReasonUnspecified;
    CERTCrlEntry* entry = NULL;
    dpcacheStatus ds;

    if (!cert || !issuer)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }

    if (revStatus)
    {
        *revStatus = status;
    }
    if (revReason)
    {
        *revReason = reason;
    }

    if (t && SECSuccess != CERT_CheckCertValidTimes(issuer, t, PR_FALSE))
    {
        


        PORT_SetError(SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE);
        return SECFailure;
    }

    rv = AcquireDPCache(issuer, &issuer->derSubject, dp, t, wincx, &dpcache,
                        &lockedwrite);
    PORT_Assert(SECSuccess == rv);
    if (SECSuccess != rv)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    
    ds = DPCache_Lookup(dpcache, &cert->serialNumber, &entry);
    switch (ds)
    {
        case dpcacheFoundEntry:
            PORT_Assert(entry);
            
            if (entry->revocationDate.data && entry->revocationDate.len)
            {
                PRTime revocationDate = 0;
                if (SECSuccess == DER_DecodeTimeChoice(&revocationDate,
                                               &entry->revocationDate))
                {
                    


                    if (t>=revocationDate)
                    {
                        rv = SECFailure;
                    }
                    else
                    {
                        status = certRevocationStatusValid;
                    }
                }
                else
                {
                    

                    rv = SECFailure;
                }
            }
            else
            {
                
                rv = SECFailure;
            }
            if (SECFailure == rv)
            {
                SECStatus rv2 = CERT_FindCRLEntryReasonExten(entry, &reason);
                PORT_SetError(SEC_ERROR_REVOKED_CERTIFICATE);
            }
            break;

        case dpcacheEmpty:
            
            status = certRevocationStatusUnknown;
            break;

        case dpcacheNoEntry:
            status = certRevocationStatusValid;
            break;

        case dpcacheInvalidCacheError:
            

            if (!t)
            {
                status = certRevocationStatusUnknown;
            }
            break;

        default:
            
            break;
    }

    ReleaseDPCache(dpcache, lockedwrite);
    if (revStatus)
    {
        *revStatus = status;
    }
    if (revReason)
    {
        *revReason = reason;
    }
    return rv;
}


SECStatus
CERT_CheckCRL(CERTCertificate* cert, CERTCertificate* issuer,
              const SECItem* dp, PRTime t, void* wincx)
{
    return cert_CheckCertRevocationStatus(cert, issuer, dp, t, wincx,
                                          NULL, NULL);
}


CERTSignedCrl *
SEC_FindCrlByName(CERTCertDBHandle *handle, SECItem *crlKey, int type)
{
    CERTSignedCrl* acrl = NULL;
    CRLDPCache* dpcache = NULL;
    SECStatus rv = SECSuccess;
    PRBool writeLocked = PR_FALSE;

    if (!crlKey)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }

    rv = AcquireDPCache(NULL, crlKey, NULL, 0, NULL, &dpcache, &writeLocked);
    if (SECSuccess == rv)
    {
        acrl = GetBestCRL(dpcache, PR_TRUE); 

        ReleaseDPCache(dpcache, writeLocked);
    }
    return acrl;
}



void CERT_CRLCacheRefreshIssuer(CERTCertDBHandle* dbhandle, SECItem* crlKey)
{
    CRLDPCache* cache = NULL;
    SECStatus rv = SECSuccess;
    PRBool writeLocked = PR_FALSE;
    PRBool readlocked;

    (void) dbhandle; 

    

    rv = AcquireDPCache(NULL, crlKey, NULL, 0, NULL, &cache, &writeLocked);
    if (SECSuccess != rv)
    {
        return;
    }
    
    readlocked = (writeLocked == PR_TRUE? PR_FALSE : PR_TRUE);
    DPCache_LockWrite();
    cache->refresh = PR_TRUE;
    DPCache_UnlockWrite();
    ReleaseDPCache(cache, writeLocked);
    return;
}


SECStatus CERT_CacheCRL(CERTCertDBHandle* dbhandle, SECItem* newdercrl)
{
    CRLDPCache* cache = NULL;
    SECStatus rv = SECSuccess;
    PRBool writeLocked = PR_FALSE;
    PRBool readlocked;
    CachedCrl* returned = NULL;
    PRBool added = PR_FALSE;
    CERTSignedCrl* newcrl = NULL;
    int realerror = 0;
    
    if (!dbhandle || !newdercrl)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    
    newcrl = CERT_DecodeDERCrlWithFlags(NULL, newdercrl, SEC_CRL_TYPE,
                                        CRL_DECODE_DONT_COPY_DER |
                                        CRL_DECODE_SKIP_ENTRIES);

    if (!newcrl)
    {
        return SECFailure;
    }

    

    rv = AcquireDPCache(NULL,
                        &newcrl->crl.derName,
                        NULL, 0, NULL, &cache, &writeLocked);
    if (SECSuccess == rv)
    {
        readlocked = (writeLocked == PR_TRUE? PR_FALSE : PR_TRUE);
    
        rv = CachedCrl_Create(&returned, newcrl, CRL_OriginExplicit);
        if (SECSuccess == rv && returned)
        {
            DPCache_LockWrite();
            rv = DPCache_AddCRL(cache, returned, &added);
            if (PR_TRUE != added)
            {
                realerror = PORT_GetError();
                CachedCrl_Destroy(returned);
                returned = NULL;
            }
            DPCache_UnlockWrite();
        }
    
        ReleaseDPCache(cache, writeLocked);
    
        if (!added)
        {
            rv = SECFailure;
        }
    }
    SEC_DestroyCrl(newcrl); 


    if (realerror)
    {
        PORT_SetError(realerror);
    }
    return rv;
}


SECStatus CERT_UncacheCRL(CERTCertDBHandle* dbhandle, SECItem* olddercrl)
{
    CRLDPCache* cache = NULL;
    SECStatus rv = SECSuccess;
    PRBool writeLocked = PR_FALSE;
    PRBool readlocked;
    PRBool removed = PR_FALSE;
    PRUint32 i;
    CERTSignedCrl* oldcrl = NULL;
    
    if (!dbhandle || !olddercrl)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    
    oldcrl = CERT_DecodeDERCrlWithFlags(NULL, olddercrl, SEC_CRL_TYPE,
                                        CRL_DECODE_DONT_COPY_DER |
                                        CRL_DECODE_SKIP_ENTRIES);

    if (!oldcrl)
    {
        
        return SECFailure;
    }

    rv = AcquireDPCache(NULL,
                        &oldcrl->crl.derName,
                        NULL, 0, NULL, &cache, &writeLocked);
    if (SECSuccess == rv)
    {
        CachedCrl* returned = NULL;

        readlocked = (writeLocked == PR_TRUE? PR_FALSE : PR_TRUE);
    
        rv = CachedCrl_Create(&returned, oldcrl, CRL_OriginExplicit);
        if (SECSuccess == rv && returned)
        {
            DPCache_LockWrite();
            for (i=0;i<cache->ncrls;i++)
            {
                PRBool dupe = PR_FALSE, updated = PR_FALSE;
                rv = CachedCrl_Compare(returned, cache->crls[i],
                                                      &dupe, &updated);
                if (SECSuccess != rv)
                {
                    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
                    break;
                }
                if (PR_TRUE == dupe)
                {
                    rv = DPCache_RemoveCRL(cache, i); 
                    if (SECSuccess == rv) {
                        cache->mustchoose = PR_TRUE;
                        removed = PR_TRUE;
                    }
                    break;
                }
            }
            
            DPCache_UnlockWrite();

            if (SECSuccess != CachedCrl_Destroy(returned) ) {
                rv = SECFailure;
            }
        }

        ReleaseDPCache(cache, writeLocked);
    }
    if (SECSuccess != SEC_DestroyCrl(oldcrl) ) { 
        
        rv = SECFailure;
    }
    if (SECSuccess == rv && PR_TRUE != removed)
    {
        PORT_SetError(SEC_ERROR_CRL_NOT_FOUND);
    }
    return rv;
}

SECStatus cert_AcquireNamedCRLCache(NamedCRLCache** returned)
{
    PORT_Assert(returned);
    if (!namedCRLCache.lock)
    {
        PORT_Assert(0);
        return SECFailure;
    }
    PR_Lock(namedCRLCache.lock);
    *returned = &namedCRLCache;
    return SECSuccess;
}




SECStatus cert_FindCRLByGeneralName(NamedCRLCache* ncc,
                                    const SECItem* canonicalizedName,
                                    NamedCRLCacheEntry** retEntry)
{
    if (!ncc || !canonicalizedName || !retEntry)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    *retEntry = (NamedCRLCacheEntry*) PL_HashTableLookup(namedCRLCache.entries,
                                         (void*) canonicalizedName);
    return SECSuccess;
}

SECStatus cert_ReleaseNamedCRLCache(NamedCRLCache* ncc)
{
    if (!ncc)
    {
        return SECFailure;
    }
    if (!ncc->lock)
    {
        PORT_Assert(0);
        return SECFailure;
    }
    PR_Unlock(namedCRLCache.lock);
    return SECSuccess;
}


static SECStatus addCRLToCache(CERTCertDBHandle* dbhandle, SECItem* crl,
                                    const SECItem* canonicalizedName,
                                    NamedCRLCacheEntry** newEntry)
{
    SECStatus rv = SECSuccess;
    NamedCRLCacheEntry* entry = NULL;

    
    if (SECSuccess != NamedCRLCacheEntry_Create(newEntry) || !*newEntry)
    {
        
        SECITEM_ZfreeItem(crl, PR_TRUE);
        return SECFailure;
    }
    entry = *newEntry;
    entry->crl = crl; 
    entry->lastAttemptTime = PR_Now();
    entry->canonicalizedName = SECITEM_DupItem(canonicalizedName);
    if (!entry->canonicalizedName)
    {
        rv = NamedCRLCacheEntry_Destroy(entry); 
        PORT_Assert(SECSuccess == rv);
        return SECFailure;
    }
    
    if (SECSuccess == CERT_CacheCRL(dbhandle, entry->crl))
    {
        entry->inCRLCache = PR_TRUE;
        entry->successfulInsertionTime = entry->lastAttemptTime;
    }
    else
    {
        switch (PR_GetError())
        {
            case SEC_ERROR_CRL_ALREADY_EXISTS:
                entry->dupe = PR_TRUE;
                break;

            case SEC_ERROR_BAD_DER:
                entry->badDER = PR_TRUE;
                break;

            
            default:
                entry->unsupported = PR_TRUE;
                break;
        }
        rv = SECFailure;
        
        SECITEM_ZfreeItem(entry->crl, PR_TRUE);
        entry->crl = NULL;
    }
    return rv;
}




SECStatus cert_CacheCRLByGeneralName(CERTCertDBHandle* dbhandle, SECItem* crl,
                                     const SECItem* canonicalizedName)
{
    NamedCRLCacheEntry* oldEntry, * newEntry = NULL;
    NamedCRLCache* ncc = NULL;
    SECStatus rv = SECSuccess, rv2;

    PORT_Assert(namedCRLCache.lock);
    PORT_Assert(namedCRLCache.entries);

    if (!crl || !canonicalizedName)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    rv = cert_AcquireNamedCRLCache(&ncc);
    PORT_Assert(SECSuccess == rv);
    if (SECSuccess != rv)
    {
        SECITEM_ZfreeItem(crl, PR_TRUE);
        return SECFailure;
    }
    rv = cert_FindCRLByGeneralName(ncc, canonicalizedName, &oldEntry);
    PORT_Assert(SECSuccess == rv);
    if (SECSuccess != rv)
    {
        rv = cert_ReleaseNamedCRLCache(ncc);
        SECITEM_ZfreeItem(crl, PR_TRUE);
        return SECFailure;
    }
    if (SECSuccess == addCRLToCache(dbhandle, crl, canonicalizedName,
                                    &newEntry) )
    {
        if (!oldEntry)
        {
            
            if (NULL == PL_HashTableAdd(namedCRLCache.entries,
                                        (void*) newEntry->canonicalizedName,
                                        (void*) newEntry))
            {
                PORT_Assert(0);
                rv2 = NamedCRLCacheEntry_Destroy(newEntry);
                PORT_Assert(SECSuccess == rv2);
                rv = SECFailure;
            }
        }
        else
        {
            PRBool removed;
            
            if (oldEntry->inCRLCache)
            {
                rv = CERT_UncacheCRL(dbhandle, oldEntry->crl);
                PORT_Assert(SECSuccess == rv);
            }
            removed = PL_HashTableRemove(namedCRLCache.entries,
                                      (void*) oldEntry->canonicalizedName);
            PORT_Assert(removed);
            if (!removed)
            {
                rv = SECFailure;
		
            }
            else
            {
                rv2 = NamedCRLCacheEntry_Destroy(oldEntry);
                PORT_Assert(SECSuccess == rv2);
            }
            if (NULL == PL_HashTableAdd(namedCRLCache.entries,
                                      (void*) newEntry->canonicalizedName,
                                      (void*) newEntry))
            {
                PORT_Assert(0);
                rv = SECFailure;
            }
        }
    } else
    {
        
        if (!oldEntry)
        {
            
            if (NULL == PL_HashTableAdd(namedCRLCache.entries,
                                        (void*) newEntry->canonicalizedName,
                                        (void*) newEntry))
            {
                PORT_Assert(0);
                rv = SECFailure;
            }
        }
        else
        {
            if (oldEntry->inCRLCache)
            {
                
                oldEntry-> lastAttemptTime = newEntry->lastAttemptTime;
                
                rv = NamedCRLCacheEntry_Destroy(newEntry);
                PORT_Assert(SECSuccess == rv);
            }
            else
            {
                
                PRBool removed = PL_HashTableRemove(namedCRLCache.entries,
                                          (void*) oldEntry->canonicalizedName);
                PORT_Assert(removed);
                if (!removed)
                {
		    
                    rv = SECFailure;
                }
                else
                {
                    rv2 = NamedCRLCacheEntry_Destroy(oldEntry);
                    PORT_Assert(SECSuccess == rv2);
                }
                if (NULL == PL_HashTableAdd(namedCRLCache.entries,
                                          (void*) newEntry->canonicalizedName,
                                          (void*) newEntry))
                {
                    PORT_Assert(0);
                    rv = SECFailure;
                }
            }
        }
    }
    rv2 = cert_ReleaseNamedCRLCache(ncc);
    PORT_Assert(SECSuccess == rv2);

    return rv;
}

static SECStatus CachedCrl_Create(CachedCrl** returned, CERTSignedCrl* crl,
                                  CRLOrigin origin)
{
    CachedCrl* newcrl = NULL;
    if (!returned)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    newcrl = PORT_ZAlloc(sizeof(CachedCrl));
    if (!newcrl)
    {
        return SECFailure;
    }
    newcrl->crl = SEC_DupCrl(crl);
    newcrl->origin = origin;
    *returned = newcrl;
    return SECSuccess;
}


static SECStatus CachedCrl_Depopulate(CachedCrl* crl)
{
    if (!crl)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
     
    if (crl->entries)
    {
        PL_HashTableDestroy(crl->entries);
        crl->entries = NULL;
    }

    
    if (crl->prebuffer)
    {
        PreAllocator_Destroy(crl->prebuffer);
        crl->prebuffer = NULL;
    }
    return SECSuccess;
}

static SECStatus CachedCrl_Destroy(CachedCrl* crl)
{
    if (!crl)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    CachedCrl_Depopulate(crl);
    SEC_DestroyCrl(crl->crl);
    PORT_Free(crl);
    return SECSuccess;
}


static SECStatus CachedCrl_Populate(CachedCrl* crlobject)
{
    SECStatus rv = SECFailure;
    CERTCrlEntry** crlEntry = NULL;
    PRUint32 numEntries = 0;

    if (!crlobject)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    
    rv = CERT_CompleteCRLDecodeEntries(crlobject->crl);
    if (SECSuccess != rv)
    {
        crlobject->unbuildable = PR_TRUE; 
        return SECFailure;
    }

    if (crlobject->entries && crlobject->prebuffer)
    {
        
        return SECSuccess;
    }

        
    
    for (crlEntry = crlobject->crl->crl.entries; crlEntry && *crlEntry;
         crlEntry++)
    {
        numEntries++;
    }
    crlobject->prebuffer = PreAllocator_Create(numEntries*sizeof(PLHashEntry));
    PORT_Assert(crlobject->prebuffer);
    if (!crlobject->prebuffer)
    {
        return SECFailure;
    }
    
    crlobject->entries = PL_NewHashTable(0, SECITEM_Hash, SECITEM_HashCompare,
                         PL_CompareValues, &preAllocOps, crlobject->prebuffer);
    PORT_Assert(crlobject->entries);
    if (!crlobject->entries)
    {
        return SECFailure;
    }
    
    for (crlEntry = crlobject->crl->crl.entries; crlEntry && *crlEntry;
         crlEntry++)
    {
        PL_HashTableAdd(crlobject->entries, &(*crlEntry)->serialNumber,
                        *crlEntry);
    }

    return SECSuccess;
}


static PRBool DPCache_HasTokenCRLs(CRLDPCache* cache)
{
    PRBool answer = PR_FALSE;
    PRUint32 i;
    for (i=0;i<cache->ncrls;i++)
    {
        if (cache->crls[i] && (CRL_OriginToken == cache->crls[i]->origin) )
        {
            answer = PR_TRUE;
            break;
        }
    }
    return answer;
}






static SECStatus CachedCrl_Compare(CachedCrl* a, CachedCrl* b, PRBool* isDupe,
                                PRBool* isUpdated)
{
    PORT_Assert(a);
    PORT_Assert(b);
    PORT_Assert(isDupe);
    PORT_Assert(isUpdated);
    if (!a || !b || !isDupe || !isUpdated || !a->crl || !b->crl)
    {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }

    *isDupe = *isUpdated = PR_FALSE;

    if (a == b)
    {
        
        *isDupe = PR_TRUE;
        *isUpdated = PR_FALSE;
        return SECSuccess;
    }
    if (b->origin != a->origin)
    {
        

        return SECSuccess;
    }
    if (CRL_OriginToken == b->origin)
    {
        

        if ( (b->crl->slot == a->crl->slot) &&
             (b->crl->pkcs11ID == a->crl->pkcs11ID) )
        {
            
            
            if ( SECEqual == SECITEM_CompareItem(b->crl->derCrl,
                                                 a->crl->derCrl) )
            {
                *isDupe = PR_TRUE;
            }
            else
            {
                *isUpdated = PR_TRUE;
            }
        }
        return SECSuccess;
    }
    if (CRL_OriginExplicit == b->origin)
    {
        



        if (b->crl->derCrl == a->crl->derCrl)
        {
            *isDupe = PR_TRUE;
        }
    }
    return SECSuccess;
}


SECStatus DPCache_GetAllCRLs(CRLDPCache* dpc, PRArenaPool* arena,
                             CERTSignedCrl*** crls, PRUint16* status)
{
    CERTSignedCrl** allcrls;
    PRUint32 index;
    if (!dpc || !crls || !status)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    *status = dpc->invalid;
    *crls = NULL;
    if (!dpc->ncrls)
    {
        
        return SECSuccess;
    }
    allcrls = PORT_ArenaZNewArray(arena, CERTSignedCrl*, dpc->ncrls +1);
    if (!allcrls)
    {
        return SECFailure;
    }
    for (index=0; index < dpc->ncrls ; index ++) {
        CachedCrl* cachedcrl = dpc->crls[index];
        if (!cachedcrl || !cachedcrl->crl)
        {
            PORT_Assert(0); 
            continue;
        }
        allcrls[index] = SEC_DupCrl(cachedcrl->crl);
    }
    *crls = allcrls;
    return SECSuccess;
}

static CachedCrl* DPCache_FindCRL(CRLDPCache* cache, CERTSignedCrl* crl)
{
    PRUint32 index;
    CachedCrl* cachedcrl = NULL;
    for (index=0; index < cache->ncrls ; index ++) {
        cachedcrl = cache->crls[index];
        if (!cachedcrl || !cachedcrl->crl)
        {
            PORT_Assert(0); 
            continue;
        }
        if (cachedcrl->crl == crl) {
            break;
        }
    }
    return cachedcrl;
}


SECStatus DPCache_GetCRLEntry(CRLDPCache* cache, PRBool readlocked,
                              CERTSignedCrl* crl, SECItem* sn,
                              CERTCrlEntry** returned)
{
    CachedCrl* cachedcrl = NULL;
    if (!cache || !crl || !sn || !returned)
    {
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    *returned = NULL;
    
    cachedcrl = DPCache_FindCRL(cache, crl);
    if (!cachedcrl) {
        PORT_SetError(SEC_ERROR_CRL_NOT_FOUND);
        return SECFailure;
    }

    if (cachedcrl->unbuildable) {
        
        PORT_SetError(SEC_ERROR_BAD_DER);
        return SECFailure;
    }
    
    if (!cachedcrl->entries || !cachedcrl->prebuffer) {
        DPCache_LockWrite();
        CachedCrl_Populate(cachedcrl);
        DPCache_UnlockWrite();
    }

           
    return CachedCrl_GetEntry(cachedcrl, sn, returned);
}

