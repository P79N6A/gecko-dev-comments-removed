


































#include "sftkdbt.h"
#include "sdb.h"
#include "pkcs11i.h"
#include "pkcs11t.h"


CK_RV sftkdb_write(SFTKDBHandle *handle, SFTKObject *,CK_OBJECT_HANDLE *);
CK_RV sftkdb_FindObjectsInit(SFTKDBHandle *sdb, const CK_ATTRIBUTE *template,
				 CK_ULONG count, SDBFind **find);
CK_RV sftkdb_FindObjects(SFTKDBHandle *sdb, SDBFind *find, 
			CK_OBJECT_HANDLE *ids, int arraySize, CK_ULONG *count);
CK_RV sftkdb_FindObjectsFinal(SFTKDBHandle *sdb, SDBFind *find);
CK_RV sftkdb_GetAttributeValue(SFTKDBHandle *handle,
	 CK_OBJECT_HANDLE object_id, CK_ATTRIBUTE *template, CK_ULONG count);
CK_RV sftkdb_SetAttributeValue(SFTKDBHandle *handle, SFTKObject *object, 
	 		const CK_ATTRIBUTE *template, CK_ULONG count);
CK_RV sftkdb_DestroyObject(SFTKDBHandle *handle, CK_OBJECT_HANDLE object_id);
CK_RV sftkdb_closeDB(SFTKDBHandle *handle);



char ** sftkdb_ReadSecmodDB(SDBType dbType, const char *appName, 
			    const char *filename, const char *dbname, 
			    char *params, PRBool rw);
SECStatus sftkdb_ReleaseSecmodDBData(SDBType dbType, const char *appName, 
				     const char *filename, const char *dbname, 
				     char **moduleSpecList, PRBool rw);
SECStatus sftkdb_DeleteSecmodDB(SDBType dbType, const char *appName, 
				const char *filename, const char *dbname, 
				char *args, PRBool rw);
SECStatus sftkdb_AddSecmodDB(SDBType dbType, const char *appName, 
			     const char *filename, const char *dbname, 
			     char *module, PRBool rw);



SECStatus sftkdb_PWIsInitialized(SFTKDBHandle *keydb);
SECStatus sftkdb_CheckPassword(SFTKDBHandle *keydb, const char *pw,
			       PRBool *tokenRemoved);
SECStatus sftkdb_PWCached(SFTKDBHandle *keydb);
SECStatus sftkdb_HasPasswordSet(SFTKDBHandle *keydb);
SECStatus sftkdb_ResetKeyDB(SFTKDBHandle *keydb);
SECStatus sftkdb_ChangePassword(SFTKDBHandle *keydb, 
				char *oldPin, char *newPin,
				PRBool *tokenRemoved);
SECStatus sftkdb_ClearPassword(SFTKDBHandle *keydb);
PRBool sftkdb_InUpdateMerge(SFTKDBHandle *keydb);
PRBool sftkdb_NeedUpdateDBPassword(SFTKDBHandle *keydb);
const char *sftkdb_GetUpdateID(SFTKDBHandle *keydb);
SECItem *sftkdb_GetUpdatePasswordKey(SFTKDBHandle *keydb);
void sftkdb_FreeUpdatePasswordKey(SFTKDBHandle *keydb);



















CK_RV sftk_DBInit(const char *configdir, const char *certPrefix,
	 	const char *keyPrefix, const char *updatedir, 
		const char *updCertPrefix, const char *updKeyPrefix,
		const char *updateID, PRBool readOnly, PRBool noCertDB, 
		PRBool noKeyDB, PRBool forceOpen, PRBool isFIPS,
		SFTKDBHandle **certDB, SFTKDBHandle **keyDB);
CK_RV sftkdb_Shutdown(void);

SFTKDBHandle *sftk_getCertDB(SFTKSlot *slot);
SFTKDBHandle *sftk_getKeyDB(SFTKSlot *slot);
SFTKDBHandle *sftk_getDBForTokenObject(SFTKSlot *slot, 
                                       CK_OBJECT_HANDLE objectID);
void sftk_freeDB(SFTKDBHandle *certHandle);
