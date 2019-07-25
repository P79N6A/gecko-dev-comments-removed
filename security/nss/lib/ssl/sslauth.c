



































#include "cert.h"
#include "secitem.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"
#include "pk11func.h"


CERTCertificate *
SSL_PeerCertificate(PRFileDesc *fd)
{
    sslSocket *ss;

    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in PeerCertificate",
		 SSL_GETPID(), fd));
	return 0;
    }
    if (ss->opt.useSecurity && ss->sec.peerCert) {
	return CERT_DupCertificate(ss->sec.peerCert);
    }
    return 0;
}


CERTCertificate *
SSL_LocalCertificate(PRFileDesc *fd)
{
    sslSocket *ss;

    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in PeerCertificate",
		 SSL_GETPID(), fd));
	return NULL;
    }
    if (ss->opt.useSecurity) {
    	if (ss->sec.localCert) {
	    return CERT_DupCertificate(ss->sec.localCert);
	}
	if (ss->sec.ci.sid && ss->sec.ci.sid->localCert) {
	    return CERT_DupCertificate(ss->sec.ci.sid->localCert);
	}
    }
    return NULL;
}




SECStatus
SSL_SecurityStatus(PRFileDesc *fd, int *op, char **cp, int *kp0, int *kp1,
		   char **ip, char **sp)
{
    sslSocket *ss;
    const char *cipherName;
    PRBool isDes = PR_FALSE;
    PRBool enoughFirstHsDone = PR_FALSE;

    ss = ssl_FindSocket(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in SecurityStatus",
		 SSL_GETPID(), fd));
	return SECFailure;
    }

    if (cp) *cp = 0;
    if (kp0) *kp0 = 0;
    if (kp1) *kp1 = 0;
    if (ip) *ip = 0;
    if (sp) *sp = 0;
    if (op) {
	*op = SSL_SECURITY_STATUS_OFF;
    }

    if (ss->firstHsDone) {
	enoughFirstHsDone = PR_TRUE;
    } else if (ss->version >= SSL_LIBRARY_VERSION_3_0 &&
	       ssl3_CanFalseStart(ss)) {
	enoughFirstHsDone = PR_TRUE;
    }

    if (ss->opt.useSecurity && enoughFirstHsDone) {
	if (ss->version < SSL_LIBRARY_VERSION_3_0) {
	    cipherName = ssl_cipherName[ss->sec.cipherType];
	} else {
	    cipherName = ssl3_cipherName[ss->sec.cipherType];
	}
	PORT_Assert(cipherName);
	if (cipherName) {
            if (PORT_Strstr(cipherName, "DES")) isDes = PR_TRUE;

            if (cp) {
                *cp = PORT_Strdup(cipherName);
            }
        }

	if (kp0) {
	    *kp0 = ss->sec.keyBits;
	    if (isDes) *kp0 = (*kp0 * 7) / 8;
	}
	if (kp1) {
	    *kp1 = ss->sec.secretKeyBits;
	    if (isDes) *kp1 = (*kp1 * 7) / 8;
	}
	if (op) {
	    if (ss->sec.keyBits == 0) {
		*op = SSL_SECURITY_STATUS_OFF;
	    } else if (ss->sec.secretKeyBits < 90) {
		*op = SSL_SECURITY_STATUS_ON_LOW;

	    } else {
		*op = SSL_SECURITY_STATUS_ON_HIGH;
	    }
	}

	if (ip || sp) {
	    CERTCertificate *cert;

	    cert = ss->sec.peerCert;
	    if (cert) {
		if (ip) {
		    *ip = CERT_NameToAscii(&cert->issuer);
		}
		if (sp) {
		    *sp = CERT_NameToAscii(&cert->subject);
		}
	    } else {
		if (ip) {
		    *ip = PORT_Strdup("no certificate");
		}
		if (sp) {
		    *sp = PORT_Strdup("no certificate");
		}
	    }
	}
    }

    return SECSuccess;
}




SECStatus
SSL_AuthCertificateHook(PRFileDesc *s, SSLAuthCertificate func, void *arg)
{
    sslSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in AuthCertificateHook",
		 SSL_GETPID(), s));
	return SECFailure;
    }

    ss->authCertificate = func;
    ss->authCertificateArg = arg;

    return SECSuccess;
}


SECStatus 
SSL_GetClientAuthDataHook(PRFileDesc *s, SSLGetClientAuthData func,
			      void *arg)
{
    sslSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in GetClientAuthDataHook",
		 SSL_GETPID(), s));
	return SECFailure;
    }

    ss->getClientAuthData = func;
    ss->getClientAuthDataArg = arg;
    return SECSuccess;
}


SECStatus 
SSL_SetPKCS11PinArg(PRFileDesc *s, void *arg)
{
    sslSocket *ss;

    ss = ssl_FindSocket(s);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in GetClientAuthDataHook",
		 SSL_GETPID(), s));
	return SECFailure;
    }

    ss->pkcs11PinArg = arg;
    return SECSuccess;
}






SECStatus
SSL_AuthCertificate(void *arg, PRFileDesc *fd, PRBool checkSig, PRBool isServer)
{
    SECStatus          rv;
    CERTCertDBHandle * handle;
    sslSocket *        ss;
    SECCertUsage       certUsage;
    const char *             hostname    = NULL;
    
    ss = ssl_FindSocket(fd);
    PORT_Assert(ss != NULL);
    if (!ss) {
	return SECFailure;
    }

    handle = (CERTCertDBHandle *)arg;

    
    certUsage = isServer ? certUsageSSLClient : certUsageSSLServer;

    rv = CERT_VerifyCertNow(handle, ss->sec.peerCert, checkSig, certUsage,
			    ss->pkcs11PinArg);

    if ( rv != SECSuccess || isServer )
	return rv;
  
    



    hostname = ss->url;
    if (hostname && hostname[0])
	rv = CERT_VerifyCertName(ss->sec.peerCert, hostname);
    else 
	rv = SECFailure;
    if (rv != SECSuccess)
	PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);

    return rv;
}
