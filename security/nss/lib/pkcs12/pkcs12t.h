



































#ifndef _PKCS12T_H_
#define _PKCS12T_H_

#include "seccomon.h"
#include "secoid.h"
#include "cert.h"
#include "key.h"
#include "plarena.h"
#include "secpkcs7.h"
#include "secdig.h"	

typedef enum {
  SECPKCS12TargetTokenNoCAs,		

  SECPKCS12TargetTokenIntermediateCAs,  


  SECPKCS12TargetTokenAllCAs		
} SECPKCS12TargetTokenCAs;


typedef struct SEC_PKCS12PFXItemStr SEC_PKCS12PFXItem;
typedef struct SEC_PKCS12MacDataStr SEC_PKCS12MacData;
typedef struct SEC_PKCS12AuthenticatedSafeStr SEC_PKCS12AuthenticatedSafe;
typedef struct SEC_PKCS12BaggageItemStr SEC_PKCS12BaggageItem;
typedef struct SEC_PKCS12BaggageStr SEC_PKCS12Baggage;
typedef struct SEC_PKCS12Baggage_OLDStr SEC_PKCS12Baggage_OLD;
typedef struct SEC_PKCS12ESPVKItemStr SEC_PKCS12ESPVKItem;
typedef struct SEC_PKCS12PVKSupportingDataStr SEC_PKCS12PVKSupportingData;
typedef struct SEC_PKCS12PVKAdditionalDataStr SEC_PKCS12PVKAdditionalData;
typedef struct SEC_PKCS12SafeContentsStr SEC_PKCS12SafeContents;
typedef struct SEC_PKCS12SafeBagStr SEC_PKCS12SafeBag;
typedef struct SEC_PKCS12PrivateKeyStr SEC_PKCS12PrivateKey;
typedef struct SEC_PKCS12PrivateKeyBagStr SEC_PKCS12PrivateKeyBag;
typedef struct SEC_PKCS12CertAndCRLBagStr SEC_PKCS12CertAndCRLBag;
typedef struct SEC_PKCS12CertAndCRLStr SEC_PKCS12CertAndCRL;
typedef struct SEC_PKCS12X509CertCRLStr SEC_PKCS12X509CertCRL;
typedef struct SEC_PKCS12SDSICertStr SEC_PKCS12SDSICert;
typedef struct SEC_PKCS12SecretStr SEC_PKCS12Secret;
typedef struct SEC_PKCS12SecretAdditionalStr SEC_PKCS12SecretAdditional;
typedef struct SEC_PKCS12SecretItemStr SEC_PKCS12SecretItem;
typedef struct SEC_PKCS12SecretBagStr SEC_PKCS12SecretBag;

typedef SECItem *(* SEC_PKCS12PasswordFunc)(SECItem *args);




struct SEC_PKCS12BaggageStr
{
    PRArenaPool     *poolp;
    SEC_PKCS12BaggageItem **bags;

    int luggage_size;		
};




struct SEC_PKCS12PVKAdditionalDataStr
{
    PRArenaPool	*poolp;
    SECOidData	*pvkAdditionalTypeTag;	
    SECItem     pvkAdditionalType;
    SECItem     pvkAdditionalContent;
};




struct SEC_PKCS12PVKSupportingDataStr
{
    PRArenaPool		*poolp;
    SGNDigestInfo 	**assocCerts;
    SECItem		regenerable;
    SECItem         	nickname;
    SEC_PKCS12PVKAdditionalData     pvkAdditional;
    SECItem		pvkAdditionalDER;

    SECItem		uniNickName;
    
    int			nThumbs;
};




struct SEC_PKCS12ESPVKItemStr
{
    PRArenaPool *poolp;		
    SECOidData	*espvkTag;	
    SECItem	espvkOID;
    SEC_PKCS12PVKSupportingData espvkData;
    union
    {
	SECKEYEncryptedPrivateKeyInfo *pkcs8KeyShroud;
    } espvkCipherText;

    PRBool duplicate;	
    PRBool problem_cert; 	
    PRBool single_cert;		
    int nCerts;			
    SECItem derCert;		
};




struct SEC_PKCS12SafeBagStr
{
    PRArenaPool *poolp;
    SECOidData	*safeBagTypeTag;	
    SECItem     safeBagType;
    union
    {
	SEC_PKCS12PrivateKeyBag	*keyBag;
	SEC_PKCS12CertAndCRLBag *certAndCRLBag;
	SEC_PKCS12SecretBag     *secretBag;
    } safeContent;

    SECItem	derSafeContent;
    SECItem 	safeBagName;

    SECItem	uniSafeBagName;
};




struct SEC_PKCS12SafeContentsStr
{
    PRArenaPool     	*poolp;
    SEC_PKCS12SafeBag	**contents;

    
    int safe_size;
    PRBool old;
    PRBool swapUnicode;
    PRBool possibleSwapUnicode;
};




struct SEC_PKCS12PrivateKeyStr
{
    PRArenaPool *poolp;
    SEC_PKCS12PVKSupportingData pvkData;
    SECKEYPrivateKeyInfo	pkcs8data;   

    PRBool duplicate;	
    PRBool problem_cert;
    PRBool single_cert;	
    int nCerts;		
    SECItem derCert;	
};




struct SEC_PKCS12PrivateKeyBagStr
{
    PRArenaPool     *poolp;
    SEC_PKCS12PrivateKey 	**privateKeys;

    int bag_size;	
};




struct SEC_PKCS12CertAndCRLStr
{
    PRArenaPool     *poolp;
    SECOidData	    *BagTypeTag;    
    SECItem         BagID;
    union
    {
    	SEC_PKCS12X509CertCRL	*x509;
    	SEC_PKCS12SDSICert	*sdsi;
    } value;

    SECItem derValue;
    SECItem nickname;		
    PRBool duplicate;		
};





struct SEC_PKCS12X509CertCRLStr
{
    PRArenaPool     		*poolp;
    SEC_PKCS7ContentInfo	certOrCRL;
    SGNDigestInfo		thumbprint;

    SECItem *derLeafCert;	
};





struct SEC_PKCS12SDSICertStr
{
    PRArenaPool     *poolp;
    SECItem         value;
    SGNDigestInfo   thumbprint;
};


struct SEC_PKCS12CertAndCRLBagStr
{
    PRArenaPool     		*poolp;
    SEC_PKCS12CertAndCRL	**certAndCRLs;

    int bag_size;	
};




struct SEC_PKCS12SecretAdditionalStr
{
    PRArenaPool     *poolp;
    SECOidData	    *secretTypeTag;         
    SECItem         secretAdditionalType;
    SECItem         secretAdditionalContent;
};




struct SEC_PKCS12SecretStr
{
    PRArenaPool     *poolp;
    SECItem	secretName;
    SECItem	value;
    SEC_PKCS12SecretAdditional	secretAdditional;

    SECItem	uniSecretName;
};

struct SEC_PKCS12SecretItemStr
{
    PRArenaPool     *poolp;
    SEC_PKCS12Secret	secret;
    SEC_PKCS12SafeBag	subFolder;
};    



struct SEC_PKCS12SecretBagStr
{
    PRArenaPool     	*poolp;
    SEC_PKCS12SecretItem	**secrets;

    int bag_size;	
};

struct SEC_PKCS12MacDataStr
{
    SGNDigestInfo	safeMac;
    SECItem		macSalt;
};


struct SEC_PKCS12PFXItemStr
{
    PRArenaPool		*poolp;
    SEC_PKCS12MacData	macData;
    SEC_PKCS7ContentInfo	authSafe; 

    
    PRBool		old;
    SGNDigestInfo 	old_safeMac;
    SECItem		old_macSalt;

    
    PRBool		swapUnicode;
};

struct SEC_PKCS12BaggageItemStr {
    PRArenaPool	    *poolp;
    SEC_PKCS12ESPVKItem	**espvks;
    SEC_PKCS12SafeBag	**unencSecrets;

    int nEspvks;
    int nSecrets; 
};
    

struct SEC_PKCS12Baggage_OLDStr
{
    PRArenaPool     *poolp;
    SEC_PKCS12ESPVKItem **espvks;

    int luggage_size;		
};


struct SEC_PKCS12AuthenticatedSafeStr
{
    PRArenaPool     *poolp;
    SECItem         version;
    SECOidData	    *transportTypeTag;	
    SECItem         transportMode;
    SECItem         privacySalt;
    SEC_PKCS12Baggage	  baggage;
    SEC_PKCS7ContentInfo  *safe;

    
    PRBool old;
    PRBool emptySafe;
    SEC_PKCS12Baggage_OLD old_baggage;
    SEC_PKCS7ContentInfo old_safe;
    PRBool swapUnicode;
};
#define SEC_PKCS12_PFX_VERSION		1		/* what we create */




extern const SEC_ASN1Template SEC_PKCS12PFXItemTemplate_OLD[];
extern const SEC_ASN1Template SEC_PKCS12AuthenticatedSafeTemplate_OLD[];
extern const SEC_ASN1Template SEC_PKCS12BaggageTemplate_OLD[];
extern const SEC_ASN1Template SEC_PKCS12PFXItemTemplate[];
extern const SEC_ASN1Template SEC_PKCS12MacDataTemplate[];
extern const SEC_ASN1Template SEC_PKCS12AuthenticatedSafeTemplate[];
extern const SEC_ASN1Template SEC_PKCS12BaggageTemplate[];
extern const SEC_ASN1Template SEC_PKCS12ESPVKItemTemplate[];
extern const SEC_ASN1Template SEC_PKCS12PVKSupportingDataTemplate[];
extern const SEC_ASN1Template SEC_PKCS12PVKAdditionalTemplate[];
extern const SEC_ASN1Template SEC_PKCS12SafeContentsTemplate_OLD[];
extern const SEC_ASN1Template SEC_PKCS12SafeContentsTemplate[];
extern const SEC_ASN1Template SEC_PKCS12SafeBagTemplate[];
extern const SEC_ASN1Template SEC_PKCS12PrivateKeyTemplate[];
extern const SEC_ASN1Template SEC_PKCS12PrivateKeyBagTemplate[];
extern const SEC_ASN1Template SEC_PKCS12CertAndCRLTemplate[];
extern const SEC_ASN1Template SEC_PKCS12CertAndCRLBagTemplate[];
extern const SEC_ASN1Template SEC_PKCS12X509CertCRLTemplate_OLD[];
extern const SEC_ASN1Template SEC_PKCS12X509CertCRLTemplate[];
extern const SEC_ASN1Template SEC_PKCS12SDSICertTemplate[];
extern const SEC_ASN1Template SEC_PKCS12SecretBagTemplate[];
extern const SEC_ASN1Template SEC_PKCS12SecretTemplate[];
extern const SEC_ASN1Template SEC_PKCS12SecretItemTemplate[];
extern const SEC_ASN1Template SEC_PKCS12SecretAdditionalTemplate[];
extern const SEC_ASN1Template SGN_DigestInfoTemplate[];
extern const SEC_ASN1Template SEC_PointerToPKCS12KeyBagTemplate[];
extern const SEC_ASN1Template SEC_PointerToPKCS12CertAndCRLBagTemplate[];
extern const SEC_ASN1Template SEC_PointerToPKCS12CertAndCRLBagTemplate_OLD[];
extern const SEC_ASN1Template SEC_PointerToPKCS12SecretBagTemplate[];
extern const SEC_ASN1Template SEC_PointerToPKCS12X509CertCRLTemplate_OLD[];
extern const SEC_ASN1Template SEC_PointerToPKCS12X509CertCRLTemplate[];
extern const SEC_ASN1Template SEC_PointerToPKCS12SDSICertTemplate[];
extern const SEC_ASN1Template SEC_PKCS12CodedSafeBagTemplate[];
extern const SEC_ASN1Template SEC_PKCS12CodedCertBagTemplate[];
extern const SEC_ASN1Template SEC_PKCS12CodedCertAndCRLBagTemplate[];
extern const SEC_ASN1Template SEC_PKCS12PVKSupportingDataTemplate_OLD[];
extern const SEC_ASN1Template SEC_PKCS12ESPVKItemTemplate_OLD[];
#endif
