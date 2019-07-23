




































#include "seccomon.h"
#include "secoid.h"
#include "secasn1.h"
#include "pkcs11.h"
#include "pk11func.h"
#include "pk11sdr.h"





struct SDRResult
{
  SECItem keyid;
  SECAlgorithmID alg;
  SECItem data;
};
typedef struct SDRResult SDRResult;

SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)

static SEC_ASN1Template template[] = {
  { SEC_ASN1_SEQUENCE, 0, NULL, sizeof (SDRResult) },
  { SEC_ASN1_OCTET_STRING, offsetof(SDRResult, keyid) },
  { SEC_ASN1_INLINE | SEC_ASN1_XTRN, offsetof(SDRResult, alg),
    SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
  { SEC_ASN1_OCTET_STRING, offsetof(SDRResult, data) },
  { 0 }
};

static unsigned char keyID[] = {
  0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static SECItem keyIDItem = {
  0,
  keyID,
  sizeof keyID
};




static SECStatus
padBlock(SECItem *data, int blockSize, SECItem *result)
{
  SECStatus rv = SECSuccess;
  int padLength;
  unsigned int i;

  result->data = 0;
  result->len = 0;

  


  padLength = blockSize - (data->len % blockSize);
  result->len = data->len + padLength;
  result->data = (unsigned char *)PORT_Alloc(result->len);

  
  PORT_Memcpy(result->data, data->data, data->len);

  
  for(i = data->len; i < result->len; i++)
    result->data[i] = (unsigned char)padLength;

  return rv;
}

static SECStatus
unpadBlock(SECItem *data, int blockSize, SECItem *result)
{
  SECStatus rv = SECSuccess;
  int padLength;
  int i;

  result->data = 0;
  result->len = 0;

  
  if (data->len == 0 || data->len % blockSize  != 0) { rv = SECFailure; goto loser; }

  padLength = data->data[data->len-1];
  if (padLength > blockSize) { rv = SECFailure; goto loser; }

  
  for (i=data->len - padLength; i < data->len; i++) {
    if (data->data[i] != padLength) {
	rv = SECFailure;
	goto loser;
    }
  }

  result->len = data->len - padLength;
  result->data = (unsigned char *)PORT_Alloc(result->len);
  if (!result->data) { rv = SECFailure; goto loser; }

  PORT_Memcpy(result->data, data->data, result->len);

  if (padLength < 2) {
    return SECWouldBlock;
  }

loser:
  return rv;
}

static PRLock *pk11sdrLock = NULL;

void
pk11sdr_Init (void)
{
   pk11sdrLock = PR_NewLock();
}

void
pk11sdr_Shutdown(void)
{
    if (pk11sdrLock) {
	PR_DestroyLock(pk11sdrLock);
	pk11sdrLock = NULL;
    }
}






SECStatus
PK11SDR_Encrypt(SECItem *keyid, SECItem *data, SECItem *result, void *cx)
{
  SECStatus rv = SECSuccess;
  PK11SlotInfo *slot = 0;
  PK11SymKey *key = 0;
  SECItem *params = 0;
  PK11Context *ctx = 0;
  CK_MECHANISM_TYPE type;
  SDRResult sdrResult;
  SECItem paddedData;
  SECItem *pKeyID;
  PLArenaPool *arena = 0;

  
  paddedData.len = 0;
  paddedData.data = 0;

  arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
  if (!arena) { rv = SECFailure; goto loser; }

  





  slot = PK11_GetInternalKeySlot();
  if (!slot) { rv = SECFailure; goto loser; }

  
  type = CKM_DES3_CBC;

  



  rv = PK11_Authenticate(slot, PR_TRUE, cx);
  if (rv != SECSuccess) goto loser;

  
  pKeyID = keyid;
  if (pKeyID->len == 0) {
	  pKeyID = &keyIDItem;  

	  



	  if (pk11sdrLock) PR_Lock(pk11sdrLock);

	  
	  key = PK11_FindFixedKey(slot, type, pKeyID, cx);
	  
	  
	  if (!key) key = PK11_GenDES3TokenKey(slot, pKeyID, cx);
	  if (pk11sdrLock) PR_Unlock(pk11sdrLock);
  } else {
	  key = PK11_FindFixedKey(slot, type, pKeyID, cx);
  }

  if (!key) { rv = SECFailure; goto loser; }

  params = PK11_GenerateNewParam(type, key);
  if (!params) { rv = SECFailure; goto loser; }

  ctx = PK11_CreateContextBySymKey(type, CKA_ENCRYPT, key, params);
  if (!ctx) { rv = SECFailure; goto loser; }

  rv = padBlock(data, PK11_GetBlockSize(type, 0), &paddedData);
  if (rv != SECSuccess) goto loser;

  sdrResult.data.len = paddedData.len;
  sdrResult.data.data = (unsigned char *)PORT_ArenaAlloc(arena, sdrResult.data.len);

  rv = PK11_CipherOp(ctx, sdrResult.data.data, (int*)&sdrResult.data.len, sdrResult.data.len,
                     paddedData.data, paddedData.len);
  if (rv != SECSuccess) goto loser;

  PK11_Finalize(ctx);

  sdrResult.keyid = *pKeyID;

  rv = PK11_ParamToAlgid(SEC_OID_DES_EDE3_CBC, params, arena, &sdrResult.alg);
  if (rv != SECSuccess) goto loser;

  if (!SEC_ASN1EncodeItem(0, result, &sdrResult, template)) { rv = SECFailure; goto loser; }

loser:
  SECITEM_ZfreeItem(&paddedData, PR_FALSE);
  if (arena) PORT_FreeArena(arena, PR_TRUE);
  if (ctx) PK11_DestroyContext(ctx, PR_TRUE);
  if (params) SECITEM_ZfreeItem(params, PR_TRUE);
  if (key) PK11_FreeSymKey(key);
  if (slot) PK11_FreeSlot(slot);

  return rv;
}


static SECStatus
pk11Decrypt(PK11SlotInfo *slot, PLArenaPool *arena, 
	    CK_MECHANISM_TYPE type, PK11SymKey *key, 
	    SECItem *params, SECItem *in, SECItem *result)
{
  PK11Context *ctx = 0;
  SECItem paddedResult;
  SECStatus rv;

  paddedResult.len = 0;
  paddedResult.data = 0;

  ctx = PK11_CreateContextBySymKey(type, CKA_DECRYPT, key, params);
  if (!ctx) { rv = SECFailure; goto loser; }

  paddedResult.len = in->len;
  paddedResult.data = PORT_ArenaAlloc(arena, paddedResult.len);

  rv = PK11_CipherOp(ctx, paddedResult.data, 
			(int*)&paddedResult.len, paddedResult.len,
			in->data, in->len);
  if (rv != SECSuccess) goto loser;

  PK11_Finalize(ctx);

  
  rv = unpadBlock(&paddedResult, PK11_GetBlockSize(type, 0), result);
  if (rv) goto loser;

loser:
  if (ctx) PK11_DestroyContext(ctx, PR_TRUE);
  return rv;
}






SECStatus
PK11SDR_Decrypt(SECItem *data, SECItem *result, void *cx)
{
  SECStatus rv = SECSuccess;
  PK11SlotInfo *slot = 0;
  PK11SymKey *key = 0;
  CK_MECHANISM_TYPE type;
  SDRResult sdrResult;
  SECItem *params = 0;
  SECItem possibleResult = { 0, NULL, 0 };
  PLArenaPool *arena = 0;

  arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
  if (!arena) { rv = SECFailure; goto loser; }

  
  memset(&sdrResult, 0, sizeof sdrResult);
  rv = SEC_QuickDERDecodeItem(arena, &sdrResult, template, data);
  if (rv != SECSuccess) goto loser;  

  
  slot = PK11_GetInternalKeySlot();
  if (!slot) { rv = SECFailure; goto loser; }

  rv = PK11_Authenticate(slot, PR_TRUE, cx);
  if (rv != SECSuccess) goto loser;

  
  params = PK11_ParamFromAlgid(&sdrResult.alg);
  if (!params) { rv = SECFailure; goto loser; }

  
  type = CKM_DES3_CBC;
  key = PK11_FindFixedKey(slot, type, &sdrResult.keyid, cx);
  if (!key) { 
	rv = SECFailure;  
  } else {
	rv = pk11Decrypt(slot, arena, type, key, params, 
			&sdrResult.data, result);
  }

  





  if (rv == SECWouldBlock) {
	possibleResult = *result;
  }

  


  if (rv != SECSuccess) {
	PK11SymKey *keyList = PK11_ListFixedKeysInSlot(slot, NULL, cx);
	PK11SymKey *testKey = NULL;
	PK11SymKey *nextKey = NULL;

	for (testKey = keyList; testKey; 
				testKey = PK11_GetNextSymKey(testKey)) {
	    rv = pk11Decrypt(slot, arena, type, testKey, params, 
			     &sdrResult.data, result);
	    if (rv == SECSuccess) {
		break;
	    } 
	    
	    if (rv == SECWouldBlock) {
		if (possibleResult.data) {
		    



		    SECITEM_ZfreeItem(result, PR_FALSE);
		} else {
		    possibleResult = *result;
		}
	    }
	}

	
	for (testKey = keyList; testKey; testKey = nextKey) {
	    nextKey = PK11_GetNextSymKey(testKey);
	    PK11_FreeSymKey(testKey);
	}
  }

  
  if ((rv != SECSuccess) && (possibleResult.data)) {
	*result = possibleResult;
	possibleResult.data = NULL;
	rv = SECSuccess;
  }

loser:
  if (arena) PORT_FreeArena(arena, PR_TRUE);
  if (key) PK11_FreeSymKey(key);
  if (params) SECITEM_ZfreeItem(params, PR_TRUE);
  if (slot) PK11_FreeSlot(slot);
  if (possibleResult.data) SECITEM_ZfreeItem(&possibleResult, PR_FALSE);

  return rv;
}
