










































#ifndef _PKIXT_H
#define _PKIXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "secerr.h"































#define PKIX_MAJOR_VERSION              ((PKIX_UInt32) 0)
#define PKIX_MINOR_VERSION              ((PKIX_UInt32) 3)







#define PKIX_MAX_MINOR_VERSION          ((PKIX_UInt32) 4000000000)


#define PKIX_STORE_TYPE_NONE            0
#define PKIX_STORE_TYPE_PK11            1







typedef struct PKIX_ErrorStruct PKIX_Error;
typedef struct PKIX_ProcessingParamsStruct PKIX_ProcessingParams;
typedef struct PKIX_ValidateParamsStruct PKIX_ValidateParams;
typedef struct PKIX_ValidateResultStruct PKIX_ValidateResult;
typedef struct PKIX_ResourceLimitsStruct PKIX_ResourceLimits;
typedef struct PKIX_BuildResultStruct PKIX_BuildResult;
typedef struct PKIX_CertStoreStruct PKIX_CertStore;
typedef struct PKIX_CertChainCheckerStruct PKIX_CertChainChecker;
typedef struct PKIX_RevocationCheckerStruct PKIX_RevocationChecker;
typedef struct PKIX_CertSelectorStruct PKIX_CertSelector;
typedef struct PKIX_CRLSelectorStruct PKIX_CRLSelector;
typedef struct PKIX_ComCertSelParamsStruct PKIX_ComCertSelParams;
typedef struct PKIX_ComCRLSelParamsStruct PKIX_ComCRLSelParams;
typedef struct PKIX_TrustAnchorStruct PKIX_TrustAnchor;
typedef struct PKIX_PolicyNodeStruct PKIX_PolicyNode;
typedef struct PKIX_LoggerStruct PKIX_Logger;
typedef struct PKIX_ListStruct PKIX_List;
typedef struct PKIX_ForwardBuilderStateStruct PKIX_ForwardBuilderState;
typedef struct PKIX_DefaultRevocationCheckerStruct
                        PKIX_DefaultRevocationChecker;
typedef struct PKIX_OcspCheckerStruct PKIX_OcspChecker;
typedef struct PKIX_VerifyNodeStruct PKIX_VerifyNode;







typedef struct PKIX_PL_ObjectStruct PKIX_PL_Object;
typedef struct PKIX_PL_ByteArrayStruct PKIX_PL_ByteArray;
typedef struct PKIX_PL_HashTableStruct PKIX_PL_HashTable;
typedef struct PKIX_PL_MutexStruct PKIX_PL_Mutex;
typedef struct PKIX_PL_RWLockStruct PKIX_PL_RWLock;
typedef struct PKIX_PL_MonitorLockStruct PKIX_PL_MonitorLock;
typedef struct PKIX_PL_BigIntStruct PKIX_PL_BigInt;
typedef struct PKIX_PL_StringStruct PKIX_PL_String;
typedef struct PKIX_PL_OIDStruct PKIX_PL_OID;
typedef struct PKIX_PL_CertStruct PKIX_PL_Cert;
typedef struct PKIX_PL_GeneralNameStruct PKIX_PL_GeneralName;
typedef struct PKIX_PL_X500NameStruct PKIX_PL_X500Name;
typedef struct PKIX_PL_PublicKeyStruct PKIX_PL_PublicKey;
typedef struct PKIX_PL_DateStruct PKIX_PL_Date;
typedef struct PKIX_PL_CertNameConstraintsStruct PKIX_PL_CertNameConstraints;
typedef struct PKIX_PL_CertBasicConstraintsStruct PKIX_PL_CertBasicConstraints;
typedef struct PKIX_PL_CertPoliciesStruct PKIX_PL_CertPolicies;
typedef struct PKIX_PL_CertPolicyInfoStruct PKIX_PL_CertPolicyInfo;
typedef struct PKIX_PL_CertPolicyQualifierStruct PKIX_PL_CertPolicyQualifier;
typedef struct PKIX_PL_CertPolicyMapStruct PKIX_PL_CertPolicyMap;
typedef struct PKIX_PL_CRLStruct PKIX_PL_CRL;
typedef struct PKIX_PL_CRLEntryStruct PKIX_PL_CRLEntry;
typedef struct PKIX_PL_CollectionCertStoreStruct PKIX_PL_CollectionCertStore;
typedef struct PKIX_PL_CollectionCertStoreContext
                        PKIX_PL_CollectionCertStoreContext;
typedef struct PKIX_PL_LdapCertStoreContext PKIX_PL_LdapCertStoreContext;
typedef struct PKIX_PL_LdapRequestStruct PKIX_PL_LdapRequest;
typedef struct PKIX_PL_LdapResponseStruct PKIX_PL_LdapResponse;
typedef struct PKIX_PL_LdapDefaultClientStruct PKIX_PL_LdapDefaultClient;
typedef struct PKIX_PL_SocketStruct PKIX_PL_Socket;
typedef struct PKIX_PL_InfoAccessStruct PKIX_PL_InfoAccess;
typedef struct PKIX_PL_AIAMgrStruct PKIX_PL_AIAMgr;
typedef struct PKIX_PL_OcspCertIDStruct PKIX_PL_OcspCertID;
typedef struct PKIX_PL_OcspRequestStruct PKIX_PL_OcspRequest;
typedef struct PKIX_PL_OcspResponseStruct PKIX_PL_OcspResponse;
typedef struct PKIX_PL_HttpClientStruct PKIX_PL_HttpClient;
typedef struct PKIX_PL_HttpDefaultClientStruct PKIX_PL_HttpDefaultClient;
typedef struct PKIX_PL_HttpCertStoreContextStruct PKIX_PL_HttpCertStoreContext;



















typedef unsigned int PKIX_UInt32;
typedef int PKIX_Int32;

typedef int PKIX_Boolean;





#define PKIX_TYPES \
    TYPEMACRO(AIAMGR), \
    TYPEMACRO(BASICCONSTRAINTSCHECKERSTATE), \
    TYPEMACRO(BIGINT), \
    TYPEMACRO(BUILDRESULT), \
    TYPEMACRO(BYTEARRAY), \
    TYPEMACRO(CERT), \
    TYPEMACRO(CERTBASICCONSTRAINTS), \
    TYPEMACRO(CERTCHAINCHECKER), \
    TYPEMACRO(CERTNAMECONSTRAINTS), \
    TYPEMACRO(CERTNAMECONSTRAINTSCHECKERSTATE), \
    TYPEMACRO(CERTPOLICYCHECKERSTATE), \
    TYPEMACRO(CERTPOLICYINFO), \
    TYPEMACRO(CERTPOLICYMAP), \
    TYPEMACRO(CERTPOLICYNODE), \
    TYPEMACRO(CERTPOLICYQUALIFIER), \
    TYPEMACRO(CERTSELECTOR), \
    TYPEMACRO(CERTSTORE), \
    TYPEMACRO(COLLECTIONCERTSTORECONTEXT), \
    TYPEMACRO(COMCERTSELPARAMS), \
    TYPEMACRO(COMCRLSELPARAMS), \
    TYPEMACRO(CRL), \
    TYPEMACRO(CRLENTRY), \
    TYPEMACRO(CRLSELECTOR), \
    TYPEMACRO(DATE), \
    TYPEMACRO(DEFAULTCRLCHECKERSTATE), \
    TYPEMACRO(DEFAULTREVOCATIONCHECKER), \
    TYPEMACRO(EKUCHECKER), \
    TYPEMACRO(ERROR), \
    TYPEMACRO(FORWARDBUILDERSTATE), \
    TYPEMACRO(GENERALNAME), \
    TYPEMACRO(HASHTABLE), \
    TYPEMACRO(HTTPCERTSTORECONTEXT), \
    TYPEMACRO(HTTPDEFAULTCLIENT), \
    TYPEMACRO(INFOACCESS), \
    TYPEMACRO(LDAPDEFAULTCLIENT), \
    TYPEMACRO(LDAPREQUEST), \
    TYPEMACRO(LDAPRESPONSE), \
    TYPEMACRO(LIST), \
    TYPEMACRO(LOGGER), \
    TYPEMACRO(MONITORLOCK), \
    TYPEMACRO(MUTEX), \
    TYPEMACRO(OBJECT), \
    TYPEMACRO(OCSPCERTID), \
    TYPEMACRO(OCSPCHECKER), \
    TYPEMACRO(OCSPREQUEST), \
    TYPEMACRO(OCSPRESPONSE), \
    TYPEMACRO(OID), \
    TYPEMACRO(PROCESSINGPARAMS), \
    TYPEMACRO(PUBLICKEY), \
    TYPEMACRO(RESOURCELIMITS), \
    TYPEMACRO(REVOCATIONCHECKER), \
    TYPEMACRO(RWLOCK), \
    TYPEMACRO(SIGNATURECHECKERSTATE), \
    TYPEMACRO(SOCKET), \
    TYPEMACRO(STRING), \
    TYPEMACRO(TARGETCERTCHECKERSTATE), \
    TYPEMACRO(TRUSTANCHOR), \
    TYPEMACRO(VALIDATEPARAMS), \
    TYPEMACRO(VALIDATERESULT), \
    TYPEMACRO(VERIFYNODE), \
    TYPEMACRO(X500NAME)

#define TYPEMACRO(type) PKIX_ ## type ## _TYPE

typedef enum {     
   PKIX_TYPES,
   PKIX_NUMTYPES   
} PKIX_TYPENUM;


#ifdef PKIX_USER_OBJECT_TYPE





#define PKIX_USER_OBJECT_TYPEBASE 1000

#endif 










#define PKIX_ERRORCLASSES \
   ERRMACRO(AIAMGR), \
   ERRMACRO(BASICCONSTRAINTSCHECKERSTATE), \
   ERRMACRO(BIGINT), \
   ERRMACRO(BUILD), \
   ERRMACRO(BUILDRESULT), \
   ERRMACRO(BYTEARRAY), \
   ERRMACRO(CERT), \
   ERRMACRO(CERTBASICCONSTRAINTS), \
   ERRMACRO(CERTCHAINCHECKER), \
   ERRMACRO(CERTNAMECONSTRAINTS), \
   ERRMACRO(CERTNAMECONSTRAINTSCHECKERSTATE), \
   ERRMACRO(CERTPOLICYCHECKERSTATE), \
   ERRMACRO(CERTPOLICYINFO), \
   ERRMACRO(CERTPOLICYMAP), \
   ERRMACRO(CERTPOLICYNODE), \
   ERRMACRO(CERTPOLICYQUALIFIER), \
   ERRMACRO(CERTSELECTOR), \
   ERRMACRO(CERTSTORE), \
   ERRMACRO(CERTVFYPKIX), \
   ERRMACRO(COLLECTIONCERTSTORECONTEXT), \
   ERRMACRO(COMCERTSELPARAMS), \
   ERRMACRO(COMCRLSELPARAMS), \
   ERRMACRO(CONTEXT), \
   ERRMACRO(CRL), \
   ERRMACRO(CRLENTRY), \
   ERRMACRO(CRLSELECTOR), \
   ERRMACRO(DATE), \
   ERRMACRO(DEFAULTCRLCHECKERSTATE), \
   ERRMACRO(DEFAULTREVOCATIONCHECKER), \
   ERRMACRO(EKUCHECKER), \
   ERRMACRO(ERROR), \
   ERRMACRO(FATAL), \
   ERRMACRO(FORWARDBUILDERSTATE), \
   ERRMACRO(GENERALNAME), \
   ERRMACRO(HASHTABLE), \
   ERRMACRO(HTTPCERTSTORECONTEXT), \
   ERRMACRO(HTTPDEFAULTCLIENT), \
   ERRMACRO(INFOACCESS), \
   ERRMACRO(LDAPCLIENT), \
   ERRMACRO(LDAPDEFAULTCLIENT), \
   ERRMACRO(LDAPREQUEST), \
   ERRMACRO(LDAPRESPONSE), \
   ERRMACRO(LIFECYCLE), \
   ERRMACRO(LIST), \
   ERRMACRO(LOGGER), \
   ERRMACRO(MEM), \
   ERRMACRO(MONITORLOCK), \
   ERRMACRO(MUTEX), \
   ERRMACRO(OBJECT), \
   ERRMACRO(OCSPCERTID), \
   ERRMACRO(OCSPCHECKER), \
   ERRMACRO(OCSPREQUEST), \
   ERRMACRO(OCSPRESPONSE), \
   ERRMACRO(OID), \
   ERRMACRO(PROCESSINGPARAMS), \
   ERRMACRO(PUBLICKEY), \
   ERRMACRO(RESOURCELIMITS), \
   ERRMACRO(REVOCATIONCHECKER), \
   ERRMACRO(RWLOCK), \
   ERRMACRO(SIGNATURECHECKERSTATE), \
   ERRMACRO(SOCKET), \
   ERRMACRO(STRING), \
   ERRMACRO(TARGETCERTCHECKERSTATE), \
   ERRMACRO(TRUSTANCHOR), \
   ERRMACRO(USERDEFINEDMODULES), \
   ERRMACRO(VALIDATE), \
   ERRMACRO(VALIDATEPARAMS), \
   ERRMACRO(VALIDATERESULT), \
   ERRMACRO(VERIFYNODE), \
   ERRMACRO(X500NAME)

#define ERRMACRO(type) PKIX_ ## type ## _ERROR

typedef enum {     
   PKIX_ERRORCLASSES,
   PKIX_NUMERRORCLASSES   
} PKIX_ERRORCLASS;



#define PKIX_ERRORENTRY(name,desc,plerr) PKIX_ ## name


typedef enum    {
#include "pkix_errorstrings.h"
} PKIX_ERRORCODE;

extern const char * const PKIX_ErrorText[];






#define PKIX_ESCASCII           0
#define PKIX_UTF8               1
#define PKIX_UTF16              2
#define PKIX_UTF8_NULL_TERM     3
#define PKIX_ESCASCII_DEBUG     4






#define PKIX_OTHER_NAME         1
#define PKIX_RFC822_NAME        2
#define PKIX_DNS_NAME           3
#define PKIX_X400_ADDRESS       4
#define PKIX_DIRECTORY_NAME     5
#define PKIX_EDIPARTY_NAME      6
#define PKIX_URI_NAME           7
#define PKIX_IP_NAME            8
#define PKIX_OID_NAME           9






#define PKIX_DIGITAL_SIGNATURE  0x001
#define PKIX_NON_REPUDIATION    0x002
#define PKIX_KEY_ENCIPHERMENT   0x004
#define PKIX_DATA_ENCIPHERMENT  0x008
#define PKIX_KEY_AGREEMENT      0x010
#define PKIX_KEY_CERT_SIGN      0x020
#define PKIX_CRL_SIGN           0x040
#define PKIX_ENCIPHER_ONLY      0x080
#define PKIX_DECIPHER_ONLY      0x100






#define PKIX_UNUSED                     0x001
#define PKIX_KEY_COMPROMISE             0x002
#define PKIX_CA_COMPROMISE              0x004
#define PKIX_AFFILIATION_CHANGED        0x008
#define PKIX_SUPERSEDED                 0x010
#define PKIX_CESSATION_OF_OPERATION     0x020
#define PKIX_CERTIFICATE_HOLD           0x040
#define PKIX_PRIVILEGE_WITHDRAWN        0x080
#define PKIX_AA_COMPROMISE              0x100








#define PKIX_TRUE                       ((PKIX_Boolean) 1)
#define PKIX_FALSE                      ((PKIX_Boolean) 0)






#define PKIX_CERTSEL_ENDENTITY_MIN_PATHLENGTH (-2)
#define PKIX_CERTSEL_ALL_MATCH_MIN_PATHLENGTH (-1)










PKIX_Error* PKIX_ALLOC_ERROR(void);









#define PKIX_UNLIMITED_PATH_CONSTRAINT ((PKIX_Int32) -1)




#define PKIX_CERTKEYUSAGE_OID                  "2.5.29.15"
#define PKIX_CERTSUBJALTNAME_OID               "2.5.29.17"
#define PKIX_BASICCONSTRAINTS_OID              "2.5.29.19"
#define PKIX_CRLREASONCODE_OID                 "2.5.29.21"
#define PKIX_NAMECONSTRAINTS_OID               "2.5.29.30"
#define PKIX_CERTIFICATEPOLICIES_OID           "2.5.29.32"
#define PKIX_CERTIFICATEPOLICIES_ANYPOLICY_OID "2.5.29.32.0"
#define PKIX_POLICYMAPPINGS_OID                "2.5.29.33"
#define PKIX_POLICYCONSTRAINTS_OID             "2.5.29.36"
#define PKIX_EXTENDEDKEYUSAGE_OID              "2.5.29.37"
#define PKIX_INHIBITANYPOLICY_OID              "2.5.29.54"
#define PKIX_NSCERTTYPE_OID "2.16.840.1.113730.1.1"

#ifdef __cplusplus
}
#endif

#endif 
