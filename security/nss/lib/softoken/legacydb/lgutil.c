


#include "lgdb.h"
#include "secerr.h"
#include "lgglue.h"










const CK_ATTRIBUTE *
lg_FindAttribute(CK_ATTRIBUTE_TYPE type, const CK_ATTRIBUTE *templ,
		 CK_ULONG count )
{
    unsigned int i;

    for (i=0; i < count; i++) {
	if (templ[i].type == type) {
	    return &templ[i];
	}
    }
    return NULL;
}





PRBool
lg_hasAttribute(CK_ATTRIBUTE_TYPE type, const CK_ATTRIBUTE *templ,
		 CK_ULONG count )
{
   if (lg_FindAttribute(type, templ, count) == NULL) {
	return PR_FALSE;
   }
   return PR_TRUE;
}





CK_RV
lg_Attribute2SecItem(PLArenaPool *arena, CK_ATTRIBUTE_TYPE type,
			const CK_ATTRIBUTE *templ, CK_ULONG count,
			SECItem *item)
{
    int len;
    const CK_ATTRIBUTE *attribute;

    attribute = lg_FindAttribute(type, templ, count);
    if (attribute == NULL) return CKR_TEMPLATE_INCOMPLETE;
    len = attribute->ulValueLen;

    if (arena) {
    	item->data = (unsigned char *) PORT_ArenaAlloc(arena,len);
    } else {
    	item->data = (unsigned char *) PORT_Alloc(len);
    }
    if (item->data == NULL) {
	return CKR_HOST_MEMORY;
    }
    item->len = len;
    PORT_Memcpy(item->data, attribute->pValue, len);
    return CKR_OK;
}






CK_RV
lg_Attribute2SSecItem(PLArenaPool *arena, CK_ATTRIBUTE_TYPE type,
			const CK_ATTRIBUTE *templ, CK_ULONG count,
			SECItem *item)
{
    const CK_ATTRIBUTE *attribute;
    item->data = NULL;

    attribute = lg_FindAttribute(type, templ, count);
    if (attribute == NULL) return CKR_TEMPLATE_INCOMPLETE;

    (void)SECITEM_AllocItem(arena, item, attribute->ulValueLen);
    if (item->data == NULL) {
	return CKR_HOST_MEMORY;
    }
    PORT_Memcpy(item->data, attribute->pValue, item->len);
    return CKR_OK;
}





CK_RV
lg_PrivAttr2SSecItem(PLArenaPool *arena, CK_ATTRIBUTE_TYPE type,
			const CK_ATTRIBUTE *templ, CK_ULONG count,
			SECItem *item, SDB *sdbpw)
{
    const CK_ATTRIBUTE *attribute;
    SECItem epki, *dest = NULL;
    SECStatus rv;

    item->data = NULL;

    attribute = lg_FindAttribute(type, templ, count);
    if (attribute == NULL) return CKR_TEMPLATE_INCOMPLETE;

    epki.data = attribute->pValue;
    epki.len = attribute->ulValueLen;

    rv = lg_util_decrypt(sdbpw, &epki, &dest);
    if (rv != SECSuccess) {
	return CKR_USER_NOT_LOGGED_IN;
    }
    (void)SECITEM_AllocItem(arena, item, dest->len);
    if (item->data == NULL) {
	SECITEM_FreeItem(dest, PR_TRUE);
	return CKR_HOST_MEMORY;
    }
    
    PORT_Memcpy(item->data, dest->data, item->len);
    SECITEM_FreeItem(dest, PR_TRUE);
    return CKR_OK;
}

CK_RV
lg_PrivAttr2SecItem(PLArenaPool *arena, CK_ATTRIBUTE_TYPE type,
			const CK_ATTRIBUTE *templ, CK_ULONG count,
			SECItem *item, SDB *sdbpw)
{
    return lg_PrivAttr2SSecItem(arena, type, templ, count, item, sdbpw);
}





PRBool
lg_isTrue(CK_ATTRIBUTE_TYPE type, const CK_ATTRIBUTE *templ, CK_ULONG count)
{
    const CK_ATTRIBUTE *attribute;
    PRBool tok = PR_FALSE;

    attribute=lg_FindAttribute(type, templ, count);
    if (attribute == NULL) { return PR_FALSE; }
    tok = (PRBool)(*(CK_BBOOL *)attribute->pValue);

    return tok;
}





char *
lg_getString(CK_ATTRIBUTE_TYPE type, const CK_ATTRIBUTE *templ, CK_ULONG count)
{
    const CK_ATTRIBUTE *attribute;
    char *label = NULL;

    attribute = lg_FindAttribute(type, templ, count);
    if (attribute == NULL) return NULL;

    if (attribute->pValue != NULL) {
	label = (char *) PORT_Alloc(attribute->ulValueLen+1);
	if (label == NULL) {
	    return NULL;
	}

	PORT_Memcpy(label,attribute->pValue, attribute->ulValueLen);
	label[attribute->ulValueLen] = 0;
    }
    return label;
}

CK_RV
lg_GetULongAttribute(CK_ATTRIBUTE_TYPE type, const CK_ATTRIBUTE *templ,
				 CK_ULONG count, CK_ULONG *longData)
{
    const CK_ATTRIBUTE *attribute;
    CK_ULONG value = 0;
    const unsigned char *data;
    int i;

    attribute = lg_FindAttribute(type, templ, count);
    if (attribute == NULL) return CKR_TEMPLATE_INCOMPLETE;

    if (attribute->ulValueLen != 4) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }
    data = (const unsigned char *)attribute->pValue;
    for (i=0; i < 4; i++) {
	value |= (CK_ULONG)(data[i]) << ((3-i)*8);
    }

    *longData = value;
    return CKR_OK;
}





SECStatus
lg_deleteTokenKeyByHandle(SDB *sdb, CK_OBJECT_HANDLE handle)
{
    SECItem *item;
    PRBool rem;
    PLHashTable *hashTable= lg_GetHashTable(sdb);

    item = (SECItem *)PL_HashTableLookup(hashTable, (void *)handle);
    rem = PL_HashTableRemove(hashTable,(void *)handle) ;
    if (rem && item) {
	SECITEM_FreeItem(item,PR_TRUE);
    }
    return rem ? SECSuccess : SECFailure;
}


static SECStatus
lg_addTokenKeyByHandle(SDB *sdb, CK_OBJECT_HANDLE handle, SECItem *key)
{
    PLHashEntry *entry;
    SECItem *item;
    PLHashTable *hashTable= lg_GetHashTable(sdb);

    item = SECITEM_DupItem(key);
    if (item == NULL) {
	return SECFailure;
    }
    entry = PL_HashTableAdd(hashTable,(void *)handle,item);
    if (entry == NULL) {
	SECITEM_FreeItem(item,PR_TRUE);
	return SECFailure;
    }
    return SECSuccess;
}


const SECItem *
lg_lookupTokenKeyByHandle(SDB *sdb, CK_OBJECT_HANDLE handle)
{
    PLHashTable *hashTable= lg_GetHashTable(sdb);
    return (const SECItem *)PL_HashTableLookup(hashTable, (void *)handle);
}


static PRIntn
lg_freeHashItem(PLHashEntry* entry, PRIntn index, void *arg)
{
    SECItem *item = (SECItem *)entry->value;

    SECITEM_FreeItem(item, PR_TRUE);
    return HT_ENUMERATE_NEXT;
}

CK_RV
lg_ClearTokenKeyHashTable(SDB *sdb)
{
    PLHashTable *hashTable;
    lg_DBLock(sdb);
    hashTable= lg_GetHashTable(sdb);
    PL_HashTableEnumerateEntries(hashTable, lg_freeHashItem, NULL);
    lg_DBUnlock(sdb);
    return CKR_OK;
}




static void
lg_XORHash(unsigned char *key, unsigned char *dbkey, int len)
{
   int i;

   PORT_Memset(key, 0, 4);

   for (i=0; i < len-4; i += 4) {
	key[0] ^= dbkey[i];
	key[1] ^= dbkey[i+1];
	key[2] ^= dbkey[i+2];
	key[3] ^= dbkey[i+3];
   }
}


CK_OBJECT_HANDLE
lg_mkHandle(SDB *sdb, SECItem *dbKey, CK_OBJECT_HANDLE class)
{
    unsigned char hashBuf[4];
    CK_OBJECT_HANDLE handle;
    const SECItem *key;

    handle = class;
    
    if (handle != LG_TOKEN_KRL_HANDLE) {
	lg_XORHash(hashBuf,dbKey->data,dbKey->len);
	handle = (hashBuf[0] << 24) | (hashBuf[1] << 16) | 
					(hashBuf[2] << 8)  | hashBuf[3];
	handle =  class | (handle & ~(LG_TOKEN_TYPE_MASK|LG_TOKEN_MASK));
	

	if (handle == LG_TOKEN_KRL_HANDLE) {
	    handle++;
	}
    }

    lg_DBLock(sdb);
    while ((key = lg_lookupTokenKeyByHandle(sdb,handle)) != NULL) {
	if (SECITEM_ItemsAreEqual(key,dbKey)) {
    	   lg_DBUnlock(sdb);
	   return handle;
	}
	handle++;
    }
    lg_addTokenKeyByHandle(sdb,handle,dbKey);
    lg_DBUnlock(sdb);
    return handle;
}

PRBool
lg_poisonHandle(SDB *sdb, SECItem *dbKey, CK_OBJECT_HANDLE class)
{
    unsigned char hashBuf[4];
    CK_OBJECT_HANDLE handle;
    const SECItem *key;

    handle = class;
    
    if (handle != LG_TOKEN_KRL_HANDLE) {
	lg_XORHash(hashBuf,dbKey->data,dbKey->len);
	handle = (hashBuf[0] << 24) | (hashBuf[1] << 16) | 
					(hashBuf[2] << 8)  | hashBuf[3];
	handle = class | (handle & ~(LG_TOKEN_TYPE_MASK|LG_TOKEN_MASK));
	

	if (handle == LG_TOKEN_KRL_HANDLE) {
	    handle++;
	}
    }
    lg_DBLock(sdb);
    while ((key = lg_lookupTokenKeyByHandle(sdb,handle)) != NULL) {
	if (SECITEM_ItemsAreEqual(key,dbKey)) {
	   key->data[0] ^= 0x80;
    	   lg_DBUnlock(sdb);
	   return PR_TRUE;
	}
	handle++;
    }
    lg_DBUnlock(sdb);
    return PR_FALSE;
}

static LGEncryptFunc lg_encrypt_stub = NULL;
static LGDecryptFunc lg_decrypt_stub = NULL;

void
legacy_SetCryptFunctions(LGEncryptFunc enc, LGDecryptFunc dec)
{
   lg_encrypt_stub = enc;
   lg_decrypt_stub = dec;
}

SECStatus lg_util_encrypt(PLArenaPool *arena, SDB *sdb, 
			  SECItem *plainText, SECItem **cipherText)
{
    if (lg_encrypt_stub == NULL) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return (*lg_encrypt_stub)(arena, sdb, plainText, cipherText);
}

SECStatus lg_util_decrypt(SDB *sdb, SECItem *cipherText, SECItem **plainText)
{
    if (lg_decrypt_stub == NULL) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return (*lg_decrypt_stub)(sdb, cipherText, plainText);
}


