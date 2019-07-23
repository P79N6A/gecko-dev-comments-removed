








































#include "nssrenam.h"
#include "cert.h"
#include "secitem.h"
#include "sechash.h"
#include "cryptohi.h"		
#include "keyhi.h" 		
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"
#include "ssl3prot.h"
#include "sslerr.h"
#include "pk11func.h"
#include "prinit.h"
#include "prtime.h" 	

#define XXX
static PRBool policyWasSet;



static const PRUint8 allCipherSuites[] = {
    0,						0,    0,
    SSL_CK_RC4_128_WITH_MD5,			0x00, 0x80,
    SSL_CK_RC4_128_EXPORT40_WITH_MD5,		0x00, 0x80,
    SSL_CK_RC2_128_CBC_WITH_MD5,		0x00, 0x80,
    SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5,	0x00, 0x80,
    SSL_CK_IDEA_128_CBC_WITH_MD5,		0x00, 0x80,
    SSL_CK_DES_64_CBC_WITH_MD5,			0x00, 0x40,
    SSL_CK_DES_192_EDE3_CBC_WITH_MD5,		0x00, 0xC0,
    0,						0,    0
};

#define ssl2_NUM_SUITES_IMPLEMENTED 6






static const PRUint8 implementedCipherSuites[ssl2_NUM_SUITES_IMPLEMENTED * 3] = {
    SSL_CK_RC4_128_WITH_MD5,			0x00, 0x80,
    SSL_CK_RC2_128_CBC_WITH_MD5,		0x00, 0x80,
    SSL_CK_DES_192_EDE3_CBC_WITH_MD5,		0x00, 0xC0,
    SSL_CK_DES_64_CBC_WITH_MD5,			0x00, 0x40,
    SSL_CK_RC4_128_EXPORT40_WITH_MD5,		0x00, 0x80,
    SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5,	0x00, 0x80
};

typedef struct ssl2SpecsStr {
    PRUint8           nkm; 
    PRUint8           nkd; 
    PRUint8           blockSize;
    PRUint8           blockShift;
    CK_MECHANISM_TYPE mechanism;
    PRUint8           keyLen;	
    PRUint8           pubLen;	
    PRUint8           ivLen;	
} ssl2Specs;

static const ssl2Specs ssl_Specs[] = {
 
				{  0,  0, 0, 0, },
 
				{  2, 16, 1, 0, CKM_RC4,       16,   0, 0, },
 
				{  2, 16, 1, 0, CKM_RC4,       16,  11, 0, },
 
				{  2, 16, 8, 3, CKM_RC2_CBC,   16,   0, 8, },
 
				{  2, 16, 8, 3, CKM_RC2_CBC,   16,  11, 8, },
 
				{  0,  0, 0, 0, },
 
				{  1,  8, 8, 3, CKM_DES_CBC,    8,   0, 8, },
 
				{  3, 24, 8, 3, CKM_DES3_CBC,  24,   0, 8, },
};

#define SET_ERROR_CODE
#define TEST_FOR_FAILURE





const char *ssl_version = "SECURITY_VERSION:"
			" +us"
			" +export"
#ifdef TRACE
			" +trace"
#endif
#ifdef DEBUG
			" +debug"
#endif
			;

const char * const ssl_cipherName[] = {
    "unknown",
    "RC4",
    "RC4-Export",
    "RC2-CBC",
    "RC2-CBC-Export",
    "IDEA-CBC",
    "DES-CBC",
    "DES-EDE3-CBC",
    "unknown",
    "unknown", 
};





static PRUint16	allowedByPolicy;          
static PRUint16	maybeAllowedByPolicy;     
static PRUint16	chosenPreference = 0xff;  


#define SSL_CB_RC4_128_WITH_MD5              (1 << SSL_CK_RC4_128_WITH_MD5)
#define SSL_CB_RC4_128_EXPORT40_WITH_MD5     (1 << SSL_CK_RC4_128_EXPORT40_WITH_MD5)
#define SSL_CB_RC2_128_CBC_WITH_MD5          (1 << SSL_CK_RC2_128_CBC_WITH_MD5)
#define SSL_CB_RC2_128_CBC_EXPORT40_WITH_MD5 (1 << SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5)
#define SSL_CB_IDEA_128_CBC_WITH_MD5         (1 << SSL_CK_IDEA_128_CBC_WITH_MD5)
#define SSL_CB_DES_64_CBC_WITH_MD5           (1 << SSL_CK_DES_64_CBC_WITH_MD5)
#define SSL_CB_DES_192_EDE3_CBC_WITH_MD5     (1 << SSL_CK_DES_192_EDE3_CBC_WITH_MD5)
#define SSL_CB_IMPLEMENTED \
   (SSL_CB_RC4_128_WITH_MD5              | \
    SSL_CB_RC4_128_EXPORT40_WITH_MD5     | \
    SSL_CB_RC2_128_CBC_WITH_MD5          | \
    SSL_CB_RC2_128_CBC_EXPORT40_WITH_MD5 | \
    SSL_CB_DES_64_CBC_WITH_MD5           | \
    SSL_CB_DES_192_EDE3_CBC_WITH_MD5)




static SECStatus
ssl2_ConstructCipherSpecs(sslSocket *ss) 
{
    PRUint8 *	        cs		= NULL;
    unsigned int	allowed;
    unsigned int	count;
    int 		ssl3_count	= 0;
    int 		final_count;
    int 		i;
    SECStatus 		rv;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    count = 0;
    PORT_Assert(ss != 0);
    allowed = !ss->opt.enableSSL2 ? 0 :
    	(ss->allowedByPolicy & ss->chosenPreference & SSL_CB_IMPLEMENTED);
    while (allowed) {
    	if (allowed & 1) 
	    ++count;
	allowed >>= 1;
    }

    




    ssl3_config_match_init(ss);

    
    rv = ssl3_ConstructV2CipherSpecsHack(ss, NULL, &ssl3_count);
    if (rv < 0) 
	return rv;
    count += ssl3_count;

    
    if (count > 0)
	cs = (PRUint8*) PORT_Alloc(count * 3);
    else
	PORT_SetError(SSL_ERROR_SSL_DISABLED);
    if (cs == NULL)
    	return SECFailure;

    if (ss->cipherSpecs != NULL) {
	PORT_Free(ss->cipherSpecs);
    }
    ss->cipherSpecs     = cs;
    ss->sizeCipherSpecs = count * 3;

    
    allowed = !ss->opt.enableSSL2 ? 0 :
    	(ss->allowedByPolicy & ss->chosenPreference & SSL_CB_IMPLEMENTED);
    for (i = 0; i < ssl2_NUM_SUITES_IMPLEMENTED * 3; i += 3) {
	const PRUint8 * hs = implementedCipherSuites + i;
	int             ok = allowed & (1U << hs[0]);
	if (ok) {
	    cs[0] = hs[0];
	    cs[1] = hs[1];
	    cs[2] = hs[2];
	    cs += 3;
	}
    }

    
    rv = ssl3_ConstructV2CipherSpecsHack(ss, cs, &final_count);
    
    
    ss->sizeCipherSpecs -= (ssl3_count - final_count) * 3;

    return rv;
}









static SECStatus
ssl2_CheckConfigSanity(sslSocket *ss)
{
    unsigned int      allowed;
    int               ssl3CipherCount = 0;
    SECStatus         rv;

    


    if (!ss->cipherSpecs)
    	goto disabled;

    allowed = ss->allowedByPolicy & ss->chosenPreference;
    if (! allowed)
	ss->opt.enableSSL2 = PR_FALSE; 

    
    
    rv = ssl3_ConstructV2CipherSpecsHack(ss, NULL, &ssl3CipherCount);
    if (rv != SECSuccess || ssl3CipherCount <= 0) {
	ss->opt.enableSSL3 = PR_FALSE; 
	ss->opt.enableTLS  = PR_FALSE;
    }

    if (!ss->opt.enableSSL2 && !ss->opt.enableSSL3 && !ss->opt.enableTLS) {
	SSL_DBG(("%d: SSL[%d]: Can't handshake! both v2 and v3 disabled.",
		 SSL_GETPID(), ss->fd));
disabled:
	PORT_SetError(SSL_ERROR_SSL_DISABLED);
	return SECFailure;
    }
    return SECSuccess;
}





SECStatus
ssl2_SetPolicy(PRInt32 which, PRInt32 policy)
{
    PRUint32  bitMask;
    SECStatus rv       = SECSuccess;

    which &= 0x000f;
    bitMask = 1 << which;

    if (!(bitMask & SSL_CB_IMPLEMENTED)) {
    	PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
    	return SECFailure;
    }

    if (policy == SSL_ALLOWED) {
	allowedByPolicy 	|= bitMask;
	maybeAllowedByPolicy 	|= bitMask;
    } else if (policy == SSL_RESTRICTED) {
    	allowedByPolicy 	&= ~bitMask;
	maybeAllowedByPolicy 	|= bitMask;
    } else {
    	allowedByPolicy 	&= ~bitMask;
    	maybeAllowedByPolicy 	&= ~bitMask;
    }
    allowedByPolicy 		&= SSL_CB_IMPLEMENTED;
    maybeAllowedByPolicy 	&= SSL_CB_IMPLEMENTED;

    policyWasSet = PR_TRUE;
    return rv;
}

SECStatus
ssl2_GetPolicy(PRInt32 which, PRInt32 *oPolicy)
{
    PRUint32     bitMask;
    PRInt32      policy;

    which &= 0x000f;
    bitMask = 1 << which;

    
    if (!(bitMask & SSL_CB_IMPLEMENTED)) {
    	PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
	*oPolicy = SSL_NOT_ALLOWED;
    	return SECFailure;
    }

    if (maybeAllowedByPolicy & bitMask) {
    	policy = (allowedByPolicy & bitMask) ? SSL_ALLOWED : SSL_RESTRICTED;
    } else {
	policy = SSL_NOT_ALLOWED;
    }

    *oPolicy = policy;
    return SECSuccess;
}







SECStatus
ssl2_CipherPrefSetDefault(PRInt32 which, PRBool enabled)
{
    PRUint32     bitMask;
    
    which &= 0x000f;
    bitMask = 1 << which;

    if (!(bitMask & SSL_CB_IMPLEMENTED)) {
    	PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
    	return SECFailure;
    }

    if (enabled)
	chosenPreference |= bitMask;
    else
    	chosenPreference &= ~bitMask;
    chosenPreference &= SSL_CB_IMPLEMENTED;

    return SECSuccess;
}

SECStatus 
ssl2_CipherPrefGetDefault(PRInt32 which, PRBool *enabled)
{
    PRBool       rv       = PR_FALSE;
    PRUint32     bitMask;

    which &= 0x000f;
    bitMask = 1 << which;

    if (!(bitMask & SSL_CB_IMPLEMENTED)) {
    	PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
	*enabled = PR_FALSE;
    	return SECFailure;
    }

    rv = (PRBool)((chosenPreference & bitMask) != 0);
    *enabled = rv;
    return SECSuccess;
}

SECStatus 
ssl2_CipherPrefSet(sslSocket *ss, PRInt32 which, PRBool enabled)
{
    PRUint32     bitMask;
    
    which &= 0x000f;
    bitMask = 1 << which;

    if (!(bitMask & SSL_CB_IMPLEMENTED)) {
    	PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
    	return SECFailure;
    }

    if (enabled)
	ss->chosenPreference |= bitMask;
    else
    	ss->chosenPreference &= ~bitMask;
    ss->chosenPreference &= SSL_CB_IMPLEMENTED;

    return SECSuccess;
}

SECStatus 
ssl2_CipherPrefGet(sslSocket *ss, PRInt32 which, PRBool *enabled)
{
    PRBool       rv       = PR_FALSE;
    PRUint32     bitMask;

    which &= 0x000f;
    bitMask = 1 << which;

    if (!(bitMask & SSL_CB_IMPLEMENTED)) {
    	PORT_SetError(SSL_ERROR_UNKNOWN_CIPHER_SUITE);
	*enabled = PR_FALSE;
    	return SECFailure;
    }

    rv = (PRBool)((ss->chosenPreference & bitMask) != 0);
    *enabled = rv;
    return SECSuccess;
}



void      
ssl2_InitSocketPolicy(sslSocket *ss)
{
    ss->allowedByPolicy		= allowedByPolicy;
    ss->maybeAllowedByPolicy	= maybeAllowedByPolicy;
    ss->chosenPreference 	= chosenPreference;
}






static SECStatus
ssl2_CreateMAC(sslSecurityInfo *sec, SECItem *readKey, SECItem *writeKey, 
          int cipherChoice)
{
    switch (cipherChoice) {

      case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
      case SSL_CK_RC2_128_CBC_WITH_MD5:
      case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
      case SSL_CK_RC4_128_WITH_MD5:
      case SSL_CK_DES_64_CBC_WITH_MD5:
      case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	sec->hash = HASH_GetHashObject(HASH_AlgMD5);
	SECITEM_CopyItem(0, &sec->sendSecret, writeKey);
	SECITEM_CopyItem(0, &sec->rcvSecret, readKey);
	break;

      default:
	PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	return SECFailure;
    }
    sec->hashcx = (*sec->hash->create)();
    if (sec->hashcx == NULL)
	return SECFailure;
    return SECSuccess;
}







static SECStatus
ssl2_GetSendBuffer(sslSocket *ss, unsigned int len)
{
    SECStatus rv = SECSuccess;

    PORT_Assert(ss->opt.noLocks || ssl_HaveXmitBufLock(ss));

    if (len < 128) {
	len = 128;
    }
    if (len > ss->sec.ci.sendBuf.space) {
	rv = sslBuffer_Grow(&ss->sec.ci.sendBuf, len);
	if (rv != SECSuccess) {
	    SSL_DBG(("%d: SSL[%d]: ssl2_GetSendBuffer failed, tried to get %d bytes",
		     SSL_GETPID(), ss->fd, len));
	    rv = SECFailure;
	}
    }
    return rv;
}













	
int
ssl2_SendErrorMessage(sslSocket *ss, int error)
{
    int rv;
    PRUint8 msg[SSL_HL_ERROR_HBYTES];

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    msg[0] = SSL_MT_ERROR;
    msg[1] = MSB(error);
    msg[2] = LSB(error);

    ssl_GetXmitBufLock(ss);    

    SSL_TRC(3, ("%d: SSL[%d]: sending error %d", SSL_GETPID(), ss->fd, error));

    ss->handshakeBegun = 1;
    rv = (*ss->sec.send)(ss, msg, sizeof(msg), 0);
    if (rv >= 0) {
	rv = SECSuccess;
    }
    ssl_ReleaseXmitBufLock(ss);    
    return rv;
}




static SECStatus
ssl2_SendClientFinishedMessage(sslSocket *ss)
{
    SECStatus        rv    = SECSuccess;
    int              sent;
    PRUint8    msg[1 + SSL_CONNECTIONID_BYTES];

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetXmitBufLock(ss);    

    if (ss->sec.ci.sentFinished == 0) {
	ss->sec.ci.sentFinished = 1;

	SSL_TRC(3, ("%d: SSL[%d]: sending client-finished",
		    SSL_GETPID(), ss->fd));

	msg[0] = SSL_MT_CLIENT_FINISHED;
	PORT_Memcpy(msg+1, ss->sec.ci.connectionID, 
	            sizeof(ss->sec.ci.connectionID));

	DUMP_MSG(29, (ss, msg, 1 + sizeof(ss->sec.ci.connectionID)));
	sent = (*ss->sec.send)(ss, msg, 1 + sizeof(ss->sec.ci.connectionID), 0);
	rv = (sent >= 0) ? SECSuccess : (SECStatus)sent;
    }
    ssl_ReleaseXmitBufLock(ss);    
    return rv;
}







static SECStatus
ssl2_SendServerVerifyMessage(sslSocket *ss)
{
    PRUint8 *        msg;
    int              sendLen;
    int              sent;
    SECStatus        rv;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetXmitBufLock(ss);    

    sendLen = 1 + SSL_CHALLENGE_BYTES;
    rv = ssl2_GetSendBuffer(ss, sendLen);
    if (rv != SECSuccess) {
	goto done;
    }

    msg = ss->sec.ci.sendBuf.buf;
    msg[0] = SSL_MT_SERVER_VERIFY;
    PORT_Memcpy(msg+1, ss->sec.ci.clientChallenge, SSL_CHALLENGE_BYTES);

    DUMP_MSG(29, (ss, msg, sendLen));
    sent = (*ss->sec.send)(ss, msg, sendLen, 0);

    rv = (sent >= 0) ? SECSuccess : (SECStatus)sent;

done:
    ssl_ReleaseXmitBufLock(ss);    
    return rv;
}




static SECStatus
ssl2_SendServerFinishedMessage(sslSocket *ss)
{
    sslSessionID *   sid;
    PRUint8 *        msg;
    int              sendLen, sent;
    SECStatus        rv    = SECSuccess;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetXmitBufLock(ss);    

    if (ss->sec.ci.sentFinished == 0) {
	ss->sec.ci.sentFinished = 1;
	PORT_Assert(ss->sec.ci.sid != 0);
	sid = ss->sec.ci.sid;

	SSL_TRC(3, ("%d: SSL[%d]: sending server-finished",
		    SSL_GETPID(), ss->fd));

	sendLen = 1 + sizeof(sid->u.ssl2.sessionID);
	rv = ssl2_GetSendBuffer(ss, sendLen);
	if (rv != SECSuccess) {
	    goto done;
	}

	msg = ss->sec.ci.sendBuf.buf;
	msg[0] = SSL_MT_SERVER_FINISHED;
	PORT_Memcpy(msg+1, sid->u.ssl2.sessionID,
		    sizeof(sid->u.ssl2.sessionID));

	DUMP_MSG(29, (ss, msg, sendLen));
	sent = (*ss->sec.send)(ss, msg, sendLen, 0);

	if (sent < 0) {
	    
	    (*ss->sec.uncache)(sid);
	    rv = (SECStatus)sent;
	} else if (!ss->opt.noCache) {
	    
	    (*ss->sec.cache)(sid);
	    rv = SECSuccess;
	}
	ssl_FreeSID(sid);
	ss->sec.ci.sid = 0;
    }
done:
    ssl_ReleaseXmitBufLock(ss);    
    return rv;
}






static SECStatus
ssl2_SendSessionKeyMessage(sslSocket *ss, int cipher, int keySize,
		      PRUint8 *ca, int caLen,
		      PRUint8 *ck, int ckLen,
		      PRUint8 *ek, int ekLen)
{
    PRUint8 *        msg;
    int              sendLen;
    int              sent;
    SECStatus        rv;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetXmitBufLock(ss);    

    sendLen = SSL_HL_CLIENT_MASTER_KEY_HBYTES + ckLen + ekLen + caLen;
    rv = ssl2_GetSendBuffer(ss, sendLen);
    if (rv != SECSuccess) 
	goto done;

    SSL_TRC(3, ("%d: SSL[%d]: sending client-session-key",
		SSL_GETPID(), ss->fd));

    msg = ss->sec.ci.sendBuf.buf;
    msg[0] = SSL_MT_CLIENT_MASTER_KEY;
    msg[1] = cipher;
    msg[2] = MSB(keySize);
    msg[3] = LSB(keySize);
    msg[4] = MSB(ckLen);
    msg[5] = LSB(ckLen);
    msg[6] = MSB(ekLen);
    msg[7] = LSB(ekLen);
    msg[8] = MSB(caLen);
    msg[9] = LSB(caLen);
    PORT_Memcpy(msg+SSL_HL_CLIENT_MASTER_KEY_HBYTES, ck, ckLen);
    PORT_Memcpy(msg+SSL_HL_CLIENT_MASTER_KEY_HBYTES+ckLen, ek, ekLen);
    PORT_Memcpy(msg+SSL_HL_CLIENT_MASTER_KEY_HBYTES+ckLen+ekLen, ca, caLen);

    DUMP_MSG(29, (ss, msg, sendLen));
    sent = (*ss->sec.send)(ss, msg, sendLen, 0);
    rv = (sent >= 0) ? SECSuccess : (SECStatus)sent;
done:
    ssl_ReleaseXmitBufLock(ss);    
    return rv;
}




static SECStatus
ssl2_SendCertificateRequestMessage(sslSocket *ss)
{
    PRUint8 *        msg;
    int              sent;
    int              sendLen;
    SECStatus        rv;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetXmitBufLock(ss);    

    sendLen = SSL_HL_REQUEST_CERTIFICATE_HBYTES + SSL_CHALLENGE_BYTES;
    rv = ssl2_GetSendBuffer(ss, sendLen);
    if (rv != SECSuccess) 
	goto done;

    SSL_TRC(3, ("%d: SSL[%d]: sending certificate request",
		SSL_GETPID(), ss->fd));

    
    PK11_GenerateRandom(ss->sec.ci.serverChallenge, SSL_CHALLENGE_BYTES);

    msg = ss->sec.ci.sendBuf.buf;
    msg[0] = SSL_MT_REQUEST_CERTIFICATE;
    msg[1] = SSL_AT_MD5_WITH_RSA_ENCRYPTION;
    PORT_Memcpy(msg + SSL_HL_REQUEST_CERTIFICATE_HBYTES, 
                ss->sec.ci.serverChallenge, SSL_CHALLENGE_BYTES);

    DUMP_MSG(29, (ss, msg, sendLen));
    sent = (*ss->sec.send)(ss, msg, sendLen, 0);
    rv = (sent >= 0) ? SECSuccess : (SECStatus)sent;
done:
    ssl_ReleaseXmitBufLock(ss);    
    return rv;
}





static int
ssl2_SendCertificateResponseMessage(sslSocket *ss, SECItem *cert, 
                                    SECItem *encCode)
{
    PRUint8 *msg;
    int rv, sendLen;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetXmitBufLock(ss);    

    sendLen = SSL_HL_CLIENT_CERTIFICATE_HBYTES + encCode->len + cert->len;
    rv = ssl2_GetSendBuffer(ss, sendLen);
    if (rv) 
    	goto done;

    SSL_TRC(3, ("%d: SSL[%d]: sending certificate response",
		SSL_GETPID(), ss->fd));

    msg = ss->sec.ci.sendBuf.buf;
    msg[0] = SSL_MT_CLIENT_CERTIFICATE;
    msg[1] = SSL_CT_X509_CERTIFICATE;
    msg[2] = MSB(cert->len);
    msg[3] = LSB(cert->len);
    msg[4] = MSB(encCode->len);
    msg[5] = LSB(encCode->len);
    PORT_Memcpy(msg + SSL_HL_CLIENT_CERTIFICATE_HBYTES, cert->data, cert->len);
    PORT_Memcpy(msg + SSL_HL_CLIENT_CERTIFICATE_HBYTES + cert->len,
	      encCode->data, encCode->len);

    DUMP_MSG(29, (ss, msg, sendLen));
    rv = (*ss->sec.send)(ss, msg, sendLen, 0);
    if (rv >= 0) {
	rv = SECSuccess;
    }
done:
    ssl_ReleaseXmitBufLock(ss);    
    return rv;
}











static SECStatus
ssl2_CalcMAC(PRUint8             * result, 
	     sslSecurityInfo     * sec,
	     const PRUint8       * data, 
	     unsigned int          dataLen,
	     unsigned int          paddingLen)
{
    const PRUint8 *      secret		= sec->sendSecret.data;
    unsigned int         secretLen	= sec->sendSecret.len;
    unsigned long        sequenceNumber = sec->sendSequence;
    unsigned int         nout;
    PRUint8              seq[4];
    PRUint8              padding[32];

    if (!sec->hash || !sec->hash->length)
    	return SECSuccess;
    if (!sec->hashcx)
    	return SECFailure;

    
    (*sec->hash->begin)(sec->hashcx);

    
    (*sec->hash->update)(sec->hashcx, secret, secretLen);
    (*sec->hash->update)(sec->hashcx, data, dataLen);
    PORT_Memset(padding, paddingLen, paddingLen);
    (*sec->hash->update)(sec->hashcx, padding, paddingLen);

    seq[0] = (PRUint8) (sequenceNumber >> 24);
    seq[1] = (PRUint8) (sequenceNumber >> 16);
    seq[2] = (PRUint8) (sequenceNumber >> 8);
    seq[3] = (PRUint8) (sequenceNumber);

    PRINT_BUF(60, (0, "calc-mac secret:", secret, secretLen));
    PRINT_BUF(60, (0, "calc-mac data:", data, dataLen));
    PRINT_BUF(60, (0, "calc-mac padding:", padding, paddingLen));
    PRINT_BUF(60, (0, "calc-mac seq:", seq, 4));

    (*sec->hash->update)(sec->hashcx, seq, 4);

    
    (*sec->hash->end)(sec->hashcx, result, &nout, sec->hash->length);

    return SECSuccess;
}







#define MAX_STREAM_CYPHER_LEN	0x7fe0
#define MAX_BLOCK_CYPHER_LEN	0x3fe0







static PRInt32 
ssl2_SendClear(sslSocket *ss, const PRUint8 *in, PRInt32 len, PRInt32 flags)
{
    PRUint8         * out;
    int               rv;
    int               amount;
    int               count	= 0;

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    SSL_TRC(10, ("%d: SSL[%d]: sending %d bytes in the clear",
		 SSL_GETPID(), ss->fd, len));
    PRINT_BUF(50, (ss, "clear data:", (PRUint8*) in, len));

    while (len) {
	amount = PR_MIN( len, MAX_STREAM_CYPHER_LEN );
	if (amount + 2 > ss->sec.writeBuf.space) {
	    rv = sslBuffer_Grow(&ss->sec.writeBuf, amount + 2);
	    if (rv != SECSuccess) {
		count = rv;
		break;
	    }
	}
	out = ss->sec.writeBuf.buf;

	


	out[0] = 0x80 | MSB(amount);
	out[1] = LSB(amount);
	PORT_Memcpy(&out[2], in, amount);

	
	rv = ssl_DefSend(ss, out, amount + 2, flags & ~ssl_SEND_FLAG_MASK);
	if (rv < 0) {
	    if (PORT_GetError() == PR_WOULD_BLOCK_ERROR) {
		rv = 0;
	    } else {
		
		if (count == 0)
		    count = rv;
		break;
	    }
	}

	if ((unsigned)rv < (amount + 2)) {
	    
	    if (ssl_SaveWriteData(ss, out + rv, amount + 2 - rv) 
	        == SECFailure) {
		count = SECFailure;
	    } else {
		count += amount;
		ss->sec.sendSequence++;
	    }
	    break;
	}

	ss->sec.sendSequence++;
	in    += amount;
	count += amount;
	len   -= amount;
    }

    return count;
}






static PRInt32 
ssl2_SendStream(sslSocket *ss, const PRUint8 *in, PRInt32 len, PRInt32 flags)
{
    PRUint8       *  out;
    int              rv;
    int              count	= 0;

    int              amount;
    PRUint8          macLen;
    int              nout;
    int              buflen;

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    SSL_TRC(10, ("%d: SSL[%d]: sending %d bytes using stream cipher",
		 SSL_GETPID(), ss->fd, len));
    PRINT_BUF(50, (ss, "clear data:", (PRUint8*) in, len));

    while (len) {
	ssl_GetSpecReadLock(ss);  

	macLen = ss->sec.hash->length;
	amount = PR_MIN( len, MAX_STREAM_CYPHER_LEN );
	buflen = amount + 2 + macLen;
	if (buflen > ss->sec.writeBuf.space) {
	    rv = sslBuffer_Grow(&ss->sec.writeBuf, buflen);
	    if (rv != SECSuccess) {
		goto loser;
	    }
	}
	out    = ss->sec.writeBuf.buf;
	nout   = amount + macLen;
	out[0] = 0x80 | MSB(nout);
	out[1] = LSB(nout);

	
	rv = ssl2_CalcMAC(out+2, 		
	                  &ss->sec, 
		          in, amount, 		
			  0); 			
	if (rv != SECSuccess) 
	    goto loser;

	
	rv = (*ss->sec.enc)(ss->sec.writecx, out+2, &nout, macLen, out+2, macLen);
	if (rv) goto loser;

	
	rv = (*ss->sec.enc)(ss->sec.writecx, out+2+macLen, &nout, amount, in, amount);
	if (rv) goto loser;

	ssl_ReleaseSpecReadLock(ss);  

	PRINT_BUF(50, (ss, "encrypted data:", out, buflen));

	rv = ssl_DefSend(ss, out, buflen, flags & ~ssl_SEND_FLAG_MASK);
	if (rv < 0) {
	    if (PORT_GetError() == PR_WOULD_BLOCK_ERROR) {
		SSL_TRC(50, ("%d: SSL[%d]: send stream would block, "
			     "saving data", SSL_GETPID(), ss->fd));
		rv = 0;
	    } else {
		SSL_TRC(10, ("%d: SSL[%d]: send stream error %d",
			     SSL_GETPID(), ss->fd, PORT_GetError()));
		
		if (count == 0)
		    count = rv;
		goto done;
	    }
	}

	if ((unsigned)rv < buflen) {
	    
	    if (ssl_SaveWriteData(ss, out + rv, buflen - rv) == SECFailure) {
		count = SECFailure;
	    } else {
	    	count += amount;
		ss->sec.sendSequence++;
	    }
	    goto done;
	}

	ss->sec.sendSequence++;
	in    += amount;
	count += amount;
	len   -= amount;
    }

done:
    return count;

loser:
    ssl_ReleaseSpecReadLock(ss);
    return SECFailure;
}






static PRInt32
ssl2_SendBlock(sslSocket *ss, const PRUint8 *in, PRInt32 len, PRInt32 flags)
{
    PRUint8       *  out;  		    
    PRUint8       *  op;		    
    int              rv;		    
    int              count	= 0;        

    unsigned int     hlen;		    
    unsigned int     macLen;		    
    int              amount;		    
    unsigned int     padding;		    
    int              nout;		    
    int              buflen;		    

    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss) );

    SSL_TRC(10, ("%d: SSL[%d]: sending %d bytes using block cipher",
		 SSL_GETPID(), ss->fd, len));
    PRINT_BUF(50, (ss, "clear data:", in, len));

    while (len) {
	ssl_GetSpecReadLock(ss);  

	macLen = ss->sec.hash->length;
	
	amount  = PR_MIN( len, MAX_BLOCK_CYPHER_LEN );
	nout    = amount + macLen;
	padding = nout & (ss->sec.blockSize - 1);
	if (padding) {
	    hlen    = 3;
	    padding = ss->sec.blockSize - padding;
	    nout   += padding;
	} else {
	    hlen = 2;
	}
	buflen = hlen + nout;
	if (buflen > ss->sec.writeBuf.space) {
	    rv = sslBuffer_Grow(&ss->sec.writeBuf, buflen);
	    if (rv != SECSuccess) {
		goto loser;
	    }
	}
	out = ss->sec.writeBuf.buf;

	
	op = out;
	if (padding) {
	    *op++ = MSB(nout);
	    *op++ = LSB(nout);
	    *op++ = padding;
	} else {
	    *op++ = 0x80 | MSB(nout);
	    *op++ = LSB(nout);
	}

	
	rv = ssl2_CalcMAC(op, 		
	                  &ss->sec, 
		          in, amount, 	
			  padding);
	if (rv != SECSuccess) 
	    goto loser;
	op += macLen;

	
	
	PORT_Memcpy(op, in, amount);
	op += amount;
	if (padding) {
	    PORT_Memset(op, padding, padding);
	    op += padding;
	}

	
	rv = (*ss->sec.enc)(ss->sec.writecx, out+hlen, &nout, buflen-hlen,
			 out+hlen, op - (out + hlen));
	if (rv) 
	    goto loser;

	ssl_ReleaseSpecReadLock(ss);  

	PRINT_BUF(50, (ss, "final xmit data:", out, op - out));

	rv = ssl_DefSend(ss, out, op - out, flags & ~ssl_SEND_FLAG_MASK);
	if (rv < 0) {
	    if (PORT_GetError() == PR_WOULD_BLOCK_ERROR) {
		rv = 0;
	    } else {
		SSL_TRC(10, ("%d: SSL[%d]: send block error %d",
			     SSL_GETPID(), ss->fd, PORT_GetError()));
		
		if (count == 0)
		    count = rv;
		goto done;
	    }
	}

	if (rv < (op - out)) {
	    
	    if (ssl_SaveWriteData(ss, out + rv, op - out - rv) == SECFailure) {
		count = SECFailure;
	    } else {
		count += amount;
		ss->sec.sendSequence++;
	    }
	    goto done;
	}

	ss->sec.sendSequence++;
	in    += amount;
	count += amount;
	len   -= amount;
    }

done:
    return count;

loser:
    ssl_ReleaseSpecReadLock(ss);
    return SECFailure;
}








static void
ssl2_UseEncryptedSendFunc(sslSocket *ss)
{
    ssl_GetXmitBufLock(ss);
    PORT_Assert(ss->sec.hashcx != 0);

    ss->gs.encrypted = 1;
    ss->sec.send = (ss->sec.blockSize > 1) ? ssl2_SendBlock : ssl2_SendStream;
    ssl_ReleaseXmitBufLock(ss);
}




void
ssl2_UseClearSendFunc(sslSocket *ss)
{
    ss->sec.send = ssl2_SendClear;
}












































SECStatus
ssl_GatherRecord1stHandshake(sslSocket *ss)
{
    int rv;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetRecvBufLock(ss);

    if (ss->version >= SSL_LIBRARY_VERSION_3_0) {
	
	rv = ssl3_GatherCompleteHandshake(ss, 0);
    } else {
	
	rv = ssl2_GatherRecord(ss, 0);
    }
    SSL_TRC(10, ("%d: SSL[%d]: handshake gathering, rv=%d",
		 SSL_GETPID(), ss->fd, rv));

    ssl_ReleaseRecvBufLock(ss);

    if (rv <= 0) {
	if (rv == SECWouldBlock) {
	    
	    SSL_TRC(10, ("%d: SSL[%d]: handshake blocked (need %d)",
			 SSL_GETPID(), ss->fd, ss->gs.remainder));
	    return SECWouldBlock;
	}
	if (rv == 0) {
	    
	    PORT_SetError(PR_END_OF_FILE_ERROR);
	}
	return SECFailure;	
    }

    SSL_TRC(10, ("%d: SSL[%d]: got handshake record of %d bytes",
		 SSL_GETPID(), ss->fd, ss->gs.recordLen));

    ss->handshake = 0;	
    return SECSuccess;
}






static SECStatus
ssl2_FillInSID(sslSessionID * sid, 
          int            cipher,
	  PRUint8       *keyData, 
	  int            keyLen,
	  PRUint8       *ca, 
	  int            caLen,
	  int            keyBits, 
	  int            secretKeyBits,
	  SSLSignType    authAlgorithm,
	  PRUint32       authKeyBits,
	  SSLKEAType     keaType,
	  PRUint32       keaKeyBits)
{
    PORT_Assert(sid->references == 1);
    PORT_Assert(sid->cached == never_cached);
    PORT_Assert(sid->u.ssl2.masterKey.data == 0);
    PORT_Assert(sid->u.ssl2.cipherArg.data == 0);

    sid->version = SSL_LIBRARY_VERSION_2;

    sid->u.ssl2.cipherType = cipher;
    sid->u.ssl2.masterKey.data = (PRUint8*) PORT_Alloc(keyLen);
    if (!sid->u.ssl2.masterKey.data) {
	return SECFailure;
    }
    PORT_Memcpy(sid->u.ssl2.masterKey.data, keyData, keyLen);
    sid->u.ssl2.masterKey.len = keyLen;
    sid->u.ssl2.keyBits       = keyBits;
    sid->u.ssl2.secretKeyBits = secretKeyBits;
    sid->authAlgorithm        = authAlgorithm;
    sid->authKeyBits          = authKeyBits;
    sid->keaType              = keaType;
    sid->keaKeyBits           = keaKeyBits;
    sid->lastAccessTime = sid->creationTime = ssl_Time();
    sid->expirationTime = sid->creationTime + ssl_sid_timeout;

    if (caLen) {
	sid->u.ssl2.cipherArg.data = (PRUint8*) PORT_Alloc(caLen);
	if (!sid->u.ssl2.cipherArg.data) {
	    return SECFailure;
	}
	sid->u.ssl2.cipherArg.len = caLen;
	PORT_Memcpy(sid->u.ssl2.cipherArg.data, ca, caLen);
    }
    return SECSuccess;
}







static SECStatus
ssl2_ProduceKeys(sslSocket *    ss, 
            SECItem *      readKey, 
	    SECItem *      writeKey,
	    SECItem *      masterKey, 
	    PRUint8 *      challenge,
	    PRUint8 *      nonce, 
	    int            cipherType)
{
    PK11Context * cx        = 0;
    unsigned      nkm       = 0; 
    unsigned      nkd       = 0; 
    unsigned      part;
    unsigned      i;
    unsigned      off;
    SECStatus     rv;
    PRUint8       countChar;
    PRUint8       km[3*16];	

    readKey->data = 0;
    writeKey->data = 0;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    rv = SECSuccess;
    cx = PK11_CreateDigestContext(SEC_OID_MD5);
    if (cx == NULL) {
	ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
	return SECFailure;
    }

    nkm = ssl_Specs[cipherType].nkm;
    nkd = ssl_Specs[cipherType].nkd;

    readKey->data = (PRUint8*) PORT_Alloc(nkd);
    if (!readKey->data) 
    	goto loser;
    readKey->len = nkd;

    writeKey->data = (PRUint8*) PORT_Alloc(nkd);
    if (!writeKey->data) 
    	goto loser;
    writeKey->len = nkd;

    
    countChar = '0';
    for (i = 0, off = 0; i < nkm; i++, off += 16) {
	rv  = PK11_DigestBegin(cx);
	rv |= PK11_DigestOp(cx, masterKey->data, masterKey->len);
	rv |= PK11_DigestOp(cx, &countChar,      1);
	rv |= PK11_DigestOp(cx, challenge,       SSL_CHALLENGE_BYTES);
	rv |= PK11_DigestOp(cx, nonce,           SSL_CONNECTIONID_BYTES);
	rv |= PK11_DigestFinal(cx, km+off, &part, MD5_LENGTH);
	if (rv != SECSuccess) {
	    ssl_MapLowLevelError(SSL_ERROR_MD5_DIGEST_FAILURE);
	    rv = SECFailure;
	    goto loser;
	}
	countChar++;
    }

    
    PORT_Memcpy(readKey->data,  km,       nkd);
    PORT_Memcpy(writeKey->data, km + nkd, nkd);

loser:
    PK11_DestroyContext(cx, PR_TRUE);
    return rv;
}







static SECStatus
ssl2_CreateSessionCypher(sslSocket *ss, sslSessionID *sid, PRBool isClient)
{
    SECItem         * rk = NULL;
    SECItem         * wk = NULL;
    SECItem *         param;
    SECStatus         rv;
    int               cipherType  = sid->u.ssl2.cipherType;
    PK11SlotInfo *    slot        = NULL;
    CK_MECHANISM_TYPE mechanism;
    SECItem           readKey;
    SECItem           writeKey;

    void *readcx = 0;
    void *writecx = 0;
    readKey.data = 0;
    writeKey.data = 0;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );
    if((ss->sec.ci.sid == 0))
    	goto sec_loser;	

    


    switch (cipherType) {
    case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
    case SSL_CK_RC4_128_WITH_MD5:
    case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
    case SSL_CK_RC2_128_CBC_WITH_MD5:
    case SSL_CK_DES_64_CBC_WITH_MD5:
    case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	break;

    default:
	SSL_DBG(("%d: SSL[%d]: ssl2_CreateSessionCypher: unknown cipher=%d",
		 SSL_GETPID(), ss->fd, cipherType));
	PORT_SetError(isClient ? SSL_ERROR_BAD_SERVER : SSL_ERROR_BAD_CLIENT);
	goto sec_loser;
    }
 
    rk = isClient ? &readKey  : &writeKey;
    wk = isClient ? &writeKey : &readKey;

    
    rv = ssl2_ProduceKeys(ss, &readKey, &writeKey, &sid->u.ssl2.masterKey,
		     ss->sec.ci.clientChallenge, ss->sec.ci.connectionID,
		     cipherType);
    if (rv != SECSuccess) 
	goto loser;
    PRINT_BUF(7, (ss, "Session read-key: ", rk->data, rk->len));
    PRINT_BUF(7, (ss, "Session write-key: ", wk->data, wk->len));

    PORT_Memcpy(ss->sec.ci.readKey, readKey.data, readKey.len);
    PORT_Memcpy(ss->sec.ci.writeKey, writeKey.data, writeKey.len);
    ss->sec.ci.keySize = readKey.len;

    
    rv = ssl2_CreateMAC(&ss->sec, rk, wk, cipherType);
    if (rv != SECSuccess) 
    	goto loser;

    
    SSL_TRC(3, ("%d: SSL[%d]: using %s", SSL_GETPID(), ss->fd,
	    ssl_cipherName[cipherType]));


    mechanism  = ssl_Specs[cipherType].mechanism;

    
    ss->sec.destroy = (void (*)(void*, PRBool)) PK11_DestroyContext;
    slot = PK11_GetBestSlot(mechanism, ss->pkcs11PinArg);
    if (slot == NULL)
	goto loser;

    param = PK11_ParamFromIV(mechanism, &sid->u.ssl2.cipherArg);
    if (param == NULL)
	goto loser;
    readcx = PK11_CreateContextByRawKey(slot, mechanism, PK11_OriginUnwrap,
					CKA_DECRYPT, rk, param,
					ss->pkcs11PinArg);
    SECITEM_FreeItem(param, PR_TRUE);
    if (readcx == NULL)
	goto loser;

    
    param = PK11_ParamFromIV(mechanism, &sid->u.ssl2.cipherArg);
    if (param == NULL)
	goto loser;
    writecx = PK11_CreateContextByRawKey(slot, mechanism, PK11_OriginUnwrap,
					 CKA_ENCRYPT, wk, param,
					 ss->pkcs11PinArg);
    SECITEM_FreeItem(param,PR_TRUE);
    if (writecx == NULL)
	goto loser;
    PK11_FreeSlot(slot);

    rv = SECSuccess;
    ss->sec.enc           = (SSLCipher) PK11_CipherOp;
    ss->sec.dec           = (SSLCipher) PK11_CipherOp;
    ss->sec.readcx        = (void *) readcx;
    ss->sec.writecx       = (void *) writecx;
    ss->sec.blockSize     = ssl_Specs[cipherType].blockSize;
    ss->sec.blockShift    = ssl_Specs[cipherType].blockShift;
    ss->sec.cipherType    = sid->u.ssl2.cipherType;
    ss->sec.keyBits       = sid->u.ssl2.keyBits;
    ss->sec.secretKeyBits = sid->u.ssl2.secretKeyBits;
    goto done;

  loser:
    if (ss->sec.destroy) {
	if (readcx)  (*ss->sec.destroy)(readcx, PR_TRUE);
	if (writecx) (*ss->sec.destroy)(writecx, PR_TRUE);
    }
    ss->sec.destroy = NULL;
    if (slot) PK11_FreeSlot(slot);

  sec_loser:
    rv = SECFailure;

  done:
    if (rk) {
	SECITEM_ZfreeItem(rk, PR_FALSE);
    }
    if (wk) {
	SECITEM_ZfreeItem(wk, PR_FALSE);
    }
    return rv;
}























static SECStatus
ssl2_ServerSetupSessionCypher(sslSocket *ss, int cipher, unsigned int keyBits,
			 PRUint8 *ck, unsigned int ckLen,
			 PRUint8 *ek, unsigned int ekLen,
			 PRUint8 *ca, unsigned int caLen)
{
    PRUint8      *    dk   = NULL; 
    sslSessionID *    sid;
    sslServerCerts *  sc   = ss->serverCerts + kt_rsa;
    PRUint8       *   kbuf = 0;	
    unsigned int      ddLen;	
    unsigned int      keySize;
    unsigned int      dkLen;    
    int               modulusLen;
    SECStatus         rv;
    PRUint16          allowed;  
    PRUint8           mkbuf[SSL_MAX_MASTER_KEY_BYTES];

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss)   );
    PORT_Assert((sc->SERVERKEY != 0));
    PORT_Assert((ss->sec.ci.sid != 0));
    sid = ss->sec.ci.sid;

    


    switch (cipher) {
    case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
    case SSL_CK_RC4_128_WITH_MD5:
    case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
    case SSL_CK_RC2_128_CBC_WITH_MD5:
    case SSL_CK_DES_64_CBC_WITH_MD5:
    case SSL_CK_DES_192_EDE3_CBC_WITH_MD5:
	break;

    default:
	SSL_DBG(("%d: SSL[%d]: ssl2_ServerSetupSessionCypher: unknown cipher=%d",
		 SSL_GETPID(), ss->fd, cipher));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    allowed = ss->allowedByPolicy & ss->chosenPreference & SSL_CB_IMPLEMENTED;
    if (!(allowed & (1 << cipher))) {
    	
	SSL_DBG(("%d: SSL[%d]: disallowed cipher=%d",
		 SSL_GETPID(), ss->fd, cipher));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    keySize = ssl_Specs[cipher].keyLen;
    if (keyBits != keySize * BPB) {
	SSL_DBG(("%d: SSL[%d]: invalid master secret key length=%d (bits)!",
		 SSL_GETPID(), ss->fd, keyBits));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    if (ckLen != ssl_Specs[cipher].pubLen) {
	SSL_DBG(("%d: SSL[%d]: invalid clear key length, ckLen=%d (bytes)!",
		 SSL_GETPID(), ss->fd, ckLen));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    if (caLen != ssl_Specs[cipher].ivLen) {
	SSL_DBG(("%d: SSL[%d]: invalid key args length, caLen=%d (bytes)!",
		 SSL_GETPID(), ss->fd, caLen));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    modulusLen = PK11_GetPrivateModulusLen(sc->SERVERKEY);
    if (modulusLen == -1) {
	
	modulusLen = ekLen;
    }
    if (ekLen > modulusLen || ekLen + ckLen < keySize) {
	SSL_DBG(("%d: SSL[%d]: invalid encrypted key length, ekLen=%d (bytes)!",
		 SSL_GETPID(), ss->fd, ekLen));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto loser;
    }

    
    kbuf = (PRUint8*)PORT_Alloc(modulusLen);
    if (!kbuf) {
	goto loser;
    }
    dkLen = keySize - ckLen;
    dk    = kbuf + modulusLen - dkLen;

    



    rv = PK11_PubDecryptRaw(sc->SERVERKEY, kbuf, &ddLen, modulusLen, ek, ekLen);
    if (rv != SECSuccess) 
	goto hide_loser;

    
    if (modulusLen != ddLen) 
	goto hide_loser;

    
    if ((kbuf[0] != 0x00) || (kbuf[1] != 0x02) || (dk[-1] != 0x00)) {
	SSL_DBG(("%d: SSL[%d]: strange encryption block",
		 SSL_GETPID(), ss->fd));
	PORT_SetError(SSL_ERROR_BAD_CLIENT);
	goto hide_loser;
    }

    
    if (ss->opt.enableSSL3 || ss->opt.enableTLS) {
	static const PRUint8 threes[8] = { 0x03, 0x03, 0x03, 0x03,
			                   0x03, 0x03, 0x03, 0x03 };
	
	if (PORT_Memcmp(dk - 8 - 1, threes, 8) == 0) {
	    PORT_SetError(SSL_ERROR_BAD_CLIENT);
	    goto hide_loser;
	}
    }
    if (0) {
hide_loser:
	




	PK11_GenerateRandom(dk, dkLen);
    }

    


    if (ckLen) {
	PORT_Memcpy(mkbuf, ck, ckLen);
    }
    PORT_Memcpy(mkbuf + ckLen, dk, dkLen);

    
    rv = ssl2_FillInSID(sid, cipher, mkbuf, keySize, ca, caLen,
		   keyBits, keyBits - (ckLen<<3),
		   ss->sec.authAlgorithm, ss->sec.authKeyBits,
		   ss->sec.keaType,       ss->sec.keaKeyBits);
    if (rv != SECSuccess) {
	goto loser;
    }

    
    rv = ssl2_CreateSessionCypher(ss, sid, PR_FALSE);
    if (rv != SECSuccess) {
	goto loser;
    }

    SSL_TRC(1, ("%d: SSL[%d]: server, using %s cipher, clear=%d total=%d",
		SSL_GETPID(), ss->fd, ssl_cipherName[cipher],
		ckLen<<3, keySize<<3));
    rv = SECSuccess;
    goto done;

  loser:
    rv = SECFailure;

  done:
    PORT_Free(kbuf);
    return rv;
}
















static int
ssl2_QualifyCypherSpecs(sslSocket *ss, 
                        PRUint8 *  cs, 
		        int        csLen)
{
    PRUint8 *    ms;
    PRUint8 *    hs;
    PRUint8 *    qs;
    int          mc;
    int          hc;
    PRUint8      qualifiedSpecs[ssl2_NUM_SUITES_IMPLEMENTED * 3];

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss)   );

    if (!ss->cipherSpecs) {
	SECStatus rv = ssl2_ConstructCipherSpecs(ss);
	if (rv != SECSuccess || !ss->cipherSpecs) 
	    return 0;
    }

    PRINT_BUF(10, (ss, "specs from client:", cs, csLen));
    qs = qualifiedSpecs;
    ms = ss->cipherSpecs;
    for (mc = ss->sizeCipherSpecs; mc > 0; mc -= 3, ms += 3) {
	if (ms[0] == 0)
	    continue;
	for (hs = cs, hc = csLen; hc > 0; hs += 3, hc -= 3) {
	    if ((hs[0] == ms[0]) &&
		(hs[1] == ms[1]) &&
		(hs[2] == ms[2])) {
		
		qs[0] = hs[0];
		qs[1] = hs[1];
		qs[2] = hs[2];
		qs   += 3;
		break;
	    }
	}
    }
    hc = qs - qualifiedSpecs;
    PRINT_BUF(10, (ss, "qualified specs from client:", qualifiedSpecs, hc));
    PORT_Memcpy(cs, qualifiedSpecs, hc);
    return hc;
}















static int
ssl2_ChooseSessionCypher(sslSocket *ss, 
                         int        hc,    
		         PRUint8 *  hs,    
		         int *      pKeyLen) 
{
    PRUint8 *       ms;
    unsigned int    i;
    int             bestKeySize;
    int             bestRealKeySize;
    int             bestCypher;
    int             keySize;
    int             realKeySize;
    PRUint8 *       ohs               = hs;
    const PRUint8 * preferred;
    static const PRUint8 noneSuch[3] = { 0, 0, 0 };

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss)   );

    if (!ss->cipherSpecs) {
	SECStatus rv = ssl2_ConstructCipherSpecs(ss);
	if (rv != SECSuccess || !ss->cipherSpecs) 
	    goto loser;
    }

    if (!ss->preferredCipher) {
    	unsigned int allowed = ss->allowedByPolicy & ss->chosenPreference &
	                       SSL_CB_IMPLEMENTED;
	if (allowed) {
	    preferred = implementedCipherSuites;
	    for (i = ssl2_NUM_SUITES_IMPLEMENTED; i > 0; --i) {
		if (0 != (allowed & (1U << preferred[0]))) {
		    ss->preferredCipher = preferred;
		    break;
		}
		preferred += 3;
	    }
	}
    }
    preferred = ss->preferredCipher ? ss->preferredCipher : noneSuch;
    







    bestKeySize = bestRealKeySize = 0;
    bestCypher = -1;
    while (--hc >= 0) {
	for (i = 0, ms = ss->cipherSpecs; i < ss->sizeCipherSpecs; i += 3, ms += 3) {
	    if ((hs[0] == preferred[0]) &&
		(hs[1] == preferred[1]) &&
		(hs[2] == preferred[2]) &&
		 hs[0] != 0) {
		
		*pKeyLen = (((hs[1] << 8) | hs[2]) + 7) >> 3;
		return hs[0];
	    }
	    if ((hs[0] == ms[0]) && (hs[1] == ms[1]) && (hs[2] == ms[2]) &&
	         hs[0] != 0) {
		

		
		realKeySize = (hs[1] << 8) | hs[2];
		switch (hs[0]) {
		  case SSL_CK_RC4_128_EXPORT40_WITH_MD5:
		  case SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5:
		    keySize = 40;
		    break;
		  default:
		    keySize = realKeySize;
		    break;
		}
		if (keySize > bestKeySize) {
		    bestCypher = hs[0];
		    bestKeySize = keySize;
		    bestRealKeySize = realKeySize;
		}
	    }
	}
	hs += 3;
    }
    if (bestCypher < 0) {
	




	if ((ohs[0] == SSL_CK_RC4_128_WITH_MD5) ||
	    (ohs[0] == SSL_CK_RC2_128_CBC_WITH_MD5)) {
	    PORT_SetError(SSL_ERROR_US_ONLY_SERVER);
	} else if ((ohs[0] == SSL_CK_RC4_128_EXPORT40_WITH_MD5) ||
		   (ohs[0] == SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5)) {
	    PORT_SetError(SSL_ERROR_EXPORT_ONLY_SERVER);
	} else {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	}
	SSL_DBG(("%d: SSL[%d]: no cipher overlap", SSL_GETPID(), ss->fd));
	goto loser;
    }
    *pKeyLen = (bestRealKeySize + 7) >> 3;
    return bestCypher;

  loser:
    return -1;
}

static SECStatus
ssl2_ClientHandleServerCert(sslSocket *ss, PRUint8 *certData, int certLen)
{
    CERTCertificate *cert      = NULL;
    SECItem          certItem;

    certItem.data = certData;
    certItem.len  = certLen;

    
    cert = CERT_NewTempCertificate(ss->dbHandle, &certItem, NULL,
				   PR_FALSE, PR_TRUE);
    
    if (cert == NULL) {
	SSL_DBG(("%d: SSL[%d]: decode of server certificate fails",
		 SSL_GETPID(), ss->fd));
	PORT_SetError(SSL_ERROR_BAD_CERTIFICATE);
	return SECFailure;
    }

#ifdef TRACE
    {
	if (ssl_trace >= 1) {
	    char *issuer;
	    char *subject;
	    issuer = CERT_NameToAscii(&cert->issuer);
	    subject = CERT_NameToAscii(&cert->subject);
	    SSL_TRC(1,("%d: server certificate issuer: '%s'",
		       SSL_GETPID(), issuer ? issuer : "OOPS"));
	    SSL_TRC(1,("%d: server name: '%s'",
		       SSL_GETPID(), subject ? subject : "OOPS"));
	    PORT_Free(issuer);
	    PORT_Free(subject);
	}
    }
#endif

    ss->sec.peerCert = cert;
    return SECSuccess;
}







#define RSA_BLOCK_MIN_PAD_LEN           8
#define RSA_BLOCK_FIRST_OCTET           0x00
#define RSA_BLOCK_AFTER_PAD_OCTET       0x00
#define RSA_BLOCK_PUBLIC_OCTET       	0x02
unsigned char *
ssl_FormatSSL2Block(unsigned modulusLen, SECItem *data)
{
    unsigned char *block;
    unsigned char *bp;
    int padLen;
    SECStatus rv;
    int i;

    if (modulusLen < data->len + (3 + RSA_BLOCK_MIN_PAD_LEN)) {
	PORT_SetError(SEC_ERROR_BAD_KEY);
    	return NULL;
    }
    block = (unsigned char *) PORT_Alloc(modulusLen);
    if (block == NULL)
	return NULL;

    bp = block;

    



    *bp++ = RSA_BLOCK_FIRST_OCTET;
    *bp++ = RSA_BLOCK_PUBLIC_OCTET;

    




    padLen = modulusLen - data->len - 3;
    PORT_Assert (padLen >= RSA_BLOCK_MIN_PAD_LEN);
    rv = PK11_GenerateRandom(bp, padLen);
    if (rv == SECFailure) goto loser;
    
    for (i = 0; i < padLen; i++) {
	while (bp[i] == RSA_BLOCK_AFTER_PAD_OCTET) {
    	    rv = PK11_GenerateRandom(bp+i, 1);
	    if (rv == SECFailure) goto loser;
	}
    }
    bp += padLen;
    *bp++ = RSA_BLOCK_AFTER_PAD_OCTET;
    PORT_Memcpy (bp, data->data, data->len);

    return block;
loser:
    if (block) PORT_Free(block);
    return NULL;
}









static SECStatus
ssl2_ClientSetupSessionCypher(sslSocket *ss, PRUint8 *cs, int csLen)
{
    sslSessionID *    sid;
    PRUint8 *         ca;	
    PRUint8 *         ekbuf 		= 0;
    CERTCertificate * cert 		= 0;
    SECKEYPublicKey * serverKey 	= 0;
    unsigned          modulusLen 	= 0;
    SECStatus         rv;
    int               cipher;
    int               keyLen;	
    int               ckLen;	
    int               caLen;	
    int               nc;

    unsigned char *eblock;	
    SECItem           rek;	

    PRUint8           keyData[SSL_MAX_MASTER_KEY_BYTES];
    PRUint8           iv     [8];

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    eblock = NULL;

    sid = ss->sec.ci.sid;
    PORT_Assert(sid != 0);

    cert = ss->sec.peerCert;
    
    serverKey = CERT_ExtractPublicKey(cert);
    if (!serverKey) {
	SSL_DBG(("%d: SSL[%d]: extract public key failed: error=%d",
		 SSL_GETPID(), ss->fd, PORT_GetError()));
	PORT_SetError(SSL_ERROR_BAD_CERTIFICATE);
	rv = SECFailure;
	goto loser2;
    }

    ss->sec.authAlgorithm = ssl_sign_rsa;
    ss->sec.keaType       = ssl_kea_rsa;
    ss->sec.keaKeyBits    = \
    ss->sec.authKeyBits   = SECKEY_PublicKeyStrengthInBits(serverKey);

    
    nc = csLen / 3;
    cipher = ssl2_ChooseSessionCypher(ss, nc, cs, &keyLen);
    if (cipher < 0) {
	
	ssl2_SendErrorMessage(ss, SSL_PE_NO_CYPHERS);
	goto loser;
    }

    
    PK11_GenerateRandom(keyData, sizeof(keyData));

    






    ca = 0;

    


    ckLen = ssl_Specs[cipher].pubLen;	
    caLen = ssl_Specs[cipher].ivLen;	
    if (caLen) {
	PORT_Assert(sizeof iv >= caLen);
    	PK11_GenerateRandom(iv, caLen);
	ca = iv;
    }

    
    rv = ssl2_FillInSID(sid, cipher, keyData, keyLen,
		   ca, caLen, keyLen << 3, (keyLen - ckLen) << 3,
		   ss->sec.authAlgorithm, ss->sec.authKeyBits,
		   ss->sec.keaType,       ss->sec.keaKeyBits);
    if (rv != SECSuccess) {
	goto loser;
    }

    SSL_TRC(1, ("%d: SSL[%d]: client, using %s cipher, clear=%d total=%d",
		SSL_GETPID(), ss->fd, ssl_cipherName[cipher],
		ckLen<<3, keyLen<<3));

    
    rv = ssl2_CreateSessionCypher(ss, sid, PR_TRUE);
    if (rv != SECSuccess) {
	goto loser;
    }

    



    modulusLen = SECKEY_PublicKeyStrength(serverKey);
    rek.data   = keyData + ckLen;
    rek.len    = keyLen  - ckLen;
    eblock = ssl_FormatSSL2Block(modulusLen, &rek);
    if (eblock == NULL) 
    	goto loser;

    
    
    if (ss->opt.enableSSL3 || ss->opt.enableTLS) {
	PORT_Assert((modulusLen - rek.len) > 12);
	PORT_Memset(eblock + modulusLen - rek.len - 8 - 1, 0x03, 8);
    }
    ekbuf = (PRUint8*) PORT_Alloc(modulusLen);
    if (!ekbuf) 
	goto loser;
    PRINT_BUF(10, (ss, "master key encryption block:",
		   eblock, modulusLen));

    
    rv = PK11_PubEncryptRaw(serverKey, ekbuf, eblock, modulusLen,
						ss->pkcs11PinArg);
    if (rv) 
    	goto loser;

    
    rv = ssl2_SendSessionKeyMessage(ss, cipher, keyLen << 3, ca, caLen,
			       keyData, ckLen, ekbuf, modulusLen);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = SECSuccess;
    goto done;

  loser:
    rv = SECFailure;

  loser2:
  done:
    PORT_Memset(keyData, 0, sizeof(keyData));
    PORT_ZFree(ekbuf, modulusLen);
    PORT_ZFree(eblock, modulusLen);
    SECKEY_DestroyPublicKey(serverKey);
    return rv;
}







static void
ssl2_ClientRegSessionID(sslSocket *ss, PRUint8 *s)
{
    sslSessionID *sid = ss->sec.ci.sid;

    
    if (sid->peerCert == NULL) {
	PORT_Memcpy(sid->u.ssl2.sessionID, s, sizeof(sid->u.ssl2.sessionID));
	sid->peerCert = CERT_DupCertificate(ss->sec.peerCert);

    }
    if (!ss->opt.noCache)
	(*ss->sec.cache)(sid);
}


static SECStatus
ssl2_TriggerNextMessage(sslSocket *ss)
{
    SECStatus        rv;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    if ((ss->sec.ci.requiredElements & CIS_HAVE_CERTIFICATE) &&
	!(ss->sec.ci.sentElements & CIS_HAVE_CERTIFICATE)) {
	ss->sec.ci.sentElements |= CIS_HAVE_CERTIFICATE;
	rv = ssl2_SendCertificateRequestMessage(ss);
	return rv;
    }
    return SECSuccess;
}












static SECStatus
ssl2_TryToFinish(sslSocket *ss)
{
    SECStatus        rv;
    char             e, ef;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    e = ss->sec.ci.elements;
    ef = e | CIS_HAVE_FINISHED;
    if ((ef & ss->sec.ci.requiredElements) == ss->sec.ci.requiredElements) {
	if (ss->sec.isServer) {
	    
	    rv = ssl2_SendServerFinishedMessage(ss);
	} else {
	    
	    rv = ssl2_SendClientFinishedMessage(ss);
	}
	if (rv != SECSuccess) {
	    return rv;
	}
	if ((e & ss->sec.ci.requiredElements) == ss->sec.ci.requiredElements) {
	    
	    ss->handshake = 0;
	    return SECSuccess;
	}
    }
    return SECSuccess;
}





static SECStatus
ssl2_SignResponse(sslSocket *ss,
	     SECKEYPrivateKey *key,
	     SECItem *response)
{
    SGNContext *     sgn = NULL;
    PRUint8 *        challenge;
    unsigned int     len;
    SECStatus        rv		= SECFailure;
    
    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    challenge = ss->sec.ci.serverChallenge;
    len = ss->sec.ci.serverChallengeLen;
    
    
    sgn = SGN_NewContext(SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION,key);
    if (!sgn) 
    	goto done;
    rv = SGN_Begin(sgn);
    if (rv != SECSuccess) 
    	goto done;
    rv = SGN_Update(sgn, ss->sec.ci.readKey, ss->sec.ci.keySize);
    if (rv != SECSuccess) 
    	goto done;
    rv = SGN_Update(sgn, ss->sec.ci.writeKey, ss->sec.ci.keySize);
    if (rv != SECSuccess) 
    	goto done;
    rv = SGN_Update(sgn, challenge, len);
    if (rv != SECSuccess) 
    	goto done;
    rv = SGN_Update(sgn, ss->sec.peerCert->derCert.data, 
                         ss->sec.peerCert->derCert.len);
    if (rv != SECSuccess) 
    	goto done;
    rv = SGN_End(sgn, response);
    if (rv != SECSuccess) 
    	goto done;

done:
    SGN_DestroyContext(sgn, PR_TRUE);
    return rv == SECSuccess ? SECSuccess : SECFailure;
}








static int
ssl2_HandleRequestCertificate(sslSocket *ss)
{
    CERTCertificate * cert	= NULL;	
    SECKEYPrivateKey *key	= NULL;	
    SECStatus         rv;
    SECItem           response;
    int               ret	= 0;
    PRUint8           authType;


    


    response.data = NULL;

    
    authType = ss->sec.ci.authType;

    if (authType != SSL_AT_MD5_WITH_RSA_ENCRYPTION) {
	SSL_TRC(7, ("%d: SSL[%d]: unsupported auth type 0x%x", SSL_GETPID(),
		    ss->fd, authType));
	goto no_cert_error;
    }

    
    if (!ss->getClientAuthData) {
	SSL_TRC(7, ("%d: SSL[%d]: client doesn't support client-auth",
		    SSL_GETPID(), ss->fd));
	goto no_cert_error;
    }
    ret = (*ss->getClientAuthData)(ss->getClientAuthDataArg, ss->fd,
				   NULL, &cert, &key);
    if ( ret == SECWouldBlock ) {
	ssl_SetAlwaysBlock(ss);
	goto done;
    }

    if (ret) {
	goto no_cert_error;
    }

    
    if ((!cert) || (!key)) {
        
        if (cert) {
            
            CERT_DestroyCertificate(cert);
            cert = NULL;
        }
        if (key) {
            
            SECKEY_DestroyPrivateKey(key);
            key = NULL;
        }
        goto no_cert_error;
    }

    rv = ssl2_SignResponse(ss, key, &response);
    if ( rv != SECSuccess ) {
	ret = -1;
	goto loser;
    }

    
    ret = ssl2_SendCertificateResponseMessage(ss, &cert->derCert, &response);

    
    if (ss->sec.localCert) {
	CERT_DestroyCertificate(ss->sec.localCert);
    }
    ss->sec.localCert = CERT_DupCertificate(cert);
    PORT_Assert(!ss->sec.ci.sid->localCert);
    if (ss->sec.ci.sid->localCert) {
	CERT_DestroyCertificate(ss->sec.ci.sid->localCert);
    }
    ss->sec.ci.sid->localCert = cert;
    cert = NULL;

    goto done;

  no_cert_error:
    SSL_TRC(7, ("%d: SSL[%d]: no certificate (ret=%d)", SSL_GETPID(),
		ss->fd, ret));
    ret = ssl2_SendErrorMessage(ss, SSL_PE_NO_CERTIFICATE);

  loser:
  done:
    if ( cert ) {
	CERT_DestroyCertificate(cert);
    }
    if ( key ) {
	SECKEY_DestroyPrivateKey(key);
    }
    if ( response.data ) {
	PORT_Free(response.data);
    }
    
    return ret;
}






static SECStatus
ssl2_HandleClientCertificate(sslSocket *    ss, 
                             PRUint8        certType,	
			     PRUint8 *      cd, 
			     unsigned int   cdLen,
			     PRUint8 *      response,
			     unsigned int   responseLen)
{
    CERTCertificate *cert	= NULL;
    SECKEYPublicKey *pubKey	= NULL;
    VFYContext *     vfy	= NULL;
    SECItem *        derCert;
    SECStatus        rv		= SECFailure;
    SECItem          certItem;
    SECItem          rep;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss)   );

    
    certItem.data = cd;
    certItem.len  = cdLen;

    cert = CERT_NewTempCertificate(ss->dbHandle, &certItem, NULL,
			 	   PR_FALSE, PR_TRUE);
    if (cert == NULL) {
	goto loser;
    }

    
    ss->sec.peerCert = cert;

    
    pubKey = CERT_ExtractPublicKey(cert);
    if (!pubKey) 
    	goto loser;
    
    
    rep.data = response;
    rep.len = responseLen;
    

    vfy = VFY_CreateContext(pubKey, &rep, SEC_OID_PKCS1_RSA_ENCRYPTION,
			    ss->pkcs11PinArg);
    if (!vfy) 
    	goto loser;
    rv = VFY_Begin(vfy);
    if (rv) 
    	goto loser;

    rv = VFY_Update(vfy, ss->sec.ci.readKey, ss->sec.ci.keySize);
    if (rv) 
    	goto loser;
    rv = VFY_Update(vfy, ss->sec.ci.writeKey, ss->sec.ci.keySize);
    if (rv) 
    	goto loser;
    rv = VFY_Update(vfy, ss->sec.ci.serverChallenge, SSL_CHALLENGE_BYTES);
    if (rv) 
    	goto loser;

    derCert = &ss->serverCerts[kt_rsa].serverCert->derCert;
    rv = VFY_Update(vfy, derCert->data, derCert->len);
    if (rv) 
    	goto loser;
    rv = VFY_End(vfy);
    if (rv) 
    	goto loser;

    
    rv = (SECStatus) (*ss->authCertificate)(ss->authCertificateArg,
					    ss->fd, PR_TRUE, PR_TRUE);
    
    if (SECSuccess == rv) 
	goto done;

loser:
    ss->sec.peerCert = NULL;
    CERT_DestroyCertificate(cert);

done:
    VFY_DestroyContext(vfy, PR_TRUE);
    SECKEY_DestroyPublicKey(pubKey);
    return rv;
}











static SECStatus
ssl2_HandleMessage(sslSocket *ss)
{
    PRUint8 *        data;
    PRUint8 *        cid;
    unsigned         len, certType, certLen, responseLen;
    int              rv;
    int              rv2;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ssl_GetRecvBufLock(ss);

    data = ss->gs.buf.buf + ss->gs.recordOffset;

    if (ss->gs.recordLen < 1) {
	goto bad_peer;
    }
    SSL_TRC(3, ("%d: SSL[%d]: received %d message",
		SSL_GETPID(), ss->fd, data[0]));
    DUMP_MSG(29, (ss, data, ss->gs.recordLen));

    switch (data[0]) {
    case SSL_MT_CLIENT_FINISHED:
	if (ss->sec.ci.elements & CIS_HAVE_FINISHED) {
	    SSL_DBG(("%d: SSL[%d]: dup client-finished message",
		     SSL_GETPID(), ss->fd));
	    goto bad_peer;
	}

	
	len = ss->gs.recordLen - 1;
	cid = data + 1;
	if ((len != sizeof(ss->sec.ci.connectionID)) ||
	    (PORT_Memcmp(ss->sec.ci.connectionID, cid, len) != 0)) {
	    SSL_DBG(("%d: SSL[%d]: bad connection-id", SSL_GETPID(), ss->fd));
	    PRINT_BUF(5, (ss, "sent connection-id",
			  ss->sec.ci.connectionID, 
			  sizeof(ss->sec.ci.connectionID)));
	    PRINT_BUF(5, (ss, "rcvd connection-id", cid, len));
	    goto bad_peer;
	}

	SSL_TRC(5, ("%d: SSL[%d]: got client finished, waiting for 0x%d",
		    SSL_GETPID(), ss->fd, 
		    ss->sec.ci.requiredElements ^ ss->sec.ci.elements));
	ss->sec.ci.elements |= CIS_HAVE_FINISHED;
	break;

    case SSL_MT_SERVER_FINISHED:
	if (ss->sec.ci.elements & CIS_HAVE_FINISHED) {
	    SSL_DBG(("%d: SSL[%d]: dup server-finished message",
		     SSL_GETPID(), ss->fd));
	    goto bad_peer;
	}

	if (ss->gs.recordLen - 1 != SSL2_SESSIONID_BYTES) {
	    SSL_DBG(("%d: SSL[%d]: bad server-finished message, len=%d",
		     SSL_GETPID(), ss->fd, ss->gs.recordLen));
	    goto bad_peer;
	}
	ssl2_ClientRegSessionID(ss, data+1);
	SSL_TRC(5, ("%d: SSL[%d]: got server finished, waiting for 0x%d",
		    SSL_GETPID(), ss->fd, 
		    ss->sec.ci.requiredElements ^ ss->sec.ci.elements));
	ss->sec.ci.elements |= CIS_HAVE_FINISHED;
	break;

    case SSL_MT_REQUEST_CERTIFICATE:
	len = ss->gs.recordLen - 2;
	if ((len < SSL_MIN_CHALLENGE_BYTES) ||
	    (len > SSL_MAX_CHALLENGE_BYTES)) {
	    
	    SSL_DBG(("%d: SSL[%d]: bad cert request message: code len=%d",
		     SSL_GETPID(), ss->fd, len));
	    goto bad_peer;
	}
	
	
	ss->sec.ci.authType           = data[1];
	ss->sec.ci.serverChallengeLen = len;
	PORT_Memcpy(ss->sec.ci.serverChallenge, data + 2, len);
	
	rv = ssl2_HandleRequestCertificate(ss);
	if (rv == SECWouldBlock) {
	    SSL_TRC(3, ("%d: SSL[%d]: async cert request",
			SSL_GETPID(), ss->fd));
	    
	    ssl_ReleaseRecvBufLock(ss);
	    return SECWouldBlock;
	}
	if (rv) {
	    SET_ERROR_CODE
	    goto loser;
	}
	break;

    case SSL_MT_CLIENT_CERTIFICATE:
	if (!ss->authCertificate) {
	    
	    PORT_SetError(SSL_ERROR_BAD_SERVER);
	    goto loser;
	}
	if (ss->gs.recordLen < SSL_HL_CLIENT_CERTIFICATE_HBYTES) {
	    SET_ERROR_CODE
	    goto loser;
	}
	certType    = data[1];
	certLen     = (data[2] << 8) | data[3];
	responseLen = (data[4] << 8) | data[5];
	if (certType != SSL_CT_X509_CERTIFICATE) {
	    PORT_SetError(SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE);
	    goto loser;
	}
	if (certLen + responseLen + SSL_HL_CLIENT_CERTIFICATE_HBYTES 
	    > ss->gs.recordLen) {
	    
	    rv = SECFailure;
	} else
	rv = ssl2_HandleClientCertificate(ss, data[1],
		data + SSL_HL_CLIENT_CERTIFICATE_HBYTES,
		certLen,
		data + SSL_HL_CLIENT_CERTIFICATE_HBYTES + certLen,
		responseLen);
	if (rv) {
	    rv2 = ssl2_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    SET_ERROR_CODE
	    goto loser;
	}
	ss->sec.ci.elements |= CIS_HAVE_CERTIFICATE;
	break;

    case SSL_MT_ERROR:
	rv = (data[1] << 8) | data[2];
	SSL_TRC(2, ("%d: SSL[%d]: got error message, error=0x%x",
		    SSL_GETPID(), ss->fd, rv));

	
	switch (rv) {
	  case SSL_PE_NO_CYPHERS:
	    rv = SSL_ERROR_NO_CYPHER_OVERLAP;
	    break;
	  case SSL_PE_NO_CERTIFICATE:
	    rv = SSL_ERROR_NO_CERTIFICATE;
	    break;
	  case SSL_PE_BAD_CERTIFICATE:
	    rv = SSL_ERROR_BAD_CERTIFICATE;
	    break;
	  case SSL_PE_UNSUPPORTED_CERTIFICATE_TYPE:
	    rv = SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE;
	    break;
	  default:
	    goto bad_peer;
	}
	
	PORT_SetError(rv);
	goto loser;

    default:
	SSL_DBG(("%d: SSL[%d]: unknown message %d",
		 SSL_GETPID(), ss->fd, data[0]));
	goto loser;
    }

    SSL_TRC(3, ("%d: SSL[%d]: handled %d message, required=0x%x got=0x%x",
		SSL_GETPID(), ss->fd, data[0],
		ss->sec.ci.requiredElements, ss->sec.ci.elements));

    rv = ssl2_TryToFinish(ss);
    if (rv != SECSuccess) 
	goto loser;

    ss->gs.recordLen = 0;
    ssl_ReleaseRecvBufLock(ss);

    if (ss->handshake == 0) {
	return SECSuccess;
    }

    ss->handshake     = ssl_GatherRecord1stHandshake;
    ss->nextHandshake = ssl2_HandleMessage;
    return ssl2_TriggerNextMessage(ss);

  bad_peer:
    PORT_SetError(ss->sec.isServer ? SSL_ERROR_BAD_CLIENT : SSL_ERROR_BAD_SERVER);
    

  loser:
    ssl_ReleaseRecvBufLock(ss);
    return SECFailure;
}






static SECStatus
ssl2_HandleVerifyMessage(sslSocket *ss)
{
    PRUint8 *        data;
    SECStatus        rv;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );
    ssl_GetRecvBufLock(ss);

    data = ss->gs.buf.buf + ss->gs.recordOffset;
    DUMP_MSG(29, (ss, data, ss->gs.recordLen));
    if ((ss->gs.recordLen != 1 + SSL_CHALLENGE_BYTES) ||
	(data[0] != SSL_MT_SERVER_VERIFY) ||
	NSS_SecureMemcmp(data+1, ss->sec.ci.clientChallenge,
	                 SSL_CHALLENGE_BYTES)) {
	
	PORT_SetError(SSL_ERROR_BAD_SERVER);
	goto loser;
    }
    ss->sec.ci.elements |= CIS_HAVE_VERIFY;

    SSL_TRC(5, ("%d: SSL[%d]: got server-verify, required=0x%d got=0x%x",
		SSL_GETPID(), ss->fd, ss->sec.ci.requiredElements,
		ss->sec.ci.elements));

    rv = ssl2_TryToFinish(ss);
    if (rv) 
	goto loser;

    ss->gs.recordLen = 0;
    ssl_ReleaseRecvBufLock(ss);

    if (ss->handshake == 0) {
	return SECSuccess;
    }
    ss->handshake         = ssl_GatherRecord1stHandshake;
    ss->nextHandshake     = ssl2_HandleMessage;
    return SECSuccess;


  loser:
    ssl_ReleaseRecvBufLock(ss);
    return SECFailure;
}





SECStatus
ssl2_HandleServerHelloMessage(sslSocket *ss)
{
    sslSessionID *   sid;
    PRUint8 *        cert;
    PRUint8 *        cs;
    PRUint8 *        data;
    SECStatus        rv; 
    int              needed, sidHit, certLen, csLen, cidLen, certType, err;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    if (!ss->opt.enableSSL2) {
	PORT_SetError(SSL_ERROR_SSL2_DISABLED);
	return SECFailure;
    }

    ssl_GetRecvBufLock(ss);

    PORT_Assert(ss->sec.ci.sid != 0);
    sid = ss->sec.ci.sid;

    data = ss->gs.buf.buf + ss->gs.recordOffset;
    DUMP_MSG(29, (ss, data, ss->gs.recordLen));

    
    if ((ss->gs.recordLen < SSL_HL_SERVER_HELLO_HBYTES)
	|| (data[0] != SSL_MT_SERVER_HELLO)) {
	if ((data[0] == SSL_MT_ERROR) && (ss->gs.recordLen == 3)) {
	    err = (data[1] << 8) | data[2];
	    if (err == SSL_PE_NO_CYPHERS) {
		PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
		goto loser;
	    }
	}
	goto bad_server;
    }

    sidHit      = data[1];
    certType    = data[2];
    ss->version = (data[3] << 8) | data[4];
    certLen     = (data[5] << 8) | data[6];
    csLen       = (data[7] << 8) | data[8];
    cidLen      = (data[9] << 8) | data[10];
    cert        = data + SSL_HL_SERVER_HELLO_HBYTES;
    cs          = cert + certLen;

    SSL_TRC(5,
	    ("%d: SSL[%d]: server-hello, hit=%d vers=%x certLen=%d csLen=%d cidLen=%d",
	     SSL_GETPID(), ss->fd, sidHit, ss->version, certLen,
	     csLen, cidLen));
    if (ss->version != SSL_LIBRARY_VERSION_2) {
        if (ss->version < SSL_LIBRARY_VERSION_2) {
	  SSL_TRC(3, ("%d: SSL[%d]: demoting self (%x) to server version (%x)",
		      SSL_GETPID(), ss->fd, SSL_LIBRARY_VERSION_2,
		      ss->version));
	} else {
	  SSL_TRC(1, ("%d: SSL[%d]: server version is %x (we are %x)",
		    SSL_GETPID(), ss->fd, ss->version, SSL_LIBRARY_VERSION_2));
	  
	  PORT_SetError(SSL_ERROR_UNSUPPORTED_VERSION);
	  goto loser;
	}
    }

    if ((SSL_HL_SERVER_HELLO_HBYTES + certLen + csLen + cidLen 
                                                  > ss->gs.recordLen)
	|| (csLen % 3) != 0   
	
	) {
	goto bad_server;
    }

    



    if (cidLen < sizeof ss->sec.ci.connectionID)
	memset(ss->sec.ci.connectionID, 0, sizeof ss->sec.ci.connectionID);
    cidLen = PR_MIN(cidLen, sizeof ss->sec.ci.connectionID);
    PORT_Memcpy(ss->sec.ci.connectionID, cs + csLen, cidLen);

    
    needed = CIS_HAVE_MASTER_KEY | CIS_HAVE_FINISHED | CIS_HAVE_VERIFY;
    if (sidHit) {
	if (certLen || csLen) {
	    
	    SSL_DBG(("%d: SSL[%d]: client, huh? hit=%d certLen=%d csLen=%d",
		     SSL_GETPID(), ss->fd, sidHit, certLen, csLen));
	    goto bad_server;
	}

	
	SSL_TRC(1, ("%d: SSL[%d]: client, using nonce for peer=0x%08x "
		    "port=0x%04x",
		    SSL_GETPID(), ss->fd, ss->sec.ci.peer, ss->sec.ci.port));
	ss->sec.peerCert = CERT_DupCertificate(sid->peerCert);
        ss->sec.authAlgorithm = sid->authAlgorithm;
	ss->sec.authKeyBits   = sid->authKeyBits;
	ss->sec.keaType       = sid->keaType;
	ss->sec.keaKeyBits    = sid->keaKeyBits;
	rv = ssl2_CreateSessionCypher(ss, sid, PR_TRUE);
	if (rv != SECSuccess) {
	    goto loser;
	}
    } else {
	if (certType != SSL_CT_X509_CERTIFICATE) {
	    PORT_SetError(SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE);
	    goto loser;
	}
	if (csLen == 0) {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	    SSL_DBG(("%d: SSL[%d]: no cipher overlap",
		     SSL_GETPID(), ss->fd));
	    goto loser;
	}
	if (certLen == 0) {
	    SSL_DBG(("%d: SSL[%d]: client, huh? certLen=%d csLen=%d",
		     SSL_GETPID(), ss->fd, certLen, csLen));
	    goto bad_server;
	}

	if (sid->cached != never_cached) {
	    
	    SSL_TRC(7, ("%d: SSL[%d]: server forgot me, uncaching session-id",
			SSL_GETPID(), ss->fd));
	    (*ss->sec.uncache)(sid);
	    ssl_FreeSID(sid);
	    ss->sec.ci.sid = sid = (sslSessionID*) PORT_ZAlloc(sizeof(sslSessionID));
	    if (!sid) {
		goto loser;
	    }
	    sid->references = 1;
	    sid->addr = ss->sec.ci.peer;
	    sid->port = ss->sec.ci.port;
	}

	
	rv = ssl2_ClientHandleServerCert(ss, cert, certLen);
	if (rv != SECSuccess) {
	    if (PORT_GetError() == SSL_ERROR_BAD_CERTIFICATE) {
		(void) ssl2_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    }
	    goto loser;
	}

	
	rv = ssl2_ClientSetupSessionCypher(ss, cs, csLen);
	if (rv != SECSuccess) {
	    if (PORT_GetError() == SSL_ERROR_BAD_CERTIFICATE) {
		(void) ssl2_SendErrorMessage(ss, SSL_PE_BAD_CERTIFICATE);
	    }
	    goto loser;
	}
    }

    
    ss->sec.ci.elements         = CIS_HAVE_MASTER_KEY;
    ss->sec.ci.requiredElements = needed;

  if (!sidHit) {
    
    rv = (* ss->authCertificate)(ss->authCertificateArg, ss->fd, 
				 (PRBool)(!sidHit), PR_FALSE);
    if (rv) {
	if (ss->handleBadCert) {
	    rv = (*ss->handleBadCert)(ss->badCertArg, ss->fd);
	    if ( rv ) {
		if ( rv == SECWouldBlock ) {
		    

		    SSL_DBG(("%d: SSL[%d]: go to async cert handler",
			     SSL_GETPID(), ss->fd));
		    ssl_ReleaseRecvBufLock(ss);
		    ssl_SetAlwaysBlock(ss);
		    return SECWouldBlock;
		}
		
		SSL_DBG(("%d: SSL[%d]: server certificate is no good: error=%d",
			 SSL_GETPID(), ss->fd, PORT_GetError()));
		goto loser;

	    }
	    
	} else {
	    SSL_DBG(("%d: SSL[%d]: server certificate is no good: error=%d",
		     SSL_GETPID(), ss->fd, PORT_GetError()));
	    goto loser;
	}
    }
  }
    




    ssl2_UseEncryptedSendFunc(ss);

    rv = ssl2_TryToFinish(ss);
    if (rv != SECSuccess) 
	goto loser;

    ss->gs.recordLen = 0;

    ssl_ReleaseRecvBufLock(ss);

    if (ss->handshake == 0) {
	return SECSuccess;
    }

    SSL_TRC(5, ("%d: SSL[%d]: got server-hello, required=0x%d got=0x%x",
		SSL_GETPID(), ss->fd, ss->sec.ci.requiredElements, 
		ss->sec.ci.elements));
    ss->handshake     = ssl_GatherRecord1stHandshake;
    ss->nextHandshake = ssl2_HandleVerifyMessage;
    return SECSuccess;

  bad_server:
    PORT_SetError(SSL_ERROR_BAD_SERVER);
    

  loser:
    ssl_ReleaseRecvBufLock(ss);
    return SECFailure;
}




SECStatus
ssl2_BeginClientHandshake(sslSocket *ss)
{
    sslSessionID      *sid;
    PRUint8           *msg;
    PRUint8           *cp;
    PRUint8           *localCipherSpecs = NULL;
    unsigned int      localCipherSize;
    unsigned int      i;
    int               sendLen, sidLen = 0;
    SECStatus         rv;
    TLSExtensionData  *xtnData;

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    ss->sec.isServer     = 0;
    ss->sec.sendSequence = 0;
    ss->sec.rcvSequence  = 0;
    ssl_ChooseSessionIDProcs(&ss->sec);

    if (!ss->cipherSpecs) {
	rv = ssl2_ConstructCipherSpecs(ss);
	if (rv != SECSuccess)
	    goto loser;
    }

    


    rv = ssl2_CheckConfigSanity(ss);
    if (rv != SECSuccess)
	goto loser;

    
    rv = ssl_GetPeerInfo(ss);
    if (rv < 0) {
#ifdef HPUX11
        





        if (PR_GetError() == PR_NOT_CONNECTED_ERROR) {
            char dummy;
            (void) PR_Write(ss->fd->lower, &dummy, 0);
            rv = ssl_GetPeerInfo(ss);
            if (rv < 0) {
                goto loser;
            }
        }
#else
	goto loser;
#endif
    }

    SSL_TRC(3, ("%d: SSL[%d]: sending client-hello", SSL_GETPID(), ss->fd));

    
    if (ss->opt.noCache) {
	sid = NULL;
    } else {
	sid = ssl_LookupSID(&ss->sec.ci.peer, ss->sec.ci.port, ss->peerID, 
	                    ss->url);
    }
    while (sid) {  
	
	if (((sid->version  < SSL_LIBRARY_VERSION_3_0) && !ss->opt.enableSSL2) ||
	    ((sid->version == SSL_LIBRARY_VERSION_3_0) && !ss->opt.enableSSL3) ||
	    ((sid->version >  SSL_LIBRARY_VERSION_3_0) && !ss->opt.enableTLS)) {
	    ss->sec.uncache(sid);
	    ssl_FreeSID(sid);
	    sid = NULL;
	    break;
	}
	if (ss->opt.enableSSL2 && sid->version < SSL_LIBRARY_VERSION_3_0) {
	    
	    for (i = 0; i < ss->sizeCipherSpecs; i += 3) {
		if (ss->cipherSpecs[i] == sid->u.ssl2.cipherType)
		    break;
	    }
	    if (i >= ss->sizeCipherSpecs) {
		ss->sec.uncache(sid);
		ssl_FreeSID(sid);
		sid = NULL;
		break;
	    }
	}
	sidLen = sizeof(sid->u.ssl2.sessionID);
	PRINT_BUF(4, (ss, "client, found session-id:", sid->u.ssl2.sessionID,
		      sidLen));
	ss->version = sid->version;
	PORT_Assert(!ss->sec.localCert);
	if (ss->sec.localCert) {
	    CERT_DestroyCertificate(ss->sec.localCert);
	}
	ss->sec.localCert     = CERT_DupCertificate(sid->localCert);
	break;  
    } 
    if (!sid) {
	sidLen = 0;
	sid = (sslSessionID*) PORT_ZAlloc(sizeof(sslSessionID));
	if (!sid) {
	    goto loser;
	}
	sid->references = 1;
	sid->cached     = never_cached;
	sid->addr       = ss->sec.ci.peer;
	sid->port       = ss->sec.ci.port;
	if (ss->peerID != NULL) {
	    sid->peerID = PORT_Strdup(ss->peerID);
	}
	if (ss->url != NULL) {
	    sid->urlSvrName = PORT_Strdup(ss->url);
	}
    }
    ss->sec.ci.sid = sid;

    PORT_Assert(sid != NULL);

    if ((sid->version >= SSL_LIBRARY_VERSION_3_0 || !ss->opt.v2CompatibleHello) &&
        (ss->opt.enableSSL3 || ss->opt.enableTLS)) {

	ss->gs.state      = GS_INIT;
	ss->handshake     = ssl_GatherRecord1stHandshake;

	
	ss->version       = SSL_LIBRARY_VERSION_3_0;

	ssl_GetXmitBufLock(ss);    
	ssl_GetSSL3HandshakeLock(ss);
	rv =  ssl3_SendClientHello(ss);
	ssl_ReleaseSSL3HandshakeLock(ss);
	ssl_ReleaseXmitBufLock(ss); 

	return rv;
    }
#if defined(NSS_ENABLE_ECC) && !defined(NSS_ECC_MORE_THAN_SUITE_B)
    
    ssl3_DisableECCSuites(ss, NULL); 
    if (ss->cipherSpecs != NULL) {
	PORT_Free(ss->cipherSpecs);
	ss->cipherSpecs     = NULL;
	ss->sizeCipherSpecs = 0;
    }
#endif

    if (!ss->cipherSpecs) {
        rv = ssl2_ConstructCipherSpecs(ss);
	if (rv < 0) {
	    return rv;
    	}
    }
    localCipherSpecs = ss->cipherSpecs;
    localCipherSize  = ss->sizeCipherSpecs;

    
    sendLen = SSL_HL_CLIENT_HELLO_HBYTES + localCipherSize + 3 + sidLen +
	SSL_CHALLENGE_BYTES;

    
    PK11_GenerateRandom(ss->sec.ci.clientChallenge, SSL_CHALLENGE_BYTES);

    ssl_GetXmitBufLock(ss);    

    rv = ssl2_GetSendBuffer(ss, sendLen);
    if (rv) 
    	goto unlock_loser;

    
    cp = msg = ss->sec.ci.sendBuf.buf;
    msg[0] = SSL_MT_CLIENT_HELLO;
    if ( ss->opt.enableTLS ) {
	ss->clientHelloVersion = SSL_LIBRARY_VERSION_3_1_TLS;
    } else if ( ss->opt.enableSSL3 ) {
	ss->clientHelloVersion = SSL_LIBRARY_VERSION_3_0;
    } else {
	ss->clientHelloVersion = SSL_LIBRARY_VERSION_2;
    }
    
    msg[1] = MSB(ss->clientHelloVersion);
    msg[2] = LSB(ss->clientHelloVersion);
    
    msg[3] = MSB(localCipherSize + 3);
    msg[4] = LSB(localCipherSize + 3);
    msg[5] = MSB(sidLen);
    msg[6] = LSB(sidLen);
    msg[7] = MSB(SSL_CHALLENGE_BYTES);
    msg[8] = LSB(SSL_CHALLENGE_BYTES);
    cp += SSL_HL_CLIENT_HELLO_HBYTES;
    PORT_Memcpy(cp, localCipherSpecs, localCipherSize);
    cp += localCipherSize;
    





    cp[0] = 0x00;
    cp[1] = 0x00;
    cp[2] = 0xff;
    cp += 3;
    if (sidLen) {
	PORT_Memcpy(cp, sid->u.ssl2.sessionID, sidLen);
	cp += sidLen;
    }
    PORT_Memcpy(cp, ss->sec.ci.clientChallenge, SSL_CHALLENGE_BYTES);

    
    DUMP_MSG(29, (ss, msg, sendLen));
    ss->handshakeBegun = 1;
    rv = (*ss->sec.send)(ss, msg, sendLen, 0);

    ssl_ReleaseXmitBufLock(ss);    

    if (rv < 0) {
	goto loser;
    }

    rv = ssl3_StartHandshakeHash(ss, msg, sendLen);
    if (rv < 0) {
	goto loser;
    }

    




    xtnData = &ss->xtnData;
    xtnData->advertised[xtnData->numAdvertised++] = ssl_renegotiation_info_xtn;

    
    ssl_GetRecvBufLock(ss);
    ss->gs.recordLen = 0;
    ssl_ReleaseRecvBufLock(ss);

    ss->handshake     = ssl_GatherRecord1stHandshake;
    ss->nextHandshake = ssl2_HandleServerHelloMessage;
    return SECSuccess;

unlock_loser:
    ssl_ReleaseXmitBufLock(ss);
loser:
    return SECFailure;
}







static SECStatus
ssl2_HandleClientSessionKeyMessage(sslSocket *ss)
{
    PRUint8 *        data;
    unsigned int     caLen;
    unsigned int     ckLen;
    unsigned int     ekLen;
    unsigned int     keyBits;
    int              cipher;
    SECStatus        rv;


    ssl_GetRecvBufLock(ss);

    data = ss->gs.buf.buf + ss->gs.recordOffset;
    DUMP_MSG(29, (ss, data, ss->gs.recordLen));

    if ((ss->gs.recordLen < SSL_HL_CLIENT_MASTER_KEY_HBYTES)
	|| (data[0] != SSL_MT_CLIENT_MASTER_KEY)) {
	goto bad_client;
    }
    cipher  = data[1];
    keyBits = (data[2] << 8) | data[3];
    ckLen   = (data[4] << 8) | data[5];
    ekLen   = (data[6] << 8) | data[7];
    caLen   = (data[8] << 8) | data[9];

    SSL_TRC(5, ("%d: SSL[%d]: session-key, cipher=%d keyBits=%d ckLen=%d ekLen=%d caLen=%d",
		SSL_GETPID(), ss->fd, cipher, keyBits, ckLen, ekLen, caLen));

    if (ss->gs.recordLen < 
    	    SSL_HL_CLIENT_MASTER_KEY_HBYTES + ckLen + ekLen + caLen) {
	SSL_DBG(("%d: SSL[%d]: protocol size mismatch dataLen=%d",
		 SSL_GETPID(), ss->fd, ss->gs.recordLen));
	goto bad_client;
    }

    
    rv = ssl2_ServerSetupSessionCypher(ss, cipher, keyBits,
		data + SSL_HL_CLIENT_MASTER_KEY_HBYTES,                 ckLen,
		data + SSL_HL_CLIENT_MASTER_KEY_HBYTES + ckLen,         ekLen,
		data + SSL_HL_CLIENT_MASTER_KEY_HBYTES + ckLen + ekLen, caLen);
    ss->gs.recordLen = 0;	

    ssl_ReleaseRecvBufLock(ss);

    if (rv != SECSuccess) {
	goto loser;
    }
    ss->sec.ci.elements |= CIS_HAVE_MASTER_KEY;
    ssl2_UseEncryptedSendFunc(ss);

    
    rv = ssl2_SendServerVerifyMessage(ss);
    if (rv != SECSuccess) 
	goto loser;

    rv = ssl2_TryToFinish(ss);
    if (rv != SECSuccess) 
	goto loser;
    if (ss->handshake == 0) {
	return SECSuccess;
    }

    SSL_TRC(5, ("%d: SSL[%d]: server: waiting for elements=0x%d",
		SSL_GETPID(), ss->fd, 
		ss->sec.ci.requiredElements ^ ss->sec.ci.elements));
    ss->handshake         = ssl_GatherRecord1stHandshake;
    ss->nextHandshake     = ssl2_HandleMessage;

    return ssl2_TriggerNextMessage(ss);

bad_client:
    ssl_ReleaseRecvBufLock(ss);
    PORT_SetError(SSL_ERROR_BAD_CLIENT);
    

loser:
    return SECFailure;
}
















int
ssl2_RestartHandshakeAfterCertReq(sslSocket *          ss,
				  CERTCertificate *    cert, 
				  SECKEYPrivateKey *   key)
{
    int              ret;
    SECStatus        rv          = SECSuccess;
    SECItem          response;

    if (ss->version >= SSL_LIBRARY_VERSION_3_0) 
    	return SECFailure;

    response.data = NULL;

    
    if ( ( cert == NULL ) || ( key == NULL ) ) {
	goto no_cert;
    }
    
    
    rv = ssl2_SignResponse(ss, key, &response);
    if ( rv != SECSuccess ) {
	goto no_cert;
    }
    
    
    ret = ssl2_SendCertificateResponseMessage(ss, &cert->derCert, &response);
    if (ret) {
	goto no_cert;
    }

    
    ret = ssl2_TryToFinish(ss);
    if (ret) {
	goto loser;
    }
    
    
    if (ss->handshake == 0) {
	ret = SECSuccess;
	goto done;
    }

    
    ssl_GetRecvBufLock(ss);
    ss->gs.recordLen = 0;
    ssl_ReleaseRecvBufLock(ss);

    ss->handshake     = ssl_GatherRecord1stHandshake;
    ss->nextHandshake = ssl2_HandleMessage;
    ret = ssl2_TriggerNextMessage(ss);
    goto done;
    
no_cert:
    
    ret = ssl2_SendErrorMessage(ss, SSL_PE_NO_CERTIFICATE);
    goto done;
    
loser:
    ret = SECFailure;
done:
    
    if ( response.data ) {
	PORT_Free(response.data);
    }
    
    return ret;
}










int
ssl2_RestartHandshakeAfterServerCert(sslSocket *ss)
{
    int rv	= SECSuccess;

    if (ss->version >= SSL_LIBRARY_VERSION_3_0) 
	return SECFailure;

    




    ssl2_UseEncryptedSendFunc(ss);

    rv = ssl2_TryToFinish(ss);
    if (rv == SECSuccess && ss->handshake != NULL) {	
	

	SSL_TRC(5, ("%d: SSL[%d]: got server-hello, required=0x%d got=0x%x",
		SSL_GETPID(), ss->fd, ss->sec.ci.requiredElements,
		ss->sec.ci.elements));

	ssl_GetRecvBufLock(ss);
	ss->gs.recordLen = 0;	
	ssl_ReleaseRecvBufLock(ss);

	ss->handshake     = ssl_GatherRecord1stHandshake;
	ss->nextHandshake = ssl2_HandleVerifyMessage;
    }

    return rv;
}






SECStatus
ssl2_HandleClientHelloMessage(sslSocket *ss)
{
    sslSessionID    *sid;
    sslServerCerts * sc;
    CERTCertificate *serverCert;
    PRUint8         *msg;
    PRUint8         *data;
    PRUint8         *cs;
    PRUint8         *sd;
    PRUint8         *cert = NULL;
    PRUint8         *challenge;
    unsigned int    challengeLen;
    SECStatus       rv; 
    int             csLen;
    int             sendLen;
    int             sdLen;
    int             certLen;
    int             pid;
    int             sent;
    int             gotXmitBufLock = 0;
#if defined(SOLARIS) && defined(i386)
    volatile PRUint8 hit;
#else
    int             hit;
#endif
    PRUint8         csImpl[sizeof implementedCipherSuites];

    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    sc = ss->serverCerts + kt_rsa;
    serverCert = sc->serverCert;

    ssl_GetRecvBufLock(ss);


    data = ss->gs.buf.buf + ss->gs.recordOffset;
    DUMP_MSG(29, (ss, data, ss->gs.recordLen));

    
    if ((ss->gs.recordLen < SSL_HL_CLIENT_HELLO_HBYTES)
	|| (data[0] != SSL_MT_CLIENT_HELLO)) {
	goto bad_client;
    }

    
    rv = ssl_GetPeerInfo(ss);
    if (rv != SECSuccess) {
	goto loser;
    }

    
    


    if ((data[0] == SSL_MT_CLIENT_HELLO) && 
        (data[1] >= MSB(SSL_LIBRARY_VERSION_3_0)) && 
	(ss->opt.enableSSL3 || ss->opt.enableTLS)) {
	rv = ssl3_HandleV2ClientHello(ss, data, ss->gs.recordLen);
	if (rv != SECFailure) { 
	    ss->handshake             = NULL;
	    ss->nextHandshake         = ssl_GatherRecord1stHandshake;
	    ss->securityHandshake     = NULL;
	    ss->gs.state              = GS_INIT;

	    


	    ss->sec.ci.sid->version  = ss->version;
	}
	ssl_ReleaseRecvBufLock(ss);
	return rv;
    }
    









    
    ss->version = (data[1] << 8) | data[2];

    
    if (ss->version >= SSL_LIBRARY_VERSION_3_0) {
	ss->version = SSL_LIBRARY_VERSION_2;
    }
    
    csLen        = (data[3] << 8) | data[4];
    sdLen        = (data[5] << 8) | data[6];
    challengeLen = (data[7] << 8) | data[8];
    cs           = data + SSL_HL_CLIENT_HELLO_HBYTES;
    sd           = cs + csLen;
    challenge    = sd + sdLen;
    PRINT_BUF(7, (ss, "server, client session-id value:", sd, sdLen));

    if (!csLen || (csLen % 3) != 0 || 
        (sdLen != 0 && sdLen != SSL2_SESSIONID_BYTES) ||
	challengeLen < SSL_MIN_CHALLENGE_BYTES || 
	challengeLen > SSL_MAX_CHALLENGE_BYTES ||
        (unsigned)ss->gs.recordLen != 
            SSL_HL_CLIENT_HELLO_HBYTES + csLen + sdLen + challengeLen) {
	SSL_DBG(("%d: SSL[%d]: bad client hello message, len=%d should=%d",
		 SSL_GETPID(), ss->fd, ss->gs.recordLen,
		 SSL_HL_CLIENT_HELLO_HBYTES+csLen+sdLen+challengeLen));
	goto bad_client;
    }

    SSL_TRC(3, ("%d: SSL[%d]: client version is %x",
		SSL_GETPID(), ss->fd, ss->version));
    if (ss->version != SSL_LIBRARY_VERSION_2) {
	if (ss->version > SSL_LIBRARY_VERSION_2) {
	    





	    ss->version = SSL_LIBRARY_VERSION_2;
	} else {
	    SSL_TRC(1, ("%d: SSL[%d]: client version is %x (we are %x)",
		SSL_GETPID(), ss->fd, ss->version, SSL_LIBRARY_VERSION_2));
	    PORT_SetError(SSL_ERROR_UNSUPPORTED_VERSION);
	    goto loser;
	}
    }

    
    csLen = ssl2_QualifyCypherSpecs(ss, cs, csLen);
    if (csLen == 0) {
	
        cs    = csImpl;
	csLen = sizeof implementedCipherSuites;
    	PORT_Memcpy(cs, implementedCipherSuites, csLen);
	csLen = ssl2_QualifyCypherSpecs(ss, cs, csLen);
	if (csLen == 0) {
	  
	  ssl2_SendErrorMessage(ss, SSL_PE_NO_CYPHERS);
	  PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	  goto loser;
	}
	
	ss->opt.noCache = 1; 
    }

    
    PORT_Memcpy(ss->sec.ci.clientChallenge, challenge, challengeLen);

    
    ss->sec.ci.elements = 0;
    if (sdLen > 0 && !ss->opt.noCache) {
	SSL_TRC(7, ("%d: SSL[%d]: server, lookup client session-id for 0x%08x%08x%08x%08x",
		    SSL_GETPID(), ss->fd, ss->sec.ci.peer.pr_s6_addr32[0],
		    ss->sec.ci.peer.pr_s6_addr32[1], 
		    ss->sec.ci.peer.pr_s6_addr32[2],
		    ss->sec.ci.peer.pr_s6_addr32[3]));
	sid = (*ssl_sid_lookup)(&ss->sec.ci.peer, sd, sdLen, ss->dbHandle);
    } else {
	sid = NULL;
    }
    if (sid) {
	
	SSL_TRC(1, ("%d: SSL[%d]: server, using session-id for 0x%08x (age=%d)",
		    SSL_GETPID(), ss->fd, ss->sec.ci.peer, 
		    ssl_Time() - sid->creationTime));
	PRINT_BUF(1, (ss, "session-id value:", sd, sdLen));
	ss->sec.ci.sid = sid;
	ss->sec.ci.elements = CIS_HAVE_MASTER_KEY;
	hit = 1;
	certLen = 0;
	csLen = 0;

        ss->sec.authAlgorithm = sid->authAlgorithm;
	ss->sec.authKeyBits   = sid->authKeyBits;
	ss->sec.keaType       = sid->keaType;
	ss->sec.keaKeyBits    = sid->keaKeyBits;

	rv = ssl2_CreateSessionCypher(ss, sid, PR_FALSE);
	if (rv != SECSuccess) {
	    goto loser;
	}
    } else {
	SECItem * derCert   = &serverCert->derCert;

	SSL_TRC(7, ("%d: SSL[%d]: server, lookup nonce missed",
		    SSL_GETPID(), ss->fd));
	if (!serverCert) {
	    SET_ERROR_CODE
	    goto loser;
	}
	hit = 0;
	sid = (sslSessionID*) PORT_ZAlloc(sizeof(sslSessionID));
	if (!sid) {
	    goto loser;
	}
	sid->references = 1;
	sid->addr = ss->sec.ci.peer;
	sid->port = ss->sec.ci.port;

	
	ss->sec.ci.sid = sid;
	PK11_GenerateRandom(sid->u.ssl2.sessionID+2, SSL2_SESSIONID_BYTES-2);

	pid = SSL_GETPID();
	sid->u.ssl2.sessionID[0] = MSB(pid);
	sid->u.ssl2.sessionID[1] = LSB(pid);
	cert    = derCert->data;
	certLen = derCert->len;

	
	PORT_Assert(!sid->localCert);
	if (sid->localCert) {
	    CERT_DestroyCertificate(sid->localCert);
	}
	sid->localCert     = CERT_DupCertificate(serverCert);

	ss->sec.authAlgorithm = ssl_sign_rsa;
	ss->sec.keaType       = ssl_kea_rsa;
	ss->sec.keaKeyBits    = \
	ss->sec.authKeyBits   = ss->serverCerts[kt_rsa].serverKeyBits;
    }

    


    if (ss->sec.localCert) {
	CERT_DestroyCertificate(ss->sec.localCert);
    }
    ss->sec.localCert     = CERT_DupCertificate(serverCert);

    
    ss->sec.ci.requiredElements = CIS_HAVE_MASTER_KEY | CIS_HAVE_FINISHED;
    if (ss->opt.requestCertificate) {
	ss->sec.ci.requiredElements |= CIS_HAVE_CERTIFICATE;
    }
    ss->sec.ci.sentElements = 0;

    
    sendLen = SSL_HL_SERVER_HELLO_HBYTES + certLen + csLen
	    + SSL_CONNECTIONID_BYTES;

    ssl_GetXmitBufLock(ss); gotXmitBufLock = 1;
    rv = ssl2_GetSendBuffer(ss, sendLen);
    if (rv != SECSuccess) {
	goto loser;
    }

    SSL_TRC(3, ("%d: SSL[%d]: sending server-hello (%d)",
		SSL_GETPID(), ss->fd, sendLen));

    msg = ss->sec.ci.sendBuf.buf;
    msg[0] = SSL_MT_SERVER_HELLO;
    msg[1] = hit;
    msg[2] = SSL_CT_X509_CERTIFICATE;
    msg[3] = MSB(ss->version);
    msg[4] = LSB(ss->version);
    msg[5] = MSB(certLen);
    msg[6] = LSB(certLen);
    msg[7] = MSB(csLen);
    msg[8] = LSB(csLen);
    msg[9] = MSB(SSL_CONNECTIONID_BYTES);
    msg[10] = LSB(SSL_CONNECTIONID_BYTES);
    if (certLen) {
	PORT_Memcpy(msg+SSL_HL_SERVER_HELLO_HBYTES, cert, certLen);
    }
    if (csLen) {
	PORT_Memcpy(msg+SSL_HL_SERVER_HELLO_HBYTES+certLen, cs, csLen);
    }
    PORT_Memcpy(msg+SSL_HL_SERVER_HELLO_HBYTES+certLen+csLen, 
                ss->sec.ci.connectionID, SSL_CONNECTIONID_BYTES);

    DUMP_MSG(29, (ss, msg, sendLen));

    ss->handshakeBegun = 1;
    sent = (*ss->sec.send)(ss, msg, sendLen, 0);
    if (sent < 0) {
	goto loser;
    }
    ssl_ReleaseXmitBufLock(ss); gotXmitBufLock = 0;

    ss->gs.recordLen = 0;
    ss->handshake = ssl_GatherRecord1stHandshake;
    if (hit) {
	
	ssl2_UseEncryptedSendFunc(ss);

	
	rv = ssl2_SendServerVerifyMessage(ss);
	if (rv != SECSuccess) 
	    goto loser;

	ss->nextHandshake = ssl2_HandleMessage;
	ssl_ReleaseRecvBufLock(ss);
	rv = ssl2_TriggerNextMessage(ss);
	return rv;
    }
    ss->nextHandshake = ssl2_HandleClientSessionKeyMessage;
    ssl_ReleaseRecvBufLock(ss);
    return SECSuccess;

  bad_client:
    PORT_SetError(SSL_ERROR_BAD_CLIENT);
    

  loser:
    if (gotXmitBufLock) {
    	ssl_ReleaseXmitBufLock(ss); gotXmitBufLock = 0;
    }
    SSL_TRC(10, ("%d: SSL[%d]: server, wait for client-hello lossage",
		 SSL_GETPID(), ss->fd));
    ssl_ReleaseRecvBufLock(ss);
    return SECFailure;
}

SECStatus
ssl2_BeginServerHandshake(sslSocket *ss)
{
    SECStatus        rv;
    sslServerCerts * rsaAuth = ss->serverCerts + kt_rsa;

    ss->sec.isServer = 1;
    ssl_ChooseSessionIDProcs(&ss->sec);
    ss->sec.sendSequence = 0;
    ss->sec.rcvSequence = 0;

    
    if (!rsaAuth->serverKeyPair || !rsaAuth->SERVERKEY || 
        !rsaAuth->serverCert) {
	ss->opt.enableSSL2 = PR_FALSE;
    }

    if (!ss->cipherSpecs) {
	rv = ssl2_ConstructCipherSpecs(ss);
	if (rv != SECSuccess)
	    goto loser;
    }

    


    rv = ssl2_CheckConfigSanity(ss);
    if (rv != SECSuccess)
	goto loser;

    




    PK11_GenerateRandom(ss->sec.ci.connectionID, 
                        sizeof(ss->sec.ci.connectionID));

    ss->gs.recordLen = 0;
    ss->handshake     = ssl_GatherRecord1stHandshake;
    ss->nextHandshake = ssl2_HandleClientHelloMessage;
    return SECSuccess;

loser:
    return SECFailure;
}






#include "nss.h"
extern const char __nss_ssl_rcsid[];
extern const char __nss_ssl_sccsid[];

PRBool
NSSSSL_VersionCheck(const char *importedVersion)
{
    








    volatile char c; 

    c = __nss_ssl_rcsid[0] + __nss_ssl_sccsid[0]; 
    return NSS_VersionCheck(importedVersion);
}
