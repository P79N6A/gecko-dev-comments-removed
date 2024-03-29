






#include "seccomon.h"
#include "secmod.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pkcs11.h"
#include "pkcs11t.h"
#include "pk11func.h"
#include "cert.h"
#include "key.h"
#include "secitem.h"
#include "secasn1.h" 
#include "secoid.h" 
#include "secerr.h"
#include "sslerr.h"
#include "sechash.h"

#include "secpkcs5.h"  
#include "blapit.h"

static SECItem *
pk11_MakeIDFromPublicKey(SECKEYPublicKey *pubKey)
{
    
    SECItem *pubKeyIndex =  NULL;
    switch (pubKey->keyType) {
    case rsaKey:
      pubKeyIndex = &pubKey->u.rsa.modulus;
      break;
    case dsaKey:
      pubKeyIndex = &pubKey->u.dsa.publicValue;
      break;
    case dhKey:
      pubKeyIndex = &pubKey->u.dh.publicValue;
      break;      
    case ecKey:
      pubKeyIndex = &pubKey->u.ec.publicValue;
      break;      
    default:
      return NULL;
    }
    PORT_Assert(pubKeyIndex != NULL);

    return PK11_MakeIDFromPubKey(pubKeyIndex);
} 










CK_OBJECT_HANDLE
PK11_ImportPublicKey(PK11SlotInfo *slot, SECKEYPublicKey *pubKey, 
								PRBool isToken)
{
    CK_BBOOL cktrue = CK_TRUE;
    CK_BBOOL ckfalse = CK_FALSE;
    CK_OBJECT_CLASS keyClass = CKO_PUBLIC_KEY;
    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
    CK_OBJECT_HANDLE objectID;
    CK_ATTRIBUTE theTemplate[11];
    CK_ATTRIBUTE *signedattr = NULL;
    CK_ATTRIBUTE *attrs = theTemplate;
    SECItem *ckaId = NULL;
    SECItem *pubValue = NULL;
    int signedcount = 0;
    int templateCount = 0;
    SECStatus rv;

    
    if (!isToken && pubKey->pkcs11Slot == slot) {
	return pubKey->pkcs11ID;
    }

    
    if (pubKey->pkcs11Slot != NULL) {
	PK11SlotInfo *oSlot = pubKey->pkcs11Slot;
	if (!PK11_IsPermObject(pubKey->pkcs11Slot,pubKey->pkcs11ID)) {
	    PK11_EnterSlotMonitor(oSlot);
	    (void) PK11_GETTAB(oSlot)->C_DestroyObject(oSlot->session,
							pubKey->pkcs11ID);
	    PK11_ExitSlotMonitor(oSlot);
	}
	PK11_FreeSlot(oSlot);
	pubKey->pkcs11Slot = NULL;
    }
    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, sizeof(keyClass) ); attrs++;
    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, sizeof(keyType) ); attrs++;
    PK11_SETATTRS(attrs, CKA_TOKEN, isToken ? &cktrue : &ckfalse,
						 sizeof(CK_BBOOL) ); attrs++;
    if (isToken) {
	ckaId = pk11_MakeIDFromPublicKey(pubKey);
	if (ckaId == NULL) {
	    PORT_SetError( SEC_ERROR_BAD_KEY );
	    return CK_INVALID_HANDLE;
	}
	PK11_SETATTRS(attrs, CKA_ID, ckaId->data, ckaId->len); attrs++;
    }

    
    {
        switch (pubKey->keyType) {
        case rsaKey:
	    keyType = CKK_RSA;
	    PK11_SETATTRS(attrs, CKA_WRAP, &cktrue, sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_ENCRYPT, &cktrue, 
						sizeof(CK_BBOOL) ); attrs++;
	    PK11_SETATTRS(attrs, CKA_VERIFY, &cktrue, sizeof(CK_BBOOL)); attrs++;
 	    signedattr = attrs;
	    PK11_SETATTRS(attrs, CKA_MODULUS, pubKey->u.rsa.modulus.data,
					 pubKey->u.rsa.modulus.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_PUBLIC_EXPONENT, 
	     	pubKey->u.rsa.publicExponent.data,
				 pubKey->u.rsa.publicExponent.len); attrs++;
	    break;
        case dsaKey:
	    keyType = CKK_DSA;
	    PK11_SETATTRS(attrs, CKA_VERIFY, &cktrue, sizeof(CK_BBOOL));attrs++;
 	    signedattr = attrs;
	    PK11_SETATTRS(attrs, CKA_PRIME,    pubKey->u.dsa.params.prime.data,
				pubKey->u.dsa.params.prime.len); attrs++;
	    PK11_SETATTRS(attrs,CKA_SUBPRIME,pubKey->u.dsa.params.subPrime.data,
				pubKey->u.dsa.params.subPrime.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_BASE,  pubKey->u.dsa.params.base.data,
					pubKey->u.dsa.params.base.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_VALUE,    pubKey->u.dsa.publicValue.data, 
					pubKey->u.dsa.publicValue.len); attrs++;
	    break;
	case fortezzaKey:
	    keyType = CKK_DSA;
	    PK11_SETATTRS(attrs, CKA_VERIFY, &cktrue, sizeof(CK_BBOOL));attrs++;
 	    signedattr = attrs;
	    PK11_SETATTRS(attrs, CKA_PRIME,pubKey->u.fortezza.params.prime.data,
				pubKey->u.fortezza.params.prime.len); attrs++;
	    PK11_SETATTRS(attrs,CKA_SUBPRIME,
				pubKey->u.fortezza.params.subPrime.data,
				pubKey->u.fortezza.params.subPrime.len);attrs++;
	    PK11_SETATTRS(attrs, CKA_BASE,  pubKey->u.fortezza.params.base.data,
				pubKey->u.fortezza.params.base.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_VALUE, pubKey->u.fortezza.DSSKey.data, 
				pubKey->u.fortezza.DSSKey.len); attrs++;
            break;
        case dhKey:
	    keyType = CKK_DH;
	    PK11_SETATTRS(attrs, CKA_DERIVE, &cktrue, sizeof(CK_BBOOL));attrs++;
 	    signedattr = attrs;
	    PK11_SETATTRS(attrs, CKA_PRIME,    pubKey->u.dh.prime.data,
				pubKey->u.dh.prime.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_BASE,  pubKey->u.dh.base.data,
					pubKey->u.dh.base.len); attrs++;
	    PK11_SETATTRS(attrs, CKA_VALUE,    pubKey->u.dh.publicValue.data, 
					pubKey->u.dh.publicValue.len); attrs++;
	    break;
        case ecKey:
	    keyType = CKK_EC;
	    PK11_SETATTRS(attrs, CKA_VERIFY, &cktrue, sizeof(CK_BBOOL));attrs++;
	    PK11_SETATTRS(attrs, CKA_DERIVE, &cktrue, sizeof(CK_BBOOL));attrs++;
 	    signedattr = attrs;
	    PK11_SETATTRS(attrs, CKA_EC_PARAMS, 
		          pubKey->u.ec.DEREncodedParams.data,
		          pubKey->u.ec.DEREncodedParams.len); attrs++;
	    if (PR_GetEnv("NSS_USE_DECODED_CKA_EC_POINT")) {
	    	PK11_SETATTRS(attrs, CKA_EC_POINT, 
			  pubKey->u.ec.publicValue.data,
			  pubKey->u.ec.publicValue.len); attrs++;
	    } else {
		pubValue = SEC_ASN1EncodeItem(NULL, NULL,
			&pubKey->u.ec.publicValue,
			SEC_ASN1_GET(SEC_OctetStringTemplate));
		if (pubValue == NULL) {
		    if (ckaId) {
			SECITEM_FreeItem(ckaId,PR_TRUE);
		    }
		    return CK_INVALID_HANDLE;
		}
	    	PK11_SETATTRS(attrs, CKA_EC_POINT, 
			  pubValue->data, pubValue->len); attrs++;
	    }
	    break;
	default:
	    if (ckaId) {
		SECITEM_FreeItem(ckaId,PR_TRUE);
	    }
	    PORT_SetError( SEC_ERROR_BAD_KEY );
	    return CK_INVALID_HANDLE;
	}

	templateCount = attrs - theTemplate;
	signedcount = attrs - signedattr;
	PORT_Assert(templateCount <= (sizeof(theTemplate)/sizeof(CK_ATTRIBUTE)));
	for (attrs=signedattr; signedcount; attrs++, signedcount--) {
		pk11_SignedToUnsigned(attrs);
	} 
        rv = PK11_CreateNewObject(slot, CK_INVALID_SESSION, theTemplate,
				 	templateCount, isToken, &objectID);
	if (ckaId) {
	    SECITEM_FreeItem(ckaId,PR_TRUE);
	}
	if (pubValue) {
	    SECITEM_FreeItem(pubValue,PR_TRUE);
	}
	if ( rv != SECSuccess) {
	    return CK_INVALID_HANDLE;
	}
    }

    pubKey->pkcs11ID = objectID;
    pubKey->pkcs11Slot = PK11_ReferenceSlot(slot);

    return objectID;
}




static CK_RV
pk11_Attr2SecItem(PLArenaPool *arena, const CK_ATTRIBUTE *attr, SECItem *item)
{
    item->data = NULL;

    (void)SECITEM_AllocItem(arena, item, attr->ulValueLen);
    if (item->data == NULL) {
	return CKR_HOST_MEMORY;
    } 
    PORT_Memcpy(item->data, attr->pValue, item->len);
    return CKR_OK;
}


















static int
pk11_get_EC_PointLenInBytes(PLArenaPool *arena, const SECItem *ecParams)
{
   SECItem oid;
   SECOidTag tag;
   SECStatus rv;

   
   rv = SEC_QuickDERDecodeItem(arena, &oid,
		SEC_ASN1_GET(SEC_ObjectIDTemplate), ecParams);
   if (rv != SECSuccess) {
	




	return 0;
   }

   tag = SECOID_FindOIDTag(&oid);
   switch (tag) {
    case SEC_OID_SECG_EC_SECP112R1:
    case SEC_OID_SECG_EC_SECP112R2:
	return 29; 
    case SEC_OID_SECG_EC_SECT113R1:
    case SEC_OID_SECG_EC_SECT113R2:
	return 31; 
    case SEC_OID_SECG_EC_SECP128R1:
    case SEC_OID_SECG_EC_SECP128R2:
	return 33; 
    case SEC_OID_SECG_EC_SECT131R1:
    case SEC_OID_SECG_EC_SECT131R2:
	return 35; 
    case SEC_OID_SECG_EC_SECP160K1:
    case SEC_OID_SECG_EC_SECP160R1:
    case SEC_OID_SECG_EC_SECP160R2:
	return 41; 
    case SEC_OID_SECG_EC_SECT163K1:
    case SEC_OID_SECG_EC_SECT163R1:
    case SEC_OID_SECG_EC_SECT163R2:
    case SEC_OID_ANSIX962_EC_C2PNB163V1:
    case SEC_OID_ANSIX962_EC_C2PNB163V2:
    case SEC_OID_ANSIX962_EC_C2PNB163V3:
	return 43; 
    case SEC_OID_ANSIX962_EC_C2PNB176V1:
	return 45; 
    case SEC_OID_ANSIX962_EC_C2TNB191V1:
    case SEC_OID_ANSIX962_EC_C2TNB191V2:
    case SEC_OID_ANSIX962_EC_C2TNB191V3:
    case SEC_OID_SECG_EC_SECP192K1:
    case SEC_OID_ANSIX962_EC_PRIME192V1:
    case SEC_OID_ANSIX962_EC_PRIME192V2:
    case SEC_OID_ANSIX962_EC_PRIME192V3:
	return 49; 
    case SEC_OID_SECG_EC_SECT193R1:
    case SEC_OID_SECG_EC_SECT193R2:
	return 51; 
    case SEC_OID_ANSIX962_EC_C2PNB208W1:
	return 53; 
    case SEC_OID_SECG_EC_SECP224K1:
    case SEC_OID_SECG_EC_SECP224R1:
	return 57; 
    case SEC_OID_SECG_EC_SECT233K1:
    case SEC_OID_SECG_EC_SECT233R1:
    case SEC_OID_SECG_EC_SECT239K1:
    case SEC_OID_ANSIX962_EC_PRIME239V1:
    case SEC_OID_ANSIX962_EC_PRIME239V2:
    case SEC_OID_ANSIX962_EC_PRIME239V3:
    case SEC_OID_ANSIX962_EC_C2TNB239V1:
    case SEC_OID_ANSIX962_EC_C2TNB239V2:
    case SEC_OID_ANSIX962_EC_C2TNB239V3:
	return 61; 
    case SEC_OID_ANSIX962_EC_PRIME256V1:
    case SEC_OID_SECG_EC_SECP256K1:
	return 65; 
    case SEC_OID_ANSIX962_EC_C2PNB272W1:
	return 69; 
    case SEC_OID_SECG_EC_SECT283K1:
    case SEC_OID_SECG_EC_SECT283R1:
	return 73; 
    case SEC_OID_ANSIX962_EC_C2PNB304W1:
	return 77; 
    case SEC_OID_ANSIX962_EC_C2TNB359V1:
	return 91; 
    case SEC_OID_ANSIX962_EC_C2PNB368W1:
	return 93; 
    case SEC_OID_SECG_EC_SECP384R1:
	return 97; 
    case SEC_OID_SECG_EC_SECT409K1:
    case SEC_OID_SECG_EC_SECT409R1:
	return 105; 
    case SEC_OID_ANSIX962_EC_C2TNB431R1:
	return 109; 
    case SEC_OID_SECG_EC_SECP521R1:
	return 133; 
    case SEC_OID_SECG_EC_SECT571K1:
    case SEC_OID_SECG_EC_SECT571R1:
	return 145; 
    
    default:
	break;
   }
   return 0;
}








static CK_RV
pk11_get_Decoded_ECPoint(PLArenaPool *arena, const SECItem *ecParams,
	const CK_ATTRIBUTE *ecPoint, SECItem *publicKeyValue)
{
    SECItem encodedPublicValue;
    SECStatus rv;
    int keyLen;

    if (ecPoint->ulValueLen == 0) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }

    









    






    keyLen = pk11_get_EC_PointLenInBytes(arena, ecParams);
    if (keyLen < 0) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }


    

    if ((*((char *)ecPoint->pValue) == EC_POINT_FORM_UNCOMPRESSED) 
	&& (ecPoint->ulValueLen == keyLen)) {
	    return pk11_Attr2SecItem(arena, ecPoint, publicKeyValue);
    }

    
    if (*((char *)ecPoint->pValue) == SEC_ASN1_OCTET_STRING) {
	
	encodedPublicValue.data = ecPoint->pValue;
	encodedPublicValue.len = ecPoint->ulValueLen;
	rv = SEC_QuickDERDecodeItem(arena, publicKeyValue,
		SEC_ASN1_GET(SEC_OctetStringTemplate), &encodedPublicValue);

	

        if (keyLen && rv == SECSuccess && publicKeyValue->len == keyLen) {
	    return CKR_OK;
	}

	

	if (keyLen) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
		

	













	if ((rv != SECSuccess)
	    || ((publicKeyValue->len & 1) != 1)
	    || (publicKeyValue->data[0] != EC_POINT_FORM_UNCOMPRESSED)
	    || (PORT_Memcmp(&encodedPublicValue.data[encodedPublicValue.len -
		 	    publicKeyValue->len], publicKeyValue->data, 
			    publicKeyValue->len) != 0)) {
	    



	    if ((encodedPublicValue.len & 1) == 0) {
		return CKR_ATTRIBUTE_VALUE_INVALID;
	    }
	    return pk11_Attr2SecItem(arena, ecPoint, publicKeyValue);
	}

	






























































	return CKR_OK;
    }

    










	
    return CKR_ATTRIBUTE_VALUE_INVALID;
}




SECKEYPublicKey *
PK11_ExtractPublicKey(PK11SlotInfo *slot,KeyType keyType,CK_OBJECT_HANDLE id)
{
    CK_OBJECT_CLASS keyClass = CKO_PUBLIC_KEY;
    PLArenaPool *arena;
    PLArenaPool *tmp_arena;
    SECKEYPublicKey *pubKey;
    int templateCount = 0;
    CK_KEY_TYPE pk11KeyType;
    CK_RV crv;
    CK_ATTRIBUTE template[8];
    CK_ATTRIBUTE *attrs= template;
    CK_ATTRIBUTE *modulus,*exponent,*base,*prime,*subprime,*value;
    CK_ATTRIBUTE *ecparams;

    
    if (keyType== nullKey) {

        pk11KeyType = PK11_ReadULongAttribute(slot,id,CKA_KEY_TYPE);
	if (pk11KeyType ==  CK_UNAVAILABLE_INFORMATION) {
	    return NULL;
	}
	switch (pk11KeyType) {
	case CKK_RSA:
	    keyType = rsaKey;
	    break;
	case CKK_DSA:
	    keyType = dsaKey;
	    break;
	case CKK_DH:
	    keyType = dhKey;
	    break;
	case CKK_EC:
	    keyType = ecKey;
	    break;
	default:
	    PORT_SetError( SEC_ERROR_BAD_KEY );
	    return NULL;
	}
    }


    
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return NULL;
    tmp_arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (tmp_arena == NULL) {
	PORT_FreeArena (arena, PR_FALSE);
	return NULL;
    }


    pubKey = (SECKEYPublicKey *) 
			PORT_ArenaZAlloc(arena, sizeof(SECKEYPublicKey));
    if (pubKey == NULL) {
	PORT_FreeArena (arena, PR_FALSE);
	PORT_FreeArena (tmp_arena, PR_FALSE);
	return NULL;
    }

    pubKey->arena = arena;
    pubKey->keyType = keyType;
    pubKey->pkcs11Slot = PK11_ReferenceSlot(slot);
    pubKey->pkcs11ID = id;
    PK11_SETATTRS(attrs, CKA_CLASS, &keyClass, 
						sizeof(keyClass)); attrs++;
    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &pk11KeyType, 
						sizeof(pk11KeyType) ); attrs++;
    switch (pubKey->keyType) {
    case rsaKey:
	modulus = attrs;
	PK11_SETATTRS(attrs, CKA_MODULUS, NULL, 0); attrs++; 
	exponent = attrs;
	PK11_SETATTRS(attrs, CKA_PUBLIC_EXPONENT, NULL, 0); attrs++; 

	templateCount = attrs - template;
	PR_ASSERT(templateCount <= sizeof(template)/sizeof(CK_ATTRIBUTE));
	crv = PK11_GetAttributes(tmp_arena,slot,id,template,templateCount);
	if (crv != CKR_OK) break;

	if ((keyClass != CKO_PUBLIC_KEY) || (pk11KeyType != CKK_RSA)) {
	    crv = CKR_OBJECT_HANDLE_INVALID;
	    break;
	} 
	crv = pk11_Attr2SecItem(arena,modulus,&pubKey->u.rsa.modulus);
	if (crv != CKR_OK) break;
	crv = pk11_Attr2SecItem(arena,exponent,&pubKey->u.rsa.publicExponent);
	if (crv != CKR_OK) break;
	break;
    case dsaKey:
	prime = attrs;
	PK11_SETATTRS(attrs, CKA_PRIME, NULL, 0); attrs++; 
	subprime = attrs;
	PK11_SETATTRS(attrs, CKA_SUBPRIME, NULL, 0); attrs++; 
	base = attrs;
	PK11_SETATTRS(attrs, CKA_BASE, NULL, 0); attrs++; 
	value = attrs;
	PK11_SETATTRS(attrs, CKA_VALUE, NULL, 0); attrs++; 
	templateCount = attrs - template;
	PR_ASSERT(templateCount <= sizeof(template)/sizeof(CK_ATTRIBUTE));
	crv = PK11_GetAttributes(tmp_arena,slot,id,template,templateCount);
	if (crv != CKR_OK) break;

	if ((keyClass != CKO_PUBLIC_KEY) || (pk11KeyType != CKK_DSA)) {
	    crv = CKR_OBJECT_HANDLE_INVALID;
	    break;
	} 
	crv = pk11_Attr2SecItem(arena,prime,&pubKey->u.dsa.params.prime);
	if (crv != CKR_OK) break;
	crv = pk11_Attr2SecItem(arena,subprime,&pubKey->u.dsa.params.subPrime);
	if (crv != CKR_OK) break;
	crv = pk11_Attr2SecItem(arena,base,&pubKey->u.dsa.params.base);
	if (crv != CKR_OK) break;
	crv = pk11_Attr2SecItem(arena,value,&pubKey->u.dsa.publicValue);
	if (crv != CKR_OK) break;
	break;
    case dhKey:
	prime = attrs;
	PK11_SETATTRS(attrs, CKA_PRIME, NULL, 0); attrs++; 
	base = attrs;
	PK11_SETATTRS(attrs, CKA_BASE, NULL, 0); attrs++; 
	value =attrs;
	PK11_SETATTRS(attrs, CKA_VALUE, NULL, 0); attrs++; 
	templateCount = attrs - template;
	PR_ASSERT(templateCount <= sizeof(template)/sizeof(CK_ATTRIBUTE));
	crv = PK11_GetAttributes(tmp_arena,slot,id,template,templateCount);
	if (crv != CKR_OK) break;

	if ((keyClass != CKO_PUBLIC_KEY) || (pk11KeyType != CKK_DH)) {
	    crv = CKR_OBJECT_HANDLE_INVALID;
	    break;
	} 
	crv = pk11_Attr2SecItem(arena,prime,&pubKey->u.dh.prime);
	if (crv != CKR_OK) break;
	crv = pk11_Attr2SecItem(arena,base,&pubKey->u.dh.base);
	if (crv != CKR_OK) break;
	crv = pk11_Attr2SecItem(arena,value,&pubKey->u.dh.publicValue);
	if (crv != CKR_OK) break;
	break;
    case ecKey:
	pubKey->u.ec.size = 0;
	ecparams = attrs;
	PK11_SETATTRS(attrs, CKA_EC_PARAMS, NULL, 0); attrs++; 
	value =attrs;
	PK11_SETATTRS(attrs, CKA_EC_POINT, NULL, 0); attrs++; 
	templateCount = attrs - template;
	PR_ASSERT(templateCount <= sizeof(template)/sizeof(CK_ATTRIBUTE));
	crv = PK11_GetAttributes(arena,slot,id,template,templateCount);
	if (crv != CKR_OK) break;

	if ((keyClass != CKO_PUBLIC_KEY) || (pk11KeyType != CKK_EC)) {
	    crv = CKR_OBJECT_HANDLE_INVALID;
	    break;
	} 

	crv = pk11_Attr2SecItem(arena,ecparams,
	                        &pubKey->u.ec.DEREncodedParams);
	if (crv != CKR_OK) break;
	crv = pk11_get_Decoded_ECPoint(arena,
		 &pubKey->u.ec.DEREncodedParams, value, 
		 &pubKey->u.ec.publicValue);
	break;
    case fortezzaKey:
    case nullKey:
    default:
	crv = CKR_OBJECT_HANDLE_INVALID;
	break;
    }

    PORT_FreeArena(tmp_arena,PR_FALSE);

    if (crv != CKR_OK) {
	PORT_FreeArena(arena,PR_FALSE);
	PK11_FreeSlot(slot);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }

    return pubKey;
}




SECKEYPrivateKey *
PK11_MakePrivKey(PK11SlotInfo *slot, KeyType keyType, 
			PRBool isTemp, CK_OBJECT_HANDLE privID, void *wincx)
{
    PLArenaPool *arena;
    SECKEYPrivateKey *privKey;
    PRBool isPrivate;
    SECStatus rv;

    
    if (keyType == nullKey) {
	CK_KEY_TYPE pk11Type = CKK_RSA;

	pk11Type = PK11_ReadULongAttribute(slot,privID,CKA_KEY_TYPE);
	isTemp = (PRBool)!PK11_HasAttributeSet(slot,privID,CKA_TOKEN,PR_FALSE);
	switch (pk11Type) {
	case CKK_RSA: keyType = rsaKey; break;
	case CKK_DSA: keyType = dsaKey; break;
	case CKK_DH: keyType = dhKey; break;
	case CKK_KEA: keyType = fortezzaKey; break;
	case CKK_EC: keyType = ecKey; break;
	default:
		break;
	}
    }

    

    isPrivate = (PRBool)PK11_HasAttributeSet(slot,privID,CKA_PRIVATE,PR_FALSE);
    if (isPrivate) {
	rv = PK11_Authenticate(slot, PR_TRUE, wincx);
 	if (rv != SECSuccess) {
 	    return NULL;
 	}
    }

    
    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) return NULL;

    privKey = (SECKEYPrivateKey *) 
			PORT_ArenaZAlloc(arena, sizeof(SECKEYPrivateKey));
    if (privKey == NULL) {
	PORT_FreeArena(arena, PR_FALSE);
	return NULL;
    }

    privKey->arena = arena;
    privKey->keyType = keyType;
    privKey->pkcs11Slot = PK11_ReferenceSlot(slot);
    privKey->pkcs11ID = privID;
    privKey->pkcs11IsTemp = isTemp;
    privKey->wincx = wincx;

    return privKey;
}


PK11SlotInfo *
PK11_GetSlotFromPrivateKey(SECKEYPrivateKey *key)
{
    PK11SlotInfo *slot = key->pkcs11Slot;
    slot = PK11_ReferenceSlot(slot);
    return slot;
}




int
PK11_GetPrivateModulusLen(SECKEYPrivateKey *key)
{
    CK_ATTRIBUTE theTemplate = { CKA_MODULUS, NULL, 0 };
    PK11SlotInfo *slot = key->pkcs11Slot;
    CK_RV crv;
    int length;

    switch (key->keyType) {
    case rsaKey:
	crv = PK11_GetAttributes(NULL, slot, key->pkcs11ID, &theTemplate, 1);
	if (crv != CKR_OK) {
	    PORT_SetError( PK11_MapError(crv) );
	    return -1;
	}
	length = theTemplate.ulValueLen;
	if ( *(unsigned char *)theTemplate.pValue == 0) {
	    length--;
	}
	if (theTemplate.pValue != NULL)
	    PORT_Free(theTemplate.pValue);
	return (int) length;
	
    case fortezzaKey:
    case dsaKey:
    case dhKey:
    default:
	break;
    }
    if (theTemplate.pValue != NULL)
	PORT_Free(theTemplate.pValue);
    PORT_SetError( SEC_ERROR_INVALID_KEY );
    return -1;
}









static SECKEYPrivateKey *
pk11_loadPrivKeyWithFlags(PK11SlotInfo *slot,SECKEYPrivateKey *privKey, 
		SECKEYPublicKey *pubKey, PK11AttrFlags attrFlags) 
{
    CK_ATTRIBUTE privTemplate[] = {
        
	{ CKA_CLASS, NULL, 0 },
	{ CKA_KEY_TYPE, NULL, 0 },
	{ CKA_ID, NULL, 0 },
	


	{ CKA_MODULUS, NULL, 0 },
	{ CKA_PRIVATE_EXPONENT, NULL, 0 },
	{ CKA_PUBLIC_EXPONENT, NULL, 0 },
	{ CKA_PRIME_1, NULL, 0 },
	{ CKA_PRIME_2, NULL, 0 },
	{ CKA_EXPONENT_1, NULL, 0 },
	{ CKA_EXPONENT_2, NULL, 0 },
	{ CKA_COEFFICIENT, NULL, 0 },
	{ CKA_DECRYPT, NULL, 0 },
	{ CKA_DERIVE, NULL, 0 },
	{ CKA_SIGN, NULL, 0 },
	{ CKA_SIGN_RECOVER, NULL, 0 },
	{ CKA_UNWRAP, NULL, 0 },
	

	{ CKA_TOKEN, NULL, 0 },
	{ CKA_PRIVATE, NULL, 0 },
	{ CKA_MODIFIABLE, NULL, 0 },
	{ CKA_SENSITIVE, NULL, 0 },
	{ CKA_EXTRACTABLE, NULL, 0 },
#define NUM_RESERVED_ATTRS 5    
    };
    CK_BBOOL cktrue = CK_TRUE;
    CK_BBOOL ckfalse = CK_FALSE;
    CK_ATTRIBUTE *attrs = NULL, *ap;
    const int templateSize = sizeof(privTemplate)/sizeof(privTemplate[0]);
    PLArenaPool *arena;
    CK_OBJECT_HANDLE objectID;
    int i, count = 0;
    int extra_count = 0;
    CK_RV crv;
    SECStatus rv;
    PRBool token = ((attrFlags & PK11_ATTR_TOKEN) != 0);

    if (pk11_BadAttrFlags(attrFlags)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }

    for (i=0; i < templateSize; i++) {
	if (privTemplate[i].type == CKA_MODULUS) {
	    attrs= &privTemplate[i];
	    count = i;
	    break;
	}
    }
    PORT_Assert(attrs != NULL);
    if (attrs == NULL) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return NULL;
    }

    ap = attrs;

    switch (privKey->keyType) {
    case rsaKey:
	count = templateSize - NUM_RESERVED_ATTRS;
	extra_count = count - (attrs - privTemplate);
	break;
    case dsaKey:
	ap->type = CKA_PRIME; ap++; count++; extra_count++;
	ap->type = CKA_SUBPRIME; ap++; count++; extra_count++;
	ap->type = CKA_BASE; ap++; count++; extra_count++;
	ap->type = CKA_VALUE; ap++; count++; extra_count++;
	ap->type = CKA_SIGN; ap++; count++; extra_count++;
	break;
    case dhKey:
	ap->type = CKA_PRIME; ap++; count++; extra_count++;
	ap->type = CKA_BASE; ap++; count++; extra_count++;
	ap->type = CKA_VALUE; ap++; count++; extra_count++;
	ap->type = CKA_DERIVE; ap++; count++; extra_count++;
	break;
    case ecKey:
	ap->type = CKA_EC_PARAMS; ap++; count++; extra_count++;
	ap->type = CKA_VALUE; ap++; count++; extra_count++;
	ap->type = CKA_DERIVE; ap++; count++; extra_count++;
	ap->type = CKA_SIGN; ap++; count++; extra_count++;
	break;
     default:
	count = 0;
	extra_count = 0;
	break;
     }

     if (count == 0) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return NULL;
     }

     arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
     if (arena == NULL) return NULL;
     


     crv = PK11_GetAttributes(arena, privKey->pkcs11Slot, privKey->pkcs11ID,
		privTemplate,count);
     if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	PORT_FreeArena(arena, PR_TRUE);
	return NULL;
     }

     
     count += pk11_AttrFlagsToAttributes(attrFlags, &privTemplate[count],
					&cktrue, &ckfalse);

     

     for (ap=attrs; extra_count; ap++, extra_count--) {
	pk11_SignedToUnsigned(ap);
     }

     
     rv = PK11_CreateNewObject(slot, CK_INVALID_SESSION, privTemplate, 
						count, token, &objectID);
     PORT_FreeArena(arena, PR_TRUE);
     if (rv != SECSuccess) {
	return NULL;
     }

     
     if (pubKey) {
	PK11_ImportPublicKey(slot, pubKey, token);
	if (pubKey->pkcs11Slot) {
	    PK11_FreeSlot(pubKey->pkcs11Slot);
	    pubKey->pkcs11Slot = NULL;
	    pubKey->pkcs11ID = CK_INVALID_HANDLE;
	}
     }

     
     return PK11_MakePrivKey(slot, privKey->keyType, !token, 
						objectID, privKey->wincx);
}

static SECKEYPrivateKey *
pk11_loadPrivKey(PK11SlotInfo *slot,SECKEYPrivateKey *privKey, 
		SECKEYPublicKey *pubKey, PRBool token, PRBool sensitive) 
{
    PK11AttrFlags attrFlags = 0;
    if (token) {
	attrFlags |= (PK11_ATTR_TOKEN | PK11_ATTR_PRIVATE);
    } else {
	attrFlags |= (PK11_ATTR_SESSION | PK11_ATTR_PUBLIC);
    }
    if (sensitive) {
	attrFlags |= PK11_ATTR_SENSITIVE;
    } else {
	attrFlags |= PK11_ATTR_INSENSITIVE;
    }
    return pk11_loadPrivKeyWithFlags(slot, privKey, pubKey, attrFlags);
}




SECKEYPrivateKey *
PK11_LoadPrivKey(PK11SlotInfo *slot,SECKEYPrivateKey *privKey, 
		SECKEYPublicKey *pubKey, PRBool token, PRBool sensitive) 
{
    return pk11_loadPrivKey(slot,privKey,pubKey,token,sensitive);
}





SECKEYPrivateKey *
PK11_GenerateKeyPairWithOpFlags(PK11SlotInfo *slot,CK_MECHANISM_TYPE type, 
   void *param, SECKEYPublicKey **pubKey, PK11AttrFlags attrFlags, 
   CK_FLAGS opFlags, CK_FLAGS opFlagsMask, void *wincx)
{
    


    CK_BBOOL ckfalse = CK_FALSE;
    CK_BBOOL cktrue = CK_TRUE;
    CK_ULONG modulusBits;
    CK_BYTE publicExponent[4];
    CK_ATTRIBUTE privTemplate[] = {
	{ CKA_SENSITIVE, NULL, 0},
	{ CKA_TOKEN,  NULL, 0},
	{ CKA_PRIVATE,  NULL, 0},
	{ CKA_DERIVE,  NULL, 0},
	{ CKA_UNWRAP,  NULL, 0},
	{ CKA_SIGN,  NULL, 0},
	{ CKA_DECRYPT,  NULL, 0},
	{ CKA_EXTRACTABLE, NULL, 0},
	{ CKA_MODIFIABLE,  NULL, 0},
    };
    CK_ATTRIBUTE rsaPubTemplate[] = {
	{ CKA_MODULUS_BITS, NULL, 0},
	{ CKA_PUBLIC_EXPONENT, NULL, 0},
	{ CKA_TOKEN,  NULL, 0},
	{ CKA_DERIVE,  NULL, 0},
	{ CKA_WRAP,  NULL, 0},
	{ CKA_VERIFY,  NULL, 0},
	{ CKA_VERIFY_RECOVER,  NULL, 0},
	{ CKA_ENCRYPT,  NULL, 0},
	{ CKA_MODIFIABLE,  NULL, 0},
    };
    CK_ATTRIBUTE dsaPubTemplate[] = {
	{ CKA_PRIME, NULL, 0 },
	{ CKA_SUBPRIME, NULL, 0 },
	{ CKA_BASE, NULL, 0 },
	{ CKA_TOKEN,  NULL, 0},
	{ CKA_DERIVE,  NULL, 0},
	{ CKA_WRAP,  NULL, 0},
	{ CKA_VERIFY,  NULL, 0},
	{ CKA_VERIFY_RECOVER,  NULL, 0},
	{ CKA_ENCRYPT,  NULL, 0},
	{ CKA_MODIFIABLE,  NULL, 0},
    };
    CK_ATTRIBUTE dhPubTemplate[] = {
      { CKA_PRIME, NULL, 0 }, 
      { CKA_BASE, NULL, 0 }, 
      { CKA_TOKEN,  NULL, 0},
      { CKA_DERIVE,  NULL, 0},
      { CKA_WRAP,  NULL, 0},
      { CKA_VERIFY,  NULL, 0},
      { CKA_VERIFY_RECOVER,  NULL, 0},
      { CKA_ENCRYPT,  NULL, 0},
      { CKA_MODIFIABLE,  NULL, 0},
    };
    CK_ATTRIBUTE ecPubTemplate[] = {
      { CKA_EC_PARAMS, NULL, 0 }, 
      { CKA_TOKEN,  NULL, 0},
      { CKA_DERIVE,  NULL, 0},
      { CKA_WRAP,  NULL, 0},
      { CKA_VERIFY,  NULL, 0},
      { CKA_VERIFY_RECOVER,  NULL, 0},
      { CKA_ENCRYPT,  NULL, 0},
      { CKA_MODIFIABLE,  NULL, 0},
    };
    SECKEYECParams * ecParams;

    
    CK_ATTRIBUTE *pubTemplate;
    int privCount = 0;
    int pubCount = 0;
    PK11RSAGenParams *rsaParams;
    SECKEYPQGParams *dsaParams;
    SECKEYDHParams * dhParams;
    CK_MECHANISM mechanism;
    CK_MECHANISM test_mech;
    CK_MECHANISM test_mech2;
    CK_SESSION_HANDLE session_handle;
    CK_RV crv;
    CK_OBJECT_HANDLE privID,pubID;
    SECKEYPrivateKey *privKey;
    KeyType keyType;
    PRBool restore;
    int peCount,i;
    CK_ATTRIBUTE *attrs;
    CK_ATTRIBUTE *privattrs;
    CK_ATTRIBUTE setTemplate;
    CK_MECHANISM_INFO mechanism_info;
    CK_OBJECT_CLASS keyClass;
    SECItem *cka_id;
    PRBool haslock = PR_FALSE;
    PRBool pubIsToken = PR_FALSE;
    PRBool token = ((attrFlags & PK11_ATTR_TOKEN) != 0);
    
    PK11AttrFlags pubKeyAttrFlags = attrFlags &
	(PK11_ATTR_TOKEN | PK11_ATTR_SESSION
	| PK11_ATTR_MODIFIABLE | PK11_ATTR_UNMODIFIABLE);

    if (pk11_BadAttrFlags(attrFlags)) {
	PORT_SetError( SEC_ERROR_INVALID_ARGS );
	return NULL;
    }

    if (!param) {
        PORT_SetError( SEC_ERROR_INVALID_ARGS );
        return NULL;
    }

    









    
    PORT_Assert ((opFlags & ~opFlagsMask) == 0);
    opFlags &= opFlagsMask;

    PORT_Assert(slot != NULL);
    if (slot == NULL) {
	PORT_SetError( SEC_ERROR_NO_MODULE);
	return NULL;
    }

    

    if (!PK11_DoesMechanism(slot,type)) {
	PK11SlotInfo *int_slot = PK11_GetInternalSlot();

	
	if (slot == int_slot) {
	    PK11_FreeSlot(int_slot);
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return NULL;
	}

	
	if (int_slot == NULL) {
	    PORT_SetError( SEC_ERROR_NO_MODULE );
	    return NULL;
	}

	
	privKey = PK11_GenerateKeyPair(int_slot,type, param, pubKey, PR_FALSE, 
							PR_FALSE, wincx);
	PK11_FreeSlot(int_slot);

	
	if (privKey != NULL) {
	    SECKEYPrivateKey *newPrivKey = pk11_loadPrivKeyWithFlags(slot,
						privKey,*pubKey,attrFlags);
	    SECKEY_DestroyPrivateKey(privKey);
	    if (newPrivKey == NULL) {
		SECKEY_DestroyPublicKey(*pubKey);
		*pubKey = NULL;
	    }
	    return newPrivKey;
	}
	return NULL;
   }


    mechanism.mechanism = type;
    mechanism.pParameter = NULL;
    mechanism.ulParameterLen = 0;
    test_mech.pParameter = NULL;
    test_mech.ulParameterLen = 0;
    test_mech2.mechanism = CKM_INVALID_MECHANISM;
    test_mech2.pParameter = NULL;
    test_mech2.ulParameterLen = 0;

    
    privattrs = privTemplate;
    privattrs += pk11_AttrFlagsToAttributes(attrFlags, privattrs,
						&cktrue, &ckfalse);

    
    switch (type) {
    case CKM_RSA_PKCS_KEY_PAIR_GEN:
    case CKM_RSA_X9_31_KEY_PAIR_GEN:
	rsaParams = (PK11RSAGenParams *)param;
	if (rsaParams->pe == 0) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return NULL;
	}
	modulusBits = rsaParams->keySizeInBits;
	peCount = 0;

	
	for (i=0; i < 4; i++) {
	    if (peCount || (rsaParams->pe & 
				((unsigned long)0xff000000L >> (i*8)))) {
		publicExponent[peCount] = 
				(CK_BYTE)((rsaParams->pe >> (3-i)*8) & 0xff);
		peCount++;
	    }
	}
	PORT_Assert(peCount != 0);
	attrs = rsaPubTemplate;
	PK11_SETATTRS(attrs, CKA_MODULUS_BITS, 
				&modulusBits, sizeof(modulusBits)); attrs++;
	PK11_SETATTRS(attrs, CKA_PUBLIC_EXPONENT, 
				publicExponent, peCount);attrs++;
	pubTemplate = rsaPubTemplate;
	keyType = rsaKey;
	test_mech.mechanism = CKM_RSA_PKCS;
	break;
    case CKM_DSA_KEY_PAIR_GEN:
	dsaParams = (SECKEYPQGParams *)param;
	attrs = dsaPubTemplate;
	PK11_SETATTRS(attrs, CKA_PRIME, dsaParams->prime.data,
				dsaParams->prime.len); attrs++;
	PK11_SETATTRS(attrs, CKA_SUBPRIME, dsaParams->subPrime.data,
					dsaParams->subPrime.len); attrs++;
	PK11_SETATTRS(attrs, CKA_BASE, dsaParams->base.data,
						dsaParams->base.len); attrs++;
	pubTemplate = dsaPubTemplate;
	keyType = dsaKey;
	test_mech.mechanism = CKM_DSA;
	break;
    case CKM_DH_PKCS_KEY_PAIR_GEN:
        dhParams = (SECKEYDHParams *)param;
        attrs = dhPubTemplate;
        PK11_SETATTRS(attrs, CKA_PRIME, dhParams->prime.data,
                      dhParams->prime.len);   attrs++;
        PK11_SETATTRS(attrs, CKA_BASE, dhParams->base.data,
                      dhParams->base.len);    attrs++;
        pubTemplate = dhPubTemplate;
        keyType = dhKey;
        test_mech.mechanism = CKM_DH_PKCS_DERIVE;
	break;
    case CKM_EC_KEY_PAIR_GEN:
        ecParams = (SECKEYECParams *)param;
        attrs = ecPubTemplate;
        PK11_SETATTRS(attrs, CKA_EC_PARAMS, ecParams->data, 
	              ecParams->len);   attrs++;
        pubTemplate = ecPubTemplate;
        keyType = ecKey;
	






	if ((opFlags & (CKF_SIGN|CKF_DERIVE)) == (CKF_SIGN|CKF_DERIVE)) {
	    
	    test_mech.mechanism = CKM_ECDH1_DERIVE;
	    test_mech2.mechanism = CKM_ECDSA;
	} else if (opFlags & CKF_SIGN) {
	    
	    test_mech.mechanism = CKM_ECDSA;
	} else if (opFlags & CKF_DERIVE) {
	    
	    test_mech.mechanism = CKM_ECDH1_DERIVE;
	} else {
	    
	    test_mech.mechanism = CKM_ECDH1_DERIVE;
	    test_mech2.mechanism = CKM_ECDSA;
	}
        break;
    default:
	PORT_SetError( SEC_ERROR_BAD_KEY );
	return NULL;
    }

    
    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_GetMechanismInfo(slot->slotID,
				test_mech.mechanism,&mechanism_info);
    




    if (test_mech2.mechanism != CKM_INVALID_MECHANISM) {
	CK_MECHANISM_INFO mechanism_info2;
	CK_RV crv2;

	if (crv != CKR_OK) {
	    

	    mechanism_info.flags = 0;
	}
	crv2 = PK11_GETTAB(slot)->C_GetMechanismInfo(slot->slotID,
				test_mech2.mechanism, &mechanism_info2);
	if (crv2 == CKR_OK) {
	    crv = CKR_OK; 
	    
	    mechanism_info.flags |= mechanism_info2.flags;
	}
    }
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    if ((crv != CKR_OK) || (mechanism_info.flags == 0)) {
	
	switch (test_mech.mechanism) {
	case CKM_RSA_PKCS:
	    mechanism_info.flags = (CKF_SIGN | CKF_DECRYPT | 
		CKF_WRAP | CKF_VERIFY_RECOVER | CKF_ENCRYPT | CKF_WRAP);
	    break;
	case CKM_DSA:
	    mechanism_info.flags = CKF_SIGN | CKF_VERIFY;
	    break;
	case CKM_DH_PKCS_DERIVE:
	    mechanism_info.flags = CKF_DERIVE;
	    break;
	case CKM_ECDH1_DERIVE:
	    mechanism_info.flags = CKF_DERIVE;
	    if (test_mech2.mechanism == CKM_ECDSA) {
		mechanism_info.flags |= CKF_SIGN | CKF_VERIFY;
	    }
	    break;
	case CKM_ECDSA:
	    mechanism_info.flags =  CKF_SIGN | CKF_VERIFY;
	    break;
	default:
	    break;
	}
    }
    
    mechanism_info.flags = (mechanism_info.flags & (~opFlagsMask)) | opFlags;
    
    attrs += pk11_AttrFlagsToAttributes(pubKeyAttrFlags, attrs,
						&cktrue, &ckfalse);
    PK11_SETATTRS(attrs, CKA_DERIVE, 
		mechanism_info.flags & CKF_DERIVE ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_WRAP, 
		mechanism_info.flags & CKF_WRAP ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_VERIFY, 
		mechanism_info.flags & CKF_VERIFY ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_VERIFY_RECOVER, 
		mechanism_info.flags & CKF_VERIFY_RECOVER ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); attrs++;
    PK11_SETATTRS(attrs, CKA_ENCRYPT, 
		mechanism_info.flags & CKF_ENCRYPT? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); attrs++;
    
    PK11_SETATTRS(privattrs, CKA_DERIVE, 
		mechanism_info.flags & CKF_DERIVE ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); privattrs++;
    PK11_SETATTRS(privattrs, CKA_UNWRAP, 
		mechanism_info.flags & CKF_UNWRAP ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); privattrs++;
    PK11_SETATTRS(privattrs, CKA_SIGN, 
		mechanism_info.flags & CKF_SIGN ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); privattrs++;
    PK11_SETATTRS(privattrs, CKA_DECRYPT, 
		mechanism_info.flags & CKF_DECRYPT ? &cktrue : &ckfalse,
					 sizeof(CK_BBOOL)); privattrs++;

    if (token) {
	session_handle = PK11_GetRWSession(slot);
	haslock = PK11_RWSessionHasLock(slot,session_handle);
	restore = PR_TRUE;
    } else {
	session_handle = slot->session;
	if (session_handle != CK_INVALID_SESSION)
	    PK11_EnterSlotMonitor(slot);
	restore = PR_FALSE;
	haslock = PR_TRUE;
    }

    if (session_handle == CK_INVALID_SESSION) {
    	PORT_SetError(SEC_ERROR_BAD_DATA);
	return NULL;
    }
    privCount = privattrs - privTemplate;
    pubCount = attrs - pubTemplate;
    crv = PK11_GETTAB(slot)->C_GenerateKeyPair(session_handle, &mechanism,
	pubTemplate,pubCount,privTemplate,privCount,&pubID,&privID);

    if (crv != CKR_OK) {
	if (restore)  {
	    PK11_RestoreROSession(slot,session_handle);
	} else PK11_ExitSlotMonitor(slot);
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }
    


    if (haslock) { PK11_ExitSlotMonitor(slot); }

    
    keyClass = PK11_ReadULongAttribute(slot,pubID,CKA_CLASS);
    if (keyClass != CKO_PUBLIC_KEY) {
	CK_OBJECT_HANDLE tmp = pubID;
	pubID = privID;
	privID = tmp;
    }

    *pubKey = PK11_ExtractPublicKey(slot, keyType, pubID);
    if (*pubKey == NULL) {
	if (restore)  {
	    

            if (haslock)  PK11_EnterSlotMonitor(slot); 
	    PK11_RestoreROSession(slot,session_handle);
	} 
	PK11_DestroyObject(slot,pubID);
	PK11_DestroyObject(slot,privID);
	return NULL;
    }

    
    cka_id = pk11_MakeIDFromPublicKey(*pubKey);
    pubIsToken = (PRBool)PK11_HasAttributeSet(slot,pubID, CKA_TOKEN,PR_FALSE);

    PK11_SETATTRS(&setTemplate, CKA_ID, cka_id->data, cka_id->len);

    if (haslock) { PK11_EnterSlotMonitor(slot); }
    crv = PK11_GETTAB(slot)->C_SetAttributeValue(session_handle, privID,
		&setTemplate, 1);
   
    if (crv == CKR_OK && pubIsToken) {
    	crv = PK11_GETTAB(slot)->C_SetAttributeValue(session_handle, pubID,
		&setTemplate, 1);
    }


    if (restore) {
	PK11_RestoreROSession(slot,session_handle);
    } else {
	PK11_ExitSlotMonitor(slot);
    }
    SECITEM_FreeItem(cka_id,PR_TRUE);


    if (crv != CKR_OK) {
	PK11_DestroyObject(slot,pubID);
	PK11_DestroyObject(slot,privID);
	PORT_SetError( PK11_MapError(crv) );
	*pubKey = NULL;
	return NULL;
    }

    privKey = PK11_MakePrivKey(slot,keyType,!token,privID,wincx);
    if (privKey == NULL) {
	SECKEY_DestroyPublicKey(*pubKey);
	PK11_DestroyObject(slot,privID);
	*pubKey = NULL;
	return NULL;
    }

    return privKey;
}

SECKEYPrivateKey *
PK11_GenerateKeyPairWithFlags(PK11SlotInfo *slot,CK_MECHANISM_TYPE type, 
   void *param, SECKEYPublicKey **pubKey, PK11AttrFlags attrFlags, void *wincx)
{
    return PK11_GenerateKeyPairWithOpFlags(slot,type,param,pubKey,attrFlags,
					   0, 0, wincx);
}




SECKEYPrivateKey *
PK11_GenerateKeyPair(PK11SlotInfo *slot,CK_MECHANISM_TYPE type, 
   void *param, SECKEYPublicKey **pubKey, PRBool token, 
					PRBool sensitive, void *wincx)
{
    PK11AttrFlags attrFlags = 0;

    if (token) {
	attrFlags |= PK11_ATTR_TOKEN;
    } else {
	attrFlags |= PK11_ATTR_SESSION;
    }
    if (sensitive) {
	attrFlags |= (PK11_ATTR_SENSITIVE | PK11_ATTR_PRIVATE);
    } else {
	attrFlags |= (PK11_ATTR_INSENSITIVE | PK11_ATTR_PUBLIC);
    }
    return PK11_GenerateKeyPairWithFlags(slot, type, param, pubKey,
						attrFlags, wincx);
}


SECKEYPublicKey *
PK11_MakeKEAPubKey(unsigned char *keyData,int length)
{
    SECKEYPublicKey *pubk;
    SECItem pkData;
    SECStatus rv;
    PLArenaPool *arena;

    pkData.data = keyData;
    pkData.len = length;

    arena = PORT_NewArena (DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	return NULL;

    pubk = (SECKEYPublicKey *) PORT_ArenaZAlloc(arena, sizeof(SECKEYPublicKey));
    if (pubk == NULL) {
	PORT_FreeArena (arena, PR_FALSE);
	return NULL;
    }

    pubk->arena = arena;
    pubk->pkcs11Slot = 0;
    pubk->pkcs11ID = CK_INVALID_HANDLE;
    pubk->keyType = fortezzaKey;
    rv = SECITEM_CopyItem(arena, &pubk->u.fortezza.KEAKey, &pkData);
    if (rv != SECSuccess) {
	PORT_FreeArena (arena, PR_FALSE);
	return NULL;
    }
    return pubk;
}







SECStatus 
PK11_ImportEncryptedPrivateKeyInfo(PK11SlotInfo *slot,
			SECKEYEncryptedPrivateKeyInfo *epki, SECItem *pwitem,
			SECItem *nickname, SECItem *publicValue, PRBool isPerm,
			PRBool isPrivate, KeyType keyType, 
			unsigned int keyUsage, void *wincx)
{
    if (!isPerm) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    return PK11_ImportEncryptedPrivateKeyInfoAndReturnKey(slot, epki,
		pwitem, nickname, publicValue, isPerm, isPrivate, keyType,
		keyUsage, NULL, wincx);
}

SECStatus
PK11_ImportEncryptedPrivateKeyInfoAndReturnKey(PK11SlotInfo *slot,
			SECKEYEncryptedPrivateKeyInfo *epki, SECItem *pwitem,
			SECItem *nickname, SECItem *publicValue, PRBool isPerm,
			PRBool isPrivate, KeyType keyType,
			unsigned int keyUsage, SECKEYPrivateKey **privk,
			void *wincx)
{
    CK_MECHANISM_TYPE pbeMechType;
    SECItem *crypto_param = NULL;
    PK11SymKey *key = NULL;
    SECStatus rv = SECSuccess;
    CK_MECHANISM_TYPE cryptoMechType;
    SECKEYPrivateKey *privKey = NULL;
    PRBool faulty3DES = PR_FALSE;
    int usageCount = 0;
    CK_KEY_TYPE key_type;
    CK_ATTRIBUTE_TYPE *usage = NULL;
    CK_ATTRIBUTE_TYPE rsaUsage[] = {
		 CKA_UNWRAP, CKA_DECRYPT, CKA_SIGN, CKA_SIGN_RECOVER };
    CK_ATTRIBUTE_TYPE dsaUsage[] = { CKA_SIGN };
    CK_ATTRIBUTE_TYPE dhUsage[] = { CKA_DERIVE };
    CK_ATTRIBUTE_TYPE ecUsage[] = { CKA_SIGN, CKA_DERIVE };
    if((epki == NULL) || (pwitem == NULL))
	return SECFailure;

    pbeMechType = PK11_AlgtagToMechanism(SECOID_FindOIDTag(
					&epki->algorithm.algorithm));

    switch (keyType) {
    default:
    case rsaKey:
	key_type = CKK_RSA;
	switch  (keyUsage & (KU_KEY_ENCIPHERMENT|KU_DIGITAL_SIGNATURE)) {
	case KU_KEY_ENCIPHERMENT:
	    usage = rsaUsage;
	    usageCount = 2;
	    break;
	case KU_DIGITAL_SIGNATURE:
	    usage = &rsaUsage[2];
	    usageCount = 2;
	    break;
	case KU_KEY_ENCIPHERMENT|KU_DIGITAL_SIGNATURE:
	case 0: 
	    usage = rsaUsage;
	    usageCount = 4;
	    break;
	}
        break;
    case dhKey:
	key_type = CKK_DH;
	usage = dhUsage;
	usageCount = sizeof(dhUsage)/sizeof(dhUsage[0]);
	break;
    case dsaKey:
	key_type = CKK_DSA;
	usage = dsaUsage;
	usageCount = sizeof(dsaUsage)/sizeof(dsaUsage[0]);
	break;
    case ecKey:
	key_type = CKK_EC;
	switch  (keyUsage & (KU_DIGITAL_SIGNATURE|KU_KEY_AGREEMENT)) {
	case KU_DIGITAL_SIGNATURE:
	    usage = ecUsage;
	    usageCount = 1;
	    break;
	case KU_KEY_AGREEMENT:
	    usage = &ecUsage[1];
	    usageCount = 1;
	    break;
	case KU_DIGITAL_SIGNATURE|KU_KEY_AGREEMENT:
	default: 
	    usage = ecUsage;
	    usageCount = 2;
	    break;
	}
	break;	
    }

try_faulty_3des:

    key = PK11_PBEKeyGen(slot, &epki->algorithm, pwitem, faulty3DES, wincx);
    if (key == NULL) {
	rv = SECFailure;
	goto done;
    }
    cryptoMechType = pk11_GetPBECryptoMechanism(&epki->algorithm,
					 &crypto_param, pwitem, faulty3DES);
    if (cryptoMechType == CKM_INVALID_MECHANISM) {
	rv = SECFailure;
	goto done;
    }


    cryptoMechType = PK11_GetPadMechanism(cryptoMechType);

    PORT_Assert(usage != NULL);
    PORT_Assert(usageCount != 0);
    privKey = PK11_UnwrapPrivKey(slot, key, cryptoMechType, 
				 crypto_param, &epki->encryptedData, 
				 nickname, publicValue, isPerm, isPrivate,
				 key_type, usage, usageCount, wincx);
    if(privKey) {
	if (privk) {
	    *privk = privKey;
	} else {
	    SECKEY_DestroyPrivateKey(privKey);
	}
	privKey = NULL;
	rv = SECSuccess;
	goto done;
    }

    




 
    if((pbeMechType == CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC) && (!faulty3DES)) {
	

	PK11_FreeSymKey(key);
	key = NULL;

	if(crypto_param) {
	    SECITEM_ZfreeItem(crypto_param, PR_TRUE);
	    crypto_param = NULL;
	}

	faulty3DES = PR_TRUE;
	goto try_faulty_3des;
    }

    
    rv = SECFailure;

done:
    if(crypto_param != NULL) {
	SECITEM_ZfreeItem(crypto_param, PR_TRUE);
    }

    if(key != NULL) {
    	PK11_FreeSymKey(key);
    }

    return rv;
}

SECKEYPrivateKeyInfo *
PK11_ExportPrivateKeyInfo(CERTCertificate *cert, void *wincx)
{
    SECKEYPrivateKeyInfo *pki = NULL;
    SECKEYPrivateKey     *pk  = PK11_FindKeyByAnyCert(cert, wincx);
    if (pk != NULL) {
	pki = PK11_ExportPrivKeyInfo(pk, wincx);
	SECKEY_DestroyPrivateKey(pk);
    }
    return pki;
}

SECKEYEncryptedPrivateKeyInfo * 
PK11_ExportEncryptedPrivKeyInfo(
   PK11SlotInfo     *slot,      
   SECOidTag         algTag,    
   SECItem          *pwitem,    
   SECKEYPrivateKey *pk,        
   int               iteration, 
   void             *wincx)     
{
    SECKEYEncryptedPrivateKeyInfo *epki      = NULL;
    PLArenaPool                   *arena     = NULL;
    SECAlgorithmID                *algid;
    SECOidTag			  pbeAlgTag = SEC_OID_UNKNOWN;
    SECItem                       *crypto_param = NULL;
    PK11SymKey                    *key       = NULL;
    SECKEYPrivateKey		  *tmpPK = NULL;
    SECStatus                      rv        = SECSuccess;
    CK_RV                          crv;
    CK_ULONG                       encBufLen;
    CK_MECHANISM_TYPE              pbeMechType;
    CK_MECHANISM_TYPE              cryptoMechType;
    CK_MECHANISM                   cryptoMech;

    if (!pwitem || !pk) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }

    algid = sec_pkcs5CreateAlgorithmID(algTag, SEC_OID_UNKNOWN, SEC_OID_UNKNOWN,
				&pbeAlgTag, 0, NULL, iteration);
    if (algid == NULL) {
	return NULL;
    }

    arena = PORT_NewArena(2048);
    if (arena)
	epki = PORT_ArenaZNew(arena, SECKEYEncryptedPrivateKeyInfo);
    if(epki == NULL) {
	rv = SECFailure;
	goto loser;
    }
    epki->arena = arena;


    
    if (!slot) {
	slot = pk->pkcs11Slot;
    }

    


    pbeMechType = PK11_AlgtagToMechanism(pbeAlgTag);
    if (slot != pk->pkcs11Slot) {
	if (PK11_DoesMechanism(pk->pkcs11Slot,pbeMechType)) {
	    slot = pk->pkcs11Slot;
	}
    }
    key = PK11_PBEKeyGen(slot, algid, pwitem, PR_FALSE, wincx);
    if (key == NULL) {
	rv = SECFailure;
	goto loser;
    }

    cryptoMechType = PK11_GetPBECryptoMechanism(algid, &crypto_param, pwitem);
    if (cryptoMechType == CKM_INVALID_MECHANISM) {
	rv = SECFailure;
	goto loser;
    }

    cryptoMech.mechanism = PK11_GetPadMechanism(cryptoMechType);
    cryptoMech.pParameter = crypto_param ? crypto_param->data : NULL;
    cryptoMech.ulParameterLen = crypto_param ? crypto_param->len : 0;

    
    if (key->slot != pk->pkcs11Slot) {
	PK11SymKey *newkey = pk11_CopyToSlot(pk->pkcs11Slot,
						key->type, CKA_WRAP, key);
	if (newkey == NULL) {
            

	    tmpPK = pk11_loadPrivKey(key->slot, pk, NULL, PR_FALSE, PR_TRUE);
	    if (tmpPK == NULL) {
		rv = SECFailure;
		goto loser;
	    }
	    pk = tmpPK;
	} else {
	    
	    PK11_FreeSymKey(key);
	    key = newkey;
	}
    }
    	
    




    encBufLen = 0;
    PK11_EnterSlotMonitor(pk->pkcs11Slot);
    crv = PK11_GETTAB(pk->pkcs11Slot)->C_WrapKey(pk->pkcs11Slot->session, 
    		&cryptoMech, key->objectID, pk->pkcs11ID, NULL, 
		&encBufLen); 
    PK11_ExitSlotMonitor(pk->pkcs11Slot);
    if (crv != CKR_OK) {
	rv = SECFailure;
	goto loser;
    }
    epki->encryptedData.data = PORT_ArenaAlloc(arena, encBufLen);
    if (!epki->encryptedData.data) {
	rv = SECFailure;
	goto loser;
    }
    PK11_EnterSlotMonitor(pk->pkcs11Slot);
    crv = PK11_GETTAB(pk->pkcs11Slot)->C_WrapKey(pk->pkcs11Slot->session, 
    		&cryptoMech, key->objectID, pk->pkcs11ID, 
		epki->encryptedData.data, &encBufLen); 
    PK11_ExitSlotMonitor(pk->pkcs11Slot);
    epki->encryptedData.len = (unsigned int) encBufLen;
    if(crv != CKR_OK) {
	rv = SECFailure;
	goto loser;
    }

    if(!epki->encryptedData.len) {
	rv = SECFailure;
	goto loser;
    }

    rv = SECOID_CopyAlgorithmID(arena, &epki->algorithm, algid);

loser:
    if(crypto_param != NULL) {
	SECITEM_ZfreeItem(crypto_param, PR_TRUE);
	crypto_param = NULL;
    }

    if(key != NULL) {
    	PK11_FreeSymKey(key);
    }
    if (tmpPK != NULL) {
	SECKEY_DestroyPrivateKey(tmpPK);
    }
    SECOID_DestroyAlgorithmID(algid, PR_TRUE);

    if(rv == SECFailure) {
	if(arena != NULL) {
	    PORT_FreeArena(arena, PR_TRUE);
	}
	epki = NULL;
    }

    return epki;
}

SECKEYEncryptedPrivateKeyInfo * 
PK11_ExportEncryptedPrivateKeyInfo(
   PK11SlotInfo    *slot,      
   SECOidTag        algTag,    
   SECItem         *pwitem,    
   CERTCertificate *cert,      
   int              iteration, 
   void            *wincx)     
{
    SECKEYEncryptedPrivateKeyInfo *epki = NULL;
    SECKEYPrivateKey              *pk   = PK11_FindKeyByAnyCert(cert, wincx);
    if (pk != NULL) {
	epki = PK11_ExportEncryptedPrivKeyInfo(slot, algTag, pwitem, pk, 
	                                       iteration, wincx);
	SECKEY_DestroyPrivateKey(pk);
    }
    return epki;
}

SECItem*
PK11_DEREncodePublicKey(const SECKEYPublicKey *pubk)
{
    return SECKEY_EncodeDERSubjectPublicKeyInfo(pubk);
}

char *
PK11_GetPrivateKeyNickname(SECKEYPrivateKey *privKey)
{
    return PK11_GetObjectNickname(privKey->pkcs11Slot,privKey->pkcs11ID);
}

char *
PK11_GetPublicKeyNickname(SECKEYPublicKey *pubKey)
{
    return PK11_GetObjectNickname(pubKey->pkcs11Slot,pubKey->pkcs11ID);
}

SECStatus
PK11_SetPrivateKeyNickname(SECKEYPrivateKey *privKey, const char *nickname)
{
    return PK11_SetObjectNickname(privKey->pkcs11Slot,
					privKey->pkcs11ID,nickname);
}

SECStatus
PK11_SetPublicKeyNickname(SECKEYPublicKey *pubKey, const char *nickname)
{
    return PK11_SetObjectNickname(pubKey->pkcs11Slot,
					pubKey->pkcs11ID,nickname);
}

SECKEYPQGParams *
PK11_GetPQGParamsFromPrivateKey(SECKEYPrivateKey *privKey)
{
    CK_ATTRIBUTE pTemplate[] = {
	{ CKA_PRIME, NULL, 0 },
	{ CKA_SUBPRIME, NULL, 0 },
	{ CKA_BASE, NULL, 0 },
    };
    int pTemplateLen = sizeof(pTemplate)/sizeof(pTemplate[0]);
    PLArenaPool *arena = NULL;
    SECKEYPQGParams *params;
    CK_RV crv;


    arena = PORT_NewArena(2048);
    if (arena == NULL) {
	goto loser;
    }
    params=(SECKEYPQGParams *)PORT_ArenaZAlloc(arena,sizeof(SECKEYPQGParams));
    if (params == NULL) {
	goto loser;
    }

    crv = PK11_GetAttributes(arena, privKey->pkcs11Slot, privKey->pkcs11ID, 
						pTemplate, pTemplateLen);
    if (crv != CKR_OK) {
        PORT_SetError( PK11_MapError(crv) );
	goto loser;
    }

    params->arena = arena;
    params->prime.data = pTemplate[0].pValue;
    params->prime.len = pTemplate[0].ulValueLen;
    params->subPrime.data = pTemplate[1].pValue;
    params->subPrime.len = pTemplate[1].ulValueLen;
    params->base.data = pTemplate[2].pValue;
    params->base.len = pTemplate[2].ulValueLen;

    return params;

loser:
    if (arena != NULL) {
	PORT_FreeArena(arena,PR_FALSE);
    }
    return NULL;
}

SECKEYPrivateKey*
PK11_CopyTokenPrivKeyToSessionPrivKey(PK11SlotInfo *destSlot,
				      SECKEYPrivateKey *privKey)
{
    CK_RV             crv;
    CK_OBJECT_HANDLE  newKeyID;

    static const CK_BBOOL     ckfalse = CK_FALSE;
    static const CK_ATTRIBUTE template[1] = { 
       { CKA_TOKEN, (CK_BBOOL *)&ckfalse, sizeof ckfalse }
    };

    if (destSlot && destSlot != privKey->pkcs11Slot) {
	SECKEYPrivateKey *newKey =
	       pk11_loadPrivKey(destSlot, 
				privKey, 
			        NULL,     
			        PR_FALSE, 
			        PR_FALSE);
	if (newKey)
	    return newKey;
    }
    destSlot = privKey->pkcs11Slot;
    PK11_Authenticate(destSlot, PR_TRUE, privKey->wincx);
    PK11_EnterSlotMonitor(destSlot);
    crv = PK11_GETTAB(destSlot)->C_CopyObject(	destSlot->session, 
						privKey->pkcs11ID,
						(CK_ATTRIBUTE *)template, 
						1, &newKeyID);
    PK11_ExitSlotMonitor(destSlot);

    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	return NULL;
    }

    return PK11_MakePrivKey(destSlot, privKey->keyType, PR_TRUE , 
			    newKeyID, privKey->wincx);
}

SECKEYPrivateKey*
PK11_ConvertSessionPrivKeyToTokenPrivKey(SECKEYPrivateKey *privk, void* wincx)
{
    PK11SlotInfo* slot = privk->pkcs11Slot;
    CK_ATTRIBUTE template[1];
    CK_ATTRIBUTE *attrs = template;
    CK_BBOOL cktrue = CK_TRUE;
    CK_RV crv;
    CK_OBJECT_HANDLE newKeyID;
    CK_SESSION_HANDLE rwsession;

    PK11_SETATTRS(attrs, CKA_TOKEN, &cktrue, sizeof(cktrue)); attrs++;

    PK11_Authenticate(slot, PR_TRUE, wincx);
    rwsession = PK11_GetRWSession(slot);
    if (rwsession == CK_INVALID_SESSION) {
    	PORT_SetError(SEC_ERROR_BAD_DATA);
	return NULL;
    }
    crv = PK11_GETTAB(slot)->C_CopyObject(rwsession, privk->pkcs11ID,
        template, 1, &newKeyID);
    PK11_RestoreROSession(slot, rwsession);

    if (crv != CKR_OK) {
        PORT_SetError( PK11_MapError(crv) );
        return NULL;
    }

    return PK11_MakePrivKey(slot, nullKey , PR_FALSE ,
        newKeyID, NULL );
}





SECStatus
PK11_DeleteTokenPrivateKey(SECKEYPrivateKey *privKey, PRBool force)
{
    CERTCertificate *cert=PK11_GetCertFromPrivateKey(privKey);
    SECStatus rv = SECWouldBlock;

    if (!cert || force) {
	
	rv = PK11_DestroyTokenObject(privKey->pkcs11Slot,privKey->pkcs11ID);
    }
    if (cert) {
	CERT_DestroyCertificate(cert);
    }
    SECKEY_DestroyPrivateKey(privKey);
    return rv;
}





SECStatus
PK11_DeleteTokenPublicKey(SECKEYPublicKey *pubKey)
{
    
    if (pubKey->pkcs11Slot == NULL) {
	return SECFailure;
    }
    PK11_DestroyTokenObject(pubKey->pkcs11Slot,pubKey->pkcs11ID);
    SECKEY_DestroyPublicKey(pubKey);
    return SECSuccess;
}




typedef struct pk11KeyCallbackStr {
	SECStatus (* callback)(SECKEYPrivateKey *,void *);
	void *callbackArg;
	void *wincx;
} pk11KeyCallback;




SECStatus
pk11_DoKeys(PK11SlotInfo *slot, CK_OBJECT_HANDLE keyHandle, void *arg)
{
    SECStatus rv = SECSuccess;
    SECKEYPrivateKey *privKey;
    pk11KeyCallback *keycb = (pk11KeyCallback *) arg;
    if (!arg) {
        return SECFailure;
    }

    privKey = PK11_MakePrivKey(slot,nullKey,PR_TRUE,keyHandle,keycb->wincx);

    if (privKey == NULL) {
	return SECFailure;
    }

    if (keycb->callback) {
	rv = (*keycb->callback)(privKey,keycb->callbackArg);
    }

    SECKEY_DestroyPrivateKey(privKey);	    
    return rv;
}














SECStatus
PK11_TraversePrivateKeysInSlot( PK11SlotInfo *slot,
    SECStatus(* callback)(SECKEYPrivateKey*, void*), void *arg)
{
    pk11KeyCallback perKeyCB;
    pk11TraverseSlot perObjectCB;
    CK_OBJECT_CLASS privkClass = CKO_PRIVATE_KEY;
    CK_BBOOL ckTrue = CK_TRUE;
    CK_ATTRIBUTE theTemplate[2];
    int templateSize = 2;

    theTemplate[0].type = CKA_CLASS;
    theTemplate[0].pValue = &privkClass;
    theTemplate[0].ulValueLen = sizeof(privkClass);
    theTemplate[1].type = CKA_TOKEN;
    theTemplate[1].pValue = &ckTrue;
    theTemplate[1].ulValueLen = sizeof(ckTrue);

    if(slot==NULL) {
        return SECSuccess;
    }

    perObjectCB.callback = pk11_DoKeys;
    perObjectCB.callbackArg = &perKeyCB;
    perObjectCB.findTemplate = theTemplate;
    perObjectCB.templateCount = templateSize;
    perKeyCB.callback = callback;
    perKeyCB.callbackArg = arg;
    perKeyCB.wincx = NULL;

    return PK11_TraverseSlot(slot, &perObjectCB);
}




CK_OBJECT_HANDLE
pk11_FindPrivateKeyFromCertID(PK11SlotInfo *slot, SECItem *keyID)
{
    CK_OBJECT_CLASS privKey = CKO_PRIVATE_KEY;
    CK_ATTRIBUTE theTemplate[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_CLASS, NULL, 0 },
    };
    
    int tsize = sizeof(theTemplate)/sizeof(theTemplate[0]);
    CK_ATTRIBUTE *attrs = theTemplate;

    PK11_SETATTRS(attrs, CKA_ID, keyID->data, keyID->len ); attrs++;
    PK11_SETATTRS(attrs, CKA_CLASS, &privKey, sizeof(privKey));

    return pk11_FindObjectByTemplate(slot,theTemplate,tsize);
} 


SECKEYPrivateKey *
PK11_FindKeyByKeyID(PK11SlotInfo *slot, SECItem *keyID, void *wincx)
{
    CK_OBJECT_HANDLE keyHandle;
    SECKEYPrivateKey *privKey;

    keyHandle = pk11_FindPrivateKeyFromCertID(slot, keyID);
    if (keyHandle == CK_INVALID_HANDLE) { 
	return NULL;
    }
    privKey =  PK11_MakePrivKey(slot, nullKey, PR_TRUE, keyHandle, wincx);
    return privKey;
}






SECItem *
PK11_MakeIDFromPubKey(SECItem *pubKeyData) 
{
    PK11Context *context;
    SECItem *certCKA_ID;
    SECStatus rv;

    if (pubKeyData->len <= SHA1_LENGTH) {
	




	return SECITEM_DupItem(pubKeyData);
    }

    context = PK11_CreateDigestContext(SEC_OID_SHA1);
    if (context == NULL) {
	return NULL;
    }

    rv = PK11_DigestBegin(context);
    if (rv == SECSuccess) {
    	rv = PK11_DigestOp(context,pubKeyData->data,pubKeyData->len);
    }
    if (rv != SECSuccess) {
	PK11_DestroyContext(context,PR_TRUE);
	return NULL;
    }

    certCKA_ID = (SECItem *)PORT_Alloc(sizeof(SECItem));
    if (certCKA_ID == NULL) {
	PK11_DestroyContext(context,PR_TRUE);
	return NULL;
    }

    certCKA_ID->len = SHA1_LENGTH;
    certCKA_ID->data = (unsigned char*)PORT_Alloc(certCKA_ID->len);
    if (certCKA_ID->data == NULL) {
	PORT_Free(certCKA_ID);
	PK11_DestroyContext(context,PR_TRUE);
        return NULL;
    }

    rv = PK11_DigestFinal(context,certCKA_ID->data,&certCKA_ID->len,
								SHA1_LENGTH);
    PK11_DestroyContext(context,PR_TRUE);
    if (rv != SECSuccess) {
    	SECITEM_FreeItem(certCKA_ID,PR_TRUE);
	return NULL;
    }

    return certCKA_ID;
}






SECItem *
PK11_GetLowLevelKeyIDForPrivateKey(SECKEYPrivateKey *privKey)
{
    return pk11_GetLowLevelKeyFromHandle(privKey->pkcs11Slot,privKey->pkcs11ID);
}

static SECStatus
privateKeyListCallback(SECKEYPrivateKey *key, void *arg)
{
    SECKEYPrivateKeyList *list = (SECKEYPrivateKeyList*)arg;
    return SECKEY_AddPrivateKeyToListTail(list, SECKEY_CopyPrivateKey(key));
}

SECKEYPrivateKeyList*
PK11_ListPrivateKeysInSlot(PK11SlotInfo *slot)
{
    SECStatus status;
    SECKEYPrivateKeyList *keys;

    keys = SECKEY_NewPrivateKeyList();
    if(keys == NULL) return NULL;

    status = PK11_TraversePrivateKeysInSlot(slot, privateKeyListCallback,
		(void*)keys);

    if( status != SECSuccess ) {
	SECKEY_DestroyPrivateKeyList(keys);
	keys = NULL;
    }

    return keys;
}

SECKEYPublicKeyList*
PK11_ListPublicKeysInSlot(PK11SlotInfo *slot, char *nickname)
{
    CK_ATTRIBUTE findTemp[4];
    CK_ATTRIBUTE *attrs;
    CK_BBOOL ckTrue = CK_TRUE;
    CK_OBJECT_CLASS keyclass = CKO_PUBLIC_KEY;
    int tsize = 0;
    int objCount = 0;
    CK_OBJECT_HANDLE *key_ids;
    SECKEYPublicKeyList *keys;
    int i,len;


    attrs = findTemp;
    PK11_SETATTRS(attrs, CKA_CLASS, &keyclass, sizeof(keyclass)); attrs++;
    PK11_SETATTRS(attrs, CKA_TOKEN, &ckTrue, sizeof(ckTrue)); attrs++;
    if (nickname) {
	len = PORT_Strlen(nickname);
	PK11_SETATTRS(attrs, CKA_LABEL, nickname, len); attrs++;
    }
    tsize = attrs - findTemp;
    PORT_Assert(tsize <= sizeof(findTemp)/sizeof(CK_ATTRIBUTE));

    key_ids = pk11_FindObjectsByTemplate(slot,findTemp,tsize,&objCount);
    if (key_ids == NULL) {
	return NULL;
    }
    keys = SECKEY_NewPublicKeyList();
    if (keys == NULL) {
	PORT_Free(key_ids);
	return NULL;
    }

    for (i=0; i < objCount ; i++) {
	SECKEYPublicKey *pubKey = 
				PK11_ExtractPublicKey(slot,nullKey,key_ids[i]);
	if (pubKey) {
	    SECKEY_AddPublicKeyToListTail(keys, pubKey);
	}
   }

   PORT_Free(key_ids);
   return keys;
}

SECKEYPrivateKeyList*
PK11_ListPrivKeysInSlot(PK11SlotInfo *slot, char *nickname, void *wincx)
{
    CK_ATTRIBUTE findTemp[4];
    CK_ATTRIBUTE *attrs;
    CK_BBOOL ckTrue = CK_TRUE;
    CK_OBJECT_CLASS keyclass = CKO_PRIVATE_KEY;
    int tsize = 0;
    int objCount = 0;
    CK_OBJECT_HANDLE *key_ids;
    SECKEYPrivateKeyList *keys;
    int i,len;


    attrs = findTemp;
    PK11_SETATTRS(attrs, CKA_CLASS, &keyclass, sizeof(keyclass)); attrs++;
    PK11_SETATTRS(attrs, CKA_TOKEN, &ckTrue, sizeof(ckTrue)); attrs++;
    if (nickname) {
	len = PORT_Strlen(nickname);
	PK11_SETATTRS(attrs, CKA_LABEL, nickname, len); attrs++;
    }
    tsize = attrs - findTemp;
    PORT_Assert(tsize <= sizeof(findTemp)/sizeof(CK_ATTRIBUTE));

    key_ids = pk11_FindObjectsByTemplate(slot,findTemp,tsize,&objCount);
    if (key_ids == NULL) {
	return NULL;
    }
    keys = SECKEY_NewPrivateKeyList();
    if (keys == NULL) {
	PORT_Free(key_ids);
	return NULL;
    }

    for (i=0; i < objCount ; i++) {
	SECKEYPrivateKey *privKey = 
		PK11_MakePrivKey(slot,nullKey,PR_TRUE,key_ids[i],wincx);
	SECKEY_AddPrivateKeyToListTail(keys, privKey);
   }

   PORT_Free(key_ids);
   return keys;
}

