






#include "cert.h"
#include "pk11pub.h"
#include "secitem.h"
#include "ssl.h"
#include "nss.h"

#include "sslimpl.h"
#include "sslproto.h"
#include "nssilock.h"
#if defined(XP_UNIX) || defined(XP_WIN) || defined(_WINDOWS) || defined(XP_BEOS)
#include <time.h>
#endif

PRUint32 ssl_sid_timeout = 100;
PRUint32 ssl3_sid_timeout = 86400L; 

static sslSessionID *cache = NULL;
static PZLock *      cacheLock = NULL;









#define LOCK_CACHE 	lock_cache()
#define UNLOCK_CACHE	PZ_Unlock(cacheLock)

static SECStatus
ssl_InitClientSessionCacheLock(void)
{
    cacheLock = PZ_NewLock(nssILockCache);
    return cacheLock ? SECSuccess : SECFailure;
}

static SECStatus
ssl_FreeClientSessionCacheLock(void)
{
    if (cacheLock) {
        PZ_DestroyLock(cacheLock);
        cacheLock = NULL;
        return SECSuccess;
    }
    PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
    return SECFailure;
}

static PRBool LocksInitializedEarly = PR_FALSE;

static SECStatus
FreeSessionCacheLocks()
{
    SECStatus rv1, rv2;
    rv1 = ssl_FreeSymWrapKeysLock();
    rv2 = ssl_FreeClientSessionCacheLock();
    if ( (SECSuccess == rv1) && (SECSuccess == rv2) ) {
        return SECSuccess;
    }
    return SECFailure;
}

static SECStatus
InitSessionCacheLocks(void)
{
    SECStatus rv1, rv2;
    PRErrorCode rc;
    rv1 = ssl_InitSymWrapKeysLock();
    rv2 = ssl_InitClientSessionCacheLock();
    if ( (SECSuccess == rv1) && (SECSuccess == rv2) ) {
        return SECSuccess;
    }
    rc = PORT_GetError();
    FreeSessionCacheLocks();
    PORT_SetError(rc);
    return SECFailure;
}


SECStatus
ssl_FreeSessionCacheLocks()
{
    PORT_Assert(PR_TRUE == LocksInitializedEarly);
    if (!LocksInitializedEarly) {
        PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
        return SECFailure;
    }
    FreeSessionCacheLocks();
    LocksInitializedEarly = PR_FALSE;
    return SECSuccess;
}

static PRCallOnceType lockOnce;


static SECStatus ssl_ShutdownLocks(void* appData, void* nssData)
{
    PORT_Assert(PR_FALSE == LocksInitializedEarly);
    if (LocksInitializedEarly) {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    FreeSessionCacheLocks();
    memset(&lockOnce, 0, sizeof(lockOnce));
    return SECSuccess;
}

static PRStatus initSessionCacheLocksLazily(void)
{
    SECStatus rv = InitSessionCacheLocks();
    if (SECSuccess != rv) {
        return PR_FAILURE;
    }
    rv = NSS_RegisterShutdown(ssl_ShutdownLocks, NULL);
    PORT_Assert(SECSuccess == rv);
    if (SECSuccess != rv) {
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}




SECStatus
ssl_InitSessionCacheLocks(PRBool lazyInit)
{
    if (LocksInitializedEarly) {
        return SECSuccess;
    }

    if (lazyInit) {
        return (PR_SUCCESS ==
                PR_CallOnce(&lockOnce, initSessionCacheLocksLazily)) ?
               SECSuccess : SECFailure;
    }
     
    if (SECSuccess == InitSessionCacheLocks()) {
        LocksInitializedEarly = PR_TRUE;
        return SECSuccess;
    }

    return SECFailure;
}

static void 
lock_cache(void)
{
    ssl_InitSessionCacheLocks(PR_TRUE);
    PZ_Lock(cacheLock);
}




static void
ssl_DestroySID(sslSessionID *sid)
{
    SSL_TRC(8, ("SSL: destroy sid: sid=0x%x cached=%d", sid, sid->cached));
    PORT_Assert((sid->references == 0));

    if (sid->cached == in_client_cache)
    	return;	

    if (sid->version < SSL_LIBRARY_VERSION_3_0) {
	SECITEM_ZfreeItem(&sid->u.ssl2.masterKey, PR_FALSE);
	SECITEM_ZfreeItem(&sid->u.ssl2.cipherArg, PR_FALSE);
    } else {
        if (sid->u.ssl3.locked.sessionTicket.ticket.data) {
            SECITEM_FreeItem(&sid->u.ssl3.locked.sessionTicket.ticket,
                             PR_FALSE);
        }
        if (sid->u.ssl3.srvName.data) {
            SECITEM_FreeItem(&sid->u.ssl3.srvName, PR_FALSE);
        }

        if (sid->u.ssl3.lock) {
            PR_DestroyRWLock(sid->u.ssl3.lock);
        }
    }

    if (sid->peerID != NULL)
	PORT_Free((void *)sid->peerID);		

    if (sid->urlSvrName != NULL)
	PORT_Free((void *)sid->urlSvrName);	

    if ( sid->peerCert ) {
	CERT_DestroyCertificate(sid->peerCert);
    }
    if (sid->peerCertStatus.items) {
        SECITEM_FreeArray(&sid->peerCertStatus, PR_FALSE);
    }

    if ( sid->localCert ) {
	CERT_DestroyCertificate(sid->localCert);
    }
    
    PORT_ZFree(sid, sizeof(sslSessionID));
}








static void 
ssl_FreeLockedSID(sslSessionID *sid)
{
    PORT_Assert(sid->references >= 1);
    if (--sid->references == 0) {
	ssl_DestroySID(sid);
    }
}







void
ssl_FreeSID(sslSessionID *sid)
{
    LOCK_CACHE;
    ssl_FreeLockedSID(sid);
    UNLOCK_CACHE;
}









sslSessionID *
ssl_LookupSID(const PRIPv6Addr *addr, PRUint16 port, const char *peerID, 
              const char * urlSvrName)
{
    sslSessionID **sidp;
    sslSessionID * sid;
    PRUint32       now;

    if (!urlSvrName)
    	return NULL;
    now = ssl_Time();
    LOCK_CACHE;
    sidp = &cache;
    while ((sid = *sidp) != 0) {
	PORT_Assert(sid->cached == in_client_cache);
	PORT_Assert(sid->references >= 1);

	SSL_TRC(8, ("SSL: Lookup1: sid=0x%x", sid));

	if (sid->expirationTime < now || !sid->references) {
	    



	    SSL_TRC(7, ("SSL: lookup1, throwing sid out, age=%d refs=%d",
			now - sid->creationTime, sid->references));

	    *sidp = sid->next; 			
	    sid->cached = invalid_cache;	
	    if (!sid->references)
	    	ssl_DestroySID(sid);
	    else
		ssl_FreeLockedSID(sid);		

	} else if (!memcmp(&sid->addr, addr, sizeof(PRIPv6Addr)) && 
	           (sid->port == port) && 
		   
		   (((peerID == NULL) && (sid->peerID == NULL)) ||
		    ((peerID != NULL) && (sid->peerID != NULL) &&
		     PORT_Strcmp(sid->peerID, peerID) == 0)) &&
		   
		   (sid->version < SSL_LIBRARY_VERSION_3_0 ||
		    sid->u.ssl3.keys.resumable) &&
		   
	           (sid->urlSvrName != NULL) &&
		   ((0 == PORT_Strcmp(urlSvrName, sid->urlSvrName)) ||
		    ((sid->peerCert != NULL) && (SECSuccess == 
		      CERT_VerifyCertName(sid->peerCert, urlSvrName))) )
		  ) {
	    
	    sid->lastAccessTime = now;
	    sid->references++;
	    break;
	} else {
	    sidp = &sid->next;
	}
    }
    UNLOCK_CACHE;
    return sid;
}





static void 
CacheSID(sslSessionID *sid)
{
    PRUint32  expirationPeriod;

    PORT_Assert(sid->cached == never_cached);

    SSL_TRC(8, ("SSL: Cache: sid=0x%x cached=%d addr=0x%08x%08x%08x%08x port=0x%04x "
		"time=%x cached=%d",
		sid, sid->cached, sid->addr.pr_s6_addr32[0], 
		sid->addr.pr_s6_addr32[1], sid->addr.pr_s6_addr32[2],
		sid->addr.pr_s6_addr32[3],  sid->port, sid->creationTime,
		sid->cached));

    if (!sid->urlSvrName) {
        
        return;
    }

    
    if (sid->version < SSL_LIBRARY_VERSION_3_0) {
	expirationPeriod = ssl_sid_timeout;
	PRINT_BUF(8, (0, "sessionID:",
		  sid->u.ssl2.sessionID, sizeof(sid->u.ssl2.sessionID)));
	PRINT_BUF(8, (0, "masterKey:",
		  sid->u.ssl2.masterKey.data, sid->u.ssl2.masterKey.len));
	PRINT_BUF(8, (0, "cipherArg:",
		  sid->u.ssl2.cipherArg.data, sid->u.ssl2.cipherArg.len));
    } else {
	if (sid->u.ssl3.sessionIDLength == 0 &&
	    sid->u.ssl3.locked.sessionTicket.ticket.data == NULL)
	    return;

	
	if (sid->u.ssl3.sessionIDLength == 0) {
	    SECStatus rv;
	    rv = PK11_GenerateRandom(sid->u.ssl3.sessionID,
		SSL3_SESSIONID_BYTES);
	    if (rv != SECSuccess)
		return;
	    sid->u.ssl3.sessionIDLength = SSL3_SESSIONID_BYTES;
	}
	expirationPeriod = ssl3_sid_timeout;
	PRINT_BUF(8, (0, "sessionID:",
		      sid->u.ssl3.sessionID, sid->u.ssl3.sessionIDLength));

	sid->u.ssl3.lock = PR_NewRWLock(PR_RWLOCK_RANK_NONE, NULL);
	if (!sid->u.ssl3.lock) {
	    return;
	}
    }
    PORT_Assert(sid->creationTime != 0 && sid->expirationTime != 0);
    if (!sid->creationTime)
	sid->lastAccessTime = sid->creationTime = ssl_Time();
    if (!sid->expirationTime)
	sid->expirationTime = sid->creationTime + expirationPeriod;

    




    LOCK_CACHE;
    sid->references++;
    sid->cached = in_client_cache;
    sid->next   = cache;
    cache       = sid;
    UNLOCK_CACHE;
}






static void
UncacheSID(sslSessionID *zap)
{
    sslSessionID **sidp = &cache;
    sslSessionID *sid;

    if (zap->cached != in_client_cache) {
	return;
    }

    SSL_TRC(8,("SSL: Uncache: zap=0x%x cached=%d addr=0x%08x%08x%08x%08x port=0x%04x "
	       "time=%x cipher=%d",
	       zap, zap->cached, zap->addr.pr_s6_addr32[0],
	       zap->addr.pr_s6_addr32[1], zap->addr.pr_s6_addr32[2],
	       zap->addr.pr_s6_addr32[3], zap->port, zap->creationTime,
	       zap->u.ssl2.cipherType));
    if (zap->version < SSL_LIBRARY_VERSION_3_0) {
	PRINT_BUF(8, (0, "sessionID:",
		      zap->u.ssl2.sessionID, sizeof(zap->u.ssl2.sessionID)));
	PRINT_BUF(8, (0, "masterKey:",
		      zap->u.ssl2.masterKey.data, zap->u.ssl2.masterKey.len));
	PRINT_BUF(8, (0, "cipherArg:",
		      zap->u.ssl2.cipherArg.data, zap->u.ssl2.cipherArg.len));
    }

    
    while ((sid = *sidp) != 0) {
	if (sid == zap) {
	    



	    *sidp = zap->next;
	    zap->cached = invalid_cache;
	    ssl_FreeLockedSID(zap);
	    return;
	}
	sidp = &sid->next;
    }
}






static void
LockAndUncacheSID(sslSessionID *zap)
{
    LOCK_CACHE;
    UncacheSID(zap);
    UNLOCK_CACHE;

}


void 
ssl_ChooseSessionIDProcs(sslSecurityInfo *sec)
{
    if (sec->isServer) {
	sec->cache   = ssl_sid_cache;
	sec->uncache = ssl_sid_uncache;
    } else {
	sec->cache   = CacheSID;
	sec->uncache = LockAndUncacheSID;
    }
}


void
SSL_ClearSessionCache(void)
{
    LOCK_CACHE;
    while(cache != NULL)
	UncacheSID(cache);
    UNLOCK_CACHE;
}


PRUint32
ssl_Time(void)
{
    PRUint32 myTime;
#if defined(XP_UNIX) || defined(XP_WIN) || defined(_WINDOWS) || defined(XP_BEOS)
    myTime = time(NULL);	
#else
    
    PRTime now;
    PRInt64 ll;

    now = PR_Now();
    LL_I2L(ll, 1000000L);
    LL_DIV(now, now, ll);
    LL_L2UI(myTime, now);
#endif
    return myTime;
}

void
ssl3_SetSIDSessionTicket(sslSessionID *sid,
                          NewSessionTicket *newSessionTicket)
{
    PORT_Assert(sid);
    PORT_Assert(newSessionTicket);

    




    if (sid->u.ssl3.lock) {
	PR_RWLock_Wlock(sid->u.ssl3.lock);

	


	if (sid->u.ssl3.locked.sessionTicket.ticket.data) {
	    SECITEM_FreeItem(&sid->u.ssl3.locked.sessionTicket.ticket,
			     PR_FALSE);
	}
    }

    PORT_Assert(!sid->u.ssl3.locked.sessionTicket.ticket.data);

    
    sid->u.ssl3.locked.sessionTicket = *newSessionTicket;
    newSessionTicket->ticket.data = NULL;
    newSessionTicket->ticket.len = 0;

    if (sid->u.ssl3.lock) {
	PR_RWLock_Unlock(sid->u.ssl3.lock);
    }
}
