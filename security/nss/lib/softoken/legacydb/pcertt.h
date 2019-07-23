







































#ifndef _PCERTT_H_
#define _PCERTT_H_

#include "prclist.h"
#include "pkcs11t.h"
#include "seccomon.h"
#include "secoidt.h"
#include "plarena.h"
#include "prcvar.h"
#include "nssilock.h"
#include "prio.h"
#include "prmon.h"


typedef struct NSSLOWCERTCertDBHandleStr               NSSLOWCERTCertDBHandle;
typedef struct NSSLOWCERTCertKeyStr                    NSSLOWCERTCertKey;

typedef struct NSSLOWCERTTrustStr                      NSSLOWCERTTrust;
typedef struct NSSLOWCERTCertTrustStr                  NSSLOWCERTCertTrust;
typedef struct NSSLOWCERTCertificateStr                NSSLOWCERTCertificate;
typedef struct NSSLOWCERTCertificateListStr            NSSLOWCERTCertificateList;
typedef struct NSSLOWCERTIssuerAndSNStr                NSSLOWCERTIssuerAndSN;
typedef struct NSSLOWCERTSignedDataStr                 NSSLOWCERTSignedData;
typedef struct NSSLOWCERTSubjectPublicKeyInfoStr       NSSLOWCERTSubjectPublicKeyInfo;
typedef struct NSSLOWCERTValidityStr                   NSSLOWCERTValidity;




struct NSSLOWCERTValidityStr {
    PRArenaPool *arena;
    SECItem notBefore;
    SECItem notAfter;
};




struct NSSLOWCERTCertKeyStr {
    SECItem serialNumber;
    SECItem derIssuer;
};





struct NSSLOWCERTSignedDataStr {
    SECItem data;
    SECAlgorithmID signatureAlgorithm;
    SECItem signature;
};




struct NSSLOWCERTSubjectPublicKeyInfoStr {
    PRArenaPool *arena;
    SECAlgorithmID algorithm;
    SECItem subjectPublicKey;
};

typedef struct _certDBEntryCert certDBEntryCert;
typedef struct _certDBEntryRevocation certDBEntryRevocation;

struct NSSLOWCERTCertTrustStr {
    unsigned int sslFlags;
    unsigned int emailFlags;
    unsigned int objectSigningFlags;
};




struct NSSLOWCERTTrustStr {
    NSSLOWCERTTrust *next;
    NSSLOWCERTCertDBHandle *dbhandle;
    SECItem dbKey;			
    certDBEntryCert *dbEntry;		
    NSSLOWCERTCertTrust *trust;
    SECItem *derCert;			
    unsigned char dbKeySpace[512];
};




struct NSSLOWCERTCertificateStr {
    





    NSSLOWCERTCertificate *next;
    NSSLOWCERTCertDBHandle *dbhandle;

    SECItem derCert;			
    SECItem derIssuer;			
    SECItem derSN;
    SECItem serialNumber;
    SECItem derSubject;			
    SECItem derSubjKeyInfo;
    NSSLOWCERTSubjectPublicKeyInfo *subjectPublicKeyInfo;
    SECItem certKey;			
    SECItem validity;
    certDBEntryCert *dbEntry;		
    SECItem subjectKeyID;	
    SECItem extensions;
    char *nickname;
    char *emailAddr;
    NSSLOWCERTCertTrust *trust;

    


    int referenceCount;

    char nicknameSpace[200];
    char emailAddrSpace[200];
    unsigned char certKeySpace[512];
};

#define SEC_CERTIFICATE_VERSION_1		0	/* default created */
#define SEC_CERTIFICATE_VERSION_2		1	/* v2 */
#define SEC_CERTIFICATE_VERSION_3		2	/* v3 extensions */

#define SEC_CRL_VERSION_1		0	/* default */
#define SEC_CRL_VERSION_2		1	/* v2 extensions */

#define NSS_MAX_LEGACY_DB_KEY_SIZE (60 * 1024)

struct NSSLOWCERTIssuerAndSNStr {
    SECItem derIssuer;
    SECItem serialNumber;
};

typedef SECStatus (* NSSLOWCERTCertCallback)(NSSLOWCERTCertificate *cert, void *arg);



typedef char * (*NSSLOWCERTDBNameFunc)(void *arg, int dbVersion);



#include "secasn1t.h"	







#define CERT_DB_FILE_VERSION		8
#define CERT_DB_V7_FILE_VERSION		7
#define CERT_DB_CONTENT_VERSION		2

#define SEC_DB_ENTRY_HEADER_LEN		3
#define SEC_DB_KEY_HEADER_LEN		1











typedef enum {
    certDBEntryTypeVersion = 0,
    certDBEntryTypeCert = 1,
    certDBEntryTypeNickname = 2,
    certDBEntryTypeSubject = 3,
    certDBEntryTypeRevocation = 4,
    certDBEntryTypeKeyRevocation = 5,
    certDBEntryTypeSMimeProfile = 6,
    certDBEntryTypeContentVersion = 7,
    certDBEntryTypeBlob = 8
} certDBEntryType;

typedef struct {
    certDBEntryType type;
    unsigned int version;
    unsigned int flags;
    PRArenaPool *arena;
} certDBEntryCommon;

























struct _certDBEntryCert {
    certDBEntryCommon common;
    certDBEntryCert *next;
    NSSLOWCERTCertTrust trust;
    SECItem derCert;
    char *nickname;
    char nicknameSpace[200];
    unsigned char derCertSpace[2048];
};














typedef struct {
    certDBEntryCommon common;
    char *nickname;
    SECItem subjectName;
} certDBEntryNickname;

#define DB_NICKNAME_ENTRY_HEADER_LEN 2



























typedef struct _certDBEntrySubject {
    certDBEntryCommon common;
    SECItem derSubject;
    unsigned int ncerts;
    char *nickname;
    SECItem *certKeys;
    SECItem *keyIDs;
    char **emailAddrs;
    unsigned int nemailAddrs;
} certDBEntrySubject;

#define DB_SUBJECT_ENTRY_HEADER_LEN 6

























typedef struct {
    certDBEntryCommon common;
    char *emailAddr;
    SECItem subjectName;
    SECItem smimeOptions;
    SECItem optionsDate;
} certDBEntrySMime;

#define DB_SMIME_ENTRY_HEADER_LEN 6



















#define DB_CRL_ENTRY_HEADER_LEN	4
struct _certDBEntryRevocation {
    certDBEntryCommon common;
    SECItem	derCrl;
    char	*url;	
};










typedef struct {
    certDBEntryCommon common;
} certDBEntryVersion;

#define SEC_DB_VERSION_KEY "Version"
#define SEC_DB_VERSION_KEY_LEN sizeof(SEC_DB_VERSION_KEY)










typedef struct {
    certDBEntryCommon common;
    char contentVersion;
} certDBEntryContentVersion;

#define SEC_DB_CONTENT_VERSION_KEY "ContentVersion"
#define SEC_DB_CONTENT_VERSION_KEY_LEN sizeof(SEC_DB_CONTENT_VERSION_KEY)

typedef union {
    certDBEntryCommon         common;
    certDBEntryCert           cert;
    certDBEntryContentVersion content;
    certDBEntryNickname       nickname;
    certDBEntryRevocation     revocation;
    certDBEntrySMime          smime;
    certDBEntrySubject        subject;
    certDBEntryVersion        version;
} certDBEntry;


#define DBCERT_V4_HEADER_LEN	7
#define DB_CERT_V5_ENTRY_HEADER_LEN	7
#define DB_CERT_V6_ENTRY_HEADER_LEN	7
#define DB_CERT_ENTRY_HEADER_LEN	10


#define CERTDB_VALID_PEER	(1<<0)
#define CERTDB_TRUSTED		(1<<1)
#define CERTDB_SEND_WARN	(1<<2)
#define CERTDB_VALID_CA		(1<<3)
#define CERTDB_TRUSTED_CA	(1<<4) /* trusted for issuing server certs */
#define CERTDB_NS_TRUSTED_CA	(1<<5)
#define CERTDB_USER		(1<<6)
#define CERTDB_TRUSTED_CLIENT_CA (1<<7) /* trusted for issuing client certs */
#define CERTDB_INVISIBLE_CA	(1<<8) /* don't show in UI */
#define CERTDB_GOVT_APPROVED_CA	(1<<9) /* can do strong crypto in export ver */
#define CERTDB_NOT_TRUSTED	(1<<10) /* explicitly don't trust this cert */
#define CERTDB_TRUSTED_UNKNOWN	(1<<11) /* accept trust from another source */


#define CERTDB_PRESERVE_TRUST_BITS (CERTDB_USER | CERTDB_VALID_PEER | \
        CERTDB_NS_TRUSTED_CA | CERTDB_VALID_CA | CERTDB_INVISIBLE_CA | \
                                        CERTDB_GOVT_APPROVED_CA)

#endif 
