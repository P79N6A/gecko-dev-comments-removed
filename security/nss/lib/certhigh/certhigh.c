


































#include "nspr.h"
#include "secerr.h"
#include "secasn1.h"
#include "seccomon.h"
#include "pk11func.h"
#include "certdb.h"
#include "certt.h"
#include "cert.h"
#include "certxutl.h"

#include "nsspki.h"
#include "pki.h"
#include "pkit.h"
#include "pkitm.h"
#include "pki3hack.h"


PRBool
CERT_MatchNickname(char *name1, char *name2) {
    char *nickname1= NULL;
    char *nickname2 = NULL;
    char *token1;
    char *token2;
    char *token = NULL;
    int len;

    
    if (PORT_Strcmp(name1, name2) == 0) {
	return PR_TRUE;
    }
    

    token1 = PORT_Strchr(name1,':');
    token2 = PORT_Strchr(name2,':');
    if ((token1 && token2) || (!token1 && !token2)) {
	
	return PR_FALSE;
    }
    if (token1) {
	token=name1;
	nickname1=token1;
	nickname2=name2;
    } else {
	token=name2;
	nickname1=token2;
	nickname2=name1;
    }
    len = nickname1-token;
    nickname1++;
    if (PORT_Strcmp(nickname1,nickname2) != 0) {
	return PR_FALSE;
    }
    
    return PR_TRUE;
}











CERTCertList *
CERT_FindUserCertsByUsage(CERTCertDBHandle *handle,
			  SECCertUsage usage,
			  PRBool oneCertPerName,
			  PRBool validOnly,
			  void *proto_win)
{
    CERTCertNicknames *nicknames = NULL;
    char **nnptr;
    int nn;
    CERTCertificate *cert = NULL;
    CERTCertList *certList = NULL;
    SECStatus rv;
    int64 time;
    CERTCertListNode *node = NULL;
    CERTCertListNode *freenode = NULL;
    int n;
    
    time = PR_Now();
    
    nicknames = CERT_GetCertNicknames(handle, SEC_CERT_NICKNAMES_USER,
				      proto_win);
    
    if ( ( nicknames == NULL ) || ( nicknames->numnicknames == 0 ) ) {
	goto loser;
    }

    nnptr = nicknames->nicknames;
    nn = nicknames->numnicknames;

    while ( nn > 0 ) {
	cert = NULL;
	


	if ( proto_win != NULL ) {
	    cert = PK11_FindCertFromNickname(*nnptr,proto_win);
	}

	







	if (cert == NULL) { 
	    cert = CERT_FindCertByNickname(handle,*nnptr);
	}

	if ( cert != NULL ) {
	   
	    certList = CERT_CreateSubjectCertList(certList, handle, 
				&cert->derSubject, time, validOnly);

	    CERT_FilterCertListForUserCerts(certList);
	
	    
	    CERT_DestroyCertificate(cert);
	}
	
	nnptr++;
	nn--;
    }

    
    rv = CERT_FilterCertListByUsage(certList, usage, PR_FALSE);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    
    if ( oneCertPerName ) {
	PRBool *flags;

	nn = nicknames->numnicknames;
	nnptr = nicknames->nicknames;
	
	flags = (PRBool *)PORT_ZAlloc(sizeof(PRBool) * nn);
	if ( flags == NULL ) {
	    goto loser;
	}
	
	node = CERT_LIST_HEAD(certList);
	
	
	while ( !CERT_LIST_END(node, certList) ) {

	    
	    for ( n = 0; n < nn; n++ ) {
		if ( CERT_MatchNickname(nnptr[n], node->cert->nickname) ) {
		    



		    if ( flags[n] ) {
			


			freenode = node;
			node = CERT_LIST_NEXT(node);
			CERT_RemoveCertListNode(freenode);
		    } else {
			



			flags[n] = PR_TRUE;
			node = CERT_LIST_NEXT(node);
		    }
		    break;
		}
	    }
	    if ( n == nn ) {
		


		PORT_Assert(0);
		node = CERT_LIST_NEXT(node);
	    }
	}
	PORT_Free(flags);
    }

    goto done;
    
loser:
    if ( certList != NULL ) {
	CERT_DestroyCertList(certList);
	certList = NULL;
    }

done:
    if ( nicknames != NULL ) {
	CERT_FreeNicknames(nicknames);
    }

    return(certList);
}










CERTCertificate *
CERT_FindUserCertByUsage(CERTCertDBHandle *handle,
			 const char *nickname,
			 SECCertUsage usage,
			 PRBool validOnly,
			 void *proto_win)
{
    CERTCertificate *cert = NULL;
    CERTCertList *certList = NULL;
    SECStatus rv;
    int64 time;
    
    time = PR_Now();
    
    


    
    if ( proto_win != NULL ) {
	cert = PK11_FindCertFromNickname(nickname,proto_win);
    }


    



    if (cert == NULL) {
	cert = CERT_FindCertByNickname(handle,nickname);
    }

    if ( cert != NULL ) {
	unsigned int requiredKeyUsage;
	unsigned int requiredCertType;

	rv = CERT_KeyUsageAndTypeForCertUsage(usage, PR_FALSE,
					&requiredKeyUsage, &requiredCertType);
	if ( rv != SECSuccess ) {
	    
	    CERT_DestroyCertificate(cert);
	    cert = NULL;
	    goto loser;
	}
	
	if ( (!validOnly || CERT_CheckCertValidTimes(cert, time, PR_FALSE)
	      == secCertTimeValid) &&
	     (CERT_CheckKeyUsage(cert, requiredKeyUsage) == SECSuccess) &&
	     (cert->nsCertType & requiredCertType) &&
	      CERT_IsUserCert(cert) ) {
	    return(cert);
	}

 	
	certList = CERT_CreateSubjectCertList(certList, handle, 
					&cert->derSubject, time, validOnly);

	CERT_FilterCertListForUserCerts(certList);

	
	CERT_DestroyCertificate(cert);
	cert = NULL;
    }
	
    if ( certList == NULL ) {
	goto loser;
    }
    
    
    rv = CERT_FilterCertListByUsage(certList, usage, PR_FALSE);

    if ( rv != SECSuccess ) {
	goto loser;
    }

    if ( ! CERT_LIST_END(CERT_LIST_HEAD(certList), certList) ) {
	cert = CERT_DupCertificate(CERT_LIST_HEAD(certList)->cert);
    }
    
loser:
    if ( certList != NULL ) {
	CERT_DestroyCertList(certList);
    }

    return(cert);
}

CERTCertList *
CERT_MatchUserCert(CERTCertDBHandle *handle,
		   SECCertUsage usage,
		   int nCANames, char **caNames,
		   void *proto_win)
{
    CERTCertList *certList = NULL;
    SECStatus rv;

    certList = CERT_FindUserCertsByUsage(handle, usage, PR_TRUE, PR_TRUE,
					 proto_win);
    if ( certList == NULL ) {
	goto loser;
    }
    
    rv = CERT_FilterCertListByCANames(certList, nCANames, caNames, usage);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    goto done;
    
loser:
    if ( certList != NULL ) {
	CERT_DestroyCertList(certList);
	certList = NULL;
    }

done:

    return(certList);
}


typedef struct stringNode {
    struct stringNode *next;
    char *string;
} stringNode;
    
static PRStatus
CollectNicknames( NSSCertificate *c, void *data)
{
    CERTCertNicknames *names;
    PRBool saveit = PR_FALSE;
    stringNode *node;
    int len;
#ifdef notdef
    NSSTrustDomain *td;
    NSSTrust *trust;
#endif
    char *stanNickname;
    char *nickname = NULL;
    
    names = (CERTCertNicknames *)data;

    stanNickname = nssCertificate_GetNickname(c,NULL);
    
    if ( stanNickname ) {
	if (names->what == SEC_CERT_NICKNAMES_USER) {
	    saveit = NSSCertificate_IsPrivateKeyAvailable(c, NULL, NULL);
	}
#ifdef notdef
	  else {
	    td = NSSCertificate_GetTrustDomain(c);
	    if (!td) {
		return PR_SUCCESS;
	    }
	    trust = nssTrustDomain_FindTrustForCertificate(td,c);
	
	    switch(names->what) {
	     case SEC_CERT_NICKNAMES_ALL:
		if ((trust->sslFlags & (CERTDB_VALID_CA|CERTDB_VALID_PEER) ) ||
		 (trust->emailFlags & (CERTDB_VALID_CA|CERTDB_VALID_PEER) ) ||
		 (trust->objectSigningFlags & 
					(CERTDB_VALID_CA|CERTDB_VALID_PEER))) {
		    saveit = PR_TRUE;
		}
	    
		break;
	     case SEC_CERT_NICKNAMES_SERVER:
		if ( trust->sslFlags & CERTDB_VALID_PEER ) {
		    saveit = PR_TRUE;
		}
	    
		break;
	     case SEC_CERT_NICKNAMES_CA:
		if (((trust->sslFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA)||
		 ((trust->emailFlags & CERTDB_VALID_CA ) == CERTDB_VALID_CA) ||
		 ((trust->objectSigningFlags & CERTDB_VALID_CA ) 
							== CERTDB_VALID_CA)) {
		    saveit = PR_TRUE;
		}
		break;
	    }
	}
#endif
    }

    


    if ( saveit ) {
	nickname = STAN_GetCERTCertificateName(NULL, c);
	

	if (nickname == NULL) {
	    return PR_FAILURE;
	}
	node = (stringNode *)names->head;
	while ( node != NULL ) {
	    if ( PORT_Strcmp(nickname, node->string) == 0 ) { 
		
		saveit = PR_FALSE;
		break;
	    }
	    node = node->next;
	}
    }

    if ( saveit ) {
	
	
	node = (stringNode*)PORT_ArenaAlloc(names->arena, sizeof(stringNode));
	if ( node == NULL ) {
	    PORT_Free(nickname);
	    return PR_FAILURE;
	}

	
	len = PORT_Strlen(nickname) + 1;
	node->string = (char*)PORT_ArenaAlloc(names->arena, len);
	if ( node->string == NULL ) {
	    PORT_Free(nickname);
	    return PR_FAILURE;
	}
	PORT_Memcpy(node->string, nickname, len);

	
	node->next = (stringNode *)names->head;
	names->head = (void *)node;

	
	names->numnicknames++;
    }
    
    if (nickname) PORT_Free(nickname);
    return(PR_SUCCESS);
}

CERTCertNicknames *
CERT_GetCertNicknames(CERTCertDBHandle *handle, int what, void *wincx)
{
    PRArenaPool *arena;
    CERTCertNicknames *names;
    int i;
    stringNode *node;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return(NULL);
    }
    
    names = (CERTCertNicknames *)PORT_ArenaAlloc(arena, sizeof(CERTCertNicknames));
    if ( names == NULL ) {
	goto loser;
    }

    names->arena = arena;
    names->head = NULL;
    names->numnicknames = 0;
    names->nicknames = NULL;
    names->what = what;
    names->totallen = 0;

    
    (void) pk11_TraverseAllSlots(NULL, NULL, PR_TRUE, wincx);
   
    NSSTrustDomain_TraverseCertificates(handle,
					    CollectNicknames, (void *)names);
    if ( names->numnicknames ) {
	names->nicknames = (char**)PORT_ArenaAlloc(arena,
					 names->numnicknames * sizeof(char *));

	if ( names->nicknames == NULL ) {
	    goto loser;
	}
    
	node = (stringNode *)names->head;
	
	for ( i = 0; i < names->numnicknames; i++ ) {
	    PORT_Assert(node != NULL);
	    
	    names->nicknames[i] = node->string;
	    names->totallen += PORT_Strlen(node->string);
	    node = node->next;
	}

	PORT_Assert(node == NULL);
    }

    return(names);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(NULL);
}

void
CERT_FreeNicknames(CERTCertNicknames *nicknames)
{
    PORT_FreeArena(nicknames->arena, PR_FALSE);
    
    return;
}



typedef struct dnameNode {
    struct dnameNode *next;
    SECItem name;
} dnameNode;

void
CERT_FreeDistNames(CERTDistNames *names)
{
    PORT_FreeArena(names->arena, PR_FALSE);
    
    return;
}

static SECStatus
CollectDistNames( CERTCertificate *cert, SECItem *k, void *data)
{
    CERTDistNames *names;
    PRBool saveit = PR_FALSE;
    CERTCertTrust *trust;
    dnameNode *node;
    int len;
    
    names = (CERTDistNames *)data;
    
    if ( cert->trust ) {
	trust = cert->trust;
	
	
	if (  trust->sslFlags &  CERTDB_TRUSTED_CLIENT_CA )  {
	    saveit = PR_TRUE;
	}
    }

    if ( saveit ) {
	
	node = (dnameNode*)PORT_ArenaAlloc(names->arena, sizeof(dnameNode));
	if ( node == NULL ) {
	    return(SECFailure);
	}

	
	node->name.len = len = cert->derSubject.len;
	node->name.type = siBuffer;
	node->name.data = (unsigned char*)PORT_ArenaAlloc(names->arena, len);
	if ( node->name.data == NULL ) {
	    return(SECFailure);
	}
	PORT_Memcpy(node->name.data, cert->derSubject.data, len);

	
	node->next = (dnameNode *)names->head;
	names->head = (void *)node;

	
	names->nnames++;
    }
    
    return(SECSuccess);
}




CERTDistNames *
CERT_GetSSLCACerts(CERTCertDBHandle *handle)
{
    PRArenaPool *arena;
    CERTDistNames *names;
    int i;
    SECStatus rv;
    dnameNode *node;
    
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return(NULL);
    }
    
    
    names = (CERTDistNames *)PORT_ArenaAlloc(arena, sizeof(CERTDistNames));
    if ( names == NULL ) {
	goto loser;
    }

    
    names->arena = arena;
    names->head = NULL;
    names->nnames = 0;
    names->names = NULL;
    
    
    rv = PK11_TraverseSlotCerts(CollectDistNames, (void *)names, NULL);
    if ( rv ) {
	goto loser;
    }

    
    if ( names->nnames ) {
	names->names = (SECItem*)PORT_ArenaAlloc(arena, names->nnames * sizeof(SECItem));

	if ( names->names == NULL ) {
	    goto loser;
	}
    
	node = (dnameNode *)names->head;
	
	for ( i = 0; i < names->nnames; i++ ) {
	    PORT_Assert(node != NULL);
	    
	    names->names[i] = node->name;
	    node = node->next;
	}

	PORT_Assert(node == NULL);
    }

    return(names);
    
loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(NULL);
}

CERTDistNames *
CERT_DistNamesFromNicknames(CERTCertDBHandle *handle, char **nicknames,
			   int nnames)
{
    CERTDistNames *dnames = NULL;
    PRArenaPool *arena;
    int i, rv;
    SECItem *names = NULL;
    CERTCertificate *cert = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) goto loser;
    dnames = PORT_ArenaZNew(arena, CERTDistNames);
    if (dnames == NULL) goto loser;

    dnames->arena = arena;
    dnames->nnames = nnames;
    dnames->names = names = PORT_ArenaZNewArray(arena, SECItem, nnames);
    if (names == NULL) goto loser;
    
    for (i = 0; i < nnames; i++) {
	cert = CERT_FindCertByNicknameOrEmailAddr(handle, nicknames[i]);
	if (cert == NULL) goto loser;
	rv = SECITEM_CopyItem(arena, &names[i], &cert->derSubject);
	if (rv == SECFailure) goto loser;
	CERT_DestroyCertificate(cert);
    }
    return dnames;
    
loser:
    if (cert != NULL)
	CERT_DestroyCertificate(cert);
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}





CERTCertificate *
CERT_FindCertByNameString(CERTCertDBHandle *handle, char *nameStr)
{
    CERTName *name;
    SECItem *nameItem;
    CERTCertificate *cert = NULL;
    PRArenaPool *arena = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( arena == NULL ) {
	goto loser;
    }
    
    name = CERT_AsciiToName(nameStr);
    
    if ( name ) {
	nameItem = SEC_ASN1EncodeItem (arena, NULL, (void *)name,
				       CERT_NameTemplate);
	if ( nameItem != NULL ) {
            cert = CERT_FindCertByName(handle, nameItem);
	}
	CERT_DestroyName(name);
    }

loser:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(cert);
}



CERTCrlDistributionPoints *
CERT_FindCRLDistributionPoints (CERTCertificate *cert)
{
    SECItem encodedExtenValue;
    SECStatus rv;
    CERTCrlDistributionPoints *dps;

    encodedExtenValue.data = NULL;
    encodedExtenValue.len = 0;

    rv = cert_FindExtension(cert->extensions, SEC_OID_X509_CRL_DIST_POINTS,
			    &encodedExtenValue);
    if ( rv != SECSuccess ) {
	return (NULL);
    }

    dps = CERT_DecodeCRLDistributionPoints(cert->arena, &encodedExtenValue);

    PORT_Free(encodedExtenValue.data);

    return dps;
}


CERTSignedCrl * CERT_ImportCRL
   (CERTCertDBHandle *handle, SECItem *derCRL, char *url, int type, void *wincx)
{
    CERTSignedCrl* retCrl = NULL;
    PK11SlotInfo* slot = PK11_GetInternalKeySlot();
    retCrl = PK11_ImportCRL(slot, derCRL, url, type, wincx,
        CRL_IMPORT_DEFAULT_OPTIONS, NULL, CRL_DECODE_DEFAULT_OPTIONS);
    PK11_FreeSlot(slot);

    return retCrl;
}


static SECStatus
cert_ImportCAChain(SECItem *certs, int numcerts, SECCertUsage certUsage, PRBool trusted)
{
    SECStatus rv;
    SECItem *derCert;
    CERTCertificate *cert = NULL;
    CERTCertificate *newcert = NULL;
    CERTCertDBHandle *handle;
    CERTCertTrust trust;
    PRBool isca;
    char *nickname;
    unsigned int certtype;
    
    handle = CERT_GetDefaultCertDB();
    
    while (numcerts--) {
	derCert = certs;
	certs++;

	
	
	newcert = CERT_DecodeDERCertificate(derCert, PR_FALSE, NULL);
	if ( newcert == NULL ) {
	    goto loser;
	}

	if (!trusted) {
	    
	    rv = CERT_CertTimesValid(newcert);
	    if ( rv == SECFailure ) {
		goto endloop;
	    }
	}

	
	
	



	isca = CERT_IsCACert(newcert, &certtype);

	if ( !isca ) {
	    if (!trusted) {
		goto endloop;
	    }
	    trust.sslFlags = CERTDB_VALID_CA;
	    trust.emailFlags = CERTDB_VALID_CA;
	    trust.objectSigningFlags = CERTDB_VALID_CA;
	} else {
	    
	    if ( ( certUsage == certUsageSSLCA ) &&
		(( certtype & NS_CERT_TYPE_SSL_CA ) != NS_CERT_TYPE_SSL_CA )) {
		goto endloop;
	    }

	    
	    
	    PORT_Memset((void *)&trust, 0, sizeof(trust));
	    switch ( certUsage ) {
	      case certUsageSSLCA:
		trust.sslFlags = CERTDB_VALID_CA;
		break;
	      case certUsageUserCertImport:
		if ((certtype & NS_CERT_TYPE_SSL_CA) == NS_CERT_TYPE_SSL_CA) {
		    trust.sslFlags = CERTDB_VALID_CA;
		}
		if ((certtype & NS_CERT_TYPE_EMAIL_CA) 
						== NS_CERT_TYPE_EMAIL_CA ) {
		    trust.emailFlags = CERTDB_VALID_CA;
		}
		if ( ( certtype & NS_CERT_TYPE_OBJECT_SIGNING_CA ) ==
					NS_CERT_TYPE_OBJECT_SIGNING_CA ) {
		     trust.objectSigningFlags = CERTDB_VALID_CA;
		}
		break;
	      default:
		PORT_Assert(0);
		break;
	    }
	}
	
	cert = CERT_NewTempCertificate(handle, derCert, NULL, 
							PR_FALSE, PR_FALSE);
	if ( cert == NULL ) {
	    goto loser;
	}
	
	
	if (cert->istemp) {
	    
	    nickname = CERT_MakeCANickname(cert);

	    rv = CERT_AddTempCertToPerm(cert, nickname, &trust);

	    
	    if ( nickname ) {
		PORT_Free(nickname);
	    }
	} else {
	    rv = SECSuccess;
	}

	CERT_DestroyCertificate(cert);
	cert = NULL;
	
	if ( rv != SECSuccess ) {
	    goto loser;
	}

endloop:
	if ( newcert ) {
	    CERT_DestroyCertificate(newcert);
	    newcert = NULL;
	}
	
    }

    rv = SECSuccess;
    goto done;
loser:
    rv = SECFailure;
done:
    
    if ( newcert ) {
	CERT_DestroyCertificate(newcert);
	newcert = NULL;
    }
    
    if ( cert ) {
	CERT_DestroyCertificate(cert);
	cert = NULL;
    }
    
    return(rv);
}

SECStatus
CERT_ImportCAChain(SECItem *certs, int numcerts, SECCertUsage certUsage)
{
    return cert_ImportCAChain(certs, numcerts, certUsage, PR_FALSE);
}

SECStatus
CERT_ImportCAChainTrusted(SECItem *certs, int numcerts, SECCertUsage certUsage) {
    return cert_ImportCAChain(certs, numcerts, certUsage, PR_TRUE);
}











typedef struct certNode {
    struct certNode *next;
    CERTCertificate *cert;
} certNode;

CERTCertificateList *
CERT_CertChainFromCert(CERTCertificate *cert, SECCertUsage usage,
		       PRBool includeRoot)
{
    CERTCertificateList *chain = NULL;
    NSSCertificate **stanChain;
    NSSCertificate *stanCert;
    PRArenaPool *arena;
    NSSUsage nssUsage;
    int i, len;
    NSSTrustDomain *td   = STAN_GetDefaultTrustDomain();
    NSSCryptoContext *cc = STAN_GetDefaultCryptoContext();

    stanCert = STAN_GetNSSCertificate(cert);
    if (!stanCert) {
        
        return NULL;
    }
    nssUsage.anyUsage = PR_FALSE;
    nssUsage.nss3usage = usage;
    nssUsage.nss3lookingForCA = PR_FALSE;
    stanChain = NSSCertificate_BuildChain(stanCert, NULL, &nssUsage, NULL, NULL,
					  CERT_MAX_CERT_CHAIN, NULL, NULL, td, cc);
    if (!stanChain) {
	PORT_SetError(SEC_ERROR_UNKNOWN_ISSUER);
	return NULL;
    }

    len = 0;
    stanCert = stanChain[0];
    while (stanCert) {
	stanCert = stanChain[++len];
    }

    arena = PORT_NewArena(4096);
    if (arena == NULL) {
	goto loser;
    }

    chain = (CERTCertificateList *)PORT_ArenaAlloc(arena, 
                                                 sizeof(CERTCertificateList));
    if (!chain) goto loser;
    chain->certs = (SECItem*)PORT_ArenaAlloc(arena, len * sizeof(SECItem));
    if (!chain->certs) goto loser;
    i = 0;
    stanCert = stanChain[i];
    while (stanCert) {
	SECItem derCert;
	CERTCertificate *cCert = STAN_GetCERTCertificate(stanCert);
	if (!cCert) {
	    goto loser;
	}
	derCert.len = (unsigned int)stanCert->encoding.size;
	derCert.data = (unsigned char *)stanCert->encoding.data;
	derCert.type = siBuffer;
	SECITEM_CopyItem(arena, &chain->certs[i], &derCert);
	stanCert = stanChain[++i];
	if (!stanCert && !cCert->isRoot) {
	    


	    includeRoot = PR_TRUE;
	}
	CERT_DestroyCertificate(cCert);
    }
    if ( !includeRoot && len > 1) {
	chain->len = len - 1;
    } else {
	chain->len = len;
    }
    
    chain->arena = arena;
    nss_ZFreeIf(stanChain);
    return chain;
loser:
    i = 0;
    stanCert = stanChain[i];
    while (stanCert) {
	CERTCertificate *cCert = STAN_GetCERTCertificate(stanCert);
	if (cCert) {
	    CERT_DestroyCertificate(cCert);
	}
	stanCert = stanChain[++i];
    }
    nss_ZFreeIf(stanChain);
    if (arena) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return NULL;
}




CERTCertificateList *
CERT_CertListFromCert(CERTCertificate *cert)
{
    CERTCertificateList *chain = NULL;
    int rv;
    PRArenaPool *arena;

    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) goto no_memory;

    
    chain = (CERTCertificateList *)PORT_ArenaAlloc(arena, sizeof(CERTCertificateList));
    if (chain == NULL) goto no_memory;
    chain->certs = (SECItem*)PORT_ArenaAlloc(arena, 1 * sizeof(SECItem));
    if (chain->certs == NULL) goto no_memory;
    rv = SECITEM_CopyItem(arena, chain->certs, &(cert->derCert));
    if (rv < 0) goto loser;
    chain->len = 1;
    chain->arena = arena;

    return chain;

no_memory:
    PORT_SetError(SEC_ERROR_NO_MEMORY);
loser:
    if (arena != NULL) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return NULL;
}

CERTCertificateList *
CERT_DupCertList(CERTCertificateList * oldList)
{
    CERTCertificateList *newList = NULL;
    PRArenaPool         *arena   = NULL;
    SECItem             *newItem;
    SECItem             *oldItem;
    int                 len      = oldList->len;
    int                 rv;

    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) 
	goto no_memory;

    
    newList = PORT_ArenaNew(arena, CERTCertificateList);
    if (newList == NULL) 
	goto no_memory;
    newList->arena = arena;
    newItem = (SECItem*)PORT_ArenaAlloc(arena, len * sizeof(SECItem));
    if (newItem == NULL) 
	goto no_memory;
    newList->certs = newItem;
    newList->len   = len;

    for (oldItem = oldList->certs; len > 0; --len, ++newItem, ++oldItem) {
	rv = SECITEM_CopyItem(arena, newItem, oldItem);
	if (rv < 0) 
	    goto loser;
    }
    return newList;

no_memory:
    PORT_SetError(SEC_ERROR_NO_MEMORY);
loser:
    if (arena != NULL) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return NULL;
}

void
CERT_DestroyCertificateList(CERTCertificateList *list)
{
    PORT_FreeArena(list->arena, PR_FALSE);
}

