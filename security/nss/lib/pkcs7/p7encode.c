










































#include "p7local.h"

#include "cert.h"
#include "cryptohi.h"
#include "keyhi.h"
#include "secasn1.h"
#include "secoid.h"
#include "secitem.h"
#include "pk11func.h"
#include "secerr.h"
#include "sechash.h"	

struct sec_pkcs7_encoder_output {
    SEC_PKCS7EncoderOutputCallback outputfn;
    void *outputarg;
};

struct SEC_PKCS7EncoderContextStr {
    SEC_ASN1EncoderContext *ecx;
    SEC_PKCS7ContentInfo *cinfo;
    struct sec_pkcs7_encoder_output output;
    sec_PKCS7CipherObject *encryptobj;
    const SECHashObject *digestobj;
    void *digestcx;
};







static void
sec_pkcs7_encoder_out(void *arg, const char *buf, unsigned long len,
		      int depth, SEC_ASN1EncodingPart data_kind)
{
    struct sec_pkcs7_encoder_output *output;

    output = (struct sec_pkcs7_encoder_output*)arg;
    output->outputfn (output->outputarg, buf, len);
}

static sec_PKCS7CipherObject *
sec_pkcs7_encoder_start_encrypt (SEC_PKCS7ContentInfo *cinfo,
						 PK11SymKey *orig_bulkkey)
{
    SECOidTag kind;
    sec_PKCS7CipherObject *encryptobj;
    SEC_PKCS7RecipientInfo **recipientinfos, *ri;
    SEC_PKCS7EncryptedContentInfo *enccinfo;
    SECKEYPublicKey *publickey = NULL;
    SECKEYPrivateKey *ourPrivKey = NULL;
    PK11SymKey  *bulkkey;
    void *mark, *wincx;
    int i;
    PRArenaPool *arena = NULL;

    
    wincx = cinfo->pwfn_arg;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
      case SEC_OID_PKCS7_DIGESTED_DATA:
      case SEC_OID_PKCS7_SIGNED_DATA:
	recipientinfos = NULL;
	enccinfo = NULL;
	break;
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	{
	    SEC_PKCS7EncryptedData *encdp;

	    
	    PORT_Assert (orig_bulkkey != NULL);
	    if (orig_bulkkey == NULL) {
		
		return NULL;
	    }

	    encdp = cinfo->content.encryptedData;
	    recipientinfos = NULL;
	    enccinfo = &(encdp->encContentInfo);
	}
	break;
      case SEC_OID_PKCS7_ENVELOPED_DATA:
	{
	    SEC_PKCS7EnvelopedData *envdp;

	    envdp = cinfo->content.envelopedData;
	    recipientinfos = envdp->recipientInfos;
	    enccinfo = &(envdp->encContentInfo);
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    recipientinfos = saedp->recipientInfos;
	    enccinfo = &(saedp->encContentInfo);
	}
	break;
    }

    if (enccinfo == NULL)
	return NULL;

    bulkkey = orig_bulkkey;
    if (bulkkey == NULL) {
	CK_MECHANISM_TYPE type = PK11_AlgtagToMechanism(enccinfo->encalg);
	PK11SlotInfo *slot;


	slot = PK11_GetBestSlot(type,cinfo->pwfn_arg);
	if (slot == NULL) {
	    return NULL;
	}
	bulkkey = PK11_KeyGen(slot,type,NULL, enccinfo->keysize/8,
			      cinfo->pwfn_arg);
	PK11_FreeSlot(slot);
	if (bulkkey == NULL) {
	    return NULL;
	}
    }

    encryptobj = NULL;
    mark = PORT_ArenaMark (cinfo->poolp);

    


    for (i = 0; recipientinfos && (ri = recipientinfos[i]) != NULL; i++) {
	CERTCertificate *cert;
	SECOidTag certalgtag, encalgtag;
	SECStatus rv;
	int data_len;
	SECItem *params = NULL;

	cert = ri->cert;
	PORT_Assert (cert != NULL);
	if (cert == NULL)
	    continue;

	










	certalgtag=SECOID_GetAlgorithmTag(&(cert->subjectPublicKeyInfo.algorithm));

	switch (certalgtag) {
	case SEC_OID_PKCS1_RSA_ENCRYPTION:
	    encalgtag = certalgtag;
	    publickey = CERT_ExtractPublicKey (cert);
	    if (publickey == NULL) goto loser;
		
	    data_len = SECKEY_PublicKeyStrength(publickey);
	    ri->encKey.data = 
	        (unsigned char*)PORT_ArenaAlloc(cinfo->poolp ,data_len);
	    ri->encKey.len = data_len;
	    if (ri->encKey.data == NULL) goto loser;

	    rv = PK11_PubWrapSymKey(PK11_AlgtagToMechanism(certalgtag),publickey,
				bulkkey,&ri->encKey);

	    SECKEY_DestroyPublicKey(publickey);
	    publickey = NULL;
	    if (rv != SECSuccess) goto loser;
	    params = NULL; 
	    break;
	default:
	    PORT_SetError (SEC_ERROR_INVALID_ALGORITHM);
	    goto loser;
	}

	rv = SECOID_SetAlgorithmID(cinfo->poolp, &ri->keyEncAlg, encalgtag, 
			params);
	if (rv != SECSuccess)
	    goto loser;
	if (arena) PORT_FreeArena(arena,PR_FALSE);
	arena = NULL;
    }

    encryptobj = sec_PKCS7CreateEncryptObject (cinfo->poolp, bulkkey,
					       enccinfo->encalg,
					       &(enccinfo->contentEncAlg));
    if (encryptobj != NULL) {
	PORT_ArenaUnmark (cinfo->poolp, mark);
	mark = NULL;		
    }
    

loser:
    if (arena) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    if (publickey) {
        SECKEY_DestroyPublicKey(publickey);
    }
    if (ourPrivKey) {
        SECKEY_DestroyPrivateKey(ourPrivKey);
    }
    if (mark != NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
    }
    if (orig_bulkkey == NULL) {
	if (bulkkey) PK11_FreeSymKey(bulkkey);
    }

    return encryptobj;
}


static void
sec_pkcs7_encoder_notify (void *arg, PRBool before, void *dest, int depth)
{
    SEC_PKCS7EncoderContext *p7ecx;
    SEC_PKCS7ContentInfo *cinfo;
    SECOidTag kind;
    PRBool before_content;

    



    if (!before)
	return;

    p7ecx = (SEC_PKCS7EncoderContext*)arg;
    cinfo = p7ecx->cinfo;

    before_content = PR_FALSE;

    







    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
	if (dest == &(cinfo->content.data))
	    before_content = PR_TRUE;
	break;

      case SEC_OID_PKCS7_DIGESTED_DATA:
	{
	    SEC_PKCS7DigestedData *digd;

	    digd = cinfo->content.digestedData;
	    if (digd == NULL)
		break;

	    if (dest == &(digd->contentInfo.content))
		before_content = PR_TRUE;
	}
	break;

      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	{
	    SEC_PKCS7EncryptedData *encd;

	    encd = cinfo->content.encryptedData;
	    if (encd == NULL)
		break;

	    if (dest == &(encd->encContentInfo.encContent))
		before_content = PR_TRUE;
	}
	break;

      case SEC_OID_PKCS7_ENVELOPED_DATA:
	{
	    SEC_PKCS7EnvelopedData *envd;

	    envd = cinfo->content.envelopedData;
	    if (envd == NULL)
		break;

	    if (dest == &(envd->encContentInfo.encContent))
		before_content = PR_TRUE;
	}
	break;

      case SEC_OID_PKCS7_SIGNED_DATA:
	{
	    SEC_PKCS7SignedData *sigd;

	    sigd = cinfo->content.signedData;
	    if (sigd == NULL)
		break;

	    if (dest == &(sigd->contentInfo.content))
		before_content = PR_TRUE;
	}
	break;

      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saed;

	    saed = cinfo->content.signedAndEnvelopedData;
	    if (saed == NULL)
		break;

	    if (dest == &(saed->encContentInfo.encContent))
		before_content = PR_TRUE;
	}
	break;
    }

    if (before_content) {
	



	SEC_ASN1EncoderSetTakeFromBuf (p7ecx->ecx);
	


	SEC_ASN1EncoderClearNotifyProc (p7ecx->ecx);
    }
}


static SEC_PKCS7EncoderContext *
sec_pkcs7_encoder_start_contexts (SEC_PKCS7ContentInfo *cinfo,
				  PK11SymKey *bulkkey)
{
    SEC_PKCS7EncoderContext *p7ecx;
    SECOidTag kind;
    PRBool encrypt;
    SECItem **digests;
    SECAlgorithmID *digestalg, **digestalgs;

    p7ecx = 
      (SEC_PKCS7EncoderContext*)PORT_ZAlloc (sizeof(SEC_PKCS7EncoderContext));
    if (p7ecx == NULL)
	return NULL;

    digests = NULL;
    digestalg = NULL;
    digestalgs = NULL;
    encrypt = PR_FALSE;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
	break;
      case SEC_OID_PKCS7_DIGESTED_DATA:
	digestalg = &(cinfo->content.digestedData->digestAlg);
	break;
      case SEC_OID_PKCS7_SIGNED_DATA:
	digests = cinfo->content.signedData->digests;
	digestalgs = cinfo->content.signedData->digestAlgorithms;
	break;
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
      case SEC_OID_PKCS7_ENVELOPED_DATA:
	encrypt = PR_TRUE;
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	digests = cinfo->content.signedAndEnvelopedData->digests;
	digestalgs = cinfo->content.signedAndEnvelopedData->digestAlgorithms;
	encrypt = PR_TRUE;
	break;
    }

    if (encrypt) {
	p7ecx->encryptobj = sec_pkcs7_encoder_start_encrypt (cinfo, bulkkey);
	if (p7ecx->encryptobj == NULL) {
	    PORT_Free (p7ecx);
	    return NULL;
	}
    }

    if (digestalgs != NULL) {
	if (digests != NULL) {
	    
	    digestalg = NULL;
	} else {
	    



	    PORT_Assert (digestalgs[0] != NULL && digestalgs[1] == NULL);
	    digestalg = digestalgs[0];
	}
    }

    if (digestalg != NULL) {
	SECOidTag  oidTag = SECOID_FindOIDTag(&(digestalg->algorithm));

	p7ecx->digestobj = HASH_GetHashObjectByOidTag(oidTag);
	if (p7ecx->digestobj != NULL) {
	    p7ecx->digestcx = (* p7ecx->digestobj->create) ();
	    if (p7ecx->digestcx == NULL)
		p7ecx->digestobj = NULL;
	    else
		(* p7ecx->digestobj->begin) (p7ecx->digestcx);
	}
	if (p7ecx->digestobj == NULL) {
	    if (p7ecx->encryptobj != NULL)
		sec_PKCS7DestroyEncryptObject (p7ecx->encryptobj);
	    PORT_Free (p7ecx);
	    return NULL;
	}
    }

    p7ecx->cinfo = cinfo;
    return p7ecx;
}


SEC_PKCS7EncoderContext *
SEC_PKCS7EncoderStart (SEC_PKCS7ContentInfo *cinfo,
		       SEC_PKCS7EncoderOutputCallback outputfn,
		       void *outputarg,
		       PK11SymKey *bulkkey)
{
    SEC_PKCS7EncoderContext *p7ecx;
    SECStatus rv;

    p7ecx = sec_pkcs7_encoder_start_contexts (cinfo, bulkkey);
    if (p7ecx == NULL)
	return NULL;

    p7ecx->output.outputfn = outputfn;
    p7ecx->output.outputarg = outputarg;

    


    p7ecx->ecx = SEC_ASN1EncoderStart (cinfo, sec_PKCS7ContentInfoTemplate,
				       sec_pkcs7_encoder_out, &(p7ecx->output));
    if (p7ecx->ecx == NULL) {
	PORT_Free (p7ecx);
	return NULL;
    }

    



    SEC_ASN1EncoderSetStreaming (p7ecx->ecx);

    


    SEC_ASN1EncoderSetNotifyProc (p7ecx->ecx, sec_pkcs7_encoder_notify, p7ecx);

    





    rv = SEC_ASN1EncoderUpdate (p7ecx->ecx, NULL, 0);
    if (rv != SECSuccess) {
	PORT_Free (p7ecx);
	return NULL;
    }

    return p7ecx;
}





static SECStatus
sec_pkcs7_encoder_work_data (SEC_PKCS7EncoderContext *p7ecx, SECItem *dest,
			     const unsigned char *data, unsigned long len,
			     PRBool final)
{
    unsigned char *buf = NULL;
    SECStatus rv;


    rv = SECSuccess;		

    






    PORT_Assert ((data != NULL && len) || final);

    



    if (len && p7ecx->digestobj != NULL) {
	(* p7ecx->digestobj->update) (p7ecx->digestcx, data, len);
    }

    


    if (p7ecx->encryptobj != NULL) {
	
	unsigned int inlen;	
	unsigned int outlen;	
	unsigned int buflen;	

	inlen = len;
	buflen = sec_PKCS7EncryptLength (p7ecx->encryptobj, inlen, final);
	if (buflen == 0) {
	    



	    rv = sec_PKCS7Encrypt (p7ecx->encryptobj, NULL, NULL, 0,
				   data, inlen, final);
	    if (final) {
		len = 0;
		goto done;
	    }
	    return rv;
	}

	if (dest != NULL)
	    buf = (unsigned char*)PORT_ArenaAlloc(p7ecx->cinfo->poolp, buflen);
	else
	    buf = (unsigned char*)PORT_Alloc (buflen);

	if (buf == NULL) {
	    rv = SECFailure;
	} else {
	    rv = sec_PKCS7Encrypt (p7ecx->encryptobj, buf, &outlen, buflen,
				   data, inlen, final);
	    data = buf;
	    len = outlen;
	}
	if (rv != SECSuccess) {
	    if (final)
		goto done;
	    return rv;
	}
    }

    if (p7ecx->ecx != NULL) {
	


	if(len) {
	    rv = SEC_ASN1EncoderUpdate (p7ecx->ecx, (const char *)data, len);
	}
    }

done:
    if (p7ecx->encryptobj != NULL) {
	if (final)
	    sec_PKCS7DestroyEncryptObject (p7ecx->encryptobj);
	if (dest != NULL) {
	    dest->data = buf;
	    dest->len = len;
	} else if (buf != NULL) {
	    PORT_Free (buf);
	}
    }

    if (final && p7ecx->digestobj != NULL) {
	SECItem *digest, **digests, ***digestsp;
	unsigned char *digdata;
	SECOidTag kind;

	kind = SEC_PKCS7ContentType (p7ecx->cinfo);
	switch (kind) {
	  default:
	    PORT_Assert (0);
	    return SECFailure;
	  case SEC_OID_PKCS7_DIGESTED_DATA:
	    digest = &(p7ecx->cinfo->content.digestedData->digest);
	    digestsp = NULL;
	    break;
	  case SEC_OID_PKCS7_SIGNED_DATA:
	    digest = NULL;
	    digestsp = &(p7ecx->cinfo->content.signedData->digests);
	    break;
	  case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	    digest = NULL;
	    digestsp = &(p7ecx->cinfo->content.signedAndEnvelopedData->digests);
	    break;
	}

	digdata = (unsigned char*)PORT_ArenaAlloc (p7ecx->cinfo->poolp,
				   p7ecx->digestobj->length);
	if (digdata == NULL)
	    return SECFailure;

	if (digestsp != NULL) {
	    PORT_Assert (digest == NULL);

	    digest = (SECItem*)PORT_ArenaAlloc (p7ecx->cinfo->poolp, 
						sizeof(SECItem));
	    digests = (SECItem**)PORT_ArenaAlloc (p7ecx->cinfo->poolp,
				       2 * sizeof(SECItem *));
	    if (digests == NULL || digest == NULL)
		return SECFailure;

	    digests[0] = digest;
	    digests[1] = NULL;

	    *digestsp = digests;
	}

	PORT_Assert (digest != NULL);

	digest->data = digdata;
	digest->len = p7ecx->digestobj->length;

	(* p7ecx->digestobj->end) (p7ecx->digestcx, digest->data,
				   &(digest->len), digest->len);
	(* p7ecx->digestobj->destroy) (p7ecx->digestcx, PR_TRUE);
    }

    return rv;
}


SECStatus
SEC_PKCS7EncoderUpdate (SEC_PKCS7EncoderContext *p7ecx,
			const char *data, unsigned long len)
{
    
    return sec_pkcs7_encoder_work_data (p7ecx, NULL,
					(const unsigned char *)data, len,
					PR_FALSE);
}

static SECStatus
sec_pkcs7_encoder_sig_and_certs (SEC_PKCS7ContentInfo *cinfo,
				 SECKEYGetPasswordKey pwfn, void *pwfnarg)
{
    SECOidTag kind;
    CERTCertificate **certs;
    CERTCertificateList **certlists;
    SECAlgorithmID **digestalgs;
    SECItem **digests;
    SEC_PKCS7SignerInfo *signerinfo, **signerinfos;
    SECItem **rawcerts, ***rawcertsp;
    PRArenaPool *poolp;
    int certcount;
    int ci, cli, rci, si;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
      case SEC_OID_PKCS7_DIGESTED_DATA:
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
      case SEC_OID_PKCS7_ENVELOPED_DATA:
	certs = NULL;
	certlists = NULL;
	digestalgs = NULL;
	digests = NULL;
	signerinfos = NULL;
	rawcertsp = NULL;
	break;
      case SEC_OID_PKCS7_SIGNED_DATA:
	{
	    SEC_PKCS7SignedData *sdp;

	    sdp = cinfo->content.signedData;
	    certs = sdp->certs;
	    certlists = sdp->certLists;
	    digestalgs = sdp->digestAlgorithms;
	    digests = sdp->digests;
	    signerinfos = sdp->signerInfos;
	    rawcertsp = &(sdp->rawCerts);
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    certs = saedp->certs;
	    certlists = saedp->certLists;
	    digestalgs = saedp->digestAlgorithms;
	    digests = saedp->digests;
	    signerinfos = saedp->signerInfos;
	    rawcertsp = &(saedp->rawCerts);
	}
	break;
    }

    if (certs == NULL && certlists == NULL && signerinfos == NULL)
	return SECSuccess;		

    poolp = cinfo->poolp;
    certcount = 0;

    if (signerinfos != NULL) {
	SECOidTag digestalgtag;
	int di;
	SECStatus rv;
	CERTCertificate *cert;
	SECKEYPrivateKey *privkey;
	SECItem signature;
	SECOidTag signalgtag;

	PORT_Assert (digestalgs != NULL && digests != NULL);

	




	for (si = 0; signerinfos[si] != NULL; si++) {

	    signerinfo = signerinfos[si];

	    
	    digestalgtag = SECOID_GetAlgorithmTag (&(signerinfo->digestAlg));
	    for (di = 0; digestalgs[di] != NULL; di++) {
		
		if (digestalgtag == SECOID_GetAlgorithmTag (digestalgs[di]))
		    break;
	    }
	    if (digestalgs[di] == NULL) {
		
		return SECFailure;
	    }
	    PORT_Assert (digests[di] != NULL);

	    cert = signerinfo->cert;
	    privkey = PK11_FindKeyByAnyCert (cert, pwfnarg);
	    if (privkey == NULL)
		return SECFailure;

	    



	    signalgtag = SECOID_GetAlgorithmTag (&(cert->subjectPublicKeyInfo.algorithm));

	    if (signerinfo->authAttr != NULL) {
		SEC_PKCS7Attribute *attr;
		SECItem encoded_attrs;
		SECItem *dummy;
		SECOidTag algid;

		


		attr = sec_PKCS7FindAttribute (signerinfo->authAttr,
					       SEC_OID_PKCS9_MESSAGE_DIGEST,
					       PR_TRUE);
		PORT_Assert (attr != NULL);
		if (attr == NULL) {
		    SECKEY_DestroyPrivateKey (privkey);
		    return SECFailure;
		}

		






		PORT_Assert (attr->values != NULL && attr->values[0] == NULL);
		attr->values[0] = digests[di];

		











		rv = sec_PKCS7ReorderAttributes (signerinfo->authAttr);
		if (rv != SECSuccess) {
		    SECKEY_DestroyPrivateKey (privkey);
		    return SECFailure;
		}

		encoded_attrs.data = NULL;
		encoded_attrs.len = 0;
		dummy = sec_PKCS7EncodeAttributes (NULL, &encoded_attrs,
						   &(signerinfo->authAttr));
		if (dummy == NULL) {
		    SECKEY_DestroyPrivateKey (privkey);
		    return SECFailure;
		}

	        algid = SEC_GetSignatureAlgorithmOidTag(privkey->keyType,
 							digestalgtag);
		if (algid == SEC_OID_UNKNOWN) {
		    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
		    SECKEY_DestroyPrivateKey (privkey);
		    return SECFailure;
		}
		rv = SEC_SignData (&signature,
				   encoded_attrs.data, encoded_attrs.len,
				   privkey,
				   algid);
		SECITEM_FreeItem (&encoded_attrs, PR_FALSE);
	    } else {
		rv = SGN_Digest (privkey, digestalgtag, &signature,
				 digests[di]);
	    }

	    SECKEY_DestroyPrivateKey (privkey);

	    if (rv != SECSuccess)
		return rv;

	    rv = SECITEM_CopyItem (poolp, &(signerinfo->encDigest), &signature);
	    if (rv != SECSuccess)
		return rv;

	    SECITEM_FreeItem (&signature, PR_FALSE);

	    rv = SECOID_SetAlgorithmID (poolp, &(signerinfo->digestEncAlg),
					signalgtag, NULL);
	    if (rv != SECSuccess)
		return SECFailure;

	    


	    if (signerinfo->certList != NULL)
		certcount += signerinfo->certList->len;
	}
    }

    if (certs != NULL) {
	for (ci = 0; certs[ci] != NULL; ci++)
	    certcount++;
    }

    if (certlists != NULL) {
	for (cli = 0; certlists[cli] != NULL; cli++)
	    certcount += certlists[cli]->len;
    }

    if (certcount == 0)
	return SECSuccess;		

    





    rawcerts = (SECItem**)PORT_ArenaAlloc (poolp, 
					(certcount + 1) * sizeof(SECItem *));
    if (rawcerts == NULL)
	return SECFailure;

    





    rci = 0;
    if (signerinfos != NULL) {
	for (si = 0; signerinfos[si] != NULL; si++) {
	    signerinfo = signerinfos[si];
	    for (ci = 0; ci < signerinfo->certList->len; ci++)
		rawcerts[rci++] = &(signerinfo->certList->certs[ci]);
	}

    }

    if (certs != NULL) {
	for (ci = 0; certs[ci] != NULL; ci++)
	    rawcerts[rci++] = &(certs[ci]->derCert);
    }

    if (certlists != NULL) {
	for (cli = 0; certlists[cli] != NULL; cli++) {
	    for (ci = 0; ci < certlists[cli]->len; ci++)
		rawcerts[rci++] = &(certlists[cli]->certs[ci]);
	}
    }

    rawcerts[rci] = NULL;
    *rawcertsp = rawcerts;

    return SECSuccess;
}


SECStatus
SEC_PKCS7EncoderFinish (SEC_PKCS7EncoderContext *p7ecx,
			SECKEYGetPasswordKey pwfn, void *pwfnarg)
{
    SECStatus rv;

    


    rv = sec_pkcs7_encoder_work_data (p7ecx, NULL, NULL, 0, PR_TRUE);

    


    SEC_ASN1EncoderClearTakeFromBuf (p7ecx->ecx);
    SEC_ASN1EncoderClearStreaming (p7ecx->ecx);

    if (rv != SECSuccess)
	goto loser;

    rv = sec_pkcs7_encoder_sig_and_certs (p7ecx->cinfo, pwfn, pwfnarg);
    if (rv != SECSuccess)
	goto loser;

    rv = SEC_ASN1EncoderUpdate (p7ecx->ecx, NULL, 0);

loser:
    SEC_ASN1EncoderFinish (p7ecx->ecx);
    PORT_Free (p7ecx);
    return rv;
}




void
SEC_PKCS7EncoderAbort(SEC_PKCS7EncoderContext *p7ecx, int error)
{
    PORT_Assert(p7ecx);
    SEC_ASN1EncoderAbort(p7ecx->ecx, error);
}







SECStatus
SEC_PKCS7PrepareForEncode (SEC_PKCS7ContentInfo *cinfo,
			   PK11SymKey *bulkkey,
			   SECKEYGetPasswordKey pwfn,
			   void *pwfnarg)
{
    SEC_PKCS7EncoderContext *p7ecx;
    SECItem *content, *enc_content;
    SECStatus rv;

    p7ecx = sec_pkcs7_encoder_start_contexts (cinfo, bulkkey);
    if (p7ecx == NULL)
	return SECFailure;

    content = SEC_PKCS7GetContent (cinfo);

    if (p7ecx->encryptobj != NULL) {
	SECOidTag kind;
	SEC_PKCS7EncryptedContentInfo *enccinfo;

	kind = SEC_PKCS7ContentType (p7ecx->cinfo);
	switch (kind) {
	  default:
	    PORT_Assert (0);
	    rv = SECFailure;
	    goto loser;
	  case SEC_OID_PKCS7_ENCRYPTED_DATA:
	    enccinfo = &(p7ecx->cinfo->content.encryptedData->encContentInfo);
	    break;
	  case SEC_OID_PKCS7_ENVELOPED_DATA:
	    enccinfo = &(p7ecx->cinfo->content.envelopedData->encContentInfo);
	    break;
	  case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	    enccinfo = &(p7ecx->cinfo->content.signedAndEnvelopedData->encContentInfo);
	    break;
	}
	enc_content = &(enccinfo->encContent);
    } else {
	enc_content = NULL;
    }

    if (content != NULL && content->data != NULL && content->len) {
	rv = sec_pkcs7_encoder_work_data (p7ecx, enc_content,
					  content->data, content->len, PR_TRUE);
	if (rv != SECSuccess)
	    goto loser;
    }

    rv = sec_pkcs7_encoder_sig_and_certs (cinfo, pwfn, pwfnarg);

loser:
    PORT_Free (p7ecx);
    return rv;
}

























SECStatus
SEC_PKCS7Encode (SEC_PKCS7ContentInfo *cinfo,
		 SEC_PKCS7EncoderOutputCallback outputfn,
		 void *outputarg,
		 PK11SymKey *bulkkey,
		 SECKEYGetPasswordKey pwfn,
		 void *pwfnarg)
{
    SECStatus rv;

    rv = SEC_PKCS7PrepareForEncode (cinfo, bulkkey, pwfn, pwfnarg);
    if (rv == SECSuccess) {
	struct sec_pkcs7_encoder_output outputcx;

	outputcx.outputfn = outputfn;
	outputcx.outputarg = outputarg;

	rv = SEC_ASN1Encode (cinfo, sec_PKCS7ContentInfoTemplate,
			     sec_pkcs7_encoder_out, &outputcx);
    }

    return rv;
}





























SECItem *
SEC_PKCS7EncodeItem (PRArenaPool *pool,
		     SECItem *dest,
		     SEC_PKCS7ContentInfo *cinfo,
		     PK11SymKey *bulkkey,
		     SECKEYGetPasswordKey pwfn,
		     void *pwfnarg)
{
    SECStatus rv;

    rv = SEC_PKCS7PrepareForEncode (cinfo, bulkkey, pwfn, pwfnarg);
    if (rv != SECSuccess)
	return NULL;

    return SEC_ASN1EncodeItem (pool, dest, cinfo, sec_PKCS7ContentInfoTemplate);
}

