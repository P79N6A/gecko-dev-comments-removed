











































#include "prerror.h"
#include "prprf.h"
#include "plarena.h"
#include "prnetdb.h"

#include "seccomon.h"
#include "secitem.h"
#include "secoidt.h"
#include "secasn1.h"
#include "secder.h"
#include "cert.h"
#include "xconst.h"
#include "secerr.h"
#include "secoid.h"
#include "hasht.h"
#include "sechash.h"
#include "secasn1.h"
#include "keyhi.h"
#include "cryptohi.h"
#include "ocsp.h"
#include "ocspti.h"
#include "ocspi.h"
#include "genname.h"
#include "certxutl.h"
#include "pk11func.h"	
#include <stdarg.h>
#include <plhash.h>

#define DEFAULT_OCSP_CACHE_SIZE 1000
#define DEFAULT_MINIMUM_SECONDS_TO_NEXT_OCSP_FETCH_ATTEMPT 1*60*60L
#define DEFAULT_MAXIMUM_SECONDS_TO_NEXT_OCSP_FETCH_ATTEMPT 24*60*60L
#define DEFAULT_OSCP_TIMEOUT_SECONDS 60
#define MICROSECONDS_PER_SECOND 1000000L

typedef struct OCSPCacheItemStr OCSPCacheItem;
typedef struct OCSPCacheDataStr OCSPCacheData;

struct OCSPCacheItemStr {
    
    OCSPCacheItem *moreRecent;
    OCSPCacheItem *lessRecent;

    
    CERTOCSPCertID *certID;
    

    
    PRTime nextFetchAttemptTime;

    
    PRArenaPool *certStatusArena; 
    ocspCertStatus certStatus;

    
    SECErrorCodes missingResponseError;

    PRPackedBool haveThisUpdate;
    PRPackedBool haveNextUpdate;
    PRTime thisUpdate;
    PRTime nextUpdate;
};

struct OCSPCacheDataStr {
    PLHashTable *entries;
    PRUint32 numberOfEntries;
    OCSPCacheItem *MRUitem; 
    OCSPCacheItem *LRUitem; 
};

static struct OCSPGlobalStruct {
    PRMonitor *monitor;
    const SEC_HttpClientFcn *defaultHttpClientFcn;
    PRInt32 maxCacheEntries;
    PRUint32 minimumSecondsToNextFetchAttempt;
    PRUint32 maximumSecondsToNextFetchAttempt;
    PRUint32 timeoutSeconds;
    OCSPCacheData cache;
    SEC_OcspFailureMode ocspFailureMode;
    CERT_StringFromCertFcn alternateOCSPAIAFcn;
} OCSP_Global = { NULL, 
                  NULL, 
                  DEFAULT_OCSP_CACHE_SIZE, 
                  DEFAULT_MINIMUM_SECONDS_TO_NEXT_OCSP_FETCH_ATTEMPT,
                  DEFAULT_MAXIMUM_SECONDS_TO_NEXT_OCSP_FETCH_ATTEMPT,
                  DEFAULT_OSCP_TIMEOUT_SECONDS,
                  {NULL, 0, NULL, NULL},
                  ocspMode_FailureIsVerificationFailure,
                  NULL
                };




static SECItem *
ocsp_GetEncodedOCSPResponseFromRequest(PRArenaPool *arena, 
                                       CERTOCSPRequest *request,
                                       char *location, int64 time,
                                       PRBool addServiceLocator,
                                       void *pwArg,
                                       CERTOCSPRequest **pRequest);
static SECStatus
ocsp_GetOCSPStatusFromNetwork(CERTCertDBHandle *handle, 
                              CERTOCSPCertID *certID, 
                              CERTCertificate *cert, 
                              int64 time, 
                              void *pwArg,
                              PRBool *certIDWasConsumed,
                              SECStatus *rv_ocsp);
static SECStatus
ocsp_GetVerifiedSingleResponseForCertID(CERTCertDBHandle *handle, 
                                        CERTOCSPResponse *response, 
                                        CERTOCSPCertID   *certID,
                                        CERTCertificate  *signerCert,
                                        int64             time,
                                        CERTOCSPSingleResponse **pSingleResponse);

#ifndef DEBUG
#define OCSP_TRACE(msg)
#define OCSP_TRACE_TIME(msg, time)
#define OCSP_TRACE_CERT(cert)
#define OCSP_TRACE_CERTID(certid)
#else
#define OCSP_TRACE(msg) ocsp_Trace msg
#define OCSP_TRACE_TIME(msg, time) ocsp_dumpStringWithTime(msg, time)
#define OCSP_TRACE_CERT(cert) dumpCertificate(cert)
#define OCSP_TRACE_CERTID(certid) dumpCertID(certid)

#if (defined(XP_UNIX) || defined(XP_WIN32) || defined(XP_BEOS) \
     || defined(XP_MACOSX)) && !defined(_WIN32_WCE)
#define NSS_HAVE_GETENV 1
#endif

static PRBool wantOcspTrace()
{
    static PRBool firstTime = PR_TRUE;
    static PRBool wantTrace = PR_FALSE;

#ifdef NSS_HAVE_GETENV
    if (firstTime) {
        char *ev = getenv("NSS_TRACE_OCSP");
        if (ev && ev[0]) {
            wantTrace = PR_TRUE;
        }
        firstTime = PR_FALSE;
    }
#endif
    return wantTrace;
}

static void
ocsp_Trace(const char *format, ...)
{
    char buf[2000];
    va_list args;
  
    if (!wantOcspTrace())
        return;
    va_start(args, format);
    PR_vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    PR_LogPrint("%s", buf);
}

static void
ocsp_dumpStringWithTime(const char *str, int64 time)
{
    PRExplodedTime timePrintable;
    char timestr[256];

    if (!wantOcspTrace())
        return;
    PR_ExplodeTime(time, PR_GMTParameters, &timePrintable);
    if (PR_FormatTime(timestr, 256, "%a %b %d %H:%M:%S %Y", &timePrintable)) {
        ocsp_Trace("OCSP %s %s\n", str, timestr);
    }
}

static void
printHexString(const char *prefix, SECItem *hexval)
{
    unsigned int i;
    char *hexbuf = NULL;

    for (i = 0; i < hexval->len; i++) {
        if (i != hexval->len - 1) {
            hexbuf = PR_sprintf_append(hexbuf, "%02x:", hexval->data[i]);
        } else {
            hexbuf = PR_sprintf_append(hexbuf, "%02x", hexval->data[i]);
        }
    }
    if (hexbuf) {
        ocsp_Trace("%s %s\n", prefix, hexbuf);
        PR_smprintf_free(hexbuf);
    }
}

static void
dumpCertificate(CERTCertificate *cert)
{
    if (!wantOcspTrace())
        return;

    ocsp_Trace("OCSP ----------------\n");
    ocsp_Trace("OCSP ## SUBJECT:  %s\n", cert->subjectName);
    {
        int64 timeBefore, timeAfter;
        PRExplodedTime beforePrintable, afterPrintable;
        char beforestr[256], afterstr[256];
        PRStatus rv1, rv2;
        DER_DecodeTimeChoice(&timeBefore, &cert->validity.notBefore);
        DER_DecodeTimeChoice(&timeAfter, &cert->validity.notAfter);
        PR_ExplodeTime(timeBefore, PR_GMTParameters, &beforePrintable);
        PR_ExplodeTime(timeAfter, PR_GMTParameters, &afterPrintable);
        rv1 = PR_FormatTime(beforestr, 256, "%a %b %d %H:%M:%S %Y", 
                      &beforePrintable);
        rv2 = PR_FormatTime(afterstr, 256, "%a %b %d %H:%M:%S %Y", 
                      &afterPrintable);
        ocsp_Trace("OCSP ## VALIDITY:  %s to %s\n", rv1 ? beforestr : "",
                   rv2 ? afterstr : "");
    }
    ocsp_Trace("OCSP ## ISSUER:  %s\n", cert->issuerName);
    printHexString("OCSP ## SERIAL NUMBER:", &cert->serialNumber);
}

static void
dumpCertID(CERTOCSPCertID *certID)
{
    if (!wantOcspTrace())
        return;

    printHexString("OCSP certID issuer", &certID->issuerNameHash);
    printHexString("OCSP certID serial", &certID->serialNumber);
}
#endif

SECStatus
SEC_RegisterDefaultHttpClient(const SEC_HttpClientFcn *fcnTable)
{
    if (!OCSP_Global.monitor) {
      PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
      return SECFailure;
    }
    
    PR_EnterMonitor(OCSP_Global.monitor);
    OCSP_Global.defaultHttpClientFcn = fcnTable;
    PR_ExitMonitor(OCSP_Global.monitor);
    
    return SECSuccess;
}

SECStatus
CERT_RegisterAlternateOCSPAIAInfoCallBack(
			CERT_StringFromCertFcn   newCallback,
			CERT_StringFromCertFcn * oldCallback)
{
    CERT_StringFromCertFcn old;

    if (!OCSP_Global.monitor) {
      PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
      return SECFailure;
    }

    PR_EnterMonitor(OCSP_Global.monitor);
    old = OCSP_Global.alternateOCSPAIAFcn;
    OCSP_Global.alternateOCSPAIAFcn = newCallback;
    PR_ExitMonitor(OCSP_Global.monitor);
    if (oldCallback)
    	*oldCallback = old;
    return SECSuccess;
}

static PLHashNumber PR_CALLBACK
ocsp_CacheKeyHashFunction(const void *key)
{
    CERTOCSPCertID *cid = (CERTOCSPCertID *)key;
    PLHashNumber hash = 0;
    unsigned int i;
    unsigned char *walk;
  
    
    walk = (unsigned char*)cid->issuerNameHash.data;
    for (i=0; i < cid->issuerNameHash.len; ++i, ++walk) {
        hash += *walk;
    }
    walk = (unsigned char*)cid->issuerKeyHash.data;
    for (i=0; i < cid->issuerKeyHash.len; ++i, ++walk) {
        hash += *walk;
    }
    walk = (unsigned char*)cid->serialNumber.data;
    for (i=0; i < cid->serialNumber.len; ++i, ++walk) {
        hash += *walk;
    }
    return hash;
}

static PRIntn PR_CALLBACK
ocsp_CacheKeyCompareFunction(const void *v1, const void *v2)
{
    CERTOCSPCertID *cid1 = (CERTOCSPCertID *)v1;
    CERTOCSPCertID *cid2 = (CERTOCSPCertID *)v2;
  
    return (SECEqual == SECITEM_CompareItem(&cid1->issuerNameHash, 
                                            &cid2->issuerNameHash)
            && SECEqual == SECITEM_CompareItem(&cid1->issuerKeyHash, 
                                               &cid2->issuerKeyHash)
            && SECEqual == SECITEM_CompareItem(&cid1->serialNumber, 
                                               &cid2->serialNumber));
}

static SECStatus
ocsp_CopyRevokedInfo(PRArenaPool *arena, ocspCertStatus *dest, 
                     ocspRevokedInfo *src)
{
    SECStatus rv = SECFailure;
    void *mark;
  
    mark = PORT_ArenaMark(arena);
  
    dest->certStatusInfo.revokedInfo = 
        (ocspRevokedInfo *) PORT_ArenaZAlloc(arena, sizeof(ocspRevokedInfo));
    if (!dest->certStatusInfo.revokedInfo) {
        goto loser;
    }
  
    rv = SECITEM_CopyItem(arena, 
                          &dest->certStatusInfo.revokedInfo->revocationTime, 
                          &src->revocationTime);
    if (rv != SECSuccess) {
        goto loser;
    }
  
    if (src->revocationReason) {
        dest->certStatusInfo.revokedInfo->revocationReason = 
            SECITEM_ArenaDupItem(arena, src->revocationReason);
        if (!dest->certStatusInfo.revokedInfo->revocationReason) {
            goto loser;
        }
    }  else {
        dest->certStatusInfo.revokedInfo->revocationReason = NULL;
    }
  
    PORT_ArenaUnmark(arena, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(arena, mark);
    return SECFailure;
}

static SECStatus
ocsp_CopyCertStatus(PRArenaPool *arena, ocspCertStatus *dest, 
                    ocspCertStatus*src)
{
    SECStatus rv = SECFailure;
    dest->certStatusType = src->certStatusType;
  
    switch (src->certStatusType) {
    case ocspCertStatus_good:
        dest->certStatusInfo.goodInfo = 
            SECITEM_ArenaDupItem(arena, src->certStatusInfo.goodInfo);
        if (dest->certStatusInfo.goodInfo != NULL) {
            rv = SECSuccess;
        }
        break;
    case ocspCertStatus_revoked:
        rv = ocsp_CopyRevokedInfo(arena, dest, 
                                  src->certStatusInfo.revokedInfo);
        break;
    case ocspCertStatus_unknown:
        dest->certStatusInfo.unknownInfo = 
            SECITEM_ArenaDupItem(arena, src->certStatusInfo.unknownInfo);
        if (dest->certStatusInfo.unknownInfo != NULL) {
            rv = SECSuccess;
        }
        break;
    case ocspCertStatus_other:
    default:
        PORT_Assert(src->certStatusType == ocspCertStatus_other);
        dest->certStatusInfo.otherInfo = 
            SECITEM_ArenaDupItem(arena, src->certStatusInfo.otherInfo);
        if (dest->certStatusInfo.otherInfo != NULL) {
            rv = SECSuccess;
        }
        break;
    }
    return rv;
}

static void
ocsp_AddCacheItemToLinkedList(OCSPCacheData *cache, OCSPCacheItem *new_most_recent)
{
    PR_EnterMonitor(OCSP_Global.monitor);

    if (!cache->LRUitem) {
        cache->LRUitem = new_most_recent;
    }
    new_most_recent->lessRecent = cache->MRUitem;
    new_most_recent->moreRecent = NULL;

    if (cache->MRUitem) {
        cache->MRUitem->moreRecent = new_most_recent;
    }
    cache->MRUitem = new_most_recent;

    PR_ExitMonitor(OCSP_Global.monitor);
}

static void
ocsp_RemoveCacheItemFromLinkedList(OCSPCacheData *cache, OCSPCacheItem *item)
{
    PR_EnterMonitor(OCSP_Global.monitor);

    if (!item->lessRecent && !item->moreRecent) {
        




        if (item == cache->LRUitem &&
            item == cache->MRUitem) {
            
            PORT_Assert(cache->numberOfEntries == 1);
            PORT_Assert(item->moreRecent == NULL);
            cache->MRUitem = NULL;
            cache->LRUitem = NULL;
        }
        PR_ExitMonitor(OCSP_Global.monitor);
        return;
    }

    PORT_Assert(cache->numberOfEntries > 1);
  
    if (item == cache->LRUitem) {
        PORT_Assert(item != cache->MRUitem);
        PORT_Assert(item->lessRecent == NULL);
        PORT_Assert(item->moreRecent != NULL);
        PORT_Assert(item->moreRecent->lessRecent == item);
        cache->LRUitem = item->moreRecent;
        cache->LRUitem->lessRecent = NULL;
    }
    else if (item == cache->MRUitem) {
        PORT_Assert(item->moreRecent == NULL);
        PORT_Assert(item->lessRecent != NULL);
        PORT_Assert(item->lessRecent->moreRecent == item);
        cache->MRUitem = item->lessRecent;
        cache->MRUitem->moreRecent = NULL;
    } else {
        
        PORT_Assert(item->moreRecent != NULL);
        PORT_Assert(item->lessRecent != NULL);
        PORT_Assert(item->lessRecent->moreRecent == item);
        PORT_Assert(item->moreRecent->lessRecent == item);
        item->moreRecent->lessRecent = item->lessRecent;
        item->lessRecent->moreRecent = item->moreRecent;
    }

    item->lessRecent = NULL;
    item->moreRecent = NULL;

    PR_ExitMonitor(OCSP_Global.monitor);
}

static void
ocsp_MakeCacheEntryMostRecent(OCSPCacheData *cache, OCSPCacheItem *new_most_recent)
{
    OCSP_TRACE(("OCSP ocsp_MakeCacheEntryMostRecent THREADID %p\n", 
                PR_GetCurrentThread()));
    PR_EnterMonitor(OCSP_Global.monitor);
    if (cache->MRUitem == new_most_recent) {
        OCSP_TRACE(("OCSP ocsp_MakeCacheEntryMostRecent ALREADY MOST\n"));
        PR_ExitMonitor(OCSP_Global.monitor);
        return;
    }
    OCSP_TRACE(("OCSP ocsp_MakeCacheEntryMostRecent NEW entry\n"));
    ocsp_RemoveCacheItemFromLinkedList(cache, new_most_recent);
    ocsp_AddCacheItemToLinkedList(cache, new_most_recent);
    PR_ExitMonitor(OCSP_Global.monitor);
}

static PRBool
ocsp_IsCacheDisabled()
{
    



    PRBool retval;
    PR_EnterMonitor(OCSP_Global.monitor);
    retval = (OCSP_Global.maxCacheEntries < 0);
    PR_ExitMonitor(OCSP_Global.monitor);
    return retval;
}

static OCSPCacheItem *
ocsp_FindCacheEntry(OCSPCacheData *cache, CERTOCSPCertID *certID)
{
    OCSPCacheItem *found_ocsp_item = NULL;
    OCSP_TRACE(("OCSP ocsp_FindCacheEntry\n"));
    OCSP_TRACE_CERTID(certID);
    PR_EnterMonitor(OCSP_Global.monitor);
    if (ocsp_IsCacheDisabled())
        goto loser;
  
    found_ocsp_item = (OCSPCacheItem *)PL_HashTableLookup(
                          cache->entries, certID);
    if (!found_ocsp_item)
        goto loser;
  
    OCSP_TRACE(("OCSP ocsp_FindCacheEntry FOUND!\n"));
    ocsp_MakeCacheEntryMostRecent(cache, found_ocsp_item);

loser:
    PR_ExitMonitor(OCSP_Global.monitor);
    return found_ocsp_item;
}

static void
ocsp_FreeCacheItem(OCSPCacheItem *item)
{
    OCSP_TRACE(("OCSP ocsp_FreeCacheItem\n"));
    if (item->certStatusArena) {
        PORT_FreeArena(item->certStatusArena, PR_FALSE);
    }
    if (item->certID->poolp) {
        
        PORT_FreeArena(item->certID->poolp, PR_FALSE);
    }
}

static void
ocsp_RemoveCacheItem(OCSPCacheData *cache, OCSPCacheItem *item)
{
    




    PRBool couldRemoveFromHashTable;
    OCSP_TRACE(("OCSP ocsp_RemoveCacheItem, THREADID %p\n", PR_GetCurrentThread()));
    PR_EnterMonitor(OCSP_Global.monitor);

    ocsp_RemoveCacheItemFromLinkedList(cache, item);
    couldRemoveFromHashTable = PL_HashTableRemove(cache->entries, 
                                                  item->certID);
    PORT_Assert(couldRemoveFromHashTable);
    --cache->numberOfEntries;
    ocsp_FreeCacheItem(item);
    PR_ExitMonitor(OCSP_Global.monitor);
}

static void
ocsp_CheckCacheSize(OCSPCacheData *cache)
{
    OCSP_TRACE(("OCSP ocsp_CheckCacheSize\n"));
    PR_EnterMonitor(OCSP_Global.monitor);
    if (OCSP_Global.maxCacheEntries <= 0) 
        return;
    while (cache->numberOfEntries > OCSP_Global.maxCacheEntries) {
        ocsp_RemoveCacheItem(cache, cache->LRUitem);
    }
    PR_ExitMonitor(OCSP_Global.monitor);
}

SECStatus
CERT_ClearOCSPCache()
{
    OCSP_TRACE(("OCSP CERT_ClearOCSPCache\n"));
    PR_EnterMonitor(OCSP_Global.monitor);
    while (OCSP_Global.cache.numberOfEntries > 0) {
        ocsp_RemoveCacheItem(&OCSP_Global.cache, 
                             OCSP_Global.cache.LRUitem);
    }
    PR_ExitMonitor(OCSP_Global.monitor);
    return SECSuccess;
}

static SECStatus
ocsp_CreateCacheItemAndConsumeCertID(OCSPCacheData *cache,
                                     CERTOCSPCertID *certID, 
                                     OCSPCacheItem **pCacheItem)
{
    PRArenaPool *arena;
    void *mark;
    PLHashEntry *new_hash_entry;
    OCSPCacheItem *item;
  
    PORT_Assert(pCacheItem != NULL);
    *pCacheItem = NULL;

    PR_EnterMonitor(OCSP_Global.monitor);
    arena = certID->poolp;
    mark = PORT_ArenaMark(arena);
  
    

    item = (OCSPCacheItem *)PORT_ArenaZAlloc(certID->poolp, 
                                             sizeof(OCSPCacheItem));
    if (!item) {
        goto loser; 
    }
    item->certID = certID;
    new_hash_entry = PL_HashTableAdd(cache->entries, item->certID, 
                                     item);
    if (!new_hash_entry) {
        goto loser;
    }
    ++cache->numberOfEntries;
    PORT_ArenaUnmark(arena, mark);
    ocsp_AddCacheItemToLinkedList(cache, item);
    *pCacheItem = item;

    PR_ExitMonitor(OCSP_Global.monitor);
    return SECSuccess;
  
loser:
    PORT_ArenaRelease(arena, mark);
    PR_ExitMonitor(OCSP_Global.monitor);
    return SECFailure;
}

static SECStatus
ocsp_SetCacheItemResponse(OCSPCacheItem *item,
                          const CERTOCSPSingleResponse *response)
{
    if (item->certStatusArena) {
        PORT_FreeArena(item->certStatusArena, PR_FALSE);
        item->certStatusArena = NULL;
    }
    item->haveThisUpdate = item->haveNextUpdate = PR_FALSE;
    if (response) {
        SECStatus rv;
        item->certStatusArena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (item->certStatusArena == NULL) {
            return SECFailure;
        }
        rv = ocsp_CopyCertStatus(item->certStatusArena, &item->certStatus, 
                                 response->certStatus);
        if (rv != SECSuccess) {
            PORT_FreeArena(item->certStatusArena, PR_FALSE);
            item->certStatusArena = NULL;
            return rv;
        }
        item->missingResponseError = 0;
        rv = DER_GeneralizedTimeToTime(&item->thisUpdate, 
                                       &response->thisUpdate);
        item->haveThisUpdate = (rv == SECSuccess);
        if (response->nextUpdate) {
            rv = DER_GeneralizedTimeToTime(&item->nextUpdate, 
                                           response->nextUpdate);
            item->haveNextUpdate = (rv == SECSuccess);
        } else {
            item->haveNextUpdate = PR_FALSE;
        }
    }
    return SECSuccess;
}

static void
ocsp_FreshenCacheItemNextFetchAttemptTime(OCSPCacheItem *cacheItem)
{
    PRTime now;
    PRTime earliestAllowedNextFetchAttemptTime;
    PRTime latestTimeWhenResponseIsConsideredFresh;
  
    OCSP_TRACE(("OCSP ocsp_FreshenCacheItemNextFetchAttemptTime\n"));

    PR_EnterMonitor(OCSP_Global.monitor);
  
    now = PR_Now();
    OCSP_TRACE_TIME("now:", now);
  
    if (cacheItem->haveThisUpdate) {
        OCSP_TRACE_TIME("thisUpdate:", cacheItem->thisUpdate);
        latestTimeWhenResponseIsConsideredFresh = cacheItem->thisUpdate +
            OCSP_Global.maximumSecondsToNextFetchAttempt * 
                MICROSECONDS_PER_SECOND;
        OCSP_TRACE_TIME("latestTimeWhenResponseIsConsideredFresh:", 
                        latestTimeWhenResponseIsConsideredFresh);
    } else {
        latestTimeWhenResponseIsConsideredFresh = now +
            OCSP_Global.minimumSecondsToNextFetchAttempt *
                MICROSECONDS_PER_SECOND;
        OCSP_TRACE_TIME("no thisUpdate, "
                        "latestTimeWhenResponseIsConsideredFresh:", 
                        latestTimeWhenResponseIsConsideredFresh);
    }
  
    if (cacheItem->haveNextUpdate) {
        OCSP_TRACE_TIME("have nextUpdate:", cacheItem->nextUpdate);
    }
  
    if (cacheItem->haveNextUpdate &&
        cacheItem->nextUpdate < latestTimeWhenResponseIsConsideredFresh) {
        latestTimeWhenResponseIsConsideredFresh = cacheItem->nextUpdate;
        OCSP_TRACE_TIME("nextUpdate is smaller than latestFresh, setting "
                        "latestTimeWhenResponseIsConsideredFresh:", 
                        latestTimeWhenResponseIsConsideredFresh);
    }
  
    earliestAllowedNextFetchAttemptTime = now +
        OCSP_Global.minimumSecondsToNextFetchAttempt * 
            MICROSECONDS_PER_SECOND;
    OCSP_TRACE_TIME("earliestAllowedNextFetchAttemptTime:", 
                    earliestAllowedNextFetchAttemptTime);
  
    if (latestTimeWhenResponseIsConsideredFresh < 
        earliestAllowedNextFetchAttemptTime) {
        latestTimeWhenResponseIsConsideredFresh = 
            earliestAllowedNextFetchAttemptTime;
        OCSP_TRACE_TIME("latest < earliest, setting latest to:", 
                        latestTimeWhenResponseIsConsideredFresh);
    }
  
    cacheItem->nextFetchAttemptTime = 
        latestTimeWhenResponseIsConsideredFresh;
    OCSP_TRACE_TIME("nextFetchAttemptTime", 
        latestTimeWhenResponseIsConsideredFresh);

    PR_ExitMonitor(OCSP_Global.monitor);
}

static PRBool
ocsp_IsCacheItemFresh(OCSPCacheItem *cacheItem)
{
    PRTime now;
    PRBool retval;

    PR_EnterMonitor(OCSP_Global.monitor);
    now = PR_Now();
    retval = (cacheItem->nextFetchAttemptTime > now);
    OCSP_TRACE(("OCSP ocsp_IsCacheItemFresh: %d\n", retval));
    PR_ExitMonitor(OCSP_Global.monitor);
    return retval;
}





static SECStatus
ocsp_CreateOrUpdateCacheEntry(OCSPCacheData *cache, 
                              CERTOCSPCertID *certID,
                              CERTOCSPSingleResponse *single,
                              PRBool *certIDWasConsumed)
{
    SECStatus rv;
    OCSPCacheItem *cacheItem;
    OCSP_TRACE(("OCSP ocsp_CreateOrUpdateCacheEntry\n"));
  
    if (!certIDWasConsumed) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    *certIDWasConsumed = PR_FALSE;
  
    PR_EnterMonitor(OCSP_Global.monitor);
    PORT_Assert(OCSP_Global.maxCacheEntries >= 0);
  
    cacheItem = ocsp_FindCacheEntry(cache, certID);
    if (!cacheItem) {
        rv = ocsp_CreateCacheItemAndConsumeCertID(cache, certID, 
                                                  &cacheItem);
        if (rv != SECSuccess) {
            PR_ExitMonitor(OCSP_Global.monitor);
            return rv;
        }
        *certIDWasConsumed = PR_TRUE;
    }
    if (single) {
        rv = ocsp_SetCacheItemResponse(cacheItem, single);
        if (rv != SECSuccess) {
            ocsp_RemoveCacheItem(cache, cacheItem);
            PR_ExitMonitor(OCSP_Global.monitor);
            return rv;
        }
    } else {
        cacheItem->missingResponseError = PORT_GetError();
    }
    ocsp_FreshenCacheItemNextFetchAttemptTime(cacheItem);
    ocsp_CheckCacheSize(cache);

    PR_ExitMonitor(OCSP_Global.monitor);
    return SECSuccess;
}

extern SECStatus
CERT_SetOCSPFailureMode(SEC_OcspFailureMode ocspFailureMode)
{
    switch (ocspFailureMode) {
    case ocspMode_FailureIsVerificationFailure:
    case ocspMode_FailureIsNotAVerificationFailure:
        break;
    default:
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    PR_EnterMonitor(OCSP_Global.monitor);
    OCSP_Global.ocspFailureMode = ocspFailureMode;
    PR_ExitMonitor(OCSP_Global.monitor);
    return SECSuccess;
}

SECStatus
CERT_OCSPCacheSettings(PRInt32 maxCacheEntries,
                       PRUint32 minimumSecondsToNextFetchAttempt,
                       PRUint32 maximumSecondsToNextFetchAttempt)
{
    if (minimumSecondsToNextFetchAttempt > maximumSecondsToNextFetchAttempt
        || maxCacheEntries < -1) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
  
    PR_EnterMonitor(OCSP_Global.monitor);
  
    if (maxCacheEntries < 0) {
        OCSP_Global.maxCacheEntries = -1; 
    } else if (maxCacheEntries == 0) {
        OCSP_Global.maxCacheEntries = 0; 
    } else {
        OCSP_Global.maxCacheEntries = maxCacheEntries;
    }
  
    if (minimumSecondsToNextFetchAttempt < 
            OCSP_Global.minimumSecondsToNextFetchAttempt
        || maximumSecondsToNextFetchAttempt < 
            OCSP_Global.maximumSecondsToNextFetchAttempt) {
        



        CERT_ClearOCSPCache();
    }
  
    OCSP_Global.minimumSecondsToNextFetchAttempt = 
        minimumSecondsToNextFetchAttempt;
    OCSP_Global.maximumSecondsToNextFetchAttempt = 
        maximumSecondsToNextFetchAttempt;
    ocsp_CheckCacheSize(&OCSP_Global.cache);
  
    PR_ExitMonitor(OCSP_Global.monitor);
    return SECSuccess;
}

SECStatus
CERT_SetOCSPTimeout(PRUint32 seconds)
{
    
    OCSP_Global.timeoutSeconds = seconds;
    return SECSuccess;
}


SECStatus OCSP_InitGlobal(void)
{
    SECStatus rv = SECFailure;

    if (OCSP_Global.monitor == NULL) {
        OCSP_Global.monitor = PR_NewMonitor();
    }
    if (!OCSP_Global.monitor)
        return SECFailure;

    PR_EnterMonitor(OCSP_Global.monitor);
    if (!OCSP_Global.cache.entries) {
        OCSP_Global.cache.entries = 
            PL_NewHashTable(0, 
                            ocsp_CacheKeyHashFunction, 
                            ocsp_CacheKeyCompareFunction, 
                            PL_CompareValues, 
                            NULL, 
                            NULL);
        OCSP_Global.ocspFailureMode = ocspMode_FailureIsVerificationFailure;
        OCSP_Global.cache.numberOfEntries = 0;
        OCSP_Global.cache.MRUitem = NULL;
        OCSP_Global.cache.LRUitem = NULL;
    } else {
        



        PORT_Assert(OCSP_Global.cache.numberOfEntries == 0);
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
    }
    if (OCSP_Global.cache.entries)
        rv = SECSuccess;
    PR_ExitMonitor(OCSP_Global.monitor);
    return rv;
}

SECStatus OCSP_ShutdownGlobal(void)
{
    if (!OCSP_Global.monitor)
        return SECSuccess;

    PR_EnterMonitor(OCSP_Global.monitor);
    if (OCSP_Global.cache.entries) {
        CERT_ClearOCSPCache();
        PL_HashTableDestroy(OCSP_Global.cache.entries);
        OCSP_Global.cache.entries = NULL;
    }
    PORT_Assert(OCSP_Global.cache.numberOfEntries == 0);
    OCSP_Global.cache.MRUitem = NULL;
    OCSP_Global.cache.LRUitem = NULL;

    OCSP_Global.defaultHttpClientFcn = NULL;
    OCSP_Global.maxCacheEntries = DEFAULT_OCSP_CACHE_SIZE;
    OCSP_Global.minimumSecondsToNextFetchAttempt = 
      DEFAULT_MINIMUM_SECONDS_TO_NEXT_OCSP_FETCH_ATTEMPT;
    OCSP_Global.maximumSecondsToNextFetchAttempt =
      DEFAULT_MAXIMUM_SECONDS_TO_NEXT_OCSP_FETCH_ATTEMPT;
    OCSP_Global.ocspFailureMode =
      ocspMode_FailureIsVerificationFailure;
    PR_ExitMonitor(OCSP_Global.monitor);

    PR_DestroyMonitor(OCSP_Global.monitor);
    OCSP_Global.monitor = NULL;
    return SECSuccess;
}





const SEC_HttpClientFcn *SEC_GetRegisteredHttpClient()
{
    const SEC_HttpClientFcn *retval;

    if (!OCSP_Global.monitor) {
      PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
      return NULL;
    }

    PR_EnterMonitor(OCSP_Global.monitor);
    retval = OCSP_Global.defaultHttpClientFcn;
    PR_ExitMonitor(OCSP_Global.monitor);
    
    return retval;
}







typedef struct ocspCheckingContextStr {
    PRBool useDefaultResponder;
    char *defaultResponderURI;
    char *defaultResponderNickname;
    CERTCertificate *defaultResponderCert;
} ocspCheckingContext;

SEC_ASN1_MKSUB(SEC_AnyTemplate)
SEC_ASN1_MKSUB(SEC_IntegerTemplate)
SEC_ASN1_MKSUB(SEC_NullTemplate)
SEC_ASN1_MKSUB(SEC_OctetStringTemplate)
SEC_ASN1_MKSUB(SEC_PointerToAnyTemplate)
SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)
SEC_ASN1_MKSUB(SEC_SequenceOfAnyTemplate)
SEC_ASN1_MKSUB(SEC_PointerToGeneralizedTimeTemplate)
SEC_ASN1_MKSUB(SEC_PointerToEnumeratedTemplate)







extern const SEC_ASN1Template ocsp_CertIDTemplate[];
extern const SEC_ASN1Template ocsp_PointerToSignatureTemplate[];
extern const SEC_ASN1Template ocsp_PointerToResponseBytesTemplate[];
extern const SEC_ASN1Template ocsp_ResponseDataTemplate[];
extern const SEC_ASN1Template ocsp_RevokedInfoTemplate[];
extern const SEC_ASN1Template ocsp_SingleRequestTemplate[];
extern const SEC_ASN1Template ocsp_SingleResponseTemplate[];
extern const SEC_ASN1Template ocsp_TBSRequestTemplate[];











static const SEC_ASN1Template ocsp_OCSPRequestTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(CERTOCSPRequest) },
    { SEC_ASN1_POINTER,
	offsetof(CERTOCSPRequest, tbsRequest),
	ocsp_TBSRequestTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	offsetof(CERTOCSPRequest, optionalSignature),
	ocsp_PointerToSignatureTemplate },
    { 0 }
};














const SEC_ASN1Template ocsp_TBSRequestTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(ocspTBSRequest) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |		
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	offsetof(ocspTBSRequest, version),
	SEC_ASN1_SUB(SEC_IntegerTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 1,
	offsetof(ocspTBSRequest, derRequestorName),
	SEC_ASN1_SUB(SEC_PointerToAnyTemplate) },
    { SEC_ASN1_SEQUENCE_OF,
	offsetof(ocspTBSRequest, requestList),
	ocsp_SingleRequestTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 2,
	offsetof(ocspTBSRequest, requestExtensions),
	CERT_SequenceOfCertExtensionTemplate },
    { 0 }
};







static const SEC_ASN1Template ocsp_SignatureTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(ocspSignature) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	offsetof(ocspSignature, signatureAlgorithm),
	SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_BIT_STRING,
	offsetof(ocspSignature, signature) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	offsetof(ocspSignature, derCerts), 
	SEC_ASN1_SUB(SEC_SequenceOfAnyTemplate) },
    { 0 }
};









const SEC_ASN1Template ocsp_PointerToSignatureTemplate[] = {
    { SEC_ASN1_POINTER, 0, ocsp_SignatureTemplate }
};










const SEC_ASN1Template ocsp_SingleRequestTemplate[] = {
    { SEC_ASN1_SEQUENCE, 
	0, NULL, sizeof(ocspSingleRequest) },
    { SEC_ASN1_POINTER,
	offsetof(ocspSingleRequest, reqCert),
	ocsp_CertIDTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	offsetof(ocspSingleRequest, singleRequestExtensions),
	CERT_SequenceOfCertExtensionTemplate },
    { 0 }
};


















const SEC_ASN1Template ocsp_CertIDTemplate[] = {
    { SEC_ASN1_SEQUENCE, 
	0, NULL, sizeof(CERTOCSPCertID) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	offsetof(CERTOCSPCertID, hashAlgorithm),
	SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_OCTET_STRING,
	offsetof(CERTOCSPCertID, issuerNameHash) },
    { SEC_ASN1_OCTET_STRING,
	offsetof(CERTOCSPCertID, issuerKeyHash) },
    { SEC_ASN1_INTEGER, 
	offsetof(CERTOCSPCertID, serialNumber) },
    { 0 }
};











static const SEC_ASN1Template ocsp_OCSPResponseTemplate[] = {
    { SEC_ASN1_SEQUENCE, 
	0, NULL, sizeof(CERTOCSPResponse) },
    { SEC_ASN1_ENUMERATED, 
	offsetof(CERTOCSPResponse, responseStatus) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	offsetof(CERTOCSPResponse, responseBytes),
	ocsp_PointerToResponseBytesTemplate },
    { 0 }
};






static const SEC_ASN1Template ocsp_ResponseBytesTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(ocspResponseBytes) },
    { SEC_ASN1_OBJECT_ID,
	offsetof(ocspResponseBytes, responseType) },
    { SEC_ASN1_OCTET_STRING,
	offsetof(ocspResponseBytes, response) },
    { 0 }
};









const SEC_ASN1Template ocsp_PointerToResponseBytesTemplate[] = {
    { SEC_ASN1_POINTER, 0, ocsp_ResponseBytesTemplate }
};








static const SEC_ASN1Template ocsp_BasicOCSPResponseTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(ocspBasicOCSPResponse) },
    { SEC_ASN1_ANY | SEC_ASN1_SAVE,
	offsetof(ocspBasicOCSPResponse, tbsResponseDataDER) },
    { SEC_ASN1_POINTER,
	offsetof(ocspBasicOCSPResponse, tbsResponseData),
	ocsp_ResponseDataTemplate },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	offsetof(ocspBasicOCSPResponse, responseSignature.signatureAlgorithm),
	SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_BIT_STRING,
	offsetof(ocspBasicOCSPResponse, responseSignature.signature) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	offsetof(ocspBasicOCSPResponse, responseSignature.derCerts),
	SEC_ASN1_SUB(SEC_SequenceOfAnyTemplate) },
    { 0 }
};













const SEC_ASN1Template ocsp_ResponseDataTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(ocspResponseData) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |		
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	offsetof(ocspResponseData, version),
	SEC_ASN1_SUB(SEC_IntegerTemplate) },
    { SEC_ASN1_ANY,
	offsetof(ocspResponseData, derResponderID) },
    { SEC_ASN1_GENERALIZED_TIME,
	offsetof(ocspResponseData, producedAt) },
    { SEC_ASN1_SEQUENCE_OF,
	offsetof(ocspResponseData, responses),
	ocsp_SingleResponseTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1,
	offsetof(ocspResponseData, responseExtensions),
	CERT_SequenceOfCertExtensionTemplate },
    { 0 }
};
















static const SEC_ASN1Template ocsp_ResponderIDByNameTemplate[] = {
    { SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1,
	offsetof(ocspResponderID, responderIDValue.name),
	CERT_NameTemplate }
};
static const SEC_ASN1Template ocsp_ResponderIDByKeyTemplate[] = {
    { SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        SEC_ASN1_XTRN | 2,
	offsetof(ocspResponderID, responderIDValue.keyHash),
	SEC_ASN1_SUB(SEC_OctetStringTemplate) }
};
static const SEC_ASN1Template ocsp_ResponderIDOtherTemplate[] = {
    { SEC_ASN1_ANY,
	offsetof(ocspResponderID, responderIDValue.other) }
};


static const SEC_ASN1Template ocsp_ResponderIDDerNameTemplate[] = {
    { SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        SEC_ASN1_XTRN | 1, 0, SEC_ASN1_SUB(SEC_AnyTemplate) }
};













const SEC_ASN1Template ocsp_SingleResponseTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(CERTOCSPSingleResponse) },
    { SEC_ASN1_POINTER,
	offsetof(CERTOCSPSingleResponse, certID),
	ocsp_CertIDTemplate },
    { SEC_ASN1_ANY,
	offsetof(CERTOCSPSingleResponse, derCertStatus) },
    { SEC_ASN1_GENERALIZED_TIME,
	offsetof(CERTOCSPSingleResponse, thisUpdate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	offsetof(CERTOCSPSingleResponse, nextUpdate),
	SEC_ASN1_SUB(SEC_PointerToGeneralizedTimeTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1,
	offsetof(CERTOCSPSingleResponse, singleExtensions),
	CERT_SequenceOfCertExtensionTemplate },
    { 0 }
};














static const SEC_ASN1Template ocsp_CertStatusGoodTemplate[] = {
    { SEC_ASN1_POINTER | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	offsetof(ocspCertStatus, certStatusInfo.goodInfo),
	SEC_ASN1_SUB(SEC_NullTemplate) }
};
static const SEC_ASN1Template ocsp_CertStatusRevokedTemplate[] = {
    { SEC_ASN1_POINTER | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 1, 
	offsetof(ocspCertStatus, certStatusInfo.revokedInfo),
	ocsp_RevokedInfoTemplate }
};
static const SEC_ASN1Template ocsp_CertStatusUnknownTemplate[] = {
    { SEC_ASN1_POINTER | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 2,
	offsetof(ocspCertStatus, certStatusInfo.unknownInfo),
	SEC_ASN1_SUB(SEC_NullTemplate) }
};
static const SEC_ASN1Template ocsp_CertStatusOtherTemplate[] = {
    { SEC_ASN1_POINTER | SEC_ASN1_XTRN,
	offsetof(ocspCertStatus, certStatusInfo.otherInfo),
	SEC_ASN1_SUB(SEC_AnyTemplate) }
};










const SEC_ASN1Template ocsp_RevokedInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(ocspRevokedInfo) },
    { SEC_ASN1_GENERALIZED_TIME,
	offsetof(ocspRevokedInfo, revocationTime) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_EXPLICIT |
      SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        SEC_ASN1_XTRN | 0,
	offsetof(ocspRevokedInfo, revocationReason), 
	SEC_ASN1_SUB(SEC_PointerToEnumeratedTemplate) },
    { 0 }
};











static const SEC_ASN1Template ocsp_ServiceLocatorTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(ocspServiceLocator) },
    { SEC_ASN1_POINTER,
	offsetof(ocspServiceLocator, issuer),
	CERT_NameTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_ANY,
	offsetof(ocspServiceLocator, locator) },
    { 0 }
};
























SECItem *
CERT_EncodeOCSPRequest(PRArenaPool *arena, CERTOCSPRequest *request, 
		       void *pwArg)
{
    ocspTBSRequest *tbsRequest;
    SECStatus rv;

    
    PORT_Assert(request);
    PORT_Assert(request->tbsRequest);

    tbsRequest = request->tbsRequest;

    if (request->tbsRequest->extensionHandle != NULL) {
	rv = CERT_FinishExtensions(request->tbsRequest->extensionHandle);
	request->tbsRequest->extensionHandle = NULL;
	if (rv != SECSuccess)
	    return NULL;
    }

    









    return SEC_ASN1EncodeItem(arena, NULL, request, ocsp_OCSPRequestTemplate);
}













CERTOCSPRequest *
CERT_DecodeOCSPRequest(SECItem *src)
{
    PRArenaPool *arena = NULL;
    SECStatus rv = SECFailure;
    CERTOCSPRequest *dest = NULL;
    int i;
    SECItem newSrc;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto loser;
    }
    dest = (CERTOCSPRequest *) PORT_ArenaZAlloc(arena, 
						sizeof(CERTOCSPRequest));
    if (dest == NULL) {
	goto loser;
    }
    dest->arena = arena;

    

    rv = SECITEM_CopyItem(arena, &newSrc, src);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = SEC_QuickDERDecodeItem(arena, dest, ocsp_OCSPRequestTemplate, &newSrc);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_BAD_DER)
	    PORT_SetError(SEC_ERROR_OCSP_MALFORMED_REQUEST);
	goto loser;
    }

    



    for (i = 0; dest->tbsRequest->requestList[i] != NULL; i++) {
	dest->tbsRequest->requestList[i]->arena = arena;
    }

    return dest;

loser:
    if (arena != NULL) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return NULL;
}

SECStatus
CERT_DestroyOCSPCertID(CERTOCSPCertID* certID)
{
    if (certID && certID->poolp) {
	PORT_FreeArena(certID->poolp, PR_FALSE);
	return SECSuccess;
    }
    PORT_SetError(SEC_ERROR_INVALID_ARGS);
    return SECFailure;
}









static SECItem *
ocsp_DigestValue(PRArenaPool *arena, SECOidTag digestAlg, 
                 SECItem *fill, const SECItem *src)
{
    const SECHashObject *digestObject;
    SECItem *result = NULL;
    void *mark = NULL;
    void *digestBuff = NULL;

    if ( arena != NULL ) {
        mark = PORT_ArenaMark(arena);
    }

    digestObject = HASH_GetHashObjectByOidTag(digestAlg);
    if ( digestObject == NULL ) {
        goto loser;
    }

    if (fill == NULL || fill->data == NULL) {
	result = SECITEM_AllocItem(arena, fill, digestObject->length);
	if ( result == NULL ) {
	   goto loser;
	}
	digestBuff = result->data;
    } else {
	if (fill->len < digestObject->length) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    goto loser;
	}
	digestBuff = fill->data;
    }

    if (PK11_HashBuf(digestAlg, digestBuff,
                     src->data, src->len) != SECSuccess) {
        goto loser;
    }

    if ( arena != NULL ) {
        PORT_ArenaUnmark(arena, mark);
    }

    if (result == NULL) {
        result = fill;
    }
    return result;

loser:
    if (arena != NULL) {
        PORT_ArenaRelease(arena, mark);
    } else {
        if (result != NULL) {
            SECITEM_FreeItem(result, (fill == NULL) ? PR_TRUE : PR_FALSE);
        }
    }
    return(NULL);
}








SECItem *
CERT_GetSPKIDigest(PRArenaPool *arena, const CERTCertificate *cert,
                           SECOidTag digestAlg, SECItem *fill)
{
    SECItem spk;

    




    spk = cert->subjectPublicKeyInfo.subjectPublicKey;
    DER_ConvertBitString(&spk);

    return ocsp_DigestValue(arena, digestAlg, fill, &spk);
}




static SECItem *
cert_GetSubjectNameDigest(PRArenaPool *arena, const CERTCertificate *cert,
                           SECOidTag digestAlg, SECItem *fill)
{
    SECItem name;

    



    name = cert->derSubject;

    return ocsp_DigestValue(arena, digestAlg, fill, &name);
}











static CERTOCSPCertID *
ocsp_CreateCertID(PRArenaPool *arena, CERTCertificate *cert, int64 time)
{
    CERTOCSPCertID *certID;
    CERTCertificate *issuerCert = NULL;
    void *mark = PORT_ArenaMark(arena);
    SECStatus rv;

    PORT_Assert(arena != NULL);

    certID = PORT_ArenaZNew(arena, CERTOCSPCertID);
    if (certID == NULL) {
	goto loser;
    }

    rv = SECOID_SetAlgorithmID(arena, &certID->hashAlgorithm, SEC_OID_SHA1,
			       NULL);
    if (rv != SECSuccess) {
	goto loser; 
    }

    issuerCert = CERT_FindCertIssuer(cert, time, certUsageAnyCA);
    if (issuerCert == NULL) {
	goto loser;
    }

    if (cert_GetSubjectNameDigest(arena, issuerCert, SEC_OID_SHA1,
                                  &(certID->issuerNameHash)) == NULL) {
        goto loser;
    }
    certID->issuerSHA1NameHash.data = certID->issuerNameHash.data;
    certID->issuerSHA1NameHash.len = certID->issuerNameHash.len;

    if (cert_GetSubjectNameDigest(arena, issuerCert, SEC_OID_MD5,
                                  &(certID->issuerMD5NameHash)) == NULL) {
        goto loser;
    }

    if (cert_GetSubjectNameDigest(arena, issuerCert, SEC_OID_MD2,
                                  &(certID->issuerMD2NameHash)) == NULL) {
        goto loser;
    }

    if (CERT_GetSPKIDigest(arena, issuerCert, SEC_OID_SHA1,
				   &(certID->issuerKeyHash)) == NULL) {
	goto loser;
    }
    certID->issuerSHA1KeyHash.data = certID->issuerKeyHash.data;
    certID->issuerSHA1KeyHash.len = certID->issuerKeyHash.len;
    
    if (CERT_GetSPKIDigest(arena, issuerCert, SEC_OID_MD5,
				   &(certID->issuerMD5KeyHash)) == NULL) {
	goto loser;
    }
    if (CERT_GetSPKIDigest(arena, issuerCert, SEC_OID_MD2,
				   &(certID->issuerMD2KeyHash)) == NULL) {
	goto loser;
    }


    
    CERT_DestroyCertificate(issuerCert);
    issuerCert = NULL;

    rv = SECITEM_CopyItem(arena, &certID->serialNumber, &cert->serialNumber);
    if (rv != SECSuccess) {
	goto loser; 
    }

    PORT_ArenaUnmark(arena, mark);
    return certID;

loser:
    if (issuerCert != NULL) {
	CERT_DestroyCertificate(issuerCert);
    }
    PORT_ArenaRelease(arena, mark);
    return NULL;
}

CERTOCSPCertID*
CERT_CreateOCSPCertID(CERTCertificate *cert, int64 time)
{
    PRArenaPool *arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    CERTOCSPCertID *certID;
    PORT_Assert(arena != NULL);
    if (!arena)
	return NULL;
    
    certID = ocsp_CreateCertID(arena, cert, time);
    if (!certID) {
	PORT_FreeArena(arena, PR_FALSE);
	return NULL;
    }
    certID->poolp = arena;
    return certID;
}




void SetSingleReqExts(void *object, CERTCertExtension **exts)
{
  ocspSingleRequest *singleRequest =
    (ocspSingleRequest *)object;

  singleRequest->singleRequestExtensions = exts;
}







static SECStatus
ocsp_AddServiceLocatorExtension(ocspSingleRequest *singleRequest,
				CERTCertificate *cert)
{
    ocspServiceLocator *serviceLocator = NULL;
    void *extensionHandle = NULL;
    SECStatus rv = SECFailure;

    serviceLocator = PORT_ZNew(ocspServiceLocator);
    if (serviceLocator == NULL)
	goto loser;

    






    serviceLocator->issuer = &cert->issuer;

    rv = CERT_FindCertExtension(cert, SEC_OID_X509_AUTH_INFO_ACCESS,
				&serviceLocator->locator);
    if (rv != SECSuccess) {
	if (PORT_GetError() != SEC_ERROR_EXTENSION_NOT_FOUND)
	    goto loser;
    }

    
    rv = SECFailure;
    PORT_SetError(0);

    extensionHandle = cert_StartExtensions(singleRequest,
                       singleRequest->arena, SetSingleReqExts);
    if (extensionHandle == NULL)
	goto loser;

    rv = CERT_EncodeAndAddExtension(extensionHandle,
				    SEC_OID_PKIX_OCSP_SERVICE_LOCATOR,
				    serviceLocator, PR_FALSE,
				    ocsp_ServiceLocatorTemplate);

loser:
    if (extensionHandle != NULL) {
	



	SECStatus tmprv = CERT_FinishExtensions(extensionHandle);
	if (rv == SECSuccess)
	    rv = tmprv;
    }

    


    if (serviceLocator != NULL) {
	if (serviceLocator->locator.data != NULL)
	    SECITEM_FreeItem(&serviceLocator->locator, PR_FALSE);
	PORT_Free(serviceLocator);
    }

    return rv;
}













static ocspSingleRequest **
ocsp_CreateSingleRequestList(PRArenaPool *arena, CERTCertList *certList,
                             int64 time, PRBool includeLocator)
{
    ocspSingleRequest **requestList = NULL;
    CERTCertListNode *node = NULL;
    int i, count;
    void *mark = PORT_ArenaMark(arena);
 
    node = CERT_LIST_HEAD(certList);
    for (count = 0; !CERT_LIST_END(node, certList); count++) {
        node = CERT_LIST_NEXT(node);
    }

    if (count == 0)
	goto loser;

    requestList = PORT_ArenaNewArray(arena, ocspSingleRequest *, count + 1);
    if (requestList == NULL)
	goto loser;

    node = CERT_LIST_HEAD(certList);
    for (i = 0; !CERT_LIST_END(node, certList); i++) {
        requestList[i] = PORT_ArenaZNew(arena, ocspSingleRequest);
        if (requestList[i] == NULL)
            goto loser;

        OCSP_TRACE(("OCSP CERT_CreateOCSPRequest %s\n", node->cert->subjectName));
        requestList[i]->arena = arena;
        requestList[i]->reqCert = ocsp_CreateCertID(arena, node->cert, time);
        if (requestList[i]->reqCert == NULL)
            goto loser;

        if (includeLocator == PR_TRUE) {
            SECStatus rv;

            rv = ocsp_AddServiceLocatorExtension(requestList[i], node->cert);
            if (rv != SECSuccess)
                goto loser;
        }

        node = CERT_LIST_NEXT(node);
    }

    PORT_Assert(i == count);

    PORT_ArenaUnmark(arena, mark);
    requestList[i] = NULL;
    return requestList;

loser:
    PORT_ArenaRelease(arena, mark);
    return NULL;
}

static ocspSingleRequest **
ocsp_CreateRequestFromCert(PRArenaPool *arena, 
                           CERTOCSPCertID *certID, 
                           CERTCertificate *singleCert,
                           int64 time, 
                           PRBool includeLocator)
{
    ocspSingleRequest **requestList = NULL;
    void *mark = PORT_ArenaMark(arena);
    PORT_Assert(certID != NULL && singleCert != NULL);

    
    requestList = PORT_ArenaNewArray(arena, ocspSingleRequest *, 2);
    if (requestList == NULL)
        goto loser;
    requestList[0] = PORT_ArenaZNew(arena, ocspSingleRequest);
    if (requestList[0] == NULL)
        goto loser;
    requestList[0]->arena = arena;
    
    requestList[0]->reqCert = certID; 

    if (includeLocator == PR_TRUE) {
        SECStatus rv;
        rv = ocsp_AddServiceLocatorExtension(requestList[0], singleCert);
        if (rv != SECSuccess)
            goto loser;
    }

    PORT_ArenaUnmark(arena, mark);
    requestList[1] = NULL;
    return requestList;

loser:
    PORT_ArenaRelease(arena, mark);
    return NULL;
}

static CERTOCSPRequest *
ocsp_prepareEmptyOCSPRequest()
{
    PRArenaPool *arena = NULL;
    CERTOCSPRequest *request = NULL;
    ocspTBSRequest *tbsRequest = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
        goto loser;
    }
    request = PORT_ArenaZNew(arena, CERTOCSPRequest);
    if (request == NULL) {
        goto loser;
    }
    request->arena = arena;

    tbsRequest = PORT_ArenaZNew(arena, ocspTBSRequest);
    if (tbsRequest == NULL) {
        goto loser;
    }
    request->tbsRequest = tbsRequest;
    
    return request;

loser:
    if (arena != NULL) {
        PORT_FreeArena(arena, PR_FALSE);
    }
    return NULL;
}

CERTOCSPRequest *
cert_CreateSingleCertOCSPRequest(CERTOCSPCertID *certID, 
                                 CERTCertificate *singleCert, 
                                 int64 time, 
                                 PRBool addServiceLocator,
                                 CERTCertificate *signerCert)
{
    CERTOCSPRequest *request;
    OCSP_TRACE(("OCSP cert_CreateSingleCertOCSPRequest %s\n", singleCert->subjectName));

    


    if (signerCert != NULL) {
        PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
        return NULL;
    }

    request = ocsp_prepareEmptyOCSPRequest();
    if (!request)
        return NULL;
    



    request->tbsRequest->requestList = 
        ocsp_CreateRequestFromCert(request->arena, 
                                   certID,
                                   singleCert,
                                   time,
                                   addServiceLocator);
    if (request->tbsRequest->requestList == NULL) {
        PORT_FreeArena(request->arena, PR_FALSE);
        return NULL;
    }
    return request;
}
































CERTOCSPRequest *
CERT_CreateOCSPRequest(CERTCertList *certList, int64 time, 
		       PRBool addServiceLocator,
		       CERTCertificate *signerCert)
{
    CERTOCSPRequest *request = NULL;

    if (!certList) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }
    






    if (signerCert != NULL) {
        PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
        return NULL;
    }
    request = ocsp_prepareEmptyOCSPRequest();
    if (!request)
        return NULL;
    


    request->tbsRequest->requestList = 
        ocsp_CreateSingleRequestList(request->arena, 
                                     certList,
                                     time,
                                     addServiceLocator);
    if (request->tbsRequest->requestList == NULL) {
        PORT_FreeArena(request->arena, PR_FALSE);
        return NULL;
    }
    return request;
}


















void SetRequestExts(void *object, CERTCertExtension **exts)
{
  CERTOCSPRequest *request = (CERTOCSPRequest *)object;

  request->tbsRequest->requestExtensions = exts;
}

SECStatus
CERT_AddOCSPAcceptableResponses(CERTOCSPRequest *request,
				SECOidTag responseType0, ...)
{
    void *extHandle;
    va_list ap;
    int i, count;
    SECOidTag responseType;
    SECOidData *responseOid;
    SECItem **acceptableResponses = NULL;
    SECStatus rv = SECFailure;

    extHandle = request->tbsRequest->extensionHandle;
    if (extHandle == NULL) {
	extHandle = cert_StartExtensions(request, request->arena, SetRequestExts);
	if (extHandle == NULL)
	    goto loser;
    }

    
    count = 1;
    if (responseType0 != SEC_OID_PKIX_OCSP_BASIC_RESPONSE) {
	va_start(ap, responseType0);
	do {
	    count++;
	    responseType = va_arg(ap, SECOidTag);
	} while (responseType != SEC_OID_PKIX_OCSP_BASIC_RESPONSE);
	va_end(ap);
    }

    acceptableResponses = PORT_NewArray(SECItem *, count + 1);
    if (acceptableResponses == NULL)
	goto loser;

    i = 0;
    responseOid = SECOID_FindOIDByTag(responseType0);
    acceptableResponses[i++] = &(responseOid->oid);
    if (count > 1) {
	va_start(ap, responseType0);
	for ( ; i < count; i++) {
	    responseType = va_arg(ap, SECOidTag);
	    responseOid = SECOID_FindOIDByTag(responseType);
	    acceptableResponses[i] = &(responseOid->oid);
	}
	va_end(ap);
    }
    acceptableResponses[i] = NULL;

    rv = CERT_EncodeAndAddExtension(extHandle, SEC_OID_PKIX_OCSP_RESPONSE,
                                &acceptableResponses, PR_FALSE,
                                SEC_ASN1_GET(SEC_SequenceOfObjectIDTemplate));
    if (rv != SECSuccess)
	goto loser;

    PORT_Free(acceptableResponses);
    if (request->tbsRequest->extensionHandle == NULL)
	request->tbsRequest->extensionHandle = extHandle;
    return SECSuccess;

loser:
    if (acceptableResponses != NULL)
	PORT_Free(acceptableResponses);
    if (extHandle != NULL)
	(void) CERT_FinishExtensions(extHandle);
    return rv;
}











void
CERT_DestroyOCSPRequest(CERTOCSPRequest *request)
{
    if (request == NULL)
	return;

    if (request->tbsRequest != NULL) {
	if (request->tbsRequest->requestorName != NULL)
	    CERT_DestroyGeneralNameList(request->tbsRequest->requestorName);
	if (request->tbsRequest->extensionHandle != NULL)
	    (void) CERT_FinishExtensions(request->tbsRequest->extensionHandle);
    }

    if (request->optionalSignature != NULL) {
	if (request->optionalSignature->cert != NULL)
	    CERT_DestroyCertificate(request->optionalSignature->cert);

	





    }

    




    PORT_Assert(request->arena != NULL);
    if (request->arena != NULL)
	PORT_FreeArena(request->arena, PR_FALSE);
}










static const SEC_ASN1Template *
ocsp_ResponderIDTemplateByType(ocspResponderIDType responderIDType)
{
    const SEC_ASN1Template *responderIDTemplate;

    switch (responderIDType) {
	case ocspResponderID_byName:
	    responderIDTemplate = ocsp_ResponderIDByNameTemplate;
	    break;
	case ocspResponderID_byKey:
	    responderIDTemplate = ocsp_ResponderIDByKeyTemplate;
	    break;
	case ocspResponderID_other:
	default:
	    PORT_Assert(responderIDType == ocspResponderID_other);
	    responderIDTemplate = ocsp_ResponderIDOtherTemplate;
	    break;
    }

    return responderIDTemplate;
}





static const SEC_ASN1Template *
ocsp_CertStatusTemplateByType(ocspCertStatusType certStatusType)
{
    const SEC_ASN1Template *certStatusTemplate;

    switch (certStatusType) {
	case ocspCertStatus_good:
	    certStatusTemplate = ocsp_CertStatusGoodTemplate;
	    break;
	case ocspCertStatus_revoked:
	    certStatusTemplate = ocsp_CertStatusRevokedTemplate;
	    break;
	case ocspCertStatus_unknown:
	    certStatusTemplate = ocsp_CertStatusUnknownTemplate;
	    break;
	case ocspCertStatus_other:
	default:
	    PORT_Assert(certStatusType == ocspCertStatus_other);
	    certStatusTemplate = ocsp_CertStatusOtherTemplate;
	    break;
    }

    return certStatusTemplate;
}





static ocspCertStatusType
ocsp_CertStatusTypeByTag(int derTag)
{
    ocspCertStatusType certStatusType;

    switch (derTag) {
	case 0:
	    certStatusType = ocspCertStatus_good;
	    break;
	case 1:
	    certStatusType = ocspCertStatus_revoked;
	    break;
	case 2:
	    certStatusType = ocspCertStatus_unknown;
	    break;
	default:
	    certStatusType = ocspCertStatus_other;
	    break;
    }

    return certStatusType;
}








static SECStatus
ocsp_FinishDecodingSingleResponses(PRArenaPool *reqArena,
				   CERTOCSPSingleResponse **responses)
{
    ocspCertStatus *certStatus;
    ocspCertStatusType certStatusType;
    const SEC_ASN1Template *certStatusTemplate;
    int derTag;
    int i;
    SECStatus rv = SECFailure;

    if (!reqArena) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (responses == NULL)			
	return SECSuccess;

    for (i = 0; responses[i] != NULL; i++) {
        SECItem* newStatus;
	



	PORT_Assert(responses[i]->derCertStatus.data != NULL);

	derTag = responses[i]->derCertStatus.data[0] & SEC_ASN1_TAGNUM_MASK;
	certStatusType = ocsp_CertStatusTypeByTag(derTag);
	certStatusTemplate = ocsp_CertStatusTemplateByType(certStatusType);

	certStatus = PORT_ArenaZAlloc(reqArena, sizeof(ocspCertStatus));
	if (certStatus == NULL) {
	    goto loser;
	}
        newStatus = SECITEM_ArenaDupItem(reqArena, &responses[i]->derCertStatus);
        if (!newStatus) {
            goto loser;
        }
	rv = SEC_QuickDERDecodeItem(reqArena, certStatus, certStatusTemplate,
				newStatus);
	if (rv != SECSuccess) {
	    if (PORT_GetError() == SEC_ERROR_BAD_DER)
		PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
	    goto loser;
	}

	certStatus->certStatusType = certStatusType;
	responses[i]->certStatus = certStatus;
    }

    return SECSuccess;

loser:
    return rv;
}





static ocspResponderIDType
ocsp_ResponderIDTypeByTag(int derTag)
{
    ocspResponderIDType responderIDType;

    switch (derTag) {
	case 1:
	    responderIDType = ocspResponderID_byName;
	    break;
	case 2:
	    responderIDType = ocspResponderID_byKey;
	    break;
	default:
	    responderIDType = ocspResponderID_other;
	    break;
    }

    return responderIDType;
}




static ocspBasicOCSPResponse *
ocsp_DecodeBasicOCSPResponse(PRArenaPool *arena, SECItem *src)
{
    void *mark;
    ocspBasicOCSPResponse *basicResponse;
    ocspResponseData *responseData;
    ocspResponderID *responderID;
    ocspResponderIDType responderIDType;
    const SEC_ASN1Template *responderIDTemplate;
    int derTag;
    SECStatus rv;
    SECItem newsrc;

    mark = PORT_ArenaMark(arena);

    basicResponse = PORT_ArenaZAlloc(arena, sizeof(ocspBasicOCSPResponse));
    if (basicResponse == NULL) {
	goto loser;
    }

    

    rv = SECITEM_CopyItem(arena, &newsrc, src);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = SEC_QuickDERDecodeItem(arena, basicResponse,
			    ocsp_BasicOCSPResponseTemplate, &newsrc);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_BAD_DER)
	    PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
	goto loser;
    }

    responseData = basicResponse->tbsResponseData;

    



    PORT_Assert(responseData != NULL);
    PORT_Assert(responseData->derResponderID.data != NULL);

    



    derTag = responseData->derResponderID.data[0] & SEC_ASN1_TAGNUM_MASK;
    responderIDType = ocsp_ResponderIDTypeByTag(derTag);
    responderIDTemplate = ocsp_ResponderIDTemplateByType(responderIDType);

    responderID = PORT_ArenaZAlloc(arena, sizeof(ocspResponderID));
    if (responderID == NULL) {
	goto loser;
    }

    rv = SEC_QuickDERDecodeItem(arena, responderID, responderIDTemplate,
			    &responseData->derResponderID);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_BAD_DER)
	    PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
	goto loser;
    }

    responderID->responderIDType = responderIDType;
    responseData->responderID = responderID;

    



    rv = ocsp_FinishDecodingSingleResponses(arena, responseData->responses);
    if (rv != SECSuccess) {
	goto loser;
    }

    PORT_ArenaUnmark(arena, mark);
    return basicResponse;

loser:
    PORT_ArenaRelease(arena, mark);
    return NULL;
}






static SECStatus
ocsp_DecodeResponseBytes(PRArenaPool *arena, ocspResponseBytes *rbytes)
{
    PORT_Assert(rbytes != NULL);		
    if (rbytes == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);	
	return SECFailure;
    }

    rbytes->responseTypeTag = SECOID_FindOIDTag(&rbytes->responseType);
    switch (rbytes->responseTypeTag) {
	case SEC_OID_PKIX_OCSP_BASIC_RESPONSE:
	    {
		ocspBasicOCSPResponse *basicResponse;

		basicResponse = ocsp_DecodeBasicOCSPResponse(arena,
							     &rbytes->response);
		if (basicResponse == NULL)
		    return SECFailure;

		rbytes->decodedResponse.basic = basicResponse;
	    }
	    break;

	



	default:
	    PORT_SetError(SEC_ERROR_OCSP_UNKNOWN_RESPONSE_TYPE);
	    return SECFailure;
    }

    return SECSuccess;
}















CERTOCSPResponse *
CERT_DecodeOCSPResponse(SECItem *src)
{
    PRArenaPool *arena = NULL;
    CERTOCSPResponse *response = NULL;
    SECStatus rv = SECFailure;
    ocspResponseStatus sv;
    SECItem newSrc;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto loser;
    }
    response = (CERTOCSPResponse *) PORT_ArenaZAlloc(arena,
						     sizeof(CERTOCSPResponse));
    if (response == NULL) {
	goto loser;
    }
    response->arena = arena;

    

    rv = SECITEM_CopyItem(arena, &newSrc, src);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    rv = SEC_QuickDERDecodeItem(arena, response, ocsp_OCSPResponseTemplate, &newSrc);
    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_BAD_DER)
	    PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
	goto loser;
    }

    sv = (ocspResponseStatus) DER_GetInteger(&response->responseStatus);
    response->statusValue = sv;
    if (sv != ocspResponse_successful) {
	



	return response;
    }

    



    rv = ocsp_DecodeResponseBytes(arena, response->responseBytes);
    if (rv != SECSuccess) {
	goto loser;
    }

    return response;

loser:
    if (arena != NULL) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return NULL;
}






























ocspResponseData *
ocsp_GetResponseData(CERTOCSPResponse *response, SECItem **tbsResponseDataDER)
{
    ocspBasicOCSPResponse *basic;
    ocspResponseData *responseData;

    PORT_Assert(response != NULL);

    PORT_Assert(response->responseBytes != NULL);

    PORT_Assert(response->responseBytes->responseTypeTag
		== SEC_OID_PKIX_OCSP_BASIC_RESPONSE);

    basic = response->responseBytes->decodedResponse.basic;
    PORT_Assert(basic != NULL);

    responseData = basic->tbsResponseData;
    PORT_Assert(responseData != NULL);

    if (tbsResponseDataDER) {
        *tbsResponseDataDER = &basic->tbsResponseDataDER;

        PORT_Assert((*tbsResponseDataDER)->data != NULL);
        PORT_Assert((*tbsResponseDataDER)->len != 0);
    }

    return responseData;
}





ocspSignature *
ocsp_GetResponseSignature(CERTOCSPResponse *response)
{
    ocspBasicOCSPResponse *basic;

    PORT_Assert(response != NULL);
    if (NULL == response->responseBytes) {
        return NULL;
    }
    PORT_Assert(response->responseBytes != NULL);
    PORT_Assert(response->responseBytes->responseTypeTag
		== SEC_OID_PKIX_OCSP_BASIC_RESPONSE);

    basic = response->responseBytes->decodedResponse.basic;
    PORT_Assert(basic != NULL);

    return &(basic->responseSignature);
}











void
CERT_DestroyOCSPResponse(CERTOCSPResponse *response)
{
    if (response != NULL) {
	ocspSignature *signature = ocsp_GetResponseSignature(response);
	if (signature && signature->cert != NULL)
	    CERT_DestroyCertificate(signature->cert);

	




	PORT_Assert(response->arena != NULL);
	if (response->arena != NULL) {
	    PORT_FreeArena(response->arena, PR_FALSE);
	}
    }
}



















static SECStatus
ocsp_ParseURL(const char *url, char **pHostname, PRUint16 *pPort, char **pPath)
{
    unsigned short port = 80;		
    char *hostname = NULL;
    char *path = NULL;
    const char *save;
    char c;
    int len;

    if (url == NULL)
	goto loser;

    


    c = *url;
    while ((c == ' ' || c == '\t') && c != '\0') {
	url++;
	c = *url;
    }
    if (c == '\0')
	goto loser;

    



    if (PORT_Strncasecmp(url, "http://", 7) != 0)
	goto loser;
    url += 7;

    











    save = url;
    c = *url;
    while (c != '/' && c != ':' && c != '\0' && c != ' ' && c != '\t') {
	url++;
	c = *url;
    }
    len = url - save;
    hostname = PORT_Alloc(len + 1);
    if (hostname == NULL)
	goto loser;
    PORT_Memcpy(hostname, save, len);
    hostname[len] = '\0';

    



    if (c == ':') {
	url++;
	port = (unsigned short) PORT_Atoi(url);
	c = *url;
	while (c != '/' && c != '\0' && c != ' ' && c != '\t') {
	    if (c < '0' || c > '9')
		goto loser;
	    url++;
	    c = *url;
	}
    }

    



    if (c == '/') {
	save = url;
	while (c != '\0' && c != ' ' && c != '\t') {
	    url++;
	    c = *url;
	}
	len = url - save;
	path = PORT_Alloc(len + 1);
	if (path == NULL)
	    goto loser;
	PORT_Memcpy(path, save, len);
	path[len] = '\0';
    } else {
	path = PORT_Strdup("/");
	if (path == NULL)
	    goto loser;
    }

    *pHostname = hostname;
    *pPort = port;
    *pPath = path;
    return SECSuccess;

loser:
    if (hostname != NULL)
	PORT_Free(hostname);
    PORT_SetError(SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
    return SECFailure;
}





static PRFileDesc *
ocsp_ConnectToHost(const char *host, PRUint16 port)
{
    PRFileDesc *sock = NULL;
    PRIntervalTime timeout;
    PRNetAddr addr;
    char *netdbbuf = NULL;

    sock = PR_NewTCPSocket();
    if (sock == NULL)
	goto loser;

    
    timeout = PR_SecondsToInterval(30);

    







    if (PR_StringToNetAddr(host, &addr) != PR_SUCCESS) {
	PRIntn hostIndex;
	PRHostEnt hostEntry;

	netdbbuf = PORT_Alloc(PR_NETDB_BUF_SIZE);
	if (netdbbuf == NULL)
	    goto loser;

	if (PR_GetHostByName(host, netdbbuf, PR_NETDB_BUF_SIZE,
			     &hostEntry) != PR_SUCCESS)
	    goto loser;

	hostIndex = 0;
	do {
	    hostIndex = PR_EnumerateHostEnt(hostIndex, &hostEntry, port, &addr);
	    if (hostIndex <= 0)
		goto loser;
	} while (PR_Connect(sock, &addr, timeout) != PR_SUCCESS);

	PORT_Free(netdbbuf);
    } else {
	


	if (PR_InitializeNetAddr(PR_IpAddrNull, port, &addr) != PR_SUCCESS)
	    goto loser;
	if (PR_Connect(sock, &addr, timeout) != PR_SUCCESS)
	    goto loser;
    }

    return sock;

loser:
    if (sock != NULL)
	PR_Close(sock);
    if (netdbbuf != NULL)
	PORT_Free(netdbbuf);
    return NULL;
}









static PRFileDesc *
ocsp_SendEncodedRequest(char *location, SECItem *encodedRequest)
{
    char *hostname = NULL;
    char *path = NULL;
    PRUint16 port;
    SECStatus rv;
    PRFileDesc *sock = NULL;
    PRFileDesc *returnSock = NULL;
    char *header = NULL;

    


    rv = ocsp_ParseURL(location, &hostname, &port, &path);
    if (rv != SECSuccess)
	goto loser;

    PORT_Assert(hostname != NULL);
    PORT_Assert(path != NULL);

    sock = ocsp_ConnectToHost(hostname, port);
    if (sock == NULL)
	goto loser;

    header = PR_smprintf("POST %s HTTP/1.0\r\n"
			 "Host: %s:%d\r\n"
			 "Content-Type: application/ocsp-request\r\n"
			 "Content-Length: %u\r\n\r\n",
			 path, hostname, port, encodedRequest->len);
    if (header == NULL)
	goto loser;

    



    if (PR_Write(sock, header, (PRInt32) PORT_Strlen(header)) < 0)
	goto loser;

    if (PR_Write(sock, encodedRequest->data,
		 (PRInt32) encodedRequest->len) < 0)
	goto loser;

    returnSock = sock;
    sock = NULL;

loser:
    if (header != NULL)
	PORT_Free(header);
    if (sock != NULL)
	PR_Close(sock);
    if (path != NULL)
	PORT_Free(path);
    if (hostname != NULL)
	PORT_Free(hostname);

    return returnSock;
}






static int
ocsp_read(PRFileDesc *fd, char *buf, int toread, PRIntervalTime timeout)
{
    int total = 0;

    while (total < toread)
    {
        PRInt32 got;

        got = PR_Recv(fd, buf + total, (PRInt32) (toread - total), 0, timeout);
        if (got < 0)
        {
            if (0 == total)
            {
                total = -1; 
            }
            break;
        }
        else
        if (got == 0)
        {			
            break;
        }

        total += got;
    }

    return total;
}

#define OCSP_BUFSIZE 1024

#define AbortHttpDecode(error) \
{ \
        if (inBuffer) \
            PORT_Free(inBuffer); \
        PORT_SetError(error); \
        return NULL; \
}










static SECItem *
ocsp_GetEncodedResponse(PRArenaPool *arena, PRFileDesc *sock)
{
    

    char* inBuffer = NULL;
    PRInt32 offset = 0;
    PRInt32 inBufsize = 0;
    const PRInt32 bufSizeIncrement = OCSP_BUFSIZE; 
    const PRInt32 maxBufSize = 8 * bufSizeIncrement ; 
    const char* CRLF = "\r\n";
    const PRInt32 CRLFlen = strlen(CRLF);
    const char* headerEndMark = "\r\n\r\n";
    const PRInt32 markLen = strlen(headerEndMark);
    const PRIntervalTime ocsptimeout =
        PR_SecondsToInterval(30); 
    char* headerEnd = NULL;
    PRBool EOS = PR_FALSE;
    const char* httpprotocol = "HTTP/";
    const PRInt32 httplen = strlen(httpprotocol);
    const char* httpcode = NULL;
    const char* contenttype = NULL;
    PRInt32 contentlength = 0;
    PRInt32 bytesRead = 0;
    char* statusLineEnd = NULL;
    char* space = NULL;
    char* nextHeader = NULL;
    SECItem* result = NULL;

    
    do
    {
        inBufsize += bufSizeIncrement;
        inBuffer = PORT_Realloc(inBuffer, inBufsize+1);
        if (NULL == inBuffer)
        {
            AbortHttpDecode(SEC_ERROR_NO_MEMORY);
        }
        bytesRead = ocsp_read(sock, inBuffer + offset, bufSizeIncrement,
            ocsptimeout);
        if (bytesRead > 0)
        {
            PRInt32 searchOffset = (offset - markLen) >0 ? offset-markLen : 0;
            offset += bytesRead;
            *(inBuffer + offset) = '\0'; 
            headerEnd = strstr((const char*)inBuffer + searchOffset, headerEndMark);
            if (bytesRead < bufSizeIncrement)
            {
                

                EOS = PR_TRUE;
            }
        }
        else
        {
            
            EOS = PR_TRUE;
        }
    } while ( (!headerEnd) && (PR_FALSE == EOS) &&
              (inBufsize < maxBufSize) );

    if (!headerEnd)
    {
        AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
    }

    
    statusLineEnd = strstr((const char*)inBuffer, CRLF);
    if (!statusLineEnd)
    {
        AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
    }
    *statusLineEnd = '\0';

    
    space = strchr((const char*)inBuffer, ' ');
    if (!space || PORT_Strncasecmp((const char*)inBuffer, httpprotocol, httplen) != 0 )
    {
        AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
    }

    
    httpcode = space +1;
    space = strchr(httpcode, ' ');
    if (!space)
    {
        AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
    }
    *space = 0;
    if (0 != strcmp(httpcode, "200"))
    {
        AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
    }

    



    nextHeader = statusLineEnd + CRLFlen;
    *headerEnd = '\0'; 
    do
    {
        char* thisHeaderEnd = NULL;
        char* value = NULL;
        char* colon = strchr(nextHeader, ':');
        
        if (!colon)
        {
            AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
        }

        *colon = '\0';
        value = colon + 1;

        





        if (*value != ' ')
        {
            AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
        }

        value++;
        thisHeaderEnd  = strstr(value, CRLF);
        if (thisHeaderEnd )
        {
            *thisHeaderEnd  = '\0';
        }

        if (0 == PORT_Strcasecmp(nextHeader, "content-type"))
        {
            contenttype = value;
        }
        else
        if (0 == PORT_Strcasecmp(nextHeader, "content-length"))
        {
            contentlength = atoi(value);
        }

        if (thisHeaderEnd )
        {
            nextHeader = thisHeaderEnd + CRLFlen;
        }
        else
        {
            nextHeader = NULL;
        }

    } while (nextHeader && (nextHeader < (headerEnd + CRLFlen) ) );

    
    if (!contenttype ||
        (0 != PORT_Strcasecmp(contenttype, "application/ocsp-response")) )
    {
        AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
    }

    
    offset = offset - (PRInt32) (headerEnd - (const char*)inBuffer) - markLen;
    if (offset)
    {
        
        PORT_Memmove(inBuffer, headerEnd + markLen, offset);
    }

    
    inBufsize = (1 + (offset-1) / bufSizeIncrement ) * bufSizeIncrement ;

    while ( (PR_FALSE == EOS) &&
            ( (contentlength == 0) || (offset < contentlength) ) &&
            (inBufsize < maxBufSize)
            )
    {
        
        inBufsize += bufSizeIncrement;
        inBuffer = PORT_Realloc(inBuffer, inBufsize+1);
        if (NULL == inBuffer)
        {
            AbortHttpDecode(SEC_ERROR_NO_MEMORY);
        }
        bytesRead = ocsp_read(sock, inBuffer + offset, bufSizeIncrement,
                              ocsptimeout);
        if (bytesRead > 0)
        {
            offset += bytesRead;
            if (bytesRead < bufSizeIncrement)
            {
                

                EOS = PR_TRUE;
            }
        }
        else
        {
            
            EOS = PR_TRUE;
        }
    }

    if (0 == offset)
    {
        AbortHttpDecode(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
    }

    


    result = SECITEM_AllocItem(arena, NULL, offset);
    if (NULL == result)
    {
        AbortHttpDecode(SEC_ERROR_NO_MEMORY);
    }

    


    PORT_Memcpy(result->data, inBuffer, offset);

    
    PORT_Free(inBuffer);
    return result;
}

SECStatus
CERT_ParseURL(const char *url, char **pHostname, PRUint16 *pPort, char **pPath)
{
    return ocsp_ParseURL(url, pHostname, pPort, pPath);
}




#define MAX_WANTED_OCSP_RESPONSE_LEN 64*1024

static SECItem *
fetchOcspHttpClientV1(PRArenaPool *arena, 
                      const SEC_HttpClientFcnV1 *hcv1, 
                      char *location, 
                      SECItem *encodedRequest)
{
    char *hostname = NULL;
    char *path = NULL;
    PRUint16 port;
    SECItem *encodedResponse = NULL;
    SEC_HTTP_SERVER_SESSION pServerSession = NULL;
    SEC_HTTP_REQUEST_SESSION pRequestSession = NULL;
    PRUint16 myHttpResponseCode;
    const char *myHttpResponseData;
    PRUint32 myHttpResponseDataLen;

    if (ocsp_ParseURL(location, &hostname, &port, &path) == SECFailure) {
        PORT_SetError(SEC_ERROR_OCSP_MALFORMED_REQUEST);
        goto loser;
    }
    
    PORT_Assert(hostname != NULL);
    PORT_Assert(path != NULL);

    if ((*hcv1->createSessionFcn)(
            hostname, 
            port, 
            &pServerSession) != SECSuccess) {
        PORT_SetError(SEC_ERROR_OCSP_SERVER_ERROR);
        goto loser;
    }

    






    if ((*hcv1->createFcn)(
            pServerSession,
            "http",
            path,
            "POST",
            PR_TicksPerSecond() * OCSP_Global.timeoutSeconds,
            &pRequestSession) != SECSuccess) {
        PORT_SetError(SEC_ERROR_OCSP_SERVER_ERROR);
        goto loser;
    }

    if ((*hcv1->setPostDataFcn)(
            pRequestSession, 
            (char*)encodedRequest->data,
            encodedRequest->len,
            "application/ocsp-request") != SECSuccess) {
        PORT_SetError(SEC_ERROR_OCSP_SERVER_ERROR);
        goto loser;
    }

    
    myHttpResponseDataLen = MAX_WANTED_OCSP_RESPONSE_LEN;

    OCSP_TRACE(("OCSP trySendAndReceive %s\n", location));

    if ((*hcv1->trySendAndReceiveFcn)(
            pRequestSession, 
            NULL,
            &myHttpResponseCode,
            NULL,
            NULL,
            &myHttpResponseData,
            &myHttpResponseDataLen) != SECSuccess) {
        PORT_SetError(SEC_ERROR_OCSP_SERVER_ERROR);
        goto loser;
    }

    OCSP_TRACE(("OCSP trySendAndReceive result http %d\n", myHttpResponseCode));

    if (myHttpResponseCode != 200) {
        PORT_SetError(SEC_ERROR_OCSP_BAD_HTTP_RESPONSE);
        goto loser;
    }

    encodedResponse = SECITEM_AllocItem(arena, NULL, myHttpResponseDataLen);

    if (!encodedResponse) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }

    PORT_Memcpy(encodedResponse->data, myHttpResponseData, myHttpResponseDataLen);

loser:
    if (pRequestSession != NULL) 
        (*hcv1->freeFcn)(pRequestSession);
    if (pServerSession != NULL)
        (*hcv1->freeSessionFcn)(pServerSession);
    if (path != NULL)
	PORT_Free(path);
    if (hostname != NULL)
	PORT_Free(hostname);
    
    return encodedResponse;
}















































SECItem *
CERT_GetEncodedOCSPResponse(PRArenaPool *arena, CERTCertList *certList,
			    char *location, int64 time,
			    PRBool addServiceLocator,
			    CERTCertificate *signerCert, void *pwArg,
			    CERTOCSPRequest **pRequest)
{
    CERTOCSPRequest *request;
    request = CERT_CreateOCSPRequest(certList, time, addServiceLocator,
                                     signerCert);
    if (!request)
        return NULL;
    return ocsp_GetEncodedOCSPResponseFromRequest(arena, request, location, 
                                                  time, addServiceLocator, 
                                                  pwArg, pRequest);
}

static SECItem *
ocsp_GetEncodedOCSPResponseFromRequest(PRArenaPool *arena, 
                                       CERTOCSPRequest *request,
                                       char *location, int64 time,
                                       PRBool addServiceLocator,
                                       void *pwArg,
                                       CERTOCSPRequest **pRequest)
{
    SECItem *encodedRequest = NULL;
    SECItem *encodedResponse = NULL;
    PRFileDesc *sock = NULL;
    SECStatus rv;
    const SEC_HttpClientFcn *registeredHttpClient = NULL;

    rv = CERT_AddOCSPAcceptableResponses(request,
					 SEC_OID_PKIX_OCSP_BASIC_RESPONSE);
    if (rv != SECSuccess)
	goto loser;

    encodedRequest = CERT_EncodeOCSPRequest(NULL, request, pwArg);
    if (encodedRequest == NULL)
	goto loser;

    registeredHttpClient = SEC_GetRegisteredHttpClient();

    if (registeredHttpClient
            &&
            registeredHttpClient->version == 1) {
        encodedResponse = fetchOcspHttpClientV1(
                              arena,
                              &registeredHttpClient->fcnTable.ftable1,
                              location,
                              encodedRequest);
    }
    else {
      
    
      sock = ocsp_SendEncodedRequest(location, encodedRequest);
      if (sock == NULL)
	  goto loser;

      encodedResponse = ocsp_GetEncodedResponse(arena, sock);
    }

    if (encodedResponse != NULL && pRequest != NULL) {
	*pRequest = request;
	request = NULL;			
    }

loser:
    if (request != NULL)
	CERT_DestroyOCSPRequest(request);
    if (encodedRequest != NULL)
	SECITEM_FreeItem(encodedRequest, PR_TRUE);
    if (sock != NULL)
	PR_Close(sock);

    return encodedResponse;
}

static SECItem *
ocsp_GetEncodedOCSPResponseForSingleCert(PRArenaPool *arena, 
                                         CERTOCSPCertID *certID, 
                                         CERTCertificate *singleCert, 
                                         char *location, int64 time,
                                         PRBool addServiceLocator,
                                         void *pwArg,
                                         CERTOCSPRequest **pRequest)
{
    CERTOCSPRequest *request;
    request = cert_CreateSingleCertOCSPRequest(certID, singleCert, time, 
                                               addServiceLocator, NULL);
    if (!request)
        return NULL;
    return ocsp_GetEncodedOCSPResponseFromRequest(arena, request, location, 
                                                  time, addServiceLocator, 
                                                  pwArg, pRequest);
}


static PRBool
ocsp_CertIsOCSPDesignatedResponder(CERTCertificate *cert)
{
    SECStatus rv;
    SECItem extItem;
    SECItem **oids;
    SECItem *oid;
    SECOidTag oidTag;
    PRBool retval;
    CERTOidSequence *oidSeq = NULL;


    extItem.data = NULL;
    rv = CERT_FindCertExtension(cert, SEC_OID_X509_EXT_KEY_USAGE, &extItem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    oidSeq = CERT_DecodeOidSequence(&extItem);
    if ( oidSeq == NULL ) {
	goto loser;
    }

    oids = oidSeq->oids;
    while ( *oids != NULL ) {
	oid = *oids;
	
	oidTag = SECOID_FindOIDTag(oid);
	
	if ( oidTag == SEC_OID_OCSP_RESPONDER ) {
	    goto success;
	}
	
	oids++;
    }

loser:
    retval = PR_FALSE;
    PORT_SetError(SEC_ERROR_OCSP_INVALID_SIGNING_CERT);
    goto done;
success:
    retval = PR_TRUE;
done:
    if ( extItem.data != NULL ) {
	PORT_Free(extItem.data);
    }
    if ( oidSeq != NULL ) {
	CERT_DestroyOidSequence(oidSeq);
    }
    
    return(retval);
}


#ifdef LATER	








static PRBool
ocsp_CertHasNoCheckExtension(CERTCertificate *cert)
{
    SECStatus rv;
    
    rv = CERT_FindCertExtension(cert, SEC_OID_PKIX_OCSP_NO_CHECK, 
				NULL);
    if (rv == SECSuccess) {
	return PR_TRUE;
    }
    return PR_FALSE;
}
#endif	

static PRBool
ocsp_matchcert(SECItem *certIndex,CERTCertificate *testCert)
{
    SECItem item;
    unsigned char buf[HASH_LENGTH_MAX];

    item.data = buf;
    item.len = SHA1_LENGTH;

    if (CERT_GetSPKIDigest(NULL,testCert,SEC_OID_SHA1, &item) == NULL) {
	return PR_FALSE;
    }
    if  (SECITEM_ItemsAreEqual(certIndex,&item)) {
	return PR_TRUE;
    }
    if (CERT_GetSPKIDigest(NULL,testCert,SEC_OID_MD5, &item) == NULL) {
	return PR_FALSE;
    }
    if  (SECITEM_ItemsAreEqual(certIndex,&item)) {
	return PR_TRUE;
    }
    if (CERT_GetSPKIDigest(NULL,testCert,SEC_OID_MD2, &item) == NULL) {
	return PR_FALSE;
    }
    if  (SECITEM_ItemsAreEqual(certIndex,&item)) {
	return PR_TRUE;
    }

    return PR_FALSE;
}

static CERTCertificate *
ocsp_CertGetDefaultResponder(CERTCertDBHandle *handle,CERTOCSPCertID *certID);

CERTCertificate *
ocsp_GetSignerCertificate(CERTCertDBHandle *handle, ocspResponseData *tbsData,
                          ocspSignature *signature, CERTCertificate *issuer)
{
    CERTCertificate **certs = NULL;
    CERTCertificate *signerCert = NULL;
    SECStatus rv = SECFailure;
    PRBool lookupByName = PR_TRUE;
    void *certIndex = NULL;
    int certCount = 0;

    PORT_Assert(tbsData->responderID != NULL);
    switch (tbsData->responderID->responderIDType) {
    case ocspResponderID_byName:
	lookupByName = PR_TRUE;
	certIndex = &tbsData->derResponderID;
	break;
    case ocspResponderID_byKey:
	lookupByName = PR_FALSE;
	certIndex = &tbsData->responderID->responderIDValue.keyHash;
	break;
    case ocspResponderID_other:
    default:
	PORT_Assert(0);
	PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
	return NULL;
    }

    






    if (signature->derCerts != NULL) {
	for (; signature->derCerts[certCount] != NULL; certCount++) {
	    
	}
	rv = CERT_ImportCerts(handle, certUsageStatusResponder, certCount,
	                      signature->derCerts, &certs,
	                      PR_FALSE, PR_FALSE, NULL);
	if (rv != SECSuccess)
	     goto finish;
    }

    



    if (lookupByName) {
	SECItem *crIndex = (SECItem*)certIndex;
	SECItem encodedName;
	PLArenaPool *arena;

	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (arena != NULL) {

	    rv = SEC_QuickDERDecodeItem(arena, &encodedName,
	                                ocsp_ResponderIDDerNameTemplate,
	                                crIndex);
	    if (rv != SECSuccess) {
	        if (PORT_GetError() == SEC_ERROR_BAD_DER)
	            PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
	    } else {
	            signerCert = CERT_FindCertByName(handle, &encodedName);
	    }
	    PORT_FreeArena(arena, PR_FALSE);
	}
    } else {
	




	int i;
	CERTCertificate *responder = 
            ocsp_CertGetDefaultResponder(handle, NULL);
	if (responder && ocsp_matchcert(certIndex,responder)) {
	    signerCert = CERT_DupCertificate(responder);
	} else if (issuer && ocsp_matchcert(certIndex,issuer)) {
	    signerCert = CERT_DupCertificate(issuer);
	} 
	for (i=0; (signerCert == NULL) && (i < certCount); i++) {
	    if (ocsp_matchcert(certIndex,certs[i])) {
		signerCert = CERT_DupCertificate(certs[i]);
	    }
	}
    }

finish:
    if (certs != NULL) {
	CERT_DestroyCertArray(certs, certCount);
    }

    return signerCert;
}

SECStatus
ocsp_VerifyResponseSignature(CERTCertificate *signerCert,
                             ocspSignature *signature,
                             SECItem *tbsResponseDataDER,
                             void *pwArg)
{
    SECItem rawSignature;
    SECKEYPublicKey *signerKey = NULL;
    SECStatus rv = SECFailure;

    



    signerKey = CERT_ExtractPublicKey(signerCert);
    if (signerKey == NULL)
	return SECFailure;
    




    rawSignature = signature->signature;
    



    DER_ConvertBitString(&rawSignature);

    rv = VFY_VerifyDataWithAlgorithmID(tbsResponseDataDER->data,
                                       tbsResponseDataDER->len,
                                       signerKey, &rawSignature,
                                       &signature->signatureAlgorithm,
                                       NULL, pwArg);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_BAD_SIGNATURE) {
        PORT_SetError(SEC_ERROR_OCSP_BAD_SIGNATURE);
    }
    
    if (signerKey != NULL) {
        SECKEY_DestroyPublicKey(signerKey);
    }

    return rv;
}































SECStatus
CERT_VerifyOCSPResponseSignature(CERTOCSPResponse *response,	
				 CERTCertDBHandle *handle, void *pwArg,
				 CERTCertificate **pSignerCert,
				 CERTCertificate *issuer)
{
    SECItem *tbsResponseDataDER;
    CERTCertificate *signerCert = NULL;
    SECStatus rv = SECFailure;
    int64 producedAt;

    



    ocspResponseData *tbsData = ocsp_GetResponseData(response,
                                                     &tbsResponseDataDER);
    ocspSignature *signature = ocsp_GetResponseSignature(response);

    if (!signature) {
        PORT_SetError(SEC_ERROR_OCSP_BAD_SIGNATURE);
        return SECFailure;
    }

    



    if (signature->wasChecked) {
	if (signature->status == SECSuccess) {
	    if (pSignerCert != NULL)
		*pSignerCert = CERT_DupCertificate(signature->cert);
	} else {
	    PORT_SetError(signature->failureReason);
	}
	return signature->status;
    }

    signerCert = ocsp_GetSignerCertificate(handle, tbsData,
                                           signature, issuer);
    if (signerCert == NULL) {
	rv = SECFailure;
	if (PORT_GetError() == SEC_ERROR_UNKNOWN_CERT) {
	    
	    PORT_SetError(SEC_ERROR_OCSP_INVALID_SIGNING_CERT);
	}
	goto finish;
    }

    





    signature->wasChecked = PR_TRUE;

    





    rv = DER_GeneralizedTimeToTime(&producedAt, &tbsData->producedAt);
    if (rv != SECSuccess)
        goto finish;

    



    if (ocsp_CertIsOCSPDefaultResponder(handle, signerCert)) {
        rv = SECSuccess;
    } else {
        SECCertUsage certUsage;
        if (CERT_IsCACert(signerCert, NULL)) {
            certUsage = certUsageAnyCA;
        } else {
            certUsage = certUsageStatusResponder;
        }
        rv = CERT_VerifyCert(handle, signerCert, PR_TRUE,
                             certUsage, producedAt, pwArg, NULL);
        if (rv != SECSuccess) {
            PORT_SetError(SEC_ERROR_OCSP_INVALID_SIGNING_CERT);
            goto finish;
        }
    }

    rv = ocsp_VerifyResponseSignature(signerCert, signature,
                                      tbsResponseDataDER,
                                      pwArg);

finish:
    if (signature->wasChecked)
	signature->status = rv;

    if (rv != SECSuccess) {
	signature->failureReason = PORT_GetError();
	if (signerCert != NULL)
	    CERT_DestroyCertificate(signerCert);
    } else {
	


	signature->cert = signerCert;
	if (pSignerCert != NULL) {
	    



	    *pSignerCert = CERT_DupCertificate(signerCert);
	}
    }

    return rv;
}






static PRBool
ocsp_CertIDsMatch(CERTCertDBHandle *handle,
		  CERTOCSPCertID *requestCertID,
		  CERTOCSPCertID *responseCertID)
{
    PRBool match = PR_FALSE;
    SECOidTag hashAlg;
    SECItem *keyHash = NULL;
    SECItem *nameHash = NULL;

    





    if (SECITEM_CompareItem(&requestCertID->serialNumber,
			    &responseCertID->serialNumber) != SECEqual) {
	goto done;
    }

    



    if (responseCertID->hashAlgorithm.parameters.len > 2) {
	goto done;
    }
    if (SECITEM_CompareItem(&requestCertID->hashAlgorithm.algorithm,
		&responseCertID->hashAlgorithm.algorithm) == SECEqual) {
	



	if ((SECITEM_CompareItem(&requestCertID->issuerNameHash,
				&responseCertID->issuerNameHash) == SECEqual)
	    && (SECITEM_CompareItem(&requestCertID->issuerKeyHash,
				&responseCertID->issuerKeyHash) == SECEqual)) {
	    match = PR_TRUE;
	}
	goto done;
    }

    hashAlg = SECOID_FindOIDTag(&responseCertID->hashAlgorithm.algorithm);
    switch (hashAlg) {
    case SEC_OID_SHA1:
	keyHash = &requestCertID->issuerSHA1KeyHash;
	nameHash = &requestCertID->issuerSHA1NameHash;
	break;
    case SEC_OID_MD5:
	keyHash = &requestCertID->issuerMD5KeyHash;
	nameHash = &requestCertID->issuerMD5NameHash;
	break;
    case SEC_OID_MD2:
	keyHash = &requestCertID->issuerMD2KeyHash;
	nameHash = &requestCertID->issuerMD2NameHash;
	break;
    default:
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
 	return SECFailure;
    }

    if ((keyHash != NULL)
	&& (SECITEM_CompareItem(nameHash,
				&responseCertID->issuerNameHash) == SECEqual)
	&& (SECITEM_CompareItem(keyHash,
				&responseCertID->issuerKeyHash) == SECEqual)) {
	match = PR_TRUE;
    }

done:
    return match;
}







static CERTOCSPSingleResponse *
ocsp_GetSingleResponseForCertID(CERTOCSPSingleResponse **responses,
				CERTCertDBHandle *handle,
				CERTOCSPCertID *certID)
{
    CERTOCSPSingleResponse *single;
    int i;

    if (responses == NULL)
	return NULL;

    for (i = 0; responses[i] != NULL; i++) {
	single = responses[i];
	if (ocsp_CertIDsMatch(handle, certID, single->certID)) {
	    return single;
	}
    }

    







    PORT_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT);
    return NULL;
}

static ocspCheckingContext *
ocsp_GetCheckingContext(CERTCertDBHandle *handle)
{
    CERTStatusConfig *statusConfig;
    ocspCheckingContext *ocspcx = NULL;

    statusConfig = CERT_GetStatusConfig(handle);
    if (statusConfig != NULL) {
	ocspcx = statusConfig->statusContext;

	





	PORT_Assert(ocspcx != NULL);
    }

    if (ocspcx == NULL)
	PORT_SetError(SEC_ERROR_OCSP_NOT_ENABLED);

    return ocspcx;
}





static CERTCertificate *
ocsp_CertGetDefaultResponder(CERTCertDBHandle *handle, CERTOCSPCertID *certID)
{
    ocspCheckingContext *ocspcx;

    ocspcx = ocsp_GetCheckingContext(handle);
    if (ocspcx == NULL)
	goto loser;

   







    if (ocspcx->useDefaultResponder) {
	PORT_Assert(ocspcx->defaultResponderCert != NULL);
	return ocspcx->defaultResponderCert;
    }

loser:
    return NULL;
}





PRBool
ocsp_CertIsOCSPDefaultResponder(CERTCertDBHandle *handle, CERTCertificate *cert)
{
    ocspCheckingContext *ocspcx;

    ocspcx = ocsp_GetCheckingContext(handle);
    if (ocspcx == NULL)
	return PR_FALSE;

   







    if (ocspcx->useDefaultResponder &&
        CERT_CompareCerts(ocspcx->defaultResponderCert, cert)) {
	return PR_TRUE;
    }

    return PR_FALSE;
}


















static PRBool
ocsp_AuthorizedResponderForCertID(CERTCertDBHandle *handle,
				  CERTCertificate *signerCert,
				  CERTOCSPCertID *certID,
				  int64 thisUpdate)
{
    CERTCertificate *issuerCert = NULL, *defRespCert;
    SECItem *keyHash = NULL;
    SECItem *nameHash = NULL;
    SECOidTag hashAlg;
    PRBool keyHashEQ = PR_FALSE, nameHashEQ = PR_FALSE;

    


    if ((defRespCert = ocsp_CertGetDefaultResponder(handle, certID)) &&
        CERT_CompareCerts(defRespCert, signerCert)) {
        return PR_TRUE;
    }

    











    hashAlg = SECOID_FindOIDTag(&certID->hashAlgorithm.algorithm);

    keyHash = CERT_GetSPKIDigest(NULL, signerCert, hashAlg, NULL);
    if (keyHash != NULL) {

        keyHashEQ =
            (SECITEM_CompareItem(keyHash,
                                 &certID->issuerKeyHash) == SECEqual);
        SECITEM_FreeItem(keyHash, PR_TRUE);
    }
    if (keyHashEQ &&
        (nameHash = cert_GetSubjectNameDigest(NULL, signerCert,
                                              hashAlg, NULL))) {
        nameHashEQ =
            (SECITEM_CompareItem(nameHash,
                                 &certID->issuerNameHash) == SECEqual);
            
        SECITEM_FreeItem(nameHash, PR_TRUE);
        if (nameHashEQ) {
            
            return PR_TRUE;
        }
    }


    keyHashEQ = PR_FALSE;
    nameHashEQ = PR_FALSE;

    if (!ocsp_CertIsOCSPDesignatedResponder(signerCert)) {
        PORT_SetError(SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE);
        return PR_FALSE;
    }

    



    issuerCert = CERT_FindCertIssuer(signerCert, thisUpdate,
                                     certUsageAnyCA);
    if (issuerCert == NULL) {
        




        PORT_SetError(SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE);
        return PR_FALSE;
    }

    keyHash = CERT_GetSPKIDigest(NULL, issuerCert, hashAlg, NULL);
    nameHash = cert_GetSubjectNameDigest(NULL, issuerCert, hashAlg, NULL);

    CERT_DestroyCertificate(issuerCert);

    if (keyHash != NULL && nameHash != NULL) {
        keyHashEQ = 
            (SECITEM_CompareItem(keyHash,
                                 &certID->issuerKeyHash) == SECEqual);

        nameHashEQ =
            (SECITEM_CompareItem(nameHash,
                                 &certID->issuerNameHash) == SECEqual);
    }

    if (keyHash) {
        SECITEM_FreeItem(keyHash, PR_TRUE);
    }
    if (nameHash) {
        SECITEM_FreeItem(nameHash, PR_TRUE);
    }

    if (keyHashEQ && nameHashEQ) {
        return PR_TRUE;
    }

    PORT_SetError(SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE);
    return PR_FALSE;
}













#define OCSP_ALLOWABLE_LAPSE_SECONDS	(24L * 60L * 60L)

static PRBool
ocsp_TimeIsRecent(int64 checkTime)
{
    int64 now = PR_Now();
    int64 lapse, tmp;

    LL_I2L(lapse, OCSP_ALLOWABLE_LAPSE_SECONDS);
    LL_I2L(tmp, PR_USEC_PER_SEC);
    LL_MUL(lapse, lapse, tmp);		

    LL_ADD(checkTime, checkTime, lapse);
    if (LL_CMP(now, >, checkTime))
	return PR_FALSE;

    return PR_TRUE;
}

#define OCSP_SLOP (5L*60L) /* OCSP responses are allowed to be 5 minutes
                              in the future by default */

static PRUint32 ocspsloptime = OCSP_SLOP;	























 
static SECStatus
ocsp_VerifySingleResponse(CERTOCSPSingleResponse *single,
			  CERTCertDBHandle *handle,
			  CERTCertificate *signerCert,
			  int64 producedAt)
{
    CERTOCSPCertID *certID = single->certID;
    int64 now, thisUpdate, nextUpdate, tmstamp, tmp;
    SECStatus rv;

    OCSP_TRACE(("OCSP ocsp_VerifySingleResponse, nextUpdate: %d\n", 
               ((single->nextUpdate) != 0)));
    





    PORT_Assert(single->certStatus != NULL);
    if (single->certStatus->certStatusType == ocspCertStatus_unknown)
	return SECSuccess;

    




    rv = DER_GeneralizedTimeToTime(&thisUpdate, &single->thisUpdate);
    if (rv != SECSuccess)
	return rv;

    


    if (ocsp_AuthorizedResponderForCertID(handle, signerCert, certID,
					  thisUpdate) != PR_TRUE)
	return SECFailure;

    


    now = PR_Now();
    
    LL_UI2L(tmstamp, ocspsloptime); 
    LL_UI2L(tmp, PR_USEC_PER_SEC);
    LL_MUL(tmp, tmstamp, tmp); 
    LL_ADD(tmstamp, tmp, now); 

    if (LL_CMP(thisUpdate, >, tmstamp) || LL_CMP(producedAt, <, thisUpdate)) {
	PORT_SetError(SEC_ERROR_OCSP_FUTURE_RESPONSE);
	return SECFailure;
    }
    if (single->nextUpdate != NULL) {
	rv = DER_GeneralizedTimeToTime(&nextUpdate, single->nextUpdate);
	if (rv != SECSuccess)
	    return rv;

	LL_ADD(tmp, tmp, nextUpdate);
	if (LL_CMP(tmp, <, now) || LL_CMP(producedAt, >, nextUpdate)) {
	    PORT_SetError(SEC_ERROR_OCSP_OLD_RESPONSE);
	    return SECFailure;
	}
    } else if (ocsp_TimeIsRecent(thisUpdate) != PR_TRUE) {
	PORT_SetError(SEC_ERROR_OCSP_OLD_RESPONSE);
	return SECFailure;
    }

    return SECSuccess;
}



















char *
CERT_GetOCSPAuthorityInfoAccessLocation(CERTCertificate *cert)
{
    CERTGeneralName *locname = NULL;
    SECItem *location = NULL;
    SECItem *encodedAuthInfoAccess = NULL;
    CERTAuthInfoAccess **authInfoAccess = NULL;
    char *locURI = NULL;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    int i;

    




    encodedAuthInfoAccess = SECITEM_AllocItem(NULL, NULL, 0);
    if (encodedAuthInfoAccess == NULL)
	goto loser;

    rv = CERT_FindCertExtension(cert, SEC_OID_X509_AUTH_INFO_ACCESS,
				encodedAuthInfoAccess);
    if (rv == SECFailure) {
	PORT_SetError(SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
	goto loser;
    }

    





    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	goto loser;

    authInfoAccess = CERT_DecodeAuthInfoAccessExtension(arena,
							encodedAuthInfoAccess);
    if (authInfoAccess == NULL)
	goto loser;

    for (i = 0; authInfoAccess[i] != NULL; i++) {
	if (SECOID_FindOIDTag(&authInfoAccess[i]->method) == SEC_OID_PKIX_OCSP)
	    locname = authInfoAccess[i]->location;
    }

    






    if (locname == NULL) {
	PORT_SetError(SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
	goto loser;
    }

    



    location = CERT_GetGeneralNameByType(locname, certURI, PR_FALSE);
    if (location == NULL) {
	






	PORT_SetError(SEC_ERROR_CERT_BAD_ACCESS_LOCATION);
	goto loser;
    }

    





    locURI = PORT_Alloc(location->len + 1);
    if (locURI == NULL) {
	goto loser;
    }
    PORT_Memcpy(locURI, location->data, location->len);
    locURI[location->len] = '\0';

loser:
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);

    if (encodedAuthInfoAccess != NULL)
	SECITEM_FreeItem(encodedAuthInfoAccess, PR_TRUE);

    return locURI;
}















char *
ocsp_GetResponderLocation(CERTCertDBHandle *handle, CERTCertificate *cert,
			  PRBool canUseDefault, PRBool *isDefault)
{
    ocspCheckingContext *ocspcx = NULL;
    char *ocspUrl = NULL;

    if (canUseDefault) {
        ocspcx = ocsp_GetCheckingContext(handle);
    }
    if (ocspcx != NULL && ocspcx->useDefaultResponder) {
	





	PORT_Assert(ocspcx->defaultResponderURI != NULL);
	*isDefault = PR_TRUE;
	return (PORT_Strdup(ocspcx->defaultResponderURI));
    }

    



    *isDefault = PR_FALSE;
    ocspUrl = CERT_GetOCSPAuthorityInfoAccessLocation(cert);
    if (!ocspUrl) {
	CERT_StringFromCertFcn altFcn;

	PR_EnterMonitor(OCSP_Global.monitor);
	altFcn = OCSP_Global.alternateOCSPAIAFcn;
	PR_ExitMonitor(OCSP_Global.monitor);
	if (altFcn) {
	    ocspUrl = (*altFcn)(cert);
	    if (ocspUrl)
		*isDefault = PR_TRUE;
    	}
    }
    return ocspUrl;
}





static SECStatus
ocsp_CertRevokedAfter(ocspRevokedInfo *revokedInfo, int64 time)
{
    int64 revokedTime;
    SECStatus rv;

    rv = DER_GeneralizedTimeToTime(&revokedTime, &revokedInfo->revocationTime);
    if (rv != SECSuccess)
	return rv;

    


    PORT_SetError(SEC_ERROR_REVOKED_CERTIFICATE);

    if (LL_CMP(revokedTime, >, time))
	return SECSuccess;

    return SECFailure;
}





static SECStatus
ocsp_CertHasGoodStatus(ocspCertStatus *status, int64 time)
{
    SECStatus rv;
    switch (status->certStatusType) {
    case ocspCertStatus_good:
        rv = SECSuccess;
        break;
    case ocspCertStatus_revoked:
        rv = ocsp_CertRevokedAfter(status->certStatusInfo.revokedInfo, time);
        break;
    case ocspCertStatus_unknown:
        PORT_SetError(SEC_ERROR_OCSP_UNKNOWN_CERT);
        rv = SECFailure;
        break;
    case ocspCertStatus_other:
    default:
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_OCSP_MALFORMED_RESPONSE);
        rv = SECFailure;
        break;
    }
    return rv;
}

static SECStatus
ocsp_SingleResponseCertHasGoodStatus(CERTOCSPSingleResponse *single, 
                                     int64 time)
{
    return ocsp_CertHasGoodStatus(single->certStatus, time);
}










SECStatus
ocsp_GetCachedOCSPResponseStatusIfFresh(CERTOCSPCertID *certID, 
                                        int64 time, 
                                        PRBool ignoreGlobalOcspFailureSetting,
                                        SECStatus *rvOcsp,
                                        SECErrorCodes *missingResponseError)
{
    OCSPCacheItem *cacheItem = NULL;
    SECStatus rv = SECFailure;
  
    if (!certID || !missingResponseError || !rvOcsp) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    *rvOcsp = SECFailure;
    *missingResponseError = 0;
  
    PR_EnterMonitor(OCSP_Global.monitor);
    cacheItem = ocsp_FindCacheEntry(&OCSP_Global.cache, certID);
    if (cacheItem && ocsp_IsCacheItemFresh(cacheItem)) {
        
        if (cacheItem->certStatusArena) {
            *rvOcsp = ocsp_CertHasGoodStatus(&cacheItem->certStatus, time);
            rv = SECSuccess;
        } else {
            





            if (!ignoreGlobalOcspFailureSetting &&
                OCSP_Global.ocspFailureMode == 
                    ocspMode_FailureIsNotAVerificationFailure) {
                rv = SECSuccess;
                *rvOcsp = SECSuccess;
            }
            *missingResponseError = cacheItem->missingResponseError;
        }
    }
    PR_ExitMonitor(OCSP_Global.monitor);
    return rv;
}

PRBool
ocsp_FetchingFailureIsVerificationFailure()
{
    PRBool isFailure;

    PR_EnterMonitor(OCSP_Global.monitor);
    isFailure =
        OCSP_Global.ocspFailureMode == ocspMode_FailureIsVerificationFailure;
    PR_ExitMonitor(OCSP_Global.monitor);
    return isFailure;
}


















































    
SECStatus 
CERT_CheckOCSPStatus(CERTCertDBHandle *handle, CERTCertificate *cert,
		     int64 time, void *pwArg)
{
    CERTOCSPCertID *certID;
    PRBool certIDWasConsumed = PR_FALSE;
    SECStatus rv = SECFailure;
    SECStatus rvOcsp;
    SECErrorCodes dummy_error_code; 
  
    OCSP_TRACE_CERT(cert);
    OCSP_TRACE_TIME("## requested validity time:", time);
  
    certID = CERT_CreateOCSPCertID(cert, time);
    if (!certID)
        return SECFailure;
    rv = ocsp_GetCachedOCSPResponseStatusIfFresh(
        certID, time, PR_FALSE, 
        &rvOcsp, &dummy_error_code);
    if (rv == SECSuccess) {
        CERT_DestroyOCSPCertID(certID);
        return rvOcsp;
    }
    rv = ocsp_GetOCSPStatusFromNetwork(handle, certID, cert, time, pwArg, 
                                       &certIDWasConsumed, 
                                       &rvOcsp);
    if (rv != SECSuccess) {
        

        rvOcsp = ocsp_FetchingFailureIsVerificationFailure() ?
            SECFailure : SECSuccess;
    }
    if (!certIDWasConsumed) {
        CERT_DestroyOCSPCertID(certID);
    }
    return rvOcsp;
}





static SECStatus
ocsp_GetOCSPStatusFromNetwork(CERTCertDBHandle *handle, 
                              CERTOCSPCertID *certID, 
                              CERTCertificate *cert, 
                              int64 time, 
                              void *pwArg,
                              PRBool *certIDWasConsumed,
                              SECStatus *rv_ocsp)
{
    char *location = NULL;
    PRBool locationIsDefault;
    SECItem *encodedResponse = NULL;
    CERTOCSPRequest *request = NULL;
    CERTOCSPResponse *response = NULL;
    CERTCertificate *signerCert = NULL;
    CERTCertificate *issuerCert = NULL;
    SECStatus rv = SECFailure;
    CERTOCSPSingleResponse *single = NULL;

    if (!certIDWasConsumed || !rv_ocsp) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    *certIDWasConsumed = PR_FALSE;
    *rv_ocsp = SECFailure;

    










    location = ocsp_GetResponderLocation(handle, cert, PR_TRUE,
                                         &locationIsDefault);
    if (location == NULL) {
       int err = PORT_GetError();
       if (err == SEC_ERROR_EXTENSION_NOT_FOUND ||
           err == SEC_ERROR_CERT_BAD_ACCESS_LOCATION) {
           PORT_SetError(0);
           *rv_ocsp = SECSuccess;
           return SECSuccess;
       }
       return SECFailure;
    }

    














    





    encodedResponse = 
        ocsp_GetEncodedOCSPResponseForSingleCert(NULL, certID, cert, location,
                                                 time, locationIsDefault,
                                                 pwArg, &request);
    if (encodedResponse == NULL) {
        goto loser;
    }

    response = CERT_DecodeOCSPResponse(encodedResponse);
    if (response == NULL) {
	goto loser;
    }

    








    if (CERT_GetOCSPResponseStatus(response) != SECSuccess) {
	goto loser;
    }

    



    issuerCert = CERT_FindCertIssuer(cert, time, certUsageAnyCA);
    rv = CERT_VerifyOCSPResponseSignature(response, handle, pwArg, &signerCert,
			issuerCert);
    if (rv != SECSuccess)
	goto loser;

    PORT_Assert(signerCert != NULL);	
    


    






    rv = ocsp_GetVerifiedSingleResponseForCertID(handle, response, certID, 
                                                 signerCert, time, &single);
    if (rv != SECSuccess)
        goto loser;

    *rv_ocsp = ocsp_SingleResponseCertHasGoodStatus(single, time);

loser:
    PR_EnterMonitor(OCSP_Global.monitor);
    if (OCSP_Global.maxCacheEntries >= 0) {
        
        ocsp_CreateOrUpdateCacheEntry(&OCSP_Global.cache, certID, single, 
                                      certIDWasConsumed);
        
    }
    PR_ExitMonitor(OCSP_Global.monitor);

    if (issuerCert != NULL)
	CERT_DestroyCertificate(issuerCert);
    if (signerCert != NULL)
	CERT_DestroyCertificate(signerCert);
    if (response != NULL)
	CERT_DestroyOCSPResponse(response);
    if (request != NULL)
	CERT_DestroyOCSPRequest(request);
    if (encodedResponse != NULL)
	SECITEM_FreeItem(encodedResponse, PR_TRUE);
    if (location != NULL)
	PORT_Free(location);
    return rv;
}

static SECStatus
ocsp_GetVerifiedSingleResponseForCertID(CERTCertDBHandle *handle, 
                                        CERTOCSPResponse *response, 
                                        CERTOCSPCertID   *certID,
                                        CERTCertificate  *signerCert,
                                        int64             time,
                                        CERTOCSPSingleResponse 
                                            **pSingleResponse)
{
    SECStatus rv;
    ocspResponseData *responseData;
    int64 producedAt;
    CERTOCSPSingleResponse *single;

    


    responseData = ocsp_GetResponseData(response, NULL);
    if (responseData == NULL) {
        rv = SECFailure;
        goto loser;
    }

    





    rv = DER_GeneralizedTimeToTime(&producedAt, &responseData->producedAt);
    if (rv != SECSuccess)
        goto loser;

    single = ocsp_GetSingleResponseForCertID(responseData->responses,
                                             handle, certID);
    if (single == NULL) {
        rv = SECFailure;
        goto loser;
    }

    rv = ocsp_VerifySingleResponse(single, handle, signerCert, producedAt);
    if (rv != SECSuccess)
        goto loser;
    *pSingleResponse = single;

loser:
    return rv;
}

SECStatus
CERT_GetOCSPStatusForCertID(CERTCertDBHandle *handle, 
                            CERTOCSPResponse *response, 
                            CERTOCSPCertID   *certID,
                            CERTCertificate  *signerCert,
                            int64             time)
{
    


















    return cert_ProcessOCSPResponse(handle, response, certID, 
                                    signerCert, time, 
                                    NULL, NULL);
}




SECStatus
cert_ProcessOCSPResponse(CERTCertDBHandle *handle, 
                         CERTOCSPResponse *response, 
                         CERTOCSPCertID   *certID,
                         CERTCertificate  *signerCert,
                         int64             time,
                         PRBool           *certIDWasConsumed,
                         SECStatus        *cacheUpdateStatus)
{
    SECStatus rv;
    SECStatus rv_cache;
    CERTOCSPSingleResponse *single = NULL;

    rv = ocsp_GetVerifiedSingleResponseForCertID(handle, response, certID, 
                                                 signerCert, time, &single);
    if (rv == SECSuccess) {
        



        rv = ocsp_SingleResponseCertHasGoodStatus(single, time);
    }

    if (certIDWasConsumed) {
        




  
        PR_EnterMonitor(OCSP_Global.monitor);
        if (OCSP_Global.maxCacheEntries >= 0) {
            
            rv_cache = 
                ocsp_CreateOrUpdateCacheEntry(&OCSP_Global.cache, certID,
                                              single, certIDWasConsumed);
        }
        PR_ExitMonitor(OCSP_Global.monitor);
        if (cacheUpdateStatus) {
            *cacheUpdateStatus = rv_cache;
        }
    }

    return rv;
}

SECStatus
cert_RememberOCSPProcessingFailure(CERTOCSPCertID *certID,
                                   PRBool         *certIDWasConsumed)
{
    SECStatus rv = SECSuccess;
    PR_EnterMonitor(OCSP_Global.monitor);
    if (OCSP_Global.maxCacheEntries >= 0) {
        rv = ocsp_CreateOrUpdateCacheEntry(&OCSP_Global.cache, certID, NULL, 
                                           certIDWasConsumed);
    }
    PR_ExitMonitor(OCSP_Global.monitor);
    return rv;
}




static SECStatus
ocsp_DestroyStatusChecking(CERTStatusConfig *statusConfig)
{
    ocspCheckingContext *statusContext;

    


    statusConfig->statusChecker = NULL;

    statusContext = statusConfig->statusContext;
    PORT_Assert(statusContext != NULL);
    if (statusContext == NULL)
	return SECFailure;

    if (statusContext->defaultResponderURI != NULL)
	PORT_Free(statusContext->defaultResponderURI);
    if (statusContext->defaultResponderNickname != NULL)
	PORT_Free(statusContext->defaultResponderNickname);

    PORT_Free(statusContext);
    statusConfig->statusContext = NULL;

    PORT_Free(statusConfig);

    return SECSuccess;
}

















SECStatus
CERT_DisableOCSPChecking(CERTCertDBHandle *handle)
{
    CERTStatusConfig *statusConfig;
    ocspCheckingContext *statusContext;

    if (handle == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    statusConfig = CERT_GetStatusConfig(handle);
    statusContext = ocsp_GetCheckingContext(handle);
    if (statusContext == NULL)
	return SECFailure;

    if (statusConfig->statusChecker != CERT_CheckOCSPStatus) {
	



	PORT_SetError(SEC_ERROR_OCSP_NOT_ENABLED);
	return SECFailure;
    }

    
    CERT_ClearOCSPCache();

    



    statusConfig->statusChecker = NULL;

    return SECSuccess;
}






static SECStatus
ocsp_InitStatusChecking(CERTCertDBHandle *handle)
{
    CERTStatusConfig *statusConfig = NULL;
    ocspCheckingContext *statusContext = NULL;

    PORT_Assert(CERT_GetStatusConfig(handle) == NULL);
    if (CERT_GetStatusConfig(handle) != NULL) {
	
	return SECFailure;
    }

    statusConfig = PORT_ZNew(CERTStatusConfig);
    if (statusConfig == NULL)
	goto loser;

    statusContext = PORT_ZNew(ocspCheckingContext);
    if (statusContext == NULL)
	goto loser;

    statusConfig->statusDestroy = ocsp_DestroyStatusChecking;
    statusConfig->statusContext = statusContext;

    CERT_SetStatusConfig(handle, statusConfig);

    return SECSuccess;

loser:
    if (statusConfig != NULL)
	PORT_Free(statusConfig);
    return SECFailure;
}












SECStatus
CERT_EnableOCSPChecking(CERTCertDBHandle *handle)
{
    CERTStatusConfig *statusConfig;
    
    SECStatus rv;

    if (handle == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    statusConfig = CERT_GetStatusConfig(handle);
    if (statusConfig == NULL) {
	rv = ocsp_InitStatusChecking(handle);
	if (rv != SECSuccess)
	    return rv;

	
	statusConfig = CERT_GetStatusConfig(handle);
	PORT_Assert(statusConfig != NULL);
    }

    



    statusConfig->statusChecker = CERT_CheckOCSPStatus;

    return SECSuccess;
}


























SECStatus
CERT_SetOCSPDefaultResponder(CERTCertDBHandle *handle,
			     const char *url, const char *name)
{
    CERTCertificate *cert;
    ocspCheckingContext *statusContext;
    char *url_copy = NULL;
    char *name_copy = NULL;
    SECStatus rv;

    if (handle == NULL || url == NULL || name == NULL) {
	



	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    






    cert = CERT_FindCertByNickname(handle, (char *) name);
    if (cert == NULL) {
      


      cert = PK11_FindCertFromNickname((char *)name, NULL);
    }
    if (cert == NULL)
	return SECFailure;

    


    url_copy = PORT_Strdup(url);
    name_copy = PORT_Strdup(name);
    if (url_copy == NULL || name_copy == NULL) {
	rv = SECFailure;
	goto loser;
    }

    statusContext = ocsp_GetCheckingContext(handle);

    


    if (statusContext == NULL) {
	rv = ocsp_InitStatusChecking(handle);
	if (rv != SECSuccess)
	    goto loser;

	statusContext = ocsp_GetCheckingContext(handle);
	PORT_Assert(statusContext != NULL);	
    }

    





    


    if (statusContext->defaultResponderNickname != NULL)
	PORT_Free(statusContext->defaultResponderNickname);
    if (statusContext->defaultResponderURI != NULL)
	PORT_Free(statusContext->defaultResponderURI);

    


    statusContext->defaultResponderURI = url_copy;
    statusContext->defaultResponderNickname = name_copy;

    





    if (statusContext->defaultResponderCert != NULL) {
	CERT_DestroyCertificate(statusContext->defaultResponderCert);
	statusContext->defaultResponderCert = cert;
        
        CERT_ClearOCSPCache();
    } else {
	PORT_Assert(statusContext->useDefaultResponder == PR_FALSE);
	CERT_DestroyCertificate(cert);
        
    }

    return SECSuccess;

loser:
    CERT_DestroyCertificate(cert);
    if (url_copy != NULL)
	PORT_Free(url_copy);
    if (name_copy != NULL)
	PORT_Free(name_copy);
    return rv;
}



















SECStatus
CERT_EnableOCSPDefaultResponder(CERTCertDBHandle *handle)
{
    ocspCheckingContext *statusContext;
    CERTCertificate *cert;
    SECStatus rv;
    SECCertificateUsage usage;

    if (handle == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    statusContext = ocsp_GetCheckingContext(handle);

    if (statusContext == NULL) {
	



	PORT_SetError(SEC_ERROR_OCSP_NO_DEFAULT_RESPONDER);
	return SECFailure;
    }

    if (statusContext->defaultResponderURI == NULL) {
	PORT_SetError(SEC_ERROR_OCSP_NO_DEFAULT_RESPONDER);
	return SECFailure;
    }

    if (statusContext->defaultResponderNickname == NULL) {
	PORT_SetError(SEC_ERROR_OCSP_NO_DEFAULT_RESPONDER);
	return SECFailure;
    }

    


    cert = CERT_FindCertByNickname(handle,
				   statusContext->defaultResponderNickname);
    if (cert == NULL) {
        cert = PK11_FindCertFromNickname(statusContext->defaultResponderNickname,
                                         NULL);
    }
    



    PORT_Assert(cert != NULL);
    if (cert == NULL)
	return SECFailure;

   




    rv = CERT_VerifyCertificateNow(handle, cert, PR_TRUE,
                                   certificateUsageCheckAllUsages,
                                   NULL, &usage);
    if (rv != SECSuccess || (usage & (certificateUsageSSLClient |
                                      certificateUsageSSLServer |
                                      certificateUsageSSLServerWithStepUp |
                                      certificateUsageEmailSigner |
                                      certificateUsageObjectSigner |
                                      certificateUsageStatusResponder |
                                      certificateUsageSSLCA)) == 0) {
	PORT_SetError(SEC_ERROR_OCSP_RESPONDER_CERT_INVALID);
	return SECFailure;
    }

    


    statusContext->defaultResponderCert = cert;

    
    CERT_ClearOCSPCache();

    


    statusContext->useDefaultResponder = PR_TRUE;
    return SECSuccess;
}














SECStatus
CERT_DisableOCSPDefaultResponder(CERTCertDBHandle *handle)
{
    CERTStatusConfig *statusConfig;
    ocspCheckingContext *statusContext;
    CERTCertificate *tmpCert;

    if (handle == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    statusConfig = CERT_GetStatusConfig(handle);
    if (statusConfig == NULL)
	return SECSuccess;

    statusContext = ocsp_GetCheckingContext(handle);
    PORT_Assert(statusContext != NULL);
    if (statusContext == NULL)
	return SECFailure;

    tmpCert = statusContext->defaultResponderCert;
    if (tmpCert) {
	statusContext->defaultResponderCert = NULL;
	CERT_DestroyCertificate(tmpCert);
        
        CERT_ClearOCSPCache();
    }

    


    statusContext->useDefaultResponder = PR_FALSE;
    return SECSuccess;
}


SECStatus
CERT_GetOCSPResponseStatus(CERTOCSPResponse *response)
{
    PORT_Assert(response);
    if (response->statusValue == ocspResponse_successful)
	return SECSuccess;

    switch (response->statusValue) {
      case ocspResponse_malformedRequest:
	PORT_SetError(SEC_ERROR_OCSP_MALFORMED_REQUEST);
	break;
      case ocspResponse_internalError:
	PORT_SetError(SEC_ERROR_OCSP_SERVER_ERROR);
	break;
      case ocspResponse_tryLater:
	PORT_SetError(SEC_ERROR_OCSP_TRY_SERVER_LATER);
	break;
      case ocspResponse_sigRequired:
	
	PORT_SetError(SEC_ERROR_OCSP_REQUEST_NEEDS_SIG);
	break;
      case ocspResponse_unauthorized:
	PORT_SetError(SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST);
	break;
      case ocspResponse_other:
      case ocspResponse_unused:
      default:
	PORT_SetError(SEC_ERROR_OCSP_UNKNOWN_RESPONSE_STATUS);
	break;
    }
    return SECFailure;
}
