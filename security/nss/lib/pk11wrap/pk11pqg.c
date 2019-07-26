






#include "pk11func.h"
#include "secmod.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pkcs11t.h"
#include "pk11pqg.h"
#include "secerr.h"



























extern SECStatus
PK11_PQG_ParamGenV2(unsigned int L, unsigned int N,
	 unsigned int seedBytes, PQGParams **pParams, PQGVerify **pVfy)
{
    PK11SlotInfo *slot = NULL;
    CK_ATTRIBUTE genTemplate[5];
    CK_ATTRIBUTE *attrs = genTemplate;
    int count = sizeof(genTemplate)/sizeof(genTemplate[0]);
    CK_MECHANISM mechanism;
    CK_OBJECT_HANDLE objectID = CK_INVALID_HANDLE;
    CK_RV crv;
    CK_ATTRIBUTE pTemplate[] = {
	{ CKA_PRIME, NULL, 0 },
	{ CKA_SUBPRIME, NULL, 0 },
	{ CKA_BASE, NULL, 0 },
    };
    CK_ATTRIBUTE vTemplate[] = {
	{ CKA_NETSCAPE_PQG_COUNTER, NULL, 0 },
	{ CKA_NETSCAPE_PQG_SEED, NULL, 0 },
	{ CKA_NETSCAPE_PQG_H, NULL, 0 },
    };
    CK_ULONG primeBits = L;
    CK_ULONG subPrimeBits = N;
    int pTemplateCount = sizeof(pTemplate)/sizeof(pTemplate[0]);
    int vTemplateCount = sizeof(vTemplate)/sizeof(vTemplate[0]);
    PRArenaPool *parena = NULL;
    PRArenaPool *varena = NULL;
    PQGParams *params = NULL;
    PQGVerify *verify = NULL;
    CK_ULONG seedBits = seedBytes*8;

    *pParams = NULL;
    *pVfy =  NULL;

    if (primeBits == (CK_ULONG)-1) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	goto loser;
    }
    PK11_SETATTRS(attrs, CKA_PRIME_BITS,&primeBits,sizeof(primeBits)); attrs++;
    if (subPrimeBits != 0) {
    	PK11_SETATTRS(attrs, CKA_SUB_PRIME_BITS, 
				&subPrimeBits, sizeof(subPrimeBits)); attrs++;
    }
    if (seedBits != 0) {
    	PK11_SETATTRS(attrs, CKA_NETSCAPE_PQG_SEED_BITS, 
					&seedBits, sizeof(seedBits)); attrs++;
    }
    count = attrs - genTemplate;
    PR_ASSERT(count <= sizeof(genTemplate)/sizeof(CK_ATTRIBUTE));

    slot = PK11_GetInternalSlot();
    if (slot == NULL) {
	
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	goto loser;
    }

    
    if (primeBits > 1024) {
	CK_MECHANISM_INFO mechanism_info;

	if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
	crv = PK11_GETTAB(slot)->C_GetMechanismInfo(slot->slotID,
			CKM_DSA_PARAMETER_GEN, &mechanism_info);
	if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	


	if ((crv != CKR_OK) || (mechanism_info.ulMaxKeySize < primeBits)) {
	    PK11_FreeSlot(slot);
	    slot = PK11_GetBestSlotWithAttributes(CKM_DSA_PARAMETER_GEN, 0,
						primeBits, NULL);
	    if (slot == NULL) {
		PORT_SetError(SEC_ERROR_NO_TOKEN); 
		goto loser;
	    }
	    

	    if (seedBits) {
		attrs--;
	    }
	}
    }

    
    mechanism.mechanism = CKM_DSA_PARAMETER_GEN;
    mechanism.pParameter = NULL;
    mechanism.ulParameterLen = 0;

    PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_GenerateKey(slot->session,
			 &mechanism, genTemplate, count, &objectID);
    PK11_ExitSlotMonitor(slot);

    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	goto loser;
    }

    parena = PORT_NewArena(60);
    if (!parena) {
	goto loser;
    }        

    crv = PK11_GetAttributes(parena, slot, objectID, pTemplate, pTemplateCount);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	goto loser;
    }


    params = (PQGParams *)PORT_ArenaAlloc(parena,sizeof(PQGParams));
    if (params == NULL) {
	goto loser;
    }

    
    params->arena = parena;
    params->prime.type = siUnsignedInteger;
    params->prime.data = pTemplate[0].pValue;
    params->prime.len = pTemplate[0].ulValueLen;
    params->subPrime.type = siUnsignedInteger;
    params->subPrime.data = pTemplate[1].pValue;
    params->subPrime.len = pTemplate[1].ulValueLen;
    params->base.type = siUnsignedInteger;
    params->base.data = pTemplate[2].pValue;
    params->base.len = pTemplate[2].ulValueLen;


    varena = PORT_NewArena(60);
    if (!varena) {
	goto loser;
    }        

    crv = PK11_GetAttributes(varena, slot, objectID, vTemplate, vTemplateCount);
    if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	goto loser;
    }


    verify = (PQGVerify *)PORT_ArenaAlloc(varena,sizeof(PQGVerify));
    if (verify == NULL) {
	goto loser;
    }
    
    verify->arena = varena;
    verify->counter = (unsigned int)(*(CK_ULONG*)vTemplate[0].pValue);
    verify->seed.type = siUnsignedInteger;
    verify->seed.data = vTemplate[1].pValue;
    verify->seed.len = vTemplate[1].ulValueLen;
    verify->h.type = siUnsignedInteger;
    verify->h.data = vTemplate[2].pValue;
    verify->h.len = vTemplate[2].ulValueLen;

    PK11_DestroyObject(slot,objectID);
    PK11_FreeSlot(slot);

    *pParams = params;
    *pVfy =  verify;

    return SECSuccess;

loser:
    if (objectID != CK_INVALID_HANDLE) {
	PK11_DestroyObject(slot,objectID);
    }
    if (parena != NULL) {
	PORT_FreeArena(parena,PR_FALSE);
    }
    if (varena != NULL) {
	PORT_FreeArena(varena,PR_FALSE);
    }
    if (slot) {
	PK11_FreeSlot(slot);
    }
    return SECFailure;
}






extern SECStatus
PK11_PQG_ParamGenSeedLen( unsigned int j, unsigned int seedBytes,
				 PQGParams **pParams, PQGVerify **pVfy)
{
    unsigned int primeBits = PQG_INDEX_TO_PBITS(j);
    return PK11_PQG_ParamGenV2(primeBits, 0, seedBytes, pParams, pVfy);
}





extern SECStatus
PK11_PQG_ParamGen(unsigned int j, PQGParams **pParams, PQGVerify **pVfy)
{
    unsigned int primeBits = PQG_INDEX_TO_PBITS(j);
    return PK11_PQG_ParamGenV2(primeBits, 0, 0, pParams, pVfy);
}












extern SECStatus
PK11_PQG_VerifyParams(const PQGParams *params, const PQGVerify *vfy, 
							SECStatus *result)
{
    CK_ATTRIBUTE keyTempl[] = {
	{ CKA_CLASS, NULL, 0 },
	{ CKA_KEY_TYPE, NULL, 0 },
	{ CKA_PRIME, NULL, 0 },
	{ CKA_SUBPRIME, NULL, 0 },
	{ CKA_BASE, NULL, 0 },
	{ CKA_TOKEN, NULL, 0 },
	{ CKA_NETSCAPE_PQG_COUNTER, NULL, 0 },
	{ CKA_NETSCAPE_PQG_SEED, NULL, 0 },
	{ CKA_NETSCAPE_PQG_H, NULL, 0 },
    };
    CK_ATTRIBUTE *attrs;
    CK_BBOOL ckfalse = CK_FALSE;
    CK_OBJECT_CLASS class = CKO_KG_PARAMETERS;
    CK_KEY_TYPE keyType = CKK_DSA;
    SECStatus rv = SECSuccess;
    PK11SlotInfo *slot;
    int keyCount;
    CK_OBJECT_HANDLE objectID;
    CK_ULONG counter;
    CK_RV crv;

    attrs = keyTempl;
    PK11_SETATTRS(attrs, CKA_CLASS, &class, sizeof(class)); attrs++;
    PK11_SETATTRS(attrs, CKA_KEY_TYPE, &keyType, sizeof(keyType)); attrs++;
    PK11_SETATTRS(attrs, CKA_PRIME, params->prime.data, 
						params->prime.len); attrs++;
    PK11_SETATTRS(attrs, CKA_SUBPRIME, params->subPrime.data, 
						params->subPrime.len); attrs++;
    if (params->base.len) {
        PK11_SETATTRS(attrs, CKA_BASE,params->base.data,params->base.len);
	 attrs++;
    }
    PK11_SETATTRS(attrs, CKA_TOKEN, &ckfalse, sizeof(ckfalse)); attrs++;
    if (vfy) {
	if (vfy->counter != -1) {
	    counter = vfy->counter;
	    PK11_SETATTRS(attrs, CKA_NETSCAPE_PQG_COUNTER, 
			&counter, sizeof(counter)); attrs++;
	}
	PK11_SETATTRS(attrs, CKA_NETSCAPE_PQG_SEED, 
			vfy->seed.data, vfy->seed.len); attrs++;
	if (vfy->h.len) {
	    PK11_SETATTRS(attrs, CKA_NETSCAPE_PQG_H, 
			vfy->h.data, vfy->h.len); attrs++;
	}
    }

    keyCount = attrs - keyTempl;
    PORT_Assert(keyCount <= sizeof(keyTempl)/sizeof(keyTempl[0]));


    slot = PK11_GetInternalSlot();
    if (slot == NULL) {
	return SECFailure;
    }

    PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_CreateObject(slot->session, keyTempl, keyCount, 
								&objectID);
    PK11_ExitSlotMonitor(slot);

    
    PK11_DestroyObject(slot,objectID);
    PK11_FreeSlot(slot);

    *result = SECSuccess;
    if (crv == CKR_ATTRIBUTE_VALUE_INVALID) {
	*result = SECFailure;
    } else if (crv != CKR_OK) {
	PORT_SetError( PK11_MapError(crv) );
	rv = SECFailure;
    }
    return rv;

}






extern void 
PK11_PQG_DestroyParams(PQGParams *params) {
    if (params == NULL) 
    	return;
    if (params->arena != NULL) {
	PORT_FreeArena(params->arena, PR_FALSE);	
    } else {
	SECITEM_FreeItem(&params->prime,    PR_FALSE); 
	SECITEM_FreeItem(&params->subPrime, PR_FALSE); 
	SECITEM_FreeItem(&params->base,     PR_FALSE); 
	PORT_Free(params);
    }
}




extern void
PK11_PQG_DestroyVerify(PQGVerify *vfy) {
    if (vfy == NULL) 
    	return;
    if (vfy->arena != NULL) {
	PORT_FreeArena(vfy->arena, PR_FALSE);	
    } else {
	SECITEM_FreeItem(&vfy->seed,   PR_FALSE); 
	SECITEM_FreeItem(&vfy->h,      PR_FALSE); 
	PORT_Free(vfy);
    }
}

#define PQG_DEFAULT_CHUNKSIZE 2048	/* bytes */






extern PQGParams *
PK11_PQG_NewParams(const SECItem * prime, const SECItem * subPrime, 
                                 		const SECItem * base) {
    PRArenaPool *arena;
    PQGParams *dest;
    SECStatus status;

    arena = PORT_NewArena(PQG_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	goto loser;

    dest = (PQGParams*)PORT_ArenaZAlloc(arena, sizeof(PQGParams));
    if (dest == NULL)
	goto loser;

    dest->arena = arena;

    status = SECITEM_CopyItem(arena, &dest->prime, prime);
    if (status != SECSuccess)
	goto loser;

    status = SECITEM_CopyItem(arena, &dest->subPrime, subPrime);
    if (status != SECSuccess)
	goto loser;

    status = SECITEM_CopyItem(arena, &dest->base, base);
    if (status != SECSuccess)
	goto loser;

    return dest;

loser:
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}






extern SECStatus 
PK11_PQG_GetPrimeFromParams(const PQGParams *params, SECItem * prime) {
    return SECITEM_CopyItem(NULL, prime, &params->prime);
}






extern SECStatus
PK11_PQG_GetSubPrimeFromParams(const PQGParams *params, SECItem * subPrime) {
    return SECITEM_CopyItem(NULL, subPrime, &params->subPrime);
}






extern SECStatus 
PK11_PQG_GetBaseFromParams(const PQGParams *params, SECItem *base) {
    return SECITEM_CopyItem(NULL, base, &params->base);
}







extern PQGVerify *
PK11_PQG_NewVerify(unsigned int counter, const SECItem * seed, 
							const SECItem * h) {
    PRArenaPool *arena;
    PQGVerify *  dest;
    SECStatus    status;

    arena = PORT_NewArena(PQG_DEFAULT_CHUNKSIZE);
    if (arena == NULL)
	goto loser;

    dest = (PQGVerify*)PORT_ArenaZAlloc(arena, sizeof(PQGVerify));
    if (dest == NULL)
	goto loser;

    dest->arena   = arena;
    dest->counter = counter;

    status = SECITEM_CopyItem(arena, &dest->seed, seed);
    if (status != SECSuccess)
	goto loser;

    status = SECITEM_CopyItem(arena, &dest->h, h);
    if (status != SECSuccess)
	goto loser;

    return dest;

loser:
    if (arena != NULL)
	PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}





extern unsigned int 
PK11_PQG_GetCounterFromVerify(const PQGVerify *verify) {
    return verify->counter;
}





extern SECStatus 
PK11_PQG_GetSeedFromVerify(const PQGVerify *verify, SECItem *seed) {
    return SECITEM_CopyItem(NULL, seed, &verify->seed);
}






extern SECStatus 
PK11_PQG_GetHFromVerify(const PQGVerify *verify, SECItem * h) {
    return SECITEM_CopyItem(NULL, h, &verify->h);
}
