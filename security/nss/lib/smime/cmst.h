









































#ifndef _CMST_H_
#define _CMST_H_

#include "seccomon.h"
#include "secoidt.h"
#include "certt.h"
#include "secmodt.h"
#include "secmodt.h"

#include "plarena.h"
































typedef struct NSSCMSMessageStr NSSCMSMessage;

typedef union NSSCMSContentUnion NSSCMSContent;
typedef struct NSSCMSContentInfoStr NSSCMSContentInfo;

typedef struct NSSCMSSignedDataStr NSSCMSSignedData;
typedef struct NSSCMSSignerInfoStr NSSCMSSignerInfo;
typedef struct NSSCMSSignerIdentifierStr NSSCMSSignerIdentifier;

typedef struct NSSCMSEnvelopedDataStr NSSCMSEnvelopedData;
typedef struct NSSCMSOriginatorInfoStr NSSCMSOriginatorInfo;
typedef struct NSSCMSRecipientInfoStr NSSCMSRecipientInfo;

typedef struct NSSCMSDigestedDataStr NSSCMSDigestedData;
typedef struct NSSCMSEncryptedDataStr NSSCMSEncryptedData;

typedef struct NSSCMSGenericWrapperDataStr NSSCMSGenericWrapperData;

typedef struct NSSCMSSMIMEKEAParametersStr NSSCMSSMIMEKEAParameters;

typedef struct NSSCMSAttributeStr NSSCMSAttribute;

typedef struct NSSCMSDecoderContextStr NSSCMSDecoderContext;
typedef struct NSSCMSEncoderContextStr NSSCMSEncoderContext;

typedef struct NSSCMSCipherContextStr NSSCMSCipherContext;
typedef struct NSSCMSDigestContextStr NSSCMSDigestContext;

typedef struct NSSCMSContentInfoPrivateStr NSSCMSContentInfoPrivate;

typedef SECStatus (*NSSCMSGenericWrapperDataCallback)
						(NSSCMSGenericWrapperData *);
typedef   void    (*NSSCMSGenericWrapperDataDestroy) 
						(NSSCMSGenericWrapperData *);

extern const SEC_ASN1Template NSSCMSGenericWrapperDataTemplate[];
extern const SEC_ASN1Template NSS_PointerToCMSGenericWrapperDataTemplate[];

SEC_ASN1_CHOOSER_DECLARE(NSS_PointerToCMSGenericWrapperDataTemplate)
SEC_ASN1_CHOOSER_DECLARE(NSSCMSGenericWrapperDataTemplate)














typedef void (*NSSCMSContentCallback)(void *arg, const char *buf, unsigned long len);







typedef PK11SymKey *(*NSSCMSGetDecryptKeyCallback)(void *arg, SECAlgorithmID *algid);






union NSSCMSContentUnion {
    
    SECItem *			data;
    
    NSSCMSDigestedData *	digestedData;
    NSSCMSEncryptedData	*	encryptedData;
    NSSCMSEnvelopedData	*	envelopedData;
    NSSCMSSignedData *		signedData;
    NSSCMSGenericWrapperData *	genericData;
    
    void *			pointer;
};

struct NSSCMSContentInfoStr {
    SECItem			contentType;
    NSSCMSContent		content;
    
    SECOidData *		contentTypeTag;	

    
    

    SECAlgorithmID		contentEncAlg;
    SECItem *			rawContent;		
							
    
    PK11SymKey *		bulkkey;		
    int				keysize;		

    SECOidTag			contentEncAlgTag;	

    NSSCMSContentInfoPrivate	*privateInfo;		
    void		*reserved;			
};





struct NSSCMSMessageStr {
    NSSCMSContentInfo	contentInfo;		
    
    PLArenaPool *	poolp;
    PRBool		poolp_is_ours;
    int			refCount;
    
    SECAlgorithmID **	detached_digestalgs;
    SECItem **		detached_digests;
    void *		pwfn_arg;
    NSSCMSGetDecryptKeyCallback decrypt_key_cb;
    void *		decrypt_key_cb_arg;
};






struct NSSCMSGenericWrapperDataStr {
    NSSCMSContentInfo	contentInfo;
    
    NSSCMSMessage *	cmsg;
    
};





struct NSSCMSSignedDataStr {
    SECItem			version;
    SECAlgorithmID **		digestAlgorithms;
    NSSCMSContentInfo		contentInfo;
    SECItem **			rawCerts;
    CERTSignedCrl **		crls;
    NSSCMSSignerInfo **		signerInfos;
    
    NSSCMSMessage *		cmsg;			
    SECItem **			digests;
    CERTCertificate **		certs;
    CERTCertificateList **	certLists;
    CERTCertificate **          tempCerts;              


};
#define NSS_CMS_SIGNED_DATA_VERSION_BASIC	1	/* what we *create* */
#define NSS_CMS_SIGNED_DATA_VERSION_EXT		3	/* what we *create* */

typedef enum {
    NSSCMSVS_Unverified = 0,
    NSSCMSVS_GoodSignature = 1,
    NSSCMSVS_BadSignature = 2,
    NSSCMSVS_DigestMismatch = 3,
    NSSCMSVS_SigningCertNotFound = 4,
    NSSCMSVS_SigningCertNotTrusted = 5,
    NSSCMSVS_SignatureAlgorithmUnknown = 6,
    NSSCMSVS_SignatureAlgorithmUnsupported = 7,
    NSSCMSVS_MalformedSignature = 8,
    NSSCMSVS_ProcessingError = 9
} NSSCMSVerificationStatus;

typedef enum {
    NSSCMSSignerID_IssuerSN = 0,
    NSSCMSSignerID_SubjectKeyID = 1
} NSSCMSSignerIDSelector;

struct NSSCMSSignerIdentifierStr {
    NSSCMSSignerIDSelector identifierType;
    union {
	CERTIssuerAndSN *issuerAndSN;
	SECItem *subjectKeyID;
    } id;
};

struct NSSCMSSignerInfoStr {
    SECItem			version;
    NSSCMSSignerIdentifier	signerIdentifier;
    SECAlgorithmID		digestAlg;
    NSSCMSAttribute **		authAttr;
    SECAlgorithmID		digestEncAlg;
    SECItem			encDigest;
    NSSCMSAttribute **		unAuthAttr;
    
    NSSCMSMessage *		cmsg;			
    CERTCertificate *		cert;
    CERTCertificateList *	certList;
    PRTime			signingTime;
    NSSCMSVerificationStatus	verificationStatus;
    SECKEYPrivateKey *          signingKey; 
    SECKEYPublicKey *           pubKey;
};
#define NSS_CMS_SIGNER_INFO_VERSION_ISSUERSN	1	/* what we *create* */
#define NSS_CMS_SIGNER_INFO_VERSION_SUBJKEY	3	/* what we *create* */

typedef enum {
    NSSCMSCM_None = 0,
    NSSCMSCM_CertOnly = 1,
    NSSCMSCM_CertChain = 2,
    NSSCMSCM_CertChainWithRoot = 3
} NSSCMSCertChainMode;




struct NSSCMSEnvelopedDataStr {
    SECItem			version;
    NSSCMSOriginatorInfo *	originatorInfo;		
    NSSCMSRecipientInfo **	recipientInfos;
    NSSCMSContentInfo		contentInfo;
    NSSCMSAttribute **		unprotectedAttr;
    
    NSSCMSMessage *		cmsg;			
};
#define NSS_CMS_ENVELOPED_DATA_VERSION_REG	0	/* what we *create* */
#define NSS_CMS_ENVELOPED_DATA_VERSION_ADV	2	/* what we *create* */

struct NSSCMSOriginatorInfoStr {
    SECItem **			rawCerts;
    CERTSignedCrl **		crls;
    
    CERTCertificate **		certs;
};




typedef enum {
    NSSCMSRecipientID_IssuerSN = 0,
    NSSCMSRecipientID_SubjectKeyID = 1,
    NSSCMSRecipientID_BrandNew = 2
} NSSCMSRecipientIDSelector;

struct NSSCMSRecipientIdentifierStr {
    NSSCMSRecipientIDSelector	identifierType;
    union {
	CERTIssuerAndSN		*issuerAndSN;
	SECItem 		*subjectKeyID;
    } id;
};
typedef struct NSSCMSRecipientIdentifierStr NSSCMSRecipientIdentifier;

struct NSSCMSKeyTransRecipientInfoStr {
    SECItem			version;
    NSSCMSRecipientIdentifier	recipientIdentifier;
    SECAlgorithmID		keyEncAlg;
    SECItem			encKey;
};
typedef struct NSSCMSKeyTransRecipientInfoStr NSSCMSKeyTransRecipientInfo;





struct NSSCMSKeyTransRecipientInfoExStr {
    NSSCMSKeyTransRecipientInfo recipientInfo;
    int version;  
    SECKEYPublicKey *pubKey;
};

typedef struct NSSCMSKeyTransRecipientInfoExStr NSSCMSKeyTransRecipientInfoEx;

#define NSS_CMS_KEYTRANS_RECIPIENT_INFO_VERSION_ISSUERSN	0	/* what we *create* */
#define NSS_CMS_KEYTRANS_RECIPIENT_INFO_VERSION_SUBJKEY		2	/* what we *create* */




struct NSSCMSOriginatorPublicKeyStr {
    SECAlgorithmID			algorithmIdentifier;
    SECItem				publicKey;			
};
typedef struct NSSCMSOriginatorPublicKeyStr NSSCMSOriginatorPublicKey;

typedef enum {
    NSSCMSOriginatorIDOrKey_IssuerSN = 0,
    NSSCMSOriginatorIDOrKey_SubjectKeyID = 1,
    NSSCMSOriginatorIDOrKey_OriginatorPublicKey = 2
} NSSCMSOriginatorIDOrKeySelector;

struct NSSCMSOriginatorIdentifierOrKeyStr {
    NSSCMSOriginatorIDOrKeySelector identifierType;
    union {
	CERTIssuerAndSN			*issuerAndSN;		
	SECItem				*subjectKeyID;		
	NSSCMSOriginatorPublicKey	originatorPublicKey;	
    } id;
};
typedef struct NSSCMSOriginatorIdentifierOrKeyStr NSSCMSOriginatorIdentifierOrKey;

struct NSSCMSRecipientKeyIdentifierStr {
    SECItem *				subjectKeyIdentifier;
    SECItem *				date;			
    SECItem *				other;			
};
typedef struct NSSCMSRecipientKeyIdentifierStr NSSCMSRecipientKeyIdentifier;

typedef enum {
    NSSCMSKeyAgreeRecipientID_IssuerSN = 0,
    NSSCMSKeyAgreeRecipientID_RKeyID = 1
} NSSCMSKeyAgreeRecipientIDSelector;

struct NSSCMSKeyAgreeRecipientIdentifierStr {
    NSSCMSKeyAgreeRecipientIDSelector	identifierType;
    union {
	CERTIssuerAndSN			*issuerAndSN;
	NSSCMSRecipientKeyIdentifier	recipientKeyIdentifier;
    } id;
};
typedef struct NSSCMSKeyAgreeRecipientIdentifierStr NSSCMSKeyAgreeRecipientIdentifier;

struct NSSCMSRecipientEncryptedKeyStr {
    NSSCMSKeyAgreeRecipientIdentifier	recipientIdentifier;
    SECItem				encKey;
};
typedef struct NSSCMSRecipientEncryptedKeyStr NSSCMSRecipientEncryptedKey;

struct NSSCMSKeyAgreeRecipientInfoStr {
    SECItem				version;
    NSSCMSOriginatorIdentifierOrKey	originatorIdentifierOrKey;
    SECItem *				ukm;				
    SECAlgorithmID			keyEncAlg;
    NSSCMSRecipientEncryptedKey **	recipientEncryptedKeys;
};
typedef struct NSSCMSKeyAgreeRecipientInfoStr NSSCMSKeyAgreeRecipientInfo;

#define NSS_CMS_KEYAGREE_RECIPIENT_INFO_VERSION	3	/* what we *create* */




struct NSSCMSKEKIdentifierStr {
    SECItem			keyIdentifier;
    SECItem *			date;			
    SECItem *			other;			
};
typedef struct NSSCMSKEKIdentifierStr NSSCMSKEKIdentifier;

struct NSSCMSKEKRecipientInfoStr {
    SECItem			version;
    NSSCMSKEKIdentifier		kekIdentifier;
    SECAlgorithmID		keyEncAlg;
    SECItem			encKey;
};
typedef struct NSSCMSKEKRecipientInfoStr NSSCMSKEKRecipientInfo;

#define NSS_CMS_KEK_RECIPIENT_INFO_VERSION	4	/* what we *create* */





typedef enum {
    NSSCMSRecipientInfoID_KeyTrans = 0,
    NSSCMSRecipientInfoID_KeyAgree = 1,
    NSSCMSRecipientInfoID_KEK = 2
} NSSCMSRecipientInfoIDSelector;























struct NSSCMSRecipientInfoStr {
    NSSCMSRecipientInfoIDSelector recipientInfoType;
    union {
	NSSCMSKeyTransRecipientInfo keyTransRecipientInfo;
	NSSCMSKeyAgreeRecipientInfo keyAgreeRecipientInfo;
	NSSCMSKEKRecipientInfo kekRecipientInfo;
	NSSCMSKeyTransRecipientInfoEx keyTransRecipientInfoEx;
    } ri;
    
    NSSCMSMessage *		cmsg;			
    CERTCertificate *		cert;			
};




struct NSSCMSDigestedDataStr {
    SECItem			version;
    SECAlgorithmID		digestAlg;
    NSSCMSContentInfo		contentInfo;
    SECItem			digest;
    
    NSSCMSMessage *		cmsg;		
    SECItem			cdigest;	
};
#define NSS_CMS_DIGESTED_DATA_VERSION_DATA	0	/* what we *create* */
#define NSS_CMS_DIGESTED_DATA_VERSION_ENCAP	2	/* what we *create* */




struct NSSCMSEncryptedDataStr {
    SECItem			version;
    NSSCMSContentInfo		contentInfo;
    NSSCMSAttribute **		unprotectedAttr;	
    
    NSSCMSMessage *		cmsg;		
};
#define NSS_CMS_ENCRYPTED_DATA_VERSION		0	/* what we *create* */
#define NSS_CMS_ENCRYPTED_DATA_VERSION_UPATTR	2	/* what we *create* */







typedef enum {
    NSSCMSKEAInvalid = -1,
    NSSCMSKEAUsesSkipjack = 0,
    NSSCMSKEAUsesNonSkipjack = 1,
    NSSCMSKEAUsesNonSkipjackWithPaddedEncKey = 2
} NSSCMSKEATemplateSelector;



struct NSSCMSSMIMEKEAParametersStr {
    SECItem originatorKEAKey;	
    SECItem originatorRA;	
    SECItem nonSkipjackIV;	



    SECItem bulkKeySize;	







};










struct NSSCMSAttributeStr {
    
    SECItem			type;
    SECItem **			values;	
    
    SECOidData *		typeTag;
    PRBool			encoded;	
};

#endif 
