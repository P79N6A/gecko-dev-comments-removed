







































#include "lowkeyi.h"
#include "pcert.h"
#include "keydbi.h"
#include "lgdb.h"
#include "secoid.h"
#include "prenv.h"

typedef struct LGPrivateStr {
    NSSLOWCERTCertDBHandle *certDB;
    NSSLOWKEYDBHandle *keyDB;
    PRLock *dbLock;
    PLHashTable *hashTable;
} LGPrivate;

static char *
lg_certdb_name_cb(void *arg, int dbVersion)
{
    const char *configdir = (const char *)arg;
    const char *dbver;
    char *smpname = NULL;
    char *dbname = NULL;

    switch (dbVersion) {
      case 8:
	dbver = "8";
	break;
      case 7:
	dbver = "7";
	break;
      case 6:
	dbver = "6";
	break;
      case 5:
	dbver = "5";
	break;
      case 4:
      default:
	dbver = "";
	break;
    }

    

    smpname = PR_smprintf(CERT_DB_FMT, configdir, dbver);
    if (smpname) {
	dbname = PORT_Strdup(smpname);
	PR_smprintf_free(smpname);
    }
    return dbname;
}
    
static char *
lg_keydb_name_cb(void *arg, int dbVersion)
{
    const char *configdir = (const char *)arg;
    const char *dbver;
    char *smpname = NULL;
    char *dbname = NULL;
    
    switch (dbVersion) {
      case 4:
	dbver = "4";
	break;
      case 3:
	dbver = "3";
	break;
      case 1:
	dbver = "1";
	break;
      case 2:
      default:
	dbver = "";
	break;
    }

    smpname = PR_smprintf(KEY_DB_FMT, configdir, dbver);
    if (smpname) {
	dbname = PORT_Strdup(smpname);
	PR_smprintf_free(smpname);
    }
    return dbname;
}

const char *
lg_EvaluateConfigDir(const char *configdir,char **appName)
{
    if (PORT_Strncmp(configdir, MULTIACCESS, sizeof(MULTIACCESS)-1) == 0) {
	char *cdir;

	*appName = PORT_Strdup(configdir+sizeof(MULTIACCESS)-1);
	if (*appName == NULL) {
	    return configdir;
	}
	cdir = *appName;
	while (*cdir && *cdir != ':') {
	    cdir++;
	}
	if (*cdir == ':') {
	   *cdir = 0;
	   cdir++;
	}
	configdir = cdir;
    }
    return configdir;
}

static int rdbmapflags(int flags);
static rdbfunc lg_rdbfunc = NULL;
static rdbstatusfunc lg_rdbstatusfunc = NULL;


#define RDBLIB SHLIB_PREFIX"rdb."SHLIB_SUFFIX

DB * rdbopen(const char *appName, const char *prefix, 
			const char *type, int flags, int *status)
{
    PRLibrary *lib;
    DB *db;
    char *disableUnload = NULL;

    if (lg_rdbfunc) {
	db = (*lg_rdbfunc)(appName,prefix,type,rdbmapflags(flags));
	if (!db && status && lg_rdbstatusfunc) {
	    *status = (*lg_rdbstatusfunc)();
	}
	return db;
    }

    


    lib = PR_LoadLibrary(RDBLIB);

    if (!lib) {
	return NULL;
    }

    
    lg_rdbstatusfunc = (rdbstatusfunc) PR_FindSymbol(lib,"rdbstatus");
    lg_rdbfunc = (rdbfunc) PR_FindSymbol(lib,"rdbopen");
    if (lg_rdbfunc) {
	db = (*lg_rdbfunc)(appName,prefix,type,rdbmapflags(flags));
	if (!db && status && lg_rdbstatusfunc) {
	    *status = (*lg_rdbstatusfunc)();
	}
	return db;
    }

    
    disableUnload = PR_GetEnv("NSS_DISABLE_UNLOAD");
    if (!disableUnload) {
        PR_UnloadLibrary(lib);
    }
    return NULL;
}




struct RDBStr {
    DB	db;
    int (*xactstart)(DB *db);
    int (*xactdone)(DB *db, PRBool abort);
    int version;
    int (*dbinitcomplete)(DB *db);
};

#define DB_RDB ((DBTYPE) 0xff)
#define RDB_RDONLY	1
#define RDB_RDWR 	2
#define RDB_CREATE      4

static int
rdbmapflags(int flags) {
   switch (flags) {
   case NO_RDONLY:
	return RDB_RDONLY;
   case NO_RDWR:
	return RDB_RDWR;
   case NO_CREATE:
	return RDB_CREATE;
   default:
	break;
   }
   return 0;
}

PRBool
db_IsRDB(DB *db)
{
    return (PRBool) db->type == DB_RDB;
}

int
db_BeginTransaction(DB *db)
{
    struct RDBStr *rdb = (struct RDBStr *)db;
    if (db->type != DB_RDB) {
	return 0;
    }

    return rdb->xactstart(db);
}

int
db_FinishTransaction(DB *db, PRBool abort)
{
    struct RDBStr *rdb = (struct RDBStr *)db;
    if (db->type != DB_RDB) {
	return 0;
    }

    return rdb->xactdone(db, abort);
}

static DB *
lg_getRawDB(SDB *sdb)
{
    NSSLOWCERTCertDBHandle *certDB;
    NSSLOWKEYDBHandle *keyDB;

    certDB = lg_getCertDB(sdb);
    if (certDB) {
	return certDB->permCertDB;
    }
    keyDB = lg_getKeyDB(sdb);
    if (keyDB) {
	return keyDB->db;
    }
    return NULL;
}

CK_RV
lg_Begin(SDB *sdb)
{
    DB *db = lg_getRawDB(sdb);
    int ret;

    if (db == NULL) {
	return CKR_GENERAL_ERROR; 
    }
    ret = db_BeginTransaction(db);
    if (ret != 0) {
	return CKR_GENERAL_ERROR; 
    }
    return CKR_OK;
}

CK_RV
lg_Commit(SDB *sdb)
{
    DB *db = lg_getRawDB(sdb);
    int ret;

    if (db == NULL) {
	return CKR_GENERAL_ERROR; 
    }
    ret = db_FinishTransaction(db, PR_FALSE);
    if (ret != 0) {
	return CKR_GENERAL_ERROR; 
    }
    return CKR_OK;
}

CK_RV
lg_Abort(SDB *sdb)
{
    DB *db = lg_getRawDB(sdb);
    int ret;

    if (db == NULL) {
	return CKR_GENERAL_ERROR; 
    }
    ret = db_FinishTransaction(db, PR_TRUE);
    if (ret != 0) {
	return CKR_GENERAL_ERROR; 
    }
    return CKR_OK;
}

int
db_InitComplete(DB *db)
{
    struct RDBStr *rdb = (struct RDBStr *)db;
    if (db->type != DB_RDB) {
	return 0;
    }
    


    if (!lg_rdbstatusfunc) {
	return 0;
    }

    return rdb->dbinitcomplete(db);
}



SECStatus
db_Copy(DB *dest,DB *src)
{
    int ret;
    DBT key,data;
    ret = (*src->seq)(src, &key, &data, R_FIRST);
    if (ret)  {
	return SECSuccess;
    }

    do {
	(void)(*dest->put)(dest,&key,&data, R_NOOVERWRITE);
    } while ( (*src->seq)(src, &key, &data, R_NEXT) == 0);
    (void)(*dest->sync)(dest,0);

    return SECSuccess;
}


static CK_RV
lg_OpenCertDB(const char * configdir, const char *prefix, PRBool readOnly,
    					    NSSLOWCERTCertDBHandle **certdbPtr)
{
    NSSLOWCERTCertDBHandle *certdb = NULL;
    CK_RV        crv = CKR_NETSCAPE_CERTDB_FAILED;
    SECStatus    rv;
    char * name = NULL;
    char * appName = NULL;

    if (prefix == NULL) {
	prefix = "";
    }

    configdir = lg_EvaluateConfigDir(configdir, &appName);

    name = PR_smprintf("%s" PATH_SEPARATOR "%s",configdir,prefix);
    if (name == NULL) goto loser;

    certdb = (NSSLOWCERTCertDBHandle*)PORT_ZAlloc(sizeof(NSSLOWCERTCertDBHandle));
    if (certdb == NULL) 
    	goto loser;

    certdb->ref = 1;

    rv = nsslowcert_OpenCertDB(certdb, readOnly, appName, prefix,
				lg_certdb_name_cb, (void *)name, PR_FALSE);
    if (rv == SECSuccess) {
	crv = CKR_OK;
	*certdbPtr = certdb;
	certdb = NULL;
    }
loser: 
    if (certdb) PR_Free(certdb);
    if (name) PR_smprintf_free(name);
    if (appName) PORT_Free(appName);
    return crv;
}

static CK_RV
lg_OpenKeyDB(const char * configdir, const char *prefix, PRBool readOnly,
    						NSSLOWKEYDBHandle **keydbPtr)
{
    NSSLOWKEYDBHandle *keydb;
    char * name = NULL;
    char * appName = NULL;

    if (prefix == NULL) {
	prefix = "";
    }
    configdir = lg_EvaluateConfigDir(configdir, &appName);

    name = PR_smprintf("%s" PATH_SEPARATOR "%s",configdir,prefix);	
    if (name == NULL) 
	return CKR_HOST_MEMORY;
    keydb = nsslowkey_OpenKeyDB(readOnly, appName, prefix, 
					lg_keydb_name_cb, (void *)name);
    PR_smprintf_free(name);
    if (appName) PORT_Free(appName);
    if (keydb == NULL)
	return CKR_NETSCAPE_KEYDB_FAILED;
    *keydbPtr = keydb;

    return CKR_OK;
}




void
lg_DBLock(SDB *sdb) 
{
    LGPrivate *lgdb_p = (LGPrivate *)sdb->private;
    SKIP_AFTER_FORK(PR_Lock(lgdb_p->dbLock));
}

void
lg_DBUnlock(SDB *sdb) 
{
    LGPrivate *lgdb_p = (LGPrivate *)sdb->private;
    SKIP_AFTER_FORK(PR_Unlock(lgdb_p->dbLock));
}

PLHashTable *
lg_GetHashTable(SDB *sdb) 
{
    LGPrivate *lgdb_p = (LGPrivate *)sdb->private;
    return lgdb_p->hashTable;
}

NSSLOWCERTCertDBHandle *
lg_getCertDB(SDB *sdb)
{
    LGPrivate *lgdb_p = (LGPrivate *)sdb->private;

    return lgdb_p->certDB;
}

NSSLOWKEYDBHandle *
lg_getKeyDB(SDB *sdb)
{
    LGPrivate *lgdb_p = (LGPrivate *)sdb->private;

    return lgdb_p->keyDB;
}

PRBool parentForkedAfterC_Initialize;

void lg_SetForkState(PRBool forked)
{
    parentForkedAfterC_Initialize = forked;
}

CK_RV
lg_Close(SDB *sdb)
{
    LGPrivate *lgdb_p = (LGPrivate *)sdb->private;
    lg_ClearTokenKeyHashTable(sdb);
    if (lgdb_p) {
    	if (lgdb_p->certDB) {
	    nsslowcert_ClosePermCertDB(lgdb_p->certDB);
	} else if (lgdb_p->keyDB) {
	    nsslowkey_CloseKeyDB(lgdb_p->keyDB);
	}
	if (lgdb_p->dbLock) {
	    SKIP_AFTER_FORK(PR_DestroyLock(lgdb_p->dbLock));
	}
	if (lgdb_p->hashTable) {
	    PL_HashTableDestroy(lgdb_p->hashTable);
	}
	PORT_Free(lgdb_p);
    }
    PORT_Free(sdb);
    return CKR_OK;
}

static PLHashNumber
lg_HashNumber(const void *key)
{
    return (PLHashNumber) key;
}

PRIntn
lg_CompareValues(const void *v1, const void *v2)
{
    PLHashNumber value1 = (PLHashNumber) v1;
    PLHashNumber value2 = (PLHashNumber) v2;
    return (value1 == value2);
}





CK_RV 
lg_init(SDB **pSdb, int flags, NSSLOWCERTCertDBHandle *certdbPtr,
	 NSSLOWKEYDBHandle *keydbPtr)
{
    SDB *sdb = NULL;
    LGPrivate *lgdb_p = NULL;
    CK_RV error = CKR_HOST_MEMORY;

    *pSdb = NULL;
    sdb = (SDB *) PORT_Alloc(sizeof(SDB));
    if (sdb == NULL) {
	goto loser;
    }
    lgdb_p = (LGPrivate *) PORT_Alloc(sizeof(LGPrivate));
    if (lgdb_p == NULL) {
	goto loser;
    }
    
    lgdb_p->certDB = certdbPtr;
    lgdb_p->keyDB = keydbPtr;
    lgdb_p->dbLock = PR_NewLock();
    if (lgdb_p->dbLock == NULL) {
	goto loser;
    }
    lgdb_p->hashTable = PL_NewHashTable(64, lg_HashNumber, lg_CompareValues,
			SECITEM_HashCompare, NULL, 0);
    if (lgdb_p->hashTable == NULL) {
	goto loser;
    }

    sdb->sdb_type = SDB_LEGACY;
    sdb->sdb_flags = flags;
    sdb->private = lgdb_p;
    sdb->sdb_FindObjectsInit = lg_FindObjectsInit;
    sdb->sdb_FindObjects = lg_FindObjects;
    sdb->sdb_FindObjectsFinal = lg_FindObjectsFinal;
    sdb->sdb_GetAttributeValue = lg_GetAttributeValue;
    sdb->sdb_SetAttributeValue = lg_SetAttributeValue;
    sdb->sdb_CreateObject = lg_CreateObject;
    sdb->sdb_DestroyObject = lg_DestroyObject;
    sdb->sdb_GetMetaData = lg_GetMetaData;
    sdb->sdb_PutMetaData = lg_PutMetaData;
    sdb->sdb_Begin = lg_Begin;
    sdb->sdb_Commit = lg_Commit;
    sdb->sdb_Abort = lg_Abort;
    sdb->sdb_Reset = lg_Reset;
    sdb->sdb_Close = lg_Close;
    sdb->sdb_SetForkState = lg_SetForkState;

    *pSdb = sdb;
    return CKR_OK;

loser:
    if (sdb) {
	PORT_Free(sdb);
    }
    if (lgdb_p) {
	if (lgdb_p->dbLock) {
	    PR_DestroyLock(lgdb_p->dbLock);
	}
	if (lgdb_p->hashTable) {
	    PL_HashTableDestroy(lgdb_p->hashTable);
	}
	PORT_Free(lgdb_p);
    }
    return error;

}


















CK_RV
legacy_Open(const char *configdir, const char *certPrefix, 
	    const char *keyPrefix, int certVersion, int keyVersion,
	    int flags, SDB **certDB, SDB **keyDB)
{
    CK_RV crv = CKR_OK;
    SECStatus rv;
    PRBool readOnly = (flags == SDB_RDONLY)? PR_TRUE: PR_FALSE;

    rv = SECOID_Init();
    if (SECSuccess != rv) {
        return CKR_DEVICE_ERROR;
    }
    nsslowcert_InitLocks();

    if (keyDB) *keyDB = NULL;
    if (certDB) *certDB = NULL;

    if (certDB) {
	NSSLOWCERTCertDBHandle *certdbPtr;

	crv = lg_OpenCertDB(configdir, certPrefix, readOnly, &certdbPtr);
	if (crv != CKR_OK) {
	    goto loser;
	}
	crv = lg_init(certDB, flags, certdbPtr, NULL);
	if (crv != CKR_OK) {
	    nsslowcert_ClosePermCertDB(certdbPtr);
	    goto loser;
	}
    }
    if (keyDB) {
	NSSLOWKEYDBHandle *keydbPtr;

	crv = lg_OpenKeyDB(configdir, keyPrefix, readOnly, &keydbPtr);
	if (crv != CKR_OK) {
	    goto loser;
	}
	crv = lg_init(keyDB, flags, NULL, keydbPtr);
	if (crv != CKR_OK) {
	    nsslowkey_CloseKeyDB(keydbPtr);
	    goto loser;
	}
	if (certDB && *certDB) {
	    LGPrivate *lgdb_p = (LGPrivate *)(*certDB)->private;
	    lgdb_p->keyDB = keydbPtr;
	}
    }

loser:
    if (crv != CKR_OK) {
	if (keyDB && *keyDB) {
	    lg_Close(*keyDB);
	    *keyDB = NULL;
	}
	if (certDB && *certDB) {
	    lg_Close(*certDB);
	    *certDB = NULL;
	}
    }
    return crv;
}

CK_RV
legacy_Shutdown(PRBool forked)
{
    lg_SetForkState(forked);
    nsslowcert_DestroyFreeLists();
    nsslowcert_DestroyGlobalLocks();
    SECOID_Shutdown();
    lg_SetForkState(PR_FALSE);
    return CKR_OK;
}

