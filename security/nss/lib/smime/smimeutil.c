









































#include "secmime.h"
#include "secoid.h"
#include "pk11func.h"
#include "ciferfam.h"	
#include "secasn1.h"
#include "secitem.h"
#include "cert.h"
#include "key.h"
#include "secerr.h"
#include "cms.h"
#include "nss.h"

SEC_ASN1_MKSUB(CERT_IssuerAndSNTemplate)
SEC_ASN1_MKSUB(SEC_OctetStringTemplate)
SEC_ASN1_CHOOSER_DECLARE(CERT_IssuerAndSNTemplate)


static unsigned char asn1_int40[] = { SEC_ASN1_INTEGER, 0x01, 0x28 };
static unsigned char asn1_int64[] = { SEC_ASN1_INTEGER, 0x01, 0x40 };
static unsigned char asn1_int128[] = { SEC_ASN1_INTEGER, 0x02, 0x00, 0x80 };


static SECItem param_int40 = { siBuffer, asn1_int40, sizeof(asn1_int40) };
static SECItem param_int64 = { siBuffer, asn1_int64, sizeof(asn1_int64) };
static SECItem param_int128 = { siBuffer, asn1_int128, sizeof(asn1_int128) };






typedef struct {
    SECItem capabilityID;
    SECItem parameters;
    long cipher;		
} NSSSMIMECapability;

static const SEC_ASN1Template NSSSMIMECapabilityTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(NSSSMIMECapability) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(NSSSMIMECapability,capabilityID), },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_ANY,
	  offsetof(NSSSMIMECapability,parameters), },
    { 0, }
};

static const SEC_ASN1Template NSSSMIMECapabilitiesTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, NSSSMIMECapabilityTemplate }
};





typedef enum {
    NSSSMIMEEncryptionKeyPref_IssuerSN,
    NSSSMIMEEncryptionKeyPref_RKeyID,
    NSSSMIMEEncryptionKeyPref_SubjectKeyID
} NSSSMIMEEncryptionKeyPrefSelector;

typedef struct {
    NSSSMIMEEncryptionKeyPrefSelector selector;
    union {
	CERTIssuerAndSN			*issuerAndSN;
	NSSCMSRecipientKeyIdentifier	*recipientKeyID;
	SECItem				*subjectKeyID;
    } id;
} NSSSMIMEEncryptionKeyPreference;

extern const SEC_ASN1Template NSSCMSRecipientKeyIdentifierTemplate[];

static const SEC_ASN1Template smime_encryptionkeypref_template[] = {
    { SEC_ASN1_CHOICE,
	  offsetof(NSSSMIMEEncryptionKeyPreference,selector), NULL,
	  sizeof(NSSSMIMEEncryptionKeyPreference) },
    { SEC_ASN1_POINTER | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0
          | SEC_ASN1_CONSTRUCTED,
	  offsetof(NSSSMIMEEncryptionKeyPreference,id.issuerAndSN),
	  SEC_ASN1_SUB(CERT_IssuerAndSNTemplate),
	  NSSSMIMEEncryptionKeyPref_IssuerSN },
    { SEC_ASN1_POINTER | SEC_ASN1_CONTEXT_SPECIFIC | 1
          | SEC_ASN1_CONSTRUCTED,
	  offsetof(NSSSMIMEEncryptionKeyPreference,id.recipientKeyID),
	  NSSCMSRecipientKeyIdentifierTemplate,
	  NSSSMIMEEncryptionKeyPref_RKeyID },
    { SEC_ASN1_POINTER | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 2
          | SEC_ASN1_CONSTRUCTED,
	  offsetof(NSSSMIMEEncryptionKeyPreference,id.subjectKeyID),
	  SEC_ASN1_SUB(SEC_OctetStringTemplate),
	  NSSSMIMEEncryptionKeyPref_SubjectKeyID },
    { 0, }
};


typedef struct {
    unsigned long cipher;
    SECOidTag algtag;
    SECItem *parms;
    PRBool enabled;	
    PRBool allowed;	
} smime_cipher_map_entry;


static smime_cipher_map_entry smime_cipher_map[] = {


    { SMIME_RC2_CBC_40,		SEC_OID_RC2_CBC,	&param_int40,	PR_TRUE, PR_TRUE },
    { SMIME_DES_CBC_56,		SEC_OID_DES_CBC,	NULL,		PR_TRUE, PR_TRUE },
    { SMIME_RC2_CBC_64,		SEC_OID_RC2_CBC,	&param_int64,	PR_TRUE, PR_TRUE },
    { SMIME_RC2_CBC_128,	SEC_OID_RC2_CBC,	&param_int128,	PR_TRUE, PR_TRUE },
    { SMIME_DES_EDE3_168,	SEC_OID_DES_EDE3_CBC,	NULL,		PR_TRUE, PR_TRUE },
    { SMIME_AES_CBC_128,	SEC_OID_AES_128_CBC,	NULL,		PR_TRUE, PR_TRUE }
};
static const int smime_cipher_map_count = sizeof(smime_cipher_map) / sizeof(smime_cipher_map_entry);




static int
smime_mapi_by_cipher(unsigned long cipher)
{
    int i;

    for (i = 0; i < smime_cipher_map_count; i++) {
	if (smime_cipher_map[i].cipher == cipher)
	    return i;	
    }
    return -1;		
}




SECStatus 
NSS_SMIMEUtil_EnableCipher(unsigned long which, PRBool on)
{
    unsigned long mask;
    int mapi;

    mask = which & CIPHER_FAMILYID_MASK;

    PORT_Assert (mask == CIPHER_FAMILYID_SMIME);
    if (mask != CIPHER_FAMILYID_SMIME)
	
    	return SECFailure;

    mapi = smime_mapi_by_cipher(which);
    if (mapi < 0)
	
	return SECFailure;

    
    if (!smime_cipher_map[mapi].allowed && on) {
	PORT_SetError (SEC_ERROR_BAD_EXPORT_ALGORITHM);
	return SECFailure;
    }

    if (smime_cipher_map[mapi].enabled != on)
	smime_cipher_map[mapi].enabled = on;

    return SECSuccess;
}





SECStatus 
NSS_SMIMEUtil_AllowCipher(unsigned long which, PRBool on)
{
    unsigned long mask;
    int mapi;

    mask = which & CIPHER_FAMILYID_MASK;

    PORT_Assert (mask == CIPHER_FAMILYID_SMIME);
    if (mask != CIPHER_FAMILYID_SMIME)
	
    	return SECFailure;

    mapi = smime_mapi_by_cipher(which);
    if (mapi < 0)
	
	return SECFailure;

    if (smime_cipher_map[mapi].allowed != on)
	smime_cipher_map[mapi].allowed = on;

    return SECSuccess;
}







static SECStatus
nss_smime_get_cipher_for_alg_and_key(SECAlgorithmID *algid, PK11SymKey *key, unsigned long *cipher)
{
    SECOidTag algtag;
    unsigned int keylen_bits;
    unsigned long c;

    algtag = SECOID_GetAlgorithmTag(algid);
    switch (algtag) {
    case SEC_OID_RC2_CBC:
	keylen_bits = PK11_GetKeyStrength(key, algid);
	switch (keylen_bits) {
	case 40:
	    c = SMIME_RC2_CBC_40;
	    break;
	case 64:
	    c = SMIME_RC2_CBC_64;
	    break;
	case 128:
	    c = SMIME_RC2_CBC_128;
	    break;
	default:
	    return SECFailure;
	}
	break;
    case SEC_OID_DES_CBC:
	c = SMIME_DES_CBC_56;
	break;
    case SEC_OID_DES_EDE3_CBC:
	c = SMIME_DES_EDE3_168;
	break;
    case SEC_OID_AES_128_CBC:
	c = SMIME_AES_CBC_128;
	break;
    default:
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return SECFailure;
    }
    *cipher = c;
    return SECSuccess;
}

static PRBool
nss_smime_cipher_allowed(unsigned long which)
{
    int mapi;

    mapi = smime_mapi_by_cipher(which);
    if (mapi < 0)
	return PR_FALSE;
    return smime_cipher_map[mapi].allowed;
}

PRBool
NSS_SMIMEUtil_DecryptionAllowed(SECAlgorithmID *algid, PK11SymKey *key)
{
    unsigned long which;

    if (nss_smime_get_cipher_for_alg_and_key(algid, key, &which) != SECSuccess)
	return PR_FALSE;

    return nss_smime_cipher_allowed(which);
}




















PRBool
NSS_SMIMEUtil_EncryptionPossible(void)
{
    int i;

    for (i = 0; i < smime_cipher_map_count; i++) {
	if (smime_cipher_map[i].allowed)
	    return PR_TRUE;
    }
    return PR_FALSE;
}


static int
nss_SMIME_FindCipherForSMIMECap(NSSSMIMECapability *cap)
{
    int i;
    SECOidTag capIDTag;

    
    capIDTag = SECOID_FindOIDTag(&(cap->capabilityID));

    
    for (i = 0; i < smime_cipher_map_count; i++) {
	if (smime_cipher_map[i].algtag != capIDTag)
	    continue;
	




	if (!smime_cipher_map[i].parms) { 
	    if (!cap->parameters.data || !cap->parameters.len)
		break;	
	    if (cap->parameters.len     == 2  &&
	        cap->parameters.data[0] == SEC_ASN1_NULL &&
		cap->parameters.data[1] == 0) 
		break;  
	} else if (cap->parameters.data != NULL && 
	    cap->parameters.len == smime_cipher_map[i].parms->len &&
	    PORT_Memcmp (cap->parameters.data, smime_cipher_map[i].parms->data,
			     cap->parameters.len) == 0)
	{
	    break;	
	}
    }

    if (i == smime_cipher_map_count)
	return 0;				
    return smime_cipher_map[i].cipher;	
}







static long
smime_choose_cipher(CERTCertificate *scert, CERTCertificate **rcerts)
{
    PRArenaPool *poolp;
    long cipher;
    long chosen_cipher;
    int *cipher_abilities;
    int *cipher_votes;
    int weak_mapi;
    int strong_mapi;
    int rcount, mapi, max, i;

    chosen_cipher = SMIME_RC2_CBC_40;		
    weak_mapi = smime_mapi_by_cipher(chosen_cipher);

    poolp = PORT_NewArena (1024);		
    if (poolp == NULL)
	goto done;

    cipher_abilities = (int *)PORT_ArenaZAlloc(poolp, smime_cipher_map_count * sizeof(int));
    cipher_votes     = (int *)PORT_ArenaZAlloc(poolp, smime_cipher_map_count * sizeof(int));
    if (cipher_votes == NULL || cipher_abilities == NULL)
	goto done;

    
    strong_mapi = smime_mapi_by_cipher (SMIME_DES_EDE3_168);

    
    for (rcount = 0; rcerts[rcount] != NULL; rcount++) {
	SECItem *profile;
	NSSSMIMECapability **caps;
	int pref;

	


	pref = smime_cipher_map_count;

	
	profile = CERT_FindSMimeProfile(rcerts[rcount]);

	if (profile != NULL && profile->data != NULL && profile->len > 0) {
	    
	    caps = NULL;
	    
	    if (SEC_QuickDERDecodeItem(poolp, &caps,
                    NSSSMIMECapabilitiesTemplate, profile) == SECSuccess &&
		    caps != NULL)
	    {
		
		for (i = 0; caps[i] != NULL; i++) {
		    cipher = nss_SMIME_FindCipherForSMIMECap(caps[i]);
		    mapi = smime_mapi_by_cipher(cipher);
		    if (mapi >= 0) {
			
			cipher_abilities[mapi]++;
			cipher_votes[mapi] += pref;
			--pref;
		    }
		}
	    }
	} else {
	    

	    SECKEYPublicKey *key;
	    unsigned int pklen_bits;

	    










	    key = CERT_ExtractPublicKey(rcerts[rcount]);
	    pklen_bits = 0;
	    if (key != NULL) {
		pklen_bits = SECKEY_PublicKeyStrength (key) * 8;
		SECKEY_DestroyPublicKey (key);
	    }

	    if (pklen_bits > 512) {
		
		cipher_abilities[strong_mapi]++;
		cipher_votes[strong_mapi] += pref;
		pref--;
	    } 

	    
	    cipher_abilities[weak_mapi]++;
	    cipher_votes[weak_mapi] += pref;
	}
	if (profile != NULL)
	    SECITEM_FreeItem(profile, PR_TRUE);
    }

    
    max = 0;
    for (mapi = 0; mapi < smime_cipher_map_count; mapi++) {
	
	if (cipher_abilities[mapi] != rcount)
	    continue;
	
	if (!smime_cipher_map[mapi].enabled || !smime_cipher_map[mapi].allowed)
	    continue;
	
	if (cipher_votes[mapi] >= max) {
	    
	    
	    chosen_cipher = smime_cipher_map[mapi].cipher;
	    max = cipher_votes[mapi];
	}
    }
    

done:
    if (poolp != NULL)
	PORT_FreeArena (poolp, PR_FALSE);

    return chosen_cipher;
}






static int
smime_keysize_by_cipher (unsigned long which)
{
    int keysize;

    switch (which) {
      case SMIME_RC2_CBC_40:
	keysize = 40;
	break;
      case SMIME_RC2_CBC_64:
	keysize = 64;
	break;
      case SMIME_RC2_CBC_128:
      case SMIME_AES_CBC_128:
	keysize = 128;
	break;
      case SMIME_DES_CBC_56:
      case SMIME_DES_EDE3_168:
	



	keysize = 0;
	break;
      default:
	keysize = -1;
	break;
    }

    return keysize;
}







SECStatus
NSS_SMIMEUtil_FindBulkAlgForRecipients(CERTCertificate **rcerts, SECOidTag *bulkalgtag, int *keysize)
{
    unsigned long cipher;
    int mapi;

    cipher = smime_choose_cipher(NULL, rcerts);
    mapi = smime_mapi_by_cipher(cipher);

    *bulkalgtag = smime_cipher_map[mapi].algtag;
    *keysize = smime_keysize_by_cipher(smime_cipher_map[mapi].cipher);

    return SECSuccess;
}













SECStatus
NSS_SMIMEUtil_CreateSMIMECapabilities(PLArenaPool *poolp, SECItem *dest)
{
    NSSSMIMECapability *cap;
    NSSSMIMECapability **smime_capabilities;
    smime_cipher_map_entry *map;
    SECOidData *oiddata;
    SECItem *dummy;
    int i, capIndex;

    
    
    smime_capabilities = (NSSSMIMECapability **)PORT_ZAlloc((smime_cipher_map_count + 1)
				      * sizeof(NSSSMIMECapability *));
    if (smime_capabilities == NULL)
	return SECFailure;

    capIndex = 0;

    



    for (i = smime_cipher_map_count - 1; i >= 0; i--) {
	
	map = &(smime_cipher_map[i]);
	if (!map->enabled)
	    continue;

	
	cap = (NSSSMIMECapability *)PORT_ZAlloc(sizeof(NSSSMIMECapability));
	if (cap == NULL)
	    break;
	smime_capabilities[capIndex++] = cap;

	oiddata = SECOID_FindOIDByTag(map->algtag);
	if (oiddata == NULL)
	    break;

	cap->capabilityID.data = oiddata->oid.data;
	cap->capabilityID.len = oiddata->oid.len;
	cap->parameters.data = map->parms ? map->parms->data : NULL;
	cap->parameters.len = map->parms ? map->parms->len : 0;
	cap->cipher = smime_cipher_map[i].cipher;
    }

    
    

    smime_capabilities[capIndex] = NULL;	
    dummy = SEC_ASN1EncodeItem(poolp, dest, &smime_capabilities, NSSSMIMECapabilitiesTemplate);

    

    for (i = 0; smime_capabilities[i] != NULL; i++)
	PORT_Free(smime_capabilities[i]);
    PORT_Free(smime_capabilities);

    return (dummy == NULL) ? SECFailure : SECSuccess;
}









SECStatus
NSS_SMIMEUtil_CreateSMIMEEncKeyPrefs(PLArenaPool *poolp, SECItem *dest, CERTCertificate *cert)
{
    NSSSMIMEEncryptionKeyPreference ekp;
    SECItem *dummy = NULL;
    PLArenaPool *tmppoolp = NULL;

    if (cert == NULL)
	goto loser;

    tmppoolp = PORT_NewArena(1024);
    if (tmppoolp == NULL)
	goto loser;

    
    ekp.selector = NSSSMIMEEncryptionKeyPref_IssuerSN;
    ekp.id.issuerAndSN = CERT_GetCertIssuerAndSN(tmppoolp, cert);
    if (ekp.id.issuerAndSN == NULL)
	goto loser;

    dummy = SEC_ASN1EncodeItem(poolp, dest, &ekp, smime_encryptionkeypref_template);

loser:
    if (tmppoolp) PORT_FreeArena(tmppoolp, PR_FALSE);

    return (dummy == NULL) ? SECFailure : SECSuccess;
}









SECStatus
NSS_SMIMEUtil_CreateMSSMIMEEncKeyPrefs(PLArenaPool *poolp, SECItem *dest, CERTCertificate *cert)
{
    SECItem *dummy = NULL;
    PLArenaPool *tmppoolp = NULL;
    CERTIssuerAndSN *isn;

    if (cert == NULL)
	goto loser;

    tmppoolp = PORT_NewArena(1024);
    if (tmppoolp == NULL)
	goto loser;

    isn = CERT_GetCertIssuerAndSN(tmppoolp, cert);
    if (isn == NULL)
	goto loser;

    dummy = SEC_ASN1EncodeItem(poolp, dest, isn, SEC_ASN1_GET(CERT_IssuerAndSNTemplate));

loser:
    if (tmppoolp) PORT_FreeArena(tmppoolp, PR_FALSE);

    return (dummy == NULL) ? SECFailure : SECSuccess;
}











CERTCertificate *
NSS_SMIMEUtil_GetCertFromEncryptionKeyPreference(CERTCertDBHandle *certdb, SECItem *DERekp)
{
    PLArenaPool *tmppoolp = NULL;
    CERTCertificate *cert = NULL;
    NSSSMIMEEncryptionKeyPreference ekp;

    tmppoolp = PORT_NewArena(1024);
    if (tmppoolp == NULL)
	return NULL;

    
    if (SEC_QuickDERDecodeItem(tmppoolp, &ekp, smime_encryptionkeypref_template,
                               DERekp) != SECSuccess)
	goto loser;

    
    switch (ekp.selector) {
    case NSSSMIMEEncryptionKeyPref_IssuerSN:
	cert = CERT_FindCertByIssuerAndSN(certdb, ekp.id.issuerAndSN);
	break;
    case NSSSMIMEEncryptionKeyPref_RKeyID:
    case NSSSMIMEEncryptionKeyPref_SubjectKeyID:
	
	break;
    default:
	PORT_Assert(0);
    }
loser:
    if (tmppoolp) PORT_FreeArena(tmppoolp, PR_FALSE);

    return cert;
}

extern const char __nss_smime_rcsid[];
extern const char __nss_smime_sccsid[];

PRBool
NSSSMIME_VersionCheck(const char *importedVersion)
{
    








    volatile char c; 

    c = __nss_smime_rcsid[0] + __nss_smime_sccsid[0]; 

    return NSS_VersionCheck(importedVersion);
}

const char *
NSSSMIME_GetVersion(void)
{
    return NSS_VERSION;
}
