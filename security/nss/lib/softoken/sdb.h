
























































#ifndef _SDB_H
#define _SDB_H 1
#include "pkcs11t.h"
#include "secitem.h"
#include "sftkdbt.h"
#include <sqlite3.h>

#define STATIC_CMD_SIZE 2048

typedef struct SDBFindStr SDBFind;
typedef struct SDBStr SDB;

struct SDBStr {
    void *private;
    int  version;
    SDBType sdb_type;
    int  sdb_flags;
    void *app_private;
    CK_RV (*sdb_FindObjectsInit)(SDB *sdb, const CK_ATTRIBUTE *template, 
				 CK_ULONG count, SDBFind **find);
    CK_RV (*sdb_FindObjects)(SDB *sdb, SDBFind *find, CK_OBJECT_HANDLE *ids, 
				CK_ULONG arraySize, CK_ULONG *count);
    CK_RV (*sdb_FindObjectsFinal)(SDB *sdb, SDBFind *find);
    CK_RV (*sdb_GetAttributeValue)(SDB *sdb, CK_OBJECT_HANDLE object, 
				CK_ATTRIBUTE *template, CK_ULONG count);
    CK_RV (*sdb_SetAttributeValue)(SDB *sdb, CK_OBJECT_HANDLE object, 
				const CK_ATTRIBUTE *template, CK_ULONG count);
    CK_RV (*sdb_CreateObject)(SDB *sdb, CK_OBJECT_HANDLE *object, 
				const CK_ATTRIBUTE *template, CK_ULONG count);
    CK_RV (*sdb_DestroyObject)(SDB *sdb, CK_OBJECT_HANDLE object);
    CK_RV (*sdb_GetMetaData)(SDB *sdb, const char *id, 
				SECItem *item1, SECItem *item2);
    CK_RV (*sdb_PutMetaData)(SDB *sdb, const char *id,
				const SECItem *item1, const SECItem *item2);
    CK_RV (*sdb_Begin)(SDB *sdb);
    CK_RV (*sdb_Commit)(SDB *sdb);
    CK_RV (*sdb_Abort)(SDB *sdb);
    CK_RV (*sdb_Reset)(SDB *sdb);
    CK_RV (*sdb_Close)(SDB *sdb);
    void (*sdb_SetForkState)(PRBool forked);
};

CK_RV s_open(const char *directory, const char *certPrefix, 
	     const char *keyPrefix,
	     int cert_version, int key_version, 
	     int flags, SDB **certdb, SDB **keydb, int *newInit);
CK_RV s_shutdown();


#define SDB_RDONLY      1
#define SDB_RDWR        2
#define SDB_CREATE      4
#define SDB_HAS_META    8

#endif
