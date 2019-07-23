


































#include "nspr.h"
#include "secerr.h"
#include "secport.h"
#include "seccomon.h"
#include "secoid.h"
#include "sslerr.h"
#include "genname.h"
#include "keyhi.h"
#include "cert.h"
#include "certdb.h"
#include "certi.h"
#include "cryptohi.h"
#include "pkix.h"

#include "pkix_pl_cert.h"


#include "nsspki.h"
#include "pkitm.h"
#include "pkim.h"
#include "pki3hack.h"
#include "base.h"




SECStatus
CERT_CertTimesValid(CERTCertificate *c)
{
    SECCertTimeValidity valid = CERT_CheckCertValidTimes(c, PR_Now(), PR_TRUE);
    return (valid == secCertTimeValid) ? SECSuccess : SECFailure;
}




SECStatus
CERT_VerifySignedDataWithPublicKey(CERTSignedData *sd, 
                                   SECKEYPublicKey *pubKey,
		                   void *wincx)
{
    SECStatus        rv;
    SECItem          sig;
    SECOidTag        hashAlg = SEC_OID_UNKNOWN;

    if ( !pubKey || !sd ) {
	PORT_SetError(PR_INVALID_ARGUMENT_ERROR);
	return SECFailure;
    }

    
    sig = sd->signature;
    
    DER_ConvertBitString(&sig);

    rv = VFY_VerifyDataWithAlgorithmID(sd->data.data, sd->data.len, pubKey, 
			&sig, &sd->signatureAlgorithm, &hashAlg, wincx);
    if (rv == SECSuccess) {
        
	PRUint32 policyFlags = 0;
	rv = NSS_GetAlgorithmPolicy(hashAlg, &policyFlags);
	if (rv == SECSuccess && 
	    !(policyFlags & NSS_USE_ALG_IN_CERT_SIGNATURE)) {
	    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	    rv = SECFailure;
	}
    }
    return rv;
}




SECStatus
CERT_VerifySignedDataWithPublicKeyInfo(CERTSignedData *sd, 
                                       CERTSubjectPublicKeyInfo *pubKeyInfo,
		                       void *wincx)
{
    SECKEYPublicKey *pubKey;
    SECStatus        rv		= SECFailure;

    
    pubKey = SECKEY_ExtractPublicKey(pubKeyInfo);
    if (pubKey) {
	rv =  CERT_VerifySignedDataWithPublicKey(sd, pubKey, wincx);
	SECKEY_DestroyPublicKey(pubKey);
    }
    return rv;
}




SECStatus
CERT_VerifySignedData(CERTSignedData *sd, CERTCertificate *cert,
		      int64 t, void *wincx)
{
    SECKEYPublicKey *pubKey = 0;
    SECStatus        rv     = SECFailure;
    SECCertTimeValidity validity;

    
    validity = CERT_CheckCertValidTimes(cert, t, PR_FALSE);
    if ( validity != secCertTimeValid ) {
	return rv;
    }

    
    pubKey = CERT_ExtractPublicKey(cert);
    if (pubKey) {
	rv =  CERT_VerifySignedDataWithPublicKey(sd, pubKey, wincx);
	SECKEY_DestroyPublicKey(pubKey);
    }
    return rv;
}






static int dont_use_krl = 0;

void sec_SetCheckKRLState(int value) { dont_use_krl = value; }

SECStatus
SEC_CheckKRL(CERTCertDBHandle *handle,SECKEYPublicKey *key,
	     CERTCertificate *rootCert, int64 t, void * wincx)
{
    CERTSignedCrl *crl = NULL;
    SECStatus rv = SECFailure;
    SECStatus rv2;
    CERTCrlEntry **crlEntry;
    SECCertTimeValidity validity;
    CERTCertificate *issuerCert = NULL;

    if (dont_use_krl) return SECSuccess;

    
    crl = SEC_FindCrlByName(handle,&rootCert->derSubject, SEC_KRL_TYPE);
    if (crl == NULL) {
	PORT_SetError(SEC_ERROR_NO_KRL);
	goto done;
    }

    
    issuerCert = CERT_FindCertByName(handle, &crl->crl.derName);
    if (issuerCert == NULL) {
        PORT_SetError(SEC_ERROR_KRL_BAD_SIGNATURE);
        goto done;
    }


    
    rv2 = CERT_VerifySignedData(&crl->signatureWrap, issuerCert, t, wincx);
    if (rv2 != SECSuccess) {
	PORT_SetError(SEC_ERROR_KRL_BAD_SIGNATURE);
    	goto done;
    }

    
    validity = SEC_CheckCrlTimes(&crl->crl, t);
    if (validity == secCertTimeExpired) {
	PORT_SetError(SEC_ERROR_KRL_EXPIRED);
	goto done;
    }

    
    if (key->keyType != fortezzaKey) {
	PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
	goto done; 
    }

    
    for (crlEntry = crl->crl.entries; crlEntry && *crlEntry; crlEntry++) {
	if (PORT_Memcmp((*crlEntry)->serialNumber.data,
				key->u.fortezza.KMID,
				    (*crlEntry)->serialNumber.len) == 0) {
	    PORT_SetError(SEC_ERROR_REVOKED_KEY);
	    goto done;
	}
    }
    rv = SECSuccess;

done:
    if (issuerCert) CERT_DestroyCertificate(issuerCert);
    if (crl) SEC_DestroyCrl(crl);
    return rv;
}

SECStatus
SEC_CheckCRL(CERTCertDBHandle *handle,CERTCertificate *cert,
	     CERTCertificate *caCert, int64 t, void * wincx)
{
    return CERT_CheckCRL(cert, caCert, NULL, t, wincx);
}




CERTCertificate *
CERT_FindCertIssuer(CERTCertificate *cert, int64 validTime, SECCertUsage usage)
{
    NSSCertificate *me;
    NSSTime *nssTime;
    NSSTrustDomain *td;
    NSSCryptoContext *cc;
    NSSCertificate *chain[3];
    NSSUsage nssUsage;
    PRStatus status;

    me = STAN_GetNSSCertificate(cert);
    if (!me) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }
    nssTime = NSSTime_SetPRTime(NULL, validTime);
    nssUsage.anyUsage = PR_FALSE;
    nssUsage.nss3usage = usage;
    nssUsage.nss3lookingForCA = PR_TRUE;
    memset(chain, 0, 3*sizeof(NSSCertificate *));
    td   = STAN_GetDefaultTrustDomain();
    cc = STAN_GetDefaultCryptoContext();
    (void)NSSCertificate_BuildChain(me, nssTime, &nssUsage, NULL, 
                                    chain, 2, NULL, &status, td, cc);
    nss_ZFreeIf(nssTime);
    if (status == PR_SUCCESS) {
	PORT_Assert(me == chain[0]);
	
	if (!chain[1]) {
	    
	    return cert;
	} 
	NSSCertificate_Destroy(chain[0]); 
	return STAN_GetCERTCertificate(chain[1]); 
    } 
    if (chain[0]) {
	PORT_Assert(me == chain[0]);
	NSSCertificate_Destroy(chain[0]); 
    }
    PORT_SetError (SEC_ERROR_UNKNOWN_ISSUER);
    return NULL;
}




SECStatus
CERT_TrustFlagsForCACertUsage(SECCertUsage usage,
			      unsigned int *retFlags,
			      SECTrustType *retTrustType)
{
    unsigned int requiredFlags;
    SECTrustType trustType;

    switch ( usage ) {
      case certUsageSSLClient:
	requiredFlags = CERTDB_TRUSTED_CLIENT_CA;
	trustType = trustSSL;
        break;
      case certUsageSSLServer:
      case certUsageSSLCA:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = trustSSL;
        break;
      case certUsageSSLServerWithStepUp:
	requiredFlags = CERTDB_TRUSTED_CA | CERTDB_GOVT_APPROVED_CA;
	trustType = trustSSL;
        break;
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = trustEmail;
	break;
      case certUsageObjectSigner:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = trustObjectSigning;
	break;
      case certUsageVerifyCA:
      case certUsageAnyCA:
      case certUsageStatusResponder:
	requiredFlags = CERTDB_TRUSTED_CA;
	trustType = trustTypeNone;
	break;
      default:
	PORT_Assert(0);
	goto loser;
    }
    if ( retFlags != NULL ) {
	*retFlags = requiredFlags;
    }
    if ( retTrustType != NULL ) {
	*retTrustType = trustType;
    }
    
    return(SECSuccess);
loser:
    return(SECFailure);
}

void
cert_AddToVerifyLog(CERTVerifyLog *log, CERTCertificate *cert, unsigned long error,
	       unsigned int depth, void *arg)
{
    CERTVerifyLogNode *node, *tnode;

    PORT_Assert(log != NULL);
    
    node = (CERTVerifyLogNode *)PORT_ArenaAlloc(log->arena,
						sizeof(CERTVerifyLogNode));
    if ( node != NULL ) {
	node->cert = CERT_DupCertificate(cert);
	node->error = error;
	node->depth = depth;
	node->arg = arg;
	
	if ( log->tail == NULL ) {
	    
	    log->head = log->tail = node;
	    node->prev = NULL;
	    node->next = NULL;
	} else if ( depth >= log->tail->depth ) {
	    
	    node->prev = log->tail;
	    log->tail->next = node;
	    log->tail = node;
	    node->next = NULL;
	} else if ( depth < log->head->depth ) {
	    
	    node->prev = NULL;
	    node->next = log->head;
	    log->head->prev = node;
	    log->head = node;
	} else {
	    
	    tnode = log->tail;
	    while ( tnode != NULL ) {
		if ( depth >= tnode->depth ) {
		    
		    node->prev = tnode;
		    node->next = tnode->next;
		    tnode->next->prev = node;
		    tnode->next = node;
		    break;
		}

		tnode = tnode->prev;
	    }
	}

	log->count++;
    }
    return;
}

#define EXIT_IF_NOT_LOGGING(log) \
    if ( log == NULL ) { \
	goto loser; \
    }

#define LOG_ERROR_OR_EXIT(log,cert,depth,arg) \
    if ( log != NULL ) { \
	cert_AddToVerifyLog(log, cert, PORT_GetError(), depth, (void *)arg); \
    } else { \
	goto loser; \
    }

#define LOG_ERROR(log,cert,depth,arg) \
    if ( log != NULL ) { \
	cert_AddToVerifyLog(log, cert, PORT_GetError(), depth, (void *)arg); \
    }


typedef enum { cbd_None, cbd_User, cbd_CA } cbd_FortezzaType;

static SECStatus
cert_VerifyFortezzaV1Cert(CERTCertDBHandle *handle, CERTCertificate *cert,
	cbd_FortezzaType *next_type, cbd_FortezzaType last_type,
	int64 t, void *wincx)
{
    unsigned char priv = 0;
    SECKEYPublicKey *key;
    SECStatus rv;

    *next_type = cbd_CA;

    
    key = CERT_ExtractPublicKey(cert);

    
    if (key == NULL) {
    	PORT_SetError(SEC_ERROR_BAD_KEY);
	return SECFailure;
    }


    
    if (key->keyType != fortezzaKey) {
    	SECKEY_DestroyPublicKey(key);
	
    	PORT_SetError(SEC_ERROR_NOT_FORTEZZA_ISSUER);
	return SECFailure;
    }

    
    if (key->u.fortezza.DSSpriviledge.len > 0) {
	priv = key->u.fortezza.DSSpriviledge.data[0];
    }

    


            
    rv = SEC_CheckKRL(handle, key, NULL, t, wincx);
    SECKEY_DestroyPublicKey(key);
    if (rv != SECSuccess) {
	return rv;
    }

    switch (last_type) {
      case cbd_User:
	
	
	rv = SECSuccess;

	
	if ((rv != SECSuccess) || ((priv & 0x10) == 0)) {
	    
	    PORT_SetError (SEC_ERROR_CA_CERT_INVALID);
	    return SECFailure;
	}
	break;
      case cbd_CA:
	if ((priv & 0x20) == 0) {
	    
	    PORT_SetError (SEC_ERROR_CA_CERT_INVALID);
	    return SECFailure;
	}
	break;
      case cbd_None:
	*next_type = (priv & 0x30) ? cbd_CA : cbd_User;
	break;
      default:
	 
    	PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
	return SECFailure;
    }
    return SECSuccess;
}


static SECStatus
cert_VerifyCertChainOld(CERTCertDBHandle *handle, CERTCertificate *cert,
		     PRBool checkSig, PRBool* sigerror,
                     SECCertUsage certUsage, int64 t, void *wincx,
                     CERTVerifyLog *log, PRBool* revoked)
{
    SECTrustType trustType;
    CERTBasicConstraints basicConstraint;
    CERTCertificate *issuerCert = NULL;
    CERTCertificate *subjectCert = NULL;
    CERTCertificate *badCert = NULL;
    PRBool isca;
    PRBool isFortezzaV1 = PR_FALSE;
    SECStatus rv;
    SECStatus rvFinal = SECSuccess;
    int count;
    int currentPathLen = 0;
    int pathLengthLimit = CERT_UNLIMITED_PATH_CONSTRAINT;
    unsigned int caCertType;
    unsigned int requiredCAKeyUsage;
    unsigned int requiredFlags;
    PRArenaPool *arena = NULL;
    CERTGeneralName *namesList = NULL;
    CERTCertificate **certsList      = NULL;
    int certsListLen = 16;
    int namesCount = 0;
    PRBool subjectCertIsSelfIssued;

    cbd_FortezzaType last_type = cbd_None;

    if (revoked) {
        *revoked = PR_FALSE;
    }

    if (CERT_KeyUsageAndTypeForCertUsage(certUsage, PR_TRUE,
					 &requiredCAKeyUsage,
					 &caCertType)
	!= SECSuccess ) {
	PORT_Assert(0);
	EXIT_IF_NOT_LOGGING(log);
	requiredCAKeyUsage = 0;
	caCertType = 0;
    }

    switch ( certUsage ) {
      case certUsageSSLClient:
      case certUsageSSLServer:
      case certUsageSSLCA:
      case certUsageSSLServerWithStepUp:
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
      case certUsageObjectSigner:
      case certUsageVerifyCA:
      case certUsageAnyCA:
      case certUsageStatusResponder:
	if ( CERT_TrustFlagsForCACertUsage(certUsage, &requiredFlags,
					   &trustType) != SECSuccess ) {
	    PORT_Assert(0);
	    EXIT_IF_NOT_LOGGING(log);
	    requiredFlags = 0;
	    trustType = trustSSL;
	}
	break;
      default:
	PORT_Assert(0);
	EXIT_IF_NOT_LOGGING(log);
	requiredFlags = 0;
	trustType = trustSSL;


	caCertType = 0;
    }
    
    subjectCert = CERT_DupCertificate(cert);
    if ( subjectCert == NULL ) {
	goto loser;
    }

    

    isFortezzaV1 = (PRBool)
	(CERT_GetCertKeyType(&subjectCert->subjectPublicKeyInfo) 
							== fortezzaKey);

    if (isFortezzaV1) {
	rv = cert_VerifyFortezzaV1Cert(handle, subjectCert, &last_type, 
						cbd_None, t, wincx);
	if (rv == SECFailure) {
	    
	    LOG_ERROR_OR_EXIT(log,subjectCert,0,0);
	}
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto loser;
    }

    certsList = PORT_ZNewArray(CERTCertificate *, certsListLen);
    if (certsList == NULL)
	goto loser;

    



    subjectCertIsSelfIssued = PR_FALSE;
    for ( count = 0; count < CERT_MAX_CERT_CHAIN; count++ ) {
	PRBool validCAOverride = PR_FALSE;

	




	if (subjectCertIsSelfIssued == PR_FALSE) {
	    CERTGeneralName *subjectNameList;
	    int subjectNameListLen;
	    int i;
	    subjectNameList    = CERT_GetCertificateNames(subjectCert, arena);
	    if (!subjectNameList)
		goto loser;
	    subjectNameListLen = CERT_GetNamesLength(subjectNameList);
	    if (!subjectNameListLen)
		goto loser;
	    if (certsListLen <= namesCount + subjectNameListLen) {
		CERTCertificate **tmpCertsList;
		certsListLen = (namesCount + subjectNameListLen) * 2;
		tmpCertsList = 
		    (CERTCertificate **)PORT_Realloc(certsList, 
	                            certsListLen * sizeof(CERTCertificate *));
		if (tmpCertsList == NULL) {
		    goto loser;
		}
		certsList = tmpCertsList;
	    }
	    for (i = 0; i < subjectNameListLen; i++) {
		certsList[namesCount + i] = subjectCert;
	    }
	    namesCount += subjectNameListLen;
	    namesList = cert_CombineNamesLists(namesList, subjectNameList);
	}

        
	if ( subjectCert->options.bits.hasUnsupportedCriticalExt ) {
	    PORT_SetError(SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION);
	    LOG_ERROR_OR_EXIT(log,subjectCert,count,0);
	}

	
	issuerCert = CERT_FindCertIssuer(subjectCert, t, certUsage);
	if ( ! issuerCert ) {
	    PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
	    LOG_ERROR(log,subjectCert,count,0);
	    goto loser;
	}

	
	if ( checkSig ) {
	    rv = CERT_VerifySignedData(&subjectCert->signatureWrap,
				       issuerCert, t, wincx);
    
	    if ( rv != SECSuccess ) {
                if (sigerror) {
                    *sigerror = PR_TRUE;
                }
		if ( PORT_GetError() == SEC_ERROR_EXPIRED_CERTIFICATE ) {
		    PORT_SetError(SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE);
		    LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
		} else {
		    PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		    LOG_ERROR_OR_EXIT(log,subjectCert,count,0);
		}
	    }
	}

	


	if (isFortezzaV1) {
	    rv = cert_VerifyFortezzaV1Cert(handle, issuerCert, &last_type, 
					last_type, t, wincx);
	    if (rv == SECFailure) {
		

		LOG_ERROR_OR_EXIT(log,subjectCert,0,0);
	    }
	}

	














	rv = CERT_FindBasicConstraintExten(issuerCert, &basicConstraint);
	if ( rv != SECSuccess ) {
	    if (PORT_GetError() != SEC_ERROR_EXTENSION_NOT_FOUND) {
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
	    } 
	    pathLengthLimit = CERT_UNLIMITED_PATH_CONSTRAINT;
	    


	    isca = isFortezzaV1;
	} else  {
	    if ( basicConstraint.isCA == PR_FALSE ) {
		PORT_SetError (SEC_ERROR_CA_CERT_INVALID);
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
	    }
	    pathLengthLimit = basicConstraint.pathLenConstraint;
	    isca = PR_TRUE;
	}    
	
	if (pathLengthLimit >= 0 && currentPathLen > pathLengthLimit) {
	    PORT_SetError (SEC_ERROR_PATH_LEN_CONSTRAINT_INVALID);
	    LOG_ERROR_OR_EXIT(log, issuerCert, count+1, pathLengthLimit);
	}
	
	


	
        rv = SEC_CheckCRL(handle, subjectCert, issuerCert, t, wincx);
        if (rv == SECFailure) {
            if (revoked) {
                *revoked = PR_TRUE;
            }
            LOG_ERROR_OR_EXIT(log,subjectCert,count,0);
        } else if (rv == SECWouldBlock) {
            



            rvFinal = SECFailure;
            if (revoked) {
                *revoked = PR_TRUE;
            }
            LOG_ERROR(log,subjectCert,count,0);
        }

	if ( issuerCert->trust ) {
	    




	    unsigned int flags;

	    if (certUsage != certUsageAnyCA &&
	        certUsage != certUsageStatusResponder) {

	        


	        if ( certUsage == certUsageVerifyCA ) {
	            if ( subjectCert->nsCertType & NS_CERT_TYPE_EMAIL_CA ) {
	                trustType = trustEmail;
	            } else if ( subjectCert->nsCertType & NS_CERT_TYPE_SSL_CA ) {
	                trustType = trustSSL;
	            } else {
	                trustType = trustObjectSigning;
	            }
	        }

	        flags = SEC_GET_TRUST_FLAGS(issuerCert->trust, trustType);
	        if (( flags & requiredFlags ) == requiredFlags) {
	            
	            rv = rvFinal; 
	            goto done;
	        }
	        if (flags & CERTDB_VALID_CA) {
	            validCAOverride = PR_TRUE;
	        }
	    } else {
                

                for (trustType = trustSSL; trustType < trustTypeNone;
                     trustType++) {
                    flags = SEC_GET_TRUST_FLAGS(issuerCert->trust, trustType);
                    if ((flags & requiredFlags) == requiredFlags) {
	                rv = rvFinal; 
	                goto done;
                    }
                    if (flags & CERTDB_VALID_CA)
                        validCAOverride = PR_TRUE;
                }
            }
        }

	if (!validCAOverride) {
	    



	    




	    if (!isca || (issuerCert->nsCertType & NS_CERT_TYPE_CA)) {
		isca = (issuerCert->nsCertType & caCertType) ? PR_TRUE : PR_FALSE;
	    }
	
	    if (  !isca  ) {
		PORT_SetError(SEC_ERROR_CA_CERT_INVALID);
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,0);
	    }

	    
	    if (CERT_CheckKeyUsage(issuerCert, requiredCAKeyUsage) != SECSuccess) {
		PORT_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE);
		LOG_ERROR_OR_EXIT(log,issuerCert,count+1,requiredCAKeyUsage);
	    }
	}

	


	rv = CERT_CompareNameSpace(issuerCert, namesList, certsList, 
	                           arena, &badCert);
	if (rv != SECSuccess || badCert != NULL) {
	    PORT_SetError(SEC_ERROR_CERT_NOT_IN_NAME_SPACE);
            LOG_ERROR_OR_EXIT(log, badCert, count + 1, 0);
	    goto loser;
	}
	


	if (issuerCert->isRoot) {
	    PORT_SetError(SEC_ERROR_UNTRUSTED_ISSUER);
	    LOG_ERROR(log, issuerCert, count+1, 0);
	    goto loser;
	} 
	



	subjectCertIsSelfIssued = (PRBool)
	    SECITEM_ItemsAreEqual(&issuerCert->derIssuer, 
				  &issuerCert->derSubject) &&
	    issuerCert->derSubject.len > 0;
	if (subjectCertIsSelfIssued == PR_FALSE) {
	    


	    ++currentPathLen;
	}

	CERT_DestroyCertificate(subjectCert);
	subjectCert = issuerCert;
	issuerCert = NULL;
    }

    PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
    LOG_ERROR(log,subjectCert,count,0);
loser:
    rv = SECFailure;
done:
    if (certsList != NULL) {
	PORT_Free(certsList);
    }
    if ( issuerCert ) {
	CERT_DestroyCertificate(issuerCert);
    }
    
    if ( subjectCert ) {
	CERT_DestroyCertificate(subjectCert);
    }

    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return rv;
}

SECStatus
cert_VerifyCertChain(CERTCertDBHandle *handle, CERTCertificate *cert,
                     PRBool checkSig, PRBool* sigerror,
                     SECCertUsage certUsage, int64 t, void *wincx,
                     CERTVerifyLog *log, PRBool* revoked)
{
    if (CERT_GetUsePKIXForValidation()) {
        return cert_VerifyCertChainPkix(cert, checkSig, certUsage, t,
                                        wincx, log, sigerror, revoked);
    }
    return cert_VerifyCertChainOld(handle, cert, checkSig, sigerror,
                                   certUsage, t, wincx, log, revoked);
}

SECStatus
CERT_VerifyCertChain(CERTCertDBHandle *handle, CERTCertificate *cert,
		     PRBool checkSig, SECCertUsage certUsage, int64 t,
		     void *wincx, CERTVerifyLog *log)
{
    return cert_VerifyCertChain(handle, cert, checkSig, NULL, certUsage, t,
			 wincx, log, NULL);
}




SECStatus
CERT_VerifyCACertForUsage(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertUsage certUsage, int64 t,
		void *wincx, CERTVerifyLog *log)
{
    SECTrustType trustType;
    CERTBasicConstraints basicConstraint;
    PRBool isca;
    PRBool validCAOverride = PR_FALSE;
    SECStatus rv;
    SECStatus rvFinal = SECSuccess;
    int flags;
    unsigned int caCertType;
    unsigned int requiredCAKeyUsage;
    unsigned int requiredFlags;
    CERTCertificate *issuerCert;


    if (CERT_KeyUsageAndTypeForCertUsage(certUsage, PR_TRUE,
					 &requiredCAKeyUsage,
					 &caCertType) != SECSuccess ) {
	PORT_Assert(0);
	EXIT_IF_NOT_LOGGING(log);
	requiredCAKeyUsage = 0;
	caCertType = 0;
    }

    switch ( certUsage ) {
      case certUsageSSLClient:
      case certUsageSSLServer:
      case certUsageSSLCA:
      case certUsageSSLServerWithStepUp:
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
      case certUsageObjectSigner:
      case certUsageVerifyCA:
      case certUsageStatusResponder:
	if ( CERT_TrustFlagsForCACertUsage(certUsage, &requiredFlags,
					   &trustType) != SECSuccess ) {
	    PORT_Assert(0);
	    EXIT_IF_NOT_LOGGING(log);
	    requiredFlags = 0;
	    trustType = trustSSL;
	}
	break;
      default:
	PORT_Assert(0);
	EXIT_IF_NOT_LOGGING(log);
	requiredFlags = 0;
	trustType = trustSSL;


	caCertType = 0;
    }
    
    














    rv = CERT_FindBasicConstraintExten(cert, &basicConstraint);
    if ( rv != SECSuccess ) {
	if (PORT_GetError() != SEC_ERROR_EXTENSION_NOT_FOUND) {
	    LOG_ERROR_OR_EXIT(log,cert,0,0);
	} 
	


	isca = PR_FALSE;
    } else  {
	if ( basicConstraint.isCA == PR_FALSE ) {
	    PORT_SetError (SEC_ERROR_CA_CERT_INVALID);
	    LOG_ERROR_OR_EXIT(log,cert,0,0);
	}

	
	isca = PR_TRUE;
    }
	
    if ( cert->trust ) {
	




        if (certUsage == certUsageStatusResponder) {
	    
            issuerCert = CERT_FindCertIssuer(cert, t, certUsage);
            if (issuerCert) {
                if (SEC_CheckCRL(handle, cert, issuerCert, t, wincx) 
		    != SECSuccess) {
                    PORT_SetError(SEC_ERROR_REVOKED_CERTIFICATE);
                    CERT_DestroyCertificate(issuerCert);
                    goto loser;
                }
                CERT_DestroyCertificate(issuerCert);
            }
	    



	    rv = rvFinal; 
	    goto done;
        }

	


	flags = SEC_GET_TRUST_FLAGS(cert->trust, trustType);
	if ( ( flags & requiredFlags ) == requiredFlags) {
	    
	    rv = rvFinal; 
	    goto done;
	}
	if (flags & CERTDB_VALID_CA) {
	    validCAOverride = PR_TRUE;
	}
    }
    if (!validCAOverride) {
	



	




	if (!isca || (cert->nsCertType & NS_CERT_TYPE_CA)) {
	    isca = (cert->nsCertType & caCertType) ? PR_TRUE : PR_FALSE;
	}
	
	if (!isca) {
	    PORT_SetError(SEC_ERROR_CA_CERT_INVALID);
	    LOG_ERROR_OR_EXIT(log,cert,0,0);
	}
	    
	
	if (CERT_CheckKeyUsage(cert, requiredCAKeyUsage) != SECSuccess) {
	    PORT_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE);
	    LOG_ERROR_OR_EXIT(log,cert,0,requiredCAKeyUsage);
	}
    }
    


    if (cert->isRoot) {
	    PORT_SetError(SEC_ERROR_UNTRUSTED_ISSUER);
	    LOG_ERROR(log, cert, 0, 0);
	    goto loser;
    }

    return CERT_VerifyCertChain(handle, cert, checkSig, certUsage, t, 
		     					wincx, log);
loser:
    rv = SECFailure;
done:
    return rv;
}

#define NEXT_USAGE() { \
    i*=2; \
    certUsage++; \
    continue; \
}

#define VALID_USAGE() { \
    NEXT_USAGE(); \
}

#define INVALID_USAGE() { \
    if (returnedUsages) { \
        *returnedUsages &= (~i); \
    } \
    if (PR_TRUE == requiredUsage) { \
        valid = SECFailure; \
    } \
    NEXT_USAGE(); \
}













SECStatus
CERT_VerifyCertificate(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertificateUsage requiredUsages, int64 t,
		void *wincx, CERTVerifyLog *log, SECCertificateUsage* returnedUsages)
{
    SECStatus rv;
    SECStatus valid;
    unsigned int requiredKeyUsage;
    unsigned int requiredCertType;
    unsigned int flags;
    unsigned int certType;
    PRBool       allowOverride;
    SECCertTimeValidity validity;
    CERTStatusConfig *statusConfig;
    PRInt32 i;
    SECCertUsage certUsage = 0;
    PRBool checkedOCSP = PR_FALSE;
    PRBool checkAllUsages = PR_FALSE;
    PRBool revoked = PR_FALSE;
    PRBool sigerror = PR_FALSE;

    if (!requiredUsages) {
        

        checkAllUsages = PR_TRUE;
    }

    if (returnedUsages) {
        *returnedUsages = 0;
    } else {
        

        checkAllUsages = PR_FALSE;
    }
    valid = SECSuccess ; 
   
    
    allowOverride = (PRBool)((requiredUsages & certificateUsageSSLServer) ||
                             (requiredUsages & certificateUsageSSLServerWithStepUp));
    validity = CERT_CheckCertValidTimes(cert, t, allowOverride);
    if ( validity != secCertTimeValid ) {
        valid = SECFailure;
        LOG_ERROR_OR_EXIT(log,cert,0,validity);
    }

    
    cert_GetCertType(cert);
    certType = cert->nsCertType;

    for (i=1; i<=certificateUsageHighest && 
              (SECSuccess == valid || returnedUsages || log) ; ) {
        PRBool requiredUsage = (i & requiredUsages) ? PR_TRUE : PR_FALSE;
        if (PR_FALSE == requiredUsage && PR_FALSE == checkAllUsages) {
            NEXT_USAGE();
        }
        if (returnedUsages) {
            *returnedUsages |= i; 
        }
        switch ( certUsage ) {
          case certUsageSSLClient:
          case certUsageSSLServer:
          case certUsageSSLServerWithStepUp:
          case certUsageSSLCA:
          case certUsageEmailSigner:
          case certUsageEmailRecipient:
          case certUsageObjectSigner:
          case certUsageStatusResponder:
            rv = CERT_KeyUsageAndTypeForCertUsage(certUsage, PR_FALSE,
                                                  &requiredKeyUsage,
                                                  &requiredCertType);
            if ( rv != SECSuccess ) {
                PORT_Assert(0);
                
                requiredKeyUsage = 0;
                requiredCertType = 0;
                INVALID_USAGE();
            }
            break;

          case certUsageAnyCA:
          case certUsageProtectedObjectSigner:
          case certUsageUserCertImport:
          case certUsageVerifyCA:
              
              NEXT_USAGE();

          default:
            PORT_Assert(0);
            requiredKeyUsage = 0;
            requiredCertType = 0;
            INVALID_USAGE();
        }
        if ( CERT_CheckKeyUsage(cert, requiredKeyUsage) != SECSuccess ) {
            if (PR_TRUE == requiredUsage) {
                PORT_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE);
            }
            LOG_ERROR(log,cert,0,requiredKeyUsage);
            INVALID_USAGE();
        }
        if ( !( certType & requiredCertType ) ) {
            if (PR_TRUE == requiredUsage) {
                PORT_SetError(SEC_ERROR_INADEQUATE_CERT_TYPE);
            }
            LOG_ERROR(log,cert,0,requiredCertType);
            INVALID_USAGE();
        }

        
        if ( cert->trust ) { 
            switch ( certUsage ) {
              case certUsageSSLClient:
              case certUsageSSLServer:
                flags = cert->trust->sslFlags;

                
                if ( flags & CERTDB_VALID_PEER ) {
                    if ( flags & CERTDB_TRUSTED ) {	
                        VALID_USAGE();
                    } else { 
                        if (PR_TRUE == requiredUsage) {
                            PORT_SetError(SEC_ERROR_UNTRUSTED_CERT);
                        }
                        LOG_ERROR(log,cert,0,flags);
                        INVALID_USAGE();
                    }
                }
                break;
              case certUsageSSLServerWithStepUp:
                
                break;
              case certUsageSSLCA:
                break;
              case certUsageEmailSigner:
              case certUsageEmailRecipient:
                flags = cert->trust->emailFlags;

                
                if ( ( flags & ( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) ==
                    ( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) {
                    VALID_USAGE();
                }
                break;
              case certUsageObjectSigner:
                flags = cert->trust->objectSigningFlags;

                
                if ( flags & CERTDB_VALID_PEER ) {
                    if ( flags & CERTDB_TRUSTED ) {	
                        VALID_USAGE();
                    } else { 
                        if (PR_TRUE == requiredUsage) {
                            PORT_SetError(SEC_ERROR_UNTRUSTED_CERT);
                        }
                        LOG_ERROR(log,cert,0,flags);
                        INVALID_USAGE();
                    }
                }
                break;
              case certUsageVerifyCA:
              case certUsageStatusResponder:
                flags = cert->trust->sslFlags;
                
                if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
                    ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
                    VALID_USAGE();
                }
                flags = cert->trust->emailFlags;
                
                if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
                    ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
                    VALID_USAGE();
                }
                flags = cert->trust->objectSigningFlags;
                
                if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
                    ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
                    VALID_USAGE();
                }
                break;
              case certUsageAnyCA:
              case certUsageProtectedObjectSigner:
              case certUsageUserCertImport:
                


                break;
            }
        }

        if (PR_TRUE == revoked || PR_TRUE == sigerror) {
            INVALID_USAGE();
        }

        rv = cert_VerifyCertChain(handle, cert,
            checkSig, &sigerror,
            certUsage, t, wincx, log,
            &revoked);

        if (rv != SECSuccess) {
            
            INVALID_USAGE();
        }

        







        if (PR_FALSE == checkedOCSP) {
            checkedOCSP = PR_TRUE; 
            statusConfig = CERT_GetStatusConfig(handle);
            if (requiredUsages != certificateUsageStatusResponder &&
                statusConfig != NULL) {
                if (statusConfig->statusChecker != NULL) {
                    rv = (* statusConfig->statusChecker)(handle, cert,
                                                                 t, wincx);
                    if (rv != SECSuccess) {
                        LOG_ERROR(log,cert,0,0);
                        revoked = PR_TRUE;
                        INVALID_USAGE();
                    }
                }
            }
        }

        NEXT_USAGE();
    }
    
loser:
    return(valid);
}

SECStatus
CERT_VerifyCert(CERTCertDBHandle *handle, CERTCertificate *cert,
		PRBool checkSig, SECCertUsage certUsage, int64 t,
		void *wincx, CERTVerifyLog *log)
{
    SECStatus rv;
    unsigned int requiredKeyUsage;
    unsigned int requiredCertType;
    unsigned int flags;
    unsigned int certType;
    PRBool       allowOverride;
    SECCertTimeValidity validity;
    CERTStatusConfig *statusConfig;
   
#ifdef notdef 
    
    rv = CERT_CheckForEvilCert(cert);
    if ( rv != SECSuccess ) {
	PORT_SetError(SEC_ERROR_REVOKED_CERTIFICATE);
	LOG_ERROR_OR_EXIT(log,cert,0,0);
    }
#endif
    
    
    allowOverride = (PRBool)((certUsage == certUsageSSLServer) ||
                             (certUsage == certUsageSSLServerWithStepUp));
    validity = CERT_CheckCertValidTimes(cert, t, allowOverride);
    if ( validity != secCertTimeValid ) {
	LOG_ERROR_OR_EXIT(log,cert,0,validity);
    }

    
    cert_GetCertType(cert);
    certType = cert->nsCertType;
    switch ( certUsage ) {
      case certUsageSSLClient:
      case certUsageSSLServer:
      case certUsageSSLServerWithStepUp:
      case certUsageSSLCA:
      case certUsageEmailSigner:
      case certUsageEmailRecipient:
      case certUsageObjectSigner:
      case certUsageStatusResponder:
	rv = CERT_KeyUsageAndTypeForCertUsage(certUsage, PR_FALSE,
					      &requiredKeyUsage,
					      &requiredCertType);
	if ( rv != SECSuccess ) {
	    PORT_Assert(0);
	    EXIT_IF_NOT_LOGGING(log);
	    requiredKeyUsage = 0;
	    requiredCertType = 0;
	}
	break;
      case certUsageVerifyCA:
      case certUsageAnyCA:
	requiredKeyUsage = KU_KEY_CERT_SIGN;
	requiredCertType = NS_CERT_TYPE_CA;
	if ( ! ( certType & NS_CERT_TYPE_CA ) ) {
	    certType |= NS_CERT_TYPE_CA;
	}
	break;
      default:
	PORT_Assert(0);
	EXIT_IF_NOT_LOGGING(log);
	requiredKeyUsage = 0;
	requiredCertType = 0;
    }
    if ( CERT_CheckKeyUsage(cert, requiredKeyUsage) != SECSuccess ) {
	PORT_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE);
	LOG_ERROR_OR_EXIT(log,cert,0,requiredKeyUsage);
    }
    if ( !( certType & requiredCertType ) ) {
	PORT_SetError(SEC_ERROR_INADEQUATE_CERT_TYPE);
	LOG_ERROR_OR_EXIT(log,cert,0,requiredCertType);
    }

    
    if ( cert->trust ) { 
	switch ( certUsage ) {
	  case certUsageSSLClient:
	  case certUsageSSLServer:
	    flags = cert->trust->sslFlags;
	    
	    
	    if ( flags & CERTDB_VALID_PEER ) {
		if ( flags & CERTDB_TRUSTED ) {	
		    goto winner;
		} else { 
		    PORT_SetError(SEC_ERROR_UNTRUSTED_CERT);
		    LOG_ERROR_OR_EXIT(log,cert,0,flags);
		}
	    }
	    break;
	  case certUsageSSLServerWithStepUp:
	    
	    break;
	  case certUsageSSLCA:
	    break;
	  case certUsageEmailSigner:
	  case certUsageEmailRecipient:
	    flags = cert->trust->emailFlags;
	    
	    
	    if ( ( flags & ( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) ==
		( CERTDB_VALID_PEER | CERTDB_TRUSTED ) ) {
		goto winner;
	    }
	    break;
	  case certUsageObjectSigner:
	    flags = cert->trust->objectSigningFlags;

	    
	    if ( flags & CERTDB_VALID_PEER ) {
		if ( flags & CERTDB_TRUSTED ) {	
		    goto winner;
		} else { 
		    PORT_SetError(SEC_ERROR_UNTRUSTED_CERT);
		    LOG_ERROR_OR_EXIT(log,cert,0,flags);
		}
	    }
	    break;
	  case certUsageVerifyCA:
	  case certUsageStatusResponder:
	    flags = cert->trust->sslFlags;
	    
	    if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
		( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
		goto winner;
	    }
	    flags = cert->trust->emailFlags;
	    
	    if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
		( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
		goto winner;
	    }
	    flags = cert->trust->objectSigningFlags;
	    
	    if ( ( flags & ( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) ==
		( CERTDB_VALID_CA | CERTDB_TRUSTED_CA ) ) {
		goto winner;
	    }
	    break;
	  case certUsageAnyCA:
	  case certUsageProtectedObjectSigner:
	  case certUsageUserCertImport:
	    


	    break;
	}
    }

    rv = CERT_VerifyCertChain(handle, cert, checkSig, certUsage,
			      t, wincx, log);
    if (rv != SECSuccess) {
	EXIT_IF_NOT_LOGGING(log);
    }

    






    statusConfig = CERT_GetStatusConfig(handle);
    if (certUsage != certUsageStatusResponder && statusConfig != NULL) {
	if (statusConfig->statusChecker != NULL) {
	    rv = (* statusConfig->statusChecker)(handle, cert,
							 t, wincx);
	    if (rv != SECSuccess) {
		LOG_ERROR_OR_EXIT(log,cert,0,0);
	    }
	}
    }

winner:
    return(SECSuccess);

loser:
    rv = SECFailure;
    
    return(rv);
}





SECStatus
CERT_VerifyCertificateNow(CERTCertDBHandle *handle, CERTCertificate *cert,
		   PRBool checkSig, SECCertificateUsage requiredUsages,
                   void *wincx, SECCertificateUsage* returnedUsages)
{
    return(CERT_VerifyCertificate(handle, cert, checkSig, 
		   requiredUsages, PR_Now(), wincx, NULL, returnedUsages));
}


SECStatus
CERT_VerifyCertNow(CERTCertDBHandle *handle, CERTCertificate *cert,
		   PRBool checkSig, SECCertUsage certUsage, void *wincx)
{
    return(CERT_VerifyCert(handle, cert, checkSig, 
		   certUsage, PR_Now(), wincx, NULL));
}













CERTCertificate *
CERT_FindMatchingCert(CERTCertDBHandle *handle, SECItem *derName,
		      CERTCertOwner owner, SECCertUsage usage,
		      PRBool preferTrusted, int64 validTime, PRBool validOnly)
{
    CERTCertList *certList = NULL;
    CERTCertificate *cert = NULL;
    unsigned int requiredTrustFlags;
    SECTrustType requiredTrustType;
    unsigned int flags;
    
    PRBool lookingForCA = PR_FALSE;
    SECStatus rv;
    CERTCertListNode *node;
    CERTCertificate *saveUntrustedCA = NULL;
    
    
    PORT_Assert( ! ( preferTrusted && ( owner != certOwnerCA ) ) );
    
    if ( owner == certOwnerCA ) {
	lookingForCA = PR_TRUE;
	if ( preferTrusted ) {
	    rv = CERT_TrustFlagsForCACertUsage(usage, &requiredTrustFlags,
					       &requiredTrustType);
	    if ( rv != SECSuccess ) {
		goto loser;
	    }
	    requiredTrustFlags |= CERTDB_VALID_CA;
	}
    }

    certList = CERT_CreateSubjectCertList(NULL, handle, derName, validTime,
					  validOnly);
    if ( certList != NULL ) {
	rv = CERT_FilterCertListByUsage(certList, usage, lookingForCA);
	if ( rv != SECSuccess ) {
	    goto loser;
	}
	
	node = CERT_LIST_HEAD(certList);
	
	while ( !CERT_LIST_END(node, certList) ) {
	    cert = node->cert;

	    
	    if ( ( owner == certOwnerCA ) && preferTrusted &&
		( requiredTrustType != trustTypeNone ) ) {

		if ( cert->trust == NULL ) {
		    flags = 0;
		} else {
		    flags = SEC_GET_TRUST_FLAGS(cert->trust, requiredTrustType);
		}

		if ( ( flags & requiredTrustFlags ) != requiredTrustFlags ) {
		    
		    


		    if ( saveUntrustedCA == NULL ) {
			saveUntrustedCA = cert;
		    }
		    goto endloop;
		}
	    }
	    
	    break;
	    
endloop:
	    node = CERT_LIST_NEXT(node);
	    cert = NULL;
	}

	
	if ( cert == NULL ) {
	    cert = saveUntrustedCA;
	}

	
	if ( cert != NULL ) {
	    
	    cert = CERT_DupCertificate(cert);
	}
	
	CERT_DestroyCertList(certList);
    }

    return(cert);

loser:
    if ( certList != NULL ) {
	CERT_DestroyCertList(certList);
    }

    return(NULL);
}













SECStatus
CERT_FilterCertListByCANames(CERTCertList *certList, int nCANames,
			     char **caNames, SECCertUsage usage)
{
    CERTCertificate *issuerCert = NULL;
    CERTCertificate *subjectCert;
    CERTCertListNode *node, *freenode;
    CERTCertificate *cert;
    int n;
    char **names;
    PRBool found;
    int64 time;
    
    if ( nCANames <= 0 ) {
	return(SECSuccess);
    }

    time = PR_Now();
    
    node = CERT_LIST_HEAD(certList);
    
    while ( ! CERT_LIST_END(node, certList) ) {
	cert = node->cert;
	
	subjectCert = CERT_DupCertificate(cert);

	
	found = PR_FALSE;
	while ( subjectCert != NULL ) {
	    n = nCANames;
	    names = caNames;
	   
            if (subjectCert->issuerName != NULL) { 
	        while ( n > 0 ) {
		    if ( PORT_Strcmp(*names, subjectCert->issuerName) == 0 ) {
		        found = PR_TRUE;
		        break;
		    }

		    n--;
		    names++;
                }
	    }

	    if ( found ) {
		break;
	    }
	    
	    issuerCert = CERT_FindCertIssuer(subjectCert, time, usage);
	    if ( issuerCert == subjectCert ) {
		CERT_DestroyCertificate(issuerCert);
		issuerCert = NULL;
		break;
	    }
	    CERT_DestroyCertificate(subjectCert);
	    subjectCert = issuerCert;

	}
	CERT_DestroyCertificate(subjectCert);
	if ( !found ) {
	    
	    freenode = node;
	    node = CERT_LIST_NEXT(node);
	    CERT_RemoveCertListNode(freenode);
	} else {
	    
	    node = CERT_LIST_NEXT(node);
	}
    }
    
    return(SECSuccess);
}














char *
CERT_GetCertNicknameWithValidity(PRArenaPool *arena, CERTCertificate *cert,
				 char *expiredString, char *notYetGoodString)
{
    SECCertTimeValidity validity;
    char *nickname = NULL, *tmpstr = NULL;
    
    validity = CERT_CheckCertValidTimes(cert, PR_Now(), PR_FALSE);

    
    if ( validity == secCertTimeValid ) {
	if ( arena == NULL ) {
	    nickname = PORT_Strdup(cert->nickname);
	} else {
	    nickname = PORT_ArenaStrdup(arena, cert->nickname);
	}
	
	if ( nickname == NULL ) {
	    goto loser;
	}
    } else {
	    
	


	if ( validity == secCertTimeExpired ) {
	    tmpstr = PR_smprintf("%s%s", cert->nickname,
				 expiredString);
	} else if ( validity == secCertTimeNotValidYet ) {
	    
	    tmpstr = PR_smprintf("%s%s", cert->nickname,
				 notYetGoodString);
        } else {
            
	    tmpstr = PR_smprintf("%s",
                        "(NULL) (Validity Unknown)");
        }

	if ( tmpstr == NULL ) {
	    goto loser;
	}

	if ( arena ) {
	    
	    nickname = PORT_ArenaStrdup(arena, tmpstr);
	    PORT_Free(tmpstr);
	} else {
	    nickname = tmpstr;
	}
	if ( nickname == NULL ) {
	    goto loser;
	}
    }    
    return(nickname);

loser:
    return(NULL);
}










CERTCertNicknames *
CERT_NicknameStringsFromCertList(CERTCertList *certList, char *expiredString,
				 char *notYetGoodString)
{
    CERTCertNicknames *names;
    PRArenaPool *arena;
    CERTCertListNode *node;
    char **nn;
    
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return(NULL);
    }
    
    
    names = PORT_ArenaAlloc(arena, sizeof(CERTCertNicknames));
    if ( names == NULL ) {
	goto loser;
    }

    
    names->arena = arena;
    names->head = NULL;
    names->numnicknames = 0;
    names->nicknames = NULL;
    names->totallen = 0;

    
    node = CERT_LIST_HEAD(certList);
    while ( ! CERT_LIST_END(node, certList) ) {
	names->numnicknames++;
	node = CERT_LIST_NEXT(node);
    }
    
    
    names->nicknames = PORT_ArenaAlloc(arena,
				       sizeof(char *) * names->numnicknames);
    if ( names->nicknames == NULL ) {
	goto loser;
    }

    
    if (expiredString == NULL ) {
	expiredString = "";
    }

    if ( notYetGoodString == NULL ) {
	notYetGoodString = "";
    }
    
    
    nn = names->nicknames;
    node = CERT_LIST_HEAD(certList);
    while ( ! CERT_LIST_END(node, certList) ) {
	*nn = CERT_GetCertNicknameWithValidity(arena, node->cert,
					       expiredString,
					       notYetGoodString);
	if ( *nn == NULL ) {
	    goto loser;
	}

	names->totallen += PORT_Strlen(*nn);
	
	nn++;
	node = CERT_LIST_NEXT(node);
    }

    return(names);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(NULL);
}













char *
CERT_ExtractNicknameString(char *namestring, char *expiredString,
			   char *notYetGoodString)
{
    int explen, nyglen, namelen;
    int retlen;
    char *retstr;
    
    namelen = PORT_Strlen(namestring);
    explen = PORT_Strlen(expiredString);
    nyglen = PORT_Strlen(notYetGoodString);
    
    if ( namelen > explen ) {
	if ( PORT_Strcmp(expiredString, &namestring[namelen-explen]) == 0 ) {
	    retlen = namelen - explen;
	    retstr = (char *)PORT_Alloc(retlen+1);
	    if ( retstr == NULL ) {
		goto loser;
	    }
	    
	    PORT_Memcpy(retstr, namestring, retlen);
	    retstr[retlen] = '\0';
	    goto done;
	}
    }

    if ( namelen > nyglen ) {
	if ( PORT_Strcmp(notYetGoodString, &namestring[namelen-nyglen]) == 0) {
	    retlen = namelen - nyglen;
	    retstr = (char *)PORT_Alloc(retlen+1);
	    if ( retstr == NULL ) {
		goto loser;
	    }
	    
	    PORT_Memcpy(retstr, namestring, retlen);
	    retstr[retlen] = '\0';
	    goto done;
	}
    }

    


    retstr = PORT_Strdup(namestring);
    
done:
    return(retstr);

loser:
    return(NULL);
}

CERTCertList *
CERT_GetCertChainFromCert(CERTCertificate *cert, int64 time, SECCertUsage usage)
{
    CERTCertList *chain = NULL;
    int count = 0;

    if (NULL == cert) {
        return NULL;
    }
    
    cert = CERT_DupCertificate(cert);
    if (NULL == cert) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return NULL;
    }

    chain = CERT_NewCertList();
    if (NULL == chain) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return NULL;
    }

    while (cert != NULL && ++count <= CERT_MAX_CERT_CHAIN) {
	if (SECSuccess != CERT_AddCertToListTail(chain, cert)) {
            
            PORT_SetError(SEC_ERROR_NO_MEMORY);
            return chain;
        }

	if (cert->isRoot) {
            
	    return chain;
	}

	cert = CERT_FindCertIssuer(cert, time, usage);
    }

    
    PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
    return chain;
}
