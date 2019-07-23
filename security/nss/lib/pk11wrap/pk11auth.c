





































#include "seccomon.h"
#include "secmod.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pkcs11t.h"
#include "pk11func.h"
#include "secitem.h"
#include "secerr.h"

#include "pkim.h" 
















static struct PK11GlobalStruct {
   int transaction;
   PRBool inTransaction;
   char *(PR_CALLBACK *getPass)(PK11SlotInfo *,PRBool,void *);
   PRBool (PR_CALLBACK *verifyPass)(PK11SlotInfo *,void *);
   PRBool (PR_CALLBACK *isLoggedIn)(PK11SlotInfo *,void *);
} PK11_Global = { 1, PR_FALSE, NULL, NULL, NULL };
 







SECStatus
pk11_CheckPassword(PK11SlotInfo *slot,char *pw)
{
    int len = 0;
    CK_RV crv;
    SECStatus rv;
    int64 currtime = PR_Now();
    PRBool mustRetry;
    int retry = 0;

    if (slot->protectedAuthPath) {
	len = 0;
	pw = NULL;
    } else if (pw == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    } else {
	len = PORT_Strlen(pw);
    }

    do {
	PK11_EnterSlotMonitor(slot);
	crv = PK11_GETTAB(slot)->C_Login(slot->session,CKU_USER,
						(unsigned char *)pw,len);
	slot->lastLoginCheck = 0;
	mustRetry = PR_FALSE;
	PK11_ExitSlotMonitor(slot);
	switch (crv) {
	
	case CKR_OK:
	    slot->authTransact = PK11_Global.transaction;
	    
	case CKR_USER_ALREADY_LOGGED_IN:
	    slot->authTime = currtime;
	    rv = SECSuccess;
	    break;
	case CKR_PIN_INCORRECT:
	    PORT_SetError(SEC_ERROR_BAD_PASSWORD);
	    rv = SECWouldBlock; 
	    break;
	

	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_CLOSED:
	    if (retry++ == 0) {
		rv = PK11_InitToken(slot,PR_FALSE);
		if (rv == SECSuccess) {
		    if (slot->session != CK_INVALID_SESSION) {
			mustRetry = PR_TRUE;
		    } else {
			PORT_SetError(PK11_MapError(crv));
			rv = SECFailure;
		    }
		}
		break;
	    }
	    
	default:
	    PORT_SetError(PK11_MapError(crv));
	    rv = SECFailure; 
	}
    } while (mustRetry);
    return rv;
}





SECStatus
PK11_CheckUserPassword(PK11SlotInfo *slot, const char *pw)
{
    int len = 0;
    CK_RV crv;
    SECStatus rv;
    int64 currtime = PR_Now();

    if (slot->protectedAuthPath) {
	len = 0;
	pw = NULL;
    } else if (pw == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    } else {
	len = PORT_Strlen(pw);
    }

    
    PK11_EnterSlotMonitor(slot);
    PK11_GETTAB(slot)->C_Logout(slot->session);

    crv = PK11_GETTAB(slot)->C_Login(slot->session,CKU_USER,
					(unsigned char *)pw,len);
    slot->lastLoginCheck = 0;
    PK11_ExitSlotMonitor(slot);
    switch (crv) {
    
    case CKR_OK:
	slot->authTransact = PK11_Global.transaction;
	slot->authTime = currtime;
	rv = SECSuccess;
	break;
    case CKR_PIN_INCORRECT:
	PORT_SetError(SEC_ERROR_BAD_PASSWORD);
	rv = SECWouldBlock; 
	break;
    default:
	PORT_SetError(PK11_MapError(crv));
	rv = SECFailure; 
    }
    return rv;
}

SECStatus
PK11_Logout(PK11SlotInfo *slot)
{
    CK_RV crv;

    
    PK11_EnterSlotMonitor(slot);
    crv = PK11_GETTAB(slot)->C_Logout(slot->session);
    slot->lastLoginCheck = 0;
    PK11_ExitSlotMonitor(slot);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	return SECFailure;
    }
    return  SECSuccess;
}





void PK11_StartAuthTransaction(void)
{
PK11_Global.transaction++;
PK11_Global.inTransaction = PR_TRUE;
}

void PK11_EndAuthTransaction(void)
{
PK11_Global.transaction++;
PK11_Global.inTransaction = PR_FALSE;
}





void
PK11_HandlePasswordCheck(PK11SlotInfo *slot,void *wincx)
{
    int askpw = slot->askpw;
    PRBool NeedAuth = PR_FALSE;

    if (!slot->needLogin) return;

    if ((slot->defaultFlags & PK11_OWN_PW_DEFAULTS) == 0) {
	PK11SlotInfo *def_slot = PK11_GetInternalKeySlot();

	if (def_slot) {
	    askpw = def_slot->askpw;
	    PK11_FreeSlot(def_slot);
	}
    }

    
    if (!PK11_IsLoggedIn(slot,wincx)) {
	NeedAuth = PR_TRUE;
    } else if (askpw == -1) {
	if (!PK11_Global.inTransaction	||
			 (PK11_Global.transaction != slot->authTransact)) {
    	    PK11_EnterSlotMonitor(slot);
	    PK11_GETTAB(slot)->C_Logout(slot->session);
	    slot->lastLoginCheck = 0;
    	    PK11_ExitSlotMonitor(slot);
	    NeedAuth = PR_TRUE;
	}
    }
    if (NeedAuth) PK11_DoPassword(slot,PR_TRUE,wincx);
}

void
PK11_SlotDBUpdate(PK11SlotInfo *slot)
{
    SECMOD_UpdateModule(slot->module);
}




void
PK11_SetSlotPWValues(PK11SlotInfo *slot,int askpw, int timeout)
{
        slot->askpw = askpw;
        slot->timeout = timeout;
        slot->defaultFlags |= PK11_OWN_PW_DEFAULTS;
        PK11_SlotDBUpdate(slot);
}




void
PK11_GetSlotPWValues(PK11SlotInfo *slot,int *askpw, int *timeout)
{
    *askpw = slot->askpw;
    *timeout = slot->timeout;

    if ((slot->defaultFlags & PK11_OWN_PW_DEFAULTS) == 0) {
	PK11SlotInfo *def_slot = PK11_GetInternalKeySlot();

	if (def_slot) {
	    *askpw = def_slot->askpw;
	    *timeout = def_slot->timeout;
	    PK11_FreeSlot(def_slot);
	}
    }
}






PRBool
pk11_LoginStillRequired(PK11SlotInfo *slot, void *wincx)
{
    return slot->needLogin && !PK11_IsLoggedIn(slot,wincx);
}





SECStatus
PK11_Authenticate(PK11SlotInfo *slot, PRBool loadCerts, void *wincx) {
    if (pk11_LoginStillRequired(slot,wincx)) {
	return PK11_DoPassword(slot,loadCerts,wincx);
    }
    return SECSuccess;
}





SECStatus
pk11_AuthenticateUnfriendly(PK11SlotInfo *slot, PRBool loadCerts, void *wincx)
{
    SECStatus rv = SECSuccess;
    if (!PK11_IsFriendly(slot)) {
	rv = PK11_Authenticate(slot, loadCerts, wincx);
    }
    return rv;
}





SECStatus
PK11_CheckSSOPassword(PK11SlotInfo *slot, char *ssopw)
{
    CK_SESSION_HANDLE rwsession;
    CK_RV crv;
    SECStatus rv = SECFailure;
    int len = 0;

    
    rwsession = PK11_GetRWSession(slot);
    if (rwsession == CK_INVALID_SESSION) {
    	PORT_SetError(SEC_ERROR_BAD_DATA);
    	return rv;
    }

    if (slot->protectedAuthPath) {
	len = 0;
	ssopw = NULL;
    } else if (ssopw == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    } else {
	len = PORT_Strlen(ssopw);
    }

    
    crv = PK11_GETTAB(slot)->C_Login(rwsession,CKU_SO,
						(unsigned char *)ssopw,len);
    slot->lastLoginCheck = 0;
    switch (crv) {
    
    case CKR_OK:
	rv = SECSuccess;
	break;
    case CKR_PIN_INCORRECT:
	PORT_SetError(SEC_ERROR_BAD_PASSWORD);
	rv = SECWouldBlock; 
	break;
    default:
	PORT_SetError(PK11_MapError(crv));
	rv = SECFailure; 
    }
    PK11_GETTAB(slot)->C_Logout(rwsession);
    slot->lastLoginCheck = 0;

    
    PK11_RestoreROSession(slot,rwsession);
    return rv;
}




SECStatus
PK11_VerifyPW(PK11SlotInfo *slot,char *pw)
{
    int len = PORT_Strlen(pw);

    if ((slot->minPassword > len) || (slot->maxPassword < len)) {
	PORT_SetError(SEC_ERROR_BAD_DATA);
	return SECFailure;
    }
    return SECSuccess;
}




SECStatus
PK11_InitPin(PK11SlotInfo *slot, const char *ssopw, const char *userpw)
{
    CK_SESSION_HANDLE rwsession = CK_INVALID_SESSION;
    CK_RV crv;
    SECStatus rv = SECFailure;
    int len;
    int ssolen;

    if (userpw == NULL) userpw = "";
    if (ssopw == NULL) ssopw = "";

    len = PORT_Strlen(userpw);
    ssolen = PORT_Strlen(ssopw);

    
    rwsession = PK11_GetRWSession(slot);
    if (rwsession == CK_INVALID_SESSION) {
    	PORT_SetError(SEC_ERROR_BAD_DATA);
	slot->lastLoginCheck = 0;
    	return rv;
    }

    if (slot->protectedAuthPath) {
	len = 0;
	ssolen = 0;
	ssopw = NULL;
	userpw = NULL;
    }

    
    crv = PK11_GETTAB(slot)->C_Login(rwsession,CKU_SO, 
					  (unsigned char *)ssopw,ssolen);
    slot->lastLoginCheck = 0;
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
	goto done;
    }

    crv = PK11_GETTAB(slot)->C_InitPIN(rwsession,(unsigned char *)userpw,len);
    if (crv != CKR_OK) {
	PORT_SetError(PK11_MapError(crv));
    } else {
    	rv = SECSuccess;
    }

done:
    PK11_GETTAB(slot)->C_Logout(rwsession);
    slot->lastLoginCheck = 0;
    PK11_RestoreROSession(slot,rwsession);
    if (rv == SECSuccess) {
        
        PK11_InitToken(slot,PR_TRUE);
	if (slot->needLogin) {
	    PK11_EnterSlotMonitor(slot);
	    PK11_GETTAB(slot)->C_Login(slot->session,CKU_USER,
						(unsigned char *)userpw,len);
	    slot->lastLoginCheck = 0;
	    PK11_ExitSlotMonitor(slot);
	}
    }
    return rv;
}




SECStatus
PK11_ChangePW(PK11SlotInfo *slot, const char *oldpw, const char *newpw)
{
    CK_RV crv;
    SECStatus rv = SECFailure;
    int newLen;
    int oldLen;
    CK_SESSION_HANDLE rwsession;

    
    if (slot->protectedAuthPath) {
	if (newpw == NULL) newLen = 0;
	if (oldpw == NULL) oldLen = 0;
    } else {
	if (newpw == NULL) newpw = "";
	if (oldpw == NULL) oldpw = "";
	newLen = PORT_Strlen(newpw);
	oldLen = PORT_Strlen(oldpw);
    }


    
    rwsession = PK11_GetRWSession(slot);
    if (rwsession == CK_INVALID_SESSION) {
    	PORT_SetError(SEC_ERROR_BAD_DATA);
    	return rv;
    }

    crv = PK11_GETTAB(slot)->C_SetPIN(rwsession,
		(unsigned char *)oldpw,oldLen,(unsigned char *)newpw,newLen);
    if (crv == CKR_OK) {
	rv = SECSuccess;
    } else {
	PORT_SetError(PK11_MapError(crv));
    }

    PK11_RestoreROSession(slot,rwsession);

    
    PK11_InitToken(slot,PR_TRUE);
    return rv;
}

static char *
pk11_GetPassword(PK11SlotInfo *slot, PRBool retry, void * wincx)
{
    if (PK11_Global.getPass == NULL) return NULL;
    return (*PK11_Global.getPass)(slot, retry, wincx);
}

void
PK11_SetPasswordFunc(PK11PasswordFunc func)
{
    PK11_Global.getPass = func;
}

void
PK11_SetVerifyPasswordFunc(PK11VerifyPasswordFunc func)
{
    PK11_Global.verifyPass = func;
}

void
PK11_SetIsLoggedInFunc(PK11IsLoggedInFunc func)
{
    PK11_Global.isLoggedIn = func;
}









SECStatus
PK11_DoPassword(PK11SlotInfo *slot, PRBool loadCerts, void *wincx)
{
    SECStatus rv = SECFailure;
    char * password;
    PRBool attempt = PR_FALSE;

    if (PK11_NeedUserInit(slot)) {
	PORT_SetError(SEC_ERROR_IO);
	return SECFailure;
    }


    









    if (PK11_IsLoggedIn(slot,NULL) && 
    			(PK11_Global.verifyPass != NULL)) {
	if (!PK11_Global.verifyPass(slot,wincx)) {
	    PORT_SetError(SEC_ERROR_BAD_PASSWORD);
	    return SECFailure;
	}
	return SECSuccess;
    }

    








    while ((password = pk11_GetPassword(slot, attempt, wincx)) != NULL) {
	











	attempt = PR_TRUE;
	if (slot->protectedAuthPath) {
	    

	    if (strcmp(password, PK11_PW_RETRY) == 0) {
		rv = SECWouldBlock;
		PORT_Free(password);
		continue;
	    }
	    
	    if (strcmp(password, PK11_PW_AUTHENTICATED) == 0) {
		rv = SECSuccess;
		PORT_Free(password);
		break;
	    }
	}
	rv = pk11_CheckPassword(slot,password);
	PORT_Memset(password, 0, PORT_Strlen(password));
	PORT_Free(password);
	if (rv != SECWouldBlock) break;
    }
    if (rv == SECSuccess) {
	if (!PK11_IsFriendly(slot)) {
	    nssTrustDomain_UpdateCachedTokenCerts(slot->nssToken->trustDomain,
	                                      slot->nssToken);
	}
    } else if (!attempt) PORT_SetError(SEC_ERROR_BAD_PASSWORD);
    return rv;
}

void PK11_LogoutAll(void)
{
    SECMODListLock *lock = SECMOD_GetDefaultModuleListLock();
    SECMODModuleList *modList;
    SECMODModuleList *mlp = NULL;
    int i;

    
    if (lock == NULL) {
	return;
    }

    SECMOD_GetReadLock(lock);
    modList = SECMOD_GetDefaultModuleList();
    
    for (mlp = modList; mlp != NULL; mlp = mlp->next) {
	for (i=0; i < mlp->module->slotCount; i++) {
	    PK11_Logout(mlp->module->slots[i]);
	}
    }

    SECMOD_ReleaseReadLock(lock);
}

int
PK11_GetMinimumPwdLength(PK11SlotInfo *slot)
{
    return ((int)slot->minPassword);
}


PRBool
PK11_ProtectedAuthenticationPath(PK11SlotInfo *slot)
{
	return slot->protectedAuthPath;
}






PRBool PK11_NeedPWInitForSlot(PK11SlotInfo *slot)
{
    if (slot->needLogin && PK11_NeedUserInit(slot)) {
	return PR_TRUE;
    }
    if (!slot->needLogin && !PK11_NeedUserInit(slot)) {
	return PR_TRUE;
    }
    return PR_FALSE;
}

PRBool PK11_NeedPWInit()
{
    PK11SlotInfo *slot = PK11_GetInternalKeySlot();
    PRBool ret = PK11_NeedPWInitForSlot(slot);

    PK11_FreeSlot(slot);
    return ret;
}

PRBool 
pk11_InDelayPeriod(PRIntervalTime lastTime, PRIntervalTime delayTime, 
						PRIntervalTime *retTime)
{
    PRIntervalTime time;

    *retTime = time = PR_IntervalNow();
    return (PRBool) (lastTime) && ((time-lastTime) < delayTime);
}





PRBool
PK11_IsLoggedIn(PK11SlotInfo *slot,void *wincx)
{
    CK_SESSION_INFO sessionInfo;
    int askpw = slot->askpw;
    int timeout = slot->timeout;
    CK_RV crv;
    PRIntervalTime curTime;
    static PRIntervalTime login_delay_time = 0;

    if (login_delay_time == 0) {
	login_delay_time = PR_SecondsToInterval(1);
    }

    

    if ((slot->defaultFlags & PK11_OWN_PW_DEFAULTS) == 0) {
	PK11SlotInfo *def_slot = PK11_GetInternalKeySlot();

	if (def_slot) {
	    askpw = def_slot->askpw;
	    timeout = def_slot->timeout;
	    PK11_FreeSlot(def_slot);
	}
    }

    if ((wincx != NULL) && (PK11_Global.isLoggedIn != NULL) &&
	(*PK11_Global.isLoggedIn)(slot, wincx) == PR_FALSE) { return PR_FALSE; }


    
    if (askpw == 1) {
	int64 currtime = PR_Now();
	int64 result;
	int64 mult;
	
	LL_I2L(result, timeout);
	LL_I2L(mult, 60*1000*1000);
	LL_MUL(result,result,mult);
	LL_ADD(result, result, slot->authTime);
	if (LL_CMP(result, <, currtime) ) {
	    PK11_EnterSlotMonitor(slot);
	    PK11_GETTAB(slot)->C_Logout(slot->session);
	    slot->lastLoginCheck = 0;
	    PK11_ExitSlotMonitor(slot);
	} else {
	    slot->authTime = currtime;
	}
    }

    PK11_EnterSlotMonitor(slot);
    if (pk11_InDelayPeriod(slot->lastLoginCheck,login_delay_time, &curTime)) {
	sessionInfo.state = slot->lastState;
	crv = CKR_OK;
    } else {
	crv = PK11_GETTAB(slot)->C_GetSessionInfo(slot->session,&sessionInfo);
	if (crv == CKR_OK) {
	    slot->lastState = sessionInfo.state;
	    slot->lastLoginCheck = curTime;
	}
    }
    PK11_ExitSlotMonitor(slot);
    
    if (crv != CKR_OK) {
	slot->session = CK_INVALID_SESSION;
	return PR_FALSE;
    }

    switch (sessionInfo.state) {
    case CKS_RW_PUBLIC_SESSION:
    case CKS_RO_PUBLIC_SESSION:
    default:
	break; 
    case CKS_RW_USER_FUNCTIONS:
    case CKS_RW_SO_FUNCTIONS:
    case CKS_RO_USER_FUNCTIONS:
	return PR_TRUE;
    }
    return PR_FALSE; 
}
