









































#include "cmslocal.h"

#include "cert.h"

#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "secerr.h"

NSSCMSSignedData *
NSS_CMSSignedData_Create(NSSCMSMessage *cmsg)
{
    void *mark;
    NSSCMSSignedData *sigd;
    PLArenaPool *poolp;

    if (!cmsg) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }

    poolp = cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    sigd = (NSSCMSSignedData *)PORT_ArenaZAlloc (poolp, sizeof(NSSCMSSignedData));
    if (sigd == NULL)
	goto loser;

    sigd->cmsg = cmsg;

    
    

    PORT_ArenaUnmark(poolp, mark);
    return sigd;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}

void
NSS_CMSSignedData_Destroy(NSSCMSSignedData *sigd)
{
    CERTCertificate **certs, **tempCerts, *cert;
    CERTCertificateList **certlists, *certlist;
    NSSCMSSignerInfo **signerinfos, *si;

    if (sigd == NULL)
	return;

    certs = sigd->certs;
    tempCerts = sigd->tempCerts;
    certlists = sigd->certLists;
    signerinfos = sigd->signerInfos;

    if (certs != NULL) {
	while ((cert = *certs++) != NULL)
	    CERT_DestroyCertificate (cert);
    }

    if (tempCerts != NULL) {
	while ((cert = *tempCerts++) != NULL)
	    CERT_DestroyCertificate (cert);
    }

    if (certlists != NULL) {
	while ((certlist = *certlists++) != NULL)
	    CERT_DestroyCertificateList (certlist);
    }

    if (signerinfos != NULL) {
	while ((si = *signerinfos++) != NULL)
	    NSS_CMSSignerInfo_Destroy(si);
    }

    
   NSS_CMSContentInfo_Destroy(&(sigd->contentInfo));

}












SECStatus
NSS_CMSSignedData_Encode_BeforeStart(NSSCMSSignedData *sigd)
{
    NSSCMSSignerInfo *signerinfo;
    SECOidTag digestalgtag;
    SECItem *dummy;
    int version;
    SECStatus rv;
    PRBool haveDigests = PR_FALSE;
    int n, i;
    PLArenaPool *poolp;

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    poolp = sigd->cmsg->poolp;

    
    
    if (sigd->digestAlgorithms != NULL && sigd->digests != NULL) {
	for (i=0; sigd->digestAlgorithms[i] != NULL; i++) {
	    if (sigd->digests[i] == NULL)
		break;
	}
	if (sigd->digestAlgorithms[i] == NULL)	
	    haveDigests = PR_TRUE;		
    }
	    
    version = NSS_CMS_SIGNED_DATA_VERSION_BASIC;

    
    if (NSS_CMSContentInfo_GetContentTypeTag(&(sigd->contentInfo)) != SEC_OID_PKCS7_DATA)
	version = NSS_CMS_SIGNED_DATA_VERSION_EXT;

    
    for (i=0; i < NSS_CMSSignedData_SignerInfoCount(sigd); i++) {
	signerinfo = NSS_CMSSignedData_GetSignerInfo(sigd, i);

	
	if (NSS_CMSSignerInfo_GetVersion(signerinfo) != NSS_CMS_SIGNER_INFO_VERSION_ISSUERSN)
	    version = NSS_CMS_SIGNED_DATA_VERSION_EXT;
	
	
	
	
	digestalgtag = NSS_CMSSignerInfo_GetDigestAlgTag(signerinfo);
	n = NSS_CMSAlgArray_GetIndexByAlgTag(sigd->digestAlgorithms, digestalgtag);
	if (n < 0 && haveDigests) {
	    
	    
	    goto loser;
	} else if (n < 0) {
	    
	    rv = NSS_CMSSignedData_AddDigest(poolp, sigd, digestalgtag, NULL);
	    if (rv != SECSuccess)
		goto loser;
	} else {
	    
	}
    }

    dummy = SEC_ASN1EncodeInteger(poolp, &(sigd->version), (long)version);
    if (dummy == NULL)
	return SECFailure;

    
    rv = NSS_CMSArray_SortByDER((void **)sigd->digestAlgorithms, 
                                SEC_ASN1_GET(SECOID_AlgorithmIDTemplate),
				(void **)sigd->digests);
    if (rv != SECSuccess)
	return SECFailure;
    
    return SECSuccess;

loser:
    return SECFailure;
}

SECStatus
NSS_CMSSignedData_Encode_BeforeData(NSSCMSSignedData *sigd)
{
    SECStatus rv;
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    rv = NSS_CMSContentInfo_Private_Init(&sigd->contentInfo);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    
    if (sigd->digests && sigd->digests[0]) {
	sigd->contentInfo.privateInfo->digcx = NULL; 
    } else if (sigd->digestAlgorithms != NULL) {
	sigd->contentInfo.privateInfo->digcx =
	        NSS_CMSDigestContext_StartMultiple(sigd->digestAlgorithms);
	if (sigd->contentInfo.privateInfo->digcx == NULL)
	    return SECFailure;
    }
    return SECSuccess;
}











SECStatus
NSS_CMSSignedData_Encode_AfterData(NSSCMSSignedData *sigd)
{
    NSSCMSSignerInfo **signerinfos, *signerinfo;
    NSSCMSContentInfo *cinfo;
    SECOidTag digestalgtag;
    SECStatus ret = SECFailure;
    SECStatus rv;
    SECItem *contentType;
    int certcount;
    int i, ci, cli, n, rci, si;
    PLArenaPool *poolp;
    CERTCertificateList *certlist;
    extern const SEC_ASN1Template NSSCMSSignerInfoTemplate[];

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    poolp = sigd->cmsg->poolp;
    cinfo = &(sigd->contentInfo);

    
    if (cinfo->privateInfo && cinfo->privateInfo->digcx) {
	rv = NSS_CMSDigestContext_FinishMultiple(cinfo->privateInfo->digcx, poolp,
	                                         &(sigd->digests));
	
	cinfo->privateInfo->digcx = NULL;
	if (rv != SECSuccess)
	    goto loser;		
    }

    signerinfos = sigd->signerInfos;
    certcount = 0;

    
    for (i=0; i < NSS_CMSSignedData_SignerInfoCount(sigd); i++) {
	signerinfo = NSS_CMSSignedData_GetSignerInfo(sigd, i);

	
	digestalgtag = NSS_CMSSignerInfo_GetDigestAlgTag(signerinfo);
	n = NSS_CMSAlgArray_GetIndexByAlgTag(sigd->digestAlgorithms, digestalgtag);
	if (n < 0 || sigd->digests == NULL || sigd->digests[n] == NULL) {
	    
	    PORT_SetError(SEC_ERROR_DIGEST_NOT_FOUND);
	    goto loser;
	}

	



	
	if ((contentType = NSS_CMSContentInfo_GetContentTypeOID(cinfo)) == NULL)
	    goto loser;

	
	rv = NSS_CMSSignerInfo_Sign(signerinfo, sigd->digests[n], contentType);
	if (rv != SECSuccess)
	    goto loser;

	
	certlist = NSS_CMSSignerInfo_GetCertList(signerinfo);
	if (certlist)
	    certcount += certlist->len;
    }

    
    rv = NSS_CMSArray_SortByDER((void **)signerinfos, NSSCMSSignerInfoTemplate, NULL);
    if (rv != SECSuccess)
	goto loser;

    



    
    if (sigd->certs != NULL) {
	for (ci = 0; sigd->certs[ci] != NULL; ci++)
	    certcount++;
    }

    if (sigd->certLists != NULL) {
	for (cli = 0; sigd->certLists[cli] != NULL; cli++)
	    certcount += sigd->certLists[cli]->len;
    }

    if (certcount == 0) {
	sigd->rawCerts = NULL;
    } else {
	








	sigd->rawCerts = (SECItem **)PORT_ArenaAlloc(poolp, (certcount + 1) * sizeof(SECItem *));
	if (sigd->rawCerts == NULL)
	    return SECFailure;

	








	rci = 0;
	if (signerinfos != NULL) {
	    for (si = 0; signerinfos[si] != NULL; si++) {
		signerinfo = signerinfos[si];
		for (ci = 0; ci < signerinfo->certList->len; ci++)
		    sigd->rawCerts[rci++] = &(signerinfo->certList->certs[ci]);
	    }
	}

	if (sigd->certs != NULL) {
	    for (ci = 0; sigd->certs[ci] != NULL; ci++)
		sigd->rawCerts[rci++] = &(sigd->certs[ci]->derCert);
	}

	if (sigd->certLists != NULL) {
	    for (cli = 0; sigd->certLists[cli] != NULL; cli++) {
		for (ci = 0; ci < sigd->certLists[cli]->len; ci++)
		    sigd->rawCerts[rci++] = &(sigd->certLists[cli]->certs[ci]);
	    }
	}

	sigd->rawCerts[rci] = NULL;

	
	NSS_CMSArray_Sort((void **)sigd->rawCerts, NSS_CMSUtil_DERCompare, NULL, NULL);
    }

    ret = SECSuccess;

loser:
    return ret;
}

SECStatus
NSS_CMSSignedData_Decode_BeforeData(NSSCMSSignedData *sigd)
{
    SECStatus rv;
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    rv = NSS_CMSContentInfo_Private_Init(&sigd->contentInfo);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    
    if (sigd->digestAlgorithms != NULL && sigd->digests == NULL) {
	
	sigd->contentInfo.privateInfo->digcx = NSS_CMSDigestContext_StartMultiple(sigd->digestAlgorithms);
	if (sigd->contentInfo.privateInfo->digcx == NULL)
	    return SECFailure;
    }
    return SECSuccess;
}





SECStatus
NSS_CMSSignedData_Decode_AfterData(NSSCMSSignedData *sigd)
{
    SECStatus rv = SECSuccess;

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    
    if (sigd->contentInfo.privateInfo && sigd->contentInfo.privateInfo->digcx) {
	rv = NSS_CMSDigestContext_FinishMultiple(sigd->contentInfo.privateInfo->digcx,
				       sigd->cmsg->poolp, &(sigd->digests));
	
	sigd->contentInfo.privateInfo->digcx = NULL;
    }
    return rv;
}





SECStatus
NSS_CMSSignedData_Decode_AfterEnd(NSSCMSSignedData *sigd)
{
    NSSCMSSignerInfo **signerinfos = NULL;
    int i;

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    
    signerinfos = sigd->signerInfos;

    
    if (signerinfos) {
	for (i = 0; signerinfos[i] != NULL; i++)
	    signerinfos[i]->cmsg = sigd->cmsg;
    }

    return SECSuccess;
}




NSSCMSSignerInfo **
NSS_CMSSignedData_GetSignerInfos(NSSCMSSignedData *sigd)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }
    return sigd->signerInfos;
}

int
NSS_CMSSignedData_SignerInfoCount(NSSCMSSignedData *sigd)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return 0;
    }
    return NSS_CMSArray_Count((void **)sigd->signerInfos);
}

NSSCMSSignerInfo *
NSS_CMSSignedData_GetSignerInfo(NSSCMSSignedData *sigd, int i)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }
    return sigd->signerInfos[i];
}




SECAlgorithmID **
NSS_CMSSignedData_GetDigestAlgs(NSSCMSSignedData *sigd)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }
    return sigd->digestAlgorithms;
}




NSSCMSContentInfo *
NSS_CMSSignedData_GetContentInfo(NSSCMSSignedData *sigd)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }
    return &(sigd->contentInfo);
}




SECItem **
NSS_CMSSignedData_GetCertificateList(NSSCMSSignedData *sigd)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }
    return sigd->rawCerts;
}

SECStatus
NSS_CMSSignedData_ImportCerts(NSSCMSSignedData *sigd, CERTCertDBHandle *certdb,
				SECCertUsage certusage, PRBool keepcerts)
{
    int certcount;
    CERTCertificate **certArray = NULL;
    CERTCertList *certList = NULL;
    CERTCertListNode *node;
    SECStatus rv;
    SECItem **rawArray;
    int i;
    PRTime now;

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    certcount = NSS_CMSArray_Count((void **)sigd->rawCerts);

    
    rv = CERT_ImportCerts(certdb, certusage, certcount, sigd->rawCerts, 
			 &certArray, PR_FALSE, PR_FALSE, NULL);
    if (rv != SECSuccess) {
	goto loser;
    }

    
    for (i=0; i < certcount; i++) {
	CERTCertificate *cert = certArray[i];
	if (cert)
            NSS_CMSSignedData_AddTempCertificate(sigd, cert);
    }

    if (!keepcerts) {
	goto done;
    }

    
    certList = CERT_NewCertList();
    if (certList == NULL) {
	rv = SECFailure;
	goto loser;
    }
    for (i=0; i < certcount; i++) {
	CERTCertificate *cert = certArray[i];
	if (cert)
	    cert = CERT_DupCertificate(cert);
	if (cert)
	    CERT_AddCertToListTail(certList,cert);
    }

    
    rv = CERT_FilterCertListByUsage(certList,certusage, PR_FALSE);
    if (rv != SECSuccess) {
	goto loser;
    }

    


    now = PR_Now();
    for (node = CERT_LIST_HEAD(certList) ; !CERT_LIST_END(node,certList);
						node= CERT_LIST_NEXT(node)) {
	CERTCertificateList *certChain;

	if (CERT_VerifyCert(certdb, node->cert, 
		PR_TRUE, certusage, now, NULL, NULL) != SECSuccess) {
	    continue;
	}

	certChain = CERT_CertChainFromCert(node->cert, certusage, PR_FALSE);
	if (!certChain) {
	    continue;
	}

	




	rawArray = (SECItem **)PORT_Alloc(certChain->len*sizeof (SECItem *));
	if (!rawArray) {
	    CERT_DestroyCertificateList(certChain);
	    continue;
	}
	for (i=0; i < certChain->len; i++) {
	    rawArray[i] = &certChain->certs[i];
	}
	(void )CERT_ImportCerts(certdb, certusage, certChain->len, 
			rawArray,  NULL, keepcerts, PR_FALSE, NULL);
	PORT_Free(rawArray);
	CERT_DestroyCertificateList(certChain);
    }

    rv = SECSuccess;

    

done:
    if (sigd->signerInfos != NULL) {
	
	for (i = 0; sigd->signerInfos[i] != NULL; i++)
	    (void)NSS_CMSSignerInfo_GetSigningCertificate(
						sigd->signerInfos[i], certdb);
    }

loser:
    
    if (certArray) {
	CERT_DestroyCertArray(certArray,certcount);
    }
    if (certList) {
	CERT_DestroyCertList(certList);
    }

    return rv;
}















SECStatus
NSS_CMSSignedData_VerifySignerInfo(NSSCMSSignedData *sigd, int i, 
			    CERTCertDBHandle *certdb, SECCertUsage certusage)
{
    NSSCMSSignerInfo *signerinfo;
    NSSCMSContentInfo *cinfo;
    SECOidData *algiddata;
    SECItem *contentType, *digest;
    SECOidTag oidTag;
    SECStatus rv;

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    cinfo = &(sigd->contentInfo);

    signerinfo = sigd->signerInfos[i];

    
    rv = NSS_CMSSignerInfo_VerifyCertificate(signerinfo, certdb, certusage);
    if (rv != SECSuccess)
	return rv; 

    
    algiddata = NSS_CMSSignerInfo_GetDigestAlg(signerinfo);
    oidTag = algiddata ? algiddata->offset : SEC_OID_UNKNOWN;
    digest = NSS_CMSSignedData_GetDigestValue(sigd, oidTag);
    
    contentType = NSS_CMSContentInfo_GetContentTypeOID(cinfo);
    

    
    rv = NSS_CMSSignerInfo_Verify(signerinfo, digest, contentType);
    return rv;
}




SECStatus
NSS_CMSSignedData_VerifyCertsOnly(NSSCMSSignedData *sigd, 
                                  CERTCertDBHandle *certdb, 
                                  SECCertUsage usage)
{
    CERTCertificate *cert;
    SECStatus rv = SECSuccess;
    int i;
    int count;
    PRTime now;

    if (!sigd || !certdb || !sigd->rawCerts) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    count = NSS_CMSArray_Count((void**)sigd->rawCerts);
    now = PR_Now();
    for (i=0; i < count; i++) {
	if (sigd->certs && sigd->certs[i]) {
	    cert = CERT_DupCertificate(sigd->certs[i]);
	} else {
	    cert = CERT_FindCertByDERCert(certdb, sigd->rawCerts[i]);
	    if (!cert) {
		rv = SECFailure;
		break;
	    }
	}
	rv |= CERT_VerifyCert(certdb, cert, PR_TRUE, usage, now, 
                              NULL, NULL);
	CERT_DestroyCertificate(cert);
    }

    return rv;
}




PRBool
NSS_CMSSignedData_HasDigests(NSSCMSSignedData *sigd)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return PR_FALSE;
    }
    return (sigd->digests != NULL);
}

SECStatus
NSS_CMSSignedData_AddCertList(NSSCMSSignedData *sigd, CERTCertificateList *certlist)
{
    SECStatus rv;

    if (!sigd || !certlist) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    
    rv = NSS_CMSArray_Add(sigd->cmsg->poolp, (void ***)&(sigd->certLists), (void *)certlist);

    return rv;
}




SECStatus
NSS_CMSSignedData_AddCertChain(NSSCMSSignedData *sigd, CERTCertificate *cert)
{
    CERTCertificateList *certlist;
    SECCertUsage usage;
    SECStatus rv;

    usage = certUsageEmailSigner;

    if (!sigd || !cert) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    
    certlist = CERT_CertChainFromCert(cert, usage, PR_FALSE);
    if (certlist == NULL)
	return SECFailure;

    rv = NSS_CMSSignedData_AddCertList(sigd, certlist);

    return rv;
}

extern SECStatus
NSS_CMSSignedData_AddTempCertificate(NSSCMSSignedData *sigd, CERTCertificate *cert)
{
    CERTCertificate *c;
    SECStatus rv;

    if (!sigd || !cert) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    c = CERT_DupCertificate(cert);
    rv = NSS_CMSArray_Add(sigd->cmsg->poolp, (void ***)&(sigd->tempCerts), (void *)c);
    return rv;
}

SECStatus
NSS_CMSSignedData_AddCertificate(NSSCMSSignedData *sigd, CERTCertificate *cert)
{
    CERTCertificate *c;
    SECStatus rv;

    if (!sigd || !cert) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    c = CERT_DupCertificate(cert);
    rv = NSS_CMSArray_Add(sigd->cmsg->poolp, (void ***)&(sigd->certs), (void *)c);
    return rv;
}

PRBool
NSS_CMSSignedData_ContainsCertsOrCrls(NSSCMSSignedData *sigd)
{
    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return PR_FALSE;
    }
    if (sigd->rawCerts != NULL && sigd->rawCerts[0] != NULL)
	return PR_TRUE;
    else if (sigd->crls != NULL && sigd->crls[0] != NULL)
	return PR_TRUE;
    else
	return PR_FALSE;
}

SECStatus
NSS_CMSSignedData_AddSignerInfo(NSSCMSSignedData *sigd,
				NSSCMSSignerInfo *signerinfo)
{
    void *mark;
    SECStatus rv;
    SECOidTag digestalgtag;
    PLArenaPool *poolp;

    if (!sigd || !signerinfo) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    poolp = sigd->cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    
    rv = NSS_CMSArray_Add(poolp, (void ***)&(sigd->signerInfos), (void *)signerinfo);
    if (rv != SECSuccess)
	goto loser;

    





    digestalgtag = NSS_CMSSignerInfo_GetDigestAlgTag(signerinfo);
    rv = NSS_CMSSignedData_SetDigestValue(sigd, digestalgtag, NULL);
    if (rv != SECSuccess)
	goto loser;

    



    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease (poolp, mark);
    return SECFailure;
}







SECStatus
NSS_CMSSignedData_SetDigests(NSSCMSSignedData *sigd,
				SECAlgorithmID **digestalgs,
				SECItem **digests)
{
    int cnt, i, idx;

    if (!sigd || !digestalgs || !digests) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (sigd->digestAlgorithms == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    
    PORT_Assert(sigd->digests == NULL);
    if (sigd->digests != NULL) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }

    
    cnt = NSS_CMSArray_Count((void **)sigd->digestAlgorithms);
    sigd->digests = PORT_ArenaZAlloc(sigd->cmsg->poolp, (cnt + 1) * sizeof(SECItem *));
    if (sigd->digests == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }

    for (i = 0; sigd->digestAlgorithms[i] != NULL; i++) {
	
	idx = NSS_CMSAlgArray_GetIndexByAlgID(digestalgs, sigd->digestAlgorithms[i]);
	if (idx < 0) {
	    PORT_SetError(SEC_ERROR_DIGEST_NOT_FOUND);
	    return SECFailure;
	}
	if (!digests[idx]) {
	    



	    continue;
	}

	
	if ((sigd->digests[i] = SECITEM_AllocItem(sigd->cmsg->poolp, NULL, 0)) == NULL ||
	    SECITEM_CopyItem(sigd->cmsg->poolp, sigd->digests[i], digests[idx]) != SECSuccess)
	{
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    return SECFailure;
	}
    }
    return SECSuccess;
}

SECStatus
NSS_CMSSignedData_SetDigestValue(NSSCMSSignedData *sigd,
				SECOidTag digestalgtag,
				SECItem *digestdata)
{
    SECItem *digest = NULL;
    PLArenaPool *poolp;
    void *mark;
    int n, cnt;

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    poolp = sigd->cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

   
    if (digestdata) {
        digest = (SECItem *) PORT_ArenaZAlloc(poolp,sizeof(SECItem));

	
	if (SECITEM_CopyItem(poolp, digest, digestdata) != SECSuccess)
	    goto loser;
    }

    
    if (sigd->digests == NULL) {
        cnt = NSS_CMSArray_Count((void **)sigd->digestAlgorithms);
        sigd->digests = PORT_ArenaZAlloc(sigd->cmsg->poolp, (cnt + 1) * sizeof(SECItem *));
        if (sigd->digests == NULL) {
	        PORT_SetError(SEC_ERROR_NO_MEMORY);
	        return SECFailure;
        }
    }

    n = -1;
    if (sigd->digestAlgorithms != NULL)
	n = NSS_CMSAlgArray_GetIndexByAlgTag(sigd->digestAlgorithms, digestalgtag);

    
    if (n < 0) {
	if (NSS_CMSSignedData_AddDigest(poolp, sigd, digestalgtag, digest) != SECSuccess)
	    goto loser;
    } else {
	
	sigd->digests[n] = digest;
    }

    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(poolp, mark);
    return SECFailure;
}

SECStatus
NSS_CMSSignedData_AddDigest(PRArenaPool *poolp,
				NSSCMSSignedData *sigd,
				SECOidTag digestalgtag,
				SECItem *digest)
{
    SECAlgorithmID *digestalg;
    void *mark;

    if (!sigd || !poolp) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    mark = PORT_ArenaMark(poolp);

    digestalg = PORT_ArenaZAlloc(poolp, sizeof(SECAlgorithmID));
    if (digestalg == NULL)
	goto loser;

    if (SECOID_SetAlgorithmID (poolp, digestalg, digestalgtag, NULL) != SECSuccess) 
	goto loser;

    if (NSS_CMSArray_Add(poolp, (void ***)&(sigd->digestAlgorithms), (void *)digestalg) != SECSuccess ||
	
	NSS_CMSArray_Add(poolp, (void ***)&(sigd->digests), (void *)digest) != SECSuccess)
    {
	goto loser;
    }

    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(poolp, mark);
    return SECFailure;
}


SECItem *
NSS_CMSSignedData_GetDigestValue(NSSCMSSignedData *sigd, SECOidTag digestalgtag)
{
    int n;

    if (!sigd) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }

    if (sigd->digestAlgorithms == NULL || sigd->digests == NULL) {
        PORT_SetError(SEC_ERROR_DIGEST_NOT_FOUND);
	return NULL;
    }

    n = NSS_CMSAlgArray_GetIndexByAlgTag(sigd->digestAlgorithms, digestalgtag);

    return (n < 0) ? NULL : sigd->digests[n];
}

















NSSCMSSignedData *
NSS_CMSSignedData_CreateCertsOnly(NSSCMSMessage *cmsg, CERTCertificate *cert, PRBool include_chain)
{
    NSSCMSSignedData *sigd;
    void *mark;
    PLArenaPool *poolp;
    SECStatus rv;

    if (!cmsg || !cert) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }

    poolp = cmsg->poolp;
    mark = PORT_ArenaMark(poolp);

    sigd = NSS_CMSSignedData_Create(cmsg);
    if (sigd == NULL)
	goto loser;

    

    
    if (include_chain) {
	rv = NSS_CMSSignedData_AddCertChain(sigd, cert);
    } else {
	rv = NSS_CMSSignedData_AddCertificate(sigd, cert);
    }
    if (rv != SECSuccess)
	goto loser;

    






    rv = NSS_CMSContentInfo_SetContent_Data(cmsg, &(sigd->contentInfo), NULL, PR_TRUE);
    if (rv != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return sigd;

loser:
    if (sigd)
	NSS_CMSSignedData_Destroy(sigd);
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}







