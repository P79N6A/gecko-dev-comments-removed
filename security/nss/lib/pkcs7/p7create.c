









#include "p7local.h"

#include "cert.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "prtime.h"
#include "secerr.h"
#include "secder.h"
#include "secpkcs5.h"

const int NSS_PBE_DEFAULT_ITERATION_COUNT = 2000; 

static SECStatus
sec_pkcs7_init_content_info (SEC_PKCS7ContentInfo *cinfo, PRArenaPool *poolp,
			     SECOidTag kind, PRBool detached)
{
    void *thing;
    int version;
    SECItem *versionp;
    SECStatus rv;

    PORT_Assert (cinfo != NULL && poolp != NULL);
    if (cinfo == NULL || poolp == NULL)
	return SECFailure;

    cinfo->contentTypeTag = SECOID_FindOIDByTag (kind);
    PORT_Assert (cinfo->contentTypeTag
		 && cinfo->contentTypeTag->offset == kind);

    rv = SECITEM_CopyItem (poolp, &(cinfo->contentType),
			   &(cinfo->contentTypeTag->oid));
    if (rv != SECSuccess)
	return rv;

    if (detached)
	return SECSuccess;

    switch (kind) {
      default:
      case SEC_OID_PKCS7_DATA:
	thing = PORT_ArenaZAlloc (poolp, sizeof(SECItem));
	cinfo->content.data = (SECItem*)thing;
	versionp = NULL;
	version = -1;
	break;
      case SEC_OID_PKCS7_DIGESTED_DATA:
	thing = PORT_ArenaZAlloc (poolp, sizeof(SEC_PKCS7DigestedData));
	cinfo->content.digestedData = (SEC_PKCS7DigestedData*)thing;
	versionp = &(cinfo->content.digestedData->version);
	version = SEC_PKCS7_DIGESTED_DATA_VERSION;
	break;
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	thing = PORT_ArenaZAlloc (poolp, sizeof(SEC_PKCS7EncryptedData));
	cinfo->content.encryptedData = (SEC_PKCS7EncryptedData*)thing;
	versionp = &(cinfo->content.encryptedData->version);
	version = SEC_PKCS7_ENCRYPTED_DATA_VERSION;
	break;
      case SEC_OID_PKCS7_ENVELOPED_DATA:
	thing = PORT_ArenaZAlloc (poolp, sizeof(SEC_PKCS7EnvelopedData));
	cinfo->content.envelopedData = 
	  (SEC_PKCS7EnvelopedData*)thing;
	versionp = &(cinfo->content.envelopedData->version);
	version = SEC_PKCS7_ENVELOPED_DATA_VERSION;
	break;
      case SEC_OID_PKCS7_SIGNED_DATA:
	thing = PORT_ArenaZAlloc (poolp, sizeof(SEC_PKCS7SignedData));
	cinfo->content.signedData = 
	  (SEC_PKCS7SignedData*)thing;
	versionp = &(cinfo->content.signedData->version);
	version = SEC_PKCS7_SIGNED_DATA_VERSION;
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	thing = PORT_ArenaZAlloc(poolp,sizeof(SEC_PKCS7SignedAndEnvelopedData));
	cinfo->content.signedAndEnvelopedData = 
	  (SEC_PKCS7SignedAndEnvelopedData*)thing;
	versionp = &(cinfo->content.signedAndEnvelopedData->version);
	version = SEC_PKCS7_SIGNED_AND_ENVELOPED_DATA_VERSION;
	break;
    }

    if (thing == NULL)
	return SECFailure;

    if (versionp != NULL) {
	SECItem *dummy;

	PORT_Assert (version >= 0);
	dummy = SEC_ASN1EncodeInteger (poolp, versionp, version);
	if (dummy == NULL)
	    return SECFailure;
	PORT_Assert (dummy == versionp);
    }

    return SECSuccess;
}


static SEC_PKCS7ContentInfo *
sec_pkcs7_create_content_info (SECOidTag kind, PRBool detached,
			       SECKEYGetPasswordKey pwfn, void *pwfn_arg)
{
    SEC_PKCS7ContentInfo *cinfo;
    PRArenaPool *poolp;
    SECStatus rv;

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
    cinfo->created = PR_TRUE;
    cinfo->refCount = 1;

    rv = sec_pkcs7_init_content_info (cinfo, poolp, kind, detached);
    if (rv != SECSuccess) {
	PORT_FreeArena (poolp, PR_FALSE);
	return NULL;
    }

    return cinfo;
}









static SECStatus
sec_pkcs7_add_signer (SEC_PKCS7ContentInfo *cinfo,
		      CERTCertificate *     cert,
		      SECCertUsage          certusage,
		      CERTCertDBHandle *    certdb,
		      SECOidTag             digestalgtag,
		      SECItem *             digestdata)
{
    SEC_PKCS7SignerInfo *signerinfo, **signerinfos, ***signerinfosp;
    SECAlgorithmID      *digestalg,  **digestalgs,  ***digestalgsp;
    SECItem             *digest,     **digests,     ***digestsp;
    SECItem *            dummy;
    void *               mark;
    SECStatus            rv;
    SECOidTag            kind;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      case SEC_OID_PKCS7_SIGNED_DATA:
	{
	    SEC_PKCS7SignedData *sdp;

	    sdp = cinfo->content.signedData;
	    digestalgsp = &(sdp->digestAlgorithms);
	    digestsp = &(sdp->digests);
	    signerinfosp = &(sdp->signerInfos);
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    digestalgsp = &(saedp->digestAlgorithms);
	    digestsp = &(saedp->digests);
	    signerinfosp = &(saedp->signerInfos);
	}
	break;
      default:
	return SECFailure;		
    }

    



    if (certdb == NULL) {
	certdb = CERT_GetDefaultCertDB();
	if (certdb == NULL)
	    return SECFailure;		
    }

    if (CERT_VerifyCert (certdb, cert, PR_TRUE, certusage, PR_Now(),
			 cinfo->pwfn_arg, NULL) != SECSuccess)
	{
	
	return SECFailure;
    }

    




    PORT_Assert (*signerinfosp == NULL
		 && *digestalgsp == NULL && *digestsp == NULL);
    if (*signerinfosp != NULL || *digestalgsp != NULL || *digestsp != NULL)
	return SECFailure;

    mark = PORT_ArenaMark (cinfo->poolp);

    signerinfo = (SEC_PKCS7SignerInfo*)PORT_ArenaZAlloc (cinfo->poolp, 
						  sizeof(SEC_PKCS7SignerInfo));
    if (signerinfo == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    dummy = SEC_ASN1EncodeInteger (cinfo->poolp, &signerinfo->version,
				   SEC_PKCS7_SIGNER_INFO_VERSION);
    if (dummy == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }
    PORT_Assert (dummy == &signerinfo->version);

    signerinfo->cert = CERT_DupCertificate (cert);
    if (signerinfo->cert == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    signerinfo->issuerAndSN = CERT_GetCertIssuerAndSN (cinfo->poolp, cert);
    if (signerinfo->issuerAndSN == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    rv = SECOID_SetAlgorithmID (cinfo->poolp, &signerinfo->digestAlg,
				digestalgtag, NULL);
    if (rv != SECSuccess) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    













    signerinfos = (SEC_PKCS7SignerInfo**)PORT_ArenaAlloc (cinfo->poolp,
				   2 * sizeof(SEC_PKCS7SignerInfo *));
    if (signerinfos == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }
    signerinfos[0] = signerinfo;
    signerinfos[1] = NULL;

    digestalg = PORT_ArenaZAlloc (cinfo->poolp, sizeof(SECAlgorithmID));
    digestalgs = PORT_ArenaAlloc (cinfo->poolp, 2 * sizeof(SECAlgorithmID *));
    if (digestalg == NULL || digestalgs == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }
    rv = SECOID_SetAlgorithmID (cinfo->poolp, digestalg, digestalgtag, NULL);
    if (rv != SECSuccess) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }
    digestalgs[0] = digestalg;
    digestalgs[1] = NULL;

    if (digestdata != NULL) {
	digest = (SECItem*)PORT_ArenaAlloc (cinfo->poolp, sizeof(SECItem));
	digests = (SECItem**)PORT_ArenaAlloc (cinfo->poolp, 
					      2 * sizeof(SECItem *));
	if (digest == NULL || digests == NULL) {
	    PORT_ArenaRelease (cinfo->poolp, mark);
	    return SECFailure;
	}
	rv = SECITEM_CopyItem (cinfo->poolp, digest, digestdata);
	if (rv != SECSuccess) {
	    PORT_ArenaRelease (cinfo->poolp, mark);
	    return SECFailure;
	}
	digests[0] = digest;
	digests[1] = NULL;
    } else {
	digests = NULL;
    }

    *signerinfosp = signerinfos;
    *digestalgsp = digestalgs;
    *digestsp = digests;

    PORT_ArenaUnmark(cinfo->poolp, mark);
    return SECSuccess;
}





static SEC_PKCS7ContentInfo *
sec_pkcs7_create_signed_data (SECKEYGetPasswordKey pwfn, void *pwfn_arg)
{
    SEC_PKCS7ContentInfo *cinfo;
    SEC_PKCS7SignedData *sigd;
    SECStatus rv;

    cinfo = sec_pkcs7_create_content_info (SEC_OID_PKCS7_SIGNED_DATA, PR_FALSE,
					   pwfn, pwfn_arg);
    if (cinfo == NULL)
	return NULL;

    sigd = cinfo->content.signedData;
    PORT_Assert (sigd != NULL);

    



    rv = sec_pkcs7_init_content_info (&(sigd->contentInfo), cinfo->poolp,
				      SEC_OID_PKCS7_DATA, PR_TRUE);
    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return NULL;
    }

    return cinfo;
}






























SEC_PKCS7ContentInfo *
SEC_PKCS7CreateSignedData (CERTCertificate *cert,
			   SECCertUsage certusage,
			   CERTCertDBHandle *certdb,
			   SECOidTag digestalg,
			   SECItem *digest,
 			   SECKEYGetPasswordKey pwfn, void *pwfn_arg)
{
    SEC_PKCS7ContentInfo *cinfo;
    SECStatus rv;

    cinfo = sec_pkcs7_create_signed_data (pwfn, pwfn_arg);
    if (cinfo == NULL)
	return NULL;

    rv = sec_pkcs7_add_signer (cinfo, cert, certusage, certdb,
			       digestalg, digest);
    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return NULL;
    }

    return cinfo;
}


static SEC_PKCS7Attribute *
sec_pkcs7_create_attribute (PRArenaPool *poolp, SECOidTag oidtag,
			    SECItem *value, PRBool encoded)
{
    SEC_PKCS7Attribute *attr;
    SECItem **values;
    void *mark;

    PORT_Assert (poolp != NULL);
    mark = PORT_ArenaMark (poolp);

    attr = (SEC_PKCS7Attribute*)PORT_ArenaAlloc (poolp, 
						 sizeof(SEC_PKCS7Attribute));
    if (attr == NULL)
	goto loser;

    attr->typeTag = SECOID_FindOIDByTag (oidtag);
    if (attr->typeTag == NULL)
	goto loser;

    if (SECITEM_CopyItem (poolp, &(attr->type),
			  &(attr->typeTag->oid)) != SECSuccess)
	goto loser;

    values = (SECItem**)PORT_ArenaAlloc (poolp, 2 * sizeof(SECItem *));
    if (values == NULL)
	goto loser;

    if (value != NULL) {
	SECItem *copy;

	copy = (SECItem*)PORT_ArenaAlloc (poolp, sizeof(SECItem));
	if (copy == NULL)
	    goto loser;

	if (SECITEM_CopyItem (poolp, copy, value) != SECSuccess)
	    goto loser;

	value = copy;
    }

    values[0] = value;
    values[1] = NULL;
    attr->values = values;
    attr->encoded = encoded;

    PORT_ArenaUnmark (poolp, mark);
    return attr;

loser:
    PORT_Assert (mark != NULL);
    PORT_ArenaRelease (poolp, mark);
    return NULL;
}


static SECStatus
sec_pkcs7_add_attribute (SEC_PKCS7ContentInfo *cinfo,
			 SEC_PKCS7Attribute ***attrsp,
			 SEC_PKCS7Attribute *attr)
{
    SEC_PKCS7Attribute **attrs;
    SECItem *ct_value;
    void *mark;

    PORT_Assert (SEC_PKCS7ContentType (cinfo) == SEC_OID_PKCS7_SIGNED_DATA);
    if (SEC_PKCS7ContentType (cinfo) != SEC_OID_PKCS7_SIGNED_DATA)
	return SECFailure;

    attrs = *attrsp;
    if (attrs != NULL) {
	int count;

	




	



	PORT_Assert (sec_PKCS7FindAttribute (attrs,
					     SEC_OID_PKCS9_CONTENT_TYPE,
					     PR_FALSE) != NULL);
	PORT_Assert (sec_PKCS7FindAttribute (attrs,
					     SEC_OID_PKCS9_MESSAGE_DIGEST,
					     PR_FALSE) != NULL);

	for (count = 0; attrs[count] != NULL; count++)
	    ;
	attrs = (SEC_PKCS7Attribute**)PORT_ArenaGrow (cinfo->poolp, attrs,
				(count + 1) * sizeof(SEC_PKCS7Attribute *),
				(count + 2) * sizeof(SEC_PKCS7Attribute *));
	if (attrs == NULL)
	    return SECFailure;

	attrs[count] = attr;
	attrs[count+1] = NULL;
	*attrsp = attrs;

	return SECSuccess;
    }

    





    



    attrs = (SEC_PKCS7Attribute**)PORT_ArenaAlloc (cinfo->poolp, 
					   4 * sizeof(SEC_PKCS7Attribute *));
    if (attrs == NULL)
	return SECFailure;

    mark = PORT_ArenaMark (cinfo->poolp);

    



    ct_value = &(cinfo->content.signedData->contentInfo.contentType);
    attrs[0] = sec_pkcs7_create_attribute (cinfo->poolp,
					   SEC_OID_PKCS9_CONTENT_TYPE,
					   ct_value, PR_FALSE);
    




    attrs[1] = sec_pkcs7_create_attribute (cinfo->poolp,
					   SEC_OID_PKCS9_MESSAGE_DIGEST,
					   NULL, PR_FALSE);
    if (attrs[0] == NULL || attrs[1] == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure; 
    }

    attrs[2] = attr;
    attrs[3] = NULL;
    *attrsp = attrs;

    PORT_ArenaUnmark (cinfo->poolp, mark);
    return SECSuccess;
}


















SECStatus
SEC_PKCS7AddSigningTime (SEC_PKCS7ContentInfo *cinfo)
{
    SEC_PKCS7SignerInfo **signerinfos;
    SEC_PKCS7Attribute *attr;
    SECItem stime;
    SECStatus rv;
    int si;

    PORT_Assert (SEC_PKCS7ContentType (cinfo) == SEC_OID_PKCS7_SIGNED_DATA);
    if (SEC_PKCS7ContentType (cinfo) != SEC_OID_PKCS7_SIGNED_DATA)
	return SECFailure;

    signerinfos = cinfo->content.signedData->signerInfos;

    
    if (signerinfos == NULL || signerinfos[0] == NULL)
	return SECFailure;

    rv = DER_EncodeTimeChoice(NULL, &stime, PR_Now());
    if (rv != SECSuccess)
	return rv;

    attr = sec_pkcs7_create_attribute (cinfo->poolp,
				       SEC_OID_PKCS9_SIGNING_TIME,
				       &stime, PR_FALSE);
    SECITEM_FreeItem (&stime, PR_FALSE);

    if (attr == NULL)
	return SECFailure;

    rv = SECSuccess;
    for (si = 0; signerinfos[si] != NULL; si++) {
	SEC_PKCS7Attribute *oattr;

	oattr = sec_PKCS7FindAttribute (signerinfos[si]->authAttr,
					SEC_OID_PKCS9_SIGNING_TIME, PR_FALSE);
	PORT_Assert (oattr == NULL);
	if (oattr != NULL)
	    continue;	

	rv = sec_pkcs7_add_attribute (cinfo, &(signerinfos[si]->authAttr),
				      attr);
	if (rv != SECSuccess)
	    break;	
    }

    return rv;
}




















SECStatus
SEC_PKCS7AddSignedAttribute (SEC_PKCS7ContentInfo *cinfo,
			     SECOidTag oidtag,
			     SECItem *value)
{
    SEC_PKCS7SignerInfo **signerinfos;
    SEC_PKCS7Attribute *attr;

    PORT_Assert (SEC_PKCS7ContentType (cinfo) == SEC_OID_PKCS7_SIGNED_DATA);
    if (SEC_PKCS7ContentType (cinfo) != SEC_OID_PKCS7_SIGNED_DATA)
	return SECFailure;

    signerinfos = cinfo->content.signedData->signerInfos;

    


    if (signerinfos == NULL || signerinfos[0] == NULL || signerinfos[1] != NULL)
	return SECFailure;

    attr = sec_pkcs7_create_attribute (cinfo->poolp, oidtag, value, PR_TRUE);
    if (attr == NULL)
	return SECFailure;

    return sec_pkcs7_add_attribute (cinfo, &(signerinfos[0]->authAttr), attr);
}
 












SECStatus
SEC_PKCS7IncludeCertChain (SEC_PKCS7ContentInfo *cinfo,
			   CERTCertDBHandle *certdb)
{
    SECOidTag kind;
    SEC_PKCS7SignerInfo *signerinfo, **signerinfos;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      case SEC_OID_PKCS7_SIGNED_DATA:
	signerinfos = cinfo->content.signedData->signerInfos;
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	signerinfos = cinfo->content.signedAndEnvelopedData->signerInfos;
	break;
      default:
	return SECFailure;		
    }

    if (signerinfos == NULL)		
	return SECFailure;		

    if (certdb == NULL) {
	certdb = CERT_GetDefaultCertDB();
	if (certdb == NULL) {
	    PORT_SetError (SEC_ERROR_BAD_DATABASE);
	    return SECFailure;
	}
    }

    
    while ((signerinfo = *signerinfos++) != NULL) {
	if (signerinfo->cert != NULL)
	    


	    signerinfo->certList = CERT_CertChainFromCert (signerinfo->cert,
							   certUsageEmailSigner,
							   PR_FALSE);
    }

    return SECSuccess;
}






static SECStatus
sec_pkcs7_add_cert_chain (SEC_PKCS7ContentInfo *cinfo,
			  CERTCertificate *cert,
			  CERTCertDBHandle *certdb)
{
    SECOidTag kind;
    CERTCertificateList *certlist, **certlists, ***certlistsp;
    int count;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      case SEC_OID_PKCS7_SIGNED_DATA:
	{
	    SEC_PKCS7SignedData *sdp;

	    sdp = cinfo->content.signedData;
	    certlistsp = &(sdp->certLists);
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    certlistsp = &(saedp->certLists);
	}
	break;
      default:
	return SECFailure;		
    }

    if (certdb == NULL) {
	certdb = CERT_GetDefaultCertDB();
	if (certdb == NULL) {
	    PORT_SetError (SEC_ERROR_BAD_DATABASE);
	    return SECFailure;
	}
    }

    certlist = CERT_CertChainFromCert (cert, certUsageEmailSigner, PR_FALSE);
    if (certlist == NULL)
	return SECFailure;

    certlists = *certlistsp;
    if (certlists == NULL) {
	count = 0;
	certlists = (CERTCertificateList**)PORT_ArenaAlloc (cinfo->poolp,
				     2 * sizeof(CERTCertificateList *));
    } else {
	for (count = 0; certlists[count] != NULL; count++)
	    ;
	PORT_Assert (count);	
	certlists = (CERTCertificateList**)PORT_ArenaGrow (cinfo->poolp, 
				 certlists,
				(count + 1) * sizeof(CERTCertificateList *),
				(count + 2) * sizeof(CERTCertificateList *));
    }

    if (certlists == NULL) {
	CERT_DestroyCertificateList (certlist);
	return SECFailure;
    }

    certlists[count] = certlist;
    certlists[count + 1] = NULL;

    *certlistsp = certlists;

    return SECSuccess;
}






static SECStatus
sec_pkcs7_add_certificate (SEC_PKCS7ContentInfo *cinfo,
			   CERTCertificate *cert)
{
    SECOidTag kind;
    CERTCertificate **certs, ***certsp;
    int count;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      case SEC_OID_PKCS7_SIGNED_DATA:
	{
	    SEC_PKCS7SignedData *sdp;

	    sdp = cinfo->content.signedData;
	    certsp = &(sdp->certs);
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    certsp = &(saedp->certs);
	}
	break;
      default:
	return SECFailure;		
    }

    cert = CERT_DupCertificate (cert);
    if (cert == NULL)
	return SECFailure;

    certs = *certsp;
    if (certs == NULL) {
	count = 0;
	certs = (CERTCertificate**)PORT_ArenaAlloc (cinfo->poolp, 
					      2 * sizeof(CERTCertificate *));
    } else {
	for (count = 0; certs[count] != NULL; count++)
	    ;
	PORT_Assert (count);	
	certs = (CERTCertificate**)PORT_ArenaGrow (cinfo->poolp, certs,
				(count + 1) * sizeof(CERTCertificate *),
				(count + 2) * sizeof(CERTCertificate *));
    }

    if (certs == NULL) {
	CERT_DestroyCertificate (cert);
	return SECFailure;
    }

    certs[count] = cert;
    certs[count + 1] = NULL;

    *certsp = certs;

    return SECSuccess;
}



















SEC_PKCS7ContentInfo *
SEC_PKCS7CreateCertsOnly (CERTCertificate *cert,
			  PRBool include_chain,
			  CERTCertDBHandle *certdb)
{
    SEC_PKCS7ContentInfo *cinfo;
    SECStatus rv;

    cinfo = sec_pkcs7_create_signed_data (NULL, NULL);
    if (cinfo == NULL)
	return NULL;

    if (include_chain)
	rv = sec_pkcs7_add_cert_chain (cinfo, cert, certdb);
    else
	rv = sec_pkcs7_add_certificate (cinfo, cert);

    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return NULL;
    }

    return cinfo;
}











SECStatus
SEC_PKCS7AddCertChain (SEC_PKCS7ContentInfo *cinfo,
		       CERTCertificate *cert,
		       CERTCertDBHandle *certdb)
{
    SECOidTag kind;

    kind = SEC_PKCS7ContentType (cinfo);
    if (kind != SEC_OID_PKCS7_SIGNED_DATA
	&& kind != SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA)
	return SECFailure;		

    return sec_pkcs7_add_cert_chain (cinfo, cert, certdb);
}








SECStatus
SEC_PKCS7AddCertificate (SEC_PKCS7ContentInfo *cinfo, CERTCertificate *cert)
{
    SECOidTag kind;

    kind = SEC_PKCS7ContentType (cinfo);
    if (kind != SEC_OID_PKCS7_SIGNED_DATA
	&& kind != SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA)
	return SECFailure;		

    return sec_pkcs7_add_certificate (cinfo, cert);
}


static SECStatus
sec_pkcs7_init_encrypted_content_info (SEC_PKCS7EncryptedContentInfo *enccinfo,
				       PRArenaPool *poolp,
				       SECOidTag kind, PRBool detached,
				       SECOidTag encalg, int keysize)
{
    SECStatus rv;

    PORT_Assert (enccinfo != NULL && poolp != NULL);
    if (enccinfo == NULL || poolp == NULL)
	return SECFailure;

    






    PORT_Assert (kind == SEC_OID_PKCS7_DATA);

    enccinfo->contentTypeTag = SECOID_FindOIDByTag (kind);
    PORT_Assert (enccinfo->contentTypeTag
	       && enccinfo->contentTypeTag->offset == kind);

    rv = SECITEM_CopyItem (poolp, &(enccinfo->contentType),
			   &(enccinfo->contentTypeTag->oid));
    if (rv != SECSuccess)
	return rv;

    
    enccinfo->keysize = keysize;
    enccinfo->encalg = encalg;

    return SECSuccess;
}






static SECStatus
sec_pkcs7_add_recipient (SEC_PKCS7ContentInfo *cinfo,
			 CERTCertificate *cert,
			 SECCertUsage certusage,
			 CERTCertDBHandle *certdb)
{
    SECOidTag kind;
    SEC_PKCS7RecipientInfo *recipientinfo, **recipientinfos, ***recipientinfosp;
    SECItem *dummy;
    void *mark;
    int count;

    kind = SEC_PKCS7ContentType (cinfo);
    switch (kind) {
      case SEC_OID_PKCS7_ENVELOPED_DATA:
	{
	    SEC_PKCS7EnvelopedData *edp;

	    edp = cinfo->content.envelopedData;
	    recipientinfosp = &(edp->recipientInfos);
	}
	break;
      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:
	{
	    SEC_PKCS7SignedAndEnvelopedData *saedp;

	    saedp = cinfo->content.signedAndEnvelopedData;
	    recipientinfosp = &(saedp->recipientInfos);
	}
	break;
      default:
	return SECFailure;		
    }

    



    if (certdb == NULL) {
	certdb = CERT_GetDefaultCertDB();
	if (certdb == NULL)
	    return SECFailure;		
    }

    if (CERT_VerifyCert (certdb, cert, PR_TRUE, certusage, PR_Now(),
			 cinfo->pwfn_arg, NULL) != SECSuccess)
	{
	
	return SECFailure;
    }

    mark = PORT_ArenaMark (cinfo->poolp);

    recipientinfo = (SEC_PKCS7RecipientInfo*)PORT_ArenaZAlloc (cinfo->poolp,
				      sizeof(SEC_PKCS7RecipientInfo));
    if (recipientinfo == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    dummy = SEC_ASN1EncodeInteger (cinfo->poolp, &recipientinfo->version,
				   SEC_PKCS7_RECIPIENT_INFO_VERSION);
    if (dummy == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }
    PORT_Assert (dummy == &recipientinfo->version);

    recipientinfo->cert = CERT_DupCertificate (cert);
    if (recipientinfo->cert == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    recipientinfo->issuerAndSN = CERT_GetCertIssuerAndSN (cinfo->poolp, cert);
    if (recipientinfo->issuerAndSN == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    






    recipientinfos = *recipientinfosp;
    if (recipientinfos == NULL) {
	count = 0;
	recipientinfos = (SEC_PKCS7RecipientInfo **)PORT_ArenaAlloc (
					  cinfo->poolp,
					  2 * sizeof(SEC_PKCS7RecipientInfo *));
    } else {
	for (count = 0; recipientinfos[count] != NULL; count++)
	    ;
	PORT_Assert (count);	
	recipientinfos = (SEC_PKCS7RecipientInfo **)PORT_ArenaGrow (
				 cinfo->poolp, recipientinfos,
				(count + 1) * sizeof(SEC_PKCS7RecipientInfo *),
				(count + 2) * sizeof(SEC_PKCS7RecipientInfo *));
    }

    if (recipientinfos == NULL) {
	PORT_ArenaRelease (cinfo->poolp, mark);
	return SECFailure;
    }

    recipientinfos[count] = recipientinfo;
    recipientinfos[count + 1] = NULL;

    *recipientinfosp = recipientinfos;

    PORT_ArenaUnmark (cinfo->poolp, mark);
    return SECSuccess;
}




























extern SEC_PKCS7ContentInfo *
SEC_PKCS7CreateEnvelopedData (CERTCertificate *cert,
			      SECCertUsage certusage,
			      CERTCertDBHandle *certdb,
			      SECOidTag encalg,
			      int keysize,
 			      SECKEYGetPasswordKey pwfn, void *pwfn_arg)
{
    SEC_PKCS7ContentInfo *cinfo;
    SEC_PKCS7EnvelopedData *envd;
    SECStatus rv;

    cinfo = sec_pkcs7_create_content_info (SEC_OID_PKCS7_ENVELOPED_DATA,
					   PR_FALSE, pwfn, pwfn_arg);
    if (cinfo == NULL)
	return NULL;

    rv = sec_pkcs7_add_recipient (cinfo, cert, certusage, certdb);
    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return NULL;
    }

    envd = cinfo->content.envelopedData;
    PORT_Assert (envd != NULL);

    



    rv = sec_pkcs7_init_encrypted_content_info (&(envd->encContentInfo),
						cinfo->poolp,
						SEC_OID_PKCS7_DATA, PR_FALSE,
						encalg, keysize);
    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return NULL;
    }

    

    return cinfo;
}



















SECStatus
SEC_PKCS7AddRecipient (SEC_PKCS7ContentInfo *cinfo,
		       CERTCertificate *cert,
		       SECCertUsage certusage,
		       CERTCertDBHandle *certdb)
{
    return sec_pkcs7_add_recipient (cinfo, cert, certusage, certdb);
}








SEC_PKCS7ContentInfo *
SEC_PKCS7CreateData (void)
{
    return sec_pkcs7_create_content_info (SEC_OID_PKCS7_DATA, PR_FALSE,
					  NULL, NULL);
}










SEC_PKCS7ContentInfo *
SEC_PKCS7CreateEncryptedData (SECOidTag algorithm, int keysize,
			      SECKEYGetPasswordKey pwfn, void *pwfn_arg)
{
    SEC_PKCS7ContentInfo *cinfo;
    SECAlgorithmID *algid;
    SEC_PKCS7EncryptedData *enc_data;
    SECStatus rv;

    cinfo = sec_pkcs7_create_content_info (SEC_OID_PKCS7_ENCRYPTED_DATA, 
					   PR_FALSE, pwfn, pwfn_arg);
    if (cinfo == NULL)
	return NULL;

    enc_data = cinfo->content.encryptedData;
    algid = &(enc_data->encContentInfo.contentEncAlg);

    if (!SEC_PKCS5IsAlgorithmPBEAlgTag(algorithm)) {
	rv = SECOID_SetAlgorithmID (cinfo->poolp, algid, algorithm, NULL);
    } else {
        






	SECAlgorithmID *pbe_algid;
	pbe_algid = PK11_CreatePBEAlgorithmID(algorithm,
                                              NSS_PBE_DEFAULT_ITERATION_COUNT,
                                              NULL);
	if (pbe_algid == NULL) {
	    rv = SECFailure;
	} else {
	    rv = SECOID_CopyAlgorithmID (cinfo->poolp, algid, pbe_algid);
	    SECOID_DestroyAlgorithmID (pbe_algid, PR_TRUE);
	}
    }

    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return NULL;
    }

    rv = sec_pkcs7_init_encrypted_content_info (&(enc_data->encContentInfo),
						cinfo->poolp,
						SEC_OID_PKCS7_DATA, PR_FALSE,
						algorithm, keysize);
    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return NULL;
    }

    return cinfo;
}

