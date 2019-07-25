









































#ifndef _PKCS7T_H_
#define _PKCS7T_H_

#include "plarena.h"

#include "seccomon.h"
#include "secoidt.h"
#include "certt.h"
#include "secmodt.h"


typedef struct SEC_PKCS7DecoderContextStr SEC_PKCS7DecoderContext;
typedef struct SEC_PKCS7EncoderContextStr SEC_PKCS7EncoderContext;


typedef void *(*SECKEYGetPasswordKey)(void *arg, void *handle);
































typedef struct SEC_PKCS7ContentInfoStr SEC_PKCS7ContentInfo;
typedef struct SEC_PKCS7SignedDataStr SEC_PKCS7SignedData;
typedef struct SEC_PKCS7EncryptedContentInfoStr SEC_PKCS7EncryptedContentInfo;
typedef struct SEC_PKCS7EnvelopedDataStr SEC_PKCS7EnvelopedData;
typedef struct SEC_PKCS7SignedAndEnvelopedDataStr
		SEC_PKCS7SignedAndEnvelopedData;
typedef struct SEC_PKCS7SignerInfoStr SEC_PKCS7SignerInfo;
typedef struct SEC_PKCS7RecipientInfoStr SEC_PKCS7RecipientInfo;
typedef struct SEC_PKCS7DigestedDataStr SEC_PKCS7DigestedData;
typedef struct SEC_PKCS7EncryptedDataStr SEC_PKCS7EncryptedData;






typedef struct SEC_PKCS7AttributeStr SEC_PKCS7Attribute;

struct SEC_PKCS7ContentInfoStr {
    PLArenaPool *poolp;			
    PRBool created;			
    int refCount;			
    SECOidData *contentTypeTag;		
    SECKEYGetPasswordKey pwfn;		
    void *pwfn_arg;			
    SECItem contentType;
    union {
	SECItem				*data;
	SEC_PKCS7DigestedData		*digestedData;
	SEC_PKCS7EncryptedData		*encryptedData;
	SEC_PKCS7EnvelopedData		*envelopedData;
	SEC_PKCS7SignedData		*signedData;
	SEC_PKCS7SignedAndEnvelopedData	*signedAndEnvelopedData;
    } content;
};

struct SEC_PKCS7SignedDataStr {
    SECItem version;
    SECAlgorithmID **digestAlgorithms;
    SEC_PKCS7ContentInfo contentInfo;
    SECItem **rawCerts;
    CERTSignedCrl **crls;
    SEC_PKCS7SignerInfo **signerInfos;
    SECItem **digests;			
    CERTCertificate **certs;		
    CERTCertificateList **certLists;	
};
#define SEC_PKCS7_SIGNED_DATA_VERSION		1	/* what we *create* */

struct SEC_PKCS7EncryptedContentInfoStr {
    SECOidData *contentTypeTag;		
    SECItem contentType;
    SECAlgorithmID contentEncAlg;
    SECItem encContent;
    SECItem plainContent;		
					
    int keysize;			
					

    SECOidTag encalg;			
					

};

struct SEC_PKCS7EnvelopedDataStr {
    SECItem version;
    SEC_PKCS7RecipientInfo **recipientInfos;
    SEC_PKCS7EncryptedContentInfo encContentInfo;
};
#define SEC_PKCS7_ENVELOPED_DATA_VERSION	0	/* what we *create* */

struct SEC_PKCS7SignedAndEnvelopedDataStr {
    SECItem version;
    SEC_PKCS7RecipientInfo **recipientInfos;
    SECAlgorithmID **digestAlgorithms;
    SEC_PKCS7EncryptedContentInfo encContentInfo;
    SECItem **rawCerts;
    CERTSignedCrl **crls;
    SEC_PKCS7SignerInfo **signerInfos;
    SECItem **digests;			
    CERTCertificate **certs;		
    CERTCertificateList **certLists;	
    PK11SymKey *sigKey;			
};
#define SEC_PKCS7_SIGNED_AND_ENVELOPED_DATA_VERSION 1	/* what we *create* */

struct SEC_PKCS7SignerInfoStr {
    SECItem version;
    CERTIssuerAndSN *issuerAndSN;
    SECAlgorithmID digestAlg;
    SEC_PKCS7Attribute **authAttr;
    SECAlgorithmID digestEncAlg;
    SECItem encDigest;
    SEC_PKCS7Attribute **unAuthAttr;
    CERTCertificate *cert;		
    CERTCertificateList *certList;	
};
#define SEC_PKCS7_SIGNER_INFO_VERSION		1	/* what we *create* */

struct SEC_PKCS7RecipientInfoStr {
    SECItem version;
    CERTIssuerAndSN *issuerAndSN;
    SECAlgorithmID keyEncAlg;
    SECItem encKey;
    CERTCertificate *cert;		
};
#define SEC_PKCS7_RECIPIENT_INFO_VERSION	0	/* what we *create* */

struct SEC_PKCS7DigestedDataStr {
    SECItem version;
    SECAlgorithmID digestAlg;
    SEC_PKCS7ContentInfo contentInfo;
    SECItem digest;
};
#define SEC_PKCS7_DIGESTED_DATA_VERSION		0	/* what we *create* */

struct SEC_PKCS7EncryptedDataStr {
    SECItem version;
    SEC_PKCS7EncryptedContentInfo encContentInfo;
};
#define SEC_PKCS7_ENCRYPTED_DATA_VERSION	0	/* what we *create* */




struct SEC_PKCS7AttributeStr {
    
    SECItem type;
    SECItem **values;	
    
    SECOidData *typeTag;
    PRBool encoded;	
};









typedef void (* SEC_PKCS7DecoderContentCallback)(void *arg,
						 const char *buf,
						 unsigned long len);








typedef void (* SEC_PKCS7EncoderOutputCallback)(void *arg,
						const char *buf,
						unsigned long len);








typedef PK11SymKey * (* SEC_PKCS7GetDecryptKeyCallback)(void *arg, 
							SECAlgorithmID *algid);









typedef PRBool (* SEC_PKCS7DecryptionAllowedCallback)(SECAlgorithmID *algid,  
						      PK11SymKey *bulkkey);

#endif 
