







































#ifndef _CERTI_H_
#define _CERTI_H_

#include "certt.h"
#include "nssrwlkt.h"





#define DPC_RWLOCK 1



typedef struct OpaqueCRLFieldsStr OpaqueCRLFields;
typedef struct CRLEntryCacheStr CRLEntryCache;
typedef struct CRLDPCacheStr CRLDPCache;
typedef struct CRLIssuerCacheStr CRLIssuerCache;
typedef struct CRLCacheStr CRLCache;
typedef struct CachedCrlStr CachedCrl;
typedef struct NamedCRLCacheStr NamedCRLCache;
typedef struct NamedCRLCacheEntryStr NamedCRLCacheEntry;

struct OpaqueCRLFieldsStr {
    PRBool partial;
    PRBool decodingError;
    PRBool badEntries;
    PRBool badDER;
    PRBool badExtensions;
    PRBool heapDER;
};

typedef struct PreAllocatorStr PreAllocator;

struct PreAllocatorStr
{
    PRSize len;
    void* data;
    PRSize used;
    PRArenaPool* arena;
    PRSize extra;
};





struct CRLEntryCacheStr {
    CERTCrlEntry entry;
    CRLEntryCache *prev, *next;
};

#define CRL_CACHE_INVALID_CRLS              0x0001 /* this state will be set
        if we have CRL objects with an invalid DER or signature. Can be
        cleared if the invalid objects are deleted from the token */
#define CRL_CACHE_LAST_FETCH_FAILED         0x0002 /* this state will be set
        if the last CRL fetch encountered an error. Can be cleared if a
        new fetch succeeds */

#define CRL_CACHE_OUT_OF_MEMORY             0x0004 /* this state will be set
        if we don't have enough memory to build the hash table of entries */

typedef enum {
    CRL_OriginToken = 0,    
    CRL_OriginExplicit = 1  
} CRLOrigin;

typedef enum {
    dpcacheNoEntry = 0,             
    dpcacheFoundEntry = 1,          
    dpcacheCallerError = 2,         
    dpcacheInvalidCacheError = 3,   
                                    
    dpcacheEmpty = 4,               
    dpcacheLookupError = 5          
} dpcacheStatus;


struct CachedCrlStr {
    CERTSignedCrl* crl;
    CRLOrigin origin;
    










    PLHashTable* entries;
    PreAllocator* prebuffer; 
    PRBool sigChecked; 
    PRBool sigValid; 

    PRBool unbuildable; 

};






struct CRLDPCacheStr {
#ifdef DPC_RWLOCK
    NSSRWLock* lock;
#else
    PRLock* lock;
#endif
    CERTCertificate* issuer;    




    SECItem* subject;           
    SECItem* distributionPoint; 




    
    PRUint32 ncrls;              
    CachedCrl** crls;            
    



    
    CachedCrl* selected;    
#if 0
    
    PRInt32 numdeltas;      
    CachedCrl** deltas;     
#endif
    
    PRUint16 invalid;       




    PRBool refresh;        
    PRBool mustchoose;     

    PRTime lastfetch;      
    PRTime lastcheck;      

};








struct CRLIssuerCacheStr {
    SECItem* subject;           
    CRLDPCache* dpp;
#if 0
    


    NSSRWLock* lock;
    CRLDPCache** dps;
    PLHashTable* distributionpoints;
    CERTCertificate* issuer;
#endif
};





struct CRLCacheStr {
#ifdef GLOBAL_RWLOCK
    NSSRWLock* lock;
#else
    PRLock* lock;
#endif
    

    PLHashTable* issuers;
};

SECStatus InitCRLCache(void);
SECStatus ShutdownCRLCache(void);





extern char * cert_GetCertificateEmailAddresses(CERTCertificate *cert);




SECStatus
cert_CreateSubjectKeyIDHashTable(void);

SECStatus
cert_AddSubjectKeyIDMapping(SECItem *subjKeyID, CERTCertificate *cert);




SECStatus
cert_RemoveSubjectKeyIDMapping(SECItem *subjKeyID);

SECStatus
cert_DestroySubjectKeyIDHashTable(void);

SECItem*
cert_FindDERCertBySubjectKeyID(SECItem *subjKeyID);


extern int cert_AVAOidTagToMaxLen(SECOidTag tag);


extern CERTAVA * CERT_CreateAVAFromRaw(PRArenaPool *pool, 
                               const SECItem * OID, const SECItem * value);


extern CERTAVA * CERT_CreateAVAFromSECItem(PRArenaPool *arena, SECOidTag kind, 
                                           int valueType, SECItem *value);





SECStatus AcquireDPCache(CERTCertificate* issuer, const SECItem* subject,
                         const SECItem* dp, int64 t, void* wincx,
                         CRLDPCache** dpcache, PRBool* writeLocked);


dpcacheStatus DPCache_Lookup(CRLDPCache* cache, SECItem* sn,
                             CERTCrlEntry** returned);


void ReleaseDPCache(CRLDPCache* dpcache, PRBool writeLocked);


SECStatus DPCache_GetAllCRLs(CRLDPCache* dpc, PRArenaPool* arena,
                             CERTSignedCrl*** crls, PRUint16* status);


SECStatus DPCache_GetCRLEntry(CRLDPCache* cache, PRBool readlocked,
                              CERTSignedCrl* crl, SECItem* sn,
                              CERTCrlEntry** returned);






void CERT_MapStanError();



SECStatus
cert_VerifyCertChainPkix(CERTCertificate *cert,
                         PRBool checkSig,
                         SECCertUsage     requiredUsage,
                         PRTime           time,
                         void            *wincx,
                         CERTVerifyLog   *log,
                         PRBool          *sigError,
                         PRBool          *revoked);

SECStatus cert_InitLocks(void);

SECStatus cert_DestroyLocks(void);




extern SECStatus cert_GetCertType(CERTCertificate *cert);





extern PRUint32 cert_ComputeCertType(CERTCertificate *cert);

void cert_AddToVerifyLog(CERTVerifyLog *log,CERTCertificate *cert,
                         unsigned long errorCode, unsigned int depth,
                         void *arg);














 
SECStatus cert_CacheCRLByGeneralName(CERTCertDBHandle* dbhandle, SECItem* crl,
                                     const SECItem* canonicalizedName);

struct NamedCRLCacheStr {
    PRLock* lock;
    PLHashTable* entries;
};



struct NamedCRLCacheEntryStr {
    SECItem* canonicalizedName;
    SECItem* crl;                   

    PRBool inCRLCache;
    PRTime successfulInsertionTime; 
    PRTime lastAttemptTime;         

    PRBool badDER;      
    PRBool dupe;        
    PRBool unsupported; 
};

typedef enum {
    certRevocationStatusRevoked = 0,
    certRevocationStatusValid = 1,
    certRevocationStatusUnknown = 2
} CERTRevocationStatus;



SECStatus
cert_CheckCertRevocationStatus(CERTCertificate* cert, CERTCertificate* issuer,
                               const SECItem* dp, PRTime t, void *wincx,
                               CERTRevocationStatus *revStatus,
                               CERTCRLEntryReasonCode *revReason);


SECStatus cert_AcquireNamedCRLCache(NamedCRLCache** returned);




SECStatus cert_FindCRLByGeneralName(NamedCRLCache* ncc,
                                    const SECItem* canonicalizedName,
                                    NamedCRLCacheEntry** retEntry);

SECStatus cert_ReleaseNamedCRLCache(NamedCRLCache* ncc);

#endif 

