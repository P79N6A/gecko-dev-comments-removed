







































#include <ctype.h>
#include <string.h>
#include "seccomon.h"
#include "prerror.h"
#include "prinit.h"
#include "prprf.h"
#include "prmem.h"
#include "prtypes.h"
#include "cert.h"
#include "key.h"
#include "secmod.h"
#include "secoid.h"
#include "nss.h"
#include "pk11func.h"
#include "secerr.h"
#include "nssbase.h"
#include "nssutil.h"
#include "pkixt.h"
#include "pkix.h"
#include "pkix_tools.h"

#include "pki3hack.h"
#include "certi.h"
#include "secmodi.h"
#include "ocspti.h"
#include "ocspi.h"








#ifdef WIN32_NSS3_DLL_COMPAT
#include <io.h>


char *
nss_mktemp(char *path)
{
    return _mktemp(path);
}
#endif

#define NSS_MAX_FLAG_SIZE  sizeof("readOnly")+sizeof("noCertDB")+ \
	sizeof("noModDB")+sizeof("forceOpen")+sizeof("passwordRequired")+ \
	sizeof ("optimizeSpace")
#define NSS_DEFAULT_MOD_NAME "NSS Internal Module"

static char *
nss_makeFlags(PRBool readOnly, PRBool noCertDB, 
				PRBool noModDB, PRBool forceOpen, 
				PRBool passwordRequired, PRBool optimizeSpace) 
{
    char *flags = (char *)PORT_Alloc(NSS_MAX_FLAG_SIZE);
    PRBool first = PR_TRUE;

    PORT_Memset(flags,0,NSS_MAX_FLAG_SIZE);
    if (readOnly) {
        PORT_Strcat(flags,"readOnly");
        first = PR_FALSE;
    }
    if (noCertDB) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"noCertDB");
        first = PR_FALSE;
    }
    if (noModDB) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"noModDB");
        first = PR_FALSE;
    }
    if (forceOpen) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"forceOpen");
        first = PR_FALSE;
    }
    if (passwordRequired) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"passwordRequired");
        first = PR_FALSE;
    }
    if (optimizeSpace) {
        if (!first) PORT_Strcat(flags,",");
        PORT_Strcat(flags,"optimizeSpace");
        first = PR_FALSE;
    }
    return flags;
}





char *
nss_MkConfigString(const char *man, const char *libdesc, const char *tokdesc,
	const char *ptokdesc, const char *slotdesc, const char *pslotdesc, 
	const char *fslotdesc, const char *fpslotdesc, int minPwd)
{
    char *strings = NULL;
    char *newStrings;

    
    strings = PR_smprintf("");
    if (strings == NULL) return NULL;

    if (man) {
        newStrings = PR_smprintf("%s manufacturerID='%s'",strings,man);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    if (libdesc) {
        newStrings = PR_smprintf("%s libraryDescription='%s'",strings,libdesc);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    if (tokdesc) {
        newStrings = PR_smprintf("%s cryptoTokenDescription='%s'",strings,
								tokdesc);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    if (ptokdesc) {
        newStrings = PR_smprintf("%s dbTokenDescription='%s'",strings,ptokdesc);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    if (slotdesc) {
        newStrings = PR_smprintf("%s cryptoSlotDescription='%s'",strings,
								slotdesc);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    if (pslotdesc) {
        newStrings = PR_smprintf("%s dbSlotDescription='%s'",strings,pslotdesc);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    if (fslotdesc) {
        newStrings = PR_smprintf("%s FIPSSlotDescription='%s'",
							strings,fslotdesc);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    if (fpslotdesc) {
        newStrings = PR_smprintf("%s FIPSTokenDescription='%s'",
							strings,fpslotdesc);
	PR_smprintf_free(strings);
	strings = newStrings;
    }
    if (strings == NULL) return NULL;

    newStrings = PR_smprintf("%s minPS=%d", strings, minPwd);
    PR_smprintf_free(strings);
    strings = newStrings;

    return(strings);
}





static char * pk11_config_strings = NULL;
static char * pk11_config_name = NULL;
static PRBool pk11_password_required = PR_FALSE;





void
PK11_ConfigurePKCS11(const char *man, const char *libdesc, const char *tokdesc,
	const char *ptokdesc, const char *slotdesc, const char *pslotdesc, 
	const char *fslotdesc, const char *fpslotdesc, int minPwd, 
	int pwRequired)
{
    char * strings;

    strings = nss_MkConfigString(man,libdesc,tokdesc,ptokdesc,slotdesc,
	pslotdesc,fslotdesc,fpslotdesc,minPwd);
    if (strings == NULL) {
	return;
    }

    if (libdesc) {
	if (pk11_config_name != NULL) {
	    PORT_Free(pk11_config_name);
	}
	pk11_config_name = PORT_Strdup(libdesc);
    }

    if (pk11_config_strings != NULL) {
	PR_smprintf_free(pk11_config_strings);
    }
    pk11_config_strings = strings;
    pk11_password_required = pwRequired;

    return;
}

void PK11_UnconfigurePKCS11(void)
{
    if (pk11_config_strings != NULL) {
	PR_smprintf_free(pk11_config_strings);
        pk11_config_strings = NULL;
    }
    if (pk11_config_name) {
        PORT_Free(pk11_config_name);
        pk11_config_name = NULL;
    }
}







static const char *dllname =
#if defined(XP_WIN32) || defined(XP_OS2)
	"nssckbi.dll";
#elif defined(HPUX) && !defined(__ia64)  /* HP-UX PA-RISC */
	"libnssckbi.sl";
#elif defined(DARWIN)
	"libnssckbi.dylib";
#elif defined(XP_UNIX) || defined(XP_BEOS)
	"libnssckbi.so";
#else
	#error "Uh! Oh! I don't know about this platform."
#endif


#define FILE_SEP '/'

static void nss_FindExternalRootPaths(const char *dbpath, 
                                      const char* secmodprefix,
                              char** retoldpath, char** retnewpath)
{
    char *path, *oldpath = NULL, *lastsep;
    int len, path_len, secmod_len, dll_len;

    path_len = PORT_Strlen(dbpath);
    secmod_len = secmodprefix ? PORT_Strlen(secmodprefix) : 0;
    dll_len = PORT_Strlen(dllname);
    len = path_len + secmod_len + dll_len + 2; 

    path = PORT_Alloc(len);
    if (path == NULL) return;

    
    PORT_Memcpy(path,dbpath,path_len);
    if (path[path_len-1] != FILE_SEP) {
        path[path_len++] = FILE_SEP;
    }
    PORT_Strcpy(&path[path_len],dllname);
    if (secmod_len > 0) {
        lastsep = PORT_Strrchr(secmodprefix, FILE_SEP);
        if (lastsep) {
            int secmoddir_len = lastsep-secmodprefix+1; 
            oldpath = PORT_Alloc(len);
            if (oldpath == NULL) {
                PORT_Free(path);
                return;
            }
            PORT_Memcpy(oldpath,path,path_len);
            PORT_Memcpy(&oldpath[path_len],secmodprefix,secmoddir_len);
            PORT_Strcpy(&oldpath[path_len+secmoddir_len],dllname);
        }
    }
    *retoldpath = oldpath;
    *retnewpath = path;
    return;
}

static void nss_FreeExternalRootPaths(char* oldpath, char* path)
{
    if (path) {
        PORT_Free(path);
    }
    if (oldpath) {
        PORT_Free(oldpath);
    }
}

static void
nss_FindExternalRoot(const char *dbpath, const char* secmodprefix)
{
	char *path = NULL;
        char *oldpath = NULL;
        PRBool hasrootcerts = PR_FALSE;

        




        nss_FindExternalRootPaths(dbpath, secmodprefix, &oldpath, &path);
        if (oldpath) {
            (void) SECMOD_AddNewModule("Root Certs",oldpath, 0, 0);
            hasrootcerts = SECMOD_HasRootCerts();
        }
        if (path && !hasrootcerts) {
	    (void) SECMOD_AddNewModule("Root Certs",path, 0, 0);
        }
        nss_FreeExternalRootPaths(oldpath, path);
	return;
}








static SECStatus
nss_InitModules(const char *configdir, const char *certPrefix, 
		const char *keyPrefix, const char *secmodName, 
		const char *updateDir, const char *updCertPrefix, 
		const char *updKeyPrefix, const char *updateID, 
		const char *updateName, char *configName, char *configStrings,
		PRBool pwRequired, PRBool readOnly, PRBool noCertDB,
		PRBool noModDB, PRBool forceOpen, PRBool optimizeSpace,
		PRBool isContextInit)
{
    SECStatus rv = SECFailure;
    char *moduleSpec = NULL;
    char *flags = NULL;
    char *lconfigdir = NULL;
    char *lcertPrefix = NULL;
    char *lkeyPrefix = NULL;
    char *lsecmodName = NULL;
    char *lupdateDir = NULL;
    char *lupdCertPrefix = NULL;
    char *lupdKeyPrefix = NULL;
    char *lupdateID = NULL;
    char *lupdateName = NULL;

    if (NSS_InitializePRErrorTable() != SECSuccess) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return rv;
    }

    flags = nss_makeFlags(readOnly,noCertDB,noModDB,forceOpen,
					pwRequired, optimizeSpace);
    if (flags == NULL) return rv;

    



    lconfigdir = secmod_DoubleEscape(configdir, '\'', '\"');
    if (lconfigdir == NULL) {
	goto loser;
    }
    lcertPrefix = secmod_DoubleEscape(certPrefix, '\'', '\"');
    if (lcertPrefix == NULL) {
	goto loser;
    }
    lkeyPrefix = secmod_DoubleEscape(keyPrefix, '\'', '\"');
    if (lkeyPrefix == NULL) {
	goto loser;
    }
    lsecmodName = secmod_DoubleEscape(secmodName, '\'', '\"');
    if (lsecmodName == NULL) {
	goto loser;
    }
    lupdateDir = secmod_DoubleEscape(updateDir, '\'', '\"');
    if (lupdateDir == NULL) {
	goto loser;
    }
    lupdCertPrefix = secmod_DoubleEscape(updCertPrefix, '\'', '\"');
    if (lupdCertPrefix == NULL) {
	goto loser;
    }
    lupdKeyPrefix = secmod_DoubleEscape(updKeyPrefix, '\'', '\"');
    if (lupdKeyPrefix == NULL) {
	goto loser;
    }
    lupdateID = secmod_DoubleEscape(updateID, '\'', '\"');
    if (lupdateID == NULL) {
	goto loser;
    }
    lupdateName = secmod_DoubleEscape(updateName, '\'', '\"');
    if (lupdateName == NULL) {
	goto loser;
    }

    moduleSpec = PR_smprintf(
     "name=\"%s\" parameters=\"configdir='%s' certPrefix='%s' keyPrefix='%s' "
     "secmod='%s' flags=%s updatedir='%s' updateCertPrefix='%s' "
     "updateKeyPrefix='%s' updateid='%s' updateTokenDescription='%s' %s\" "
     "NSS=\"flags=internal,moduleDB,moduleDBOnly,critical%s\"",
		configName ? configName : NSS_DEFAULT_MOD_NAME,
		lconfigdir,lcertPrefix,lkeyPrefix,lsecmodName,flags,
		lupdateDir, lupdCertPrefix, lupdKeyPrefix, lupdateID, 
		lupdateName, configStrings ? configStrings : "",
		isContextInit ? "" : ",defaultModDB,internalKeySlot");

loser:
    PORT_Free(flags);
    if (lconfigdir) PORT_Free(lconfigdir);
    if (lcertPrefix) PORT_Free(lcertPrefix);
    if (lkeyPrefix) PORT_Free(lkeyPrefix);
    if (lsecmodName) PORT_Free(lsecmodName);
    if (lupdateDir) PORT_Free(lupdateDir);
    if (lupdCertPrefix) PORT_Free(lupdCertPrefix);
    if (lupdKeyPrefix) PORT_Free(lupdKeyPrefix);
    if (lupdateID) PORT_Free(lupdateID);
    if (lupdateName) PORT_Free(lupdateName);

    if (moduleSpec) {
	SECMODModule *module = SECMOD_LoadModule(moduleSpec,NULL,PR_TRUE);
	PR_smprintf_free(moduleSpec);
	if (module) {
	    if (module->loaded) rv=SECSuccess;
	    SECMOD_DestroyModule(module);
	}
    }
    return rv;
}






































static PRBool          nssIsInitted = PR_FALSE;
static NSSInitContext *nssInitContextList = NULL;
static void*           plContext = NULL;

struct NSSInitContextStr {
    NSSInitContext *next;
    PRUint32 magic;
};

#define NSS_INIT_MAGIC 0x1413A91C
static SECStatus nss_InitShutdownList(void);

#ifdef DEBUG
static CERTCertificate dummyCert;
#endif


static PRCallOnceType nssInitOnce;
static PZLock *nssInitLock;
static PZCondVar *nssInitCondition;
static int nssIsInInit;

static PRStatus
nss_doLockInit(void)
{
    nssInitLock = PZ_NewLock(nssILockOther);
    if (nssInitLock == NULL) {
	return PR_FAILURE;
    }
    nssInitCondition = PZ_NewCondVar(nssInitLock);
    if (nssInitCondition == NULL) {
	return PR_FAILURE;
    }
    return PR_SUCCESS;
}


static SECStatus
nss_Init(const char *configdir, const char *certPrefix, const char *keyPrefix,
		 const char *secmodName, const char *updateDir, 
		 const char *updCertPrefix, const char *updKeyPrefix,
		 const char *updateID, const char *updateName,
		 NSSInitContext ** initContextPtr,
		 NSSInitParameters *initParams,
		 PRBool readOnly, PRBool noCertDB, 
		 PRBool noModDB, PRBool forceOpen, PRBool noRootInit,
		 PRBool optimizeSpace, PRBool noSingleThreadedModules,
		 PRBool allowAlreadyInitializedModules,
		 PRBool dontFinalizeModules)
{
    SECStatus rv = SECFailure;
    PKIX_UInt32 actualMinorVersion = 0;
    PKIX_Error *pkixError = NULL;
    PRBool isReallyInitted;
    char *configStrings = NULL;
    char *configName = NULL;
    PRBool passwordRequired = PR_FALSE;

    

    if (!initContextPtr && nssIsInitted) {
	return SECSuccess;
    }
  
    
 
    if (PR_CallOnce(&nssInitOnce, nss_doLockInit) != PR_SUCCESS) {
	return SECFailure;
    }

    



    PZ_Lock(nssInitLock);
    isReallyInitted = NSS_IsInitialized();
    if (!isReallyInitted) {
	while (!isReallyInitted && nssIsInInit) {
	    PZ_WaitCondVar(nssInitCondition,PR_INTERVAL_NO_TIMEOUT);
	    isReallyInitted = NSS_IsInitialized();
 	}
	

    }
    nssIsInInit++;
    PZ_Unlock(nssInitLock);

    



    if (!isReallyInitted) {
	
	PORT_Assert(sizeof(dummyCert.options) == sizeof(void *));

	if (SECSuccess != cert_InitLocks()) {
	    goto loser;
	}

	if (SECSuccess != InitCRLCache()) {
	    goto loser;
	}
    
	if (SECSuccess != OCSP_InitGlobal()) {
	    goto loser;
	}
    }

    if (noSingleThreadedModules || allowAlreadyInitializedModules ||
        dontFinalizeModules) {
        pk11_setGlobalOptions(noSingleThreadedModules,
                              allowAlreadyInitializedModules,
                              dontFinalizeModules);
    }

    if (initContextPtr) {
	*initContextPtr = PORT_ZNew(NSSInitContext);
	if (*initContextPtr == NULL) {
	    goto loser;
	}
	






        if (initParams) {
	    if (initParams->length < sizeof(NSSInitParameters)) {
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		goto loser;
	    }
	    configStrings = nss_MkConfigString(initParams->manufactureID,
		initParams->libraryDescription,
		initParams->cryptoTokenDescription,
		initParams->dbTokenDescription,
		initParams->cryptoSlotDescription,
		initParams->dbSlotDescription,
		initParams->FIPSSlotDescription,
		initParams->FIPSTokenDescription,
		initParams->minPWLen);
	    if (configStrings == NULL) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto loser;
	    }
	    configName = initParams->libraryDescription;
	    passwordRequired = initParams->passwordRequired;
	}
    } else {
	configStrings = pk11_config_strings;
	configName = pk11_config_name;
	passwordRequired = pk11_password_required;
    }

    
    rv = nss_InitModules(configdir, certPrefix, keyPrefix, secmodName, 
		updateDir, updCertPrefix, updKeyPrefix, updateID, 
		updateName, configName, configStrings, passwordRequired,
		readOnly, noCertDB, noModDB, forceOpen, optimizeSpace, 
		(initContextPtr != NULL));

    if (rv != SECSuccess) {
	goto loser;
    }


    
    if (!isReallyInitted) {
	if (SECOID_Init() != SECSuccess) {
	    goto loser;
	}
	if (STAN_LoadDefaultNSS3TrustDomain() != PR_SUCCESS) {
	    goto loser;
	}
	if (nss_InitShutdownList() != SECSuccess) {
	    goto loser;
	}
	CERT_SetDefaultCertDB((CERTCertDBHandle *)
				STAN_GetDefaultTrustDomain());
	if ((!noModDB) && (!noCertDB) && (!noRootInit)) {
	    if (!SECMOD_HasRootCerts()) {
		const char *dbpath = configdir;
		
		if (strncmp(dbpath, "sql:", 4) == 0) {
		    dbpath += 4;
		} else if(strncmp(dbpath, "dbm:", 4) == 0) {
		    dbpath += 4;
		} else if(strncmp(dbpath, "extern:", 7) == 0) {
		    dbpath += 7;
		} else if(strncmp(dbpath, "rdb:", 4) == 0) {
		    

		    dbpath = NULL;
		}
		if (dbpath) {
		    nss_FindExternalRoot(dbpath, secmodName);
		}
	    }
	}

	pk11sdr_Init();
	cert_CreateSubjectKeyIDHashTable();

	pkixError = PKIX_Initialize
	    (PKIX_FALSE, PKIX_MAJOR_VERSION, PKIX_MINOR_VERSION,
	    PKIX_MINOR_VERSION, &actualMinorVersion, &plContext);

	if (pkixError != NULL) {
	    goto loser;
	} else {
            char *ev = getenv("NSS_ENABLE_PKIX_VERIFY");
            if (ev && ev[0]) {
                CERT_SetUsePKIXForValidation(PR_TRUE);
            }
        }


    }

    




    PZ_Lock(nssInitLock);
    if (!initContextPtr) {
	nssIsInitted = PR_TRUE;
    } else {
	(*initContextPtr)->magic = NSS_INIT_MAGIC;
	(*initContextPtr)->next = nssInitContextList;
	nssInitContextList = (*initContextPtr);
    }
    nssIsInInit--;
    
    PZ_NotifyAllCondVar(nssInitCondition);
    PZ_Unlock(nssInitLock);

    return SECSuccess;

loser:
    if (initContextPtr && *initContextPtr) {
	PORT_Free(*initContextPtr);
	*initContextPtr = NULL;
	if (configStrings) {
	   PR_smprintf_free(configStrings);
	}
    }
    PZ_Lock(nssInitLock);
    nssIsInInit--;
    
    PZ_NotifyCondVar(nssInitCondition);
    PZ_Unlock(nssInitLock);
    return SECFailure;
}


SECStatus
NSS_Init(const char *configdir)
{
    return nss_Init(configdir, "", "", SECMOD_DB, "", "", "", "", "", NULL,
		NULL, PR_TRUE, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE, 
		PR_TRUE, PR_FALSE, PR_FALSE, PR_FALSE);
}

SECStatus
NSS_InitReadWrite(const char *configdir)
{
    return nss_Init(configdir, "", "", SECMOD_DB, "", "", "", "", "", NULL,
		NULL, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE, PR_FALSE, 
		PR_TRUE, PR_FALSE, PR_FALSE, PR_FALSE);
}

















































SECStatus
NSS_Initialize(const char *configdir, const char *certPrefix, 
	const char *keyPrefix, const char *secmodName, PRUint32 flags)
{
    return nss_Init(configdir, certPrefix, keyPrefix, secmodName,
	"", "", "", "", "", NULL, NULL,
	((flags & NSS_INIT_READONLY) == NSS_INIT_READONLY),
	((flags & NSS_INIT_NOCERTDB) == NSS_INIT_NOCERTDB),
	((flags & NSS_INIT_NOMODDB) == NSS_INIT_NOMODDB),
	((flags & NSS_INIT_FORCEOPEN) == NSS_INIT_FORCEOPEN),
	((flags & NSS_INIT_NOROOTINIT) == NSS_INIT_NOROOTINIT),
	((flags & NSS_INIT_OPTIMIZESPACE) == NSS_INIT_OPTIMIZESPACE),
        ((flags & NSS_INIT_PK11THREADSAFE) == NSS_INIT_PK11THREADSAFE),
        ((flags & NSS_INIT_PK11RELOAD) == NSS_INIT_PK11RELOAD),
        ((flags & NSS_INIT_NOPK11FINALIZE) == NSS_INIT_NOPK11FINALIZE));
}

NSSInitContext *
NSS_InitContext(const char *configdir, const char *certPrefix, 
	const char *keyPrefix, const char *secmodName, 
	NSSInitParameters *initParams, PRUint32 flags)
{
    SECStatus rv;
    NSSInitContext *context;

    rv = nss_Init(configdir, certPrefix, keyPrefix, secmodName,
	"", "", "", "", "", &context, initParams,
	((flags & NSS_INIT_READONLY) == NSS_INIT_READONLY),
	((flags & NSS_INIT_NOCERTDB) == NSS_INIT_NOCERTDB),
	((flags & NSS_INIT_NOMODDB) == NSS_INIT_NOMODDB),
	((flags & NSS_INIT_FORCEOPEN) == NSS_INIT_FORCEOPEN), PR_TRUE,
	((flags & NSS_INIT_OPTIMIZESPACE) == NSS_INIT_OPTIMIZESPACE),
        ((flags & NSS_INIT_PK11THREADSAFE) == NSS_INIT_PK11THREADSAFE),
        ((flags & NSS_INIT_PK11RELOAD) == NSS_INIT_PK11RELOAD),
        ((flags & NSS_INIT_NOPK11FINALIZE) == NSS_INIT_NOPK11FINALIZE));
    return (rv == SECSuccess) ? context : NULL;
}

SECStatus
NSS_InitWithMerge(const char *configdir, const char *certPrefix, 
	const char *keyPrefix, const char *secmodName, 
	const char *updateDir, const char *updCertPrefix,
	const char *updKeyPrefix, const char *updateID, 
	const char *updateName, PRUint32 flags)
{
    return nss_Init(configdir, certPrefix, keyPrefix, secmodName,
	updateDir, updCertPrefix, updKeyPrefix, updateID, updateName, 
	NULL, NULL,
	((flags & NSS_INIT_READONLY) == NSS_INIT_READONLY),
	((flags & NSS_INIT_NOCERTDB) == NSS_INIT_NOCERTDB),
	((flags & NSS_INIT_NOMODDB) == NSS_INIT_NOMODDB),
	((flags & NSS_INIT_FORCEOPEN) == NSS_INIT_FORCEOPEN),
	((flags & NSS_INIT_NOROOTINIT) == NSS_INIT_NOROOTINIT),
	((flags & NSS_INIT_OPTIMIZESPACE) == NSS_INIT_OPTIMIZESPACE),
        ((flags & NSS_INIT_PK11THREADSAFE) == NSS_INIT_PK11THREADSAFE),
        ((flags & NSS_INIT_PK11RELOAD) == NSS_INIT_PK11RELOAD),
        ((flags & NSS_INIT_NOPK11FINALIZE) == NSS_INIT_NOPK11FINALIZE));
}




SECStatus
NSS_NoDB_Init(const char * configdir)
{
      return nss_Init("","","","", "", "", "", "", "", NULL, NULL,
			PR_TRUE,PR_TRUE,PR_TRUE,PR_TRUE,PR_TRUE,PR_TRUE,
			PR_FALSE,PR_FALSE,PR_FALSE);
}


#define NSS_SHUTDOWN_STEP 10

struct NSSShutdownFuncPair {
    NSS_ShutdownFunc	func;
    void		*appData;
};

static struct NSSShutdownListStr {
    PZLock		*lock;
    int			allocatedFuncs;
    int			peakFuncs;
    struct NSSShutdownFuncPair	*funcs;
} nssShutdownList = { 0 };




static int 
nss_GetShutdownEntry(NSS_ShutdownFunc sFunc, void *appData)
{
    int count, i;
    count = nssShutdownList.peakFuncs;

    for (i=0; i < count; i++) {
	if ((nssShutdownList.funcs[i].func == sFunc) &&
	    (nssShutdownList.funcs[i].appData == appData)){
	    return i;
	}
    }
    return -1;
}
    



SECStatus
NSS_RegisterShutdown(NSS_ShutdownFunc sFunc, void *appData)
{
    int i;

    PZ_Lock(nssInitLock);
    if (!NSS_IsInitialized()) {
	PZ_Unlock(nssInitLock);
	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return SECFailure;
    }
    PZ_Unlock(nssInitLock);
    if (sFunc == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    PORT_Assert(nssShutdownList.lock);
    PZ_Lock(nssShutdownList.lock);

    
    i = nss_GetShutdownEntry(sFunc, appData);
    if (i >= 0) {
	PZ_Unlock(nssShutdownList.lock);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    
    i = nss_GetShutdownEntry(NULL, NULL);
    if (i >= 0) {
	nssShutdownList.funcs[i].func = sFunc;
	nssShutdownList.funcs[i].appData = appData;
	PZ_Unlock(nssShutdownList.lock);
	return SECSuccess;
    }
    if (nssShutdownList.allocatedFuncs == nssShutdownList.peakFuncs) {
	struct NSSShutdownFuncPair *funcs = 
		(struct NSSShutdownFuncPair *)PORT_Realloc
		(nssShutdownList.funcs, 
		(nssShutdownList.allocatedFuncs + NSS_SHUTDOWN_STEP) 
		*sizeof(struct NSSShutdownFuncPair));
	if (!funcs) {
	    PZ_Unlock(nssShutdownList.lock);
	    return SECFailure;
	}
	nssShutdownList.funcs = funcs;
	nssShutdownList.allocatedFuncs += NSS_SHUTDOWN_STEP;
    }
    nssShutdownList.funcs[nssShutdownList.peakFuncs].func = sFunc;
    nssShutdownList.funcs[nssShutdownList.peakFuncs].appData = appData;
    nssShutdownList.peakFuncs++;
    PZ_Unlock(nssShutdownList.lock);
    return SECSuccess;
}




SECStatus
NSS_UnregisterShutdown(NSS_ShutdownFunc sFunc, void *appData)
{
    int i;

    PZ_Lock(nssInitLock);
    if (!NSS_IsInitialized()) {
	PZ_Unlock(nssInitLock);
	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return SECFailure;
    }
    PZ_Unlock(nssInitLock);

    PORT_Assert(nssShutdownList.lock);
    PZ_Lock(nssShutdownList.lock);
    i = nss_GetShutdownEntry(sFunc, appData);
    if (i >= 0) {
	nssShutdownList.funcs[i].func = NULL;
	nssShutdownList.funcs[i].appData = NULL;
    }
    PZ_Unlock(nssShutdownList.lock);

    if (i < 0) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return SECSuccess;
}




static SECStatus
nss_InitShutdownList(void)
{
    if (nssShutdownList.lock != NULL) {
	return SECSuccess;
    }
    nssShutdownList.lock = PZ_NewLock(nssILockOther);
    if (nssShutdownList.lock == NULL) {
	return SECFailure;
    }
    nssShutdownList.funcs = PORT_ZNewArray(struct NSSShutdownFuncPair, 
				           NSS_SHUTDOWN_STEP);
    if (nssShutdownList.funcs == NULL) {
	PZ_DestroyLock(nssShutdownList.lock);
    	nssShutdownList.lock = NULL;
	return SECFailure;
    }
    nssShutdownList.allocatedFuncs = NSS_SHUTDOWN_STEP;
    nssShutdownList.peakFuncs = 0;

    return SECSuccess;
}

static SECStatus
nss_ShutdownShutdownList(void)
{
    SECStatus rv = SECSuccess;
    int i;

    
    for (i=0; i < nssShutdownList.peakFuncs; i++) {
	struct NSSShutdownFuncPair *funcPair = &nssShutdownList.funcs[i];
	if (funcPair->func) {
	    if ((*funcPair->func)(funcPair->appData,NULL) != SECSuccess) {
		rv = SECFailure;
	    }
	}
    }

    nssShutdownList.peakFuncs = 0;
    nssShutdownList.allocatedFuncs = 0;
    PORT_Free(nssShutdownList.funcs);
    nssShutdownList.funcs = NULL;
    if (nssShutdownList.lock) {
	PZ_DestroyLock(nssShutdownList.lock);
    }
    nssShutdownList.lock = NULL;
    return rv;
}


extern const NSSError NSS_ERROR_BUSY;

SECStatus
nss_Shutdown(void)
{
    SECStatus shutdownRV = SECSuccess;
    SECStatus rv;
    PRStatus status;
    NSSInitContext *temp;

    rv = nss_ShutdownShutdownList();
    if (rv != SECSuccess) {
	shutdownRV = SECFailure;
    }
    cert_DestroyLocks();
    ShutdownCRLCache();
    OCSP_ShutdownGlobal();
    PKIX_Shutdown(plContext);
    SECOID_Shutdown();
    status = STAN_Shutdown();
    cert_DestroySubjectKeyIDHashTable();
    pk11_SetInternalKeySlot(NULL);
    rv = SECMOD_Shutdown();
    if (rv != SECSuccess) {
	shutdownRV = SECFailure;
    }
    pk11sdr_Shutdown();
    






    nss_DestroyErrorStack();
    nssArena_Shutdown();
    if (status == PR_FAILURE) {
	if (NSS_GetError() == NSS_ERROR_BUSY) {
	    PORT_SetError(SEC_ERROR_BUSY);
	}
	shutdownRV = SECFailure;
    }
    nssIsInitted = PR_FALSE;
    temp = nssInitContextList;
    nssInitContextList = NULL;
    

    while (temp) {
	NSSInitContext *next = temp->next;
	temp->magic = 0;
	PORT_Free(temp);
	temp = next;
    }
    return shutdownRV;
}

SECStatus
NSS_Shutdown(void)
{
    SECStatus rv;
    PZ_Lock(nssInitLock);

    if (!nssIsInitted) {
	PZ_Unlock(nssInitLock);
	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return SECFailure;
    }

    

    while (nssIsInInit) {
	PZ_WaitCondVar(nssInitCondition,PR_INTERVAL_NO_TIMEOUT);
    }
    rv = nss_Shutdown();
    PZ_Unlock(nssInitLock);
    return rv;
}




PRBool
nss_RemoveList(NSSInitContext *context) {
    NSSInitContext *this = nssInitContextList;
    NSSInitContext **last = &nssInitContextList;

    while (this) {
	if (this == context) {
	    *last = this->next;
	    this->magic = 0;
	    PORT_Free(this);
	    return PR_TRUE;
	}
	last = &this->next;
	this=this->next;
    }
    return PR_FALSE;
}









SECStatus
NSS_ShutdownContext(NSSInitContext *context)
{
    SECStatus rv = SECSuccess;

    PZ_Lock(nssInitLock);
    

    while (nssIsInInit) {
	PZ_WaitCondVar(nssInitCondition,PR_INTERVAL_NO_TIMEOUT);
    }

    
    
    if (!context) {
	if (!nssIsInitted) {
	    PZ_Unlock(nssInitLock);
	    PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	    return SECFailure;
	}
	nssIsInitted = 0;
    } else if (! nss_RemoveList(context)) {
	PZ_Unlock(nssInitLock);
	
	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return SECFailure;
    }
    if ((nssIsInitted == 0) && (nssInitContextList == NULL)) {
	rv = nss_Shutdown();
    }

    



    PZ_Unlock(nssInitLock);

    return rv;
}

PRBool
NSS_IsInitialized(void)
{
    return (nssIsInitted) || (nssInitContextList != NULL);
}
	

extern const char __nss_base_rcsid[];
extern const char __nss_base_sccsid[];

PRBool
NSS_VersionCheck(const char *importedVersion)
{
    








    int vmajor = 0, vminor = 0, vpatch = 0, vbuild = 0;
    const char *ptr = importedVersion;
    volatile char c; 

    c = __nss_base_rcsid[0] + __nss_base_sccsid[0]; 

    while (isdigit(*ptr)) {
        vmajor = 10 * vmajor + *ptr - '0';
        ptr++;
    }
    if (*ptr == '.') {
        ptr++;
        while (isdigit(*ptr)) {
            vminor = 10 * vminor + *ptr - '0';
            ptr++;
        }
        if (*ptr == '.') {
            ptr++;
            while (isdigit(*ptr)) {
                vpatch = 10 * vpatch + *ptr - '0';
                ptr++;
            }
            if (*ptr == '.') {
                ptr++;
                while (isdigit(*ptr)) {
                    vbuild = 10 * vbuild + *ptr - '0';
                    ptr++;
                }
            }
        }
    }

    if (vmajor != NSS_VMAJOR) {
        return PR_FALSE;
    }
    if (vmajor == NSS_VMAJOR && vminor > NSS_VMINOR) {
        return PR_FALSE;
    }
    if (vmajor == NSS_VMAJOR && vminor == NSS_VMINOR && vpatch > NSS_VPATCH) {
        return PR_FALSE;
    }
    if (vmajor == NSS_VMAJOR && vminor == NSS_VMINOR &&
        vpatch == NSS_VPATCH && vbuild > NSS_VBUILD) {
        return PR_FALSE;
    }
    
    if (PR_VersionCheck(PR_VERSION) == PR_FALSE) {
        return PR_FALSE;
    }
    return PR_TRUE;
}

const char *
NSS_GetVersion(void)
{
    return NSS_VERSION;
}
