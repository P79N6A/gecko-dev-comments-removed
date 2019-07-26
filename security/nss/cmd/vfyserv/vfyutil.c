



#include "vfyserv.h"
#include "secerr.h"
#include "sslerr.h"
#include "nspr.h"
#include "secutil.h"


extern PRBool dumpChain;
extern void dumpCertChain(CERTCertificate *, SECCertUsage);



int ssl2CipherSuites[] = {
    SSL_EN_RC4_128_WITH_MD5,              
    SSL_EN_RC4_128_EXPORT40_WITH_MD5,     
    SSL_EN_RC2_128_CBC_WITH_MD5,          
    SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5, 
    SSL_EN_DES_64_CBC_WITH_MD5,           
    SSL_EN_DES_192_EDE3_CBC_WITH_MD5,     
    0
};

int ssl3CipherSuites[] = {
    -1, 
    -1, 
    SSL_RSA_WITH_RC4_128_MD5,			
    SSL_RSA_WITH_3DES_EDE_CBC_SHA,		
    SSL_RSA_WITH_DES_CBC_SHA,			
    SSL_RSA_EXPORT_WITH_RC4_40_MD5,		
    SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5,		
    -1, 
    SSL_RSA_WITH_NULL_MD5,			
    SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA,		
    SSL_RSA_FIPS_WITH_DES_CBC_SHA,		
    TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA,	
    TLS_RSA_EXPORT1024_WITH_RC4_56_SHA,	        
    SSL_RSA_WITH_RC4_128_SHA,			
    TLS_DHE_DSS_WITH_RC4_128_SHA,		
    SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA,		
    SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA,		
    SSL_DHE_RSA_WITH_DES_CBC_SHA,		
    SSL_DHE_DSS_WITH_DES_CBC_SHA,		
    TLS_DHE_DSS_WITH_AES_128_CBC_SHA, 	    	
    TLS_DHE_RSA_WITH_AES_128_CBC_SHA,       	
    TLS_RSA_WITH_AES_128_CBC_SHA,     	    	
    TLS_DHE_DSS_WITH_AES_256_CBC_SHA, 	    	
    TLS_DHE_RSA_WITH_AES_256_CBC_SHA,       	
    TLS_RSA_WITH_AES_256_CBC_SHA,     	    	
    SSL_RSA_WITH_NULL_SHA,			
    0
};














char *
myPasswd(PK11SlotInfo *info, PRBool retry, void *arg)
{
    char * passwd = NULL;

    if ( (!retry) && arg ) {
	passwd = PORT_Strdup((char *)arg);
    }
    return passwd;
}








SECStatus 
myAuthCertificate(void *arg, PRFileDesc *socket, 
                  PRBool checksig, PRBool isServer) 
{

    SECCertificateUsage certUsage;
    CERTCertificate *   cert;
    void *              pinArg;
    char *              hostName;
    SECStatus           secStatus;

    if (!arg || !socket) {
	errWarn("myAuthCertificate");
	return SECFailure;
    }

    

    certUsage = isServer ? certificateUsageSSLClient : certificateUsageSSLServer;

    cert = SSL_PeerCertificate(socket);
	
    pinArg = SSL_RevealPinArg(socket);
    
    if (dumpChain == PR_TRUE) {
        dumpCertChain(cert, certUsage);
    }

    secStatus = CERT_VerifyCertificateNow((CERTCertDBHandle *)arg,
				   cert,
				   checksig,
				   certUsage,
				   pinArg,
                                   NULL);

    
    if (isServer || secStatus != SECSuccess) {
	SECU_printCertProblems(stderr, (CERTCertDBHandle *)arg, cert, 
			  checksig, certUsage, pinArg, PR_FALSE);
	CERT_DestroyCertificate(cert);
	return secStatus;
    }

    





    
    hostName = SSL_RevealURL(socket);

    if (hostName && hostName[0]) {
	secStatus = CERT_VerifyCertName(cert, hostName);
    } else {
	PR_SetError(SSL_ERROR_BAD_CERT_DOMAIN, 0);
	secStatus = SECFailure;
    }

    if (hostName)
	PR_Free(hostName);

    CERT_DestroyCertificate(cert);
    return secStatus;
}










SECStatus 
myBadCertHandler(void *arg, PRFileDesc *socket) 
{

    SECStatus	secStatus = SECFailure;
    PRErrorCode	err;

    

    if (!arg) {
	return secStatus;
    }

    *(PRErrorCode *)arg = err = PORT_GetError();

    
    	
    

    switch (err) {
    case SEC_ERROR_INVALID_AVA:
    case SEC_ERROR_INVALID_TIME:
    case SEC_ERROR_BAD_SIGNATURE:
    case SEC_ERROR_EXPIRED_CERTIFICATE:
    case SEC_ERROR_UNKNOWN_ISSUER:
    case SEC_ERROR_UNTRUSTED_CERT:
    case SEC_ERROR_CERT_VALID:
    case SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE:
    case SEC_ERROR_CRL_EXPIRED:
    case SEC_ERROR_CRL_BAD_SIGNATURE:
    case SEC_ERROR_EXTENSION_VALUE_INVALID:
    case SEC_ERROR_CA_CERT_INVALID:
    case SEC_ERROR_CERT_USAGES_INVALID:
    case SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION:
	secStatus = SECSuccess;
	break;
    default:
	secStatus = SECFailure;
	break;
    }

    fprintf(stderr, "Bad certificate: %d, %s\n", err, SECU_Strerror(err));

    return secStatus;
}






SECStatus 
myGetClientAuthData(void *arg,
                    PRFileDesc *socket,
                    struct CERTDistNamesStr *caNames,
                    struct CERTCertificateStr **pRetCert,
                    struct SECKEYPrivateKeyStr **pRetKey) 
{

    CERTCertificate *  cert;
    SECKEYPrivateKey * privKey;
    char *             chosenNickName = (char *)arg;
    void *             proto_win      = NULL;
    SECStatus          secStatus      = SECFailure;

    proto_win = SSL_RevealPinArg(socket);

    if (chosenNickName) {
	cert = PK11_FindCertFromNickname(chosenNickName, proto_win);
	if (cert) {
	    privKey = PK11_FindKeyByAnyCert(cert, proto_win);
	    if (privKey) {
		secStatus = SECSuccess;
	    } else {
		CERT_DestroyCertificate(cert);
	    }
	}
    } else { 
	CERTCertNicknames *names;
	int                i;

	names = CERT_GetCertNicknames(CERT_GetDefaultCertDB(), 
				      SEC_CERT_NICKNAMES_USER, proto_win);

	if (names != NULL) {
	    for(i = 0; i < names->numnicknames; i++ ) {

		cert = PK11_FindCertFromNickname(names->nicknames[i], 
						 proto_win);
		if (!cert) {
		    continue;
		}

		
		if (CERT_CheckCertValidTimes(cert, PR_Now(), PR_FALSE)
		      != secCertTimeValid ) {
		    CERT_DestroyCertificate(cert);
		    continue;
		}

		secStatus = NSS_CmpCertChainWCANames(cert, caNames);
		if (secStatus == SECSuccess) {
		    privKey = PK11_FindKeyByAnyCert(cert, proto_win);
		    if (privKey) {
			break;
		    }
		    secStatus = SECFailure;
		}
		CERT_DestroyCertificate(cert);
	    } 
	    CERT_FreeNicknames(names);
	}
    }

    if (secStatus == SECSuccess) {
	*pRetCert = cert;
	*pRetKey  = privKey;
    }

    return secStatus;
}
























void
myHandshakeCallback(PRFileDesc *socket, void *arg) 
{
    fprintf(stderr,"Handshake Complete: SERVER CONFIGURED CORRECTLY\n");
}








void
disableAllSSLCiphers(void)
{
    const PRUint16 *cipherSuites = SSL_ImplementedCiphers;
    int             i            = SSL_NumImplementedCiphers;
    SECStatus       rv;

    
    while (--i >= 0) {
	PRUint16 suite = cipherSuites[i];
        rv = SSL_CipherPrefSetDefault(suite, PR_FALSE);
	if (rv != SECSuccess) {
	    fprintf(stderr,
		"SSL_CipherPrefSetDefault didn't like value 0x%04x (i = %d)\n",
	    	   suite, i);
	    errWarn("SSL_CipherPrefSetDefault");
	    exit(2);
	}
    }
}







void
errWarn(char *function)
{
    PRErrorCode  errorNumber = PR_GetError();
    const char * errorString = SECU_Strerror(errorNumber);

    fprintf(stderr, "Error in function %s: %d\n - %s\n",
		    function, errorNumber, errorString);
}

void
exitErr(char *function)
{
    errWarn(function);
    
    
    (void) NSS_Shutdown();
    PR_Cleanup();
    exit(1);
}

void 
printSecurityInfo(FILE *outfile, PRFileDesc *fd)
{
    char * cp;	
    char * ip;	
    char * sp;	
    int    op;	
    int    kp0;	
    int    kp1;	
    int    result;
    SSL3Statistics * ssl3stats = SSL_GetStatistics();

    if (!outfile) {
	outfile = stdout;
    }

    result = SSL_SecurityStatus(fd, &op, &cp, &kp0, &kp1, &ip, &sp);
    if (result != SECSuccess)
	return;
    fprintf(outfile,
     "   bulk cipher %s, %d secret key bits, %d key bits, status: %d\n"
     "   subject DN:\n %s\n"
     "   issuer  DN:\n %s\n", cp, kp1, kp0, op, sp, ip);
    PR_Free(cp);
    PR_Free(ip);
    PR_Free(sp);

    fprintf(outfile,
      "   %ld cache hits; %ld cache misses, %ld cache not reusable\n",
	    ssl3stats->hch_sid_cache_hits, ssl3stats->hch_sid_cache_misses,
    ssl3stats->hch_sid_cache_not_ok);

}






void
thread_wrapper(void * arg)
{
    GlobalThreadMgr *threadMGR = (GlobalThreadMgr *)arg;
    perThread *slot = &threadMGR->threads[threadMGR->index];

    
    PR_Lock(threadMGR->threadLock);
    PR_Unlock(threadMGR->threadLock);

    slot->rv = (* slot->startFunc)(slot->a, slot->b);

    PR_Lock(threadMGR->threadLock);
    slot->running = rs_zombie;

    
    PR_NotifyCondVar(threadMGR->threadEndQ);

    PR_Unlock(threadMGR->threadLock);
}

SECStatus
launch_thread(GlobalThreadMgr *threadMGR,
              startFn         *startFunc,
              void            *a,
              int              b)
{
    perThread *slot;
    int        i;

    if (!threadMGR->threadStartQ) {
	threadMGR->threadLock   = PR_NewLock();
	threadMGR->threadStartQ = PR_NewCondVar(threadMGR->threadLock);
	threadMGR->threadEndQ   = PR_NewCondVar(threadMGR->threadLock);
    }
    PR_Lock(threadMGR->threadLock);
    while (threadMGR->numRunning >= MAX_THREADS) {
	PR_WaitCondVar(threadMGR->threadStartQ, PR_INTERVAL_NO_TIMEOUT);
    }
    for (i = 0; i < threadMGR->numUsed; ++i) {
	slot = &threadMGR->threads[i];
	if (slot->running == rs_idle) 
	    break;
    }
    if (i >= threadMGR->numUsed) {
	if (i >= MAX_THREADS) {
	    
	    PORT_Assert(i < MAX_THREADS);
	    PR_Unlock(threadMGR->threadLock);
	    return SECFailure;
	}
	++(threadMGR->numUsed);
	PORT_Assert(threadMGR->numUsed == i + 1);
	slot = &threadMGR->threads[i];
    }

    slot->a = a;
    slot->b = b;
    slot->startFunc = startFunc;

    threadMGR->index = i;

    slot->prThread = PR_CreateThread(PR_USER_THREAD,
				     thread_wrapper, threadMGR,
				     PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
				     PR_JOINABLE_THREAD, 0);

    if (slot->prThread == NULL) {
	PR_Unlock(threadMGR->threadLock);
	printf("Failed to launch thread!\n");
	return SECFailure;
    } 

    slot->inUse   = 1;
    slot->running = 1;
    ++(threadMGR->numRunning);
    PR_Unlock(threadMGR->threadLock);

    return SECSuccess;
}

SECStatus 
reap_threads(GlobalThreadMgr *threadMGR)
{
    perThread * slot;
    int			i;

    if (!threadMGR->threadLock)
	return SECSuccess;
    PR_Lock(threadMGR->threadLock);
    while (threadMGR->numRunning > 0) {
	PR_WaitCondVar(threadMGR->threadEndQ, PR_INTERVAL_NO_TIMEOUT);
	for (i = 0; i < threadMGR->numUsed; ++i) {
	    slot = &threadMGR->threads[i];
	    if (slot->running == rs_zombie)  {
		

		
		PR_JoinThread(slot->prThread);
		slot->running = rs_idle;
		--threadMGR->numRunning;

		
		PR_NotifyCondVar(threadMGR->threadStartQ);
	    }
	}
    }

    
    for (i = 0; i < threadMGR->numUsed; ++i) {
	slot = &threadMGR->threads[i];
	if (slot->running != rs_idle)  {
	    fprintf(stderr, "Thread in slot %d is in state %d!\n", 
			     i, slot->running);
	}
    }
    PR_Unlock(threadMGR->threadLock);
    return SECSuccess;
}

void
destroy_thread_data(GlobalThreadMgr *threadMGR)
{
    PORT_Memset(threadMGR->threads, 0, sizeof(threadMGR->threads));

    if (threadMGR->threadEndQ) {
	PR_DestroyCondVar(threadMGR->threadEndQ);
	threadMGR->threadEndQ = NULL;
    }
    if (threadMGR->threadStartQ) {
	PR_DestroyCondVar(threadMGR->threadStartQ);
	threadMGR->threadStartQ = NULL;
    }
    if (threadMGR->threadLock) {
	PR_DestroyLock(threadMGR->threadLock);
	threadMGR->threadLock = NULL;
    }
}





void 
lockedVars_Init( lockedVars * lv)
{
    lv->count	= 0;
    lv->waiters = 0;
    lv->lock	= PR_NewLock();
    lv->condVar = PR_NewCondVar(lv->lock);
}

void
lockedVars_Destroy( lockedVars * lv)
{
    PR_DestroyCondVar(lv->condVar);
    lv->condVar = NULL;

    PR_DestroyLock(lv->lock);
    lv->lock = NULL;
}

void
lockedVars_WaitForDone(lockedVars * lv)
{
    PR_Lock(lv->lock);
    while (lv->count > 0) {
	PR_WaitCondVar(lv->condVar, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_Unlock(lv->lock);
}

int	
lockedVars_AddToCount(lockedVars * lv, int addend)
{
    int rv;

    PR_Lock(lv->lock);
    rv = lv->count += addend;
    if (rv <= 0) {
	PR_NotifyCondVar(lv->condVar);
    }
    PR_Unlock(lv->lock);
    return rv;
}









void
dumpCertChain(CERTCertificate *cert, SECCertUsage usage)
{
    CERTCertificateList *certList;
    int count = 0;

    certList = CERT_CertChainFromCert(cert, usage, PR_TRUE);
    if (certList == NULL) {
        errWarn("CERT_CertChainFromCert");
        return;
    }

    for(count = 0; count < (unsigned int)certList->len; count++) {
        char certFileName[16];
        PRFileDesc *cfd;

        PR_snprintf(certFileName, sizeof certFileName, "cert.%03d",
                    count);
        cfd = PR_Open(certFileName, PR_WRONLY|PR_CREATE_FILE|PR_TRUNCATE, 
                      0664);
        if (!cfd) {
            PR_fprintf(PR_STDOUT,
                       "Error: couldn't save cert der in file '%s'\n",
                       certFileName);
        } else {
            PR_Write(cfd,  certList->certs[count].data,  certList->certs[count].len);
            PR_Close(cfd);
            PR_fprintf(PR_STDOUT, "Cert file %s was created.\n", certFileName);
        }
    }
    CERT_DestroyCertificateList(certList);
}
