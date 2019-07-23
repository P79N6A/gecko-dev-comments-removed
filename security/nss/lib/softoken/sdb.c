




















































#include "sdb.h"
#include "pkcs11t.h"
#include "seccomon.h"
#include <sqlite3.h>
#include "prthread.h"
#include "prio.h"
#include "stdio.h"
#include "secport.h"
#include "prmon.h"
#include "prenv.h"
#include "prsystem.h" 
#include "sys/stat.h"
#if defined (_WIN32)
#include <io.h>
#endif

#ifdef SQLITE_UNSAFE_THREADS
#include "prlock.h"





static PRLock *sqlite_lock = NULL;

#define LOCK_SQLITE()  PR_Lock(sqlite_lock);
#define UNLOCK_SQLITE()  PR_Unlock(sqlite_lock);
#else
#define LOCK_SQLITE()  
#define UNLOCK_SQLITE()  
#endif

typedef enum {
	SDB_CERT = 1,
	SDB_KEY = 2
} sdbDataType;






















#define SDB_SQLITE_BUSY_TIMEOUT 1000 /* milliseconds */
#define SDB_BUSY_RETRY_TIME        5 /* seconds */
#define SDB_MAX_BUSY_RETRIES      10














struct SDBPrivateStr {
    char *sqlDBName;		
    sqlite3 *sqlXactDB;		

    PRThread *sqlXactThread;	

    sqlite3 *sqlReadDB;		
    PRIntervalTime lastUpdateTime;  
    PRIntervalTime updateInterval;  

    sdbDataType type;		
    char *table;	        
    char *cacheTable;	        
    PRMonitor *dbMon;		

};

typedef struct SDBPrivateStr SDBPrivate;




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
    CKA_SUPPORTED_CMS_ATTRIBUTES, CKA_NETSCAPE_URL, CKA_NETSCAPE_EMAIL,
    CKA_NETSCAPE_SMIME_INFO, CKA_NETSCAPE_SMIME_TIMESTAMP,
    CKA_NETSCAPE_PKCS8_SALT, CKA_NETSCAPE_PASSWORD_CHECK, CKA_NETSCAPE_EXPIRES,
    CKA_NETSCAPE_KRL, CKA_NETSCAPE_PQG_COUNTER, CKA_NETSCAPE_PQG_SEED,
    CKA_NETSCAPE_PQG_H, CKA_NETSCAPE_PQG_SEED_BITS, CKA_NETSCAPE_MODULE_SPEC,
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








const unsigned char SQLITE_EXPLICIT_NULL[] = { 0xa5, 0x0, 0x5a };
#define SQLITE_EXPLICIT_NULL_LEN 3




static int 
sdb_done(int err, int *count)
{
    
    if (err == SQLITE_ROW) {
	*count = 0;
	return 0;
    }
    if (err != SQLITE_BUSY) {
	return 1;
    }
    
    if (++(*count) >= SDB_MAX_BUSY_RETRIES) {
	return 1;
    }
    return 0;
}








static char *
sdb_strndup(const char *file, int len)
{
   char *result = PORT_Alloc(len+1);

   if (result == NULL) {
	return result;
   }

   PORT_Memcpy(result, file, len);
   result[len] = 0;
   return result;
}





static int 
sdb_getTempDirCallback(void *arg, int columnCount, char **cval, char **cname)
{
    int i;
    int found = 0;
    char *file = NULL;
    char *end, *dir;
    char dirsep;

    
    if (*(char **)arg) {
	return SQLITE_OK;
    }

    

    for (i=0; i < columnCount; i++) {
	if (PORT_Strcmp(cname[i],"name") == 0) {
	    if (PORT_Strcmp(cval[i], "temp") == 0) {
		found++;
		continue;
	    }
	}
	if (PORT_Strcmp(cname[i],"file") == 0) {
	    if (cval[i] && (*cval[i] != 0)) {
		file = cval[i];
	    }
	}
    }

    
    if (!found || !file) {
	return SQLITE_OK;
    }

    
    dirsep = PR_GetDirectorySeparator();
    end = PORT_Strrchr(file, dirsep);
    if (!end) {
	return SQLITE_OK;
    }
    dir = sdb_strndup(file, end-file);

    *(char **)arg = dir;
    return SQLITE_OK;
}





static char *
sdb_getTempDir(sqlite3 *sqlDB)
{
    char *tempDir = NULL;
    int sqlerr;

    
    sqlerr = sqlite3_exec(sqlDB, "CREATE TEMPORARY TABLE myTemp (id)",
			  NULL, 0, NULL);
    if (sqlerr != SQLITE_OK) {
	return NULL;
    }
    
    sqlerr = sqlite3_exec(sqlDB, "PRAGMA database_list", 
		sdb_getTempDirCallback, &tempDir, NULL);

    
    sqlite3_exec(sqlDB, "DROP TABLE myTemp", NULL, 0, NULL);

    if (sqlerr != SQLITE_OK) {
	return NULL;
    }
    return tempDir;
}





static CK_RV
sdb_mapSQLError(sdbDataType type, int sqlerr)
{
    switch (sqlerr) {
    
    case SQLITE_OK:
    case SQLITE_DONE:
	return CKR_OK;
    case SQLITE_NOMEM:
	return CKR_HOST_MEMORY;
    case SQLITE_READONLY:
	return CKR_TOKEN_WRITE_PROTECTED;
    
    case SQLITE_AUTH:
    case SQLITE_PERM:
	
    case SQLITE_CANTOPEN:
    case SQLITE_NOTFOUND:
	
	return type == SDB_CERT ? 
		CKR_NETSCAPE_CERTDB_FAILED : CKR_NETSCAPE_KEYDB_FAILED;
    case SQLITE_IOERR:
	return CKR_DEVICE_ERROR;
    default:
	break;
    }
    return CKR_GENERAL_ERROR;
}





static char *sdb_BuildFileName(const char * directory, 
			const char *prefix, const char *type, 
			int version, int flags)
{
    char *dbname = NULL;
    
    dbname = sqlite3_mprintf("%s/%s%s%d.db",directory, prefix, type, version);
    return dbname;
}






static PRUint32
sdb_measureAccess(const char *directory)
{
    PRUint32 i;
    PRIntervalTime time;
    PRIntervalTime delta;
    PRIntervalTime duration = PR_MillisecondsToInterval(33);

    
    if (directory == NULL) {
	return 1;
    }

    


    time =  PR_IntervalNow();
    for (i=0; i < 10000u; i++) { 
	char *temp;
	PRIntervalTime next;

        temp  = sdb_BuildFileName(directory,"","._dOeSnotExist_", time+i, 0);
	PR_Access(temp,PR_ACCESS_EXISTS);
        sqlite3_free(temp);
	next = PR_IntervalNow();
	delta = next - time;
	if (delta >= duration)
	    break;
    }

    
    return i ? i : 1u;
}









static const char DROP_CACHE_CMD[] = "DROP TABLE %s";
static const char CREATE_CACHE_CMD[] =
  "CREATE TEMPORARY TABLE %s AS SELECT * FROM %s";
static const char CREATE_ISSUER_INDEX_CMD[] = 
  "CREATE INDEX issuer ON %s (a81)";
static const char CREATE_SUBJECT_INDEX_CMD[] = 
  "CREATE INDEX subject ON %s (a101)";
static const char CREATE_LABEL_INDEX_CMD[] = "CREATE INDEX label ON %s (a3)";
static const char CREATE_ID_INDEX_CMD[] = "CREATE INDEX ckaid ON %s (a102)";

static CK_RV
sdb_buildCache(sqlite3 *sqlDB, sdbDataType type, 
		const char *cacheTable, const char *table)
{
    char *newStr;
    int sqlerr = SQLITE_OK;

    newStr = sqlite3_mprintf(CREATE_CACHE_CMD, cacheTable, table);
    if (newStr == NULL) {
	return CKR_HOST_MEMORY;
    }
    sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
    sqlite3_free(newStr);
    if (sqlerr != SQLITE_OK) {
	return sdb_mapSQLError(type, sqlerr); 
    }
    
    newStr = sqlite3_mprintf(CREATE_ISSUER_INDEX_CMD, cacheTable);
    if (newStr == NULL) {
	return CKR_OK;
    }
    sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
    sqlite3_free(newStr);
    newStr = sqlite3_mprintf(CREATE_SUBJECT_INDEX_CMD, cacheTable);
    if (newStr == NULL) {
	return CKR_OK;
    }
    sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
    sqlite3_free(newStr);
    newStr = sqlite3_mprintf(CREATE_LABEL_INDEX_CMD, cacheTable);
    if (newStr == NULL) {
	return CKR_OK;
    }
    sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
    sqlite3_free(newStr);
    newStr = sqlite3_mprintf(CREATE_ID_INDEX_CMD, cacheTable);
    if (newStr == NULL) {
	return CKR_OK;
    }
    sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
    sqlite3_free(newStr);
    return CKR_OK;
}





static CK_RV
sdb_updateCache(SDBPrivate *sdb_p)
{
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    char *newStr;

    
    newStr = sqlite3_mprintf(DROP_CACHE_CMD, sdb_p->cacheTable);
    if (newStr == NULL) {
	return CKR_HOST_MEMORY;
    }
    sqlerr = sqlite3_exec(sdb_p->sqlReadDB, newStr, NULL, 0, NULL);
    sqlite3_free(newStr);
    if ((sqlerr != SQLITE_OK) && (sqlerr != SQLITE_ERROR )) {
        


	return sdb_mapSQLError(sdb_p->type, sqlerr); 
    }
	

    
    error = sdb_buildCache(sdb_p->sqlReadDB,sdb_p->type,
				sdb_p->cacheTable,sdb_p->table );
    if (error == CKR_OK) {
	
	sdb_p->lastUpdateTime = PR_IntervalNow();
    }
    return error;
}


































static CK_RV 
sdb_openDBLocal(SDBPrivate *sdb_p, sqlite3 **sqlDB, const char **table)
{
    *sqlDB = NULL;

    PR_EnterMonitor(sdb_p->dbMon);

    if (table) {
	*table = sdb_p->table;
    }

    
    if ((sdb_p->sqlXactDB) && (sdb_p->sqlXactThread == PR_GetCurrentThread())) {
	*sqlDB =sdb_p->sqlXactDB;
	
        PR_ExitMonitor(sdb_p->dbMon);
	return CKR_OK;
    }

    





    if (table && sdb_p->cacheTable) {
	PRIntervalTime now = PR_IntervalNow();
	if ((now - sdb_p->lastUpdateTime) > sdb_p->updateInterval) {
	       sdb_updateCache(sdb_p);
        }
	*table = sdb_p->cacheTable;
    }

    *sqlDB = sdb_p->sqlReadDB;

    

	
    return CKR_OK;
}


static CK_RV 
sdb_closeDBLocal(SDBPrivate *sdb_p, sqlite3 *sqlDB) 
{
   if (sdb_p->sqlXactDB != sqlDB) {
	
        PR_ExitMonitor(sdb_p->dbMon);
   }
   return CKR_OK;
}





static int
sdb_openDB(const char *name, sqlite3 **sqlDB, int flags)
{
    int sqlerr;
    




    *sqlDB = NULL;
    sqlerr = sqlite3_open(name, sqlDB);
    if (sqlerr != SQLITE_OK) {
	return sqlerr;
    }

    sqlerr = sqlite3_busy_timeout(*sqlDB, SDB_SQLITE_BUSY_TIMEOUT);
    if (sqlerr != SQLITE_OK) {
	sqlite3_close(*sqlDB);
	*sqlDB = NULL;
	return sqlerr;
    }
    return SQLITE_OK;
}





static int 
sdb_reopenDBLocal(SDBPrivate *sdb_p, sqlite3 **sqlDB) {
    sqlite3 *newDB;
    int sqlerr;

    
    sqlerr = sdb_openDB(sdb_p->sqlDBName, &newDB, SDB_RDONLY);
    if (sqlerr != SQLITE_OK) {
	return sqlerr;
    }

    


    PR_EnterMonitor(sdb_p->dbMon);
    
    if (sdb_p->sqlReadDB == *sqlDB) {
	sdb_p->sqlReadDB = newDB;
    } else if (sdb_p->sqlXactDB == *sqlDB) {
	sdb_p->sqlXactDB = newDB;
    }
    PR_ExitMonitor(sdb_p->dbMon);

    
    sqlite3_close(*sqlDB);

    *sqlDB = newDB;
    return SQLITE_OK;
}

struct SDBFindStr {
    sqlite3 *sqlDB;
    sqlite3_stmt *findstmt;
};


static const char FIND_OBJECTS_CMD[] =  "SELECT ALL * FROM %s WHERE %s;";
static const char FIND_OBJECTS_ALL_CMD[] = "SELECT ALL * FROM %s;";
CK_RV
sdb_FindObjectsInit(SDB *sdb, const CK_ATTRIBUTE *template, CK_ULONG count, 
				SDBFind **find)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    const char *table;
    char *newStr, *findStr = NULL;
    sqlite3_stmt *findstmt = NULL;
    char *join="";
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    int i;

    LOCK_SQLITE()
    *find = NULL;
    error = sdb_openDBLocal(sdb_p, &sqlDB, &table);
    if (error != CKR_OK) {
	goto loser;
    }

    findStr = sqlite3_mprintf("");
    for (i=0; findStr && i < count; i++) {
	newStr = sqlite3_mprintf("%s%sa%x=$DATA%d", findStr, join,
				template[i].type, i);
        join=" AND ";
	sqlite3_free(findStr);
	findStr = newStr;
    }

    if (findStr == NULL) {
	error = CKR_HOST_MEMORY;
	goto loser;
    }

    if (count == 0) {
	newStr = sqlite3_mprintf(FIND_OBJECTS_ALL_CMD, table);
    } else {
	newStr = sqlite3_mprintf(FIND_OBJECTS_CMD, table, findStr);
    }
    sqlite3_free(findStr);
    if (newStr == NULL) {
	error = CKR_HOST_MEMORY;
	goto loser;
    }
    sqlerr = sqlite3_prepare_v2(sqlDB, newStr, -1, &findstmt, NULL);
    sqlite3_free(newStr);
    for (i=0; sqlerr == SQLITE_OK && i < count; i++) {
	const void *blobData = template[i].pValue;
	unsigned int blobSize = template[i].ulValueLen;
	if (blobSize == 0) {
	    blobSize = SQLITE_EXPLICIT_NULL_LEN;
	    blobData = SQLITE_EXPLICIT_NULL;
	}
	sqlerr = sqlite3_bind_blob(findstmt, i+1, blobData, blobSize,
				   SQLITE_TRANSIENT);
    }
    if (sqlerr == SQLITE_OK) {
	*find = PORT_New(SDBFind);
	if (*find == NULL) {
	    error = CKR_HOST_MEMORY;
	    goto loser;
	}
	(*find)->findstmt = findstmt;
	(*find)->sqlDB = sqlDB;
	UNLOCK_SQLITE()  
	return CKR_OK;
    } 
    error = sdb_mapSQLError(sdb_p->type, sqlerr);

loser: 
    if (findstmt) {
	sqlite3_reset(findstmt);
	sqlite3_finalize(findstmt);
    }
    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }
    UNLOCK_SQLITE()  
    return error;
}


CK_RV
sdb_FindObjects(SDB *sdb, SDBFind *sdbFind, CK_OBJECT_HANDLE *object, 
		CK_ULONG arraySize, CK_ULONG *count)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3_stmt *stmt = sdbFind->findstmt;
    int sqlerr = SQLITE_OK;
    int retry = 0;

    *count = 0;

    if (arraySize == 0) {
	return CKR_OK;
    }
    LOCK_SQLITE()  

    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
	if (sqlerr == SQLITE_ROW) {
	    
	    *object++= sqlite3_column_int(stmt, 0);
	    arraySize--;
	    (*count)++;
	}
    } while (!sdb_done(sqlerr,&retry) && (arraySize > 0));

    

    if (sqlerr == SQLITE_ROW && arraySize == 0) {
	sqlerr = SQLITE_DONE;
    }
    UNLOCK_SQLITE()  

    return sdb_mapSQLError(sdb_p->type, sqlerr);
}

CK_RV
sdb_FindObjectsFinal(SDB *sdb, SDBFind *sdbFind)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3_stmt *stmt = sdbFind->findstmt;
    sqlite3 *sqlDB = sdbFind->sqlDB;
    int sqlerr = SQLITE_OK;

    LOCK_SQLITE()  
    if (stmt) {
	sqlite3_reset(stmt);
	sqlerr = sqlite3_finalize(stmt);
    }
    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }
    PORT_Free(sdbFind);

    UNLOCK_SQLITE()  
    return sdb_mapSQLError(sdb_p->type, sqlerr);
}

static const char GET_ATTRIBUTE_CMD[] = "SELECT ALL %s FROM %s WHERE id=$ID;";
CK_RV
sdb_GetAttributeValueNoLock(SDB *sdb, CK_OBJECT_HANDLE object_id, 
				CK_ATTRIBUTE *template, CK_ULONG count)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    sqlite3_stmt *stmt = NULL;
    char *getStr = NULL;
    char *newStr = NULL;
    const char *table = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    int found = 0;
    int retry = 0;
    int i;


    
    error = sdb_openDBLocal(sdb_p, &sqlDB, &table);
    if (error != CKR_OK) {
	goto loser;
    }

    getStr = sqlite3_mprintf("");
    for (i=0; getStr && i < count; i++) {
	if (i==0) {
	    newStr = sqlite3_mprintf("a%x", template[i].type);
	} else {
	    newStr = sqlite3_mprintf("%s, a%x", getStr, template[i].type);
	}
	sqlite3_free(getStr);
	getStr = newStr;
    }

    if (getStr == NULL) {
	error = CKR_HOST_MEMORY;
	goto loser;
    }

    newStr = sqlite3_mprintf(GET_ATTRIBUTE_CMD, getStr, table);
    sqlite3_free(getStr);
    getStr = NULL;
    if (newStr == NULL) {
	error = CKR_HOST_MEMORY;
	goto loser;
    }

    sqlerr = sqlite3_prepare_v2(sqlDB, newStr, -1, &stmt, NULL);
    if (sqlerr != SQLITE_OK) { goto loser; }
    sqlerr = sqlite3_bind_int(stmt, 1, object_id);
    if (sqlerr != SQLITE_OK) { goto loser; }
    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
	if (sqlerr == SQLITE_ROW) {
	    for (i=0; i < count; i++) {
		int column = i;
	    	int blobSize;
	    	const char *blobData;

	    	blobSize = sqlite3_column_bytes(stmt, column);
		blobData = sqlite3_column_blob(stmt, column);
		if (blobData == NULL) {
		    template[i].ulValueLen = -1;
		    error = CKR_ATTRIBUTE_TYPE_INVALID; 
		    continue;
		}
		

		if ((blobSize == SQLITE_EXPLICIT_NULL_LEN) &&
		   	(PORT_Memcmp(blobData, SQLITE_EXPLICIT_NULL, 
			      SQLITE_EXPLICIT_NULL_LEN) == 0)) {
		    blobSize = 0;
		}
		if (template[i].pValue) {
		    if (template[i].ulValueLen < blobSize) {
			template[i].ulValueLen = -1;
		    	error = CKR_BUFFER_TOO_SMALL;
			continue;
		    }
	    	    PORT_Memcpy(template[i].pValue, blobData, blobSize);
		}
		template[i].ulValueLen = blobSize;
	    }
	    found = 1;
	}
    } while (!sdb_done(sqlerr,&retry));

loser:
    
    if (error == CKR_OK) {
	error = sdb_mapSQLError(sdb_p->type, sqlerr);
	if (!found && error == CKR_OK) {
	    error = CKR_OBJECT_HANDLE_INVALID;
	}
    }
    if (newStr) {
	sqlite3_free(newStr);
    }

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

    
    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }
    return error;
}

CK_RV
sdb_GetAttributeValue(SDB *sdb, CK_OBJECT_HANDLE object_id, 
				CK_ATTRIBUTE *template, CK_ULONG count)
{
    CK_RV crv;

    if (count == 0) {
	return CKR_OK;
    }

    LOCK_SQLITE()  
    crv = sdb_GetAttributeValueNoLock(sdb, object_id, template, count);
    UNLOCK_SQLITE()  
    return crv;
}
   
static const char SET_ATTRIBUTE_CMD[] = "UPDATE %s SET %s WHERE id=$ID;";
CK_RV
sdb_SetAttributeValue(SDB *sdb, CK_OBJECT_HANDLE object_id, 
			const CK_ATTRIBUTE *template, CK_ULONG count)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    sqlite3_stmt *stmt = NULL;
    char *setStr = NULL;
    char *newStr = NULL;
    int sqlerr = SQLITE_OK;
    int retry = 0;
    CK_RV error = CKR_OK;
    int i;

    if ((sdb->sdb_flags & SDB_RDONLY) != 0) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    if (count == 0) {
	return CKR_OK;
    }

    LOCK_SQLITE()  
    setStr = sqlite3_mprintf("");
    for (i=0; setStr && i < count; i++) {
	if (i==0) {
	    sqlite3_free(setStr);
   	    setStr = sqlite3_mprintf("a%x=$VALUE%d", 
				template[i].type, i);
	    continue;
	}
	newStr = sqlite3_mprintf("%s,a%x=$VALUE%d", setStr, 
				template[i].type, i);
	sqlite3_free(setStr);
	setStr = newStr;
    }
    newStr = NULL;

    if (setStr == NULL) {
	return CKR_HOST_MEMORY;
    }
    newStr =  sqlite3_mprintf(SET_ATTRIBUTE_CMD, sdb_p->table, setStr);
    sqlite3_free(setStr);
    if (newStr == NULL) {
	UNLOCK_SQLITE()  
	return CKR_HOST_MEMORY;
    }
    error = sdb_openDBLocal(sdb_p, &sqlDB, NULL);
    if (error != CKR_OK) {
	goto loser;
    }
    sqlerr = sqlite3_prepare_v2(sqlDB, newStr, -1, &stmt, NULL);
    if (sqlerr != SQLITE_OK) goto loser;
    for (i=0; i < count; i++) {
	if (template[i].ulValueLen != 0) {
	    sqlerr = sqlite3_bind_blob(stmt, i+1, template[i].pValue, 
				template[i].ulValueLen, SQLITE_STATIC);
	} else {
	    sqlerr = sqlite3_bind_blob(stmt, i+2, SQLITE_EXPLICIT_NULL, 
			SQLITE_EXPLICIT_NULL_LEN, SQLITE_STATIC);
	}
        if (sqlerr != SQLITE_OK) goto loser;
    }
    sqlerr = sqlite3_bind_int(stmt, i+1, object_id);
    if (sqlerr != SQLITE_OK) goto loser;

    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
    } while (!sdb_done(sqlerr,&retry));

loser:
    if (newStr) {
	sqlite3_free(newStr);
    }
    if (error == CKR_OK) {
	error = sdb_mapSQLError(sdb_p->type, sqlerr);
    }

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }

    UNLOCK_SQLITE()  
    return error;
}




static PRBool
sdb_objectExists(SDB *sdb, CK_OBJECT_HANDLE candidate)
{
    CK_RV crv;
    CK_ATTRIBUTE template = { CKA_LABEL, NULL, 0 };

    crv = sdb_GetAttributeValueNoLock(sdb,candidate,&template, 1);
    if (crv == CKR_OBJECT_HANDLE_INVALID) {
	return PR_FALSE;
    }
    return PR_TRUE;
}





static CK_OBJECT_HANDLE
sdb_getObjectId(SDB *sdb)
{
    CK_OBJECT_HANDLE candidate;
    static CK_OBJECT_HANDLE next_obj = CK_INVALID_HANDLE;
    int count;
    


    if (next_obj == CK_INVALID_HANDLE) {
        PRTime time;
	time = PR_Now();

	next_obj = (CK_OBJECT_HANDLE)(time & 0x3fffffffL);
    }
    candidate = next_obj++;
    
    for (count = 0; count < 0x40000000; count++, candidate = next_obj++) {
	
	candidate &= 0x3fffffff;
	
	if (candidate == CK_INVALID_HANDLE) {
	    continue;
	}
	
	if (!sdb_objectExists(sdb, candidate)) {
	    
	    return candidate;
	}
    }

    
    return CK_INVALID_HANDLE;
}

static const char CREATE_CMD[] = "INSERT INTO %s (id%s) VALUES($ID%s);";
CK_RV
sdb_CreateObject(SDB *sdb, CK_OBJECT_HANDLE *object_id, 
		 const CK_ATTRIBUTE *template, CK_ULONG count)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    sqlite3_stmt *stmt = NULL;
    char *columnStr = NULL;
    char *valueStr = NULL;
    char *newStr = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    CK_OBJECT_HANDLE this_object = CK_INVALID_HANDLE;
    int retry = 0;
    int i;

    if ((sdb->sdb_flags & SDB_RDONLY) != 0) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    LOCK_SQLITE()  
    if ((*object_id != CK_INVALID_HANDLE) && 
		!sdb_objectExists(sdb, *object_id)) {
	this_object = *object_id;
    } else {
	this_object = sdb_getObjectId(sdb);
    }
    if (this_object == CK_INVALID_HANDLE) {
	UNLOCK_SQLITE();
	return CKR_HOST_MEMORY;
    }
    columnStr = sqlite3_mprintf("");
    valueStr = sqlite3_mprintf("");
    *object_id = this_object;
    for (i=0; columnStr && valueStr && i < count; i++) {
   	newStr = sqlite3_mprintf("%s,a%x", columnStr, template[i].type);
	sqlite3_free(columnStr);
	columnStr = newStr;
   	newStr = sqlite3_mprintf("%s,$VALUE%d", valueStr, i);
	sqlite3_free(valueStr);
	valueStr = newStr;
    }
    newStr = NULL;
    if ((columnStr == NULL) || (valueStr == NULL)) {
	if (columnStr) {
	    sqlite3_free(columnStr);
	}
	if (valueStr) {
	    sqlite3_free(valueStr);
	}
	UNLOCK_SQLITE()  
	return CKR_HOST_MEMORY;
    }
    newStr =  sqlite3_mprintf(CREATE_CMD, sdb_p->table, columnStr, valueStr);
    sqlite3_free(columnStr);
    sqlite3_free(valueStr);
    error = sdb_openDBLocal(sdb_p, &sqlDB, NULL);
    if (error != CKR_OK) {
	goto loser;
    }
    sqlerr = sqlite3_prepare_v2(sqlDB, newStr, -1, &stmt, NULL);
    if (sqlerr != SQLITE_OK) goto loser;
    sqlerr = sqlite3_bind_int(stmt, 1, *object_id);
    if (sqlerr != SQLITE_OK) goto loser;
    for (i=0; i < count; i++) {
	if (template[i].ulValueLen) {
	    sqlerr = sqlite3_bind_blob(stmt, i+2, template[i].pValue, 
			template[i].ulValueLen, SQLITE_STATIC);
	} else {
	    sqlerr = sqlite3_bind_blob(stmt, i+2, SQLITE_EXPLICIT_NULL, 
			SQLITE_EXPLICIT_NULL_LEN, SQLITE_STATIC);
	}
        if (sqlerr != SQLITE_OK) goto loser;
    }

    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
    } while (!sdb_done(sqlerr,&retry));

loser:
    if (newStr) {
	sqlite3_free(newStr);
    }
    if (error == CKR_OK) {
	error = sdb_mapSQLError(sdb_p->type, sqlerr);
    }

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }
    UNLOCK_SQLITE()  

    return error;
}

static const char DESTROY_CMD[] = "DELETE FROM %s WHERE (id=$ID);";
CK_RV
sdb_DestroyObject(SDB *sdb, CK_OBJECT_HANDLE object_id)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    sqlite3_stmt *stmt = NULL;
    char *newStr = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    int retry = 0;

    if ((sdb->sdb_flags & SDB_RDONLY) != 0) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    LOCK_SQLITE()  
    error = sdb_openDBLocal(sdb_p, &sqlDB, NULL);
    if (error != CKR_OK) {
	goto loser;
    }
    newStr =  sqlite3_mprintf(DESTROY_CMD, sdb_p->table);
    if (newStr == NULL) {
	error = CKR_HOST_MEMORY;
	goto loser;
    }
    sqlerr =sqlite3_prepare_v2(sqlDB, newStr, -1, &stmt, NULL);
    sqlite3_free(newStr);
    if (sqlerr != SQLITE_OK) goto loser;
    sqlerr =sqlite3_bind_int(stmt, 1, object_id);
    if (sqlerr != SQLITE_OK) goto loser;

    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
    } while (!sdb_done(sqlerr,&retry));

loser:
    if (error == CKR_OK) {
	error = sdb_mapSQLError(sdb_p->type, sqlerr);
    }

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }

    UNLOCK_SQLITE()  
    return error;
}
   
static const char BEGIN_CMD[] = "BEGIN IMMEDIATE TRANSACTION;";







CK_RV
sdb_Begin(SDB *sdb)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    sqlite3_stmt *stmt = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    int retry = 0;


    if ((sdb->sdb_flags & SDB_RDONLY) != 0) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }


    LOCK_SQLITE()  

    
    sqlerr = sdb_openDB(sdb_p->sqlDBName, &sqlDB, SDB_RDWR);
    if (sqlerr != SQLITE_OK) {
	goto loser;
    }

    sqlerr =sqlite3_prepare_v2(sqlDB, BEGIN_CMD, -1, &stmt, NULL);

    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
    } while (!sdb_done(sqlerr,&retry));

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

loser:
    error = sdb_mapSQLError(sdb_p->type, sqlerr);

    


    if (error == CKR_OK) {
	

	PR_EnterMonitor(sdb_p->dbMon);
	PORT_Assert(sdb_p->sqlXactDB == NULL);
	sdb_p->sqlXactDB = sqlDB;
	sdb_p->sqlXactThread = PR_GetCurrentThread();
        PR_ExitMonitor(sdb_p->dbMon);
    } else {
	

	if (sqlDB) {
	    sqlite3_close(sqlDB);
	}
    }

    UNLOCK_SQLITE()  
    return error;
}







static CK_RV 
sdb_complete(SDB *sdb, const char *cmd)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    sqlite3_stmt *stmt = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    int retry = 0;


    if ((sdb->sdb_flags & SDB_RDONLY) != 0) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    
    PR_EnterMonitor(sdb_p->dbMon);
    PORT_Assert(sdb_p->sqlXactDB);
    if (sdb_p->sqlXactDB == NULL) {
        PR_ExitMonitor(sdb_p->dbMon);
	return CKR_GENERAL_ERROR; 
    }
    PORT_Assert( sdb_p->sqlXactThread == PR_GetCurrentThread());
    if ( sdb_p->sqlXactThread != PR_GetCurrentThread()) {
        PR_ExitMonitor(sdb_p->dbMon);
	return CKR_GENERAL_ERROR; 
    }
    sqlDB = sdb_p->sqlXactDB;
    sdb_p->sqlXactDB = NULL; 

    sdb_p->sqlXactThread = NULL; 
    PR_ExitMonitor(sdb_p->dbMon);

    sqlerr =sqlite3_prepare_v2(sqlDB, cmd, -1, &stmt, NULL);

    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
    } while (!sdb_done(sqlerr,&retry));

    

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

    
    if (sdb_p->cacheTable) {
	PR_EnterMonitor(sdb_p->dbMon);
	sdb_updateCache(sdb_p);
	PR_ExitMonitor(sdb_p->dbMon);
    }

    error = sdb_mapSQLError(sdb_p->type, sqlerr);

    

    sqlite3_close(sqlDB);

    return error;
}

static const char COMMIT_CMD[] = "COMMIT TRANSACTION;";
CK_RV
sdb_Commit(SDB *sdb)
{
    CK_RV crv;
    LOCK_SQLITE()  
    crv = sdb_complete(sdb,COMMIT_CMD);
    UNLOCK_SQLITE()  
    return crv;
}

static const char ROLLBACK_CMD[] = "ROLLBACK TRANSACTION;";
CK_RV
sdb_Abort(SDB *sdb)
{
    CK_RV crv;
    LOCK_SQLITE()  
    crv = sdb_complete(sdb,ROLLBACK_CMD);
    UNLOCK_SQLITE()  
    return crv;
}

static int tableExists(sqlite3 *sqlDB, const char *tableName);

static const char GET_PW_CMD[] = "SELECT ALL * FROM metaData WHERE id=$ID;";
CK_RV
sdb_GetMetaData(SDB *sdb, const char *id, SECItem *item1, SECItem *item2)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = sdb_p->sqlXactDB;
    sqlite3_stmt *stmt = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    int found = 0;
    int retry = 0;

    LOCK_SQLITE()  
    error = sdb_openDBLocal(sdb_p, &sqlDB, NULL);
    if (error != CKR_OK) {
	goto loser;
    }

    
    sqlerr = sqlite3_prepare_v2(sqlDB, GET_PW_CMD, -1, &stmt, NULL);
    



    if (sqlerr == SQLITE_SCHEMA) {
	sqlerr = sdb_reopenDBLocal(sdb_p, &sqlDB);
	if (sqlerr != SQLITE_OK) {
	    goto loser;
	}
	sqlerr = sqlite3_prepare_v2(sqlDB, GET_PW_CMD, -1, &stmt, NULL);
    }
    if (sqlerr != SQLITE_OK) goto loser;
    sqlerr = sqlite3_bind_text(stmt, 1, id, PORT_Strlen(id), SQLITE_STATIC);
    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
	if (sqlerr == SQLITE_ROW) {
	    const char *blobData;
	    unsigned int len = item1->len;
	    item1->len = sqlite3_column_bytes(stmt, 1);
	    if (item1->len > len) {
		error = CKR_BUFFER_TOO_SMALL;
		continue;
	    }
	    blobData = sqlite3_column_blob(stmt, 1);
	    PORT_Memcpy(item1->data,blobData, item1->len);
	    if (item2) {
		len = item2->len;
		item2->len = sqlite3_column_bytes(stmt, 2);
		if (item2->len > len) {
		    error = CKR_BUFFER_TOO_SMALL;
		    continue;
		}
		blobData = sqlite3_column_blob(stmt, 2);
		PORT_Memcpy(item2->data,blobData, item2->len);
	    }
	    found = 1;
	}
    } while (!sdb_done(sqlerr,&retry));

loser:
    
    if (error == CKR_OK) {
	error = sdb_mapSQLError(sdb_p->type, sqlerr);
	if (!found && error == CKR_OK) {
	    error = CKR_OBJECT_HANDLE_INVALID;
	}
    }

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }
    UNLOCK_SQLITE()  

    return error;
}

static const char PW_CREATE_TABLE_CMD[] =
 "CREATE TABLE metaData (id PRIMARY KEY UNIQUE ON CONFLICT REPLACE, item1, item2);";
static const char PW_CREATE_CMD[] =
 "INSERT INTO metaData (id,item1,item2) VALUES($ID,$ITEM1,$ITEM2);";
static const char MD_CREATE_CMD[]  =
 "INSERT INTO metaData (id,item1) VALUES($ID,$ITEM1);";
CK_RV
sdb_PutMetaData(SDB *sdb, const char *id, const SECItem *item1, 
					   const SECItem *item2)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = sdb_p->sqlXactDB;
    sqlite3_stmt *stmt = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    int retry = 0;
    const char *cmd = PW_CREATE_CMD;

    if ((sdb->sdb_flags & SDB_RDONLY) != 0) {
	return CKR_TOKEN_WRITE_PROTECTED;
    }

    LOCK_SQLITE()  
    error = sdb_openDBLocal(sdb_p, &sqlDB, NULL);
    if (error != CKR_OK) {
	goto loser;
    }

    if (!tableExists(sqlDB, "metaData")) {
    	sqlerr = sqlite3_exec(sqlDB, PW_CREATE_TABLE_CMD, NULL, 0, NULL);
        if (sqlerr != SQLITE_OK) goto loser;
    }
    if (item2 == NULL) {
	cmd = MD_CREATE_CMD;
    }
    sqlerr = sqlite3_prepare_v2(sqlDB, cmd, -1, &stmt, NULL);
    if (sqlerr != SQLITE_OK) goto loser;
    sqlerr = sqlite3_bind_text(stmt, 1, id, PORT_Strlen(id), SQLITE_STATIC);
    if (sqlerr != SQLITE_OK) goto loser;
    sqlerr = sqlite3_bind_blob(stmt, 2, item1->data, item1->len, SQLITE_STATIC);
    if (sqlerr != SQLITE_OK) goto loser;
    if (item2) {
    	sqlerr = sqlite3_bind_blob(stmt, 3, item2->data, 
				   item2->len, SQLITE_STATIC);
        if (sqlerr != SQLITE_OK) goto loser;
    }

    do {
	sqlerr = sqlite3_step(stmt);
	if (sqlerr == SQLITE_BUSY) {
	    PR_Sleep(SDB_BUSY_RETRY_TIME);
	}
    } while (!sdb_done(sqlerr,&retry));

loser:
    
    if (error == CKR_OK) {
	error = sdb_mapSQLError(sdb_p->type, sqlerr);
    }

    if (stmt) {
	sqlite3_reset(stmt);
	sqlite3_finalize(stmt);
    }

    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }
    UNLOCK_SQLITE()  

    return error;
}

static const char RESET_CMD[] = "DROP TABLE IF EXISTS %s;";
CK_RV
sdb_Reset(SDB *sdb)
{
    SDBPrivate *sdb_p = sdb->private;
    sqlite3  *sqlDB = NULL;
    char *newStr;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;

    
    if (sdb_p->type != SDB_KEY) {
	return CKR_OBJECT_HANDLE_INVALID;
    }

    LOCK_SQLITE()  
    error = sdb_openDBLocal(sdb_p, &sqlDB, NULL);
    if (error != CKR_OK) {
	goto loser;
    }

    
    newStr =  sqlite3_mprintf(RESET_CMD, sdb_p->table);
    if (newStr == NULL) {
	error = CKR_HOST_MEMORY;
	goto loser;
    }
    sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
    sqlite3_free(newStr);

    if (sqlerr != SQLITE_OK) goto loser;

    
    sqlerr = sqlite3_exec(sqlDB, "DROP TABLE IF EXISTS metaData;", 
                          NULL, 0, NULL);

loser:
    
    if (error == CKR_OK) {
	error = sdb_mapSQLError(sdb_p->type, sqlerr);
    }

    if (sqlDB) {
	sdb_closeDBLocal(sdb_p, sqlDB) ;
    }

    UNLOCK_SQLITE()  
    return error;
}


CK_RV 
sdb_Close(SDB *sdb) 
{
    SDBPrivate *sdb_p = sdb->private;
    int sqlerr = SQLITE_OK;
    sdbDataType type = sdb_p->type;

    sqlerr = sqlite3_close(sdb_p->sqlReadDB);
    PORT_Free(sdb_p->sqlDBName);
    if (sdb_p->cacheTable) {
	sqlite3_free(sdb_p->cacheTable);
    }
    if (sdb_p->dbMon) {
	PR_DestroyMonitor(sdb_p->dbMon);
    }
    free(sdb_p);
    free(sdb);
    return sdb_mapSQLError(type, sqlerr);
}






static const char CHECK_TABLE_CMD[] = "SELECT ALL * FROM %s LIMIT 0;";

static int tableExists(sqlite3 *sqlDB, const char *tableName)
{
    char * cmd = sqlite3_mprintf(CHECK_TABLE_CMD, tableName);
    int sqlerr = SQLITE_OK;

    if (cmd == NULL) {
	return 0;
    }

    sqlerr = sqlite3_exec(sqlDB, cmd, NULL, 0, 0);
    sqlite3_free(cmd);

    return (sqlerr == SQLITE_OK) ? 1 : 0;
}

void sdb_SetForkState(PRBool forked)
{
    



}




static const char INIT_CMD[] =
 "CREATE TABLE %s (id PRIMARY KEY UNIQUE ON CONFLICT ABORT%s)";
static const char ALTER_CMD[] = 
 "ALTER TABLE %s ADD COLUMN a%x";

CK_RV 
sdb_init(char *dbname, char *table, sdbDataType type, int *inUpdate,
	 int *newInit, int flags, PRUint32 accessOps, SDB **pSdb)
{
    int i;
    char *initStr = NULL;
    char *newStr;
    int inTransaction = 0;
    SDB *sdb = NULL;
    SDBPrivate *sdb_p = NULL;
    sqlite3 *sqlDB = NULL;
    int sqlerr = SQLITE_OK;
    CK_RV error = CKR_OK;
    char *cacheTable = NULL;
    PRIntervalTime now = 0;
    char *env;
    PRBool enableCache = PR_FALSE;
    PRBool create;

    *pSdb = NULL;
    *inUpdate = 0;

    



    LOCK_SQLITE();
    create = (PR_Access(dbname, PR_ACCESS_EXISTS) != PR_SUCCESS);
    if ((flags == SDB_RDONLY) && create) {
	error = sdb_mapSQLError(type, SQLITE_CANTOPEN);
	goto loser;
    }
    sqlerr = sdb_openDB(dbname, &sqlDB, flags);
    if (sqlerr != SQLITE_OK) {
	error = sdb_mapSQLError(type, sqlerr); 
	goto loser;
    }
    

    if (create) {
	
#ifndef WINCE
	chmod (dbname, 0600);
#endif
    }

    if (flags != SDB_RDONLY) {
	sqlerr = sqlite3_exec(sqlDB, BEGIN_CMD, NULL, 0, NULL);
	if (sqlerr != SQLITE_OK) {
	    error = sdb_mapSQLError(type, sqlerr);
	    goto loser;
	}
	inTransaction = 1;
    }
    if (!tableExists(sqlDB,table)) {
	*newInit = 1;
	if (flags != SDB_CREATE) {
	    error = sdb_mapSQLError(type, SQLITE_CANTOPEN);
	    goto loser;
	}
	initStr = sqlite3_mprintf("");
	for (i=0; initStr && i < known_attributes_size; i++) {
	    newStr = sqlite3_mprintf("%s, a%x",initStr, known_attributes[i]);
	    sqlite3_free(initStr);
	    initStr = newStr;
	}
	if (initStr == NULL) {
	    error = CKR_HOST_MEMORY;
	    goto loser;
	}

	newStr = sqlite3_mprintf(INIT_CMD, table, initStr);
	sqlite3_free(initStr);
	if (newStr == NULL) {
            error = CKR_HOST_MEMORY;
	    goto loser;
	}
	sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
	sqlite3_free(newStr);
	if (sqlerr != SQLITE_OK) {
            error = sdb_mapSQLError(type, sqlerr); 
	    goto loser;
	}

	newStr = sqlite3_mprintf(CREATE_ISSUER_INDEX_CMD, table);
	if (newStr == NULL) {
            error = CKR_HOST_MEMORY;
	    goto loser;
	}
	sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
	sqlite3_free(newStr);
	if (sqlerr != SQLITE_OK) {
            error = sdb_mapSQLError(type, sqlerr); 
	    goto loser;
	}

	newStr = sqlite3_mprintf(CREATE_SUBJECT_INDEX_CMD, table);
	if (newStr == NULL) {
            error = CKR_HOST_MEMORY;
	    goto loser;
	}
	sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
	sqlite3_free(newStr);
	if (sqlerr != SQLITE_OK) {
            error = sdb_mapSQLError(type, sqlerr); 
	    goto loser;
	}

	newStr = sqlite3_mprintf(CREATE_LABEL_INDEX_CMD, table);
	if (newStr == NULL) {
            error = CKR_HOST_MEMORY;
	    goto loser;
	}
	sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
	sqlite3_free(newStr);
	if (sqlerr != SQLITE_OK) {
            error = sdb_mapSQLError(type, sqlerr); 
	    goto loser;
	}

	newStr = sqlite3_mprintf(CREATE_ID_INDEX_CMD, table);
	if (newStr == NULL) {
            error = CKR_HOST_MEMORY;
	    goto loser;
	}
	sqlerr = sqlite3_exec(sqlDB, newStr, NULL, 0, NULL);
	sqlite3_free(newStr);
	if (sqlerr != SQLITE_OK) {
            error = sdb_mapSQLError(type, sqlerr); 
	    goto loser;
	}
    }
    









    if (type == SDB_KEY && !tableExists(sqlDB, "metaData")) {
	*newInit = 1;
    }
    
    





     



















     env = PR_GetEnv("NSS_SDB_USE_CACHE");

     if (env && PORT_Strcasecmp(env,"no") == 0) {
	enableCache = PR_FALSE;
     } else if (env && PORT_Strcasecmp(env,"yes") == 0) {
	enableCache = PR_TRUE;
     } else {
	char *tempDir = NULL;
	PRUint32 tempOps = 0;
	




	tempDir = sdb_getTempDir(sqlDB);
	if (tempDir) {
	    tempOps = sdb_measureAccess(tempDir);
	    PORT_Free(tempDir);

	    

	    enableCache = (PRBool)(tempOps > accessOps * 10);
	}
    }

    if (enableCache) {
	
	sqlite3_exec(sqlDB, "PRAGMA temp_store=MEMORY", NULL, 0, NULL);
	


	cacheTable = sqlite3_mprintf("%sCache",table);
	if (cacheTable == NULL) {
	    error = CKR_HOST_MEMORY;
	    goto loser;
	}
	
	error = sdb_buildCache(sqlDB, type, cacheTable, table);
	if (error != CKR_OK) {
	    goto loser;
	}
	
	now = PR_IntervalNow();
    }

    sdb = (SDB *) malloc(sizeof(SDB));
    sdb_p = (SDBPrivate *) malloc(sizeof(SDBPrivate));

    
    sdb_p->sqlDBName = PORT_Strdup(dbname);
    sdb_p->type = type;
    sdb_p->table = table;
    sdb_p->cacheTable = cacheTable;
    sdb_p->lastUpdateTime = now;
    

    sdb_p->updateInterval = PR_SecondsToInterval(10); 
    sdb_p->dbMon = PR_NewMonitor();
    
    sdb_p->sqlXactDB = NULL;
    sdb_p->sqlXactThread = NULL;
    sdb->private = sdb_p;
    sdb->sdb_type = SDB_SQL;
    sdb->sdb_flags = flags | SDB_HAS_META;
    sdb->sdb_FindObjectsInit = sdb_FindObjectsInit;
    sdb->sdb_FindObjects = sdb_FindObjects;
    sdb->sdb_FindObjectsFinal = sdb_FindObjectsFinal;
    sdb->sdb_GetAttributeValue = sdb_GetAttributeValue;
    sdb->sdb_SetAttributeValue = sdb_SetAttributeValue;
    sdb->sdb_CreateObject = sdb_CreateObject;
    sdb->sdb_DestroyObject = sdb_DestroyObject;
    sdb->sdb_GetMetaData = sdb_GetMetaData;
    sdb->sdb_PutMetaData = sdb_PutMetaData;
    sdb->sdb_Begin = sdb_Begin;
    sdb->sdb_Commit = sdb_Commit;
    sdb->sdb_Abort = sdb_Abort;
    sdb->sdb_Close = sdb_Close;
    sdb->sdb_SetForkState = sdb_SetForkState;

    if (inTransaction) {
	sqlerr = sqlite3_exec(sqlDB, COMMIT_CMD, NULL, 0, NULL);
	if (sqlerr != SQLITE_OK) {
	    error = sdb_mapSQLError(sdb_p->type, sqlerr);
	    goto loser;
	}
	inTransaction = 0;
    }

    sdb_p->sqlReadDB = sqlDB;

    *pSdb = sdb;
    UNLOCK_SQLITE();
    return CKR_OK;

loser:
    
    if (inTransaction) {
	sqlite3_exec(sqlDB, ROLLBACK_CMD, NULL, 0, NULL);
    }
    if (sdb) {
	free(sdb);
    }
    if (sdb_p) {
	free(sdb_p);
    }
    if (sqlDB) {
	sqlite3_close(sqlDB);
    }
    UNLOCK_SQLITE();
    return error;

}



CK_RV
s_open(const char *directory, const char *certPrefix, const char *keyPrefix,
	int cert_version, int key_version, int flags, 
	SDB **certdb, SDB **keydb, int *newInit)
{
    char *cert = sdb_BuildFileName(directory, certPrefix,
				   "cert", cert_version, flags);
    char *key = sdb_BuildFileName(directory, keyPrefix,
				   "key", key_version, flags);
    CK_RV error = CKR_OK;
    int inUpdate;
    PRUint32 accessOps;

    if (certdb) 
	*certdb = NULL;
    if (keydb) 
	*keydb = NULL;
    *newInit = 0;

#ifdef SQLITE_UNSAFE_THREADS
    if (sqlite_lock == NULL) {
	sqlite_lock = PR_NewLock();
	if (sqlite_lock == NULL) {
	    error = CKR_HOST_MEMORY;
	    goto loser;
	}
    }
#endif

    

    accessOps = sdb_measureAccess(directory);

    


    if (certdb) {
	
	error = sdb_init(cert, "nssPublic", SDB_CERT, &inUpdate,
			 newInit, flags, accessOps, certdb);
	if (error != CKR_OK) {
	    goto loser;
	}
    }

    







    if (keydb) {
	
	error = sdb_init(key, "nssPrivate", SDB_KEY, &inUpdate, 
			newInit, flags, accessOps, keydb);
	if (error != CKR_OK) {
	    goto loser;
	} 
    }


loser:
    if (cert) {
	sqlite3_free(cert);
    }
    if (key) {
	sqlite3_free(key);
    }

    if (error != CKR_OK) {
	

	if (keydb && *keydb) {
	    sdb_Close(*keydb);
	}
	if (certdb && *certdb) {
	    sdb_Close(*certdb);
	}
    }

    return error;
}

CK_RV
s_shutdown()
{
#ifdef SQLITE_UNSAFE_THREADS
    if (sqlite_lock) {
	PR_DestroyLock(sqlite_lock);
	sqlite_lock = NULL;
    }
#endif
    return CKR_OK;
}
