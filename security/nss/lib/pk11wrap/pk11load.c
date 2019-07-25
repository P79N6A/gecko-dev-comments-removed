






































#define FORCE_PR_LOG 1
#include "seccomon.h"
#include "pkcs11.h"
#include "secmod.h"
#include "prlink.h"
#include "pk11func.h"
#include "secmodi.h"
#include "secmodti.h"
#include "nssilock.h"
#include "secerr.h"
#include "prenv.h"

#define DEBUG_MODULE 1

#ifdef DEBUG_MODULE
static char *modToDBG = NULL;

#include "debug_module.c"
#endif


CK_RV PR_CALLBACK secmodCreateMutext(CK_VOID_PTR_PTR pmutex) {
    *pmutex = (CK_VOID_PTR) PZ_NewLock(nssILockOther);
    if ( *pmutex ) return CKR_OK;
    return CKR_HOST_MEMORY;
}

CK_RV PR_CALLBACK secmodDestroyMutext(CK_VOID_PTR mutext) {
    PZ_DestroyLock((PZLock *)mutext);
    return CKR_OK;
}

CK_RV PR_CALLBACK secmodLockMutext(CK_VOID_PTR mutext) {
    PZ_Lock((PZLock *)mutext);
    return CKR_OK;
}

CK_RV PR_CALLBACK secmodUnlockMutext(CK_VOID_PTR mutext) {
    PZ_Unlock((PZLock *)mutext);
    return CKR_OK;
}

static SECMODModuleID  nextModuleID = 1;
static const CK_C_INITIALIZE_ARGS secmodLockFunctions = {
    secmodCreateMutext, secmodDestroyMutext, secmodLockMutext, 
    secmodUnlockMutext, CKF_LIBRARY_CANT_CREATE_OS_THREADS|
	CKF_OS_LOCKING_OK
    ,NULL
};

static PRBool loadSingleThreadedModules = PR_TRUE;
static PRBool enforceAlreadyInitializedError = PR_TRUE;
static PRBool finalizeModules = PR_TRUE;


SECStatus pk11_setGlobalOptions(PRBool noSingleThreadedModules,
                                PRBool allowAlreadyInitializedModules,
                                PRBool dontFinalizeModules)
{
    if (noSingleThreadedModules) {
        loadSingleThreadedModules = PR_FALSE;
    } else {
        loadSingleThreadedModules = PR_TRUE;
    }
    if (allowAlreadyInitializedModules) {
        enforceAlreadyInitializedError = PR_FALSE;
    } else {
        enforceAlreadyInitializedError = PR_TRUE;
    }
    if (dontFinalizeModules) {
        finalizeModules = PR_FALSE;
    } else {
        finalizeModules = PR_TRUE;
    }
    return SECSuccess;
}

PRBool pk11_getFinalizeModulesOption(void)
{
    return finalizeModules;
}













static SECStatus
secmod_handleReload(SECMODModule *oldModule, SECMODModule *newModule)
{
    PK11SlotInfo *slot;
    char *modulespec;
    char *newModuleSpec;
    char **children;
    CK_SLOT_ID *ids;
    SECMODConfigList *conflist = NULL;
    SECStatus         rv       = SECFailure;
    int               count    = 0;

    
    modulespec = newModule->libraryParams;
    newModuleSpec = secmod_ParseModuleSpecForTokens(PR_TRUE,
				newModule->isFIPS, modulespec, &children, &ids);
    if (!newModuleSpec) {
	return SECFailure;
    }

    







    if (oldModule->internal) {
	conflist = secmod_GetConfigList(oldModule->isFIPS, 
					oldModule->libraryParams, &count);
    }


    
    if (conflist && secmod_MatchConfigList(newModuleSpec, conflist, count)) { 
	rv = SECSuccess;
	goto loser;
    }
    slot = SECMOD_OpenNewSlot(oldModule, newModuleSpec);
    if (slot) {
	int newID;
	char **thisChild;
	CK_SLOT_ID *thisID;
	char *oldModuleSpec;

	if (secmod_IsInternalKeySlot(newModule)) {
	    pk11_SetInternalKeySlot(slot);
	}
	newID = slot->slotID;
	PK11_FreeSlot(slot);
	for (thisChild=children, thisID=ids; thisChild && *thisChild; 
						thisChild++,thisID++) {
	    if (conflist &&
		       secmod_MatchConfigList(*thisChild, conflist, count)) {
		*thisID = (CK_SLOT_ID) -1;
		continue;
	    }
	    slot = SECMOD_OpenNewSlot(oldModule, *thisChild);
	    if (slot) {
		*thisID = slot->slotID;
		PK11_FreeSlot(slot);
	    } else {
		*thisID = (CK_SLOT_ID) -1;
	    }
	}

	


	oldModuleSpec = secmod_MkAppendTokensList(oldModule->arena, 
		oldModule->libraryParams, newModuleSpec, newID, 
		children, ids);
	if (oldModuleSpec) {
	    oldModule->libraryParams = oldModuleSpec;
	}
	
	rv = SECSuccess;
    }

loser:
    secmod_FreeChildren(children, ids);
    PORT_Free(newModuleSpec);
    if (conflist) {
	secmod_FreeConfigList(conflist, count);
    }
    return rv;
}




SECStatus
secmod_ModuleInit(SECMODModule *mod, SECMODModule **reload, 
		  PRBool* alreadyLoaded)
{
    CK_C_INITIALIZE_ARGS moduleArgs;
    CK_VOID_PTR pInitArgs;
    CK_RV crv;

    if (reload) {
	*reload = NULL;
    }

    if (!mod || !alreadyLoaded) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (mod->isThreadSafe == PR_FALSE) {
	pInitArgs = NULL;
    } else if (mod->libraryParams == NULL) {
	pInitArgs = (void *) &secmodLockFunctions;
    } else {
	moduleArgs = secmodLockFunctions;
	moduleArgs.LibraryParameters = (void *) mod->libraryParams;
	pInitArgs = &moduleArgs;
    }
    crv = PK11_GETTAB(mod)->C_Initialize(pInitArgs);
    if (CKR_CRYPTOKI_ALREADY_INITIALIZED == crv) {
	SECMODModule *oldModule = NULL;

	

	if (reload != NULL && mod->libraryParams) {
	    oldModule = secmod_FindModuleByFuncPtr(mod->functionList);
	}
	

	if (oldModule) {
	    SECStatus rv;
	    rv = secmod_handleReload(oldModule, mod);
	    if (rv == SECSuccess) {
		




		mod->functionList = NULL;
		*reload = oldModule;
		return SECSuccess;
	    }
	    SECMOD_DestroyModule(oldModule);
	}
	
	if (!enforceAlreadyInitializedError) {
       	    *alreadyLoaded = PR_TRUE;
            return SECSuccess;
	}
    }
    if (crv != CKR_OK) {
	if (pInitArgs == NULL ||
		crv == CKR_NETSCAPE_CERTDB_FAILED ||
		crv == CKR_NETSCAPE_KEYDB_FAILED) {
	    PORT_SetError(PK11_MapError(crv));
	    return SECFailure;
	}
	if (!loadSingleThreadedModules) {
	    PORT_SetError(SEC_ERROR_INCOMPATIBLE_PKCS11);
	    return SECFailure;
	}
	mod->isThreadSafe = PR_FALSE;
    	crv = PK11_GETTAB(mod)->C_Initialize(NULL);
	if ((CKR_CRYPTOKI_ALREADY_INITIALIZED == crv) &&
	    (!enforceAlreadyInitializedError)) {
	    *alreadyLoaded = PR_TRUE;
	    return SECSuccess;
	}
    	if (crv != CKR_OK)  {
	    PORT_SetError(PK11_MapError(crv));
	    return SECFailure;
	}
    }
    return SECSuccess;
}





void
SECMOD_SetRootCerts(PK11SlotInfo *slot, SECMODModule *mod) {
    PK11PreSlotInfo *psi = NULL;
    int i;

    if (slot->hasRootCerts) {
	for (i=0; i < mod->slotInfoCount; i++) {
	    if (slot->slotID == mod->slotInfo[i].slotID) {
		psi = &mod->slotInfo[i];
		break;
	    }
	}
	if (psi == NULL) {
	   
	   PK11PreSlotInfo *psi_list = (PK11PreSlotInfo *)
		PORT_ArenaAlloc(mod->arena,
			(mod->slotInfoCount+1)* sizeof(PK11PreSlotInfo));
	   
	   if (mod->slotInfoCount > 0) {
		PORT_Memcpy(psi_list,mod->slotInfo,
				(mod->slotInfoCount)*sizeof(PK11PreSlotInfo));
	   }
	   
	   psi = &psi_list[mod->slotInfoCount];
	   psi->slotID = slot->slotID;
	   psi->askpw = 0;
	   psi->timeout = 0;
	   psi ->defaultFlags = 0;

	   
	   mod->slotInfo = psi_list;
	   mod->slotInfoCount++;
	   
	}
	psi->hasRootCerts = 1;
    }
}

static const char* my_shlib_name =
    SHLIB_PREFIX"nss"SHLIB_VERSION"."SHLIB_SUFFIX;
static const char* softoken_shlib_name =
    SHLIB_PREFIX"softokn"SOFTOKEN_SHLIB_VERSION"."SHLIB_SUFFIX;
static const PRCallOnceType pristineCallOnce;
static PRCallOnceType loadSoftokenOnce;
static PRLibrary* softokenLib;
static PRInt32 softokenLoadCount;

#include "prio.h"
#include "prprf.h"
#include <stdio.h>
#include "prsystem.h"



static PRStatus
softoken_LoadDSO( void ) 
{
  PRLibrary *  handle;

  handle = PORT_LoadLibraryFromOrigin(my_shlib_name,
                                      (PRFuncPtr) &softoken_LoadDSO,
                                      softoken_shlib_name);
  if (handle) {
    softokenLib = handle;
    return PR_SUCCESS;
  }
  return PR_FAILURE;
}




SECStatus
secmod_LoadPKCS11Module(SECMODModule *mod, SECMODModule **oldModule) {
    PRLibrary *library = NULL;
    CK_C_GetFunctionList entry = NULL;
    char * full_name;
    CK_INFO info;
    CK_ULONG slotCount = 0;
    SECStatus rv;
    PRBool alreadyLoaded = PR_FALSE;
    char *disableUnload = NULL;

    if (mod->loaded) return SECSuccess;

    
    if (mod->internal && (mod->dllName == NULL)) {
    



    if (!softokenLib && 
        PR_SUCCESS != PR_CallOnce(&loadSoftokenOnce, &softoken_LoadDSO))
        return SECFailure;

    PR_ATOMIC_INCREMENT(&softokenLoadCount);

    if (mod->isFIPS) {
        entry = (CK_C_GetFunctionList) 
                    PR_FindSymbol(softokenLib, "FC_GetFunctionList");
    } else {
        entry = (CK_C_GetFunctionList) 
                    PR_FindSymbol(softokenLib, "NSC_GetFunctionList");
    }

    if (!entry)
        return SECFailure;

    if (mod->isModuleDB) {
        mod->moduleDBFunc = (CK_C_GetFunctionList) 
                    PR_FindSymbol(softokenLib, "NSC_ModuleDBFunc");
    }

    if (mod->moduleDBOnly) {
        mod->loaded = PR_TRUE;
        return SECSuccess;
    }
    } else {
	
	if (mod->dllName == NULL) {
	    return SECFailure;
	}

	full_name = PORT_Strdup(mod->dllName);

	


	library = PR_LoadLibrary(full_name);
	mod->library = (void *)library;
	PORT_Free(full_name);

	if (library == NULL) {
	    return SECFailure;
	}

	


	if (!mod->moduleDBOnly) {
	    entry = (CK_C_GetFunctionList)
			PR_FindSymbol(library, "C_GetFunctionList");
	}
	if (mod->isModuleDB) {
	    mod->moduleDBFunc = (void *)
			PR_FindSymbol(library, "NSS_ReturnModuleSpecData");
	}
	if (mod->moduleDBFunc == NULL) mod->isModuleDB = PR_FALSE;
	if (entry == NULL) {
	    if (mod->isModuleDB) {
		mod->loaded = PR_TRUE;
		mod->moduleDBOnly = PR_TRUE;
		return SECSuccess;
	    }
	    PR_UnloadLibrary(library);
	    return SECFailure;
	}
    }

    


    if ((*entry)((CK_FUNCTION_LIST_PTR *)&mod->functionList) != CKR_OK) 
								goto fail;

#ifdef DEBUG_MODULE
    if (PR_TRUE) {
	modToDBG = PR_GetEnv("NSS_DEBUG_PKCS11_MODULE");
	if (modToDBG && strcmp(mod->commonName, modToDBG) == 0) {
	    mod->functionList = (void *)nss_InsertDeviceLog(
	                           (CK_FUNCTION_LIST_PTR)mod->functionList);
	}
    }
#endif

    mod->isThreadSafe = PR_TRUE;

    
    rv = secmod_ModuleInit(mod, oldModule, &alreadyLoaded);
    if (rv != SECSuccess) {
	goto fail;
    }

    

    if (mod->functionList == NULL) {
	mod->loaded = PR_TRUE; 
	return SECSuccess;
    }

    
    if (PK11_GETTAB(mod)->C_GetInfo(&info) != CKR_OK) goto fail2;
    if (info.cryptokiVersion.major != 2) goto fail2;
    
    if (info.cryptokiVersion.minor < 1) {
        if (!loadSingleThreadedModules) {
            PORT_SetError(SEC_ERROR_INCOMPATIBLE_PKCS11);
            goto fail2;
        } else {
            mod->isThreadSafe = PR_FALSE;
        }
    }
    mod->cryptokiVersion = info.cryptokiVersion;

    
    if ((mod->commonName == NULL) || (mod->commonName[0] == 0)) {
	mod->commonName = PK11_MakeString(mod->arena,NULL,
	   (char *)info.libraryDescription, sizeof(info.libraryDescription));
	if (mod->commonName == NULL) goto fail2;
    }
    

    
    if (PK11_GETTAB(mod)->C_GetSlotList(CK_FALSE, NULL, &slotCount) == CKR_OK) {
	CK_SLOT_ID *slotIDs;
	int i;
	CK_RV crv;

	mod->slots = (PK11SlotInfo **)PORT_ArenaAlloc(mod->arena,
					sizeof(PK11SlotInfo *) * slotCount);
	if (mod->slots == NULL) goto fail2;

	slotIDs = (CK_SLOT_ID *) PORT_Alloc(sizeof(CK_SLOT_ID)*slotCount);
	if (slotIDs == NULL) {
	    goto fail2;
	}  
	crv = PK11_GETTAB(mod)->C_GetSlotList(CK_FALSE, slotIDs, &slotCount);
	if (crv != CKR_OK) {
	    PORT_Free(slotIDs);
	    goto fail2;
	}

	
	for (i=0; i < (int)slotCount; i++) {
	    mod->slots[i] = PK11_NewSlotInfo(mod);
	    PK11_InitSlot(mod,slotIDs[i],mod->slots[i]);
	    
	    PK11_LoadSlotList(mod->slots[i],mod->slotInfo,mod->slotInfoCount);
	    SECMOD_SetRootCerts(mod->slots[i],mod);
	}
	mod->slotCount = slotCount;
	mod->slotInfoCount = 0;
	PORT_Free(slotIDs);
    }
    
    mod->loaded = PR_TRUE;
    mod->moduleID = nextModuleID++;
    return SECSuccess;
fail2:
    if (enforceAlreadyInitializedError || (!alreadyLoaded)) {
        PK11_GETTAB(mod)->C_Finalize(NULL);
    }
fail:
    mod->functionList = NULL;
    disableUnload = PR_GetEnv("NSS_DISABLE_UNLOAD");
    if (library && !disableUnload) {
        PR_UnloadLibrary(library);
    }
    return SECFailure;
}

SECStatus
SECMOD_UnloadModule(SECMODModule *mod) {
    PRLibrary *library;
    char *disableUnload = NULL;

    if (!mod->loaded) {
	return SECFailure;
    }
    if (finalizeModules) {
        if (mod->functionList &&!mod->moduleDBOnly) {
	    PK11_GETTAB(mod)->C_Finalize(NULL);
	}
    }
    mod->moduleID = 0;
    mod->loaded = PR_FALSE;
    
    


    if (mod->internal) {
        if (0 == PR_ATOMIC_DECREMENT(&softokenLoadCount)) {
          if (softokenLib) {
              disableUnload = PR_GetEnv("NSS_DISABLE_UNLOAD");
              if (!disableUnload) {
                  PRStatus status = PR_UnloadLibrary(softokenLib);
                  PORT_Assert(PR_SUCCESS == status);
              }
              softokenLib = NULL;
          }
          loadSoftokenOnce = pristineCallOnce;
        }
	return SECSuccess;
    }

    library = (PRLibrary *)mod->library;
    
    if (library == NULL) {
	return SECFailure;
    }

    disableUnload = PR_GetEnv("NSS_DISABLE_UNLOAD");
    if (!disableUnload) {
        PR_UnloadLibrary(library);
    }
    return SECSuccess;
}

void
nss_DumpModuleLog(void)
{
#ifdef DEBUG_MODULE
    if (modToDBG) {
	print_final_statistics();
    }
#endif
}
