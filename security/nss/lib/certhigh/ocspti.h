









#ifndef _OCSPTI_H_
#define _OCSPTI_H_

#include "ocspt.h"

#include "certt.h"
#include "plarena.h"
#include "seccomon.h"
#include "secoidt.h"








































typedef struct ocspBasicOCSPResponseStr ocspBasicOCSPResponse;
typedef struct ocspCertStatusStr ocspCertStatus;
typedef struct ocspResponderIDStr ocspResponderID;
typedef struct ocspResponseBytesStr ocspResponseBytes;
typedef struct ocspResponseDataStr ocspResponseData;
typedef struct ocspRevokedInfoStr ocspRevokedInfo;
typedef struct ocspServiceLocatorStr ocspServiceLocator;
typedef struct ocspSignatureStr ocspSignature;
typedef struct ocspSingleRequestStr ocspSingleRequest;
typedef struct ocspSingleResponseStr ocspSingleResponse;
typedef struct ocspTBSRequestStr ocspTBSRequest;





struct CERTOCSPRequestStr {
    PRArenaPool *arena;			
    ocspTBSRequest *tbsRequest;
    ocspSignature *optionalSignature;
};















struct ocspTBSRequestStr {
    SECItem version;			
    SECItem *derRequestorName;		
    CERTGeneralNameList *requestorName;	
    ocspSingleRequest **requestList;
    CERTCertExtension **requestExtensions;
    void *extensionHandle;		
};























struct ocspSignatureStr {
    SECAlgorithmID signatureAlgorithm;
    SECItem signature;			
    SECItem **derCerts;			
    CERTCertificate *cert;		
    PRBool wasChecked;			
    SECStatus status;			
    int failureReason;			
};












struct ocspSingleRequestStr {
    PRArenaPool *arena;			


    CERTOCSPCertID *reqCert;
    CERTCertExtension **singleRequestExtensions;
};








struct CERTOCSPCertIDStr {
    SECAlgorithmID hashAlgorithm;
    SECItem issuerNameHash;		
    SECItem issuerKeyHash;		
    SECItem serialNumber;		
    SECItem issuerSHA1NameHash;		
    SECItem issuerMD5NameHash;              
    SECItem issuerMD2NameHash;
    SECItem issuerSHA1KeyHash;		
    SECItem issuerMD5KeyHash;              
    SECItem issuerMD2KeyHash;
    PRArenaPool *poolp;
};















typedef enum {
    ocspResponse_successful = 0,
    ocspResponse_malformedRequest = 1,
    ocspResponse_internalError = 2,
    ocspResponse_tryLater = 3,
    ocspResponse_unused = 4,
    ocspResponse_sigRequired = 5,
    ocspResponse_unauthorized = 6,
    ocspResponse_other			
} ocspResponseStatus;








struct CERTOCSPResponseStr {
    PRArenaPool *arena;			
    SECItem responseStatus;		
    ocspResponseStatus statusValue;	
    ocspResponseBytes *responseBytes;	
};















struct ocspResponseBytesStr {
    SECItem responseType;		
    SECOidTag responseTypeTag;		
    SECItem response;			
    union {
	ocspBasicOCSPResponse *basic;	
    } decodedResponse;			
};











struct ocspBasicOCSPResponseStr {
    SECItem tbsResponseDataDER;
    ocspResponseData *tbsResponseData;	
    ocspSignature responseSignature;
};






struct ocspResponseDataStr {
    SECItem version;			
    SECItem derResponderID;
    ocspResponderID *responderID;	
    SECItem producedAt;			
    CERTOCSPSingleResponse **responses;
    CERTCertExtension **responseExtensions;
};















typedef enum {
    ocspResponderID_byName,
    ocspResponderID_byKey,
    ocspResponderID_other		
} ocspResponderIDType;

struct ocspResponderIDStr {
    ocspResponderIDType responderIDType;
    union {
	CERTName name;			
	SECItem keyHash;		
	SECItem other;			
    } responderIDValue;
};







struct CERTOCSPSingleResponseStr {
    PRArenaPool *arena;			


    CERTOCSPCertID *certID;
    SECItem derCertStatus;
    ocspCertStatus *certStatus;		
    SECItem thisUpdate;			
    SECItem *nextUpdate;		
    CERTCertExtension **singleExtensions;
};


















typedef enum {
    ocspCertStatus_good,		
    ocspCertStatus_revoked,		
    ocspCertStatus_unknown,		
    ocspCertStatus_other		
} ocspCertStatusType;








struct ocspCertStatusStr {
    ocspCertStatusType certStatusType;	
    union {
	SECItem *goodInfo;		
	ocspRevokedInfo *revokedInfo;	
	SECItem *unknownInfo;		
	SECItem *otherInfo;		
    } certStatusInfo; 
};





struct ocspRevokedInfoStr {
    SECItem revocationTime;		
    SECItem *revocationReason;		
};







struct ocspServiceLocatorStr {
    CERTName *issuer;
    SECItem locator;	
};

#endif 
