





































#include "seccomon.h"
#include "secmod.h"
#include "nssilock.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pk11func.h"
#include "pki3hack.h"
#include "secerr.h"
#include "dev.h"
#include "pkcs11ni.h"



static  SECMODModuleList *modules = NULL;
static  SECMODModuleList *modulesDB = NULL;
static  SECMODModuleList *modulesUnload = NULL;
static  SECMODModule *internalModule = NULL;
static  SECMODModule *defaultDBModule = NULL;
static  SECMODModule *pendingModule = NULL;
static SECMODListLock *moduleLock = NULL;

int secmod_PrivateModuleCount = 0;

extern PK11DefaultArrayEntry PK11_DefaultArray[];
extern int num_pk11_default_mechanisms;


void
SECMOD_Init() 
{
    
    if (moduleLock) return;

    moduleLock = SECMOD_NewListLock();
    PK11_InitSlotLists();
}


SECStatus
SECMOD_Shutdown() 
{
    
    if (moduleLock) {
	SECMOD_DestroyListLock(moduleLock);
	moduleLock = NULL;
    }
    
    if (internalModule) {
	SECMOD_DestroyModule(internalModule);
	internalModule = NULL;
    }

    
    if (defaultDBModule) {
	SECMOD_DestroyModule(defaultDBModule);
	defaultDBModule = NULL;
    }
	
    
    if (modules) {
	SECMOD_DestroyModuleList(modules);
	modules = NULL;
    }
   
    if (modulesDB) {
	SECMOD_DestroyModuleList(modulesDB);
	modulesDB = NULL;
    }

    if (modulesUnload) {
	SECMOD_DestroyModuleList(modulesUnload);
	modulesUnload = NULL;
    }

    
    PK11_DestroySlotLists();

    nss_DumpModuleLog();

#ifdef DEBUG
    if (PR_GetEnv("NSS_STRICT_SHUTDOWN")) {
	PORT_Assert(secmod_PrivateModuleCount == 0);
    }
#endif
    if (secmod_PrivateModuleCount) {
    	PORT_SetError(SEC_ERROR_BUSY);
	return SECFailure;
    }
    return SECSuccess;
}





SECMODModule *
SECMOD_GetInternalModule(void)
{
   return internalModule;
}


SECStatus
secmod_AddModuleToList(SECMODModuleList **moduleList,SECMODModule *newModule)
{
    SECMODModuleList *mlp, *newListElement, *last = NULL;

    newListElement = SECMOD_NewModuleListElement();
    if (newListElement == NULL) {
	return SECFailure;
    }

    newListElement->module = SECMOD_ReferenceModule(newModule);

    SECMOD_GetWriteLock(moduleLock);
    


    for(mlp = *moduleList; mlp != NULL; mlp = mlp->next) {
	last = mlp;
    }

    if (last == NULL) {
	*moduleList = newListElement;
    } else {
	SECMOD_AddList(last,newListElement,NULL);
    }
    SECMOD_ReleaseWriteLock(moduleLock);
    return SECSuccess;
}

SECStatus
SECMOD_AddModuleToList(SECMODModule *newModule)
{
    if (newModule->internal && !internalModule) {
	internalModule = SECMOD_ReferenceModule(newModule);
    }
    return secmod_AddModuleToList(&modules,newModule);
}

SECStatus
SECMOD_AddModuleToDBOnlyList(SECMODModule *newModule)
{
    if (defaultDBModule == NULL) {
	defaultDBModule = SECMOD_ReferenceModule(newModule);
    }
    return secmod_AddModuleToList(&modulesDB,newModule);
}

SECStatus
SECMOD_AddModuleToUnloadList(SECMODModule *newModule)
{
    return secmod_AddModuleToList(&modulesUnload,newModule);
}




SECMODModuleList * SECMOD_GetDefaultModuleList() { return modules; }
SECMODModuleList *SECMOD_GetDeadModuleList() { return modulesUnload; }
SECMODModuleList *SECMOD_GetDBModuleList() { return modulesDB; }












SECMODListLock *SECMOD_GetDefaultModuleListLock() { return moduleLock; }







SECMODModule *
SECMOD_FindModule(const char *name)
{
    SECMODModuleList *mlp;
    SECMODModule *module = NULL;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return module;
    }
    SECMOD_GetReadLock(moduleLock);
    for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	if (PORT_Strcmp(name,mlp->module->commonName) == 0) {
	    module = mlp->module;
	    SECMOD_ReferenceModule(module);
	    break;
	}
    }
    if (module) {
	goto found;
    }
    for(mlp = modulesUnload; mlp != NULL; mlp = mlp->next) {
	if (PORT_Strcmp(name,mlp->module->commonName) == 0) {
	    module = mlp->module;
	    SECMOD_ReferenceModule(module);
	    break;
	}
    }

found:
    SECMOD_ReleaseReadLock(moduleLock);

    return module;
}





SECMODModule *
SECMOD_FindModuleByID(SECMODModuleID id) 
{
    SECMODModuleList *mlp;
    SECMODModule *module = NULL;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return module;
    }
    SECMOD_GetReadLock(moduleLock);
    for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	if (id == mlp->module->moduleID) {
	    module = mlp->module;
	    SECMOD_ReferenceModule(module);
	    break;
	}
    }
    SECMOD_ReleaseReadLock(moduleLock);
    if (module == NULL) {
	PORT_SetError(SEC_ERROR_NO_MODULE);
    }
    return module;
}




PK11SlotInfo *
SECMOD_FindSlotByID(SECMODModule *module, CK_SLOT_ID slotID)
{
    int i;
    PK11SlotInfo *slot = NULL;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return slot;
    }
    SECMOD_GetReadLock(moduleLock);
    for (i=0; i < module->slotCount; i++) {
	PK11SlotInfo *cSlot = module->slots[i];

	if (cSlot->slotID == slotID) {
	    slot = PK11_ReferenceSlot(cSlot);
	    break;
	}
    }
    SECMOD_ReleaseReadLock(moduleLock);

    if (slot == NULL) {
	PORT_SetError(SEC_ERROR_NO_SLOT_SELECTED);
    }
    return slot;
}




PK11SlotInfo *
SECMOD_LookupSlot(SECMODModuleID moduleID,CK_SLOT_ID slotID) 
{
    SECMODModule *module;
    PK11SlotInfo *slot;

    module = SECMOD_FindModuleByID(moduleID);
    if (module == NULL) return NULL;

    slot = SECMOD_FindSlotByID(module, slotID);
    SECMOD_DestroyModule(module);
    return slot;
}






SECStatus
SECMOD_DeleteModuleEx(const char *name, SECMODModule *mod, 
						int *type, PRBool permdb) 
{
    SECMODModuleList *mlp;
    SECMODModuleList **mlpp;
    SECStatus rv = SECFailure;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return rv;
    }

    *type = SECMOD_EXTERNAL;

    SECMOD_GetWriteLock(moduleLock);
    for (mlpp = &modules,mlp = modules; 
				mlp != NULL; mlpp = &mlp->next, mlp = *mlpp) {
	if ((name && (PORT_Strcmp(name,mlp->module->commonName) == 0)) ||
							mod == mlp->module) {
	    
	    if (!mlp->module->internal) {
		SECMOD_RemoveList(mlpp,mlp);
		
		rv = STAN_RemoveModuleFromDefaultTrustDomain(mlp->module);
	    } else if (mlp->module->isFIPS) {
		*type = SECMOD_FIPS;
	    } else {
		*type = SECMOD_INTERNAL;
	    }
	    break;
	}
    }
    if (mlp) {
	goto found;
    }
    
    for (mlpp = &modulesUnload,mlp = modulesUnload; 
				mlp != NULL; mlpp = &mlp->next, mlp = *mlpp) {
	if ((name && (PORT_Strcmp(name,mlp->module->commonName) == 0)) ||
							mod == mlp->module) {
	    
	    if (!mlp->module->internal) {
		SECMOD_RemoveList(mlpp,mlp);
		rv = SECSuccess;
	    } else if (mlp->module->isFIPS) {
		*type = SECMOD_FIPS;
	    } else {
		*type = SECMOD_INTERNAL;
	    }
	    break;
	}
    }
found:
    SECMOD_ReleaseWriteLock(moduleLock);


    if (rv == SECSuccess) {
	if (permdb) {
 	    SECMOD_DeletePermDB(mlp->module);
	}
	SECMOD_DestroyModuleListElement(mlp);
    }
    return rv;
}




SECStatus
SECMOD_DeleteModule(const char *name, int *type) 
{
    return SECMOD_DeleteModuleEx(name, NULL, type, PR_TRUE);
}




SECStatus
SECMOD_DeleteInternalModule(const char *name) 
{
    SECMODModuleList *mlp;
    SECMODModuleList **mlpp;
    SECStatus rv = SECFailure;

    if (pendingModule) {
	PORT_SetError(SEC_ERROR_MODULE_STUCK);
	return rv;
    }
    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return rv;
    }

    SECMOD_GetWriteLock(moduleLock);
    for(mlpp = &modules,mlp = modules; 
				mlp != NULL; mlpp = &mlp->next, mlp = *mlpp) {
	if (PORT_Strcmp(name,mlp->module->commonName) == 0) {
	    
	    if (mlp->module->internal) {
		SECMOD_RemoveList(mlpp,mlp);
		rv = STAN_RemoveModuleFromDefaultTrustDomain(mlp->module);
	    } 
	    break;
	}
    }
    SECMOD_ReleaseWriteLock(moduleLock);

    if (rv == SECSuccess) {
	SECMODModule *newModule,*oldModule;

	if (mlp->module->isFIPS) {
    	    newModule = SECMOD_CreateModule(NULL, SECMOD_INT_NAME,
				NULL, SECMOD_INT_FLAGS);
	} else {
    	    newModule = SECMOD_CreateModule(NULL, SECMOD_FIPS_NAME,
				NULL, SECMOD_FIPS_FLAGS);
	}
	if (newModule) {
	    newModule->libraryParams = 
	     PORT_ArenaStrdup(newModule->arena,mlp->module->libraryParams);
	    rv = SECMOD_AddModule(newModule);
	    if (rv != SECSuccess) {
		SECMOD_DestroyModule(newModule);
		newModule = NULL;
	    }
	}
	if (newModule == NULL) {
	    SECMODModuleList *last = NULL,*mlp2;
	   


	   SECMOD_GetWriteLock(moduleLock);
	   for(mlp2 = modules; mlp2 != NULL; mlp2 = mlp->next) {
		last = mlp2;
	   }

	   if (last == NULL) {
		modules = mlp;
	   } else {
		SECMOD_AddList(last,mlp,NULL);
	   }
	   SECMOD_ReleaseWriteLock(moduleLock);
	   return SECFailure; 
	}
	pendingModule = oldModule = internalModule;
	internalModule = NULL;
	SECMOD_DestroyModule(oldModule);
 	SECMOD_DeletePermDB(mlp->module);
	SECMOD_DestroyModuleListElement(mlp);
	internalModule = newModule; 
    }
    return rv;
}

SECStatus
SECMOD_AddModule(SECMODModule *newModule) 
{
    SECStatus rv;
    SECMODModule *oldModule;

    
    
    
    
    
    if ((oldModule = SECMOD_FindModule(newModule->commonName)) != NULL) {
	SECMOD_DestroyModule(oldModule);
        return SECWouldBlock;
        
    }

    rv = SECMOD_LoadPKCS11Module(newModule);
    if (rv != SECSuccess) {
	return rv;
    }

    if (newModule->parent == NULL) {
	newModule->parent = SECMOD_ReferenceModule(defaultDBModule);
    }

    SECMOD_AddPermDB(newModule);
    SECMOD_AddModuleToList(newModule);

    rv = STAN_AddModuleToDefaultTrustDomain(newModule);

    return rv;
}

PK11SlotInfo *
SECMOD_FindSlot(SECMODModule *module,const char *name) 
{
    int i;
    char *string;
    PK11SlotInfo *retSlot = NULL;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return retSlot;
    }
    SECMOD_GetReadLock(moduleLock);
    for (i=0; i < module->slotCount; i++) {
	PK11SlotInfo *slot = module->slots[i];

	if (PK11_IsPresent(slot)) {
	    string = PK11_GetTokenName(slot);
	} else {
	    string = PK11_GetSlotName(slot);
	}
	if (PORT_Strcmp(name,string) == 0) {
	    retSlot = PK11_ReferenceSlot(slot);
	    break;
	}
    }
    SECMOD_ReleaseReadLock(moduleLock);

    if (retSlot == NULL) {
	PORT_SetError(SEC_ERROR_NO_SLOT_SELECTED);
    }
    return retSlot;
}

SECStatus
PK11_GetModInfo(SECMODModule *mod,CK_INFO *info)
{
    CK_RV crv;

    if (mod->functionList == NULL) return SECFailure;
    crv = PK11_GETTAB(mod)->C_GetInfo(info);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
    }	
    return (crv == CKR_OK) ? SECSuccess : SECFailure;
}





PRBool
PK11_IsFIPS(void)
{
    SECMODModule *mod = SECMOD_GetInternalModule();

    if (mod && mod->internal) {
	return mod->isFIPS;
    }

    return PR_FALSE;
}




SECStatus 
SECMOD_AddNewModuleEx(const char* moduleName, const char* dllPath,
                              unsigned long defaultMechanismFlags,
                              unsigned long cipherEnableFlags,
                              char* modparms, char* nssparms)
{
    SECMODModule *module;
    SECStatus result = SECFailure;
    int s,i;
    PK11SlotInfo* slot;

    PR_SetErrorText(0, NULL);
    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return result;
    }

    module = SECMOD_CreateModule(dllPath, moduleName, modparms, nssparms);

    if (module == NULL) {
	return result;
    }

    if (module->dllName != NULL) {
        if (module->dllName[0] != 0) {
            result = SECMOD_AddModule(module);
            if (result == SECSuccess) {
                
                module->ssl[0] = cipherEnableFlags;

 		SECMOD_GetReadLock(moduleLock);
                
                for (s = 0; s < module->slotCount; s++) {
                    slot = (module->slots)[s];
                    
                    for (i=0; i < num_pk11_default_mechanisms; i++) {
                        
			PRBool add = 
			 (PK11_DefaultArray[i].flag & defaultMechanismFlags) ?
						PR_TRUE: PR_FALSE;
                        result = PK11_UpdateSlotAttribute(slot, 
					&(PK11_DefaultArray[i]),  add);
                    } 
                    
                    if (defaultMechanismFlags & PK11_DISABLE_FLAG) {
                        PK11_UserDisableSlot(slot);
                    }
                } 
 		SECMOD_ReleaseReadLock(moduleLock);

                

		result = SECMOD_UpdateModule(module);
            }
        }
    }
    SECMOD_DestroyModule(module);
    return result;
}

SECStatus 
SECMOD_AddNewModule(const char* moduleName, const char* dllPath,
                              unsigned long defaultMechanismFlags,
                              unsigned long cipherEnableFlags)
{
    return SECMOD_AddNewModuleEx(moduleName, dllPath, defaultMechanismFlags,
                  cipherEnableFlags, 
                  NULL, NULL); 
}

SECStatus 
SECMOD_UpdateModule(SECMODModule *module)
{
    SECStatus result;

    result = SECMOD_DeletePermDB(module);
                
    if (result == SECSuccess) {          
	result = SECMOD_AddPermDB(module);
    }
    return result;
}








unsigned long 
SECMOD_PubMechFlagstoInternal(unsigned long publicFlags)
{
    unsigned long internalFlags = publicFlags;

    if (publicFlags & PUBLIC_MECH_RANDOM_FLAG) {
        internalFlags &= ~PUBLIC_MECH_RANDOM_FLAG;
        internalFlags |= SECMOD_RANDOM_FLAG;
    }
    return internalFlags;
}

unsigned long 
SECMOD_InternaltoPubMechFlags(unsigned long internalFlags) 
{
    unsigned long publicFlags = internalFlags;

    if (internalFlags & SECMOD_RANDOM_FLAG) {
        publicFlags &= ~SECMOD_RANDOM_FLAG;
        publicFlags |= PUBLIC_MECH_RANDOM_FLAG;
    }
    return publicFlags;
}





unsigned long 
SECMOD_PubCipherFlagstoInternal(unsigned long publicFlags) 
{
    return publicFlags;
}

unsigned long 
SECMOD_InternaltoPubCipherFlags(unsigned long internalFlags) 
{
    return internalFlags;
}


PRBool 
SECMOD_IsModulePresent( unsigned long int pubCipherEnableFlags )
{
    PRBool result = PR_FALSE;
    SECMODModuleList *mods;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return result;
    }
    SECMOD_GetReadLock(moduleLock);
    mods = SECMOD_GetDefaultModuleList();
    for ( ; mods != NULL; mods = mods->next) {
        if (mods->module->ssl[0] & 
		SECMOD_PubCipherFlagstoInternal(pubCipherEnableFlags)) {
            result = PR_TRUE;
        }
    }

    SECMOD_ReleaseReadLock(moduleLock);
    return result;
}


SECMODModuleList *SECMOD_NewModuleListElement(void) 
{
    SECMODModuleList *newModList;

    newModList= (SECMODModuleList *) PORT_Alloc(sizeof(SECMODModuleList));
    if (newModList) {
	newModList->next = NULL;
	newModList->module = NULL;
    }
    return newModList;
}




SECMODModule *
SECMOD_ReferenceModule(SECMODModule *module) 
{
    PZ_Lock(module->refLock);
    PORT_Assert(module->refCount > 0);

    module->refCount++;
    PZ_Unlock(module->refLock);
    return module;
}



void
SECMOD_DestroyModule(SECMODModule *module) 
{
    PRBool willfree = PR_FALSE;
    int slotCount;
    int i;

    PZ_Lock(module->refLock);
    if (module->refCount-- == 1) {
	willfree = PR_TRUE;
    }
    PORT_Assert(willfree || (module->refCount > 0));
    PZ_Unlock(module->refLock);

    if (!willfree) {
	return;
    }
   
    if (module->parent != NULL) {
	SECMODModule *parent = module->parent;
	
	module->parent = NULL;
	SECMOD_DestroyModule(parent);
    }

    

    slotCount = module->slotCount;
    if (slotCount == 0) {
	SECMOD_SlotDestroyModule(module,PR_FALSE);
	return;
    }

    

    for (i=0 ; i < slotCount; i++) {
	if (!module->slots[i]->disabled) {
		PK11_ClearSlotList(module->slots[i]);
	}
	PK11_FreeSlot(module->slots[i]);
    }
    

}




void
SECMOD_SlotDestroyModule(SECMODModule *module, PRBool fromSlot) 
{
    PRBool willfree = PR_FALSE;
    if (fromSlot) {
        PORT_Assert(module->refCount == 0);
	PZ_Lock(module->refLock);
	if (module->slotCount-- == 1) {
	    willfree = PR_TRUE;
	}
	PORT_Assert(willfree || (module->slotCount > 0));
	PZ_Unlock(module->refLock);
        if (!willfree) return;
    }

    if (module == pendingModule) {
	pendingModule = NULL;
    }

    if (module->loaded) {
	SECMOD_UnloadModule(module);
    }
    PZ_DestroyLock(module->refLock);
    PORT_FreeArena(module->arena,PR_FALSE);
    secmod_PrivateModuleCount--;
}





SECMODModuleList *
SECMOD_DestroyModuleListElement(SECMODModuleList *element) 
{
    SECMODModuleList *next = element->next;

    if (element->module) {
	SECMOD_DestroyModule(element->module);
	element->module = NULL;
    }
    PORT_Free(element);
    return next;
}





void
SECMOD_DestroyModuleList(SECMODModuleList *list) 
{
    SECMODModuleList *lp;

    for ( lp = list; lp != NULL; lp = SECMOD_DestroyModuleListElement(lp)) ;
}

PRBool
SECMOD_CanDeleteInternalModule(void)
{
    return (PRBool) (pendingModule == NULL);
}









SECStatus
SECMOD_UpdateSlotList(SECMODModule *mod)
{
    CK_RV crv;
    CK_ULONG count;
    CK_ULONG i, oldCount;
    PRBool freeRef = PR_FALSE;
    void *mark = NULL;
    CK_ULONG *slotIDs = NULL;
    PK11SlotInfo **newSlots = NULL;
    PK11SlotInfo **oldSlots = NULL;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return SECFailure;
    }

    

    PZ_Lock(mod->refLock);
    freeRef = PR_TRUE;
    
    crv = PK11_GETTAB(mod)->C_GetSlotList(PR_FALSE, NULL, &count);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	goto loser;
    }
    

    if (count == mod->slotCount) {
 	PZ_Unlock(mod->refLock);
	return SECSuccess;
    }
    if (count < (CK_ULONG)mod->slotCount) {
	
	PORT_SetError( SEC_ERROR_INCOMPATIBLE_PKCS11 );
	goto loser;
    }

    
    slotIDs = PORT_NewArray(CK_SLOT_ID, count);
    if (slotIDs == NULL) {
	goto loser;
    }

    crv = PK11_GETTAB(mod)->C_GetSlotList(PR_FALSE, slotIDs, &count);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	goto loser;
    }
    freeRef = PR_FALSE;
    PZ_Unlock(mod->refLock);
    mark = PORT_ArenaMark(mod->arena);
    if (mark == NULL) {
	goto loser;
    }
    newSlots = PORT_ArenaZNewArray(mod->arena,PK11SlotInfo *,count);

    


    for (i=0; i < count; i++) {
	PK11SlotInfo *slot = SECMOD_FindSlotByID(mod,slotIDs[i]);

	if (!slot) {
	    
	    slot = PK11_NewSlotInfo(mod);
	    if (!slot) {
		goto loser;
	    }
	    PK11_InitSlot(mod, slotIDs[i], slot);
	    STAN_InitTokenForSlotInfo(NULL, slot);
	}
	newSlots[i] = slot;
    }
    STAN_ResetTokenInterator(NULL);
    PORT_Free(slotIDs);
    slotIDs = NULL;
    PORT_ArenaUnmark(mod->arena, mark);

    




    SECMOD_GetWriteLock(moduleLock);
    oldCount =mod->slotCount;
    oldSlots = mod->slots;
    mod->slots = newSlots; 


    mod->slotCount = count;
    SECMOD_ReleaseWriteLock(moduleLock);
    
    for (i=0; i < oldCount; i++) {
	PK11_FreeSlot(oldSlots[i]);
    }
    return SECSuccess;

loser:
    if (freeRef) {
	PZ_Unlock(mod->refLock);
    }
    if (slotIDs) {
	PORT_Free(slotIDs);
    }
    



    if (newSlots) {
	for (i=0; i < count; i++) {
	    if (newSlots[i] == NULL) {
		break; 
	    }
	    PK11_FreeSlot(newSlots[i]);
	}
    }
    
    if (mark) {
 	PORT_ArenaRelease(mod->arena, mark);
    }
    return SECFailure;
}






PK11SlotInfo *
secmod_HandleWaitForSlotEvent(SECMODModule *mod,  unsigned long flags,
						PRIntervalTime latency)
{
    PRBool removableSlotsFound = PR_FALSE;
    int i;
    int error = SEC_ERROR_NO_EVENT;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return NULL;
    }
    PZ_Lock(mod->refLock);
    if (mod->evControlMask & SECMOD_END_WAIT) {
	mod->evControlMask &= ~SECMOD_END_WAIT;
	PZ_Unlock(mod->refLock);
	PORT_SetError(SEC_ERROR_NO_EVENT);
	return NULL;
    }
    mod->evControlMask |= SECMOD_WAIT_SIMULATED_EVENT;
    while (mod->evControlMask & SECMOD_WAIT_SIMULATED_EVENT) {
	PZ_Unlock(mod->refLock);
	
	SECMOD_UpdateSlotList(mod);

	
	SECMOD_GetReadLock(moduleLock);
	for (i=0; i < mod->slotCount; i++) {
	    PK11SlotInfo *slot = mod->slots[i];
	    uint16 series;
	    PRBool present;

	    
	    if (slot->isPerm) {
		continue;
	    }
	    removableSlotsFound = PR_TRUE;
	    

	    series = slot->series;
	    present = PK11_IsPresent(slot);
	    if ((slot->flagSeries != series) || (slot->flagState != present)) {
		slot->flagState = present;
		slot->flagSeries = series;
		SECMOD_ReleaseReadLock(moduleLock);
		PZ_Lock(mod->refLock);
		mod->evControlMask &= ~SECMOD_END_WAIT;
		PZ_Unlock(mod->refLock);
		return PK11_ReferenceSlot(slot);
	    }
	}
	SECMOD_ReleaseReadLock(moduleLock);
	
	if (!removableSlotsFound) {
	    error =SEC_ERROR_NO_SLOT_SELECTED;
	    PZ_Lock(mod->refLock);
	    break;
	}
	if (flags & CKF_DONT_BLOCK) {
	    PZ_Lock(mod->refLock);
	    break;
	}
	PR_Sleep(latency);
 	PZ_Lock(mod->refLock);
    }
    mod->evControlMask &= ~SECMOD_END_WAIT;
    PZ_Unlock(mod->refLock);
    PORT_SetError(error);
    return NULL;
}







PK11SlotInfo *
SECMOD_WaitForAnyTokenEvent(SECMODModule *mod, unsigned long flags,
						 PRIntervalTime latency)
{
    CK_SLOT_ID id;
    CK_RV crv;
    PK11SlotInfo *slot;

    if (!pk11_getFinalizeModulesOption() ||
        ((mod->cryptokiVersion.major == 2) &&
         (mod->cryptokiVersion.minor < 1))) { 
        



	return secmod_HandleWaitForSlotEvent(mod, flags, latency);
    }
    
    PZ_Lock(mod->refLock);
    if (mod->evControlMask & SECMOD_END_WAIT) {
	goto end_wait;
    }
    mod->evControlMask |= SECMOD_WAIT_PKCS11_EVENT;
    PZ_Unlock(mod->refLock);
    crv = PK11_GETTAB(mod)->C_WaitForSlotEvent(flags, &id, NULL);
    PZ_Lock(mod->refLock);
    mod->evControlMask &= ~SECMOD_WAIT_PKCS11_EVENT;
    

    if (mod->evControlMask & SECMOD_END_WAIT) {
	goto end_wait;
    }
    PZ_Unlock(mod->refLock);
    if (crv == CKR_FUNCTION_NOT_SUPPORTED) {
	
	return secmod_HandleWaitForSlotEvent(mod, flags, latency);
    }
    if (crv != CKR_OK) {
	



	if (crv == CKR_CRYPTOKI_NOT_INITIALIZED) {
	    PORT_SetError(SEC_ERROR_NO_EVENT);
	} else {
	    PORT_SetError(PK11_MapError(crv));
	}
	return NULL;
    }
    slot = SECMOD_FindSlotByID(mod, id);
    if (slot == NULL) {
	
	SECMOD_UpdateSlotList(mod);
	slot = SECMOD_FindSlotByID(mod, id);
    }
    

    if (slot && slot->nssToken && slot->nssToken->slot) {
	nssSlot_ResetDelay(slot->nssToken->slot);
    }
    return slot;

    
end_wait:
    mod->evControlMask &= ~SECMOD_END_WAIT;
    PZ_Unlock(mod->refLock);
    PORT_SetError(SEC_ERROR_NO_EVENT);
    return NULL;
}







SECStatus
SECMOD_CancelWait(SECMODModule *mod)
{
    unsigned long controlMask = mod->evControlMask;
    SECStatus rv = SECSuccess;
    CK_RV crv;

    PZ_Lock(mod->refLock);
    mod->evControlMask |= SECMOD_END_WAIT;
    controlMask = mod->evControlMask;
    if (controlMask & SECMOD_WAIT_PKCS11_EVENT) {
        if (!pk11_getFinalizeModulesOption()) {
            
            PORT_Assert(0);
            PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
            rv = SECFailure;
            goto loser;
        }
	




	crv = PK11_GETTAB(mod)->C_Finalize(NULL);
	

	if (CKR_OK == crv) {
            PRBool alreadyLoaded;
	    secmod_ModuleInit(mod, &alreadyLoaded);
	} else {
	    

	    PORT_SetError(PK11_MapError(crv));
	    rv = SECFailure;
	}
    } else if (controlMask & SECMOD_WAIT_SIMULATED_EVENT) {
	mod->evControlMask &= ~SECMOD_WAIT_SIMULATED_EVENT; 
				

    }
loser:
    PZ_Unlock(mod->refLock);
    return rv;
}





PRBool
SECMOD_HasRemovableSlots(SECMODModule *mod)
{
    int i;
    PRBool ret = PR_FALSE;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return ret;
    }
    SECMOD_GetReadLock(moduleLock);
    for (i=0; i < mod->slotCount; i++) {
	PK11SlotInfo *slot = mod->slots[i];
	
	if (slot->isPerm) {
	    continue;
	}
	ret = PR_TRUE;
	break;
    }
    SECMOD_ReleaseReadLock(moduleLock);
    return ret;
}




static SECStatus
secmod_UserDBOp(PK11SlotInfo *slot, CK_OBJECT_CLASS objClass, 
		const char *sendSpec)
{
    CK_OBJECT_HANDLE dummy;
    CK_ATTRIBUTE template[2] ;
    CK_ATTRIBUTE *attrs = template;
    CK_RV crv;

    PK11_SETATTRS(attrs, CKA_CLASS, &objClass, sizeof(objClass)); attrs++;
    PK11_SETATTRS(attrs, CKA_NETSCAPE_MODULE_SPEC , (unsigned char *)sendSpec,
					 strlen(sendSpec)+1); attrs++;

    PORT_Assert(attrs-template <= 2);


    PK11_EnterSlotMonitor(slot);
    crv = PK11_CreateNewObject(slot, slot->session,
	template, attrs-template, PR_FALSE, &dummy);
    PK11_ExitSlotMonitor(slot);

    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    return SECMOD_UpdateSlotList(slot->module);
}




static char *
nss_addEscape(const char *string, char quote)
{
    char *newString = 0;
    int escapes = 0, size = 0;
    const char *src;
    char *dest;

    for (src=string; *src ; src++) {
        if ((*src == quote) || (*src == '\\')) escapes++;
        size++;
    }

    newString = PORT_ZAlloc(escapes+size+1);
    if (newString == NULL) {
        return NULL;
    }

    for (src=string, dest=newString; *src; src++,dest++) {
        if ((*src == '\\') || (*src == quote)) {
            *dest++ = '\\';
        }
        *dest = *src;
    }

    return newString;
}

static char *
nss_doubleEscape(const char *string)
{
    char *round1 = NULL;
    char *retValue = NULL;
    if (string == NULL) {
        goto done;
    }
    round1 = nss_addEscape(string,'>');
    if (round1) {
        retValue = nss_addEscape(round1,']');
        PORT_Free(round1);
    }

done:
    if (retValue == NULL) {
        retValue = PORT_Strdup("");
    }
    return retValue;
}




static PRBool
secmod_SlotIsEmpty(SECMODModule *mod,  CK_SLOT_ID slotID)
{
    PK11SlotInfo *slot = SECMOD_LookupSlot(mod->moduleID, slotID);
    if (slot) {
	PRBool present = PK11_IsPresent(slot);
	PK11_FreeSlot(slot);
	if (present) {
	    return PR_FALSE;
	}
    }
    
    return PR_TRUE;
}




static CK_SLOT_ID
secmod_FindFreeSlot(SECMODModule *mod)
{
    CK_SLOT_ID i, minSlotID, maxSlotID;

    
    if (mod->internal && mod->isFIPS) {
	minSlotID = SFTK_MIN_FIPS_USER_SLOT_ID;
	maxSlotID = SFTK_MAX_FIPS_USER_SLOT_ID;
    } else {
	minSlotID = SFTK_MIN_USER_SLOT_ID;
	maxSlotID = SFTK_MAX_USER_SLOT_ID;
    }
    for (i=minSlotID; i < maxSlotID; i++) {
	if (secmod_SlotIsEmpty(mod,i)) {
	    return i;
	}
    }
    PORT_SetError(SEC_ERROR_NO_SLOT_SELECTED);
    return (CK_SLOT_ID) -1;
}















PK11SlotInfo *
SECMOD_OpenNewSlot(SECMODModule *mod, const char *moduleSpec)
{
    CK_SLOT_ID slotID = 0;
    PK11SlotInfo *slot;
    char *escSpec;
    char *sendSpec;
    SECStatus rv;

    slotID = secmod_FindFreeSlot(mod);
    if (slotID == (CK_SLOT_ID) -1) {
	return NULL;
    }

    if (mod->slotCount == 0) {
	return NULL;
    }

    
    slot = PK11_ReferenceSlot(mod->slots[0]);
    if (slot == NULL) {
	return NULL;
    }

    
    escSpec = nss_doubleEscape(moduleSpec);
    if (escSpec == NULL) {
	PK11_FreeSlot(slot);
	return NULL;
    }
    sendSpec = PR_smprintf("tokens=[0x%x=<%s>]", slotID, escSpec);
    PORT_Free(escSpec);

    if (sendSpec == NULL) {
	
	PK11_FreeSlot(slot);
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }
    rv = secmod_UserDBOp(slot, CKO_NETSCAPE_NEWSLOT, sendSpec);
    PR_smprintf_free(sendSpec);
    PK11_FreeSlot(slot);
    if (rv != SECSuccess) {
	return NULL;
    }

    return SECMOD_FindSlotByID(mod, slotID);
}


















































PK11SlotInfo *
SECMOD_OpenUserDB(const char *moduleSpec)
{
    SECMODModule *mod;

    if (moduleSpec == NULL) {
	return NULL;
    }

    

    mod = SECMOD_GetInternalModule();
    if (!mod) {
	
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return NULL;
    }
    return SECMOD_OpenNewSlot(mod, moduleSpec);
}








SECStatus
SECMOD_CloseUserDB(PK11SlotInfo *slot)
{
    SECStatus rv;
    char *sendSpec;
    
    sendSpec = PR_smprintf("tokens=[0x%x=<>]", slot->slotID);
    if (sendSpec == NULL) {
	
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    rv = secmod_UserDBOp(slot, CKO_NETSCAPE_DELSLOT, sendSpec);
    PR_smprintf_free(sendSpec);
    return rv;
}
