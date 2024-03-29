





#include "seccomon.h"
#include "secmod.h"
#include "nssilock.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pkcs11t.h"
#include "pk11func.h"
#include "secitem.h"
#include "secerr.h"

#include "dev.h" 
#include "dev3hack.h" 
#include "pkim.h"
#include "utilpars.h"











const PK11DefaultArrayEntry PK11_DefaultArray[] = {
	{ "RSA", SECMOD_RSA_FLAG, CKM_RSA_PKCS },
	{ "DSA", SECMOD_DSA_FLAG, CKM_DSA },
	{ "ECC", SECMOD_ECC_FLAG, CKM_ECDSA },
	{ "DH", SECMOD_DH_FLAG, CKM_DH_PKCS_DERIVE },
	{ "RC2", SECMOD_RC2_FLAG, CKM_RC2_CBC },
	{ "RC4", SECMOD_RC4_FLAG, CKM_RC4 },
	{ "DES", SECMOD_DES_FLAG, CKM_DES_CBC },
	{ "AES", SECMOD_AES_FLAG, CKM_AES_CBC },
	{ "Camellia", SECMOD_CAMELLIA_FLAG, CKM_CAMELLIA_CBC },
	{ "SEED", SECMOD_SEED_FLAG, CKM_SEED_CBC },
	{ "RC5", SECMOD_RC5_FLAG, CKM_RC5_CBC },
	{ "SHA-1", SECMOD_SHA1_FLAG, CKM_SHA_1 },

	{ "SHA256", SECMOD_SHA256_FLAG, CKM_SHA256 },

	{ "SHA512", SECMOD_SHA512_FLAG, CKM_SHA512 },
	{ "MD5", SECMOD_MD5_FLAG, CKM_MD5 },
	{ "MD2", SECMOD_MD2_FLAG, CKM_MD2 },
	{ "SSL", SECMOD_SSL_FLAG, CKM_SSL3_PRE_MASTER_KEY_GEN },
	{ "TLS", SECMOD_TLS_FLAG, CKM_TLS_MASTER_KEY_DERIVE },
	{ "SKIPJACK", SECMOD_FORTEZZA_FLAG, CKM_SKIPJACK_CBC64 },
	{ "Publicly-readable certs", SECMOD_FRIENDLY_FLAG, CKM_INVALID_MECHANISM },
	{ "Random Num Generator", SECMOD_RANDOM_FLAG, CKM_FAKE_RANDOM },
};
const int num_pk11_default_mechanisms = 
                sizeof(PK11_DefaultArray) / sizeof(PK11_DefaultArray[0]);

const PK11DefaultArrayEntry *
PK11_GetDefaultArray(int *size)
{
    if (size) {
	*size = num_pk11_default_mechanisms;
    }
    return PK11_DefaultArray;
}





static PK11SlotList 
    pk11_seedSlotList,
    pk11_camelliaSlotList,
    pk11_aesSlotList,
    pk11_desSlotList,
    pk11_rc4SlotList,
    pk11_rc2SlotList,
    pk11_rc5SlotList,
    pk11_sha1SlotList,
    pk11_md5SlotList,
    pk11_md2SlotList,
    pk11_rsaSlotList,
    pk11_dsaSlotList,
    pk11_dhSlotList,
    pk11_ecSlotList,
    pk11_ideaSlotList,
    pk11_sslSlotList,
    pk11_tlsSlotList,
    pk11_randomSlotList,
    pk11_sha256SlotList,
    pk11_sha512SlotList;	








PK11SlotList *
PK11_NewSlotList(void)
{
    PK11SlotList *list;
 
    list = (PK11SlotList *)PORT_Alloc(sizeof(PK11SlotList));
    if (list == NULL) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->lock = PZ_NewLock(nssILockList);
    if (list->lock == NULL) {
	PORT_Free(list);
	return NULL;
    }

    return list;
}




SECStatus
PK11_FreeSlotListElement(PK11SlotList *list, PK11SlotListElement *le)
{
    PRBool freeit = PR_FALSE;

    if (list == NULL || le == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    PZ_Lock(list->lock);
    if (le->refCount-- == 1) {
	freeit = PR_TRUE;
    }
    PZ_Unlock(list->lock);
    if (freeit) {
    	PK11_FreeSlot(le->slot);
	PORT_Free(le);
    }
    return SECSuccess;
}

static void
pk11_FreeSlotListStatic(PK11SlotList *list)
{
    PK11SlotListElement *le, *next ;
    if (list == NULL) return;

    for (le = list->head ; le; le = next) {
	next = le->next;
	PK11_FreeSlotListElement(list,le);
    }
    if (list->lock) {
    	PZ_DestroyLock(list->lock);
    }
    list->lock = NULL;
    list->head = NULL;
}





void
PK11_FreeSlotList(PK11SlotList *list)
{
    pk11_FreeSlotListStatic(list);
    PORT_Free(list);
}








SECStatus
PK11_AddSlotToList(PK11SlotList *list,PK11SlotInfo *slot, PRBool sorted)
{
    PK11SlotListElement *le;
    PK11SlotListElement *element;

    le = (PK11SlotListElement *) PORT_Alloc(sizeof(PK11SlotListElement));
    if (le == NULL) return SECFailure;

    le->slot = PK11_ReferenceSlot(slot);
    le->prev = NULL;
    le->refCount = 1;
    PZ_Lock(list->lock);
    element = list->head;
    
    while (element && sorted && (element->slot->module->cipherOrder >
                                 le->slot->module->cipherOrder)) {
        element = element->next;
    }
    if (element) {
        le->prev = element->prev;
        element->prev = le;
        le->next = element;
    } else {
        le->prev = list->tail;
        le->next = NULL;
        list->tail = le;
    }
    if (le->prev) le->prev->next = le;
    if (list->head == element) list->head = le;
    PZ_Unlock(list->lock);

    return SECSuccess;
}




SECStatus
PK11_DeleteSlotFromList(PK11SlotList *list,PK11SlotListElement *le)
{
    PZ_Lock(list->lock);
    if (le->prev) le->prev->next = le->next; else list->head = le->next;
    if (le->next) le->next->prev = le->prev; else list->tail = le->prev;
    le->next = le->prev = NULL;
    PZ_Unlock(list->lock);
    PK11_FreeSlotListElement(list,le);
    return SECSuccess;
}






SECStatus
pk11_MoveListToList(PK11SlotList *target,PK11SlotList *src)
{
    if (src->head == NULL) return SECSuccess;

    if (target->tail == NULL) {
	target->head = src->head;
    } else {
	target->tail->next = src->head;
    }
    src->head->prev = target->tail;
    target->tail = src->tail;
    src->head = src->tail = NULL;
    return SECSuccess;
}




PK11SlotListElement *
PK11_GetFirstRef(PK11SlotList *list)
{
    PK11SlotListElement *le;

    le = list->head;
    if (le != NULL) (le)->refCount++;
    return le;
}




PK11SlotListElement *
PK11_GetNextRef(PK11SlotList *list, PK11SlotListElement *le, PRBool restart)
{
    PK11SlotListElement *new_le;
    new_le = le->next;
    if (new_le) new_le->refCount++;
    PK11_FreeSlotListElement(list,le);
    return new_le;
}





PK11SlotListElement *
PK11_GetFirstSafe(PK11SlotList *list)
{
    PK11SlotListElement *le;

    PZ_Lock(list->lock);
    le = list->head;
    if (le != NULL) (le)->refCount++;
    PZ_Unlock(list->lock);
    return le;
}






PK11SlotListElement *
PK11_GetNextSafe(PK11SlotList *list, PK11SlotListElement *le, PRBool restart)
{
    PK11SlotListElement *new_le;
    PZ_Lock(list->lock);
    new_le = le->next;
    if (le->next == NULL) {
	


	if ((le->prev == NULL) && restart &&  (list->head != le)) {
	    new_le = list->head;
	}
    }
    if (new_le) new_le->refCount++;
    PZ_Unlock(list->lock);
    PK11_FreeSlotListElement(list,le);
    return new_le;
}





PK11SlotListElement *
PK11_FindSlotElement(PK11SlotList *list,PK11SlotInfo *slot)
{
    PK11SlotListElement *le;

    for (le = PK11_GetFirstSafe(list); le;
			 	le = PK11_GetNextSafe(list,le,PR_TRUE)) {
	if (le->slot == slot) return le;
    }
    return NULL;
}







PK11SlotInfo *
PK11_NewSlotInfo(SECMODModule *mod)
{
    PK11SlotInfo *slot;

    slot = (PK11SlotInfo *)PORT_Alloc(sizeof(PK11SlotInfo));
    if (slot == NULL) return slot;

    slot->sessionLock = mod->isThreadSafe ?
	PZ_NewLock(nssILockSession) : mod->refLock;
    if (slot->sessionLock == NULL) {
	PORT_Free(slot);
	return NULL;
    }
    slot->freeListLock = PZ_NewLock(nssILockFreelist);
    if (slot->freeListLock == NULL) {
	if (mod->isThreadSafe) {
	    PZ_DestroyLock(slot->sessionLock);
	}
	PORT_Free(slot);
	return NULL;
    }
    slot->freeSymKeysWithSessionHead = NULL;
    slot->freeSymKeysHead = NULL;
    slot->keyCount = 0;
    slot->maxKeyCount = 0;
    slot->functionList = NULL;
    slot->needTest = PR_TRUE;
    slot->isPerm = PR_FALSE;
    slot->isHW = PR_FALSE;
    slot->isInternal = PR_FALSE;
    slot->isThreadSafe = PR_FALSE;
    slot->disabled = PR_FALSE;
    slot->series = 1;
    slot->wrapKey = 0;
    slot->wrapMechanism = CKM_INVALID_MECHANISM;
    slot->refKeys[0] = CK_INVALID_HANDLE;
    slot->reason = PK11_DIS_NONE;
    slot->readOnly = PR_TRUE;
    slot->needLogin = PR_FALSE;
    slot->hasRandom = PR_FALSE;
    slot->defRWSession = PR_FALSE;
    slot->protectedAuthPath = PR_FALSE;
    slot->flags = 0;
    slot->session = CK_INVALID_SESSION;
    slot->slotID = 0;
    slot->defaultFlags = 0;
    slot->refCount = 1;
    slot->askpw = 0;
    slot->timeout = 0;
    slot->mechanismList = NULL;
    slot->mechanismCount = 0;
    slot->cert_array = NULL;
    slot->cert_count = 0;
    slot->slot_name[0] = 0;
    slot->token_name[0] = 0;
    PORT_Memset(slot->serial,' ',sizeof(slot->serial));
    slot->module = NULL;
    slot->authTransact = 0;
    slot->authTime = LL_ZERO;
    slot->minPassword = 0;
    slot->maxPassword = 0;
    slot->hasRootCerts = PR_FALSE;
    slot->nssToken = NULL;
    return slot;
}
    

PK11SlotInfo *
PK11_ReferenceSlot(PK11SlotInfo *slot)
{
    PR_ATOMIC_INCREMENT(&slot->refCount);
    return slot;
}


void
PK11_DestroySlot(PK11SlotInfo *slot)
{
   
   PK11_CleanKeyList(slot);
   
   
   if (slot->functionList) {
	PK11_GETTAB(slot)->C_CloseAllSessions(slot->slotID);
   }

   if (slot->mechanismList) {
	PORT_Free(slot->mechanismList);
   }
   if (slot->isThreadSafe && slot->sessionLock) {
	PZ_DestroyLock(slot->sessionLock);
   }
   slot->sessionLock = NULL;
   if (slot->freeListLock) {
	PZ_DestroyLock(slot->freeListLock);
	slot->freeListLock = NULL;
   }

   
   if (slot->module) {
	SECMOD_SlotDestroyModule(slot->module,PR_TRUE);
   }

   
   PORT_Free(slot);
}



void
PK11_FreeSlot(PK11SlotInfo *slot)
{
    if (PR_ATOMIC_DECREMENT(&slot->refCount) == 0) {
	PK11_DestroySlot(slot);
    }
}

void
PK11_EnterSlotMonitor(PK11SlotInfo *slot) {
    PZ_Lock(slot->sessionLock);
}

void
PK11_ExitSlotMonitor(PK11SlotInfo *slot) {
    PZ_Unlock(slot->sessionLock);
}




PRBool
SECMOD_HasRootCerts(void)
{
   SECMODModuleList *mlp;
   SECMODModuleList *modules;
   SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();
   int i;
   PRBool found = PR_FALSE;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return found;
    }

   
   SECMOD_GetReadLock(moduleLock);
   modules = SECMOD_GetDefaultModuleList();
   for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	for (i=0; i < mlp->module->slotCount; i++) {
	    PK11SlotInfo *tmpSlot = mlp->module->slots[i];
	    if (PK11_IsPresent(tmpSlot)) {
		if (tmpSlot->hasRootCerts) {
		    found = PR_TRUE;
		    break;
		}
	    }
	}
	if (found) break;
    }
    SECMOD_ReleaseReadLock(moduleLock);

    return found;
}




PK11SlotList *
PK11_FindSlotsByNames(const char *dllName, const char* slotName,
                        const char* tokenName, PRBool presentOnly)
{
    SECMODModuleList *mlp;
    SECMODModuleList *modules;
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();
    int i;
    PK11SlotList* slotList = NULL;
    PRUint32 slotcount = 0;
    SECStatus rv = SECSuccess;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return slotList;
    }

    slotList = PK11_NewSlotList();
    if (!slotList) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return slotList;
    }

    if ( ((NULL == dllName) || (0 == *dllName)) &&
        ((NULL == slotName) || (0 == *slotName)) &&
        ((NULL == tokenName) || (0 == *tokenName)) ) {
        
        PK11_AddSlotToList(slotList, PK11_GetInternalKeySlot(), PR_TRUE);
        return slotList;
    }

    
    SECMOD_GetReadLock(moduleLock);
    modules = SECMOD_GetDefaultModuleList();
    for (mlp = modules; mlp != NULL; mlp = mlp->next) {
        PORT_Assert(mlp->module);
        if (!mlp->module) {
            rv = SECFailure;
            break;
        }
        if ((!dllName) || (mlp->module->dllName &&
            (0 == PORT_Strcmp(mlp->module->dllName, dllName)))) {
            for (i=0; i < mlp->module->slotCount; i++) {
                PK11SlotInfo *tmpSlot = (mlp->module->slots?mlp->module->slots[i]:NULL);
                PORT_Assert(tmpSlot);
                if (!tmpSlot) {
                    rv = SECFailure;
                    break;
                }
                if ((PR_FALSE == presentOnly || PK11_IsPresent(tmpSlot)) &&
                    ( (!tokenName) || (tmpSlot->token_name &&
                    (0==PORT_Strcmp(tmpSlot->token_name, tokenName)))) &&
                    ( (!slotName) || (tmpSlot->slot_name &&
                    (0==PORT_Strcmp(tmpSlot->slot_name, slotName)))) ) {
                    if (tmpSlot) {
                        PK11_AddSlotToList(slotList, tmpSlot, PR_TRUE);
                        slotcount++;
                    }
                }
            }
        }
    }
    SECMOD_ReleaseReadLock(moduleLock);

    if ( (0 == slotcount) || (SECFailure == rv) ) {
        PORT_SetError(SEC_ERROR_NO_TOKEN);
        PK11_FreeSlotList(slotList);
        slotList = NULL;
    }

    if (SECFailure == rv) {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
    }

    return slotList;
}

PK11SlotInfo *
PK11_FindSlotByName(const char *name)
{
   SECMODModuleList *mlp;
   SECMODModuleList *modules;
   SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();
   int i;
   PK11SlotInfo *slot = NULL;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return slot;
    }
   if ((name == NULL) || (*name == 0)) {
	return PK11_GetInternalKeySlot();
   }

   
   SECMOD_GetReadLock(moduleLock);
   modules = SECMOD_GetDefaultModuleList();
   for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	for (i=0; i < mlp->module->slotCount; i++) {
	    PK11SlotInfo *tmpSlot = mlp->module->slots[i];
	    if (PK11_IsPresent(tmpSlot)) {
		if (PORT_Strcmp(tmpSlot->token_name,name) == 0) {
		    slot = PK11_ReferenceSlot(tmpSlot);
		    break;
		}
	    }
	}
	if (slot != NULL) break;
    }
    SECMOD_ReleaseReadLock(moduleLock);

    if (slot == NULL) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
    }

    return slot;
}


PK11SlotInfo *
PK11_FindSlotBySerial(char *serial)
{
   SECMODModuleList *mlp;
   SECMODModuleList *modules;
   SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();
   int i;
   PK11SlotInfo *slot = NULL;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return slot;
    }
   
   SECMOD_GetReadLock(moduleLock);
   modules = SECMOD_GetDefaultModuleList();
   for(mlp = modules; mlp != NULL; mlp = mlp->next) {
	for (i=0; i < mlp->module->slotCount; i++) {
	    PK11SlotInfo *tmpSlot = mlp->module->slots[i];
	    if (PK11_IsPresent(tmpSlot)) {
		if (PORT_Memcmp(tmpSlot->serial,serial,
					sizeof(tmpSlot->serial)) == 0) {
		    slot = PK11_ReferenceSlot(tmpSlot);
		    break;
		}
	    }
	}
	if (slot != NULL) break;
    }
    SECMOD_ReleaseReadLock(moduleLock);

    if (slot == NULL) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
    }

    return slot;
}






CK_RV pk11_notify(CK_SESSION_HANDLE session, CK_NOTIFICATION event,
							 CK_VOID_PTR pdata)
{
    return CKR_OK;
}







CK_SESSION_HANDLE PK11_GetRWSession(PK11SlotInfo *slot)
{
    CK_SESSION_HANDLE rwsession;
    CK_RV crv;
    PRBool haveMonitor = PR_FALSE;

    if (!slot->isThreadSafe || slot->defRWSession) {
    	PK11_EnterSlotMonitor(slot);
	haveMonitor = PR_TRUE;
    }
    if (slot->defRWSession) {
	PORT_Assert(slot->session != CK_INVALID_SESSION);
	if (slot->session != CK_INVALID_SESSION) 
	    return slot->session;
    }

    crv = PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
				CKF_RW_SESSION|CKF_SERIAL_SESSION,
				  	  slot, pk11_notify,&rwsession);
    PORT_Assert(rwsession != CK_INVALID_SESSION || crv != CKR_OK);
    if (crv != CKR_OK || rwsession == CK_INVALID_SESSION) {
	if (crv == CKR_OK) 
	    crv = CKR_DEVICE_ERROR;
	if (haveMonitor)
	    PK11_ExitSlotMonitor(slot);
	PORT_SetError(PK11_MapError(crv));
	return CK_INVALID_SESSION;
    }
    if (slot->defRWSession) { 
    	slot->session = rwsession;
    }
    return rwsession;
}

PRBool
PK11_RWSessionHasLock(PK11SlotInfo *slot,CK_SESSION_HANDLE session_handle) 
{
    PRBool hasLock;
    hasLock = (PRBool)(!slot->isThreadSafe || 
    	      (slot->defRWSession && slot->session != CK_INVALID_SESSION));
    return hasLock;
}

static PRBool
pk11_RWSessionIsDefault(PK11SlotInfo *slot,CK_SESSION_HANDLE rwsession)
{
    PRBool isDefault;
    isDefault = (PRBool)(slot->session == rwsession &&
    	                 slot->defRWSession && 
			 slot->session != CK_INVALID_SESSION);
    return isDefault;
}






void
PK11_RestoreROSession(PK11SlotInfo *slot,CK_SESSION_HANDLE rwsession)
{
    PORT_Assert(rwsession != CK_INVALID_SESSION);
    if (rwsession != CK_INVALID_SESSION) {
    	PRBool doExit = PK11_RWSessionHasLock(slot, rwsession);
	if (!pk11_RWSessionIsDefault(slot, rwsession))
	    PK11_GETTAB(slot)->C_CloseSession(rwsession);
	if (doExit)
	    PK11_ExitSlotMonitor(slot);
    }
}







static void
pk11_InitSlotListStatic(PK11SlotList *list)
{
    list->lock = PZ_NewLock(nssILockList);
    list->head = NULL;
}



SECStatus
PK11_InitSlotLists(void)
{
    pk11_InitSlotListStatic(&pk11_seedSlotList);
    pk11_InitSlotListStatic(&pk11_camelliaSlotList);
    pk11_InitSlotListStatic(&pk11_aesSlotList);
    pk11_InitSlotListStatic(&pk11_desSlotList);
    pk11_InitSlotListStatic(&pk11_rc4SlotList);
    pk11_InitSlotListStatic(&pk11_rc2SlotList);
    pk11_InitSlotListStatic(&pk11_rc5SlotList);
    pk11_InitSlotListStatic(&pk11_md5SlotList);
    pk11_InitSlotListStatic(&pk11_md2SlotList);
    pk11_InitSlotListStatic(&pk11_sha1SlotList);
    pk11_InitSlotListStatic(&pk11_rsaSlotList);
    pk11_InitSlotListStatic(&pk11_dsaSlotList);
    pk11_InitSlotListStatic(&pk11_dhSlotList);
    pk11_InitSlotListStatic(&pk11_ecSlotList);
    pk11_InitSlotListStatic(&pk11_ideaSlotList);
    pk11_InitSlotListStatic(&pk11_sslSlotList);
    pk11_InitSlotListStatic(&pk11_tlsSlotList);
    pk11_InitSlotListStatic(&pk11_randomSlotList);
    pk11_InitSlotListStatic(&pk11_sha256SlotList);
    pk11_InitSlotListStatic(&pk11_sha512SlotList);
    return SECSuccess;
}

void
PK11_DestroySlotLists(void)
{
    pk11_FreeSlotListStatic(&pk11_seedSlotList);
    pk11_FreeSlotListStatic(&pk11_camelliaSlotList);
    pk11_FreeSlotListStatic(&pk11_aesSlotList);
    pk11_FreeSlotListStatic(&pk11_desSlotList);
    pk11_FreeSlotListStatic(&pk11_rc4SlotList);
    pk11_FreeSlotListStatic(&pk11_rc2SlotList);
    pk11_FreeSlotListStatic(&pk11_rc5SlotList);
    pk11_FreeSlotListStatic(&pk11_md5SlotList);
    pk11_FreeSlotListStatic(&pk11_md2SlotList);
    pk11_FreeSlotListStatic(&pk11_sha1SlotList);
    pk11_FreeSlotListStatic(&pk11_rsaSlotList);
    pk11_FreeSlotListStatic(&pk11_dsaSlotList);
    pk11_FreeSlotListStatic(&pk11_dhSlotList);
    pk11_FreeSlotListStatic(&pk11_ecSlotList);
    pk11_FreeSlotListStatic(&pk11_ideaSlotList);
    pk11_FreeSlotListStatic(&pk11_sslSlotList);
    pk11_FreeSlotListStatic(&pk11_tlsSlotList);
    pk11_FreeSlotListStatic(&pk11_randomSlotList);
    pk11_FreeSlotListStatic(&pk11_sha256SlotList);
    pk11_FreeSlotListStatic(&pk11_sha512SlotList);
    return;
}


PK11SlotList *
PK11_GetSlotList(CK_MECHANISM_TYPE type)
{

#if defined(HPUX) && defined(__LP64__)
    if (CKM_INVALID_MECHANISM == type)
        return NULL;
#endif
    switch (type) {
    case CKM_SEED_CBC:
    case CKM_SEED_ECB:
	return &pk11_seedSlotList;
    case CKM_CAMELLIA_CBC:
    case CKM_CAMELLIA_ECB:
	return &pk11_camelliaSlotList;
    case CKM_AES_CBC:
    case CKM_AES_CCM:
    case CKM_AES_CTR:
    case CKM_AES_CTS:
    case CKM_AES_GCM:
    case CKM_AES_ECB:
	return &pk11_aesSlotList;
    case CKM_DES_CBC:
    case CKM_DES_ECB:
    case CKM_DES3_ECB:
    case CKM_DES3_CBC:
	return &pk11_desSlotList;
    case CKM_RC4:
	return &pk11_rc4SlotList;
    case CKM_RC5_CBC:
	return &pk11_rc5SlotList;
    case CKM_SHA_1:
	return &pk11_sha1SlotList;
    case CKM_SHA224:
    case CKM_SHA256:
	return &pk11_sha256SlotList;
    case CKM_SHA384:
    case CKM_SHA512:
	return &pk11_sha512SlotList;
    case CKM_MD5:
	return &pk11_md5SlotList;
    case CKM_MD2:
	return &pk11_md2SlotList;
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	return &pk11_rc2SlotList;
    case CKM_RSA_PKCS:
    case CKM_RSA_PKCS_KEY_PAIR_GEN:
    case CKM_RSA_X_509:
	return &pk11_rsaSlotList;
    case CKM_DSA:
	return &pk11_dsaSlotList;
    case CKM_DH_PKCS_KEY_PAIR_GEN:
    case CKM_DH_PKCS_DERIVE:
	return &pk11_dhSlotList;
    case CKM_ECDSA:
    case CKM_ECDSA_SHA1:
    case CKM_EC_KEY_PAIR_GEN: 
    case CKM_ECDH1_DERIVE:
	return &pk11_ecSlotList;
    case CKM_SSL3_PRE_MASTER_KEY_GEN:
    case CKM_SSL3_MASTER_KEY_DERIVE:
    case CKM_SSL3_SHA1_MAC:
    case CKM_SSL3_MD5_MAC:
	return &pk11_sslSlotList;
    case CKM_TLS_MASTER_KEY_DERIVE:
    case CKM_TLS_KEY_AND_MAC_DERIVE:
    case CKM_NSS_TLS_KEY_AND_MAC_DERIVE_SHA256:
	return &pk11_tlsSlotList;
    case CKM_IDEA_CBC:
    case CKM_IDEA_ECB:
	return &pk11_ideaSlotList;
    case CKM_FAKE_RANDOM:
	return &pk11_randomSlotList;
    }
    return NULL;
}






void
PK11_LoadSlotList(PK11SlotInfo *slot, PK11PreSlotInfo *psi, int count)
{
    int i;

    for (i=0; i < count; i++) {
	if (psi[i].slotID == slot->slotID)
	    break;
    }

    if (i == count) return;

    slot->defaultFlags = psi[i].defaultFlags;
    slot->askpw = psi[i].askpw;
    slot->timeout = psi[i].timeout;
    slot->hasRootCerts = psi[i].hasRootCerts;

    


    if (slot->disabled) return;

    
    if (slot->defaultFlags & PK11_DISABLE_FLAG) {
	slot->disabled = PR_TRUE;
	slot->reason = PK11_DIS_USER_SELECTED;
	
	return;
    }

    for (i=0; i < num_pk11_default_mechanisms; i++) {
	if (slot->defaultFlags & PK11_DefaultArray[i].flag) {
	    CK_MECHANISM_TYPE mechanism = PK11_DefaultArray[i].mechanism;
	    PK11SlotList *slotList = PK11_GetSlotList(mechanism);

	    if (slotList) PK11_AddSlotToList(slotList,slot,PR_FALSE);
	}
    }

    return;
}






SECStatus
PK11_UpdateSlotAttribute(PK11SlotInfo *slot,
                         const PK11DefaultArrayEntry *entry,
                         PRBool add)
                         
{
    SECStatus result = SECSuccess;
    PK11SlotList *slotList = PK11_GetSlotList(entry->mechanism);

    if (add) { 
                 
        
        slot->defaultFlags |= entry->flag;
        
        
        if (slotList!=NULL)
            result = PK11_AddSlotToList(slotList, slot, PR_FALSE);
        
    } else { 
            
         
        slot->defaultFlags &= ~entry->flag;
        
        if (slotList) {
            
            PK11SlotListElement *le = PK11_FindSlotElement(slotList, slot);

            
            if (le)
                result = PK11_DeleteSlotFromList(slotList, le);
        }
    }
    return result;
}




void
PK11_ClearSlotList(PK11SlotInfo *slot)
{
    int i;

    if (slot->disabled) return;
    if (slot->defaultFlags == 0) return;

    for (i=0; i < num_pk11_default_mechanisms; i++) {
	if (slot->defaultFlags & PK11_DefaultArray[i].flag) {
	    CK_MECHANISM_TYPE mechanism = PK11_DefaultArray[i].mechanism;
	    PK11SlotList *slotList = PK11_GetSlotList(mechanism);
	    PK11SlotListElement *le = NULL;

	    if (slotList) le = PK11_FindSlotElement(slotList,slot);

	    if (le) {
		PK11_DeleteSlotFromList(slotList,le);
		PK11_FreeSlotListElement(slotList,le);
	    }
	}
    }
}








char *
PK11_MakeString(PLArenaPool *arena,char *space,
					char *staticString,int stringLen)
{
	int i;
	char *newString;
	for(i=(stringLen-1); i >= 0; i--) {
	  if (staticString[i] != ' ') break;
	}
	
	i++;
	if (arena) {
	    newString = (char*)PORT_ArenaAlloc(arena,i+1 );
	} else if (space) {
	    newString = space;
	} else {
	    newString = (char*)PORT_Alloc(i+1 );
	}
	if (newString == NULL) return NULL;

	if (i) PORT_Memcpy(newString,staticString, i);
	newString[i] = 0;

	return newString;
}




SECStatus
PK11_ReadMechanismList(PK11SlotInfo *slot)
{
    CK_ULONG count;
    CK_RV crv;
    PRUint32 i;

    if (slot->mechanismList) {
	PORT_Free(slot->mechanismList);
	slot->mechanismList = NULL;
    }
    slot->mechanismCount = 0;

    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_GetMechanismList(slot->slotID,NULL,&count);
    if (crv != CKR_OK) {
	if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }

    slot->mechanismList = (CK_MECHANISM_TYPE *)
			    PORT_Alloc(count *sizeof(CK_MECHANISM_TYPE));
    if (slot->mechanismList == NULL) {
	if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	return SECFailure;
    }
    crv = PK11_GETTAB(slot)->C_GetMechanismList(slot->slotID,
						slot->mechanismList, &count);
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_Free(slot->mechanismList);
	slot->mechanismList = NULL;
	PORT_SetError(PK11_MapError(crv));
	return SECSuccess;
    }
    slot->mechanismCount = count;
    PORT_Memset(slot->mechanismBits, 0, sizeof(slot->mechanismBits));

    for (i=0; i < count; i++) {
	CK_MECHANISM_TYPE mech = slot->mechanismList[i];
	if (mech < 0x7ff) {
	    slot->mechanismBits[mech & 0xff] |= 1 << (mech >> 8);
	}
    }
    return SECSuccess;
}







SECStatus
PK11_InitToken(PK11SlotInfo *slot, PRBool loadCerts)
{
    CK_TOKEN_INFO tokenInfo;
    CK_RV crv;
    char *tmp;
    SECStatus rv;
    PRStatus status;

    
    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_GetTokenInfo(slot->slotID,&tokenInfo);
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }

    
    slot->series++; 

    slot->flags = tokenInfo.flags;
    slot->needLogin = ((tokenInfo.flags & CKF_LOGIN_REQUIRED) ? 
							PR_TRUE : PR_FALSE);
    slot->readOnly = ((tokenInfo.flags & CKF_WRITE_PROTECTED) ? 
							PR_TRUE : PR_FALSE);
	
	 
    slot->hasRandom = ((tokenInfo.flags & CKF_RNG) ? PR_TRUE : PR_FALSE);
    slot->protectedAuthPath =
    		((tokenInfo.flags & CKF_PROTECTED_AUTHENTICATION_PATH) 
	 						? PR_TRUE : PR_FALSE);
    slot->lastLoginCheck = 0;
    slot->lastState = 0;
    

    if (slot->isActiveCard) {
	slot->protectedAuthPath = PR_FALSE;
    }
    tmp = PK11_MakeString(NULL,slot->token_name,
			(char *)tokenInfo.label, sizeof(tokenInfo.label));
    slot->minPassword = tokenInfo.ulMinPinLen;
    slot->maxPassword = tokenInfo.ulMaxPinLen;
    PORT_Memcpy(slot->serial,tokenInfo.serialNumber,sizeof(slot->serial));

    nssToken_UpdateName(slot->nssToken);

    slot->defRWSession = (PRBool)((!slot->readOnly) && 
					(tokenInfo.ulMaxSessionCount == 1));
    rv = PK11_ReadMechanismList(slot);
    if (rv != SECSuccess) return rv;

    slot->hasRSAInfo = PR_FALSE;
    slot->RSAInfoFlags = 0;

    
    if (tokenInfo.ulMaxSessionCount == 0) {
	slot->maxKeyCount = 800; 
    } else if (tokenInfo.ulMaxSessionCount < 20) {
	
	slot->maxKeyCount = 0;
    } else {
	slot->maxKeyCount = tokenInfo.ulMaxSessionCount/2;
    }

    
    if (slot->session == CK_INVALID_SESSION) {
	
	CK_SESSION_HANDLE session;

	
	if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
	crv = PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
	      (slot->defRWSession ? CKF_RW_SESSION : 0) | CKF_SERIAL_SESSION,
				  slot,pk11_notify,&session);
	if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	if (crv != CKR_OK) {
	    PORT_SetError(PK11_MapError(crv));
	    return SECFailure;
	}
	slot->session = session;
    } else {
	

	CK_SESSION_INFO sessionInfo;

	if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
	crv = PK11_GETTAB(slot)->C_GetSessionInfo(slot->session,&sessionInfo);
        if (crv == CKR_DEVICE_ERROR) {
	    PK11_GETTAB(slot)->C_CloseSession(slot->session);
	    crv = CKR_SESSION_CLOSED;
	}
	if ((crv==CKR_SESSION_CLOSED) || (crv==CKR_SESSION_HANDLE_INVALID)) {
	    crv =PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
	      (slot->defRWSession ? CKF_RW_SESSION : 0) | CKF_SERIAL_SESSION,
					slot,pk11_notify,&slot->session);
	    if (crv != CKR_OK) {
	        PORT_SetError(PK11_MapError(crv));
		slot->session = CK_INVALID_SESSION;
		if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
		return SECFailure;
	    }
	}
	if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    }

    status = nssToken_Refresh(slot->nssToken);
    if (status != PR_SUCCESS)
    	return SECFailure;

    if (!(slot->isInternal) && (slot->hasRandom)) {
	

	PK11SlotInfo *int_slot = PK11_GetInternalSlot();

	if (int_slot) {
	    unsigned char random_bytes[32];

	    


	    PK11_EnterSlotMonitor(slot);
	    crv = PK11_GETTAB(slot)->C_GenerateRandom
			(slot->session,random_bytes, sizeof(random_bytes));
	    PK11_ExitSlotMonitor(slot);
	    if (crv == CKR_OK) {
	        PK11_EnterSlotMonitor(int_slot);
		PK11_GETTAB(int_slot)->C_SeedRandom(int_slot->session,
					random_bytes, sizeof(random_bytes));
	        PK11_ExitSlotMonitor(int_slot);
	    }

	    

	    PK11_EnterSlotMonitor(int_slot);
	    crv = PK11_GETTAB(int_slot)->C_GenerateRandom(int_slot->session,
					random_bytes, sizeof(random_bytes));
	    PK11_ExitSlotMonitor(int_slot);
	    if (crv == CKR_OK) {
	        PK11_EnterSlotMonitor(slot);
		crv = PK11_GETTAB(slot)->C_SeedRandom(slot->session,
					random_bytes, sizeof(random_bytes));
	        PK11_ExitSlotMonitor(slot);
	    }
	    PK11_FreeSlot(int_slot);
	}
    }
    

    if (slot->isInternal && !slot->readOnly) {
	CK_SESSION_HANDLE session = CK_INVALID_SESSION;

	
	crv =PK11_GETTAB(slot)->C_OpenSession(slot->slotID,
	      CKF_RW_SESSION|CKF_SERIAL_SESSION, slot, pk11_notify ,&session);
	

	if (crv == CKR_TOKEN_WRITE_PROTECTED) {
	    slot->readOnly = PR_TRUE;
	} else if (crv == CKR_OK) {
	    CK_SESSION_INFO sessionInfo;

	    

	    crv = PK11_GETTAB(slot)->C_GetSessionInfo(session, &sessionInfo);
	    if (crv == CKR_OK) {
		if ((sessionInfo.flags & CKF_RW_SESSION) == 0) {
		    
		    slot->readOnly = PR_TRUE;
		}
	    }
	    PK11_GETTAB(slot)->C_CloseSession(session);
	}
    }
	
    return SECSuccess;
}







SECStatus
PK11_TokenRefresh(PK11SlotInfo *slot)
{
    CK_TOKEN_INFO tokenInfo;
    CK_RV crv;

    
    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_GetTokenInfo(slot->slotID,&tokenInfo);
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }

    slot->flags = tokenInfo.flags;
    slot->needLogin = ((tokenInfo.flags & CKF_LOGIN_REQUIRED) ? 
							PR_TRUE : PR_FALSE);
    slot->readOnly = ((tokenInfo.flags & CKF_WRITE_PROTECTED) ? 
							PR_TRUE : PR_FALSE);
    slot->hasRandom = ((tokenInfo.flags & CKF_RNG) ? PR_TRUE : PR_FALSE);
    slot->protectedAuthPath =
    		((tokenInfo.flags & CKF_PROTECTED_AUTHENTICATION_PATH) 
	 						? PR_TRUE : PR_FALSE);
    

    if (slot->isActiveCard) {
	slot->protectedAuthPath = PR_FALSE;
    }
    return SECSuccess;
}

static PRBool
pk11_isRootSlot(PK11SlotInfo *slot) 
{
    CK_ATTRIBUTE findTemp[1];
    CK_ATTRIBUTE *attrs;
    CK_OBJECT_CLASS oclass = CKO_NETSCAPE_BUILTIN_ROOT_LIST;
    int tsize;
    CK_OBJECT_HANDLE handle;

    attrs = findTemp;
    PK11_SETATTRS(attrs, CKA_CLASS, &oclass, sizeof(oclass)); attrs++;
    tsize = attrs - findTemp;
    PORT_Assert(tsize <= sizeof(findTemp)/sizeof(CK_ATTRIBUTE));

    handle = pk11_FindObjectByTemplate(slot,findTemp,tsize);
    if (handle == CK_INVALID_HANDLE) {
	return PR_FALSE;
    }
    return PR_TRUE;
}









void
PK11_InitSlot(SECMODModule *mod, CK_SLOT_ID slotID, PK11SlotInfo *slot)
{
    SECStatus rv;
    char *tmp;
    CK_SLOT_INFO slotInfo;

    slot->functionList = mod->functionList;
    slot->isInternal = mod->internal;
    slot->slotID = slotID;
    slot->isThreadSafe = mod->isThreadSafe;
    slot->hasRSAInfo = PR_FALSE;
    
    if (PK11_GETTAB(slot)->C_GetSlotInfo(slotID,&slotInfo) != CKR_OK) {
	slot->disabled = PR_TRUE;
	slot->reason = PK11_DIS_COULD_NOT_INIT_TOKEN;
	return;
    }

    
    slot->needTest = mod->internal ? PR_FALSE : PR_TRUE;
    slot->module = mod; 




    tmp = PK11_MakeString(NULL,slot->slot_name,
	 (char *)slotInfo.slotDescription, sizeof(slotInfo.slotDescription));
    slot->isHW = (PRBool)((slotInfo.flags & CKF_HW_SLOT) == CKF_HW_SLOT);
#define ACTIVE_CARD "ActivCard SA"
    slot->isActiveCard = (PRBool)(PORT_Strncmp((char *)slotInfo.manufacturerID,
				ACTIVE_CARD, sizeof(ACTIVE_CARD)-1) == 0);
    if ((slotInfo.flags & CKF_REMOVABLE_DEVICE) == 0) {
	slot->isPerm = PR_TRUE;
	
	if ((slotInfo.flags & CKF_TOKEN_PRESENT) == 0) {
	    slot->disabled = PR_TRUE;
	    slot->reason = PK11_DIS_TOKEN_NOT_PRESENT;
	    return; 
	}
    }
    
    if ((slotInfo.flags & CKF_TOKEN_PRESENT) != 0) {
	rv = PK11_InitToken(slot,PR_TRUE);
	


	if ((rv != SECSuccess) && (slot->isPerm) && (!slot->disabled)) {
	    slot->disabled = PR_TRUE;
	    slot->reason = PK11_DIS_COULD_NOT_INIT_TOKEN;
	}
	if (rv == SECSuccess && pk11_isRootSlot(slot)) {
	    if (!slot->hasRootCerts) {
		slot->module->trustOrder = 100;
	    }
	    slot->hasRootCerts= PR_TRUE;
	}
    }
}

	










static PRBool
pk11_IsPresentCertLoad(PK11SlotInfo *slot, PRBool loadCerts)
{
    CK_SLOT_INFO slotInfo;
    CK_SESSION_INFO sessionInfo;
    CK_RV crv;

    
    if (slot->disabled) {
	return PR_FALSE;
    }

    
    if (slot->isPerm && (slot->session != CK_INVALID_SESSION)) {
	return PR_TRUE;
    }

    if (slot->nssToken) {
	return nssToken_IsPresent(slot->nssToken);
    }

    
    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    if (PK11_GETTAB(slot)->C_GetSlotInfo(slot->slotID,&slotInfo) != CKR_OK) {
        if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	return PR_FALSE;
    }
    if ((slotInfo.flags & CKF_TOKEN_PRESENT) == 0) {
	
	if (slot->session != CK_INVALID_SESSION) {
	    PK11_GETTAB(slot)->C_CloseSession(slot->session);
	    slot->session = CK_INVALID_SESSION;
	}
        if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	return PR_FALSE;
    }

    

    if (slot->session != CK_INVALID_SESSION) {
	if (slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
	crv = PK11_GETTAB(slot)->C_GetSessionInfo(slot->session, &sessionInfo);
	if (crv != CKR_OK) {
	    PK11_GETTAB(slot)->C_CloseSession(slot->session);
	    slot->session = CK_INVALID_SESSION;
	}
        if (slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    }
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);

    
    if (slot->session != CK_INVALID_SESSION) return PR_TRUE;

    
    if (PK11_InitToken(slot,loadCerts) != SECSuccess) {
	return PR_FALSE;
    }

    return PR_TRUE;
}




PRBool
PK11_IsPresent(PK11SlotInfo *slot) {
   return pk11_IsPresentCertLoad(slot,PR_TRUE);
}


PRBool
PK11_IsDisabled(PK11SlotInfo *slot)
{
    return slot->disabled;
}


PK11DisableReasons
PK11_GetDisabledReason(PK11SlotInfo *slot)
{
    return slot->reason;
}



PRBool PK11_UserDisableSlot(PK11SlotInfo *slot) {

    
    if (slot->isInternal) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return PR_FALSE;
    }

    slot->defaultFlags |= PK11_DISABLE_FLAG;
    slot->disabled = PR_TRUE;
    slot->reason = PK11_DIS_USER_SELECTED;
    
    return PR_TRUE;
}

PRBool PK11_UserEnableSlot(PK11SlotInfo *slot) {

    slot->defaultFlags &= ~PK11_DISABLE_FLAG;
    slot->disabled = PR_FALSE;
    slot->reason = PK11_DIS_NONE;
    return PR_TRUE;
}

PRBool PK11_HasRootCerts(PK11SlotInfo *slot) {
    return slot->hasRootCerts;
}


SECMODModule *
PK11_GetModule(PK11SlotInfo *slot)
{
	return slot->module;
}


unsigned long
PK11_GetDefaultFlags(PK11SlotInfo *slot)
{
	return slot->defaultFlags;
}




PRBool
PK11_IsReadOnly(PK11SlotInfo *slot)
{
    return slot->readOnly;
}

PRBool
PK11_IsHW(PK11SlotInfo *slot)
{
    return slot->isHW;
}

PRBool
PK11_IsRemovable(PK11SlotInfo *slot)
{
    return !slot->isPerm;
}

PRBool
PK11_IsInternal(PK11SlotInfo *slot)
{
    return slot->isInternal;
}

PRBool
PK11_IsInternalKeySlot(PK11SlotInfo *slot)
{
    PK11SlotInfo *int_slot;
    PRBool result;

    if (!slot->isInternal) {
	return PR_FALSE;
    }

    int_slot = PK11_GetInternalKeySlot();
    result = (int_slot == slot) ? PR_TRUE : PR_FALSE;
    PK11_FreeSlot(int_slot);
    return result;
}

PRBool
PK11_NeedLogin(PK11SlotInfo *slot)
{
    return slot->needLogin;
}

PRBool
PK11_IsFriendly(PK11SlotInfo *slot)
{
    
    return (PRBool)(slot->isInternal || 
		    ((slot->defaultFlags & SECMOD_FRIENDLY_FLAG) == 
		     SECMOD_FRIENDLY_FLAG));
}

char *
PK11_GetTokenName(PK11SlotInfo *slot)
{
     return slot->token_name;
}

char *
PK11_GetSlotName(PK11SlotInfo *slot)
{
     return slot->slot_name;
}

int
PK11_GetSlotSeries(PK11SlotInfo *slot)
{
    return slot->series;
}

int
PK11_GetCurrentWrapIndex(PK11SlotInfo *slot)
{
    return slot->wrapKey;
}

CK_SLOT_ID
PK11_GetSlotID(PK11SlotInfo *slot)
{
    return slot->slotID;
}

SECMODModuleID
PK11_GetModuleID(PK11SlotInfo *slot)
{
    return slot->module->moduleID;
}

static void
pk11_zeroTerminatedToBlankPadded(CK_CHAR *buffer, size_t buffer_size)
{
    CK_CHAR *walk = buffer;
    CK_CHAR *end = buffer + buffer_size;

    
    while (walk < end && *walk != '\0') {
	walk++;
    }

    
    while (walk < end) {
	*walk++ = ' ';
    }
}


SECStatus
PK11_GetSlotInfo(PK11SlotInfo *slot, CK_SLOT_INFO *info)
{
    CK_RV crv;

    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    



    PORT_Memset(info->slotDescription,' ',sizeof(info->slotDescription));
    PORT_Memset(info->manufacturerID,' ',sizeof(info->manufacturerID));
    crv = PK11_GETTAB(slot)->C_GetSlotInfo(slot->slotID,info);
    pk11_zeroTerminatedToBlankPadded(info->slotDescription,
					sizeof(info->slotDescription));
    pk11_zeroTerminatedToBlankPadded(info->manufacturerID,
					sizeof(info->manufacturerID));
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    return SECSuccess;
}


SECStatus
PK11_GetTokenInfo(PK11SlotInfo *slot, CK_TOKEN_INFO *info)
{
    CK_RV crv;
    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    



    PORT_Memset(info->label,' ',sizeof(info->label));
    PORT_Memset(info->manufacturerID,' ',sizeof(info->manufacturerID));
    PORT_Memset(info->model,' ',sizeof(info->model));
    PORT_Memset(info->serialNumber,' ',sizeof(info->serialNumber));
    crv = PK11_GETTAB(slot)->C_GetTokenInfo(slot->slotID,info);
    pk11_zeroTerminatedToBlankPadded(info->label,sizeof(info->label));
    pk11_zeroTerminatedToBlankPadded(info->manufacturerID,
					sizeof(info->manufacturerID));
    pk11_zeroTerminatedToBlankPadded(info->model,sizeof(info->model));
    pk11_zeroTerminatedToBlankPadded(info->serialNumber,
					sizeof(info->serialNumber));
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    return SECSuccess;
}


PRBool
PK11_NeedUserInit(PK11SlotInfo *slot)
{
    PRBool needUserInit = (PRBool) ((slot->flags & CKF_USER_PIN_INITIALIZED) 
					== 0);

    if (needUserInit) {
	CK_TOKEN_INFO info;
	SECStatus rv;

	
	rv = PK11_GetTokenInfo(slot, &info);
	if (rv == SECSuccess) {
	    slot->flags = info.flags;
	}
    }
    return (PRBool)((slot->flags & CKF_USER_PIN_INITIALIZED) == 0);
}

static PK11SlotInfo *pk11InternalKeySlot = NULL;






void
pk11_SetInternalKeySlot(PK11SlotInfo *slot)
{
   if (pk11InternalKeySlot) {
	PK11_FreeSlot(pk11InternalKeySlot);
   }
   pk11InternalKeySlot = slot ? PK11_ReferenceSlot(slot) : NULL;
}






void
pk11_SetInternalKeySlotIfFirst(PK11SlotInfo *slot)
{
   if (pk11InternalKeySlot) {
	return;
   }
   pk11InternalKeySlot = slot ? PK11_ReferenceSlot(slot) : NULL;
}




PK11SlotInfo *
pk11_SwapInternalKeySlot(PK11SlotInfo *slot)
{
   PK11SlotInfo *swap = pk11InternalKeySlot;

   pk11InternalKeySlot = slot ? PK11_ReferenceSlot(slot) : NULL;
   return swap;
}




PK11SlotInfo *
PK11_GetInternalKeySlot(void)
{
    SECMODModule *mod;

    if (pk11InternalKeySlot) {
	return PK11_ReferenceSlot(pk11InternalKeySlot);
    }

    mod = SECMOD_GetInternalModule();
    PORT_Assert(mod != NULL);
    if (!mod) {
	PORT_SetError( SEC_ERROR_NO_MODULE );
	return NULL;
    }
    return PK11_ReferenceSlot(mod->isFIPS ? mod->slots[0] : mod->slots[1]);
}


PK11SlotInfo *
PK11_GetInternalSlot(void) 
{
    SECMODModule * mod = SECMOD_GetInternalModule();
    PORT_Assert(mod != NULL);
    if (!mod) {
	PORT_SetError( SEC_ERROR_NO_MODULE );
	return NULL;
    }
    if (mod->isFIPS) {
	return PK11_GetInternalKeySlot();
    }
    return PK11_ReferenceSlot(mod->slots[0]);
}




PRBool
PK11_DoesMechanism(PK11SlotInfo *slot, CK_MECHANISM_TYPE type)
{
    int i;

    


    if (type == CKM_FAKE_RANDOM) {
	return slot->hasRandom;
    }

    
    if (type < 0x7ff) {
	return (slot->mechanismBits[type & 0xff] & (1 << (type >> 8)))  ?
		PR_TRUE : PR_FALSE;
    }
	   
    for (i=0; i < (int) slot->mechanismCount; i++) {
	if (slot->mechanismList[i] == type) return PR_TRUE;
    }
    return PR_FALSE;
}






PRBool
PK11_TokenExists(CK_MECHANISM_TYPE type)
{
    SECMODModuleList *mlp;
    SECMODModuleList *modules;
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();
    PK11SlotInfo *slot;
    PRBool found = PR_FALSE;
    int i;

    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return found;
    }
    


    slot = PK11_GetInternalSlot();
    if (slot) {
    	found = PK11_DoesMechanism(slot,type);
	PK11_FreeSlot(slot);
    }
    if (found) return PR_TRUE; 

    SECMOD_GetReadLock(moduleLock);
    modules = SECMOD_GetDefaultModuleList();
    for(mlp = modules; mlp != NULL && (!found); mlp = mlp->next) {
	for (i=0; i < mlp->module->slotCount; i++) {
	    slot = mlp->module->slots[i];
	    if (PK11_IsPresent(slot)) {
		if (PK11_DoesMechanism(slot,type)) {
		    found = PR_TRUE;
		    break;
		}
	    }
	}
    }
    SECMOD_ReleaseReadLock(moduleLock);
    return found;
}







PK11SlotList *
PK11_GetAllTokens(CK_MECHANISM_TYPE type, PRBool needRW, PRBool loadCerts, 
                  void *wincx)
{
    PK11SlotList *     list;
    PK11SlotList *     loginList;
    PK11SlotList *     friendlyList;
    SECMODModuleList * mlp;
    SECMODModuleList * modules;
    SECMODListLock *   moduleLock;
    int                i;
#if defined( XP_WIN32 ) 
    int                j            = 0;
    PRInt32            waste[16];
#endif

    moduleLock   = SECMOD_GetDefaultModuleListLock();
    if (!moduleLock) {
    	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return NULL;
    }

    list         = PK11_NewSlotList();
    loginList    = PK11_NewSlotList();
    friendlyList = PK11_NewSlotList();
    if ((list == NULL)  || (loginList == NULL) || (friendlyList == NULL)) {
	if (list) PK11_FreeSlotList(list);
	if (loginList) PK11_FreeSlotList(loginList);
	if (friendlyList) PK11_FreeSlotList(friendlyList);
	return NULL;
    }

    SECMOD_GetReadLock(moduleLock);

    modules      = SECMOD_GetDefaultModuleList();
    for(mlp = modules; mlp != NULL; mlp = mlp->next) {

#if defined( XP_WIN32 ) 
	






	waste[ j & 0xf] = j++; 
#endif

	for (i = 0; i < mlp->module->slotCount; i++) {
	    PK11SlotInfo *slot = mlp->module->slots[i];

	    if (pk11_IsPresentCertLoad(slot, loadCerts)) {
		if (needRW &&  slot->readOnly) continue;
		if ((type == CKM_INVALID_MECHANISM) 
					|| PK11_DoesMechanism(slot, type)) {
		    if (pk11_LoginStillRequired(slot,wincx)) {
			if (PK11_IsFriendly(slot)) {
			    PK11_AddSlotToList(friendlyList, slot, PR_TRUE);
			} else {
			    PK11_AddSlotToList(loginList, slot, PR_TRUE);
			}
		    } else {
			PK11_AddSlotToList(list, slot, PR_TRUE);
		    }
		}
	    }
	}
    }
    SECMOD_ReleaseReadLock(moduleLock);

    pk11_MoveListToList(list,friendlyList);
    PK11_FreeSlotList(friendlyList);
    pk11_MoveListToList(list,loginList);
    PK11_FreeSlotList(loginList);

    return list;
}





PK11SlotList *
PK11_GetPrivateKeyTokens(CK_MECHANISM_TYPE type,PRBool needRW,void *wincx)
{
    PK11SlotList *list = PK11_GetAllTokens(type,needRW,PR_TRUE,wincx);
    PK11SlotListElement *le, *next ;
    SECStatus rv;

    if (list == NULL) return list;

    for (le = list->head ; le; le = next) {
	next = le->next; 

        rv = PK11_Authenticate(le->slot,PR_TRUE,wincx);
	if (rv != SECSuccess) {
	    PK11_DeleteSlotFromList(list,le);
	    continue;
	}
    }
    return list;
}




PRBool
pk11_filterSlot(PK11SlotInfo *slot, CK_MECHANISM_TYPE mechanism, 
	CK_FLAGS mechanismInfoFlags, unsigned int keySize) 
{
    CK_MECHANISM_INFO mechanism_info;
    CK_RV crv = CKR_OK;

    

    if ((keySize == 0) && (mechanism == CKM_RSA_PKCS) && (slot->hasRSAInfo)) {
	mechanism_info.flags = slot->RSAInfoFlags;
    } else {
	if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    	crv = PK11_GETTAB(slot)->C_GetMechanismInfo(slot->slotID, mechanism, 
							&mechanism_info);
	if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	
	if ((crv == CKR_OK) && (mechanism == CKM_RSA_PKCS) 
						&& (!slot->hasRSAInfo)) {
	    slot->RSAInfoFlags = mechanism_info.flags;
	    slot->hasRSAInfo = PR_TRUE;
	}
    }
    
    if (crv != CKR_OK ) {
	return PR_TRUE;
    }
    if (keySize && ((mechanism_info.ulMinKeySize > keySize)
			|| (mechanism_info.ulMaxKeySize < keySize)) ) {
	

	return PR_TRUE;
    }
    if (mechanismInfoFlags && ((mechanism_info.flags & mechanismInfoFlags) !=
				mechanismInfoFlags) ) {
	return PR_TRUE;
    }
    return PR_FALSE;
}











PK11SlotInfo *
PK11_GetBestSlotMultipleWithAttributes(CK_MECHANISM_TYPE *type, 
		CK_FLAGS *mechanismInfoFlags, unsigned int *keySize, 
		unsigned int mech_count, void *wincx)
{
    PK11SlotList *list = NULL;
    PK11SlotListElement *le ;
    PK11SlotInfo *slot = NULL;
    PRBool freeit = PR_FALSE;
    PRBool listNeedLogin = PR_FALSE;
    int i;
    SECStatus rv;

    list = PK11_GetSlotList(type[0]);

    if ((list == NULL) || (list->head == NULL)) {
	
	list = PK11_GetAllTokens(type[0],PR_FALSE,PR_TRUE,wincx);
	freeit = PR_TRUE;
    }

    
    if (list == NULL) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
	return NULL;
    }

    PORT_SetError(0);


    listNeedLogin = PR_FALSE;
    for (i=0; i < mech_count; i++) {
	if ((type[i] != CKM_FAKE_RANDOM) && 
	    (type[i] != CKM_SHA_1) &&
	    (type[i] != CKM_SHA224) &&
	    (type[i] != CKM_SHA256) &&
	    (type[i] != CKM_SHA384) &&
	    (type[i] != CKM_SHA512) &&
	    (type[i] != CKM_MD5) && 
	    (type[i] != CKM_MD2)) {
	    listNeedLogin = PR_TRUE;
	    break;
	}
    }

    for (le = PK11_GetFirstSafe(list); le;
			 	le = PK11_GetNextSafe(list,le,PR_TRUE)) {
	if (PK11_IsPresent(le->slot)) {
	    PRBool doExit = PR_FALSE;
	    for (i=0; i < mech_count; i++) {
	    	if (!PK11_DoesMechanism(le->slot,type[i])) {
		    doExit = PR_TRUE;
		    break;
		}
		if ((mechanismInfoFlags && mechanismInfoFlags[i]) ||
			(keySize && keySize[i])) {
		    if (pk11_filterSlot(le->slot, type[i], 
			    mechanismInfoFlags ?  mechanismInfoFlags[i] : 0,
			    keySize ? keySize[i] : 0)) {
			doExit = PR_TRUE;
			break;
		    }
		}
	    }
    
	    if (doExit) continue;
	      
	    if (listNeedLogin && le->slot->needLogin) {
		rv = PK11_Authenticate(le->slot,PR_TRUE,wincx);
		if (rv != SECSuccess) continue;
	    }
	    slot = le->slot;
	    PK11_ReferenceSlot(slot);
	    PK11_FreeSlotListElement(list,le);
	    if (freeit) { PK11_FreeSlotList(list); }
	    return slot;
	}
    }
    if (freeit) { PK11_FreeSlotList(list); }
    if (PORT_GetError() == 0) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
    }
    return NULL;
}

PK11SlotInfo *
PK11_GetBestSlotMultiple(CK_MECHANISM_TYPE *type, 
			 unsigned int mech_count, void *wincx)
{
    return PK11_GetBestSlotMultipleWithAttributes(type, NULL, NULL, 
						mech_count, wincx);
}


PK11SlotInfo *
PK11_GetBestSlot(CK_MECHANISM_TYPE type, void *wincx)
{
    return PK11_GetBestSlotMultipleWithAttributes(&type, NULL, NULL, 1, wincx);
}

PK11SlotInfo *
PK11_GetBestSlotWithAttributes(CK_MECHANISM_TYPE type, CK_FLAGS mechanismFlags,
		unsigned int keySize, void *wincx)
{
    return PK11_GetBestSlotMultipleWithAttributes(&type, &mechanismFlags,
						 &keySize, 1, wincx);
}

int
PK11_GetBestKeyLength(PK11SlotInfo *slot,CK_MECHANISM_TYPE mechanism)
{
    CK_MECHANISM_INFO mechanism_info;
    CK_RV crv;

    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_GetMechanismInfo(slot->slotID,
                               mechanism,&mechanism_info);
    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) return 0;

    if (mechanism_info.ulMinKeySize == mechanism_info.ulMaxKeySize) 
		return 0;
    return mechanism_info.ulMaxKeySize;
}













int
PK11_GetMaxKeyLength(CK_MECHANISM_TYPE mechanism)
{
    CK_MECHANISM_INFO mechanism_info;
    PK11SlotList *list = NULL;
    PK11SlotListElement *le ;
    PRBool freeit = PR_FALSE;
    int keyLength = 0;

    list = PK11_GetSlotList(mechanism);

    if ((list == NULL) || (list->head == NULL)) {
	
	list = PK11_GetAllTokens(mechanism,PR_FALSE,PR_FALSE,NULL);
	freeit = PR_TRUE;
    }

    
    if (list == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return 0;
    }

    for (le = PK11_GetFirstSafe(list); le;
			 	le = PK11_GetNextSafe(list,le,PR_TRUE)) {
	PK11SlotInfo *slot = le->slot;
	CK_RV crv;
	if (PK11_IsPresent(slot)) {
	    if (!slot->isThreadSafe) PK11_EnterSlotMonitor(slot);
	    crv = PK11_GETTAB(slot)->C_GetMechanismInfo(slot->slotID,
                               mechanism,&mechanism_info);
 	    if (!slot->isThreadSafe) PK11_ExitSlotMonitor(slot);
	    if ((crv == CKR_OK)  && (mechanism_info.ulMaxKeySize != 0)
		&& (mechanism_info.ulMaxKeySize != 0xffffffff)) {
		keyLength = mechanism_info.ulMaxKeySize;
		break;
	    }
	}
    }
    if (le) 
	PK11_FreeSlotListElement(list, le);
    if (freeit) 
	PK11_FreeSlotList(list);
    return keyLength;
}

SECStatus
PK11_SeedRandom(PK11SlotInfo *slot, unsigned char *data, int len) {
    CK_RV crv;

    PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_SeedRandom(slot->session, data, (CK_ULONG)len);
    PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    return SECSuccess;
}


SECStatus
PK11_GenerateRandomOnSlot(PK11SlotInfo *slot, unsigned char *data, int len) {
    CK_RV crv;

    if (!slot->isInternal) PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_GenerateRandom(slot->session,data, 
							(CK_ULONG)len);
    if (!slot->isInternal) PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return  SECFailure;
    }
    return SECSuccess;
}






SECStatus
PK11_RandomUpdate(void *data, size_t bytes)
{
    PK11SlotInfo *slot;
    PRBool        bestIsInternal;
    SECStatus     status;

    slot = PK11_GetBestSlot(CKM_FAKE_RANDOM, NULL);
    if (slot == NULL) {
	slot = PK11_GetInternalSlot();
	if (!slot)
	    return SECFailure;
    }

    bestIsInternal = PK11_IsInternal(slot);
    status = PK11_SeedRandom(slot, data, bytes);
    PK11_FreeSlot(slot);

    if (!bestIsInternal) {
    	
    	slot = PK11_GetInternalSlot();	
	status = PK11_SeedRandom(slot, data, bytes);
	PK11_FreeSlot(slot);
    }
    return status;
}


SECStatus
PK11_GenerateRandom(unsigned char *data,int len) {
    PK11SlotInfo *slot;
    SECStatus rv;

    slot = PK11_GetBestSlot(CKM_FAKE_RANDOM,NULL);
    if (slot == NULL) return SECFailure;

    rv = PK11_GenerateRandomOnSlot(slot, data, len);
    PK11_FreeSlot(slot);
    return rv;
}





SECStatus 
PK11_ResetToken(PK11SlotInfo *slot, char *sso_pwd)
{
    unsigned char tokenName[32];
    int tokenNameLen;
    CK_RV crv;

    
    tokenNameLen = PORT_Strlen(slot->token_name);
    if (tokenNameLen > sizeof(tokenName)) {
	tokenNameLen = sizeof(tokenName);
    }

    PORT_Memcpy(tokenName,slot->token_name,tokenNameLen);
    if (tokenNameLen < sizeof(tokenName)) {
	PORT_Memset(&tokenName[tokenNameLen],' ',
					 sizeof(tokenName)-tokenNameLen);
    }

        
    PK11_EnterSlotMonitor(slot);

    
    PK11_GETTAB(slot)->C_CloseAllSessions(slot->slotID);
    slot->session = CK_INVALID_SESSION;

     
    crv = PK11_GETTAB(slot)->C_InitToken(slot->slotID,
	(unsigned char *)sso_pwd, sso_pwd ? PORT_Strlen(sso_pwd): 0, tokenName);

    
    PK11_InitToken(slot,PR_TRUE);
    PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    nssTrustDomain_UpdateCachedTokenCerts(slot->nssToken->trustDomain,
	                                      slot->nssToken);
    return SECSuccess;
}
void
PK11Slot_SetNSSToken(PK11SlotInfo *sl, NSSToken *nsst) 
{
    sl->nssToken = nsst;
}

NSSToken *
PK11Slot_GetNSSToken(PK11SlotInfo *sl) 
{
    return sl->nssToken;
}





PK11TokenStatus
PK11_WaitForTokenEvent(PK11SlotInfo *slot, PK11TokenEvent event, 
	PRIntervalTime timeout, PRIntervalTime latency, int series)
{
   PRIntervalTime first_time = 0;
   PRBool first_time_set = PR_FALSE;
   PRBool waitForRemoval;

   if (slot->isPerm) {
	return PK11TokenNotRemovable;
   }
   if (latency == 0) {
	latency = PR_SecondsToInterval(5);
   }
   waitForRemoval = (PRBool) (event == PK11TokenRemovedOrChangedEvent);

   if (series == 0) {
	series = PK11_GetSlotSeries(slot);
   }
   while (PK11_IsPresent(slot) == waitForRemoval ) {
	PRIntervalTime interval;

	if (waitForRemoval && series != PK11_GetSlotSeries(slot)) {
	    return PK11TokenChanged;
	}
	if (timeout == PR_INTERVAL_NO_WAIT) {
	    return waitForRemoval ? PK11TokenPresent : PK11TokenRemoved;
	}
	if (timeout != PR_INTERVAL_NO_TIMEOUT ) {
	    interval = PR_IntervalNow();
	    if (!first_time_set) {
		first_time = interval;
		first_time_set = PR_TRUE;
	    }
	    if ((interval-first_time) > timeout) {
		return waitForRemoval ? PK11TokenPresent : PK11TokenRemoved;
	    }
	}
	PR_Sleep(latency);
   }
   return waitForRemoval ? PK11TokenRemoved : PK11TokenPresent;
}
