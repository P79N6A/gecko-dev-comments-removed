










































#include "p7local.h"

#include "cert.h"
				
#include "certdb.h"		
     				
                           	
      				

#include "cryptohi.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "prtime.h"
#include "secerr.h"
#include "sechash.h"	
#include "secder.h"
#include "secpkcs5.h"

struct sec_pkcs7_decoder_worker {
    int depth;
    int digcnt;
    void **digcxs;
    const SECHashObject **digobjs;
    sec_PKCS7CipherObject *decryptobj;
    PRBool saw_contents;
};

struct SEC_PKCS7DecoderContextStr {
    SEC_ASN1DecoderContext *dcx;
    SEC_PKCS7ContentInfo *cinfo;
    SEC_PKCS7DecoderContentCallback cb;
    void *cb_arg;
    SECKEYGetPasswordKey pwfn;
    void *pwfn_arg;
    struct sec_pkcs7_decoder_worker worker;
    PRArenaPool *tmp_poolp;
    int error;
    SEC_PKCS7GetDecryptKeyCallback dkcb;
    void *dkcb_arg;
    SEC_PKCS7DecryptionAllowedCallback decrypt_allowed_cb;
};









static void
sec_pkcs7_decoder_work_data (SEC_PKCS7DecoderContext *p7dcx,
			     struct sec_pkcs7_decoder_worker *worker,
			     const unsigned char *data, unsigned long len,
			     PRBool final)
{
    unsigned char *buf = NULL;
    SECStatus rv;
    int i;

    






    PORT_Assert ((data != NULL && len) || final);

    







    if (worker->decryptobj != NULL) {
	
	unsigned int inlen;	
	unsigned int outlen;	
	unsigned int buflen;	
	SECItem *plain;

	inlen = len;
	buflen = sec_PKCS7DecryptLength (worker->decryptobj, inlen, final);
	if (buflen == 0) {
	    if (inlen == 0)	
		return;
	    



	    rv = sec_PKCS7Decrypt (worker->decryptobj, NULL, NULL, 0,
				   data, inlen, final);
	    if (rv != SECSuccess) {
		p7dcx->error = PORT_GetError();
		return;		
	    }
	    return;
	}

	if (p7dcx->cb != NULL) {
	    buf = (unsigned char *) PORT_Alloc (buflen);
	    plain = NULL;
	} else {
	    unsigned long oldlen;

	    




	    plain = &(p7dcx->cinfo->
			content.envelopedData->encContentInfo.plainContent);

	    oldlen = plain->len;
	    if (oldlen == 0) {
		buf = (unsigned char*)PORT_ArenaAlloc (p7dcx->cinfo->poolp, 
						       buflen);
	    } else {
		buf = (unsigned char*)PORT_ArenaGrow (p7dcx->cinfo->poolp, 
				      plain->data,
				      oldlen, oldlen + buflen);
		if (buf != NULL)
		    buf += oldlen;
	    }
	    plain->data = buf;
	}
	if (buf == NULL) {
	    p7dcx->error = SEC_ERROR_NO_MEMORY;
	    return;		
	}
	rv = sec_PKCS7Decrypt (worker->decryptobj, buf, &outlen, buflen,
			       data, inlen, final);
	if (rv != SECSuccess) {
	    p7dcx->error = PORT_GetError();
	    return;		
	}
	if (plain != NULL) {
	    PORT_Assert (final || outlen == buflen);
	    plain->len += outlen;
	}
	data = buf;
	len = outlen;
    }

    


    if (len) {
	for (i = 0; i < worker->digcnt; i++) {
	    (* worker->digobjs[i]->update) (worker->digcxs[i], data, len);
	}
    }

    


    if (p7dcx->cb != NULL) {
	if (len)
	    (* p7dcx->cb) (p7dcx->cb_arg, (const char *)data, len);
	if (worker->decryptobj != NULL) {
	    PORT_Assert (buf != NULL);
	    PORT_Free (buf);
	}
    }
}

static void
sec_pkcs7_decoder_filter (void *arg, const char *data, unsigned long len,
			  int depth, SEC_ASN1EncodingPart data_kind)
{
    SEC_PKCS7DecoderContext *p7dcx;
    struct sec_pkcs7_decoder_worker *worker;

    






    if (data_kind != SEC_ASN1_Contents)
	return;

    



    PORT_Assert (len);
    if (len == 0)
	return;

    p7dcx = (SEC_PKCS7DecoderContext*)arg;

    




    worker = &(p7dcx->worker);

    worker->saw_contents = PR_TRUE;

    sec_pkcs7_decoder_work_data (p7dcx, worker,
				 (const unsigned char *) data, len, PR_FALSE);
}









static SECStatus
sec_pkcs7_decoder_start_digests (SEC_PKCS7DecoderContext *p7dcx, int depth,
				 SECAlgorithmID **digestalgs)
{
    int i, digcnt;

    if (digestalgs == NULL)
	return SECSuccess;

    


    digcnt = 0;
    while (digestalgs[digcnt] != NULL)
	digcnt++;

    



    if (digcnt == 0)
	return SECSuccess;

    p7dcx->worker.digcxs = (void**)PORT_ArenaAlloc (p7dcx->tmp_poolp,
					    digcnt * sizeof (void *));
    p7dcx->worker.digobjs = (const SECHashObject**)PORT_ArenaAlloc (p7dcx->tmp_poolp,
					     digcnt * sizeof (SECHashObject *));
    if (p7dcx->worker.digcxs == NULL || p7dcx->worker.digobjs == NULL) {
	p7dcx->error = SEC_ERROR_NO_MEMORY;
	return SECFailure;
    }

    p7dcx->worker.depth = depth;
    p7dcx->worker.digcnt = 0;

    


    for (i = 0; i < digcnt; i++) {
	SECAlgorithmID *     algid  = digestalgs[i];
	SECOidTag            oidTag = SECOID_FindOIDTag(&(algid->algorithm));
	const SECHashObject *digobj = HASH_GetHashObjectByOidTag(oidTag);
	void *digcx;

	







	if (digobj == NULL) {
	    p7dcx->worker.digcnt--;
	    continue;
	}

	digcx = (* digobj->create)();
	if (digcx != NULL) {
	    (* digobj->begin) (digcx);
	    p7dcx->worker.digobjs[p7dcx->worker.digcnt] = digobj;
	    p7dcx->worker.digcxs[p7dcx->worker.digcnt] = digcx;
	    p7dcx->worker.digcnt++;
	}
    }

    if (p7dcx->worker.digcnt != 0)
	SEC_ASN1DecoderSetFilterProc (p7dcx->dcx,
				      sec_pkcs7_decoder_filter,
				      p7dcx,
				      (PRBool)(p7dcx->cb != NULL));
    return SECSuccess;
}





static SECStatus
sec_pkcs7_decoder_finish_digests (SEC_PKCS7DecoderContext *p7dcx,
				  PRArenaPool *poolp,
				  SECItem ***digestsp)
{
    struct sec_pkcs7_decoder_worker *worker;
    const SECHashObject *digobj;
    void *digcx;
    SECItem **digests, *digest;
    int i;
    void *mark;

    




    worker = &(p7dcx->worker);

    


    if (worker->digcnt == 0)
	return SECSuccess;

    




    SEC_ASN1DecoderClearFilterProc (p7dcx->dcx);

    





    if (! worker->saw_contents) {
	for (i = 0; i < worker->digcnt; i++) {
	    digcx = worker->digcxs[i];
	    digobj = worker->digobjs[i];
	    (* digobj->destroy) (digcx, PR_TRUE);
	}
	return SECSuccess;
    }

    mark = PORT_ArenaMark (poolp);

    


    digests = 
      (SECItem**)PORT_ArenaAlloc (poolp,(worker->digcnt+1)*sizeof(SECItem *));
    digest = (SECItem*)PORT_ArenaAlloc (poolp, worker->digcnt*sizeof(SECItem));
    if (digests == NULL || digest == NULL) {
	p7dcx->error = PORT_GetError();
	PORT_ArenaRelease (poolp, mark);
	return SECFailure;
    }

    for (i = 0; i < worker->digcnt; i++, digest++) {
	digcx = worker->digcxs[i];
	digobj = worker->digobjs[i];

	digest->data = (unsigned char*)PORT_ArenaAlloc (poolp, digobj->length);
	if (digest->data == NULL) {
	    p7dcx->error = PORT_GetError();
	    PORT_ArenaRelease (poolp, mark);
	    return SECFailure;
	}

	digest->len = digobj->length;
	(* digobj->end) (digcx, digest->data, &(digest->len), digest->len);
	(* digobj->destroy) (digcx, PR_TRUE);

	digests[i] = digest;
    }
    digests[i] = NULL;
    *digestsp = digests;

    PORT_ArenaUnmark (poolp, mark);
    return SECSuccess;
}






static PK11SymKey *
sec_pkcs7_decoder_get_recipient_key (SEC_PKCS7DecoderContext *p7dcx,
				     SEC_PKCS7RecipientInfo **recipientinfos,
				     SEC_PKCS7EncryptedContentInfo *enccinfo)
{
    SEC_PKCS7RecipientInfo *ri;
    CERTCertificate *cert = NULL;
    SECKEYPrivateKey *privkey = NULL;
    PK11SymKey *bulkkey = NULL;
    SECOidTag keyalgtag, bulkalgtag, encalgtag;
    PK11SlotInfo *slot = NULL;
    int bulkLength = 0;

    if (recipientinfos == NULL || recipientinfos[0] == NULL) {
	p7dcx->error = SEC_ERROR_NOT_A_RECIPIENT;
	goto no_key_found;
    }

    cert = PK11_FindCertAndKeyByRecipientList(&slot,recipientinfos,&ri,
						&privkey, p7dcx->pwfn_arg);
    if (cert == NULL) {
	p7dcx->error = SEC_ERROR_NOT_A_RECIPIENT;
	goto no_key_found;
    }

    ri->cert = cert;		
    PORT_Assert(privkey != NULL);

    keyalgtag = SECOID_GetAlgorithmTag(&(cert->subjectPublicKeyInfo.algorithm));
    encalgtag = SECOID_GetAlgorithmTag (&(ri->keyEncAlg));
    if (keyalgtag != encalgtag) {
	p7dcx->error = SEC_ERROR_PKCS7_KEYALG_MISMATCH;
	goto no_key_found;
    }
    bulkalgtag = SECOID_GetAlgorithmTag (&(enccinfo->contentEncAlg));

    switch (encalgtag) {
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
	bulkkey = PK11_PubUnwrapSymKey (privkey, &ri->encKey,
					PK11_AlgtagToMechanism (bulkalgtag),
					CKA_DECRYPT, 0);
	if (bulkkey == NULL) {
	    p7dcx->error = PORT_GetError();
	    PORT_SetError(0);
	    goto no_key_found;
	}
	break;
      default:
	p7dcx->error = SEC_ERROR_UNSUPPORTED_KEYALG;
	break;
    }

no_key_found:
    if (privkey != NULL)
	SECKEY_DestroyPrivateKey (privkey);
    if (slot != NULL)
	PK11_FreeSlot(slot);

    return bulkkey;
}
 














static SECStatus
sec_pkcs7_decoder_start_decrypt (SEC_PKCS7DecoderContext *p7dcx, int depth,
				 SEC_PKCS7RecipientInfo **recipientinfos,
				 SEC_PKCS7EncryptedContentInfo *enccinfo,
				 PK11SymKey **copy_key_for_signature)
{
    PK11SymKey *bulkkey = NULL;
    sec_PKCS7CipherObject *decryptobj;

    








    if (SEC_PKCS7ContentType(p7dcx->cinfo) == SEC_OID_PKCS7_ENCRYPTED_DATA) {
	if (p7dcx->dkcb != NULL) {
	    bulkkey = (*p7dcx->dkcb)(p7dcx->dkcb_arg, 
				     &(enccinfo->contentEncAlg));
	}
	enccinfo->keysize = 0;
    } else {
	bulkkey = sec_pkcs7_decoder_get_recipient_key (p7dcx, recipientinfos, 
						       enccinfo);
	if (bulkkey == NULL) goto no_decryption;
	enccinfo->keysize = PK11_GetKeyStrength(bulkkey, 
						&(enccinfo->contentEncAlg));

    }

    



    if(bulkkey == NULL) {
	goto no_decryption;
    }
    
    



    if (p7dcx->decrypt_allowed_cb) {
	if ((*p7dcx->decrypt_allowed_cb) (&(enccinfo->contentEncAlg), 
					  bulkkey) == PR_FALSE) {
	    p7dcx->error = SEC_ERROR_DECRYPTION_DISALLOWED;
	    goto no_decryption;
	}
    } else {
	    p7dcx->error = SEC_ERROR_DECRYPTION_DISALLOWED;
	    goto no_decryption;
    }

    





    if (copy_key_for_signature != NULL)
	*copy_key_for_signature = PK11_ReferenceSymKey (bulkkey);

    




    decryptobj = sec_PKCS7CreateDecryptObject (bulkkey,
					       &(enccinfo->contentEncAlg));

    


    PK11_FreeSymKey (bulkkey);

    if (decryptobj == NULL) {
	p7dcx->error = PORT_GetError();
	PORT_SetError(0);
	goto no_decryption;
    }

    SEC_ASN1DecoderSetFilterProc (p7dcx->dcx,
				  sec_pkcs7_decoder_filter,
				  p7dcx,
				  (PRBool)(p7dcx->cb != NULL));

    p7dcx->worker.depth = depth;
    p7dcx->worker.decryptobj = decryptobj;

    return SECSuccess;

no_decryption:
    










    if (p7dcx->cb != NULL)
	return SECFailure;
    else
	return SECSuccess;	
}


static SECStatus
sec_pkcs7_decoder_finish_decrypt (SEC_PKCS7DecoderContext *p7dcx,
				  PRArenaPool *poolp,
				  SEC_PKCS7EncryptedContentInfo *enccinfo)
{
    struct sec_pkcs7_decoder_worker *worker;

    




    worker = &(p7dcx->worker);

    


    if (worker->decryptobj == NULL)
	return SECSuccess;

    




    SEC_ASN1DecoderClearFilterProc (p7dcx->dcx);

    


    sec_pkcs7_decoder_work_data (p7dcx, worker, NULL, 0, PR_TRUE);

    


    sec_PKCS7DestroyDecryptObject (worker->decryptobj);
    worker->decryptobj = NULL;

    return SECSuccess;
}


static void
sec_pkcs7_decoder_notify (void *arg, PRBool before, void *dest, int depth)
{
    SEC_PKCS7DecoderContext *p7dcx;
    SEC_PKCS7ContentInfo *cinfo;
    SEC_PKCS7SignedData *sigd;
    SEC_PKCS7EnvelopedData *envd;
    SEC_PKCS7SignedAndEnvelopedData *saed;
    SEC_PKCS7EncryptedData *encd;
    SEC_PKCS7DigestedData *digd;
    PRBool after;
    SECStatus rv;

    





    if (before)
	after = PR_FALSE;
    else
	after = PR_TRUE;

    p7dcx = (SEC_PKCS7DecoderContext*)arg;
    cinfo = p7dcx->cinfo;

    if (cinfo->contentTypeTag == NULL) {
	if (after && dest == &(cinfo->contentType))
	    cinfo->contentTypeTag = SECOID_FindOID(&(cinfo->contentType));
	return;
    }

    switch (cinfo->contentTypeTag->offset) {
      case SEC_OID_PKCS7_SIGNED_DATA:
	sigd = cinfo->content.signedData;
	if (sigd == NULL)
	    break;

	if (sigd->contentInfo.contentTypeTag == NULL) {
	    if (after && dest == &(sigd->contentInfo.contentType))
		sigd->contentInfo.contentTypeTag =
			SECOID_FindOID(&(sigd->contentInfo.contentType));
	    break;
	}

	










	if (sigd->contentInfo.contentTypeTag->offset != SEC_OID_PKCS7_DATA) {
	    
	    SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
	    break;
	}

	




	if (before && dest == &(sigd->contentInfo.content)) {
	    rv = sec_pkcs7_decoder_start_digests (p7dcx, depth,
						  sigd->digestAlgorithms);
	    if (rv != SECSuccess)
		SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);

	    break;
	}

	




	


	if (after && dest == &(sigd->contentInfo.content)) {
	    




	    (void) sec_pkcs7_decoder_finish_digests (p7dcx, cinfo->poolp,
						     &(sigd->digests));

	    




	    


	    SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
	}
	break;

      case SEC_OID_PKCS7_ENVELOPED_DATA:
	envd = cinfo->content.envelopedData;
	if (envd == NULL)
	    break;

	if (envd->encContentInfo.contentTypeTag == NULL) {
	    if (after && dest == &(envd->encContentInfo.contentType))
		envd->encContentInfo.contentTypeTag =
			SECOID_FindOID(&(envd->encContentInfo.contentType));
	    break;
	}

	




	if (before && dest == &(envd->encContentInfo.encContent)) {
	    rv = sec_pkcs7_decoder_start_decrypt (p7dcx, depth,
						  envd->recipientInfos,
						  &(envd->encContentInfo),
						  NULL);
	    if (rv != SECSuccess)
		SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);

	    break;
	}

	


	if (after && dest == &(envd->encContentInfo.encContent)) {
	    




	    (void) sec_pkcs7_decoder_finish_decrypt (p7dcx, cinfo->poolp,
						     &(envd->encContentInfo));

	    




	    


	    SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
	}
	break;

      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	saed = cinfo->content.signedAndEnvelopedData;
	if (saed == NULL)
	    break;

	if (saed->encContentInfo.contentTypeTag == NULL) {
	    if (after && dest == &(saed->encContentInfo.contentType))
		saed->encContentInfo.contentTypeTag =
			SECOID_FindOID(&(saed->encContentInfo.contentType));
	    break;
	}

	




	if (before && dest == &(saed->encContentInfo.encContent)) {
	    rv = sec_pkcs7_decoder_start_decrypt (p7dcx, depth,
						  saed->recipientInfos,
						  &(saed->encContentInfo),
						  &(saed->sigKey));
	    if (rv == SECSuccess)
		rv = sec_pkcs7_decoder_start_digests (p7dcx, depth,
						      saed->digestAlgorithms);
	    if (rv != SECSuccess)
		SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);

	    break;
	}

	


	if (after && dest == &(saed->encContentInfo.encContent)) {
	    









	    (void) sec_pkcs7_decoder_finish_decrypt (p7dcx, cinfo->poolp,
						     &(saed->encContentInfo));
	    (void) sec_pkcs7_decoder_finish_digests (p7dcx, cinfo->poolp,
						     &(saed->digests));

	    




	    


	    SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
	}
	break;

      case SEC_OID_PKCS7_DIGESTED_DATA:
	digd = cinfo->content.digestedData;
	
	


	if (before && dest == &(digd->contentInfo.content.data)) {
	    SEC_ASN1DecoderSetFilterProc (p7dcx->dcx, sec_pkcs7_decoder_filter,
					  p7dcx,
					  (PRBool)(p7dcx->cb != NULL));
	    break;
	}

	


	if (after && dest == &(digd->contentInfo.content.data)) {
	    SEC_ASN1DecoderClearFilterProc (p7dcx->dcx);
	}
	break;

      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	encd = cinfo->content.encryptedData;

	






	if (before && dest == &(encd->encContentInfo.encContent)) {
	    



	    rv = SECSuccess;
	    if (p7dcx->dkcb != NULL) {
		rv = sec_pkcs7_decoder_start_decrypt (p7dcx, depth, NULL,
						      &(encd->encContentInfo),
						      NULL);
	    }

	    if (rv != SECSuccess)
		SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
		
	    break;
	}

	


	if (after && dest == &(encd->encContentInfo.encContent)) {
	    




	    (void) sec_pkcs7_decoder_finish_decrypt (p7dcx, cinfo->poolp,
						     &(encd->encContentInfo));

	    


	    SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
	}
	break;

      case SEC_OID_PKCS7_DATA:
	




 
	
	if (before && dest == &(cinfo->content.data)) {

	    


	    SEC_ASN1DecoderSetFilterProc (p7dcx->dcx,
					  sec_pkcs7_decoder_filter,
					  p7dcx,
					  (PRBool)(p7dcx->cb != NULL));
	    break;
	}

	if (after && dest == &(cinfo->content.data)) {
	    



	    SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
	    SEC_ASN1DecoderClearFilterProc (p7dcx->dcx);
	}
	break;

      default:
	SEC_ASN1DecoderClearNotifyProc (p7dcx->dcx);
	break;
    }
}


SEC_PKCS7DecoderContext *
SEC_PKCS7DecoderStart(SEC_PKCS7DecoderContentCallback cb, void *cb_arg,
		      SECKEYGetPasswordKey pwfn, void *pwfn_arg,
		      SEC_PKCS7GetDecryptKeyCallback decrypt_key_cb, 
		      void *decrypt_key_cb_arg,
		      SEC_PKCS7DecryptionAllowedCallback decrypt_allowed_cb)
{
    SEC_PKCS7DecoderContext *p7dcx;
    SEC_ASN1DecoderContext *dcx;
    SEC_PKCS7ContentInfo *cinfo;
    PRArenaPool *poolp;

    poolp = PORT_NewArena (1024);		
    if (poolp == NULL)
	return NULL;

    cinfo = (SEC_PKCS7ContentInfo*)PORT_ArenaZAlloc (poolp, sizeof(*cinfo));
    if (cinfo == NULL) {
	PORT_FreeArena (poolp, PR_FALSE);
	return NULL;
    }

    cinfo->poolp = poolp;
    cinfo->pwfn = pwfn;
    cinfo->pwfn_arg = pwfn_arg;
    cinfo->created = PR_FALSE;
    cinfo->refCount = 1;

    p7dcx = 
      (SEC_PKCS7DecoderContext*)PORT_ZAlloc (sizeof(SEC_PKCS7DecoderContext));
    if (p7dcx == NULL) {
	PORT_FreeArena (poolp, PR_FALSE);
	return NULL;
    }

    p7dcx->tmp_poolp = PORT_NewArena (1024);	
    if (p7dcx->tmp_poolp == NULL) {
	PORT_Free (p7dcx);
	PORT_FreeArena (poolp, PR_FALSE);
	return NULL;
    }

    dcx = SEC_ASN1DecoderStart (poolp, cinfo, sec_PKCS7ContentInfoTemplate);
    if (dcx == NULL) {
	PORT_FreeArena (p7dcx->tmp_poolp, PR_FALSE);
	PORT_Free (p7dcx);
	PORT_FreeArena (poolp, PR_FALSE);
	return NULL;
    }

    SEC_ASN1DecoderSetNotifyProc (dcx, sec_pkcs7_decoder_notify, p7dcx);

    p7dcx->dcx = dcx;
    p7dcx->cinfo = cinfo;
    p7dcx->cb = cb;
    p7dcx->cb_arg = cb_arg;
    p7dcx->pwfn = pwfn;
    p7dcx->pwfn_arg = pwfn_arg;
    p7dcx->dkcb = decrypt_key_cb;
    p7dcx->dkcb_arg = decrypt_key_cb_arg;
    p7dcx->decrypt_allowed_cb = decrypt_allowed_cb;

    return p7dcx;
}










SECStatus
SEC_PKCS7DecoderUpdate(SEC_PKCS7DecoderContext *p7dcx,
		       const char *buf, unsigned long len)
{
    if (p7dcx->cinfo != NULL && p7dcx->dcx != NULL) { 
	PORT_Assert (p7dcx->error == 0);
	if (p7dcx->error == 0) {
	    if (SEC_ASN1DecoderUpdate (p7dcx->dcx, buf, len) != SECSuccess) {
		p7dcx->error = PORT_GetError();
		PORT_Assert (p7dcx->error);
		if (p7dcx->error == 0)
		    p7dcx->error = -1;
	    }
	}
    }

    if (p7dcx->error) {
	if (p7dcx->dcx != NULL) {
	    (void) SEC_ASN1DecoderFinish (p7dcx->dcx);
	    p7dcx->dcx = NULL;
	}
	if (p7dcx->cinfo != NULL) {
	    SEC_PKCS7DestroyContentInfo (p7dcx->cinfo);
	    p7dcx->cinfo = NULL;
	}
	PORT_SetError (p7dcx->error);
	return SECFailure;
    }

    return SECSuccess;
}


SEC_PKCS7ContentInfo *
SEC_PKCS7DecoderFinish(SEC_PKCS7DecoderContext *p7dcx)
{
    SEC_PKCS7ContentInfo *cinfo;

    cinfo = p7dcx->cinfo;
    if (p7dcx->dcx != NULL) {
	if (SEC_ASN1DecoderFinish (p7dcx->dcx) != SECSuccess) {
	    SEC_PKCS7DestroyContentInfo (cinfo);
	    cinfo = NULL;
	}
    }
    
    if (p7dcx->worker.decryptobj) {
        sec_PKCS7DestroyDecryptObject (p7dcx->worker.decryptobj);
    }
    PORT_FreeArena (p7dcx->tmp_poolp, PR_FALSE);
    PORT_Free (p7dcx);
    return cinfo;
}


SEC_PKCS7ContentInfo *
SEC_PKCS7DecodeItem(SECItem *p7item,
		    SEC_PKCS7DecoderContentCallback cb, void *cb_arg,
		    SECKEYGetPasswordKey pwfn, void *pwfn_arg,
		    SEC_PKCS7GetDecryptKeyCallback decrypt_key_cb, 
		    void *decrypt_key_cb_arg,
		    SEC_PKCS7DecryptionAllowedCallback decrypt_allowed_cb)
{
    SEC_PKCS7DecoderContext *p7dcx;

    p7dcx = SEC_PKCS7DecoderStart(cb, cb_arg, pwfn, pwfn_arg, decrypt_key_cb,
				  decrypt_key_cb_arg, decrypt_allowed_cb);
    if (!p7dcx) {
        
        return NULL;
    }
    (void) SEC_PKCS7DecoderUpdate(p7dcx, (char *) p7item->data, p7item->len);
    return SEC_PKCS7DecoderFinish(p7dcx);
}




void
SEC_PKCS7DecoderAbort(SEC_PKCS7DecoderContext *p7dcx, int error)
{
    PORT_Assert(p7dcx);
    SEC_ASN1DecoderAbort(p7dcx->dcx, error);
}





PRBool
SEC_PKCS7ContainsCertsOrCrls(SEC_PKCS7ContentInfo *cinfo)
{
    SECOidTag kind;
    SECItem **certs;
    CERTSignedCrl **crls;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
      case SEC_OID_PKCS7_DIGESTED_DATA:
      case SEC_OID_PKCS7_ENVELOPED_DATA:
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	return PR_FALSE;
      case SEC_OID_PKCS7_SIGNED_DATA:
	certs = cinfo->content.signedData->rawCerts;
	crls = cinfo->content.signedData->crls;
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	certs = cinfo->content.signedAndEnvelopedData->rawCerts;
	crls = cinfo->content.signedAndEnvelopedData->crls;
	break;
    }

    


    if (certs != NULL && certs[0] != NULL)
	return PR_TRUE;
    else if (crls != NULL && crls[0] != NULL)
	return PR_TRUE;
    else
	return PR_FALSE;
}




PRBool
SEC_PKCS7IsContentEmpty(SEC_PKCS7ContentInfo *cinfo, unsigned int minLen)
{
    SECItem *item = NULL;

    if(cinfo == NULL) {
	return PR_TRUE;
    }

    switch(SEC_PKCS7ContentType(cinfo)) 
    {
	case SEC_OID_PKCS7_DATA:
	    item = cinfo->content.data;
	    break;
	case SEC_OID_PKCS7_ENCRYPTED_DATA:
	    item = &cinfo->content.encryptedData->encContentInfo.encContent;
	    break;
	default:
	    
	    return PR_FALSE;
    }

    if(!item) {
	return PR_TRUE;
    } else if(item->len <= minLen) {
	return PR_TRUE;
    }

    return PR_FALSE;
}


PRBool
SEC_PKCS7ContentIsEncrypted(SEC_PKCS7ContentInfo *cinfo)
{
    SECOidTag kind;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
      case SEC_OID_PKCS7_DIGESTED_DATA:
      case SEC_OID_PKCS7_SIGNED_DATA:
	return PR_FALSE;
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
      case SEC_OID_PKCS7_ENVELOPED_DATA:
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	return PR_TRUE;
    }
}










PRBool
SEC_PKCS7ContentIsSigned(SEC_PKCS7ContentInfo *cinfo)
{
    SECOidTag kind;
    SEC_PKCS7SignerInfo **signerinfos;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
      case SEC_OID_PKCS7_DIGESTED_DATA:
      case SEC_OID_PKCS7_ENVELOPED_DATA:
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	return PR_FALSE;
      case SEC_OID_PKCS7_SIGNED_DATA:
	signerinfos = cinfo->content.signedData->signerInfos;
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	signerinfos = cinfo->content.signedAndEnvelopedData->signerInfos;
	break;
    }

    



    if (signerinfos != NULL && signerinfos[0] != NULL)
	return PR_TRUE;
    else
	return PR_FALSE;
}

































static PRBool
sec_pkcs7_verify_signature(SEC_PKCS7ContentInfo *cinfo,
			   SECCertUsage certusage,
			   SECItem *detached_digest,
			   HASH_HashType digest_type,
			   PRBool keepcerts)
{
    SECAlgorithmID **digestalgs, *bulkid;
    SECItem *digest;
    SECItem **digests;
    SECItem **rawcerts;
    CERTSignedCrl **crls;
    SEC_PKCS7SignerInfo **signerinfos, *signerinfo;
    CERTCertificate *cert, **certs;
    PRBool goodsig;
    CERTCertDBHandle *certdb, *defaultdb; 
    SECOidTag encTag,digestTag;
    HASH_HashType found_type;
    int i, certcount;
    SECKEYPublicKey *publickey;
    SECItem *content_type;
    PK11SymKey *sigkey;
    SECItem *encoded_stime;
    int64 stime;
    SECStatus rv;

    


    goodsig = PR_FALSE;
    certcount = 0;
    cert = NULL;
    certs = NULL;
    certdb = NULL;
    defaultdb = CERT_GetDefaultCertDB();
    publickey = NULL;

    if (! SEC_PKCS7ContentIsSigned(cinfo)) {
	PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	goto done;
    }

    PORT_Assert (cinfo->contentTypeTag != NULL);

    switch (cinfo->contentTypeTag->offset) {
      default:
      case SEC_OID_PKCS7_DATA:
      case SEC_OID_PKCS7_DIGESTED_DATA:
      case SEC_OID_PKCS7_ENVELOPED_DATA:
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	
	PORT_Assert (0);
      case SEC_OID_PKCS7_SIGNED_DATA:
	{
	    SEC_PKCS7SignedData *sdp;

	    sdp = cinfo->content.signedData;
	    digestalgs = sdp->digestAlgorithms;
	    digests = sdp->digests;
	    rawcerts = sdp->rawCerts;
	    crls = sdp->crls;
	    signerinfos = sdp->signerInfos;
	    content_type = &(sdp->contentInfo.contentType);
	    sigkey = NULL;
	    bulkid = NULL;
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    digestalgs = saedp->digestAlgorithms;
	    digests = saedp->digests;
	    rawcerts = saedp->rawCerts;
	    crls = saedp->crls;
	    signerinfos = saedp->signerInfos;
	    content_type = &(saedp->encContentInfo.contentType);
	    sigkey = saedp->sigKey;
	    bulkid = &(saedp->encContentInfo.contentEncAlg);
	}
	break;
    }

    if ((signerinfos == NULL) || (signerinfos[0] == NULL)) {
	PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	goto done;
    }

    



    if (signerinfos[1] != NULL) {
	PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	goto done;
    }

    signerinfo = signerinfos[0];

    





    certdb = defaultdb;
    if (certdb == NULL) {
	goto done;
    }

    certcount = 0;
    if (rawcerts != NULL) {
	for (; rawcerts[certcount] != NULL; certcount++) {
	    
	}
    }

    



    rv = CERT_ImportCerts(certdb, certusage, certcount, rawcerts, &certs,
			  keepcerts, PR_FALSE, NULL);
    if ( rv != SECSuccess ) {
	goto done;
    }

    





    cert = CERT_FindCertByIssuerAndSN(certdb, signerinfo->issuerAndSN);
    if (cert == NULL) {
	goto done;
    }

    signerinfo->cert = cert;

    




    encoded_stime = SEC_PKCS7GetSigningTime (cinfo);
    if (encoded_stime != NULL) {
	if (DER_DecodeTimeChoice (&stime, encoded_stime) != SECSuccess)
	    encoded_stime = NULL;	
    }

    







    if (CERT_VerifyCert (certdb, cert, PR_TRUE, certusage,
			 encoded_stime != NULL ? stime : PR_Now(),
			 cinfo->pwfn_arg, NULL) != SECSuccess)
	{
	







	goto savecert;
    }

    publickey = CERT_ExtractPublicKey (cert);
    if (publickey == NULL)
	goto done;

    









    if ((digests == NULL || digests[0] == NULL)
	&& (detached_digest == NULL || detached_digest->data == NULL))
	goto done;

    


    digestTag = SECOID_FindOIDTag(&(signerinfo->digestAlg.algorithm));

    
    found_type = HASH_GetHashTypeByOidTag(digestTag);
    if ((digestTag == SEC_OID_UNKNOWN) || (found_type == HASH_AlgNULL)) {
	PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	goto done;
    }

    if (detached_digest != NULL) {
	unsigned int hashLen     = HASH_ResultLen(found_type);

	if (digest_type != found_type || 
	    detached_digest->len != hashLen) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}
	digest = detached_digest;
    } else {
	PORT_Assert (digestalgs != NULL && digestalgs[0] != NULL);
	if (digestalgs == NULL || digestalgs[0] == NULL) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}

	


	for (i = 0; digestalgs[i] != NULL; i++) {
	    if (SECOID_FindOIDTag(&(digestalgs[i]->algorithm)) == digestTag)
		break;
	}
	if (digestalgs[i] == NULL) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}

	digest = digests[i];
    }

    encTag = SECOID_FindOIDTag(&(signerinfo->digestEncAlg.algorithm));
    if (encTag == SEC_OID_UNKNOWN) {
	PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	goto done;
    }

#ifndef NSS_ECC_MORE_THAN_SUITE_B
    if (encTag == SEC_OID_ANSIX962_EC_PUBLIC_KEY) {
	PORT_SetError(SEC_ERROR_PKCS7_BAD_SIGNATURE);
	goto done;
    }
#endif


    if (signerinfo->authAttr != NULL) {
	SEC_PKCS7Attribute *attr;
	SECItem *value;
	SECItem encoded_attrs;

	



	if (sigkey != NULL) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}

	








	attr = sec_PKCS7FindAttribute (signerinfo->authAttr,
				       SEC_OID_PKCS9_CONTENT_TYPE, PR_TRUE);
	value = sec_PKCS7AttributeValue (attr);
	if (value == NULL || value->len != content_type->len) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}
	if (PORT_Memcmp (value->data, content_type->data, value->len) != 0) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}

	attr = sec_PKCS7FindAttribute (signerinfo->authAttr,
				       SEC_OID_PKCS9_MESSAGE_DIGEST, PR_TRUE);
	value = sec_PKCS7AttributeValue (attr);
	if (value == NULL || value->len != digest->len) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}
	if (PORT_Memcmp (value->data, digest->data, value->len) != 0) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}

	





	encoded_attrs.data = NULL;
	encoded_attrs.len = 0;
	if (sec_PKCS7EncodeAttributes (NULL, &encoded_attrs,
				       &(signerinfo->authAttr)) == NULL)
	    goto done;

	if (encoded_attrs.data == NULL || encoded_attrs.len == 0) {
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}


	goodsig = (PRBool)(VFY_VerifyDataDirect(encoded_attrs.data, 
				   encoded_attrs.len,
				   publickey, &(signerinfo->encDigest),
				   encTag, digestTag, NULL,
				   cinfo->pwfn_arg) == SECSuccess);
	PORT_Free (encoded_attrs.data);
    } else {
	SECItem *sig;
	SECItem holder;
	SECStatus rv;

	




	sig = &(signerinfo->encDigest);
	if (sig->len == 0) {		
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    goto done;
	}

	if (sigkey != NULL) {
	    sec_PKCS7CipherObject *decryptobj;
	    unsigned int buflen;

	    




	    decryptobj = sec_PKCS7CreateDecryptObject (sigkey, bulkid);
	    if (decryptobj == NULL)
		goto done;

	    buflen = sec_PKCS7DecryptLength (decryptobj, sig->len, PR_TRUE);
	    PORT_Assert (buflen);
	    if (buflen == 0) {		
		sec_PKCS7DestroyDecryptObject (decryptobj);
		goto done;
	    }

	    holder.data = (unsigned char*)PORT_Alloc (buflen);
	    if (holder.data == NULL) {
		sec_PKCS7DestroyDecryptObject (decryptobj);
		goto done;
	    }

	    rv = sec_PKCS7Decrypt (decryptobj, holder.data, &holder.len, buflen,
				   sig->data, sig->len, PR_TRUE);
	    sec_PKCS7DestroyDecryptObject (decryptobj);
	    if (rv != SECSuccess) {
		goto done;
	    }

	    sig = &holder;
	}

	goodsig = (PRBool)(VFY_VerifyDigestDirect(digest, publickey, sig,
				     encTag, digestTag, cinfo->pwfn_arg)
                            == SECSuccess);

	if (sigkey != NULL) {
	    PORT_Assert (sig == &holder);
	    PORT_ZFree (holder.data, holder.len);
	}
    }

    if (! goodsig) {
	















	if (PORT_GetError() == SEC_ERROR_BAD_SIGNATURE)
	    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
    }

savecert:
    



    if ( cert->emailAddr && cert->emailAddr[0] &&
	( ( certusage == certUsageEmailSigner ) ||
	 ( certusage == certUsageEmailRecipient ) ) ) {
	SECItem *profile = NULL;
	int save_error;

	



	save_error = PORT_GetError();

	if (goodsig && (signerinfo->authAttr != NULL)) {
	    



	    SEC_PKCS7Attribute *attr;

	    attr = sec_PKCS7FindAttribute (signerinfo->authAttr,
					   SEC_OID_PKCS9_SMIME_CAPABILITIES,
					   PR_TRUE);
	    profile = sec_PKCS7AttributeValue (attr);
	}

	rv = CERT_SaveSMimeProfile (cert, profile, encoded_stime);

	



	PORT_SetError (save_error);

	




    }
	

done:

    




    if (certs != NULL)
	CERT_DestroyCertArray (certs, certcount);

    if (publickey != NULL)
	SECKEY_DestroyPublicKey (publickey);

    return goodsig;
}










PRBool
SEC_PKCS7VerifySignature(SEC_PKCS7ContentInfo *cinfo,
			 SECCertUsage certusage,
			 PRBool keepcerts)
{
    return sec_pkcs7_verify_signature (cinfo, certusage,
				       NULL, HASH_AlgNULL, keepcerts);
}











PRBool
SEC_PKCS7VerifyDetachedSignature(SEC_PKCS7ContentInfo *cinfo,
				 SECCertUsage certusage,
				 SECItem *detached_digest,
				 HASH_HashType digest_type,
				 PRBool keepcerts)
{
    return sec_pkcs7_verify_signature (cinfo, certusage,
				       detached_digest, digest_type,
				       keepcerts);
}










#define sec_common_name 1
#define sec_email_address 2

static char *
sec_pkcs7_get_signer_cert_info(SEC_PKCS7ContentInfo *cinfo, int selector)
{
    SECOidTag kind;
    SEC_PKCS7SignerInfo **signerinfos;
    CERTCertificate *signercert;
    char *container;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
      case SEC_OID_PKCS7_DIGESTED_DATA:
      case SEC_OID_PKCS7_ENVELOPED_DATA:
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	PORT_Assert (0);
	return NULL;
      case SEC_OID_PKCS7_SIGNED_DATA:
	{
	    SEC_PKCS7SignedData *sdp;

	    sdp = cinfo->content.signedData;
	    signerinfos = sdp->signerInfos;
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    signerinfos = saedp->signerInfos;
	}
	break;
    }

    if (signerinfos == NULL || signerinfos[0] == NULL)
	return NULL;

    signercert = signerinfos[0]->cert;

    


    if (signercert == NULL) {
	




	(void) sec_pkcs7_verify_signature (cinfo, certUsageEmailSigner,
					   NULL, HASH_AlgNULL, PR_FALSE);
	signercert = signerinfos[0]->cert;
	if (signercert == NULL)
	    return NULL;
    }

    switch (selector) {
      case sec_common_name:
	container = CERT_GetCommonName (&signercert->subject);
	break;
      case sec_email_address:
	if(signercert->emailAddr && signercert->emailAddr[0]) {
	    container = PORT_Strdup(signercert->emailAddr);
	} else {
	    container = NULL;
	}
	break;
      default:
	PORT_Assert (0);
	container = NULL;
	break;
    }

    return container;
}

char *
SEC_PKCS7GetSignerCommonName(SEC_PKCS7ContentInfo *cinfo)
{
    return sec_pkcs7_get_signer_cert_info(cinfo, sec_common_name);
}

char *
SEC_PKCS7GetSignerEmailAddress(SEC_PKCS7ContentInfo *cinfo)
{
    return sec_pkcs7_get_signer_cert_info(cinfo, sec_email_address);
}





SECItem *
SEC_PKCS7GetSigningTime(SEC_PKCS7ContentInfo *cinfo)
{
    SEC_PKCS7SignerInfo **signerinfos;
    SEC_PKCS7Attribute *attr;

    if (SEC_PKCS7ContentType (cinfo) != SEC_OID_PKCS7_SIGNED_DATA)
	return NULL;

    signerinfos = cinfo->content.signedData->signerInfos;

    


    if (signerinfos == NULL || signerinfos[0] == NULL || signerinfos[1] != NULL)
	return NULL;

    attr = sec_PKCS7FindAttribute (signerinfos[0]->authAttr,
				   SEC_OID_PKCS9_SIGNING_TIME, PR_TRUE);
    return sec_PKCS7AttributeValue (attr);
}
