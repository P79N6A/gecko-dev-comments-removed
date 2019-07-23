



































#include "plarena.h"

#include "seccomon.h"
#include "secitem.h"
#include "secport.h"
#include "hasht.h"
#include "pkcs11t.h"
#include "sechash.h"
#include "secasn1.h"
#include "secder.h"
#include "secoid.h"
#include "secerr.h"
#include "secmod.h"
#include "pk11func.h"
#include "secpkcs5.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pkcs11.h"
#include "pk11func.h"
#include "secitem.h"
#include "key.h"

typedef struct SEC_PKCS5PBEParameterStr SEC_PKCS5PBEParameter;
struct SEC_PKCS5PBEParameterStr {
    PRArenaPool     *poolp;
    SECItem         salt;           
    SECItem         iteration;      
    SECItem         keyLength;	
    SECAlgorithmID  *pPrfAlgId;	
    SECAlgorithmID  prfAlgId;	
};





struct sec_pkcs5V2ParameterStr {
    PRArenaPool    *poolp;
    SECAlgorithmID pbeAlgId;   
    SECAlgorithmID cipherAlgId; 
};

typedef struct sec_pkcs5V2ParameterStr sec_pkcs5V2Parameter;






const SEC_ASN1Template SEC_PKCS5PBEParameterTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 
	0, NULL, sizeof(SEC_PKCS5PBEParameter) },
    { SEC_ASN1_OCTET_STRING, 
	offsetof(SEC_PKCS5PBEParameter, salt) },
    { SEC_ASN1_INTEGER,
	offsetof(SEC_PKCS5PBEParameter, iteration) },
    { 0 }
};

const SEC_ASN1Template SEC_V2PKCS12PBEParameterTemplate[] =
{   
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS5PBEParameter) },
    { SEC_ASN1_OCTET_STRING, offsetof(SEC_PKCS5PBEParameter, salt) },
    { SEC_ASN1_INTEGER, offsetof(SEC_PKCS5PBEParameter, iteration) },
    { 0 }
};

SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)


const SEC_ASN1Template SEC_PKCS5V2PBEParameterTemplate[] =
{   
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS5PBEParameter) },
    

    { SEC_ASN1_OCTET_STRING, offsetof(SEC_PKCS5PBEParameter, salt) },
    { SEC_ASN1_INTEGER, offsetof(SEC_PKCS5PBEParameter, iteration) },
    { SEC_ASN1_INTEGER|SEC_ASN1_OPTIONAL, 
		      offsetof(SEC_PKCS5PBEParameter, keyLength) },
    { SEC_ASN1_POINTER | SEC_ASN1_XTRN | SEC_ASN1_OPTIONAL, 
	offsetof(SEC_PKCS5PBEParameter, pPrfAlgId),
	SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { 0 }
};


const SEC_ASN1Template SEC_PKCS5V2ParameterTemplate[] =
{   
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SEC_PKCS5PBEParameter) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN, offsetof(sec_pkcs5V2Parameter, pbeAlgId),
	SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	offsetof(sec_pkcs5V2Parameter, cipherAlgId),
	SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { 0 }
};






SECOidTag
sec_pkcs5GetCryptoFromAlgTag(SECOidTag algorithm)
{
    switch(algorithm)
    {
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_2KEY_TRIPLE_DES_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC:
	    return SEC_OID_DES_EDE3_CBC;
	case SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC:
	    return SEC_OID_DES_CBC;
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
	    return SEC_OID_RC2_CBC;
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC4:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC4:
	    return SEC_OID_RC4;
	case SEC_OID_PKCS5_PBKDF2:
	case SEC_OID_PKCS5_PBES2:
	case SEC_OID_PKCS5_PBMAC1:
	    return SEC_OID_PKCS5_PBKDF2;
	default:
	    break;
    }

    return SEC_OID_UNKNOWN;
}





sec_pkcs5V2Parameter *
sec_pkcs5_v2_get_v2_param(PRArenaPool *arena, SECAlgorithmID *algid)
{
    PRArenaPool *localArena = NULL;
    sec_pkcs5V2Parameter *pbeV2_param;
    SECStatus rv;

    if (arena == NULL) {
	localArena = arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
	if (arena == NULL) {
	    return NULL;
	}
    }
    pbeV2_param = PORT_ArenaZNew(arena, sec_pkcs5V2Parameter);
    if (pbeV2_param == NULL) {
	goto loser;
    }
	
    rv = SEC_ASN1DecodeItem(arena, pbeV2_param,
		SEC_PKCS5V2ParameterTemplate, &algid->parameters);
    if (rv == SECFailure) {
	goto loser;
    }

    pbeV2_param->poolp = arena;
    return pbeV2_param;
loser:
    if (localArena) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return NULL;
}

void
sec_pkcs5_v2_destroy_v2_param(sec_pkcs5V2Parameter *param)
{
   if (param && param->poolp) {
	PORT_FreeArena(param->poolp, PR_TRUE);
   }
}
	



SECOidTag 
SEC_PKCS5GetCryptoAlgorithm(SECAlgorithmID *algid)
{

    SECOidTag pbeAlg;
    SECOidTag cipherAlg;

    if(algid == NULL)
	return SEC_OID_UNKNOWN;

    pbeAlg = SECOID_GetAlgorithmTag(algid);
    cipherAlg = sec_pkcs5GetCryptoFromAlgTag(pbeAlg);
    if ((cipherAlg == SEC_OID_PKCS5_PBKDF2)  &&
	 (pbeAlg != SEC_OID_PKCS5_PBKDF2)) {
	sec_pkcs5V2Parameter *pbeV2_param;
	cipherAlg = SEC_OID_UNKNOWN;

	pbeV2_param = sec_pkcs5_v2_get_v2_param(NULL, algid);
	if (pbeV2_param != NULL) {
	    cipherAlg = SECOID_GetAlgorithmTag(&pbeV2_param->cipherAlgId);
	    sec_pkcs5_v2_destroy_v2_param(pbeV2_param);
	}
    }

    return cipherAlg;
}


 
PRBool 
SEC_PKCS5IsAlgorithmPBEAlg(SECAlgorithmID *algid)
{
    return (PRBool)(SEC_PKCS5GetCryptoAlgorithm(algid) != SEC_OID_UNKNOWN);
}

PRBool 
SEC_PKCS5IsAlgorithmPBEAlgTag(SECOidTag algtag)
{
    return (PRBool)(sec_pkcs5GetCryptoFromAlgTag(algtag) != SEC_OID_UNKNOWN);
}





static SECOidTag
sec_pkcs5v2_get_pbe(SECOidTag algTag)
{
    
    if (HASH_GetHashOidTagByHMACOidTag(algTag) != SEC_OID_UNKNOWN) {
	
	return SEC_OID_PKCS5_PBMAC1;
    }
    if (HASH_GetHashTypeByOidTag(algTag) != HASH_AlgNULL) {
	
	return SEC_OID_UNKNOWN;
    }
    if (PK11_AlgtagToMechanism(algTag) != CKM_INVALID_MECHANISM) {
	


	return SEC_OID_PKCS5_PBES2;
    }
    return SEC_OID_UNKNOWN;
}





SECOidTag 
SEC_PKCS5GetPBEAlgorithm(SECOidTag algTag, int keyLen)
{
    switch(algTag)
    {
	case SEC_OID_DES_EDE3_CBC:
	    switch(keyLen) {
		case 168:
		case 192:
		case 0:
		    return SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC;
		case 128:
		case 92:
		    return SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_2KEY_TRIPLE_DES_CBC;
		default:
		    break;
	    }
	    break;
	case SEC_OID_DES_CBC:
	    return SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC;
	case SEC_OID_RC2_CBC:
	    switch(keyLen) {
		case 40:
		    return SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC;
		case 128:
		case 0:
		    return SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC;
		default:
		    break;
	    }
	    break;
	case SEC_OID_RC4:
	    switch(keyLen) {
		case 40:
		    return SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC4;
		case 128:
		case 0:
		    return SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC4;
		default:
		    break;
	    }
	    break;
	default:
	    return sec_pkcs5v2_get_pbe(algTag);
    }

    return SEC_OID_UNKNOWN;
}




int
sec_pkcs5v2_key_length(SECAlgorithmID *algid)
{
    SECOidTag algorithm;
    PRArenaPool *arena = NULL;
    SEC_PKCS5PBEParameter p5_param;
    SECStatus rv;
    int length = -1;

    algorithm = SECOID_GetAlgorithmTag(algid);
    
    if (algorithm != SEC_OID_PKCS5_PBKDF2) {
	return -1;
    }

    arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if (arena == NULL) {
	goto loser;
    }
    PORT_Memset(&p5_param, 0, sizeof(p5_param));
    rv = SEC_ASN1DecodeItem(arena,&p5_param,
			 SEC_PKCS5V2PBEParameterTemplate, &algid->parameters);
    if (rv != SECSuccess) {
	goto loser;
    }

    if (p5_param.keyLength.data != NULL) {
        length = DER_GetInteger(&p5_param.keyLength);
    }

loser:
    if (arena) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return length;
}




int 
SEC_PKCS5GetKeyLength(SECAlgorithmID *algid)
{

    SECOidTag algorithm;

    if(algid == NULL)
	return SEC_OID_UNKNOWN;

    algorithm = SECOID_GetAlgorithmTag(algid);

    switch(algorithm)
    {
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_TRIPLE_DES_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_2KEY_TRIPLE_DES_CBC:
	    return 24;
	case SEC_OID_PKCS5_PBE_WITH_MD2_AND_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_SHA1_AND_DES_CBC:
	case SEC_OID_PKCS5_PBE_WITH_MD5_AND_DES_CBC:
	    return 8;
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC4:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
	    return 5;
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
	case SEC_OID_PKCS12_PBE_WITH_SHA1_AND_128_BIT_RC4:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC4:
	    return 16;
	case SEC_OID_PKCS5_PBKDF2:
	    return sec_pkcs5v2_key_length(algid);
	case SEC_OID_PKCS5_PBES2:
	case SEC_OID_PKCS5_PBMAC1:
	    {
		sec_pkcs5V2Parameter *pbeV2_param;
		int length = -1;
		pbeV2_param = sec_pkcs5_v2_get_v2_param(NULL, algid);
		if (pbeV2_param != NULL) {
	    	    length = sec_pkcs5v2_key_length(&pbeV2_param->pbeAlgId);
		    sec_pkcs5_v2_destroy_v2_param(pbeV2_param);
		}
		return length;
	    }
	
	default:
	    break;
    }
    return -1;
}





static PRBool
sec_pkcs5_is_algorithm_v2_pkcs12_algorithm(SECOidTag algorithm)
{
    switch(algorithm) 
    {
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC4:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC4:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_2KEY_TRIPLE_DES_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_128_BIT_RC2_CBC:
	case SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_40_BIT_RC2_CBC:
	    return PR_TRUE;
	default:
	    break;
    }

    return PR_FALSE;
}

static PRBool
sec_pkcs5_is_algorithm_v2_pkcs5_algorithm(SECOidTag algorithm)
{
    switch(algorithm) 
    {
	case SEC_OID_PKCS5_PBES2:
	case SEC_OID_PKCS5_PBMAC1:
	case SEC_OID_PKCS5_PBKDF2:
	    return PR_TRUE;
	default:
	    break;
    }

    return PR_FALSE;
}





static void 
sec_pkcs5_destroy_pbe_param(SEC_PKCS5PBEParameter *pbe_param)
{
    if(pbe_param != NULL)
	PORT_FreeArena(pbe_param->poolp, PR_TRUE);
}













#define DEFAULT_SALT_LENGTH 16
static SEC_PKCS5PBEParameter *
sec_pkcs5_create_pbe_parameter(SECOidTag algorithm, 
			SECItem *salt, 
			int iteration,
			int keyLength,
			SECOidTag prfAlg)
{
    PRArenaPool *poolp = NULL;
    SEC_PKCS5PBEParameter *pbe_param = NULL;
    SECStatus rv= SECSuccess; 
    void *dummy = NULL;

    if(iteration < 0) {
	return NULL;
    }

    poolp = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(poolp == NULL)
	return NULL;

    pbe_param = (SEC_PKCS5PBEParameter *)PORT_ArenaZAlloc(poolp,
	sizeof(SEC_PKCS5PBEParameter));
    if(!pbe_param) {
	PORT_FreeArena(poolp, PR_TRUE);
	return NULL;
    }

    pbe_param->poolp = poolp;

    rv = SECFailure;
    if (salt && salt->data) {
    	rv = SECITEM_CopyItem(poolp, &pbe_param->salt, salt);
    } else {
	

	pbe_param->salt.len = DEFAULT_SALT_LENGTH;
	pbe_param->salt.data = PORT_ArenaZAlloc(poolp,DEFAULT_SALT_LENGTH);
	if (pbe_param->salt.data) {
	   rv = PK11_GenerateRandom(pbe_param->salt.data,DEFAULT_SALT_LENGTH);
	}
    }

    if(rv != SECSuccess) {
	PORT_FreeArena(poolp, PR_TRUE);
	return NULL;
    }

    
    dummy = SEC_ASN1EncodeInteger(poolp, &pbe_param->iteration, 
		iteration);
    rv = (dummy) ? SECSuccess : SECFailure;

    if(rv != SECSuccess) {
	PORT_FreeArena(poolp, PR_FALSE);
	return NULL;
    }

    


    if (algorithm == SEC_OID_PKCS5_PBKDF2) {
	dummy = SEC_ASN1EncodeInteger(poolp, &pbe_param->keyLength, 
		keyLength);
	rv = (dummy) ? SECSuccess : SECFailure;
	if (rv != SECSuccess) {
	    PORT_FreeArena(poolp, PR_FALSE);
	    return NULL;
	}
	rv = SECOID_SetAlgorithmID(poolp, &pbe_param->prfAlgId, prfAlg, NULL);
	if (rv != SECSuccess) {
	    PORT_FreeArena(poolp, PR_FALSE);
	    return NULL;
	}
	pbe_param->pPrfAlgId = &pbe_param->prfAlgId;
    }

    return pbe_param;
}








SECAlgorithmID *
sec_pkcs5CreateAlgorithmID(SECOidTag algorithm, 
			   SECOidTag cipherAlgorithm,
			   SECOidTag prfAlg,
			   SECOidTag *pPbeAlgorithm,
			   int keyLength,
			   SECItem *salt, 
			   int iteration)
{
    PRArenaPool *poolp = NULL;
    SECAlgorithmID *algid, *ret_algid = NULL;
    SECOidTag pbeAlgorithm = algorithm;
    SECItem der_param;
    void *dummy;
    SECStatus rv = SECFailure;
    SEC_PKCS5PBEParameter *pbe_param = NULL;
    sec_pkcs5V2Parameter pbeV2_param;

    if(iteration <= 0) {
	return NULL;
    }

    poolp = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if(!poolp) {
	goto loser;
    }

    if (!SEC_PKCS5IsAlgorithmPBEAlgTag(algorithm) ||
    	 	sec_pkcs5_is_algorithm_v2_pkcs5_algorithm(algorithm)) {
	
	SECItem *cipherParams;

	





	if (sec_pkcs5_is_algorithm_v2_pkcs5_algorithm(algorithm)) {
	    if (cipherAlgorithm == SEC_OID_UNKNOWN) {
		goto loser;
	    }
	} else {
	    cipherAlgorithm = algorithm;
	    
	    algorithm = SEC_OID_PKCS5_PBKDF2;
	}
	
	pbeAlgorithm = SEC_OID_PKCS5_PBKDF2;
	














	if (algorithm == SEC_OID_PKCS5_PBKDF2) {
	     
	     algorithm = sec_pkcs5v2_get_pbe(cipherAlgorithm);
	}

	
	if (keyLength == 0) {
	    SECOidTag hashAlg = HASH_GetHashOidTagByHMACOidTag(cipherAlgorithm);
    	    if (hashAlg != SEC_OID_UNKNOWN) {
	        keyLength = HASH_ResultLenByOidTag(hashAlg);
	    } else {
		CK_MECHANISM_TYPE cryptoMech;
		cryptoMech = PK11_AlgtagToMechanism(cipherAlgorithm);
		if (cryptoMech == CKM_INVALID_MECHANISM) {
		    goto loser;
		}
	        keyLength = PK11_GetMaxKeyLength(cryptoMech);
	    }
	    if (keyLength == 0) {
		goto loser;
	    }
	}
	
	if (prfAlg == SEC_OID_UNKNOWN) {
	    prfAlg = SEC_OID_HMAC_SHA1;
	}

	
	cipherParams = pk11_GenerateNewParamWithKeyLen(	
			PK11_AlgtagToMechanism(cipherAlgorithm), keyLength);
	if (!cipherParams) {
	    goto loser;
	}

	PORT_Memset(&pbeV2_param, 0, sizeof (pbeV2_param));

	rv = PK11_ParamToAlgid(cipherAlgorithm, cipherParams, 
				poolp, &pbeV2_param.cipherAlgId);
	SECITEM_FreeItem(cipherParams, PR_TRUE);
	if (rv != SECSuccess) {
	    goto loser;
	}
    }
	

    
    pbe_param = sec_pkcs5_create_pbe_parameter(pbeAlgorithm, salt, iteration,
			keyLength, prfAlg);
    if(!pbe_param) {
	goto loser;
    }

    
    algid = (SECAlgorithmID *)PORT_ArenaZAlloc(poolp, sizeof(SECAlgorithmID));
    if(algid == NULL) {
	goto loser;
    }

    der_param.data = NULL;
    der_param.len = 0;
    if (sec_pkcs5_is_algorithm_v2_pkcs5_algorithm(algorithm)) {
	
	dummy = SEC_ASN1EncodeItem(poolp, &der_param, pbe_param,
					SEC_PKCS5V2PBEParameterTemplate);
	if (dummy == NULL) {
	    goto loser;
	}
	rv = SECOID_SetAlgorithmID(poolp, &pbeV2_param.pbeAlgId, 
				   pbeAlgorithm, &der_param);
	if (rv != SECSuccess) {
	    goto loser;
	}

	
	der_param.data = NULL;
	der_param.len = 0;
	dummy = SEC_ASN1EncodeItem(poolp, &der_param, &pbeV2_param,
					SEC_PKCS5V2ParameterTemplate);
    } else if(!sec_pkcs5_is_algorithm_v2_pkcs12_algorithm(algorithm)) {
	dummy = SEC_ASN1EncodeItem(poolp, &der_param, pbe_param,
					SEC_PKCS5PBEParameterTemplate);
    } else {
	dummy = SEC_ASN1EncodeItem(poolp, &der_param, pbe_param,
	    				SEC_V2PKCS12PBEParameterTemplate);
    }
    if (dummy == NULL) {
	goto loser;
    }

    rv = SECOID_SetAlgorithmID(poolp, algid, algorithm, &der_param);
    if (rv != SECSuccess) {
	goto loser;
    }

    ret_algid = (SECAlgorithmID *)PORT_ZAlloc(sizeof(SECAlgorithmID));
    if (ret_algid == NULL) {
	goto loser;
    }

    rv = SECOID_CopyAlgorithmID(NULL, ret_algid, algid);
    if (rv != SECSuccess) {
	SECOID_DestroyAlgorithmID(ret_algid, PR_TRUE);
	ret_algid = NULL;
    } else if (pPbeAlgorithm) {
	*pPbeAlgorithm = pbeAlgorithm;
    }

loser:
    if (poolp != NULL) {
	PORT_FreeArena(poolp, PR_TRUE);
	algid = NULL;
    }

    if (pbe_param) {
	sec_pkcs5_destroy_pbe_param(pbe_param);
    }

    return ret_algid;
}

SECStatus
pbe_PK11AlgidToParam(SECAlgorithmID *algid,SECItem *mech)
{
    SEC_PKCS5PBEParameter p5_param;
    SECItem *salt = NULL;
    SECOidTag algorithm = SECOID_GetAlgorithmTag(algid);
    PRArenaPool *arena = NULL;
    SECStatus rv = SECFailure;
    unsigned char *paramData = NULL;
    unsigned char *pSalt = NULL;
    CK_ULONG iterations;
    int paramLen = 0;
    int iv_len;
    

    arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if (arena == NULL) {
	goto loser;
    }


    


    PORT_Memset(&p5_param, 0, sizeof(p5_param));
    if (sec_pkcs5_is_algorithm_v2_pkcs12_algorithm(algorithm)) {
        iv_len = PK11_GetIVLength(PK11_AlgtagToMechanism(algorithm));
        rv = SEC_ASN1DecodeItem(arena, &p5_param,
			 SEC_V2PKCS12PBEParameterTemplate, &algid->parameters);
    } else if (algorithm == SEC_OID_PKCS5_PBKDF2) {
	iv_len = 0;
        rv = SEC_ASN1DecodeItem(arena,&p5_param,
			 SEC_PKCS5V2PBEParameterTemplate, &algid->parameters);
    } else {
        iv_len = PK11_GetIVLength(PK11_AlgtagToMechanism(algorithm));
        rv = SEC_ASN1DecodeItem(arena,&p5_param,SEC_PKCS5PBEParameterTemplate, 
						&algid->parameters);
    }

    if (iv_len < 0) {
	goto loser;
    }

    if (rv != SECSuccess) {
	goto loser;
    }
        
    
    salt = &p5_param.salt;
    iterations = (CK_ULONG) DER_GetInteger(&p5_param.iteration);

    

    if (algorithm == SEC_OID_PKCS5_PBKDF2) {
	SECOidTag prfAlgTag;
    	CK_PKCS5_PBKD2_PARAMS *pbeV2_params = 
		(CK_PKCS5_PBKD2_PARAMS *)PORT_ZAlloc(
			sizeof(CK_PKCS5_PBKD2_PARAMS)+ salt->len);

	if (pbeV2_params == NULL) {
	    goto loser;
	}
	paramData = (unsigned char *)pbeV2_params;
	paramLen = sizeof(CK_PKCS5_PBKD2_PARAMS);

	
	prfAlgTag = SEC_OID_HMAC_SHA1;
 	if (p5_param.pPrfAlgId &&
 	    p5_param.pPrfAlgId->algorithm.data != 0) {
 	    prfAlgTag = SECOID_GetAlgorithmTag(p5_param.pPrfAlgId);
	}
	if (prfAlgTag == SEC_OID_HMAC_SHA1) {
	    pbeV2_params->prf = CKP_PKCS5_PBKD2_HMAC_SHA1;
	} else {
	    
	    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	    goto loser;
	}
	
	
	pbeV2_params->pPrfData = NULL;
	pbeV2_params->ulPrfDataLen = 0;
	pbeV2_params->saltSource = CKZ_SALT_SPECIFIED;
	pSalt = ((CK_CHAR_PTR) pbeV2_params)+sizeof(CK_PKCS5_PBKD2_PARAMS);
        PORT_Memcpy(pSalt, salt->data, salt->len);
	pbeV2_params->pSaltSourceData = pSalt;
	pbeV2_params->ulSaltSourceDataLen = salt->len;
	pbeV2_params->iterations = iterations;
    } else {
	CK_PBE_PARAMS *pbe_params = NULL;
    	pbe_params = (CK_PBE_PARAMS *)PORT_ZAlloc(sizeof(CK_PBE_PARAMS)+
						salt->len+iv_len);
	if (pbe_params == NULL) {
	    goto loser;
	}
	paramData = (unsigned char *)pbe_params;
	paramLen = sizeof(CK_PBE_PARAMS);

	pSalt = ((CK_CHAR_PTR) pbe_params)+sizeof(CK_PBE_PARAMS);
    	pbe_params->pSalt = pSalt;
        PORT_Memcpy(pSalt, salt->data, salt->len);
	pbe_params->ulSaltLen = salt->len;
	if (iv_len) {
	    pbe_params->pInitVector = 
		((CK_CHAR_PTR) pbe_params)+ sizeof(CK_PBE_PARAMS)+salt->len;
	}
	pbe_params->ulIteration = iterations;
    }

    
    mech->data = paramData;
    mech->len = paramLen;
    if (arena) {
	PORT_FreeArena(arena,PR_TRUE);
    }
    return SECSuccess;

loser:
    if (paramData) {
	PORT_Free(paramData);
    }
    if (arena) {
	PORT_FreeArena(arena,PR_TRUE);
    }
    return SECFailure;
}







SECStatus
PBE_PK11ParamToAlgid(SECOidTag algTag, SECItem *param, PRArenaPool *arena, 
		     SECAlgorithmID *algId)
{
    CK_PBE_PARAMS *pbe_param;
    SECItem pbeSalt;
    SECAlgorithmID *pbeAlgID = NULL;
    SECStatus rv;

    if(!param || !algId) {
	return SECFailure;
    }

    pbe_param = (CK_PBE_PARAMS *)param->data;
    pbeSalt.data = (unsigned char *)pbe_param->pSalt;
    pbeSalt.len = pbe_param->ulSaltLen;
    pbeAlgID = sec_pkcs5CreateAlgorithmID(algTag, SEC_OID_UNKNOWN, 
	SEC_OID_UNKNOWN, NULL, 0, &pbeSalt, (int)pbe_param->ulIteration);
    if(!pbeAlgID) {
	return SECFailure;
    }

    rv = SECOID_CopyAlgorithmID(arena, algId, pbeAlgID);
    SECOID_DestroyAlgorithmID(pbeAlgID, PR_TRUE);
    return rv;
}








PBEBitGenContext *
PBE_CreateContext(SECOidTag hashAlgorithm, PBEBitGenID bitGenPurpose,
	SECItem *pwitem, SECItem *salt, unsigned int bitsNeeded,
	unsigned int iterations)
{
    SECItem *context = NULL;
    SECItem mechItem;
    CK_PBE_PARAMS pbe_params;
    CK_MECHANISM_TYPE mechanism = CKM_INVALID_MECHANISM;
    PK11SlotInfo *slot;
    PK11SymKey *symKey = NULL;
    unsigned char ivData[8];
    

    
    switch (bitGenPurpose) {
    case pbeBitGenIntegrityKey:
	switch (hashAlgorithm) {
	case SEC_OID_SHA1:
	    mechanism = CKM_PBA_SHA1_WITH_SHA1_HMAC;
	    break;
	case SEC_OID_MD2:
	    mechanism = CKM_NETSCAPE_PBE_MD2_HMAC_KEY_GEN;
	    break;
	case SEC_OID_MD5:
	    mechanism = CKM_NETSCAPE_PBE_MD5_HMAC_KEY_GEN;
	    break;
	default:
	    break;
	}
	break;
    case pbeBitGenCipherIV:
	if (bitsNeeded > 64) {
	    break;
	}
	if (hashAlgorithm != SEC_OID_SHA1) {
	    break;
	}
	mechanism = CKM_PBE_SHA1_DES3_EDE_CBC;
	break;
    case pbeBitGenCipherKey:
	if (hashAlgorithm != SEC_OID_SHA1) {
	    break;
	}
	switch (bitsNeeded) {
	case 40:
	    mechanism = CKM_PBE_SHA1_RC4_40;
	    break;
	case 128:
	    mechanism = CKM_PBE_SHA1_RC4_128;
	    break;
	default:
	    break;
	}
    case pbeBitGenIDNull:
	break;
    }

    if (mechanism == CKM_INVALID_MECHANISM) {
	

	    return NULL;
    } 

    pbe_params.pInitVector = ivData;
    pbe_params.pPassword = pwitem->data;
    pbe_params.ulPasswordLen = pwitem->len;
    pbe_params.pSalt = salt->data;
    pbe_params.ulSaltLen = salt->len;
    pbe_params.ulIteration = iterations;
    mechItem.data = (unsigned char *) &pbe_params;
    mechItem.len = sizeof(pbe_params);


    slot = PK11_GetInternalSlot();
    symKey = PK11_RawPBEKeyGen(slot,mechanism,
					&mechItem, pwitem, PR_FALSE, NULL);
    PK11_FreeSlot(slot);
    if (symKey != NULL) {
	if (bitGenPurpose == pbeBitGenCipherIV) {
	    
	    SECItem ivItem;

	    ivItem.data = ivData;
	    ivItem.len = bitsNeeded/8;
	    context = SECITEM_DupItem(&ivItem);
	} else {
	    SECItem *keyData;
	    PK11_ExtractKeyValue(symKey);
	    keyData = PK11_GetKeyData(symKey);

	    
	    if (keyData) {
	    	context = SECITEM_DupItem(keyData);
	    }
	}
	PK11_FreeSymKey(symKey);
    }

    return (PBEBitGenContext *)context;
}








SECItem *
PBE_GenerateBits(PBEBitGenContext *context)
{
    return (SECItem *)context;
}








void
PBE_DestroyContext(PBEBitGenContext *context)
{
    SECITEM_FreeItem((SECItem *)context,PR_TRUE);
}




SECItem *
SEC_PKCS5GetIV(SECAlgorithmID *algid, SECItem *pwitem, PRBool faulty3DES)
{
    
    CK_MECHANISM_TYPE type;
    SECItem *param = NULL;
    SECItem *iv = NULL;
    SECItem src;
    int iv_len = 0;
    PK11SymKey *symKey;
    PK11SlotInfo *slot;
    CK_PBE_PARAMS_PTR pPBEparams;
    SECOidTag	pbeAlg;

    pbeAlg = SECOID_GetAlgorithmTag(algid);
    if (sec_pkcs5_is_algorithm_v2_pkcs5_algorithm(pbeAlg)) {
	unsigned char *ivData;
	sec_pkcs5V2Parameter *pbeV2_param = NULL;

	
	if (pbeAlg == SEC_OID_PKCS5_PBKDF2) {
	    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	    goto loser;
	}
	pbeV2_param = sec_pkcs5_v2_get_v2_param(NULL, algid);
	if (pbeV2_param == NULL) {
	    goto loser;
	}
	

    	type = PK11_AlgtagToMechanism(
    		SECOID_GetAlgorithmTag(&pbeV2_param->cipherAlgId));
	param = PK11_ParamFromAlgid(&pbeV2_param->cipherAlgId);
	sec_pkcs5_v2_destroy_v2_param(pbeV2_param);
	if (!param) {
	    goto loser;
	}
	
	ivData = PK11_IVFromParam(type, param, &iv_len);
	src.data = ivData;
	src.len = iv_len;
	goto done;
    }

    type = PK11_AlgtagToMechanism(pbeAlg);
    param = PK11_ParamFromAlgid(algid);
    if (param == NULL) {
	goto done;
    }
    slot = PK11_GetInternalSlot();
    symKey = PK11_RawPBEKeyGen(slot, type, param, pwitem, faulty3DES, NULL);
    PK11_FreeSlot(slot);
    if (symKey == NULL) {
	goto loser;
    }
    PK11_FreeSymKey(symKey);
    pPBEparams = (CK_PBE_PARAMS_PTR)param->data;
    iv_len = PK11_GetIVLength(type);

    src.data = (unsigned char *)pPBEparams->pInitVector;
    src.len = iv_len;

done:
    iv = SECITEM_DupItem(&src);

loser:
    if (param) {
	SECITEM_ZfreeItem(param, PR_TRUE);
    }
    return iv;
}




PBEBitGenContext *
__PBE_CreateContext(SECOidTag hashAlgorithm, PBEBitGenID bitGenPurpose,
	SECItem *pwitem, SECItem *salt, unsigned int bitsNeeded,
	unsigned int iterations)
{
    PORT_Assert("__PBE_CreateContext is Deprecated" == NULL);
    return NULL;
}

SECItem *
__PBE_GenerateBits(PBEBitGenContext *context)
{
    PORT_Assert("__PBE_GenerateBits is Deprecated" == NULL);
    return NULL;
}

void
__PBE_DestroyContext(PBEBitGenContext *context)
{
    PORT_Assert("__PBE_DestroyContext is Deprecated" == NULL);
}

SECStatus
RSA_FormatBlock(SECItem *result, unsigned modulusLen,
                int blockType, SECItem *data)
{
    PORT_Assert("RSA_FormatBlock is Deprecated" == NULL);
    return SECFailure;
}







static void
pk11_destroy_ck_pbe_params(CK_PBE_PARAMS *pbe_params)
{
    if (pbe_params) {
	if (pbe_params->pPassword)
	    PORT_ZFree(pbe_params->pPassword, pbe_params->ulPasswordLen);
	if (pbe_params->pSalt)
	    PORT_ZFree(pbe_params->pSalt, pbe_params->ulSaltLen);
	PORT_ZFree(pbe_params, sizeof(CK_PBE_PARAMS));
    }
}







SECItem * 
PK11_CreatePBEParams(SECItem *salt, SECItem *pwd, unsigned int iterations)
{
    CK_PBE_PARAMS *pbe_params = NULL;
    SECItem *paramRV = NULL;

    paramRV = SECITEM_AllocItem(NULL, NULL, sizeof(CK_PBE_PARAMS));
    if (!paramRV ) {
	goto loser;
    }
    
    PORT_Memset(paramRV->data, 0, sizeof(CK_PBE_PARAMS));

    pbe_params = (CK_PBE_PARAMS *)paramRV->data;
    pbe_params->pPassword = (CK_CHAR_PTR)PORT_ZAlloc(pwd->len);
    if (!pbe_params->pPassword) {
        goto loser;
    }
    PORT_Memcpy(pbe_params->pPassword, pwd->data, pwd->len);
    pbe_params->ulPasswordLen = pwd->len;

    pbe_params->pSalt = (CK_CHAR_PTR)PORT_ZAlloc(salt->len);
    if (!pbe_params->pSalt) {
	goto loser;
    }
    PORT_Memcpy(pbe_params->pSalt, salt->data, salt->len);
    pbe_params->ulSaltLen = salt->len;

    pbe_params->ulIteration = (CK_ULONG)iterations;
    return paramRV;

loser:
    if (pbe_params)
        pk11_destroy_ck_pbe_params(pbe_params);
    if (paramRV) 
    	PORT_ZFree(paramRV, sizeof(SECItem));
    return NULL;
}




void
PK11_DestroyPBEParams(SECItem *pItem)
{
    if (pItem) {
	CK_PBE_PARAMS * params = (CK_PBE_PARAMS *)(pItem->data);
	if (params)
	    pk11_destroy_ck_pbe_params(params);
	PORT_ZFree(pItem, sizeof(SECItem));
    }
}






SECAlgorithmID *
PK11_CreatePBEAlgorithmID(SECOidTag algorithm, int iteration, SECItem *salt)
{
    SECAlgorithmID *algid = NULL;
    algid = sec_pkcs5CreateAlgorithmID(algorithm,
		 SEC_OID_UNKNOWN, SEC_OID_UNKNOWN, NULL, 0, salt, iteration);
    return algid;
}




SECAlgorithmID *
PK11_CreatePBEV2AlgorithmID(SECOidTag pbeAlgTag, SECOidTag cipherAlgTag,
			    SECOidTag prfAlgTag, int keyLength, int iteration, 
			    SECItem *salt)
{
    SECAlgorithmID *algid = NULL;
    algid = sec_pkcs5CreateAlgorithmID(pbeAlgTag, cipherAlgTag, prfAlgTag,
					NULL, keyLength, salt, iteration);
    return algid;
}




PK11SymKey *
pk11_RawPBEKeyGenWithKeyType(PK11SlotInfo *slot, CK_MECHANISM_TYPE type, 
			SECItem *params, CK_KEY_TYPE keyType, int keyLen,
			SECItem *pwitem, void *wincx)
{
    CK_ULONG pwLen;
    
    if ((params == NULL) || (params->data == NULL)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }

    if (type == CKM_INVALID_MECHANISM) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return NULL;
    }

    
    if (type == CKM_PKCS5_PBKD2) {
    	CK_PKCS5_PBKD2_PARAMS *pbev2_params;
	if (params->len < sizeof(CK_PKCS5_PBKD2_PARAMS)) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return NULL;
	}
	pbev2_params = (CK_PKCS5_PBKD2_PARAMS *)params->data;
	pbev2_params->pPassword = pwitem->data;
	pwLen = pwitem->len;
	pbev2_params->ulPasswordLen = &pwLen;
    } else {
    	CK_PBE_PARAMS *pbe_params;
	if (params->len < sizeof(CK_PBE_PARAMS)) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return NULL;
	}
	pbe_params = (CK_PBE_PARAMS *)params->data;
	pbe_params->pPassword = pwitem->data;
	pbe_params->ulPasswordLen = pwitem->len;
    }

    
    return pk11_TokenKeyGenWithFlagsAndKeyType(slot, type, params, keyType, 
	   keyLen, NULL, CKF_SIGN|CKF_ENCRYPT|CKF_DECRYPT|CKF_UNWRAP|CKF_WRAP, 
	   0, wincx);
}




PK11SymKey *
PK11_RawPBEKeyGen(PK11SlotInfo *slot, CK_MECHANISM_TYPE type, SECItem *mech,
			 SECItem *pwitem, PRBool faulty3DES, void *wincx)
{
    if(faulty3DES && (type == CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC)) {
	type = CKM_NETSCAPE_PBE_SHA1_FAULTY_3DES_CBC;
    }
    return pk11_RawPBEKeyGenWithKeyType(slot, type, mech, -1, 0, pwitem, wincx);
}








PK11SymKey *
PK11_PBEKeyGen(PK11SlotInfo *slot, SECAlgorithmID *algid, SECItem *pwitem,
	       					PRBool faulty3DES, void *wincx)
{
    CK_MECHANISM_TYPE type;
    SECItem *param = NULL;
    PK11SymKey *symKey;
    SECOidTag	pbeAlg;
    CK_KEY_TYPE keyType = -1;
    int keyLen = 0;

    pbeAlg = SECOID_GetAlgorithmTag(algid);
    

    if (sec_pkcs5_is_algorithm_v2_pkcs5_algorithm(pbeAlg)) {
	CK_MECHANISM_TYPE cipherMech;
	sec_pkcs5V2Parameter *pbeV2_param;

	pbeV2_param = sec_pkcs5_v2_get_v2_param(NULL, algid);
	if (pbeV2_param == NULL) {
	    return NULL;
	}
	cipherMech = PK11_AlgtagToMechanism(
		SECOID_GetAlgorithmTag(&pbeV2_param->cipherAlgId));
	pbeAlg = SECOID_GetAlgorithmTag(&pbeV2_param->pbeAlgId);
	param = PK11_ParamFromAlgid(&pbeV2_param->pbeAlgId);
	sec_pkcs5_v2_destroy_v2_param(pbeV2_param);
	keyLen = SEC_PKCS5GetKeyLength(algid);
	if (keyLen == -1) {
	    keyLen = 0;
	}
	keyType = PK11_GetKeyType(cipherMech, keyLen);
    } else {
	param = PK11_ParamFromAlgid(algid);
    }
    if(param == NULL) {
	return NULL;
    }

    type = PK11_AlgtagToMechanism(pbeAlg);	
    if (type == CKM_INVALID_MECHANISM) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return NULL;
    }
    if(faulty3DES && (type == CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC)) {
	type = CKM_NETSCAPE_PBE_SHA1_FAULTY_3DES_CBC;
    }
    symKey = pk11_RawPBEKeyGenWithKeyType(slot, type, param, keyType, keyLen, 
					pwitem, wincx);

    SECITEM_ZfreeItem(param, PR_TRUE);
    return symKey;
}




SECItem *
PK11_GetPBEIV(SECAlgorithmID *algid, SECItem *pwitem)
{
    return SEC_PKCS5GetIV(algid, pwitem, PR_FALSE);
}

CK_MECHANISM_TYPE
pk11_GetPBECryptoMechanism(SECAlgorithmID *algid, SECItem **param, 
			   SECItem *pbe_pwd, PRBool faulty3DES)
{
    int keyLen = 0;
    SECOidTag algTag = SEC_PKCS5GetCryptoAlgorithm(algid);
    CK_MECHANISM_TYPE mech = PK11_AlgtagToMechanism(algTag);
    CK_MECHANISM_TYPE returnedMechanism = CKM_INVALID_MECHANISM;
    SECItem *iv = NULL;

    if (mech == CKM_INVALID_MECHANISM) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	goto loser;
    }
    if (PK11_GetIVLength(mech)) {
	iv = SEC_PKCS5GetIV(algid, pbe_pwd, faulty3DES);
	if (iv == NULL) {
	    goto loser;
	}
    }

    keyLen = SEC_PKCS5GetKeyLength(algid);

    *param = pk11_ParamFromIVWithLen(mech, iv, keyLen);
    if (*param == NULL) {
	goto loser;
    }
    returnedMechanism = mech;

loser:
    if (iv) {
	SECITEM_FreeItem(iv,PR_TRUE);
    }
    return returnedMechanism;
}












CK_MECHANISM_TYPE
PK11_GetPBECryptoMechanism(SECAlgorithmID *algid, SECItem **param, 
			   SECItem *pbe_pwd)
{
    return pk11_GetPBECryptoMechanism(algid, param, pbe_pwd, PR_FALSE);
}
