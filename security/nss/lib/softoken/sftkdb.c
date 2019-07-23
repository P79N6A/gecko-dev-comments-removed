


















































#include "sftkdb.h"
#include "sftkdbti.h"
#include "pkcs11t.h"
#include "pkcs11i.h"
#include "sdb.h"
#include "prprf.h" 
#include "secmodt.h"
#include "pratom.h"
#include "lgglue.h"
#include "sftkpars.h"
#include "secerr.h"
#include "softoken.h"










#define BBP 8

static PRBool
sftkdb_isULONGAttribute(CK_ATTRIBUTE_TYPE type) 
{
    switch(type) {
    case CKA_CERTIFICATE_CATEGORY:
    case CKA_CERTIFICATE_TYPE:
    case CKA_CLASS:
    case CKA_JAVA_MIDP_SECURITY_DOMAIN:
    case CKA_KEY_GEN_MECHANISM:
    case CKA_KEY_TYPE:
    case CKA_MECHANISM_TYPE:
    case CKA_MODULUS_BITS:
    case CKA_PRIME_BITS:
    case CKA_SUBPRIME_BITS:
    case CKA_VALUE_BITS:
    case CKA_VALUE_LEN:

    case CKA_TRUST_DIGITAL_SIGNATURE:
    case CKA_TRUST_NON_REPUDIATION:
    case CKA_TRUST_KEY_ENCIPHERMENT:
    case CKA_TRUST_DATA_ENCIPHERMENT:
    case CKA_TRUST_KEY_AGREEMENT:
    case CKA_TRUST_KEY_CERT_SIGN:
    case CKA_TRUST_CRL_SIGN:

    case CKA_TRUST_SERVER_AUTH:
    case CKA_TRUST_CLIENT_AUTH:
    case CKA_TRUST_CODE_SIGNING:
    case CKA_TRUST_EMAIL_PROTECTION:
    case CKA_TRUST_IPSEC_END_SYSTEM:
    case CKA_TRUST_IPSEC_TUNNEL:
    case CKA_TRUST_IPSEC_USER:
    case CKA_TRUST_TIME_STAMPING:
    case CKA_TRUST_STEP_UP_APPROVED:
	return PR_TRUE;
    default:
	break;
    }
    return PR_FALSE;
    
}


static PRBool
sftkdb_isPrivateAttribute(CK_ATTRIBUTE_TYPE type) 
{
    switch(type) {
    case CKA_VALUE:
    case CKA_PRIVATE_EXPONENT:
    case CKA_PRIME_1:
    case CKA_PRIME_2:
    case CKA_EXPONENT_1:
    case CKA_EXPONENT_2:
    case CKA_COEFFICIENT:
	return PR_TRUE;
    default:
	break;
    }
    return PR_FALSE;
}


static PRBool
sftkdb_isAuthenticatedAttribute(CK_ATTRIBUTE_TYPE type) 
{
    switch(type) {
    case CKA_MODULUS:
    case CKA_PUBLIC_EXPONENT:
    case CKA_CERT_SHA1_HASH:
    case CKA_CERT_MD5_HASH:
    case CKA_TRUST_SERVER_AUTH:
    case CKA_TRUST_CLIENT_AUTH:
    case CKA_TRUST_EMAIL_PROTECTION:
    case CKA_TRUST_CODE_SIGNING:
    case CKA_TRUST_STEP_UP_APPROVED:
    case CKA_NSS_OVERRIDE_EXTENSIONS:
	return PR_TRUE;
    default:
	break;
    }
    return PR_FALSE;
}





void
sftk_ULong2SDBULong(unsigned char *data, CK_ULONG value)
{ 
    int i;

    for (i=0; i < SDB_ULONG_SIZE; i++) {
	data[i] = (value >> (SDB_ULONG_SIZE-1-i)*BBP) & 0xff;
    }
}





static CK_ULONG
sftk_SDBULong2ULong(unsigned char *data)
{
    int i;
    CK_ULONG value = 0;

    for (i=0; i < SDB_ULONG_SIZE; i++) {
	value |= (((CK_ULONG)data[i]) << (SDB_ULONG_SIZE-1-i)*BBP);
    }
    return value;
}






static CK_ATTRIBUTE *
sftkdb_fixupTemplateIn(const CK_ATTRIBUTE *template, int count, 
			unsigned char **dataOut)
{
    int i;
    int ulongCount = 0;
    unsigned char *data;
    CK_ATTRIBUTE *ntemplate;

    *dataOut = NULL;

    
    for (i=0; i < count; i++) {
	
	if (!template[i].pValue) {
	    continue;
	}
	if (template[i].ulValueLen == sizeof (CK_ULONG)) {
	    if ( sftkdb_isULONGAttribute(template[i].type)) {
		ulongCount++;
	    }
	}
    }
    
    if (ulongCount == 0) {
	return (CK_ATTRIBUTE *)template;
    }

    
    data = (unsigned char *)PORT_Alloc(SDB_ULONG_SIZE*ulongCount);
    if (!data) {
	return NULL;
    }

    
    ntemplate = PORT_NewArray(CK_ATTRIBUTE,count);
    if (!ntemplate) {
	PORT_Free(data);
	return NULL;
    }
    *dataOut = data;
    
    for (i=0; i < count; i++) {
	ntemplate[i] = template[i];
	
	if (!template[i].pValue) {
	    continue;
	}
	if (template[i].ulValueLen == sizeof (CK_ULONG)) {
	    if ( sftkdb_isULONGAttribute(template[i].type) ) {
		CK_ULONG value = *(CK_ULONG *) template[i].pValue;
		sftk_ULong2SDBULong(data, value);
		ntemplate[i].pValue = data;
		ntemplate[i].ulValueLen = SDB_ULONG_SIZE;
		data += SDB_ULONG_SIZE;
	    }
	}
    }
    return ntemplate;
}


static const char SFTKDB_META_SIG_TEMPLATE[] = "sig_%s_%08x_%08x";




const char *
sftkdb_TypeString(SFTKDBHandle *handle)
{
   return (handle->type == SFTK_KEYDB_TYPE) ? "key" : "cert";
}













static CK_RV
sftkdb_getAttributeSignature(SFTKDBHandle *handle, SFTKDBHandle *keyHandle, 
		CK_OBJECT_HANDLE objectID, CK_ATTRIBUTE_TYPE type,
		SECItem *signText)
{
    SDB *db;
    char id[30];
    CK_RV crv;

    db = SFTK_GET_SDB(keyHandle);

    sprintf(id, SFTKDB_META_SIG_TEMPLATE,
	sftkdb_TypeString(handle),
	(unsigned int)objectID, (unsigned int)type);

    crv = (*db->sdb_GetMetaData)(db, id, signText, NULL);
    return crv;
}











CK_RV
sftkdb_PutAttributeSignature(SFTKDBHandle *handle, SDB *keyTarget, 
		CK_OBJECT_HANDLE objectID, CK_ATTRIBUTE_TYPE type,
		SECItem *signText)
{
    char id[30];
    CK_RV crv;

    sprintf(id, SFTKDB_META_SIG_TEMPLATE,
	sftkdb_TypeString(handle),
	(unsigned int)objectID, (unsigned int)type);

    crv = (*keyTarget->sdb_PutMetaData)(keyTarget, id, signText, NULL);
    return crv;
}





static CK_RV
sftkdb_fixupTemplateOut(CK_ATTRIBUTE *template, CK_OBJECT_HANDLE objectID,
		CK_ATTRIBUTE *ntemplate, int count, SFTKDBHandle *handle)
{
    int i;
    CK_RV crv = CKR_OK;
    SFTKDBHandle *keyHandle;
    PRBool checkSig = PR_TRUE;
    PRBool checkEnc = PR_TRUE;

    PORT_Assert(handle);

    
    keyHandle = handle;
    if (handle->type != SFTK_KEYDB_TYPE) {
	checkEnc = PR_FALSE;
	keyHandle = handle->peerDB;
    }

    if ((keyHandle == NULL) || 
	((SFTK_GET_SDB(keyHandle)->sdb_flags & SDB_HAS_META) == 0)  ||
	(keyHandle->passwordKey.data == NULL)) {
	checkSig = PR_FALSE;
    }

    for (i=0; i < count; i++) {
	CK_ULONG length = template[i].ulValueLen;
	template[i].ulValueLen = ntemplate[i].ulValueLen;
	
	if (ntemplate[i].ulValueLen == SDB_ULONG_SIZE) {
	    if (sftkdb_isULONGAttribute(template[i].type)) {
		if (template[i].pValue) {
		    CK_ULONG value;
		    unsigned char *data;

		    data = (unsigned char *)ntemplate[i].pValue;
		    value = sftk_SDBULong2ULong(ntemplate[i].pValue);
		    if (length < sizeof(CK_ULONG)) {
			template[i].ulValueLen = -1;
			crv = CKR_BUFFER_TOO_SMALL;
			continue;
		    } 
		    PORT_Memcpy(template[i].pValue,&value,sizeof(CK_ULONG));
		}
		template[i].ulValueLen = sizeof(CK_ULONG);
	    }
	}

	

	if ((template[i].pValue == NULL) || (template[i].ulValueLen == -1)) {
	    continue;
	}

	
	if (checkEnc && sftkdb_isPrivateAttribute(ntemplate[i].type)) {
	    
	    

	    SECItem cipherText;
	    SECItem *plainText;
	    SECStatus rv;

	    cipherText.data = ntemplate[i].pValue;
	    cipherText.len = ntemplate[i].ulValueLen;
    	    PZ_Lock(handle->passwordLock);
	    if (handle->passwordKey.data == NULL) {
		PZ_Unlock(handle->passwordLock);
		template[i].ulValueLen = -1;
		crv = CKR_USER_NOT_LOGGED_IN;
		continue;
	    }
	    rv = sftkdb_DecryptAttribute(&handle->passwordKey, 
					&cipherText, &plainText);
	    PZ_Unlock(handle->passwordLock);
	    if (rv != SECSuccess) {
		PORT_Memset(template[i].pValue, 0, template[i].ulValueLen);
		template[i].ulValueLen = -1;
		crv = CKR_GENERAL_ERROR;
		continue;
	    }
	    PORT_Assert(template[i].ulValueLen >= plainText->len);
	    if (template[i].ulValueLen < plainText->len) {
		SECITEM_FreeItem(plainText,PR_TRUE);
		PORT_Memset(template[i].pValue, 0, template[i].ulValueLen);
		template[i].ulValueLen = -1;
		crv = CKR_GENERAL_ERROR;
		continue;
	    }
		
	    
	    PORT_Memcpy(template[i].pValue, plainText->data, plainText->len);
	    template[i].ulValueLen = plainText->len;
	    SECITEM_FreeItem(plainText,PR_TRUE);
	}
	
	if (checkSig && sftkdb_isAuthenticatedAttribute(ntemplate[i].type)) {
	    SECStatus rv;
	    SECItem signText;
	    SECItem plainText;
	    unsigned char signData[SDB_MAX_META_DATA_LEN];

	    signText.data = signData;
	    signText.len = sizeof(signData);

	    rv = sftkdb_getAttributeSignature(handle, keyHandle, 
				objectID, ntemplate[i].type, &signText);
	    if (rv != SECSuccess) {
		PORT_Memset(template[i].pValue, 0, template[i].ulValueLen);
		template[i].ulValueLen = -1;
		crv = CKR_DATA_INVALID; 
		continue;
	    }

	    plainText.data = ntemplate[i].pValue;
	    plainText.len = ntemplate[i].ulValueLen;

	    



    	    PZ_Lock(keyHandle->passwordLock);
	    if (keyHandle->passwordKey.data == NULL) {
		

		checkSig = PR_FALSE; 
		PZ_Unlock(keyHandle->passwordLock);
		continue;
	    }

	    rv = sftkdb_VerifyAttribute(&keyHandle->passwordKey, 
				objectID, ntemplate[i].type,
				&plainText, &signText);
	    PZ_Unlock(keyHandle->passwordLock);
	    if (rv != SECSuccess) {
		PORT_Memset(template[i].pValue, 0, template[i].ulValueLen);
		template[i].ulValueLen = -1;
		crv = CKR_SIGNATURE_INVALID; 
	    }
	    
	}
    }
    return crv;
}

































static CK_RV
sftk_signTemplate(PLArenaPool *arena, SFTKDBHandle *handle, 
		  PRBool mayBeUpdateDB,
		  CK_OBJECT_HANDLE objectID, const CK_ATTRIBUTE *template,
		  CK_ULONG count)
{
    int i;
    SFTKDBHandle *keyHandle = handle;
    SDB *keyTarget = NULL;

    PORT_Assert(handle);

    if (handle->type != SFTK_KEYDB_TYPE) {
	keyHandle = handle->peerDB;
    }

    
    if (keyHandle == NULL) {
	return CKR_OK;
    }

    



    keyTarget = (mayBeUpdateDB && keyHandle->update) ? 
		keyHandle->update : keyHandle->db;

    
    if ((keyTarget->sdb_flags & SDB_HAS_META) == 0) {
	return CKR_OK;
    }

    for (i=0; i < count; i ++) {
	if (sftkdb_isAuthenticatedAttribute(template[i].type)) {
	    SECStatus rv;
	    SECItem *signText;
	    SECItem plainText;

	    plainText.data = template[i].pValue;
	    plainText.len = template[i].ulValueLen;
	    PZ_Lock(keyHandle->passwordLock);
	    if (keyHandle->passwordKey.data == NULL) {
		PZ_Unlock(keyHandle->passwordLock);
		return CKR_USER_NOT_LOGGED_IN;
	    }
	    rv = sftkdb_SignAttribute(arena, &keyHandle->passwordKey, 
				objectID, template[i].type,
				&plainText, &signText);
	    PZ_Unlock(keyHandle->passwordLock);
	    if (rv != SECSuccess) {
		return CKR_GENERAL_ERROR; 
	    }
	    rv = sftkdb_PutAttributeSignature(handle, keyTarget, 
				objectID, template[i].type, signText);
	    if (rv != SECSuccess) {
		return CKR_GENERAL_ERROR; 
	    }
	}
    }
    return CKR_OK;
}

static CK_RV
sftkdb_CreateObject(PRArenaPool *arena, SFTKDBHandle *handle, 
	SDB *db, CK_OBJECT_HANDLE *objectID,
        CK_ATTRIBUTE *template, CK_ULONG count)
{
    PRBool inTransaction = PR_FALSE;
    CK_RV crv;

    inTransaction = PR_TRUE;
    
    crv = (*db->sdb_CreateObject)(db, objectID, template, count);
    if (crv != CKR_OK) {
	goto loser;
    }
    crv = sftk_signTemplate(arena, handle, (db == handle->update),
					*objectID, template, count);
loser:

    return crv;
}


CK_ATTRIBUTE * 
sftk_ExtractTemplate(PLArenaPool *arena, SFTKObject *object, 
		     SFTKDBHandle *handle,CK_ULONG *pcount, 
		     CK_RV *crv)
{
    int count;
    CK_ATTRIBUTE *template;
    int i, templateIndex;
    SFTKSessionObject *sessObject = sftk_narrowToSessionObject(object);
    PRBool doEnc = PR_TRUE;

    *crv = CKR_OK;

    if (sessObject == NULL) {
	*crv = CKR_GENERAL_ERROR; 
	return NULL;
    }

    PORT_Assert(handle);
    
    if (handle->type != SFTK_KEYDB_TYPE) {
	doEnc = PR_FALSE;
    }

    PZ_Lock(sessObject->attributeLock);
    count = 0;
    for (i=0; i < sessObject->hashSize; i++) {
	SFTKAttribute *attr;
   	for (attr=sessObject->head[i]; attr; attr=attr->next) {
	    count++;
	}
    }
    template = PORT_ArenaNewArray(arena, CK_ATTRIBUTE, count);
    if (template == NULL) {
        PZ_Unlock(sessObject->attributeLock);
	*crv = CKR_HOST_MEMORY;
	return NULL;
    }
    templateIndex = 0;
    for (i=0; i < sessObject->hashSize; i++) {
	SFTKAttribute *attr;
   	for (attr=sessObject->head[i]; attr; attr=attr->next) {
	    CK_ATTRIBUTE *tp = &template[templateIndex++];
	    
	    *tp = attr->attrib;

	    
	    if ((tp->ulValueLen == sizeof (CK_ULONG)) &&
		(sftkdb_isULONGAttribute(tp->type)) ) {
		CK_ULONG value = *(CK_ULONG *) tp->pValue;
		unsigned char *data;

		tp->pValue = PORT_ArenaAlloc(arena, SDB_ULONG_SIZE);
		data = (unsigned char *)tp->pValue;
		if (data == NULL) {
		    *crv = CKR_HOST_MEMORY;
		    break;
		}
		sftk_ULong2SDBULong(data, value);
		tp->ulValueLen = SDB_ULONG_SIZE;
	    }

	    
	    if (doEnc && sftkdb_isPrivateAttribute(tp->type)) {
		
		SECItem *cipherText;
		SECItem plainText;
		SECStatus rv;

		plainText.data = tp->pValue;
		plainText.len = tp->ulValueLen;
		PZ_Lock(handle->passwordLock);
		if (handle->passwordKey.data == NULL) {
		    PZ_Unlock(handle->passwordLock);
		    *crv = CKR_USER_NOT_LOGGED_IN;
		    break;
		}
		rv = sftkdb_EncryptAttribute(arena, &handle->passwordKey, 
						&plainText, &cipherText);
		PZ_Unlock(handle->passwordLock);
		if (rv == SECSuccess) {
		    tp->pValue = cipherText->data;
		    tp->ulValueLen = cipherText->len;
		} else {
		    *crv = CKR_GENERAL_ERROR; 
		    break;
		}
		PORT_Memset(plainText.data, 0, plainText.len);
	    }
	}
    }
    PORT_Assert(templateIndex <= count);
    PZ_Unlock(sessObject->attributeLock);

    if (*crv != CKR_OK) {
	return NULL;
    }
    if (pcount) {
	*pcount = count;
    }
    return template;

}







static CK_ATTRIBUTE *
sftkdb_getAttributeFromTemplate(CK_ATTRIBUTE_TYPE attribute, 
			    CK_ATTRIBUTE *ptemplate, CK_ULONG len)
{
    CK_ULONG i;

    for (i=0; i < len; i++) {
	if (attribute == ptemplate[i].type) {
	    return &ptemplate[i];
	}
    }
    return NULL;
}

static const CK_ATTRIBUTE *
sftkdb_getAttributeFromConstTemplate(CK_ATTRIBUTE_TYPE attribute, 
				const CK_ATTRIBUTE *ptemplate, CK_ULONG len)
{
    CK_ULONG i;

    for (i=0; i < len; i++) {
	if (attribute == ptemplate[i].type) {
	    return &ptemplate[i];
	}
    }
    return NULL;
}





static CK_RV
sftkdb_getFindTemplate(CK_OBJECT_CLASS objectType, unsigned char *objTypeData,
			CK_ATTRIBUTE *findTemplate, CK_ULONG *findCount,
			CK_ATTRIBUTE *ptemplate, int len)
{
    CK_ATTRIBUTE *attr;
    CK_ULONG count = 1;

    sftk_ULong2SDBULong(objTypeData, objectType);
    findTemplate[0].type = CKA_CLASS;
    findTemplate[0].pValue = objTypeData;
    findTemplate[0].ulValueLen = SDB_ULONG_SIZE;

    switch (objectType) {
    case CKO_CERTIFICATE:
    case CKO_NSS_TRUST:
	attr = sftkdb_getAttributeFromTemplate(CKA_ISSUER, ptemplate, len);
	if (attr == NULL) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	findTemplate[1] = *attr;
	attr = sftkdb_getAttributeFromTemplate(CKA_SERIAL_NUMBER, 
					ptemplate, len);
	if (attr == NULL) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	findTemplate[2] = *attr;
	count = 3;
	break;
	
    case CKO_PRIVATE_KEY:
    case CKO_PUBLIC_KEY:
    case CKO_SECRET_KEY:
	attr = sftkdb_getAttributeFromTemplate(CKA_ID, ptemplate, len);
	if (attr == NULL) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	findTemplate[1] = *attr;
	count = 2;
	break;

    case CKO_NSS_CRL:
	attr = sftkdb_getAttributeFromTemplate(CKA_SUBJECT, ptemplate, len);
	if (attr == NULL) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	findTemplate[1] = *attr;
	count = 2;
	break;

    case CKO_NSS_SMIME:
	attr = sftkdb_getAttributeFromTemplate(CKA_SUBJECT, ptemplate, len);
	if (attr == NULL) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	findTemplate[1] = *attr;
	attr = sftkdb_getAttributeFromTemplate(CKA_NSS_EMAIL, ptemplate, len);
	if (attr == NULL) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	findTemplate[2] = *attr;
	count = 3;
	break;
    default:
	attr = sftkdb_getAttributeFromTemplate(CKA_VALUE, ptemplate, len);
	if (attr == NULL) {
	    return CKR_TEMPLATE_INCOMPLETE;
	}
	findTemplate[1] = *attr;
	count = 2;
	break;
    }
    *findCount = count;

    return CKR_OK;
}





static CK_RV
sftkdb_lookupObject(SDB *db, CK_OBJECT_CLASS objectType, 
		 CK_OBJECT_HANDLE *id, CK_ATTRIBUTE *ptemplate, CK_ULONG len)
{
    CK_ATTRIBUTE findTemplate[3];
    CK_ULONG count = 1;
    CK_ULONG objCount = 0;
    SDBFind *find = NULL;
    unsigned char objTypeData[SDB_ULONG_SIZE];
    CK_RV crv;

    *id = CK_INVALID_HANDLE;
    if (objectType == CKO_NSS_CRL) {
	return CKR_OK;
    }
    crv = sftkdb_getFindTemplate(objectType, objTypeData,
			findTemplate, &count, ptemplate, len);
    if (crv != CKR_OK) {
	return crv;
    }

    
    crv = (*db->sdb_FindObjectsInit)(db, findTemplate, count, &find);
    if (crv != CKR_OK) {
	return crv;
    }
    (*db->sdb_FindObjects)(db, find, id, 1, &objCount);
    (*db->sdb_FindObjectsFinal)(db, find);

    if (objCount == 0) {
	*id = CK_INVALID_HANDLE;
    }
    return CKR_OK;
}





static CK_RV
sftkdb_checkConflicts(SDB *db, CK_OBJECT_CLASS objectType, 
			const CK_ATTRIBUTE *ptemplate, CK_ULONG len, 
			CK_OBJECT_HANDLE sourceID)
{
    CK_ATTRIBUTE findTemplate[2];
    unsigned char objTypeData[SDB_ULONG_SIZE];
    

    unsigned char *temp1 = NULL; 
    unsigned char *temp2 = NULL;
    CK_ULONG objCount = 0;
    SDBFind *find = NULL;
    CK_OBJECT_HANDLE id;
    const CK_ATTRIBUTE *attr, *attr2;
    CK_RV crv;
    CK_ATTRIBUTE subject;

    

    
    if (objectType != CKO_CERTIFICATE) {
	return CKR_OK;
    }
    
    attr = sftkdb_getAttributeFromConstTemplate(CKA_LABEL, ptemplate, len);
    if ((attr == NULL) || (attr->ulValueLen == 0)) {
	return CKR_OK;
    }
    

    attr2 = sftkdb_getAttributeFromConstTemplate(CKA_SUBJECT, ptemplate, len);
    if (sourceID == CK_INVALID_HANDLE) {
	if ((attr2 == NULL) || ((CK_LONG)attr2->ulValueLen < 0)) {
	    crv = CKR_TEMPLATE_INCOMPLETE; 
	    goto done;
	}
    } else if ((attr2 == NULL) || ((CK_LONG)attr2->ulValueLen <= 0)) {
	


    	subject.type = CKA_SUBJECT;
    	subject.pValue = NULL;
    	subject.ulValueLen = 0;
    	crv = (*db->sdb_GetAttributeValue)(db, sourceID, &subject, 1);
	if (crv != CKR_OK) {
	    goto done;
	}
	if ((CK_LONG)subject.ulValueLen < 0) {
	    crv = CKR_DEVICE_ERROR; 
	    goto done;
	}
	temp1 = subject.pValue = PORT_Alloc(++subject.ulValueLen);
	if (temp1 == NULL) {
	    crv = CKR_HOST_MEMORY;
	    goto done;
	}
    	crv = (*db->sdb_GetAttributeValue)(db, sourceID, &subject, 1);
	if (crv != CKR_OK) {
	    goto done;
	}
	attr2 = &subject;
    }
    
    
    sftk_ULong2SDBULong(objTypeData, objectType);
    findTemplate[0].type = CKA_CLASS;
    findTemplate[0].pValue = objTypeData;
    findTemplate[0].ulValueLen = SDB_ULONG_SIZE;
    findTemplate[1] = *attr;

    crv = (*db->sdb_FindObjectsInit)(db, findTemplate, 2, &find);
    if (crv != CKR_OK) {
	goto done;
    }
    (*db->sdb_FindObjects)(db, find, &id, 1, &objCount);
    (*db->sdb_FindObjectsFinal)(db, find);

    

    if (objCount == 0) {
	crv = CKR_OK;
	goto done;
    }

    

    findTemplate[0] = *attr2;
    




    temp2 = findTemplate[0].pValue = PORT_Alloc(++findTemplate[0].ulValueLen);
    if (temp2 == NULL) {
	crv = CKR_HOST_MEMORY;
	goto done;
    }
    crv = (*db->sdb_GetAttributeValue)(db, id, findTemplate, 1);
    if (crv != CKR_OK) {
	if (crv == CKR_BUFFER_TOO_SMALL) {
	    

	    crv = CKR_ATTRIBUTE_VALUE_INVALID;
	    goto loser;
	}
	
	goto done;
    }
	
    

    if ((findTemplate[0].ulValueLen != attr2->ulValueLen) || 
	(attr2->ulValueLen > 0 &&
	 PORT_Memcmp(findTemplate[0].pValue, attr2->pValue, attr2->ulValueLen) 
	 != 0)) {
    	crv = CKR_ATTRIBUTE_VALUE_INVALID; 
	goto loser;
    }
    crv = CKR_OK;
    
done:
    




    if (crv == CKR_ATTRIBUTE_VALUE_INVALID) {
	crv = CKR_GENERAL_ERROR; 
    }

    
loser:
    PORT_Free(temp1);
    PORT_Free(temp2);
    return crv;
}













static CK_RV
sftkdb_resolveConflicts(PRArenaPool *arena, CK_OBJECT_CLASS objectType, 
			CK_ATTRIBUTE *ptemplate, CK_ULONG *plen)
{
    CK_ATTRIBUTE *attr;
    char *nickname, *newNickname;
    int end, digit;

    
    if (objectType != CKO_CERTIFICATE) {
	return CKR_GENERAL_ERROR; 
    }
    attr = sftkdb_getAttributeFromTemplate(CKA_LABEL, ptemplate, *plen);
    if ((attr == NULL) || (attr->ulValueLen == 0)) {
	return CKR_GENERAL_ERROR; 
    }

    
    

    nickname = (char *)attr->pValue;

    
    for (end = attr->ulValueLen - 1; 
         end >= 2 && (digit = nickname[end]) <= '9' &&  digit >= '0'; 
	 end--)   ;
    if (attr->ulValueLen >= 3 &&
        end < (attr->ulValueLen - 1)  &&
	nickname[end]     == '#'  && 
	nickname[end - 1] == ' ') {
    	
    } else {
	
	static const char num2[] = " #2";
	newNickname = PORT_ArenaAlloc(arena, attr->ulValueLen + sizeof(num2));
	if (!newNickname) {
	    return CKR_HOST_MEMORY;
	}
	PORT_Memcpy(newNickname, nickname, attr->ulValueLen);
	PORT_Memcpy(&newNickname[attr->ulValueLen], num2, sizeof(num2));
	attr->pValue = newNickname; 
	attr->ulValueLen += 3;      
	return CKR_OK;
    }

    for (end = attr->ulValueLen - 1; 
	 end >= 0 && (digit = nickname[end]) <= '9' &&  digit >= '0'; 
	 end--) {
	if (digit < '9') {
	    nickname[end]++;
	    return CKR_OK;
	}
	nickname[end] = '0';
    }

    
    newNickname = PORT_ArenaAlloc(arena, attr->ulValueLen + 1);
    if (!newNickname) {
	return CKR_HOST_MEMORY;
    }
    
    PORT_Memcpy(newNickname, nickname, ++end);
    newNickname[end] = '1';
    PORT_Memset(&newNickname[end+1],'0',attr->ulValueLen - end);
    attr->pValue = newNickname;
    attr->ulValueLen++;
    return CKR_OK;
}




static CK_RV
sftkdb_setAttributeValue(PRArenaPool *arena, SFTKDBHandle *handle, 
	SDB *db, CK_OBJECT_HANDLE objectID, const CK_ATTRIBUTE *template, 
	CK_ULONG count)
{
    CK_RV crv;
    crv = (*db->sdb_SetAttributeValue)(db, objectID, template, count);
    if (crv != CKR_OK) {
	return crv;
    }
    crv = sftk_signTemplate(arena, handle, db == handle->update, 
				objectID, template, count);
    return crv;
}




CK_RV
sftkdb_write(SFTKDBHandle *handle, SFTKObject *object, 
	     CK_OBJECT_HANDLE *objectID)
{
    CK_ATTRIBUTE *template;
    PLArenaPool *arena;
    CK_ULONG count;
    CK_RV crv;
    SDB *db;
    PRBool inTransaction = PR_FALSE;
    CK_OBJECT_HANDLE id;

    *objectID = CK_INVALID_HANDLE;

    if (handle == NULL) {
	return  CKR_TOKEN_WRITE_PROTECTED;
    }
    db = SFTK_GET_SDB(handle);

    






    if (db == handle->update) {
	return CKR_USER_NOT_LOGGED_IN;
    }

    arena = PORT_NewArena(256);
    if (arena ==  NULL) {
	return CKR_HOST_MEMORY;
    }

    template = sftk_ExtractTemplate(arena, object, handle, &count, &crv);
    if (!template) {
	goto loser;
    }

    crv = (*db->sdb_Begin)(db);
    if (crv != CKR_OK) {
	goto loser;
    }
    inTransaction = PR_TRUE;

    














    
    crv = sftkdb_checkConflicts(db, object->objclass, template, count,
				 CK_INVALID_HANDLE);
    if (crv != CKR_OK) {
	goto loser;
    }
    
    crv = sftkdb_lookupObject(db, object->objclass, &id, template, count);
    if (crv != CKR_OK) {
	goto loser;
    }
    if (id == CK_INVALID_HANDLE) {
        crv = sftkdb_CreateObject(arena, handle, db, objectID, template, count);
    } else {
	
	*objectID = id;
        crv = sftkdb_setAttributeValue(arena, handle, db, id, template, count);
    }
    if (crv != CKR_OK) {
	goto loser;
    }

    crv = (*db->sdb_Commit)(db);
    inTransaction = PR_FALSE;

loser:
    if (inTransaction) {
	(*db->sdb_Abort)(db);
	


	PORT_Assert(crv != CKR_OK);
	if (crv == CKR_OK) crv = CKR_GENERAL_ERROR;
    }

    if (arena) {
	PORT_FreeArena(arena,PR_FALSE);
    }
    if (crv == CKR_OK) {
	*objectID |= (handle->type | SFTK_TOKEN_TYPE);
    } 
    return crv;
}


CK_RV 
sftkdb_FindObjectsInit(SFTKDBHandle *handle, const CK_ATTRIBUTE *template,
				 CK_ULONG count, SDBFind **find) 
{
    unsigned char *data = NULL;
    CK_ATTRIBUTE *ntemplate = NULL;
    CK_RV crv;
    SDB *db;

    if (handle == NULL) {
	return CKR_OK;
    }
    db = SFTK_GET_SDB(handle);

    if (count !=  0) {
	ntemplate = sftkdb_fixupTemplateIn(template, count, &data);
	if (ntemplate == NULL) {
	    return CKR_HOST_MEMORY;
	}
    }
	
    crv = (*db->sdb_FindObjectsInit)(db, ntemplate, 
					     count, find);
    if (data) {
	PORT_Free(ntemplate);
	PORT_Free(data);
    }
    return crv;
}

CK_RV 
sftkdb_FindObjects(SFTKDBHandle *handle, SDBFind *find, 
			CK_OBJECT_HANDLE *ids, int arraySize, CK_ULONG *count)
{
    CK_RV crv;
    SDB *db;

    if (handle == NULL) {
	*count = 0;
	return CKR_OK;
    }
    db = SFTK_GET_SDB(handle);

    crv = (*db->sdb_FindObjects)(db, find, ids, 
					    arraySize, count);
    if (crv == CKR_OK) {
	int i;
	for (i=0; i < *count; i++) {
	    ids[i] |= (handle->type | SFTK_TOKEN_TYPE);
	}
    }
    return crv;
}

CK_RV sftkdb_FindObjectsFinal(SFTKDBHandle *handle, SDBFind *find)
{
    SDB *db;
    if (handle == NULL) {
	return CKR_OK;
    }
    db = SFTK_GET_SDB(handle);
    return (*db->sdb_FindObjectsFinal)(db, find);
}

CK_RV
sftkdb_GetAttributeValue(SFTKDBHandle *handle, CK_OBJECT_HANDLE objectID,
                                CK_ATTRIBUTE *template, CK_ULONG count)
{
    CK_RV crv,crv2;
    CK_ATTRIBUTE *ntemplate;
    unsigned char *data = NULL;
    SDB *db;

    if (handle == NULL) {
	return CKR_GENERAL_ERROR;
    }

    
    if (count == 1 && 
	  (template[0].type == CKA_TOKEN || 
	   template[0].type == CKA_PRIVATE ||
	   template[0].type == CKA_SENSITIVE)) {
	CK_BBOOL boolVal = CK_TRUE;

	if (template[0].pValue == NULL) {
	    template[0].ulValueLen = sizeof(CK_BBOOL);
	    return CKR_OK;
	}
	if (template[0].ulValueLen < sizeof(CK_BBOOL)) {
	    template[0].ulValueLen = -1;
	    return CKR_BUFFER_TOO_SMALL;
	}

	if ((template[0].type == CKA_PRIVATE) &&
    				(handle->type != SFTK_KEYDB_TYPE)) {
	    boolVal = CK_FALSE;
	}
	if ((template[0].type == CKA_SENSITIVE) &&
    				(handle->type != SFTK_KEYDB_TYPE)) {
	    boolVal = CK_FALSE;
	}
	*(CK_BBOOL *)template[0].pValue = boolVal;
	template[0].ulValueLen = sizeof(CK_BBOOL);
	return CKR_OK;
    }

    db = SFTK_GET_SDB(handle);
    
    if (count == 0) {
	return CKR_OK;
    }
    ntemplate = sftkdb_fixupTemplateIn(template, count, &data);
    if (ntemplate == NULL) {
	return CKR_HOST_MEMORY;
    }
    objectID &= SFTK_OBJ_ID_MASK;
    crv = (*db->sdb_GetAttributeValue)(db, objectID, 
						ntemplate, count);
    crv2 = sftkdb_fixupTemplateOut(template, objectID, ntemplate, 
						count, handle);
    if (crv == CKR_OK) crv = crv2;
    if (data) {
	PORT_Free(ntemplate);
	PORT_Free(data);
    }
    return crv;

}

CK_RV
sftkdb_SetAttributeValue(SFTKDBHandle *handle, SFTKObject *object,
                                const CK_ATTRIBUTE *template, CK_ULONG count)
{
    CK_ATTRIBUTE *ntemplate;
    unsigned char *data = NULL;
    PLArenaPool *arena = NULL;
    SDB *db;
    CK_RV crv = CKR_OK;
    CK_OBJECT_HANDLE objectID = (object->handle & SFTK_OBJ_ID_MASK);
    PRBool inTransaction = PR_FALSE;

    if (handle == NULL) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    db = SFTK_GET_SDB(handle);
    
    if (count == 0) {
	return CKR_OK;
    }
    






    if (db == handle->update) {
	return CKR_USER_NOT_LOGGED_IN;
    }

    ntemplate = sftkdb_fixupTemplateIn(template, count, &data);
    if (ntemplate == NULL) {
	return CKR_HOST_MEMORY;
    }

    
    crv = sftkdb_checkConflicts(db, object->objclass, template, count, objectID);
    if (crv != CKR_OK) {
	goto loser;
    }

    arena = PORT_NewArena(256);
    if (arena ==  NULL) {
	crv = CKR_HOST_MEMORY;
	goto loser;
    }

    crv = (*db->sdb_Begin)(db);
    if (crv != CKR_OK) {
	goto loser;
    }
    inTransaction = PR_TRUE;
    crv = sftkdb_setAttributeValue(arena, handle, db, 
				   objectID, template, count);
    if (crv != CKR_OK) {
	goto loser;
    }
    crv = (*db->sdb_Commit)(db);
loser:
    if (crv != CKR_OK && inTransaction) {
	(*db->sdb_Abort)(db);
    }
    if (data) {
	PORT_Free(ntemplate);
	PORT_Free(data);
    }
    if (arena) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    return crv;
}

CK_RV
sftkdb_DestroyObject(SFTKDBHandle *handle, CK_OBJECT_HANDLE objectID)
{
    CK_RV crv = CKR_OK;
    SDB *db;

    if (handle == NULL) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }
    db = SFTK_GET_SDB(handle);
    objectID &= SFTK_OBJ_ID_MASK;
    crv = (*db->sdb_Begin)(db);
    if (crv != CKR_OK) {
	goto loser;
    }
    crv = (*db->sdb_DestroyObject)(db, objectID);
    if (crv != CKR_OK) {
	goto loser;
    }
    crv = (*db->sdb_Commit)(db);
loser:
    if (crv != CKR_OK) {
	(*db->sdb_Abort)(db);
    }
    return crv;
}

CK_RV
sftkdb_CloseDB(SFTKDBHandle *handle)
{
#ifdef NO_FORK_CHECK
    PRBool parentForkedAfterC_Initialize = PR_FALSE;
#endif
    if (handle == NULL) {
	return CKR_OK;
    }
    if (handle->update) {
        if (handle->db->sdb_SetForkState) {
            (*handle->db->sdb_SetForkState)(parentForkedAfterC_Initialize);
        }
	(*handle->update->sdb_Close)(handle->update);
    }
    if (handle->db) {
        if (handle->db->sdb_SetForkState) {
            (*handle->db->sdb_SetForkState)(parentForkedAfterC_Initialize);
        }
	(*handle->db->sdb_Close)(handle->db);
    }
    if (handle->passwordLock) {
	SKIP_AFTER_FORK(PZ_DestroyLock(handle->passwordLock));
    }
    if (handle->updatePasswordKey) {
	SECITEM_FreeItem(handle->updatePasswordKey, PR_TRUE);
    }
    if (handle->updateID) {
	PORT_Free(handle->updateID);
    }
    PORT_Free(handle);
    return CKR_OK;
}




static CK_RV
sftkdb_ResetDB(SFTKDBHandle *handle)
{
    CK_RV crv = CKR_OK;
    SDB *db;
    if (handle == NULL) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }
    db = SFTK_GET_SDB(handle);
    crv = (*db->sdb_Begin)(db);
    if (crv != CKR_OK) {
	goto loser;
    }
    crv = (*db->sdb_Reset)(db);
    if (crv != CKR_OK) {
	goto loser;
    }
    crv = (*db->sdb_Commit)(db);
loser:
    if (crv != CKR_OK) {
	(*db->sdb_Abort)(db);
    }
    return crv;
}


CK_RV
sftkdb_Begin(SFTKDBHandle *handle)
{
    CK_RV crv = CKR_OK;
    SDB *db;

    if (handle == NULL) {
	return CKR_OK;
    }
    db = SFTK_GET_SDB(handle);
    if (db) {
	crv = (*db->sdb_Begin)(db);
    }
    return crv;
}

CK_RV
sftkdb_Commit(SFTKDBHandle *handle)
{
    CK_RV crv = CKR_OK;
    SDB *db;

    if (handle == NULL) {
	return CKR_OK;
    }
    db = SFTK_GET_SDB(handle);
    if (db) {
	(*db->sdb_Commit)(db);
    }
    return crv;
}

CK_RV
sftkdb_Abort(SFTKDBHandle *handle)
{
    CK_RV crv = CKR_OK;
    SDB *db;

    if (handle == NULL) {
	return CKR_OK;
    }
    db = SFTK_GET_SDB(handle);
    if (db) {
	crv = (db->sdb_Abort)(db);
    }
    return crv;
}









static const CK_ATTRIBUTE_TYPE known_attributes[] = {
    CKA_CLASS, CKA_TOKEN, CKA_PRIVATE, CKA_LABEL, CKA_APPLICATION,
    CKA_VALUE, CKA_OBJECT_ID, CKA_CERTIFICATE_TYPE, CKA_ISSUER,
    CKA_SERIAL_NUMBER, CKA_AC_ISSUER, CKA_OWNER, CKA_ATTR_TYPES, CKA_TRUSTED,
    CKA_CERTIFICATE_CATEGORY, CKA_JAVA_MIDP_SECURITY_DOMAIN, CKA_URL,
    CKA_HASH_OF_SUBJECT_PUBLIC_KEY, CKA_HASH_OF_ISSUER_PUBLIC_KEY,
    CKA_CHECK_VALUE, CKA_KEY_TYPE, CKA_SUBJECT, CKA_ID, CKA_SENSITIVE,
    CKA_ENCRYPT, CKA_DECRYPT, CKA_WRAP, CKA_UNWRAP, CKA_SIGN, CKA_SIGN_RECOVER,
    CKA_VERIFY, CKA_VERIFY_RECOVER, CKA_DERIVE, CKA_START_DATE, CKA_END_DATE,
    CKA_MODULUS, CKA_MODULUS_BITS, CKA_PUBLIC_EXPONENT, CKA_PRIVATE_EXPONENT,
    CKA_PRIME_1, CKA_PRIME_2, CKA_EXPONENT_1, CKA_EXPONENT_2, CKA_COEFFICIENT,
    CKA_PRIME, CKA_SUBPRIME, CKA_BASE, CKA_PRIME_BITS, 
    CKA_SUB_PRIME_BITS, CKA_VALUE_BITS, CKA_VALUE_LEN, CKA_EXTRACTABLE,
    CKA_LOCAL, CKA_NEVER_EXTRACTABLE, CKA_ALWAYS_SENSITIVE,
    CKA_KEY_GEN_MECHANISM, CKA_MODIFIABLE, CKA_EC_PARAMS,
    CKA_EC_POINT, CKA_SECONDARY_AUTH, CKA_AUTH_PIN_FLAGS,
    CKA_ALWAYS_AUTHENTICATE, CKA_WRAP_WITH_TRUSTED, CKA_WRAP_TEMPLATE,
    CKA_UNWRAP_TEMPLATE, CKA_HW_FEATURE_TYPE, CKA_RESET_ON_INIT,
    CKA_HAS_RESET, CKA_PIXEL_X, CKA_PIXEL_Y, CKA_RESOLUTION, CKA_CHAR_ROWS,
    CKA_CHAR_COLUMNS, CKA_COLOR, CKA_BITS_PER_PIXEL, CKA_CHAR_SETS,
    CKA_ENCODING_METHODS, CKA_MIME_TYPES, CKA_MECHANISM_TYPE,
    CKA_REQUIRED_CMS_ATTRIBUTES, CKA_DEFAULT_CMS_ATTRIBUTES,
    CKA_SUPPORTED_CMS_ATTRIBUTES, CKA_NSS_URL, CKA_NSS_EMAIL,
    CKA_NSS_SMIME_INFO, CKA_NSS_SMIME_TIMESTAMP,
    CKA_NSS_PKCS8_SALT, CKA_NSS_PASSWORD_CHECK, CKA_NSS_EXPIRES,
    CKA_NSS_KRL, CKA_NSS_PQG_COUNTER, CKA_NSS_PQG_SEED,
    CKA_NSS_PQG_H, CKA_NSS_PQG_SEED_BITS, CKA_NSS_MODULE_SPEC,
    CKA_TRUST_DIGITAL_SIGNATURE, CKA_TRUST_NON_REPUDIATION,
    CKA_TRUST_KEY_ENCIPHERMENT, CKA_TRUST_DATA_ENCIPHERMENT,
    CKA_TRUST_KEY_AGREEMENT, CKA_TRUST_KEY_CERT_SIGN, CKA_TRUST_CRL_SIGN,
    CKA_TRUST_SERVER_AUTH, CKA_TRUST_CLIENT_AUTH, CKA_TRUST_CODE_SIGNING,
    CKA_TRUST_EMAIL_PROTECTION, CKA_TRUST_IPSEC_END_SYSTEM,
    CKA_TRUST_IPSEC_TUNNEL, CKA_TRUST_IPSEC_USER, CKA_TRUST_TIME_STAMPING,
    CKA_TRUST_STEP_UP_APPROVED, CKA_CERT_SHA1_HASH, CKA_CERT_MD5_HASH,
    CKA_NETSCAPE_DB, CKA_NETSCAPE_TRUST, CKA_NSS_OVERRIDE_EXTENSIONS
};

static int known_attributes_size= sizeof(known_attributes)/
			   sizeof(known_attributes[0]);

static CK_RV
sftkdb_GetObjectTemplate(SDB *source, CK_OBJECT_HANDLE id,
		CK_ATTRIBUTE *ptemplate, CK_ULONG *max)
{
    int i,j;
    CK_RV crv;

    if (*max < known_attributes_size) {
	*max = known_attributes_size;
	return CKR_BUFFER_TOO_SMALL;
    }
    for (i=0; i < known_attributes_size; i++) {
	ptemplate[i].type = known_attributes[i];
	ptemplate[i].pValue = NULL;
	ptemplate[i].ulValueLen = 0;
    }

    crv = (*source->sdb_GetAttributeValue)(source, id, 
					ptemplate, known_attributes_size);

    if ((crv != CKR_OK) && (crv != CKR_ATTRIBUTE_TYPE_INVALID)) {
	return crv;
    }

    for (i=0, j=0; i < known_attributes_size; i++, j++) {
	while (i < known_attributes_size && (ptemplate[i].ulValueLen == -1)) {
	    i++;
	}
	if (i >= known_attributes_size) {
	    break;
	}
	
	if (i == j) {
	   continue;
	}
	ptemplate[j] = ptemplate[i];
    }
    *max = j;
    return CKR_OK;
}

static const char SFTKDB_META_UPDATE_TEMPLATE[] = "upd_%s_%s";







static PRBool 
sftkdb_hasUpdate(const char *typeString, SDB *db, const char *updateID)
{
    char *id;
    CK_RV crv;
    SECItem dummy = { 0, NULL, 0 };
    unsigned char dummyData[SDB_MAX_META_DATA_LEN];

    if (!updateID) {
	return PR_FALSE;
    }
    id = PR_smprintf(SFTKDB_META_UPDATE_TEMPLATE, typeString, updateID);
    if (id == NULL) {
	return PR_FALSE;
    }
    dummy.data = dummyData;
    dummy.len = sizeof(dummyData);

    crv = (*db->sdb_GetMetaData)(db, id, &dummy, NULL);
    PR_smprintf_free(id);
    return crv == CKR_OK ? PR_TRUE : PR_FALSE;
}






static CK_RV
sftkdb_putUpdate(const char *typeString, SDB *db, const char *updateID)
{
    char *id;
    CK_RV crv;
    SECItem dummy = { 0, NULL, 0 };

    
    if (updateID == NULL) {
	return CKR_OK;
    }

    dummy.data = (unsigned char *)updateID;
    dummy.len = PORT_Strlen(updateID);

    id = PR_smprintf(SFTKDB_META_UPDATE_TEMPLATE, typeString, updateID);
    if (id == NULL) {
	return PR_FALSE;
    }

    crv = (*db->sdb_PutMetaData)(db, id, &dummy, NULL);
    PR_smprintf_free(id);
    return crv;
}





static CK_ULONG
sftkdb_getULongFromTemplate(CK_ATTRIBUTE_TYPE type, 
			CK_ATTRIBUTE *ptemplate, CK_ULONG len)
{
    CK_ATTRIBUTE *attr = sftkdb_getAttributeFromTemplate(type,
					ptemplate, len);

    if (attr && attr->pValue && attr->ulValueLen == SDB_ULONG_SIZE) {
	return sftk_SDBULong2ULong(attr->pValue);
    }
    return (CK_ULONG)-1;
}













static CK_RV
sftkdb_incrementCKAID(PRArenaPool *arena, CK_ATTRIBUTE *ptemplate)
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
		
		return CKR_OK;
	     }
	}
	

    } 
    buf = PORT_ArenaAlloc(arena, len+1);
    if (!buf) {
	return CKR_HOST_MEMORY;
    }
    if (len > 0) {
	 PORT_Memcpy(buf, ptemplate->pValue, len);
    }
    buf[len] = 0;
    ptemplate->pValue = buf;
    ptemplate->ulValueLen = len+1;
    return CKR_OK;
}




void
sftkdb_dropAttribute(CK_ATTRIBUTE *attr, CK_ATTRIBUTE *ptemplate, 
			CK_ULONG *plen)
{
   CK_ULONG count = *plen;
   CK_ULONG i;

   for (i=0; i < count; i++) {
	if (attr->type == ptemplate[i].type) {
	    break;
	}
   }

   if (i == count) {
	
	return;
   }

   
   for ( i++; i < count; i++) {
	ptemplate[i-1] = ptemplate[i];
   }

   
   *plen = count -1;
}





typedef enum {
	SFTKDB_DO_NOTHING = 0,
	SFTKDB_ADD_OBJECT,
	SFTKDB_MODIFY_OBJECT,
	SFTKDB_DROP_ATTRIBUTE
} sftkdbUpdateStatus;
















sftkdbUpdateStatus
sftkdb_reconcileTrustEntry(PRArenaPool *arena, CK_ATTRIBUTE *target, 
			   CK_ATTRIBUTE *source)
{
    CK_ULONG targetTrust = sftkdb_getULongFromTemplate(target->type,
			target, 1);
    CK_ULONG sourceTrust = sftkdb_getULongFromTemplate(target->type,
			source, 1);

    






    
    if (targetTrust == sourceTrust) {
	return SFTKDB_DO_NOTHING;
    }

    

    if (sourceTrust == (CK_ULONG)-1) {
	




	return SFTKDB_DROP_ATTRIBUTE;
    }

    
    if (targetTrust == (CK_ULONG)-1) {
	
	return SFTKDB_MODIFY_OBJECT;
    }

    



    if (sourceTrust == CKT_NSS_TRUST_UNKNOWN) {
	return SFTKDB_DROP_ATTRIBUTE;
    }

    
    if (targetTrust == CKT_NSS_TRUST_UNKNOWN) {
	
	return SFTKDB_MODIFY_OBJECT;
    }

    







    if ((sourceTrust == CKT_NSS_MUST_VERIFY) 
	|| (sourceTrust == CKT_NSS_VALID)
	|| (sourceTrust == CKT_NSS_VALID_DELEGATOR)) {
	return SFTKDB_DROP_ATTRIBUTE;
    }
    if ((targetTrust == CKT_NSS_MUST_VERIFY) 
	|| (targetTrust == CKT_NSS_VALID)
	|| (targetTrust == CKT_NSS_VALID_DELEGATOR)) {
	
	return SFTKDB_MODIFY_OBJECT;
    }

    
    return SFTKDB_DROP_ATTRIBUTE;
}

const CK_ATTRIBUTE_TYPE sftkdb_trustList[] =
	{ CKA_TRUST_SERVER_AUTH, CKA_TRUST_CLIENT_AUTH,
	  CKA_TRUST_CODE_SIGNING, CKA_TRUST_EMAIL_PROTECTION,
	  CKA_TRUST_IPSEC_TUNNEL, CKA_TRUST_IPSEC_USER,
	  CKA_TRUST_TIME_STAMPING };

#define SFTK_TRUST_TEMPLATE_COUNT \
		(sizeof(sftkdb_trustList)/sizeof(sftkdb_trustList[0]))





static sftkdbUpdateStatus
sftkdb_reconcileTrust(PRArenaPool *arena, SDB *db, CK_OBJECT_HANDLE id, 
		      CK_ATTRIBUTE *ptemplate, CK_ULONG *plen)
{
    CK_ATTRIBUTE trustTemplate[SFTK_TRUST_TEMPLATE_COUNT];
    unsigned char trustData[SFTK_TRUST_TEMPLATE_COUNT*SDB_ULONG_SIZE];
    sftkdbUpdateStatus update = SFTKDB_DO_NOTHING;
    CK_ULONG i;
    CK_RV crv;


    for (i=0; i < SFTK_TRUST_TEMPLATE_COUNT;  i++) {
	trustTemplate[i].type = sftkdb_trustList[i];
	trustTemplate[i].pValue = &trustData[i*SDB_ULONG_SIZE];
	trustTemplate[i].ulValueLen = SDB_ULONG_SIZE;
    }
    crv = (*db->sdb_GetAttributeValue)(db, id, 
				trustTemplate, SFTK_TRUST_TEMPLATE_COUNT);
    if ((crv != CKR_OK) && (crv != CKR_ATTRIBUTE_TYPE_INVALID)) {
	
	update = SFTKDB_MODIFY_OBJECT;
	goto done;
    }

    for (i=0; i < SFTK_TRUST_TEMPLATE_COUNT; i++) {
	CK_ATTRIBUTE *attr = sftkdb_getAttributeFromTemplate(
			trustTemplate[i].type, ptemplate, *plen);
	sftkdbUpdateStatus status;


	
	if (trustTemplate[i].ulValueLen == (CK_ULONG)-1) {
	    

	    if (attr && attr->ulValueLen != (CK_ULONG)-1) {
		update = SFTKDB_MODIFY_OBJECT;
	    }
	    continue;
	}

	


	if (attr == NULL) {
	    continue;
		
	}
	status = sftkdb_reconcileTrustEntry(arena, &trustTemplate[i], attr);
	if (status == SFTKDB_MODIFY_OBJECT) {
	    update = SFTKDB_MODIFY_OBJECT;
	} else if (status == SFTKDB_DROP_ATTRIBUTE) {
	    

	    sftkdb_dropAttribute(attr, ptemplate, plen);
	}
    }

    
    if (update == SFTKDB_MODIFY_OBJECT) {
	CK_BBOOL stepUpBool = CK_FALSE;
	

	trustTemplate[0].type = CKA_TRUST_STEP_UP_APPROVED;
	trustTemplate[0].pValue = &stepUpBool;
	trustTemplate[0].ulValueLen = sizeof(stepUpBool);
    	crv = (*db->sdb_GetAttributeValue)(db, id, trustTemplate, 1);
	if ((crv == CKR_OK) && (stepUpBool == CK_TRUE)) {
	    sftkdb_dropAttribute(trustTemplate, ptemplate, plen);
	}
    } else {
	

	CK_ATTRIBUTE *attr;

	attr = sftkdb_getAttributeFromTemplate(CKA_TRUST_STEP_UP_APPROVED,
			ptemplate, *plen);
	if (attr && (attr->ulValueLen == sizeof(CK_BBOOL)) &&  
			(*(CK_BBOOL *)(attr->pValue) == CK_TRUE)) {
		update = SFTKDB_MODIFY_OBJECT;
	}
    }
    
done:
    return update;
}

static sftkdbUpdateStatus
sftkdb_handleIDAndName(PRArenaPool *arena, SDB *db, CK_OBJECT_HANDLE id, 
		      CK_ATTRIBUTE *ptemplate, CK_ULONG *plen)
{
    sftkdbUpdateStatus update = SFTKDB_DO_NOTHING;
    CK_ATTRIBUTE *attr1, *attr2;
    CK_ATTRIBUTE ttemplate[2] = {
	{CKA_ID, NULL, 0},
	{CKA_LABEL, NULL, 0}
    };
    CK_RV crv;

    attr1 = sftkdb_getAttributeFromTemplate(CKA_LABEL, ptemplate, *plen);
    attr2 = sftkdb_getAttributeFromTemplate(CKA_ID, ptemplate, *plen);

    
    if ( (!attr1 || attr1->ulValueLen == 0) &&
	 (! attr2 ||  attr2->ulValueLen == 0) ) {
	return SFTKDB_DO_NOTHING;
    }

    
    crv = (*db->sdb_GetAttributeValue)(db, id, ttemplate, 2);

    
    if ( ((ttemplate[0].ulValueLen == 0) || 
	  (ttemplate[0].ulValueLen == (CK_ULONG)-1))  &&
         ((ttemplate[1].ulValueLen == 0) || 
	  (ttemplate[1].ulValueLen == (CK_ULONG)-1)) ) {
	return SFTKDB_MODIFY_OBJECT;
    }

    
    if ((ttemplate[0].ulValueLen != 0) && 
	(ttemplate[0].ulValueLen != (CK_ULONG)-1)) {
	

	if (attr1 && attr1->ulValueLen == 0) {
	    sftkdb_dropAttribute(attr1, ptemplate, plen);
	}
    } else if (attr1 && attr1->ulValueLen != 0) {
	
	update = SFTKDB_MODIFY_OBJECT;
    }


    
    if ((ttemplate[1].ulValueLen != 0) && 
	(ttemplate[1].ulValueLen != (CK_ULONG)-1)) {

	


	if (update == SFTKDB_DO_NOTHING) {
	    return update;
	}
	

	if (attr2 && attr2->ulValueLen == 0) {
	    sftkdb_dropAttribute(attr2, ptemplate, plen);
	}
    } else if (attr2 && attr2->ulValueLen != 0) {
	
	update = SFTKDB_MODIFY_OBJECT;
    }

    return update;
}


		








static PRBool
sftkdb_updateObjectTemplate(PRArenaPool *arena, SDB *db, 
		    CK_OBJECT_CLASS objectType, 
		    CK_ATTRIBUTE *ptemplate, CK_ULONG *plen,
		    CK_OBJECT_HANDLE *targetID)
{
    PRBool done; 
    CK_OBJECT_HANDLE id;
    CK_RV crv = CKR_OK;

    do {
 	crv = sftkdb_checkConflicts(db, objectType, ptemplate, 
						*plen, CK_INVALID_HANDLE);
	if (crv != CKR_ATTRIBUTE_VALUE_INVALID) {
	    break;
	}
	crv = sftkdb_resolveConflicts(arena, objectType, ptemplate, plen);
    } while (crv == CKR_OK);

    if (crv != CKR_OK) {
	return SFTKDB_DO_NOTHING;
    }

    do {
	done = PR_TRUE;
	crv = sftkdb_lookupObject(db, objectType, &id, ptemplate, *plen);
	if (crv != CKR_OK) {
	    return SFTKDB_DO_NOTHING;
	}

	
	if (id != CK_INVALID_HANDLE) {
    	    CK_ATTRIBUTE *attr = NULL;
	    
	    switch (objectType) {
	    case CKO_CERTIFICATE:
	    case CKO_PUBLIC_KEY:
	    case CKO_PRIVATE_KEY:
		

		*targetID = id;
		return sftkdb_handleIDAndName(arena, db, id, ptemplate, plen);
	    case CKO_NSS_TRUST:
		

		*targetID = id;
		return sftkdb_reconcileTrust(arena, db, id, ptemplate, plen);
	    case CKO_SECRET_KEY:
		




		
		attr = sftkdb_getAttributeFromTemplate(CKA_ID,ptemplate,*plen);
		crv = attr ? sftkdb_incrementCKAID(arena, attr) 
		           : CKR_HOST_MEMORY; 
		

		if (crv != CKR_OK) {
		    return SFTKDB_DO_NOTHING;
		}
		done = PR_FALSE; 
		break;
	    default:
		

	        return SFTKDB_DO_NOTHING;
	    }
	}
    } while (!done);

    
    return SFTKDB_ADD_OBJECT;
}


#define MAX_ATTRIBUTES 500
static CK_RV
sftkdb_mergeObject(SFTKDBHandle *handle, CK_OBJECT_HANDLE id, 
		   SECItem *key)
{
    CK_ATTRIBUTE template[MAX_ATTRIBUTES];
    CK_ATTRIBUTE *ptemplate;
    CK_ULONG max_attributes = MAX_ATTRIBUTES;
    CK_OBJECT_CLASS objectType;
    SDB *source = handle->update;
    SDB *target = handle->db;
    int i;
    CK_RV crv;
    PLArenaPool *arena = NULL;

    arena = PORT_NewArena(256);
    if (arena ==  NULL) {
	return CKR_HOST_MEMORY;
    }

    ptemplate = &template[0];
    id &= SFTK_OBJ_ID_MASK;
    crv = sftkdb_GetObjectTemplate(source, id, ptemplate, &max_attributes);
    if (crv == CKR_BUFFER_TOO_SMALL) {
	ptemplate = PORT_ArenaNewArray(arena, CK_ATTRIBUTE, max_attributes);
	if (ptemplate == NULL) {
	    crv = CKR_HOST_MEMORY;
	} else {
            crv = sftkdb_GetObjectTemplate(source, id, 
					   ptemplate, &max_attributes);
	}
    }
    if (crv != CKR_OK) {
	goto loser;
    }

    for (i=0; i < max_attributes; i++) {
	ptemplate[i].pValue = PORT_ArenaAlloc(arena,ptemplate[i].ulValueLen);
	if (ptemplate[i].pValue == NULL) {
	    crv = CKR_HOST_MEMORY;
	    goto loser;
	}
    }
    crv = (*source->sdb_GetAttributeValue)(source, id, 
					   ptemplate, max_attributes);
    if (crv != CKR_OK) {
	goto loser;
    }

    objectType = sftkdb_getULongFromTemplate(CKA_CLASS, ptemplate,
							 max_attributes);

    




    if (!handle->updateID) {
	    crv = sftkdb_CreateObject(arena, handle, target, &id, 
				ptemplate, max_attributes);
    } else {
	sftkdbUpdateStatus update_status;
	update_status  = sftkdb_updateObjectTemplate(arena, target, 
			objectType, ptemplate, &max_attributes, &id);
	switch (update_status) {
	case SFTKDB_ADD_OBJECT:
	    crv = sftkdb_CreateObject(arena, handle, target, &id, 
				ptemplate, max_attributes);
	    break;
	case SFTKDB_MODIFY_OBJECT:
    	    crv = sftkdb_setAttributeValue(arena, handle, target, 
				   id, ptemplate, max_attributes);
	    break;
	case SFTKDB_DO_NOTHING:
	case SFTKDB_DROP_ATTRIBUTE:
	    break;
	}
    } 

loser:
    if (arena) {
	PORT_FreeArena(arena,PR_TRUE);
    }
    return crv;
}
	

#define MAX_IDS 10



CK_RV
sftkdb_Update(SFTKDBHandle *handle, SECItem *key)
{
    SDBFind *find = NULL;
    CK_ULONG idCount = MAX_IDS;
    CK_OBJECT_HANDLE ids[MAX_IDS];
    SECItem *updatePasswordKey = NULL;
    CK_RV crv, crv2;
    PRBool inTransaction = PR_FALSE;
    int i;

    if (handle == NULL) {
	return CKR_OK;
    }
    if (handle->update == NULL) {
	return CKR_OK;
    }

    



    crv = (*handle->db->sdb_Begin)(handle->db);
    if (crv != CKR_OK) {
	goto loser;
    }
    inTransaction = PR_TRUE;
    
    
    if (sftkdb_hasUpdate(sftkdb_TypeString(handle), 
			 handle->db, handle->updateID)) {
	crv = CKR_OK;
	goto done;
    }

    updatePasswordKey = sftkdb_GetUpdatePasswordKey(handle);
    if (updatePasswordKey) {
	

	handle->oldKey = updatePasswordKey;
    }
    
    
    crv = sftkdb_FindObjectsInit(handle, NULL, 0, &find);

    if (crv != CKR_OK) {
	goto loser;
    }
    while ((crv == CKR_OK) && (idCount == MAX_IDS)) {
	crv = sftkdb_FindObjects(handle, find, ids, MAX_IDS, &idCount);
	for (i=0; (crv == CKR_OK) && (i < idCount); i++) {
	    crv = sftkdb_mergeObject(handle, ids[i], key);
	}
    }
    crv2 = sftkdb_FindObjectsFinal(handle, find);
    if (crv == CKR_OK) crv = crv2;

loser:
    
    handle->oldKey = NULL;

    
    if (handle->type == SFTK_KEYDB_TYPE) {
	SECItem item1, item2;
	unsigned char data1[SDB_MAX_META_DATA_LEN];
	unsigned char data2[SDB_MAX_META_DATA_LEN];

	item1.data = data1;
 	item1.len = sizeof(data1);
	item2.data = data2;
 	item2.len = sizeof(data2);

	
	crv = (*handle->db->sdb_GetMetaData)(handle->db, "password",
			&item1, &item2);
	if (crv == CKR_OK) {
	    goto done;
	}


	
	crv = (*handle->update->sdb_GetMetaData)(handle->update, "password",
			&item1, &item2);
	if (crv != CKR_OK) {
	    goto done;
	}
	crv = (*handle->db->sdb_PutMetaData)(handle->db, "password", &item1,
						&item2);
	if (crv != CKR_OK) {
	    goto done;
	}
    }

done:
    
    
    if (crv == CKR_OK) {
	crv = sftkdb_putUpdate(sftkdb_TypeString(handle), 
				handle->db, handle->updateID);
    }

    if (inTransaction) {
	if (crv == CKR_OK) {
	    crv = (*handle->db->sdb_Commit)(handle->db);
	} else {
	    (*handle->db->sdb_Abort)(handle->db);
	}
    }
    if (handle->update) {
	(*handle->update->sdb_Close)(handle->update);
	handle->update = NULL;
    }
    if (handle->updateID) {
	PORT_Free(handle->updateID);
	handle->updateID = NULL;
    }
    sftkdb_FreeUpdatePasswordKey(handle);
    if (updatePasswordKey) {
	SECITEM_ZfreeItem(updatePasswordKey, PR_TRUE);
    }
    handle->updateDBIsInit = PR_FALSE;
    return crv;
}








const char *
sftkdb_GetUpdateID(SFTKDBHandle *handle)
{
    return handle->updateID;
}


void
sftk_freeDB(SFTKDBHandle *handle)
{
    PRInt32 ref;

    if (!handle) return;
    ref = PR_AtomicDecrement(&handle->ref);
    if (ref == 0) {
	sftkdb_CloseDB(handle);
    }
    return;
}






SFTKDBHandle *
sftk_getCertDB(SFTKSlot *slot)
{
    SFTKDBHandle *dbHandle;

    PZ_Lock(slot->slotLock);
    dbHandle = slot->certDB;
    if (dbHandle) {
        PR_AtomicIncrement(&dbHandle->ref);
    }
    PZ_Unlock(slot->slotLock);
    return dbHandle;
}





SFTKDBHandle *
sftk_getKeyDB(SFTKSlot *slot)
{
    SFTKDBHandle *dbHandle;

    SKIP_AFTER_FORK(PZ_Lock(slot->slotLock));
    dbHandle = slot->keyDB;
    if (dbHandle) {
        PR_AtomicIncrement(&dbHandle->ref);
    }
    SKIP_AFTER_FORK(PZ_Unlock(slot->slotLock));
    return dbHandle;
}





SFTKDBHandle *
sftk_getDBForTokenObject(SFTKSlot *slot, CK_OBJECT_HANDLE objectID)
{
    SFTKDBHandle *dbHandle;

    PZ_Lock(slot->slotLock);
    dbHandle = objectID & SFTK_KEYDB_TYPE ? slot->keyDB : slot->certDB;
    if (dbHandle) {
        PR_AtomicIncrement(&dbHandle->ref);
    }
    PZ_Unlock(slot->slotLock);
    return dbHandle;
}




static SFTKDBHandle *
sftk_NewDBHandle(SDB *sdb, int type)
{
   SFTKDBHandle *handle = PORT_New(SFTKDBHandle);
   handle->ref = 1;
   handle->db = sdb;
   handle->update = NULL;
   handle->peerDB = NULL;
   handle->newKey = NULL;
   handle->oldKey = NULL;
   handle->updatePasswordKey = NULL;
   handle->updateID = NULL;
   handle->type = type;
   handle->passwordKey.data = NULL;
   handle->passwordKey.len = 0;
   handle->passwordLock = NULL;
   if (type == SFTK_KEYDB_TYPE) {
	handle->passwordLock = PZ_NewLock(nssILockAttribute);
   }
   sdb->app_private = handle;
   return handle;
}





SECStatus
sftkdb_ResetKeyDB(SFTKDBHandle *handle)
{
    CK_RV crv;

    
    if (handle->type != SFTK_KEYDB_TYPE) {
	return SECFailure;
    }
    crv = sftkdb_ResetDB(handle);
    if (crv != CKR_OK) {
	
	return SECFailure;
    }
    return SECSuccess;
}

static PRBool
sftk_oldVersionExists(const char *dir, int version)
{
    int i;
    PRStatus exists = PR_FAILURE;
    char *file = NULL;

    for (i=version; i > 1 ; i--) {
	file = PR_smprintf("%s%d.db",dir,i);
	if (file == NULL) {
	    continue;
	}
	exists = PR_Access(file, PR_ACCESS_EXISTS);
	PR_smprintf_free(file);
	if (exists == PR_SUCCESS) {
	    return PR_TRUE;
	}
    }
    return PR_FALSE;
}

static PRBool
sftk_hasLegacyDB(const char *confdir, const char *certPrefix, 
		 const char *keyPrefix, int certVersion, int keyVersion)
{
    char *dir;
    PRBool exists;

    if (certPrefix == NULL) {
	certPrefix = "";
    }

    if (keyPrefix == NULL) {
	keyPrefix = "";
    }

    dir= PR_smprintf("%s/%scert", confdir, certPrefix);
    if (dir == NULL) {
	return PR_FALSE;
    }

    exists = sftk_oldVersionExists(dir, certVersion);
    PR_smprintf_free(dir);
    if (exists) {
	return PR_TRUE;
    }

    dir= PR_smprintf("%s/%skey", confdir, keyPrefix);
    if (dir == NULL) {
	return PR_FALSE;
    }

    exists = sftk_oldVersionExists(dir, keyVersion);
    PR_smprintf_free(dir);
    return exists;
}








CK_RV 
sftk_DBInit(const char *configdir, const char *certPrefix,
                const char *keyPrefix, const char *updatedir,
		const char *updCertPrefix, const char *updKeyPrefix, 
		const char *updateID, PRBool readOnly, PRBool noCertDB,
                PRBool noKeyDB, PRBool forceOpen, PRBool isFIPS,
                SFTKDBHandle **certDB, SFTKDBHandle **keyDB)
{
    const char *confdir;
    SDBType dbType;
    char *appName = NULL;
    SDB *keySDB, *certSDB;
    CK_RV crv = CKR_OK;
    int flags = SDB_RDONLY;
    PRBool newInit = PR_FALSE;
    PRBool needUpdate = PR_FALSE;

    if (!readOnly) {
	flags = SDB_CREATE;
    }

    *certDB = NULL;
    *keyDB = NULL;

    if (noKeyDB && noCertDB) {
	return CKR_OK;
    }
    confdir = sftk_EvaluateConfigDir(configdir, &dbType, &appName);

    


    switch (dbType) {
    case SDB_LEGACY:
	crv = sftkdbCall_open(confdir, certPrefix, keyPrefix, 8, 3, flags,
		 isFIPS, noCertDB? NULL : &certSDB, noKeyDB ? NULL: &keySDB);
	break;
    case SDB_MULTIACCESS:
	crv = sftkdbCall_open(configdir, certPrefix, keyPrefix, 8, 3, flags,
		isFIPS, noCertDB? NULL : &certSDB, noKeyDB ? NULL: &keySDB);
	break;
    case SDB_SQL:
    case SDB_EXTERN: 
	crv = s_open(confdir, certPrefix, keyPrefix, 9, 4, flags, 
		noCertDB? NULL : &certSDB, noKeyDB ? NULL : &keySDB, &newInit);

        



	if (crv != CKR_OK) {
	    if ((flags == SDB_RDONLY)  &&
	         sftk_hasLegacyDB(confdir, certPrefix, keyPrefix, 8, 3)) {
	    

		crv = sftkdbCall_open(confdir, certPrefix, 
			keyPrefix, 8, 3, flags, isFIPS, 
			noCertDB? NULL : &certSDB, noKeyDB ? NULL : &keySDB);
	    }
	








	} else if (
	      
	      updatedir  && *updatedir && updateID  && *updateID
	      
	      && sftk_hasLegacyDB(updatedir, updCertPrefix, updKeyPrefix, 8, 3) 
	      
	      && ((noKeyDB || !sftkdb_hasUpdate("key", keySDB, updateID)) 
	      || (noCertDB || !sftkdb_hasUpdate("cert", certSDB, updateID)))) {
	    
	    confdir = updatedir;
	    certPrefix = updCertPrefix;
	    keyPrefix = updKeyPrefix;
	    needUpdate = PR_TRUE;
	} else if (newInit) {
	    


	    if (sftk_hasLegacyDB(confdir, certPrefix, keyPrefix, 8, 3)) {
		needUpdate = PR_TRUE;
	    }
	}
	break;
    default:
	crv = CKR_GENERAL_ERROR; 


    }
    if (crv != CKR_OK) {
	goto done;
    }
    if (!noCertDB) {
	*certDB = sftk_NewDBHandle(certSDB, SFTK_CERTDB_TYPE);
    } else {
	*certDB = NULL;
    }
    if (!noKeyDB) {
	*keyDB = sftk_NewDBHandle(keySDB, SFTK_KEYDB_TYPE);
    } else {
	*keyDB = NULL;
    }

    
    if (*certDB) {
	(*certDB)->peerDB = *keyDB;
    }
    if (*keyDB) {
	(*keyDB)->peerDB = *certDB;
    }

    



    if (needUpdate) {
	SDB *updateCert = NULL;
	SDB *updateKey = NULL;
	CK_RV crv2;

	crv2 = sftkdbCall_open(confdir, certPrefix, keyPrefix, 8, 3, flags,
		isFIPS, noCertDB ? NULL : &updateCert, 
		noKeyDB ? NULL : &updateKey);
	if (crv2 == CKR_OK) {
	    if (*certDB) {
		(*certDB)->update = updateCert;
		(*certDB)->updateID = updateID && *updateID 
				? PORT_Strdup(updateID) : NULL;
		updateCert->app_private = (*certDB);
	    }
	    if (*keyDB) {
		PRBool tokenRemoved = PR_FALSE;
		(*keyDB)->update = updateKey;
		(*keyDB)->updateID = updateID && *updateID ? 
					PORT_Strdup(updateID) : NULL;
		updateKey->app_private = (*keyDB);
		(*keyDB)->updateDBIsInit = PR_TRUE;
		(*keyDB)->updateDBIsInit = 
			(sftkdb_HasPasswordSet(*keyDB) == SECSuccess) ?
			 PR_TRUE : PR_FALSE;
		

		sftkdb_CheckPassword((*keyDB), "", &tokenRemoved);
	    } else {
		
		sftkdb_Update(*certDB, NULL);
	    }
	}
    }
done:
    if (appName) {
	PORT_Free(appName);
    }
   return forceOpen ? CKR_OK : crv;
}

CK_RV 
sftkdb_Shutdown(void)
{
  s_shutdown();
  sftkdbCall_Shutdown();
  return CKR_OK;
}

