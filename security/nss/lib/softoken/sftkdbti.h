



#ifndef SFTKDBTI_H
#define SFTKDBTI_H 1




struct SFTKDBHandleStr {
    SDB   *db;
    PRInt32 ref;
    CK_OBJECT_HANDLE  type;
    SECItem passwordKey;
    SECItem *newKey;
    SECItem *oldKey;
    SECItem *updatePasswordKey;
    PZLock *passwordLock;
    SFTKDBHandle *peerDB;
    SDB   *update;
    char  *updateID;
    PRBool updateDBIsInit;
};

#define SFTK_KEYDB_TYPE 0x40000000
#define SFTK_CERTDB_TYPE 0x00000000
#define SFTK_OBJ_TYPE_MASK 0xc0000000
#define SFTK_OBJ_ID_MASK (~SFTK_OBJ_TYPE_MASK)
#define SFTK_TOKEN_TYPE 0x80000000



#define SFTK_MAX_IDS 10

#define SFTK_GET_SDB(handle) \
	((handle)->update ? (handle)->update : (handle)->db)

SECStatus sftkdb_DecryptAttribute(SECItem *passKey, SECItem *cipherText,
			SECItem **plainText);
SECStatus sftkdb_EncryptAttribute(PLArenaPool *arena, SECItem *passKey,
			SECItem *plainText, SECItem **cipherText);
SECStatus sftkdb_SignAttribute(PLArenaPool *arena, SECItem *passKey,
			CK_OBJECT_HANDLE objectID,
			CK_ATTRIBUTE_TYPE attrType,
			SECItem *plainText, SECItem **sigText);
SECStatus sftkdb_VerifyAttribute(SECItem *passKey,
			CK_OBJECT_HANDLE objectID,
			CK_ATTRIBUTE_TYPE attrType,
			SECItem *plainText, SECItem *sigText);

void sftk_ULong2SDBULong(unsigned char *data, CK_ULONG value);
CK_RV sftkdb_Update(SFTKDBHandle *handle, SECItem *key);
CK_RV sftkdb_PutAttributeSignature(SFTKDBHandle *handle, 
		SDB *keyTarget, CK_OBJECT_HANDLE objectID, 
		CK_ATTRIBUTE_TYPE type, SECItem *signText);



#endif
