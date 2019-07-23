







































#ifndef _CERTT_H_
#define _CERTT_H_

#include "prclist.h"
#include "pkcs11t.h"
#include "seccomon.h"
#include "secmodt.h"
#include "secoidt.h"
#include "plarena.h"
#include "prcvar.h"
#include "nssilock.h"
#include "prio.h"
#include "prmon.h"


struct NSSCertificateStr;
struct NSSTrustDomainStr;


typedef struct CERTAVAStr                        CERTAVA;
typedef struct CERTAttributeStr                  CERTAttribute;
typedef struct CERTAuthInfoAccessStr             CERTAuthInfoAccess;
typedef struct CERTAuthKeyIDStr                  CERTAuthKeyID;
typedef struct CERTBasicConstraintsStr           CERTBasicConstraints;
typedef struct NSSTrustDomainStr                 CERTCertDBHandle;
typedef struct CERTCertExtensionStr              CERTCertExtension;
typedef struct CERTCertKeyStr                    CERTCertKey;
typedef struct CERTCertListStr                   CERTCertList;
typedef struct CERTCertListNodeStr               CERTCertListNode;
typedef struct CERTCertNicknamesStr              CERTCertNicknames;
typedef struct CERTCertTrustStr                  CERTCertTrust;
typedef struct CERTCertificateStr                CERTCertificate;
typedef struct CERTCertificateListStr            CERTCertificateList;
typedef struct CERTCertificateRequestStr         CERTCertificateRequest;
typedef struct CERTCrlStr                        CERTCrl;
typedef struct CERTCrlDistributionPointsStr      CERTCrlDistributionPoints; 
typedef struct CERTCrlEntryStr                   CERTCrlEntry;
typedef struct CERTCrlHeadNodeStr                CERTCrlHeadNode;
typedef struct CERTCrlKeyStr                     CERTCrlKey;
typedef struct CERTCrlNodeStr                    CERTCrlNode;
typedef struct CERTDERCertsStr                   CERTDERCerts;
typedef struct CERTDistNamesStr                  CERTDistNames;
typedef struct CERTGeneralNameStr                CERTGeneralName;
typedef struct CERTGeneralNameListStr            CERTGeneralNameList;
typedef struct CERTIssuerAndSNStr                CERTIssuerAndSN;
typedef struct CERTNameStr                       CERTName;
typedef struct CERTNameConstraintStr             CERTNameConstraint;
typedef struct CERTNameConstraintsStr            CERTNameConstraints;
typedef struct CERTOKDomainNameStr               CERTOKDomainName;
typedef struct CERTPrivKeyUsagePeriodStr         CERTPrivKeyUsagePeriod;
typedef struct CERTPublicKeyAndChallengeStr      CERTPublicKeyAndChallenge;
typedef struct CERTRDNStr                        CERTRDN;
typedef struct CERTSignedCrlStr                  CERTSignedCrl;
typedef struct CERTSignedDataStr                 CERTSignedData;
typedef struct CERTStatusConfigStr               CERTStatusConfig;
typedef struct CERTSubjectListStr                CERTSubjectList;
typedef struct CERTSubjectNodeStr                CERTSubjectNode;
typedef struct CERTSubjectPublicKeyInfoStr       CERTSubjectPublicKeyInfo;
typedef struct CERTValidityStr                   CERTValidity;
typedef struct CERTVerifyLogStr                  CERTVerifyLog;
typedef struct CERTVerifyLogNodeStr              CERTVerifyLogNode;
typedef struct CRLDistributionPointStr           CRLDistributionPoint;


typedef unsigned long CERTCrlNumber;




struct CERTAVAStr {
    SECItem type;
    SECItem value;
};




struct CERTRDNStr {
    CERTAVA **avas;
};




struct CERTNameStr {
    PLArenaPool *arena;
    CERTRDN **rdns;
};




struct CERTValidityStr {
    PLArenaPool *arena;
    SECItem notBefore;
    SECItem notAfter;
};




struct CERTCertKeyStr {
    SECItem serialNumber;
    SECItem derIssuer;
};





struct CERTSignedDataStr {
    SECItem data;
    SECAlgorithmID signatureAlgorithm;
    SECItem signature;
};




struct CERTSubjectPublicKeyInfoStr {
    PLArenaPool *arena;
    SECAlgorithmID algorithm;
    SECItem subjectPublicKey;
};

struct CERTPublicKeyAndChallengeStr {
    SECItem spki;
    SECItem challenge;
};

struct CERTCertTrustStr {
    unsigned int sslFlags;
    unsigned int emailFlags;
    unsigned int objectSigningFlags;
};




typedef enum SECTrustTypeEnum {
    trustSSL = 0,
    trustEmail = 1,
    trustObjectSigning = 2,
    trustTypeNone = 3
} SECTrustType;

#define SEC_GET_TRUST_FLAGS(trust,type) \
        (((type)==trustSSL)?((trust)->sslFlags): \
	 (((type)==trustEmail)?((trust)->emailFlags): \
	  (((type)==trustObjectSigning)?((trust)->objectSigningFlags):0)))




struct CERTCertExtensionStr {
    SECItem id;
    SECItem critical;
    SECItem value;
};

struct CERTSubjectNodeStr {
    struct CERTSubjectNodeStr *next;
    struct CERTSubjectNodeStr *prev;
    SECItem certKey;
    SECItem keyID;
};

struct CERTSubjectListStr {
    PLArenaPool *arena;
    int ncerts;
    char *emailAddr;
    CERTSubjectNode *head;
    CERTSubjectNode *tail; 
    void *entry;
};




struct CERTCertificateStr {
    





    PLArenaPool *arena;

    
    char *subjectName;
    char *issuerName;
    CERTSignedData signatureWrap;	
    SECItem derCert;			
    SECItem derIssuer;			
    SECItem derSubject;			
    SECItem derPublicKey;		
    SECItem certKey;			
    SECItem version;
    SECItem serialNumber;
    SECAlgorithmID signature;
    CERTName issuer;
    CERTValidity validity;
    CERTName subject;
    CERTSubjectPublicKeyInfo subjectPublicKeyInfo;
    SECItem issuerID;
    SECItem subjectID;
    CERTCertExtension **extensions;
    char *emailAddr;
    CERTCertDBHandle *dbhandle;
    SECItem subjectKeyID;	
    PRBool keyIDGenerated;	
    unsigned int keyUsage;	
    unsigned int rawKeyUsage;	
    PRBool keyUsagePresent;	
    PRUint32 nsCertType;	
				

    



    PRBool keepSession;			
    PRBool timeOK;			
    CERTOKDomainName *domainOK;		

    




    PRBool isperm;
    PRBool istemp;
    char *nickname;
    char *dbnickname;
    struct NSSCertificateStr *nssCertificate;	
    CERTCertTrust *trust;

    


    int referenceCount;

    



    CERTSubjectList *subjectList;

    


    CERTAuthKeyID * authKeyID;  
    PRBool isRoot;              

    




    union {
        void* apointer; 
        struct {
            unsigned int hasUnsupportedCriticalExt :1;
            
        } bits;
    } options;
    int series; 

    
    PK11SlotInfo *slot;		
    CK_OBJECT_HANDLE pkcs11ID;	
    PRBool ownSlot;		
};
#define SEC_CERTIFICATE_VERSION_1		0	/* default created */
#define SEC_CERTIFICATE_VERSION_2		1	/* v2 */
#define SEC_CERTIFICATE_VERSION_3		2	/* v3 extensions */

#define SEC_CRL_VERSION_1		0	/* default */
#define SEC_CRL_VERSION_2		1	/* v2 extensions */




#define SEC_CERT_CLASS_CA	1
#define SEC_CERT_CLASS_SERVER	2
#define SEC_CERT_CLASS_USER	3
#define SEC_CERT_CLASS_EMAIL	4

struct CERTDERCertsStr {
    PLArenaPool *arena;
    int numcerts;
    SECItem *rawCerts;
};






struct CERTAttributeStr {
    SECItem attrType;
    SECItem **attrValue;
};




struct CERTCertificateRequestStr {
    PLArenaPool *arena;
    SECItem version;
    CERTName subject;
    CERTSubjectPublicKeyInfo subjectPublicKeyInfo;
    CERTAttribute **attributes;
};
#define SEC_CERTIFICATE_REQUEST_VERSION		0	/* what we *create* */





struct CERTCertificateListStr {
    SECItem *certs;
    int len;					
    PLArenaPool *arena;
};

struct CERTCertListNodeStr {
    PRCList links;
    CERTCertificate *cert;
    void *appData;
};

struct CERTCertListStr {
    PRCList list;
    PLArenaPool *arena;
};

#define CERT_LIST_HEAD(l) ((CERTCertListNode *)PR_LIST_HEAD(&l->list))
#define CERT_LIST_NEXT(n) ((CERTCertListNode *)n->links.next)
#define CERT_LIST_END(n,l) (((void *)n) == ((void *)&l->list))
#define CERT_LIST_EMPTY(l) CERT_LIST_END(CERT_LIST_HEAD(l), l)

struct CERTCrlEntryStr {
    SECItem serialNumber;
    SECItem revocationDate;
    CERTCertExtension **extensions;    
};

struct CERTCrlStr {
    PLArenaPool *arena;
    SECItem version;
    SECAlgorithmID signatureAlg;
    SECItem derName;
    CERTName name;
    SECItem lastUpdate;
    SECItem nextUpdate;				
    CERTCrlEntry **entries;
    CERTCertExtension **extensions;    
    
};

struct CERTCrlKeyStr {
    SECItem derName;
    SECItem dummy;			



};

struct CERTSignedCrlStr {
    PLArenaPool *arena;
    CERTCrl crl;
    void *reserved1;
    PRBool reserved2;
    PRBool isperm;
    PRBool istemp;
    int referenceCount;
    CERTCertDBHandle *dbhandle;
    CERTSignedData signatureWrap;	
    char *url;
    SECItem *derCrl;
    PK11SlotInfo *slot;
    CK_OBJECT_HANDLE pkcs11ID;
    void* opaque; 
};


struct CERTCrlHeadNodeStr {
    PLArenaPool *arena;
    CERTCertDBHandle *dbhandle;
    CERTCrlNode *first;
    CERTCrlNode *last;
};


struct CERTCrlNodeStr {
    CERTCrlNode *next;
    int 	type;
    CERTSignedCrl *crl;
};





struct CERTDistNamesStr {
    PLArenaPool *arena;
    int nnames;
    SECItem  *names;
    void *head; 
};


#define NS_CERT_TYPE_SSL_CLIENT		(0x80)	/* bit 0 */
#define NS_CERT_TYPE_SSL_SERVER		(0x40)  /* bit 1 */
#define NS_CERT_TYPE_EMAIL		(0x20)  /* bit 2 */
#define NS_CERT_TYPE_OBJECT_SIGNING	(0x10)  /* bit 3 */
#define NS_CERT_TYPE_RESERVED		(0x08)  /* bit 4 */
#define NS_CERT_TYPE_SSL_CA		(0x04)  /* bit 5 */
#define NS_CERT_TYPE_EMAIL_CA		(0x02)  /* bit 6 */
#define NS_CERT_TYPE_OBJECT_SIGNING_CA	(0x01)  /* bit 7 */

#define EXT_KEY_USAGE_TIME_STAMP        (0x8000)
#define EXT_KEY_USAGE_STATUS_RESPONDER	(0x4000)

#define NS_CERT_TYPE_APP ( NS_CERT_TYPE_SSL_CLIENT | \
			  NS_CERT_TYPE_SSL_SERVER | \
			  NS_CERT_TYPE_EMAIL | \
			  NS_CERT_TYPE_OBJECT_SIGNING )

#define NS_CERT_TYPE_CA ( NS_CERT_TYPE_SSL_CA | \
			 NS_CERT_TYPE_EMAIL_CA | \
			 NS_CERT_TYPE_OBJECT_SIGNING_CA | \
			 EXT_KEY_USAGE_STATUS_RESPONDER )
typedef enum SECCertUsageEnum {
    certUsageSSLClient = 0,
    certUsageSSLServer = 1,
    certUsageSSLServerWithStepUp = 2,
    certUsageSSLCA = 3,
    certUsageEmailSigner = 4,
    certUsageEmailRecipient = 5,
    certUsageObjectSigner = 6,
    certUsageUserCertImport = 7,
    certUsageVerifyCA = 8,
    certUsageProtectedObjectSigner = 9,
    certUsageStatusResponder = 10,
    certUsageAnyCA = 11
} SECCertUsage;

typedef PRInt64 SECCertificateUsage;

#define certificateUsageCheckAllUsages         (0x0000)
#define certificateUsageSSLClient              (0x0001)
#define certificateUsageSSLServer              (0x0002)
#define certificateUsageSSLServerWithStepUp    (0x0004)
#define certificateUsageSSLCA                  (0x0008)
#define certificateUsageEmailSigner            (0x0010)
#define certificateUsageEmailRecipient         (0x0020)
#define certificateUsageObjectSigner           (0x0040)
#define certificateUsageUserCertImport         (0x0080)
#define certificateUsageVerifyCA               (0x0100)
#define certificateUsageProtectedObjectSigner  (0x0200)
#define certificateUsageStatusResponder        (0x0400)
#define certificateUsageAnyCA                  (0x0800)

#define certificateUsageHighest certificateUsageAnyCA




typedef enum CERTCertOwnerEnum {
    certOwnerUser = 0,
    certOwnerPeer = 1,
    certOwnerCA = 2
} CERTCertOwner;




typedef enum SECCertTimeValidityEnum {
    secCertTimeValid = 0,
    secCertTimeExpired = 1,
    secCertTimeNotValidYet = 2,
    secCertTimeUndetermined = 3 

} SECCertTimeValidity;







typedef enum CERTCompareValidityStatusEnum
{
    certValidityUndetermined = 0, 

    certValidityChooseB = 1,      
    certValidityEqual = 2,        
    certValidityChooseA = 3       
} CERTCompareValidityStatus;






#define SEC_CERT_NICKNAMES_ALL		1
#define SEC_CERT_NICKNAMES_USER		2
#define SEC_CERT_NICKNAMES_SERVER	3
#define SEC_CERT_NICKNAMES_CA		4

struct CERTCertNicknamesStr {
    PLArenaPool *arena;
    void *head;
    int numnicknames;
    char **nicknames;
    int what;
    int totallen;
};

struct CERTIssuerAndSNStr {
    SECItem derIssuer;
    CERTName issuer;
    SECItem serialNumber;
};



#define KU_DIGITAL_SIGNATURE		(0x80)	/* bit 0 */
#define KU_NON_REPUDIATION		(0x40)  /* bit 1 */
#define KU_KEY_ENCIPHERMENT		(0x20)  /* bit 2 */
#define KU_DATA_ENCIPHERMENT		(0x10)  /* bit 3 */
#define KU_KEY_AGREEMENT		(0x08)  /* bit 4 */
#define KU_KEY_CERT_SIGN		(0x04)  /* bit 5 */
#define KU_CRL_SIGN			(0x02)  /* bit 6 */
#define KU_ENCIPHER_ONLY		(0x01)  /* bit 7 */
#define KU_ALL				(KU_DIGITAL_SIGNATURE | \
					 KU_NON_REPUDIATION | \
					 KU_KEY_ENCIPHERMENT | \
					 KU_DATA_ENCIPHERMENT | \
					 KU_KEY_AGREEMENT | \
					 KU_KEY_CERT_SIGN | \
					 KU_CRL_SIGN | \
					 KU_ENCIPHER_ONLY)





#define KU_KEY_AGREEMENT_OR_ENCIPHERMENT (0x4000)




#define KU_NS_GOVT_APPROVED		(0x8000) /*don't make part of KU_ALL!*/









#define CERT_UNLIMITED_PATH_CONSTRAINT -2

struct CERTBasicConstraintsStr {
    PRBool isCA;			
    int pathLenConstraint;		



};


#define CERT_MAX_CERT_CHAIN 20

#define CERT_MAX_SERIAL_NUMBER_BYTES  20    /* from RFC 3280 */
#define CERT_MAX_DN_BYTES             4096  /* arbitrary */


#define RF_UNUSED			(0x80)	/* bit 0 */
#define RF_KEY_COMPROMISE		(0x40)  /* bit 1 */
#define RF_CA_COMPROMISE		(0x20)  /* bit 2 */
#define RF_AFFILIATION_CHANGED		(0x10)  /* bit 3 */
#define RF_SUPERSEDED			(0x08)  /* bit 4 */
#define RF_CESSATION_OF_OPERATION	(0x04)  /* bit 5 */
#define RF_CERTIFICATE_HOLD		(0x02)  /* bit 6 */


typedef enum CERTCRLEntryReasonCodeEnum {
    crlEntryReasonUnspecified = 0,
    crlEntryReasonKeyCompromise = 1,
    crlEntryReasonCaCompromise = 2,
    crlEntryReasonAffiliationChanged = 3,
    crlEntryReasonSuperseded = 4,
    crlEntryReasonCessationOfOperation = 5,
    crlEntryReasoncertificatedHold = 6,
    crlEntryReasonRemoveFromCRL = 8,
    crlEntryReasonPrivilegeWithdrawn = 9,
    crlEntryReasonAaCompromise = 10
} CERTCRLEntryReasonCode;



typedef enum CERTGeneralNameTypeEnum {
    certOtherName = 1,
    certRFC822Name = 2,
    certDNSName = 3,
    certX400Address = 4,
    certDirectoryName = 5,
    certEDIPartyName = 6,
    certURI = 7,
    certIPAddress = 8,
    certRegisterID = 9
} CERTGeneralNameType;


typedef struct OtherNameStr {
    SECItem          name;
    SECItem          oid;
}OtherName;



struct CERTGeneralNameStr {
    CERTGeneralNameType type;		
    union {
	CERTName directoryName;         
	OtherName  OthName;		
	SECItem other;                  
    }name;
    SECItem derDirectoryName;		

    PRCList l;
};

struct CERTGeneralNameListStr {
    PLArenaPool *arena;
    CERTGeneralName *name;
    int refCount;
    int len;
    PZLock *lock;
};

struct CERTNameConstraintStr {
    CERTGeneralName  name;
    SECItem          DERName;
    SECItem          min;
    SECItem          max;
    PRCList          l;
};


struct CERTNameConstraintsStr {
    CERTNameConstraint  *permited;
    CERTNameConstraint  *excluded;
    SECItem             **DERPermited;
    SECItem             **DERExcluded;
};



struct CERTPrivKeyUsagePeriodStr {
    SECItem notBefore;
    SECItem notAfter;
    PLArenaPool *arena;
};




struct CERTAuthKeyIDStr {
    SECItem keyID;			
    CERTGeneralName *authCertIssuer;	
    SECItem authCertSerialNumber;	
    SECItem **DERAuthCertIssuer;	




};






typedef enum DistributionPointTypesEnum {
    generalName = 1,			
    relativeDistinguishedName = 2
} DistributionPointTypes;

struct CRLDistributionPointStr {
    DistributionPointTypes distPointType;
    union {
	CERTGeneralName *fullName;
	CERTRDN relativeName;
    } distPoint;
    SECItem reasons;
    CERTGeneralName *crlIssuer;
    
    
    SECItem derDistPoint;
    SECItem derRelativeName;
    SECItem **derCrlIssuer;
    SECItem **derFullName;
    SECItem bitsmap;
};

struct CERTCrlDistributionPointsStr {
    CRLDistributionPoint **distPoints;
};






struct CERTVerifyLogNodeStr {
    CERTCertificate *cert;	
    long error;			
    unsigned int depth;		
    void *arg;			
    struct CERTVerifyLogNodeStr *next; 
    struct CERTVerifyLogNodeStr *prev; 
};


struct CERTVerifyLogStr {
    PLArenaPool *arena;
    unsigned int count;
    struct CERTVerifyLogNodeStr *head;
    struct CERTVerifyLogNodeStr *tail;
};


struct CERTOKDomainNameStr {
    CERTOKDomainName *next;
    char              name[1]; 
};


typedef SECStatus (PR_CALLBACK *CERTStatusChecker) (CERTCertDBHandle *handle,
						    CERTCertificate *cert,
						    PRTime time,
						    void *pwArg);

typedef SECStatus (PR_CALLBACK *CERTStatusDestroy) (CERTStatusConfig *handle);

struct CERTStatusConfigStr {
    CERTStatusChecker statusChecker;	
    CERTStatusDestroy statusDestroy;	
    void *statusContext;		
};

struct CERTAuthInfoAccessStr {
    SECItem method;
    SECItem derLocation;
    CERTGeneralName *location;		
};




typedef char * (*CERTDBNameFunc)(void *arg, int dbVersion);




typedef enum CERTPackageTypeEnum {
    certPackageNone = 0,
    certPackageCert = 1,
    certPackagePKCS7 = 2,
    certPackageNSCertSeq = 3,
    certPackageNSCertWrap = 4
} CERTPackageType;




typedef struct {
    SECOidTag oid;
    SECItem qualifierID;
    SECItem qualifierValue;
} CERTPolicyQualifier;

typedef struct {
    SECOidTag oid;
    SECItem policyID;
    CERTPolicyQualifier **policyQualifiers;
} CERTPolicyInfo;

typedef struct {
    PLArenaPool *arena;
    CERTPolicyInfo **policyInfos;
} CERTCertificatePolicies;

typedef struct {
    SECItem organization;
    SECItem **noticeNumbers;
} CERTNoticeReference;

typedef struct {
    PLArenaPool *arena;
    CERTNoticeReference noticeReference;
    SECItem derNoticeReference;
    SECItem displayText;
} CERTUserNotice;

typedef struct {
    PLArenaPool *arena;
    SECItem **oids;
} CERTOidSequence;




typedef struct {
    SECItem issuerDomainPolicy;
    SECItem subjectDomainPolicy;
} CERTPolicyMap;

typedef struct {
    PLArenaPool *arena;
    CERTPolicyMap **policyMaps;
} CERTCertificatePolicyMappings;




typedef struct {
    SECItem inhibitAnySkipCerts;
} CERTCertificateInhibitAny;




typedef struct {
    SECItem explicitPolicySkipCerts;
    SECItem inhibitMappingSkipCerts;
} CERTCertificatePolicyConstraints;







typedef enum {
   cert_pi_end             = 0, 

   cert_pi_nbioContext     = 1, 





   cert_pi_nbioAbort       = 2, 






   cert_pi_certList        = 3, 



   cert_pi_policyOID       = 4, 



   cert_pi_policyFlags     = 5, 




   cert_pi_keyusage        = 6, 






   cert_pi_extendedKeyusage= 7, 






   cert_pi_date            = 8, 


   cert_pi_revocationFlags = 9, 


   cert_pi_certStores      = 10,

   cert_pi_trustAnchors    = 11,



   cert_pi_useAIACertFetch = 12, 


   cert_pi_max                  

} CERTValParamInType;









typedef enum {
   cert_po_end             = 0, 

   cert_po_nbioContext     = 1, 






   cert_po_trustAnchor     = 2, 



   cert_po_certList        = 3, 



   cert_po_policyOID       = 4, 



   cert_po_errorLog        = 5, 

   cert_po_usages          = 6, 

   cert_po_keyUsage        = 7, 


   cert_po_extendedKeyusage= 8, 


   cert_po_max                  


} CERTValParamOutType;

typedef enum {
    cert_revocation_method_crl = 0,
    cert_revocation_method_ocsp,
    cert_revocation_method_count
} CERTRevocationMethodIndex;














#define CERT_REV_M_DO_NOT_TEST_USING_THIS_METHOD     0L
#define CERT_REV_M_TEST_USING_THIS_METHOD            1L







#define CERT_REV_M_ALLOW_NETWORK_FETCHING            0L
#define CERT_REV_M_FORBID_NETWORK_FETCHING           2L












 
#define CERT_REV_M_ALLOW_IMPLICIT_DEFAULT_SOURCE     0L
#define CERT_REV_M_IGNORE_IMPLICIT_DEFAULT_SOURCE    4L













#define CERT_REV_M_SKIP_TEST_ON_MISSING_SOURCE       0L
#define CERT_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE    8L








#define CERT_REV_M_IGNORE_MISSING_FRESH_INFO         0L
#define CERT_REV_M_FAIL_ON_MISSING_FRESH_INFO        16L











#define CERT_REV_M_STOP_TESTING_ON_FRESH_INFO        0L
#define CERT_REV_M_CONTINUE_TESTING_ON_FRESH_INFO    32L




















#define CERT_REV_MI_TEST_EACH_METHOD_SEPARATELY       0L
#define CERT_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST  1L












#define CERT_REV_MI_NO_OVERALL_INFO_REQUIREMENT       0L
#define CERT_REV_MI_REQUIRE_SOME_FRESH_INFO_AVAILABLE 2L


typedef struct {
    




    PRUint32 number_of_defined_methods;

    











 
    PRUint64 *cert_rev_flags_per_method;

    






    PRUint32 number_of_preferred_methods;

    






    CERTRevocationMethodIndex *preferred_methods;

    




    PRUint64 cert_rev_method_independent_flags;
} CERTRevocationTests;

typedef struct {
    CERTRevocationTests leafTests;
    CERTRevocationTests chainTests;
} CERTRevocationFlags;

typedef struct CERTValParamInValueStr {
    union {
        PRBool   b;
        PRInt32  i;
        PRUint32 ui;
        PRInt64  l;
        PRUint64 ul;
        PRTime time;
    } scalar;
    union {
        const void*    p;
        const char*    s;
        const CERTCertificate* cert;
        const CERTCertList *chain;
        const CERTRevocationFlags *revocation;
    } pointer;
    union {
        const PRInt32  *pi;
        const PRUint32 *pui;
        const PRInt64  *pl;
        const PRUint64 *pul;
        const SECOidTag *oids;
    } array;
    int arraySize;
} CERTValParamInValue;


typedef struct CERTValParamOutValueStr {
    union {
        PRBool   b;
        PRInt32  i;
        PRUint32 ui;
        PRInt64  l;
        PRUint64 ul;
        SECCertificateUsage usages;
    } scalar;
    union {
        void*    p;
        char*    s;
        CERTVerifyLog *log;
        CERTCertificate* cert;
        CERTCertList *chain;
    } pointer;
    union {
        void 	  *p;
        SECOidTag *oids;
    } array;
    int arraySize;
} CERTValParamOutValue;

typedef struct {
    CERTValParamInType type;
    CERTValParamInValue value;
} CERTValInParam;

typedef struct {
    CERTValParamOutType type;
    CERTValParamOutValue value;
} CERTValOutParam;




typedef enum CertStrictnessLevels {
    CERT_N2A_READABLE   =  0, 
    CERT_N2A_STRICT     = 10, 
    CERT_N2A_INVERTIBLE = 20  

} CertStrictnessLevel;




#define CERT_POLICY_FLAG_NO_MAPPING    1
#define CERT_POLICY_FLAG_EXPLICIT      2
#define CERT_POLICY_FLAG_NO_ANY        4




#define CERT_ENABLE_LDAP_FETCH          1
#define CERT_ENABLE_HTTP_FETCH          2





typedef char * (*CERT_StringFromCertFcn)(CERTCertificate *cert);



#include "secasn1t.h"	


SEC_BEGIN_PROTOS

extern const SEC_ASN1Template CERT_CertificateRequestTemplate[];
extern const SEC_ASN1Template CERT_CertificateTemplate[];
extern const SEC_ASN1Template SEC_SignedCertificateTemplate[];
extern const SEC_ASN1Template CERT_CertExtensionTemplate[];
extern const SEC_ASN1Template CERT_SequenceOfCertExtensionTemplate[];
extern const SEC_ASN1Template SECKEY_PublicKeyTemplate[];
extern const SEC_ASN1Template CERT_SubjectPublicKeyInfoTemplate[];
extern const SEC_ASN1Template CERT_TimeChoiceTemplate[];
extern const SEC_ASN1Template CERT_ValidityTemplate[];
extern const SEC_ASN1Template CERT_PublicKeyAndChallengeTemplate[];
extern const SEC_ASN1Template SEC_CertSequenceTemplate[];

extern const SEC_ASN1Template CERT_IssuerAndSNTemplate[];
extern const SEC_ASN1Template CERT_NameTemplate[];
extern const SEC_ASN1Template CERT_SetOfSignedCrlTemplate[];
extern const SEC_ASN1Template CERT_RDNTemplate[];
extern const SEC_ASN1Template CERT_SignedDataTemplate[];
extern const SEC_ASN1Template CERT_CrlTemplate[];
extern const SEC_ASN1Template CERT_SignedCrlTemplate[];




extern const SEC_ASN1Template CERT_AttributeTemplate[];
extern const SEC_ASN1Template CERT_SetOfAttributeTemplate[];




SEC_ASN1_CHOOSER_DECLARE(CERT_CertificateRequestTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_CertificateTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_CrlTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_IssuerAndSNTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_NameTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_SequenceOfCertExtensionTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_SetOfSignedCrlTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_SignedDataTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_SubjectPublicKeyInfoTemplate)
SEC_ASN1_CHOOSER_DECLARE(SEC_SignedCertificateTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_SignedCrlTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_TimeChoiceTemplate)

SEC_END_PROTOS

#endif 
