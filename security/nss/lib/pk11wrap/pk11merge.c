



#include "secmod.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pk11pub.h"
#include "pk11priv.h"
#include "pkcs11.h"
#include "seccomon.h"
#include "secerr.h"
#include "keyhi.h"
#include "hasht.h"
#include "cert.h"
#include "certdb.h"










static SECStatus
pk11_setAttributes(PK11SlotInfo *slot, CK_OBJECT_HANDLE id,
		CK_ATTRIBUTE *setTemplate, CK_ULONG setTemplCount)
{
    CK_RV crv;
    CK_SESSION_HANDLE rwsession;

    rwsession = PK11_GetRWSession(slot);
    if (rwsession == CK_INVALID_SESSION) {
    	PORT_SetError(SEC_ERROR_BAD_DATA);
    	return SECFailure;
    }
    crv = PK11_GETTAB(slot)->C_SetAttributeValue(rwsession, id,
			setTemplate, setTemplCount);
    PK11_RestoreROSession(slot, rwsession);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    return SECSuccess;
}






static SECStatus
pk11_copyAttributes(PRArenaPool *arena, 
	PK11SlotInfo *targetSlot, CK_OBJECT_HANDLE targetID,
	PK11SlotInfo *sourceSlot, CK_OBJECT_HANDLE sourceID,
	CK_ATTRIBUTE *copyTemplate, CK_ULONG copyTemplateCount)
{
    SECStatus rv = PK11_GetAttributes(arena, sourceSlot, sourceID, 
				copyTemplate, copyTemplateCount);
    if (rv != SECSuccess) {
	return rv;
    }
    if (targetID == CK_INVALID_HANDLE) {
	
	rv = PK11_CreateNewObject(targetSlot, CK_INVALID_SESSION, 
		copyTemplate, copyTemplateCount, PR_TRUE, &targetID);
    } else {
	
	rv = pk11_setAttributes(targetSlot, targetID, 
			copyTemplate, copyTemplateCount);
    }
    return rv;
}




static SECStatus
pk11_matchAcrossTokens(PRArenaPool *arena, PK11SlotInfo *targetSlot, 
		       PK11SlotInfo *sourceSlot,
		       CK_ATTRIBUTE *template, CK_ULONG tsize, 
		       CK_OBJECT_HANDLE id, CK_OBJECT_HANDLE *peer)
{

    CK_RV crv;
    *peer = CK_INVALID_HANDLE;

    crv = PK11_GetAttributes(arena, sourceSlot, id, template, tsize);
    if (crv != CKR_OK) {
 	PORT_SetError( PK11_MapError(crv) );
	goto loser;
    }

    if (template[0].ulValueLen == -1) {
	crv = CKR_ATTRIBUTE_TYPE_INVALID;
 	PORT_SetError( PK11_MapError(crv) );
	goto loser;
    }

    *peer = pk11_FindObjectByTemplate(targetSlot, template, tsize);
    return SECSuccess;

loser:
    return SECFailure;
}




SECStatus
pk11_encrypt(PK11SymKey *symKey, CK_MECHANISM_TYPE mechType, SECItem *param,
	SECItem *input, SECItem **output)
{
    PK11Context *ctxt = NULL;
    SECStatus rv = SECSuccess;

    if (*output) {
	SECITEM_FreeItem(*output,PR_TRUE);
    }
    *output = SECITEM_AllocItem(NULL, NULL, input->len+20 );
    if (!*output) {
	rv = SECFailure;
	goto done;
    }

    ctxt = PK11_CreateContextBySymKey(mechType, CKA_ENCRYPT, symKey, param);
    if (ctxt == NULL) {
	rv = SECFailure;
	goto done;
    }

    rv = PK11_CipherOp(ctxt, (*output)->data, 
		(int *)&((*output)->len), 
		(*output)->len, input->data, input->len);

done:
    if (ctxt) {
	PK11_Finalize(ctxt);
	PK11_DestroyContext(ctxt,PR_TRUE);
    }
    if (rv != SECSuccess) {
	if (*output) {
	    SECITEM_FreeItem(*output, PR_TRUE);
	    *output = NULL;
	}
    }
    return rv;
}












unsigned int
pk11_getPrivateKeyUsage(PK11SlotInfo *slot, CK_OBJECT_HANDLE id)
{
    unsigned int usage = 0;

    if ((PK11_HasAttributeSet(slot, id, CKA_UNWRAP) || 
			PK11_HasAttributeSet(slot,id, CKA_DECRYPT))) {
	usage |= KU_KEY_ENCIPHERMENT;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_DERIVE)) {
	usage |= KU_KEY_AGREEMENT;
    }
    if ((PK11_HasAttributeSet(slot, id, CKA_SIGN_RECOVER) || 
			PK11_HasAttributeSet(slot, id, CKA_SIGN))) {
	usage |= KU_DIGITAL_SIGNATURE;
    }
    return usage;
}
    
	







static SECStatus
pk11_mergePrivateKey(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{
    SECKEYPrivateKey *sourceKey = NULL;
    CK_OBJECT_HANDLE targetKeyID;
    SECKEYEncryptedPrivateKeyInfo *epki = NULL;
    char *nickname = NULL;
    SECItem nickItem;
    SECItem pwitem;
    SECItem publicValue;
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    unsigned int keyUsage;
    unsigned char randomData[SHA1_LENGTH];
    SECOidTag algTag = SEC_OID_PKCS12_V2_PBE_WITH_SHA1_AND_3KEY_TRIPLE_DES_CBC;
    CK_ATTRIBUTE privTemplate[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    CK_ULONG privTemplateCount = sizeof(privTemplate)/sizeof(privTemplate[0]);
    CK_ATTRIBUTE privCopyTemplate[] = {
	{ CKA_SUBJECT, NULL, 0 }
    };
    CK_ULONG privCopyTemplateCount = 
		sizeof(privCopyTemplate)/sizeof(privCopyTemplate[0]);

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	rv = SECFailure;
	goto done;
    }

    
    rv = pk11_matchAcrossTokens(arena, targetSlot, sourceSlot, privTemplate, 
				privTemplateCount, id, &targetKeyID);
    if (rv != SECSuccess) {
	goto done;
    }

    if (targetKeyID != CK_INVALID_HANDLE) {
	
	goto done;
    }

    
    sourceKey = PK11_MakePrivKey(sourceSlot, nullKey, PR_FALSE, 
				 id, sourcePwArg);
    if (sourceKey == NULL) {
	rv = SECFailure;
	goto done;
    }

    
    
    rv = PK11_GenerateRandom(randomData, sizeof(randomData));
    if (rv != SECSuccess) {
	goto done;
    }
    pwitem.data = randomData;
    pwitem.len = sizeof(randomData);
    
    epki = PK11_ExportEncryptedPrivKeyInfo(sourceSlot, algTag, &pwitem, 
					   sourceKey, 1, sourcePwArg);
    if (epki == NULL) {
	rv = SECFailure;
	goto done;
    }
    nickname = PK11_GetObjectNickname(sourceSlot, id);
    
    if (nickname)  {
	nickItem.data = (unsigned char *)nickname;
	nickItem.len = PORT_Strlen(nickname);
    }
    keyUsage = pk11_getPrivateKeyUsage(sourceSlot, id);
    
    publicValue.data = privTemplate[0].pValue;
    publicValue.len = privTemplate[0].ulValueLen;
    rv = PK11_ImportEncryptedPrivateKeyInfo(targetSlot, epki, &pwitem,
			nickname? &nickItem : NULL , &publicValue, 
			PR_TRUE, PR_TRUE, sourceKey->keyType, keyUsage, 
			targetPwArg);
    if (rv != SECSuccess) {
	goto done;
    }

    
    rv = pk11_matchAcrossTokens(arena, targetSlot, sourceSlot, privTemplate, 
				privTemplateCount, id, &targetKeyID);
    if (rv != SECSuccess) {
	goto done;
    }

    if (targetKeyID == CK_INVALID_HANDLE) {
	
	rv = SECFailure;
	goto done;
    }

    
    rv = pk11_copyAttributes(arena, targetSlot, targetKeyID, sourceSlot, id,
				privCopyTemplate, privCopyTemplateCount);
done:
    
    PORT_Memset(randomData, 0, sizeof(randomData));
    if (nickname) {
	PORT_Free(nickname);
    }
    if (sourceKey) {
	SECKEY_DestroyPrivateKey(sourceKey);
    }
    if (epki) {
	SECKEY_DestroyEncryptedPrivateKeyInfo(epki, PR_TRUE);
    }
    if (arena) {
         PORT_FreeArena(arena,PR_FALSE);
    }
    return rv;
}




















static SECStatus
pk11_incrementID(PRArenaPool *arena, CK_ATTRIBUTE *ptemplate)
{
    unsigned char *buf = ptemplate->pValue;
    CK_ULONG len = ptemplate->ulValueLen;

    if (buf == NULL || len == (CK_ULONG)-1) {
	
	len = 0;
    } else {
	CK_ULONG i;

	


	for (i=len; i != 0; i--) {
	    buf[i-1]++;
	    if (buf[i-1] != 0) {
		
		return SECSuccess;
	     }
	}
	

    } 
    


    buf = PORT_ArenaZAlloc(arena, len+1);
    if (buf == NULL) {
	return SECFailure;
    }
    ptemplate->pValue = buf;
    ptemplate->ulValueLen = len+1;
    return SECSuccess;
}


static CK_FLAGS
pk11_getSecretKeyFlags(PK11SlotInfo *slot, CK_OBJECT_HANDLE id)
{
    CK_FLAGS flags = 0;

    if (PK11_HasAttributeSet(slot, id, CKA_UNWRAP)) {
	flags |= CKF_UNWRAP;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_WRAP)) {
	flags |= CKF_WRAP;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_ENCRYPT)) {
	flags |= CKF_ENCRYPT;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_DECRYPT)) {
	flags |= CKF_DECRYPT;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_DERIVE)) {
	flags |= CKF_DERIVE;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_SIGN)) {
	flags |= CKF_SIGN;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_SIGN_RECOVER)) {
	flags |= CKF_SIGN_RECOVER;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_VERIFY)) {
	flags |= CKF_VERIFY;
    }
    if (PK11_HasAttributeSet(slot, id, CKA_VERIFY_RECOVER)) {
	flags |= CKF_VERIFY_RECOVER;
    }
    return flags;
}

static const char testString[] = 
	"My Encrytion Test Data (should be at least 32 bytes long)";















static SECStatus
pk11_mergeSecretKey(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{
    PK11SymKey *sourceKey = NULL;
    PK11SymKey *targetKey = NULL;
    SECItem *sourceOutput = NULL;
    SECItem *targetOutput = NULL;
    SECItem *param = NULL;
    SECItem input;
    CK_OBJECT_HANDLE targetKeyID;
    CK_FLAGS flags;
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    CK_MECHANISM_TYPE keyMechType, cryptoMechType;
    CK_KEY_TYPE sourceKeyType, targetKeyType;
    CK_ATTRIBUTE symTemplate[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    CK_ULONG symTemplateCount = sizeof(symTemplate)/sizeof(symTemplate[0]);
    CK_ATTRIBUTE symCopyTemplate[] = {
	{ CKA_LABEL, NULL, 0 }
    };
    CK_ULONG symCopyTemplateCount = 
		sizeof(symCopyTemplate)/sizeof(symCopyTemplate[0]);

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	rv = SECFailure;
	goto done;
    }

    sourceKeyType = PK11_ReadULongAttribute(sourceSlot, id, CKA_KEY_TYPE);
    if (sourceKeyType == (CK_ULONG) -1) {
	rv = SECFailure;
	goto done;
    }

    
    keyMechType = PK11_GetKeyMechanism(sourceKeyType);
    




    cryptoMechType =  keyMechType;
    if ((keyMechType == CKM_DES3_KEY_GEN) ||  
				(keyMechType == CKM_DES2_KEY_GEN)) {
	cryptoMechType = CKM_DES3_CBC;
    }

    sourceKey = PK11_SymKeyFromHandle(sourceSlot, NULL, PK11_OriginDerive,
				keyMechType , id, PR_FALSE, sourcePwArg);
    if (sourceKey == NULL) {
	rv = SECFailure;
	goto done;
    }

    



    rv = pk11_matchAcrossTokens(arena, targetSlot, sourceSlot,
			symTemplate, symTemplateCount, id, &targetKeyID);
    if (rv != SECSuccess) {
	goto done;
    }

    
    input.data = (unsigned char *)testString;
    input.len = PK11_GetBlockSize(cryptoMechType, NULL);
    if (input.len < 0) {
	rv = SECFailure;
	goto done;
    }
    if (input.len == 0) {
	input.len = sizeof (testString);
    }
    while (targetKeyID != CK_INVALID_HANDLE) {
	
	targetKeyType = PK11_ReadULongAttribute(sourceSlot, id, CKA_KEY_TYPE);
	if (targetKeyType == sourceKeyType) {
		
		targetKey = PK11_SymKeyFromHandle(targetSlot, NULL, 
			PK11_OriginDerive, keyMechType, targetKeyID, PR_FALSE,
			targetPwArg);
		
		if (!param) {
		    param = PK11_GenerateNewParam(cryptoMechType, sourceKey);
		    if (param == NULL) {
			rv = SECFailure;
			goto done;
		    }
		}
		
		if (!sourceOutput) {
		    rv = pk11_encrypt(sourceKey, cryptoMechType, param, &input,
			&sourceOutput);
		    if (rv != SECSuccess) {
			goto done;
		    }
		}
		
		rv = pk11_encrypt(targetKey, cryptoMechType, param, &input,
			&targetOutput);
		if (rv == SECSuccess) {
		    if (SECITEM_ItemsAreEqual(sourceOutput, targetOutput)) {
			

			goto done;
		    }
		    SECITEM_FreeItem(targetOutput, PR_TRUE);
		    targetOutput = NULL;
		}
		PK11_FreeSymKey(targetKey);
		targetKey = NULL;
	}
	
	rv = pk11_incrementID(arena, &symTemplate[0]);
	if (rv != SECSuccess) {
	    goto done;
	}
	targetKeyID = pk11_FindObjectByTemplate(targetSlot, 
					symTemplate, symTemplateCount);
    }

    

    flags = pk11_getSecretKeyFlags(sourceSlot, id);
    targetKey = PK11_MoveSymKey(targetSlot, PK11_OriginDerive, flags, PR_TRUE,
			sourceKey);
    if (targetKey == NULL) {
	rv = SECFailure;
	goto done;
    }
    
    rv = pk11_setAttributes(targetSlot, targetKey->objectID, symTemplate, 1);
    if (rv != SECSuccess) {
	goto done;
    }

    
    rv = pk11_copyAttributes(arena, targetSlot, targetKey->objectID, 
			sourceSlot, id, symCopyTemplate, symCopyTemplateCount);
done:
    if (sourceKey) {
	PK11_FreeSymKey(sourceKey);
    }
    if (targetKey) {
	PK11_FreeSymKey(targetKey);
    }
    if (sourceOutput) {
	SECITEM_FreeItem(sourceOutput, PR_TRUE);
    }
    if (targetOutput) {
	SECITEM_FreeItem(targetOutput, PR_TRUE);
    }
    if (param) {
	SECITEM_FreeItem(param, PR_TRUE);
    }
    if (arena) {
         PORT_FreeArena(arena,PR_FALSE);
    }
    return rv;
}













static SECStatus
pk11_mergePublicKey(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{
    SECKEYPublicKey *sourceKey = NULL;
    CK_OBJECT_HANDLE targetKeyID;
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    CK_ATTRIBUTE pubTemplate[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    CK_ULONG pubTemplateCount = sizeof(pubTemplate)/sizeof(pubTemplate[0]);
    CK_ATTRIBUTE pubCopyTemplate[] = {
	{ CKA_ID, NULL, 0 },
	{ CKA_LABEL, NULL, 0 },
	{ CKA_SUBJECT, NULL, 0 }
    };
    CK_ULONG pubCopyTemplateCount = 
		sizeof(pubCopyTemplate)/sizeof(pubCopyTemplate[0]);

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	rv = SECFailure;
	goto done;
    }


    
    rv = pk11_matchAcrossTokens(arena, targetSlot, sourceSlot, pubTemplate, 
				pubTemplateCount, id, &targetKeyID);
    if (rv != SECSuccess) {
	goto done;
    }

    
    if (targetKeyID != CK_INVALID_HANDLE) {
	
	goto done;
    }

    
    sourceKey = PK11_ExtractPublicKey(sourceSlot, nullKey, id);
    if (sourceKey== NULL) {
	rv = SECFailure;
	goto done;
    }

    
    targetKeyID = PK11_ImportPublicKey(targetSlot, sourceKey, PR_TRUE);
    if (targetKeyID == CK_INVALID_HANDLE) {
	rv = SECFailure;
	goto done;
    }

    
    rv = pk11_copyAttributes(arena, targetSlot, targetKeyID, sourceSlot, id,
				pubCopyTemplate, pubCopyTemplateCount);


done:
    if (sourceKey) {
	SECKEY_DestroyPublicKey(sourceKey);
    }
    if (arena) {
         PORT_FreeArena(arena,PR_FALSE);
    }
    return rv;
}












static char *
pk11_IncrementNickname(char *nickname)
{
    char *newNickname = NULL;
    int end;
    PRBool needCarry;
    int digit;
    int len = strlen(nickname);

    
    for (end = len - 1; 
         end >= 2 && (digit = nickname[end]) <= '9' &&  digit >= '0'; 
	 end--)   ;
    if (len >= 3 &&
        end < (len - 1)  &&
	nickname[end]     == '#'  && 
	nickname[end - 1] == ' ') {
    	
    } else {
	
	static const char num2[] = " #2";
	newNickname = PORT_Realloc(nickname, len + sizeof(num2));
	if (newNickname) {
	    PORT_Strcat(newNickname, num2);
	} else {
	    PORT_Free(nickname);
	}
	return newNickname;
    }

    for (end = len - 1; 
	 end >= 0 && (digit = nickname[end]) <= '9' &&  digit >= '0'; 
	 end--) {
	if (digit < '9') {
	    nickname[end]++;
	    return nickname;
	}
	nickname[end] = '0';
    }

    
    newNickname = PORT_Realloc(nickname, len + 2);
    if (newNickname) {
	newNickname[++end] = '1';
	PORT_Memset(&newNickname[end + 1], '0', len - end);
	newNickname[len + 1] = 0;
    } else {
	PORT_Free(nickname);
    }
    return newNickname;
}






static SECStatus
pk11_mergeCert(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{
    CERTCertificate *sourceCert = NULL;
    CK_OBJECT_HANDLE targetCertID = CK_INVALID_HANDLE;
    char *nickname = NULL;
    SECStatus rv = SECSuccess;
    PRArenaPool *arena = NULL;
    CK_ATTRIBUTE sourceCKAID = {CKA_ID, NULL, 0};
    CK_ATTRIBUTE targetCKAID = {CKA_ID, NULL, 0};
    SECStatus lrv = SECSuccess;
    int error;


    sourceCert = PK11_MakeCertFromHandle(sourceSlot, id, NULL);
    if (sourceCert == NULL) {
	rv = SECFailure;
	goto done;
    }

    nickname = PK11_GetObjectNickname(sourceSlot, id);

    


    if (nickname) {
	const char *tokenName = PK11_GetTokenName(targetSlot);
	char *tokenNickname = NULL;

	do {
	    tokenNickname = PR_smprintf("%s:%s",tokenName, nickname);
	    if (!tokenNickname) {
		break;
	    }
	    if (!SEC_CertNicknameConflict(tokenNickname, 
			&sourceCert->derSubject, CERT_GetDefaultCertDB())) {
		break;
	     }
	    nickname = pk11_IncrementNickname(nickname);
	    if (!nickname) {
		break;
	    }
	    PR_smprintf_free(tokenNickname);
	} while (1);
	if (tokenNickname) {
	    PR_smprintf_free(tokenNickname);
	}
    }

	

    
    targetCertID = PK11_FindCertInSlot(targetSlot, sourceCert, targetPwArg);
    if (targetCertID == CK_INVALID_HANDLE) {
	
	
	rv = PK11_ImportCert(targetSlot, sourceCert, CK_INVALID_HANDLE,
			     nickname, PR_FALSE);
	goto done;
    }

    


    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	rv = SECFailure;
	goto done;
    }

    
    rv = PK11_GetAttributes(arena, sourceSlot, id,  &sourceCKAID, 1);
    if (rv != SECSuccess) {
	sourceCKAID.ulValueLen = 0;
    }

    

    if (sourceCKAID.ulValueLen != 0) {
	rv = PK11_GetAttributes(arena, targetSlot, targetCertID,
				    &targetCKAID, 1);
	if (rv != SECSuccess) {
	    targetCKAID.ulValueLen = 0;
	}
	
	if (targetCKAID.ulValueLen == 0) {
	    lrv=pk11_setAttributes(targetSlot, targetCertID, &sourceCKAID, 1);
	    if (lrv != SECSuccess) {
		error = PORT_GetError();
	    }
	}
    }
    rv = SECSuccess;

    
    if (nickname && *nickname) {
	char *targetname;
	targetname = PK11_GetObjectNickname(targetSlot, targetCertID);
	if (!targetname || !*targetname) {
	    
	    rv = PK11_SetObjectNickname(targetSlot, targetCertID, nickname);
	}
	if (targetname) {
	    PORT_Free(targetname);
	}
    }

    
    if ((rv == SECSuccess) && (lrv != SECSuccess)) {
	rv = lrv;
	PORT_SetError(error);
    }

done:
    if (nickname) {
	PORT_Free(nickname);
    }
    if (sourceCert) {
	CERT_DestroyCertificate(sourceCert);
    }
    if (arena) {
         PORT_FreeArena(arena,PR_FALSE);
    }
    return rv;
}













static SECStatus
pk11_mergeCrl(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{
    CK_OBJECT_HANDLE targetCrlID;
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    CK_ATTRIBUTE crlTemplate[] = {
	{ CKA_SUBJECT, NULL, 0 },
	{ CKA_CLASS, NULL, 0 },
	{ CKA_NSS_KRL, NULL, 0 }
    };
    CK_ULONG crlTemplateCount = sizeof(crlTemplate)/sizeof(crlTemplate[0]);
    CK_ATTRIBUTE crlCopyTemplate[] = {
	{ CKA_CLASS, NULL, 0 },
	{ CKA_TOKEN, NULL, 0 },
	{ CKA_LABEL, NULL, 0 },
	{ CKA_PRIVATE, NULL, 0 },
	{ CKA_MODIFIABLE, NULL, 0 },
	{ CKA_SUBJECT, NULL, 0 },
	{ CKA_NSS_KRL, NULL, 0 },
	{ CKA_NSS_URL, NULL, 0 },
	{ CKA_VALUE, NULL, 0 }
    };
    CK_ULONG crlCopyTemplateCount = 
		sizeof(crlCopyTemplate)/sizeof(crlCopyTemplate[0]);

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	rv = SECFailure;
	goto done;
    }
    
    rv = pk11_matchAcrossTokens(arena, targetSlot, sourceSlot, crlTemplate, 
				crlTemplateCount, id, &targetCrlID);
    if (rv != SECSuccess) {
	goto done;
    }
    if (targetCrlID != CK_INVALID_HANDLE) {
	
	goto done;
    }

    
    rv = pk11_copyAttributes(arena, targetSlot, targetCrlID, sourceSlot, id,
				crlCopyTemplate, crlCopyTemplateCount);
done:
    if (arena) {
         PORT_FreeArena(arena,PR_FALSE);
    }
    return rv;
}










static SECStatus
pk11_mergeSmime(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{
    CK_OBJECT_HANDLE targetSmimeID;
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    CK_ATTRIBUTE smimeTemplate[] = {
	{ CKA_SUBJECT, NULL, 0 },
	{ CKA_NSS_EMAIL, NULL, 0 },
	{ CKA_CLASS, NULL, 0 },
    };
    CK_ULONG smimeTemplateCount = 
		sizeof(smimeTemplate)/sizeof(smimeTemplate[0]);
    CK_ATTRIBUTE smimeCopyTemplate[] = {
	{ CKA_CLASS, NULL, 0 },
	{ CKA_TOKEN, NULL, 0 },
	{ CKA_LABEL, NULL, 0 },
	{ CKA_PRIVATE, NULL, 0 },
	{ CKA_MODIFIABLE, NULL, 0 },
	{ CKA_SUBJECT, NULL, 0 },
	{ CKA_NSS_EMAIL, NULL, 0 },
	{ CKA_NSS_SMIME_TIMESTAMP, NULL, 0 },
	{ CKA_VALUE, NULL, 0 }
    };
    CK_ULONG smimeCopyTemplateCount = 
		sizeof(smimeCopyTemplate)/sizeof(smimeCopyTemplate[0]);

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	rv = SECFailure;
	goto done;
    }
    
    rv = pk11_matchAcrossTokens(arena, targetSlot, sourceSlot, smimeTemplate, 
				smimeTemplateCount, id, &targetSmimeID);
    if (rv != SECSuccess) {
	goto done;
    }
    if (targetSmimeID != CK_INVALID_HANDLE) {
	
	goto done;
    }

    
    rv = pk11_copyAttributes(arena, targetSlot, targetSmimeID, sourceSlot, id,
				smimeCopyTemplate, smimeCopyTemplateCount);
done:
    if (arena) {
         PORT_FreeArena(arena,PR_FALSE);
    }
    return rv;
}











#define USE_TARGET PR_FALSE
#define USE_SOURCE PR_TRUE
PRBool
pk11_mergeTrustEntry(CK_ATTRIBUTE *target, CK_ATTRIBUTE *source)
{
    CK_ULONG targetTrust = (target->ulValueLen == sizeof (CK_LONG)) ?
		*(CK_ULONG *)target->pValue : CKT_NSS_TRUST_UNKNOWN;
    CK_ULONG sourceTrust = (source->ulValueLen == sizeof (CK_LONG)) ?
		*(CK_ULONG *)source->pValue : CKT_NSS_TRUST_UNKNOWN;

    







    if (sourceTrust == targetTrust) {
	return USE_TARGET;  
    }

    if (sourceTrust == CKT_NSS_TRUST_UNKNOWN) {
	return USE_TARGET; 
    }

    
    if (targetTrust == CKT_NSS_TRUST_UNKNOWN) {
	
	return USE_SOURCE;
    }

    







    if ((sourceTrust == CKT_NSS_MUST_VERIFY) 
	|| (sourceTrust == CKT_NSS_VALID)
	|| (sourceTrust == CKT_NSS_VALID_DELEGATOR)) {
	return USE_TARGET;
    }
    if ((targetTrust == CKT_NSS_MUST_VERIFY) 
	|| (targetTrust == CKT_NSS_VALID)
	|| (targetTrust == CKT_NSS_VALID_DELEGATOR)) {
	
	return USE_SOURCE;
    }

    
    return USE_TARGET;
}



static SECStatus
pk11_mergeTrust(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{
    CK_OBJECT_HANDLE targetTrustID;
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    int error = 0;
    CK_ATTRIBUTE trustTemplate[] = {
	{ CKA_ISSUER, NULL, 0 },
	{ CKA_SERIAL_NUMBER, NULL, 0 },
	{ CKA_CLASS, NULL, 0 },
    };
    CK_ULONG trustTemplateCount = 
		sizeof(trustTemplate)/sizeof(trustTemplate[0]);
    CK_ATTRIBUTE trustCopyTemplate[] = {
	{ CKA_CLASS, NULL, 0 },
	{ CKA_TOKEN, NULL, 0 },
	{ CKA_LABEL, NULL, 0 },
	{ CKA_PRIVATE, NULL, 0 },
	{ CKA_MODIFIABLE, NULL, 0 },
	{ CKA_ISSUER, NULL, 0},
	{ CKA_SERIAL_NUMBER, NULL, 0},
	{ CKA_CERT_SHA1_HASH, NULL, 0 },
	{ CKA_CERT_MD5_HASH, NULL, 0 },
	{ CKA_TRUST_SERVER_AUTH, NULL, 0 },
	{ CKA_TRUST_CLIENT_AUTH, NULL, 0 },
	{ CKA_TRUST_CODE_SIGNING, NULL, 0 },
	{ CKA_TRUST_EMAIL_PROTECTION, NULL, 0 },
	{ CKA_TRUST_STEP_UP_APPROVED, NULL, 0 }
    };
    CK_ULONG trustCopyTemplateCount = 
		sizeof(trustCopyTemplate)/sizeof(trustCopyTemplate[0]);

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	rv = SECFailure;
	goto done;
    }
    
    rv = pk11_matchAcrossTokens(arena, targetSlot, sourceSlot, trustTemplate, 
				trustTemplateCount, id, &targetTrustID);
    if (rv != SECSuccess) {
	goto done;
    }
    if (targetTrustID != CK_INVALID_HANDLE) {
	
	CK_ATTRIBUTE_TYPE trustAttrs[] = {
	    CKA_TRUST_SERVER_AUTH, CKA_TRUST_CLIENT_AUTH,
	    CKA_TRUST_CODE_SIGNING, CKA_TRUST_EMAIL_PROTECTION, 
	    CKA_TRUST_IPSEC_TUNNEL, CKA_TRUST_IPSEC_USER, 
	    CKA_TRUST_TIME_STAMPING
	};
	CK_ULONG trustAttrsCount = 
		sizeof(trustAttrs)/sizeof(trustAttrs[0]);

	int i;
	CK_ATTRIBUTE targetTemplate, sourceTemplate;

	
        for (i=0; i < trustAttrsCount; i++) {
	    targetTemplate.type = sourceTemplate.type = trustAttrs[i];
	    targetTemplate.pValue = sourceTemplate.pValue = NULL;
	    targetTemplate.ulValueLen = sourceTemplate.ulValueLen = 0;
	    PK11_GetAttributes(arena, sourceSlot, id, &sourceTemplate, 1);
	    PK11_GetAttributes(arena, targetSlot, targetTrustID, 
							&targetTemplate, 1);
	    if (pk11_mergeTrustEntry(&targetTemplate, &sourceTemplate)) {
		
		SECStatus lrv = pk11_setAttributes(targetSlot, targetTrustID, 
				   &sourceTemplate, 1);
		if (lrv != SECSuccess) {
		    rv = SECFailure;
		    error = PORT_GetError();
		}
	    }
	}

	
	sourceTemplate.type = CKA_TRUST_STEP_UP_APPROVED;
	sourceTemplate.pValue = NULL;
	sourceTemplate.ulValueLen = 0;

	
	PK11_GetAttributes(arena, sourceSlot, id, &sourceTemplate, 1);
	if ((sourceTemplate.ulValueLen == sizeof(CK_BBOOL)) && 
		(sourceTemplate.pValue) &&
		(*(CK_BBOOL *)sourceTemplate.pValue == CK_TRUE)) {
	    SECStatus lrv = pk11_setAttributes(targetSlot, targetTrustID, 
				   &sourceTemplate, 1);
	    if (lrv != SECSuccess) {
		rv = SECFailure;
		error = PORT_GetError();
	    }
	}

	goto done;

    }

    
    rv = pk11_copyAttributes(arena, targetSlot, targetTrustID, sourceSlot, id,
				trustCopyTemplate, trustCopyTemplateCount);
done:
    if (arena) {
         PORT_FreeArena(arena,PR_FALSE);
    }

    
    if (rv == SECFailure && error) {
	PORT_SetError(error);
    }
	
    return rv;
}









static SECStatus
pk11_mergeObject(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE id, void *targetPwArg, void *sourcePwArg)
{

    CK_OBJECT_CLASS objClass;


    objClass = PK11_ReadULongAttribute(sourceSlot, id, CKA_CLASS);
    if (objClass == (CK_ULONG) -1) {
	PORT_SetError( SEC_ERROR_UNKNOWN_OBJECT_TYPE );
	return SECFailure;
    }

    switch (objClass) {
    case CKO_CERTIFICATE:
	return pk11_mergeCert(targetSlot, sourceSlot, id, 
					targetPwArg, sourcePwArg);
    case CKO_NSS_TRUST:
	return pk11_mergeTrust(targetSlot, sourceSlot, id, 
					targetPwArg, sourcePwArg);
    case CKO_PUBLIC_KEY:
	return pk11_mergePublicKey(targetSlot, sourceSlot, id,
					targetPwArg, sourcePwArg);
    case CKO_PRIVATE_KEY:
	return pk11_mergePrivateKey(targetSlot, sourceSlot, id, 
					targetPwArg, sourcePwArg);
    case CKO_SECRET_KEY:
	return pk11_mergeSecretKey(targetSlot, sourceSlot, id, 
					targetPwArg, sourcePwArg);
    case CKO_NSS_CRL:
	return pk11_mergeCrl(targetSlot, sourceSlot, id, 
					targetPwArg, sourcePwArg);
    case CKO_NSS_SMIME:
	return pk11_mergeSmime(targetSlot, sourceSlot, id, 
					targetPwArg, sourcePwArg);
    default:
	break;
    }

    PORT_SetError( SEC_ERROR_UNKNOWN_OBJECT_TYPE );
    return SECFailure;
}

PK11MergeLogNode *
pk11_newMergeLogNode(PRArenaPool *arena, 
		     PK11SlotInfo *slot, CK_OBJECT_HANDLE id, int error)
{
    PK11MergeLogNode *newLog;
    PK11GenericObject *obj;

    newLog = PORT_ArenaZNew(arena, PK11MergeLogNode);
    if (newLog == NULL) {
	return NULL;
    }

    obj = PORT_ArenaZNew(arena, PK11GenericObject);
    if ( !obj ) {
	return NULL;
    }

    
    obj->slot = slot;
    obj->objectID = id;

    newLog->object= obj;
    newLog->error = error;
    return newLog;
}




static SECStatus
pk11_mergeByObjectIDs(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		CK_OBJECT_HANDLE *objectIDs, int count,
		PK11MergeLog *log, void *targetPwArg, void *sourcePwArg)
{
    SECStatus rv = SECSuccess;
    int error, i;
    
    for (i=0; i < count; i++) {
	

	SECStatus lrv;
	PK11MergeLogNode *newLog;

	lrv= pk11_mergeObject(targetSlot, sourceSlot, objectIDs[i], 
				targetPwArg, sourcePwArg);
	if (lrv == SECSuccess) {
	   
	   continue;
	}

	
	rv = SECFailure;
	error = PORT_GetError();

	
	if (!log) {
	    
	    continue;
	}
	newLog = pk11_newMergeLogNode(log->arena, sourceSlot, 
				      objectIDs[i], error);
	if (!newLog) {
	    
	    continue;
	}

	
	newLog->next = NULL;
	if (log->tail) {
	    log->tail->next = newLog;
	} else {
	    log->head = newLog;
	}
	newLog->prev = log->tail;
	log->tail = newLog;
    }

    
    if (rv != SECSuccess) {
	PORT_SetError(error);
    }
    return rv;
}











SECStatus
PK11_MergeTokens(PK11SlotInfo *targetSlot, PK11SlotInfo *sourceSlot,
		PK11MergeLog *log, void *targetPwArg, void *sourcePwArg)
{
    SECStatus rv = SECSuccess, lrv = SECSuccess;
    int error, count = 0;
    CK_ATTRIBUTE search[2];
    CK_OBJECT_HANDLE *objectIDs = NULL;
    CK_BBOOL ck_true = CK_TRUE;
    CK_OBJECT_CLASS privKey = CKO_PRIVATE_KEY;

    PK11_SETATTRS(&search[0], CKA_TOKEN, &ck_true, sizeof(ck_true));
    PK11_SETATTRS(&search[1], CKA_CLASS, &privKey, sizeof(privKey));
    


    rv = PK11_Authenticate(targetSlot, PR_TRUE, targetPwArg);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = PK11_Authenticate(sourceSlot, PR_TRUE, sourcePwArg);
    if (rv != SECSuccess) {
	goto loser;
    }

    

    objectIDs = pk11_FindObjectsByTemplate(sourceSlot, search, 2, &count);
    if (objectIDs) {
	lrv = pk11_mergeByObjectIDs(targetSlot, sourceSlot, 
				    objectIDs, count, log, 
				    targetPwArg, sourcePwArg);
	if (lrv != SECSuccess) {
	    error = PORT_GetError();
	}
	PORT_Free(objectIDs);
	count = 0;
    }

    


    objectIDs = pk11_FindObjectsByTemplate(sourceSlot, search, 1, &count);
    if (!objectIDs) {
	rv = SECFailure;
	goto loser;
    }

    rv = pk11_mergeByObjectIDs(targetSlot, sourceSlot, objectIDs, count, log, 
			targetPwArg, sourcePwArg);
    if (rv == SECSuccess) {
	



	if (lrv != SECSuccess) {
	    rv = lrv;
	    PORT_SetError(error);
	}
    }

loser:
    if (objectIDs) {
	PORT_Free(objectIDs);
    }
    return rv;
}

PK11MergeLog *
PK11_CreateMergeLog(void)
{
    PRArenaPool *arena;
    PK11MergeLog *log;

    arena = PORT_NewArena( DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	return NULL;
    }

    log = PORT_ArenaZNew(arena, PK11MergeLog);
    if (log == NULL) {
         PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }
    log->arena = arena;
    log->version = 1;
    return log;
}

void
PK11_DestroyMergeLog(PK11MergeLog *log)
{
   if (log && log->arena) {
	PORT_FreeArena(log->arena, PR_FALSE);
    }
}
