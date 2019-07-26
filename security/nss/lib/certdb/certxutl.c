








#include "cert.h"
#include "secitem.h"
#include "secoid.h"
#include "secder.h"
#include "secasn1.h"
#include "certxutl.h"
#include "secerr.h"

#ifdef OLD
#include "ocspti.h"	

#endif

static CERTCertExtension *
GetExtension (CERTCertExtension **extensions, SECItem *oid)
{
    CERTCertExtension **exts;
    CERTCertExtension *ext = NULL;
    SECComparison comp;

    exts = extensions;
    
    if (exts) {
	while ( *exts ) {
	    ext = *exts;
	    comp = SECITEM_CompareItem(oid, &ext->id);
	    if ( comp == SECEqual ) 
		break;

	    exts++;
	}
	return (*exts ? ext : NULL);
    }
    return (NULL);
}

SECStatus
cert_FindExtensionByOID (CERTCertExtension **extensions, SECItem *oid, SECItem *value)
{
    CERTCertExtension *ext;
    SECStatus rv = SECSuccess;
    
    ext = GetExtension (extensions, oid);
    if (ext == NULL) {
	PORT_SetError (SEC_ERROR_EXTENSION_NOT_FOUND);
	return (SECFailure);
    }
    if (value)
	rv = SECITEM_CopyItem(NULL, value, &ext->value);
    return (rv);
}
    

SECStatus
CERT_GetExtenCriticality (CERTCertExtension **extensions, int tag, PRBool *isCritical)
{
    CERTCertExtension *ext;
    SECOidData *oid;

    if (!isCritical)
	return (SECSuccess);
    
    
    oid = SECOID_FindOIDByTag((SECOidTag)tag);
    if ( !oid ) {
	return(SECFailure);
    }
    ext = GetExtension (extensions, &oid->oid);
    if (ext == NULL) {
	PORT_SetError (SEC_ERROR_EXTENSION_NOT_FOUND);
	return (SECFailure);
    }

    

    if (ext->critical.data == NULL)
	*isCritical = PR_FALSE;
    else
	*isCritical = (ext->critical.data[0] == 0xff) ? PR_TRUE : PR_FALSE;
    return (SECSuccess);    
}

SECStatus
cert_FindExtension(CERTCertExtension **extensions, int tag, SECItem *value)
{
    SECOidData *oid;
    
    oid = SECOID_FindOIDByTag((SECOidTag)tag);
    if ( !oid ) {
	return(SECFailure);
    }

    return(cert_FindExtensionByOID(extensions, &oid->oid, value));
}


typedef struct _extNode {
    struct _extNode *next;
    CERTCertExtension *ext;
} extNode;

typedef struct {
    void (*setExts)(void *object, CERTCertExtension **exts);
    void *object;
    PRArenaPool *ownerArena;
    PRArenaPool *arena;
    extNode *head;
    int count;
}extRec;







void *
cert_StartExtensions(void *owner, PRArenaPool *ownerArena,
   void (*setExts)(void *object, CERTCertExtension **exts))
{
    PRArenaPool *arena;
    extRec *handle;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena ) {
	return(0);
    }

    handle = (extRec *)PORT_ArenaAlloc(arena, sizeof(extRec));
    if ( !handle ) {
	PORT_FreeArena(arena, PR_FALSE);
	return(0);
    }

    handle->object = owner;
    handle->ownerArena = ownerArena;
    handle->setExts = setExts;

    handle->arena = arena;
    handle->head = 0;
    handle->count = 0;
    
    return(handle);
}

static unsigned char hextrue = 0xff;




SECStatus
CERT_AddExtensionByOID (void *exthandle, SECItem *oid, SECItem *value,
			PRBool critical, PRBool copyData)
{
    CERTCertExtension *ext;
    SECStatus rv;
    extNode *node;
    extRec *handle;
    
    handle = (extRec *)exthandle;

    
    ext = (CERTCertExtension*)PORT_ArenaZAlloc(handle->ownerArena,
                                               sizeof(CERTCertExtension));
    if ( !ext ) {
	return(SECFailure);
    }

    node = (extNode*)PORT_ArenaAlloc(handle->arena, sizeof(extNode));
    if ( !node ) {
	return(SECFailure);
    }

    
    node->next = handle->head;
    handle->head = node;
   
    
    node->ext = ext;
    
    
    ext->id = *oid;
    
    
    if ( critical ) {
	ext->critical.data = (unsigned char*)&hextrue;
	ext->critical.len = 1;
    }

    
    if ( copyData ) {
	rv = SECITEM_CopyItem(handle->ownerArena, &ext->value, value);
	if ( rv ) {
	    return(SECFailure);
	}
    } else {
	ext->value = *value;
    }
    
    handle->count++;
    
    return(SECSuccess);

}

SECStatus
CERT_AddExtension(void *exthandle, int idtag, SECItem *value,
		     PRBool critical, PRBool copyData)
{
    SECOidData *oid;
    
    oid = SECOID_FindOIDByTag((SECOidTag)idtag);
    if ( !oid ) {
	return(SECFailure);
    }

    return(CERT_AddExtensionByOID(exthandle, &oid->oid, value, critical, copyData));
}

SECStatus
CERT_EncodeAndAddExtension(void *exthandle, int idtag, void *value,
			   PRBool critical, const SEC_ASN1Template *atemplate)
{
    extRec *handle;
    SECItem *encitem;

    handle = (extRec *)exthandle;

    encitem = SEC_ASN1EncodeItem(handle->ownerArena, NULL, value, atemplate);
    if ( encitem == NULL ) {
	return(SECFailure);
    }

    return CERT_AddExtension(exthandle, idtag, encitem, critical, PR_FALSE);
}

void
PrepareBitStringForEncoding (SECItem *bitsmap, SECItem *value)
{
  unsigned char onebyte;
  unsigned int i, len = 0;

   
  onebyte = '\0';   
   
  for (i = 0; i < (value->len ) * 8; ++i) {
      if (i % 8 == 0)
	  onebyte = value->data[i/8];
      if (onebyte & 0x80)
	  len = i;            
      onebyte <<= 1;
      
  }
  bitsmap->data = value->data;
   
  bitsmap->len = len + 1;
}

SECStatus
CERT_EncodeAndAddBitStrExtension (void *exthandle, int idtag,
				  SECItem *value, PRBool critical)
{
  SECItem bitsmap;
  
  PrepareBitStringForEncoding (&bitsmap, value);
  return (CERT_EncodeAndAddExtension
	  (exthandle, idtag, &bitsmap, critical,
          SEC_ASN1_GET(SEC_BitStringTemplate)));
}

SECStatus
CERT_FinishExtensions(void *exthandle)
{
    extRec *handle;
    extNode *node;
    CERTCertExtension **exts;
    SECStatus rv = SECFailure;
    
    handle = (extRec *)exthandle;

    
    exts = PORT_ArenaNewArray(handle->ownerArena, CERTCertExtension *,
			      handle->count + 1);
    if (exts == NULL) {
	goto loser;
    }

    

#ifdef OLD
    switch (handle->type) {
      case CertificateExtensions:
	handle->owner.cert->extensions = exts;
	DER_SetUInteger (ownerArena, &(handle->owner.cert->version),
			 SEC_CERTIFICATE_VERSION_3);
	break;
      case CrlExtensions:
	handle->owner.crl->extensions = exts;
	DER_SetUInteger (ownerArena, &(handle->owner.crl->version),
			 SEC_CRL_VERSION_2);
	break;
      case OCSPRequestExtensions:
	handle->owner.request->tbsRequest->requestExtensions = exts;
	break;
      case OCSPSingleRequestExtensions:
	handle->owner.singleRequest->singleRequestExtensions = exts;	
	break;
      case OCSPResponseSingleExtensions:
	handle->owner.singleResponse->singleExtensions = exts;	
	break;
    }
#endif

    handle->setExts(handle->object, exts);
	
    

    
    node = handle->head;
    while ( node ) {
	*exts = node->ext;
	
	node = node->next;
	exts++;
    }

    
    *exts = 0;

    rv = SECSuccess;

loser:
    
    PORT_FreeArena(handle->arena, PR_FALSE);
    return rv;
}

SECStatus
CERT_MergeExtensions(void *exthandle, CERTCertExtension **extensions)
{
    CERTCertExtension *ext;
    SECStatus rv = SECSuccess;
    SECOidTag tag;
    extNode *node;
    extRec *handle = exthandle;
    
    if (!exthandle || !extensions) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    while ((ext = *extensions++) != NULL) {
        tag = SECOID_FindOIDTag(&ext->id);
        for (node=handle->head; node != NULL; node=node->next) {
            if (tag == 0) {
                if (SECITEM_ItemsAreEqual(&ext->id, &node->ext->id))
                    break;
            }
            else {
                if (SECOID_FindOIDTag(&node->ext->id) == tag) {
                    break;
                }
            }
        }
        if (node == NULL) {
            PRBool critical = (ext->critical.len != 0 &&
                            ext->critical.data[ext->critical.len - 1] != 0);
            if (critical && tag == SEC_OID_UNKNOWN) {
               PORT_SetError(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION);
               rv = SECFailure;
               break;
            }
            
            rv = CERT_AddExtensionByOID (exthandle, &ext->id, &ext->value,
                                         critical, PR_TRUE);
            if (rv != SECSuccess)
                break;
        }
    }
    return rv;
}




SECStatus
CERT_FindBitStringExtension (CERTCertExtension **extensions, int tag,
			     SECItem *retItem)
{
    SECItem wrapperItem, tmpItem = {siBuffer,0};
    SECStatus rv;
    PRArenaPool *arena = NULL;
    
    wrapperItem.data = NULL;
    tmpItem.data = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	return(SECFailure);
    }
    
    rv = cert_FindExtension(extensions, tag, &wrapperItem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = SEC_QuickDERDecodeItem(arena, &tmpItem,
                                SEC_ASN1_GET(SEC_BitStringTemplate),
                                &wrapperItem);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    retItem->data = (unsigned char *)PORT_Alloc( ( tmpItem.len + 7 ) >> 3 );
    if ( retItem->data == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(retItem->data, tmpItem.data, ( tmpItem.len + 7 ) >> 3);
    retItem->len = tmpItem.len;
    
    rv = SECSuccess;
    goto done;
    
loser:
    rv = SECFailure;

done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    if ( wrapperItem.data ) {
	PORT_Free(wrapperItem.data);
    }

    return(rv);
}

PRBool
cert_HasCriticalExtension (CERTCertExtension **extensions)
{
    CERTCertExtension **exts;
    CERTCertExtension *ext = NULL;
    PRBool hasCriticalExten = PR_FALSE;
    
    exts = extensions;
    
    if (exts) {
	while ( *exts ) {
	    ext = *exts;
	    
	    if (ext->critical.data && ext->critical.data[0] == 0xff) {
		hasCriticalExten = PR_TRUE;
		break;
	    }
	    exts++;
	}
    }
    return (hasCriticalExten);
}

PRBool
cert_HasUnknownCriticalExten (CERTCertExtension **extensions)
{
    CERTCertExtension **exts;
    CERTCertExtension *ext = NULL;
    PRBool hasUnknownCriticalExten = PR_FALSE;
    
    exts = extensions;
    
    if (exts) {
	while ( *exts ) {
	    ext = *exts;
	    



	    if (ext->critical.data && ext->critical.data[0] == 0xff) {
		if (SECOID_KnownCertExtenOID (&ext->id) == PR_FALSE) {
		    hasUnknownCriticalExten = PR_TRUE;
		    break;
		}
	    }
	    exts++;
	}
    }
    return (hasUnknownCriticalExten);
}
