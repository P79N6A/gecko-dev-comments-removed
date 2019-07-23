







































#include "sftkdb.h"
#include "sftkdbti.h"
#include "sdb.h"
#include "prsystem.h"
#include "prprf.h"
#include "prenv.h"
#include "lgglue.h"
#include "secerr.h"
#include "softoken.h"

static LGOpenFunc legacy_glue_open = NULL;
static LGReadSecmodFunc legacy_glue_readSecmod = NULL;
static LGReleaseSecmodFunc legacy_glue_releaseSecmod = NULL;
static LGDeleteSecmodFunc legacy_glue_deleteSecmod = NULL;
static LGAddSecmodFunc legacy_glue_addSecmod = NULL;
static LGShutdownFunc legacy_glue_shutdown = NULL;






#ifdef XP_UNIX
#include <unistd.h>
#define LG_MAX_LINKS 20
static char *
sftkdb_resolvePath(const char *orig)
{
    int count = 0;
    int len =0;
    int ret = -1;
    char *resolved = NULL;
    char *source = NULL;

    len = 1025; 
    if (strlen(orig)+1 > len) {
	
	return NULL;
    }
    resolved = PORT_Alloc(len);
    if (!resolved) {
	return NULL;
    }
    source = PORT_Alloc(len);
    if (!source) {
	goto loser;
    }
    PORT_Strcpy(source, orig);
    
    while ( count++ < LG_MAX_LINKS) {
	char *tmp;
	
	
	ret = readlink(source, resolved, len-1);
	if (ret  < 0) {
	    break;
 	}
	resolved[ret] = 0;
	tmp = source; source = resolved; resolved = tmp;
    }
    if (count > 1) {
	ret = 0;
    }
loser:
    if (resolved) {
	PORT_Free(resolved);
    }
    if (ret < 0) {
	if (source) {
	    PORT_Free(source);
	    source = NULL;
	}
    }
    return source;
}

#endif

static PRLibrary *
sftkdb_LoadFromPath(const char *path, const char *libname)
{
    char *c;
    int pathLen, nameLen, fullPathLen;
    char *fullPathName = NULL;
    PRLibSpec libSpec;
    PRLibrary *lib = NULL;


     
    c = strrchr(path, PR_GetDirectorySeparator());
    if (!c) {
	return NULL; 
    }
    pathLen = (c-path)+1;
    nameLen = strlen(libname);
    fullPathLen = pathLen + nameLen +1;
    fullPathName = (char *)PORT_Alloc(fullPathLen);
    if (fullPathName == NULL) {
	return NULL; 
    }
    PORT_Memcpy(fullPathName, path, pathLen);
    PORT_Memcpy(fullPathName+pathLen, libname, nameLen);
    fullPathName[fullPathLen-1] = 0;

    libSpec.type = PR_LibSpec_Pathname;
    libSpec.value.pathname = fullPathName;
    lib = PR_LoadLibraryWithFlags(libSpec, PR_LD_NOW | PR_LD_LOCAL);
    PORT_Free(fullPathName);
    return lib;
}


static PRLibrary *
sftkdb_LoadLibrary(const char *libname)
{
    PRLibrary *lib = NULL;
    PRFuncPtr fn_addr;
    char *parentLibPath = NULL;

    fn_addr  = (PRFuncPtr) &sftkdb_LoadLibrary;
    parentLibPath = PR_GetLibraryFilePathname(SOFTOKEN_LIB_NAME, fn_addr);

    if (!parentLibPath) {
	goto done;
    }

    lib = sftkdb_LoadFromPath(parentLibPath, libname);
#ifdef XP_UNIX
    
    if (!lib) {
	char *trueParentLibPath = sftkdb_resolvePath(parentLibPath);
	if (!trueParentLibPath) {
	    goto done;
	}
    	lib = sftkdb_LoadFromPath(trueParentLibPath, libname);
	PORT_Free(trueParentLibPath);
    }
#endif

done:
    if (parentLibPath) {
	PORT_Free(parentLibPath);
    }

    
    if (!lib) {
	PRLibSpec libSpec;
	libSpec.type = PR_LibSpec_Pathname;
	libSpec.value.pathname = libname;
	lib = PR_LoadLibraryWithFlags(libSpec, PR_LD_NOW | PR_LD_LOCAL);
    }

    return lib;
}





static SECStatus
sftkdb_encrypt_stub(PRArenaPool *arena, SDB *sdb, SECItem *plainText,
		    SECItem **cipherText)
{
    SFTKDBHandle *handle = sdb->app_private;
    SECStatus rv;

    if (handle == NULL) {
	return SECFailure;
    }

    
    if (handle->type != SFTK_KEYDB_TYPE) {
	handle = handle->peerDB;
    }

    
    if (handle == NULL || handle->passwordLock == NULL) {
	return SECFailure;
    }

    PZ_Lock(handle->passwordLock);
    if (handle->passwordKey.data == NULL) {
	PZ_Unlock(handle->passwordLock);
	
	return SECFailure;
    }

    rv = sftkdb_EncryptAttribute(arena, 
	handle->newKey?handle->newKey:&handle->passwordKey, 
	plainText, cipherText);
    PZ_Unlock(handle->passwordLock);

    return rv;
}





static SECStatus
sftkdb_decrypt_stub(SDB *sdb, SECItem *cipherText, SECItem **plainText) 
{
    SFTKDBHandle *handle = sdb->app_private;
    SECStatus rv;
    SECItem *oldKey = NULL;

    if (handle == NULL) {
	return SECFailure;
    }

    
    oldKey = handle->oldKey;
    if (handle->type != SFTK_KEYDB_TYPE) {
	handle = handle->peerDB;
    }

    
    if (handle == NULL || handle->passwordLock == NULL) {
	return SECFailure;
    }

    PZ_Lock(handle->passwordLock);
    if (handle->passwordKey.data == NULL) {
	PZ_Unlock(handle->passwordLock);
	
	return SECFailure;
    }
    rv = sftkdb_DecryptAttribute( oldKey ? oldKey : &handle->passwordKey,
		cipherText, plainText);
    PZ_Unlock(handle->passwordLock);

    return rv;
}

static const char *LEGACY_LIB_NAME = 
	SHLIB_PREFIX"nssdbm"SHLIB_VERSION"."SHLIB_SUFFIX;




static PRBool legacy_glue_libCheckFailed;    
static PRBool legacy_glue_libCheckSucceeded; 
static PRLibrary *legacy_glue_lib = NULL;
static SECStatus 
sftkdbLoad_Legacy(PRBool isFIPS)
{
    PRLibrary *lib = NULL;
    LGSetCryptFunc setCryptFunction = NULL;

    if (legacy_glue_lib) {
	


	if (isFIPS && !legacy_glue_libCheckSucceeded) {
	    if (legacy_glue_libCheckFailed || 
		!BLAPI_SHVerify(LEGACY_LIB_NAME,(PRFuncPtr)legacy_glue_open)) {
    	    	legacy_glue_libCheckFailed = PR_TRUE;
		

		return SECFailure;
	    }
    	    legacy_glue_libCheckSucceeded = PR_TRUE;
	} 
	return SECSuccess;
    }

    lib = sftkdb_LoadLibrary(LEGACY_LIB_NAME);
    if (lib == NULL) {
	return SECFailure;
    }
    
    legacy_glue_open = (LGOpenFunc)PR_FindFunctionSymbol(lib, "legacy_Open");
    legacy_glue_readSecmod = (LGReadSecmodFunc) PR_FindFunctionSymbol(lib,
						 "legacy_ReadSecmodDB");
    legacy_glue_releaseSecmod = (LGReleaseSecmodFunc) PR_FindFunctionSymbol(lib,
					 	 "legacy_ReleaseSecmodDBData");
    legacy_glue_deleteSecmod = (LGDeleteSecmodFunc) PR_FindFunctionSymbol(lib,
						 "legacy_DeleteSecmodDB");
    legacy_glue_addSecmod = (LGAddSecmodFunc)PR_FindFunctionSymbol(lib, 
						 "legacy_AddSecmodDB");
    legacy_glue_shutdown = (LGShutdownFunc) PR_FindFunctionSymbol(lib, 
						"legacy_Shutdown");
    setCryptFunction = (LGSetCryptFunc) PR_FindFunctionSymbol(lib, 
						"legacy_SetCryptFunctions");

    if (!legacy_glue_open || !legacy_glue_readSecmod || 
	    !legacy_glue_releaseSecmod || !legacy_glue_deleteSecmod || 
	    !legacy_glue_addSecmod || !setCryptFunction) {
	PR_UnloadLibrary(lib);
	return SECFailure;
    }

    
    if (isFIPS) {
	if (!BLAPI_SHVerify(LEGACY_LIB_NAME,(PRFuncPtr)legacy_glue_open)) {
	    PR_UnloadLibrary(lib);
	    return SECFailure;
	}
    	legacy_glue_libCheckSucceeded = PR_TRUE;
    } 

    setCryptFunction(sftkdb_encrypt_stub,sftkdb_decrypt_stub);
    legacy_glue_lib = lib;
    return SECSuccess;
}

CK_RV
sftkdbCall_open(const char *dir, const char *certPrefix, const char *keyPrefix, 
		int certVersion, int keyVersion, int flags, PRBool isFIPS,
		SDB **certDB, SDB **keyDB)
{
    SECStatus rv;

    rv = sftkdbLoad_Legacy(isFIPS);
    if (rv != SECSuccess) {
	return CKR_GENERAL_ERROR;
    }
    if (!legacy_glue_open) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return (*legacy_glue_open)(dir, certPrefix, keyPrefix, 
				certVersion, keyVersion,
				flags, certDB, keyDB);
}

char **
sftkdbCall_ReadSecmodDB(const char *appName, const char *filename, 
			const char *dbname, char *params, PRBool rw)
{
    SECStatus rv;

    rv = sftkdbLoad_Legacy(PR_FALSE);
    if (rv != SECSuccess) {
	return NULL;
    }
    if (!legacy_glue_readSecmod) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return NULL;
    }
    return (*legacy_glue_readSecmod)(appName, filename, dbname, params, rw);
}

SECStatus
sftkdbCall_ReleaseSecmodDBData(const char *appName, 
			const char *filename, const char *dbname, 
			char **moduleSpecList, PRBool rw)
{
    SECStatus rv;

    rv = sftkdbLoad_Legacy(PR_FALSE);
    if (rv != SECSuccess) {
	return rv;
    }
    if (!legacy_glue_releaseSecmod) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return (*legacy_glue_releaseSecmod)(appName, filename, dbname, 
					  moduleSpecList, rw);
}

SECStatus
sftkdbCall_DeleteSecmodDB(const char *appName, 
		      const char *filename, const char *dbname, 
		      char *args, PRBool rw)
{
    SECStatus rv;

    rv = sftkdbLoad_Legacy(PR_FALSE);
    if (rv != SECSuccess) {
	return rv;
    }
    if (!legacy_glue_deleteSecmod) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return (*legacy_glue_deleteSecmod)(appName, filename, dbname, args, rw);
}

SECStatus
sftkdbCall_AddSecmodDB(const char *appName, 
		   const char *filename, const char *dbname, 
		   char *module, PRBool rw)
{
    SECStatus rv;

    rv = sftkdbLoad_Legacy(PR_FALSE);
    if (rv != SECSuccess) {
	return rv;
    }
    if (!legacy_glue_addSecmod) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return (*legacy_glue_addSecmod)(appName, filename, dbname, module, rw);
}

CK_RV
sftkdbCall_Shutdown(void)
{
    CK_RV crv = CKR_OK;
    char *disableUnload = NULL;
    if (!legacy_glue_lib) {
	return CKR_OK;
    }
    if (legacy_glue_shutdown) {
#ifdef NO_FORK_CHECK
	PRBool parentForkedAfterC_Initialize = PR_FALSE;
#endif
	crv = (*legacy_glue_shutdown)(parentForkedAfterC_Initialize);
    }
    disableUnload = PR_GetEnv("NSS_DISABLE_UNLOAD");
    if (!disableUnload) {
        PR_UnloadLibrary(legacy_glue_lib);
    }
    legacy_glue_lib = NULL;
    legacy_glue_open = NULL;
    legacy_glue_readSecmod = NULL;
    legacy_glue_releaseSecmod = NULL;
    legacy_glue_deleteSecmod = NULL;
    legacy_glue_addSecmod = NULL;
    legacy_glue_libCheckFailed    = PR_FALSE;
    legacy_glue_libCheckSucceeded = PR_FALSE;
    return crv;
}
    

