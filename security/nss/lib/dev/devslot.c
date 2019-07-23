



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: devslot.c,v $ $Revision: 1.24 $ $Date: 2008/08/09 01:25:58 $";
#endif 

#ifndef NSSCKEPV_H
#include "nssckepv.h"
#endif 

#ifndef DEVM_H
#include "devm.h"
#endif 

#ifndef CKHELPER_H
#include "ckhelper.h"
#endif 

#include "pk11pub.h"


#define NSSSLOT_TOKEN_DELAY_TIME 1



#define NSSSLOT_IS_FRIENDLY(slot) \
  (slot->base.flags & NSSSLOT_FLAGS_FRIENDLY)


static PRIntervalTime s_token_delay_time = 0;


static const CK_FLAGS s_ck_readonly_flags = CKF_SERIAL_SESSION;

NSS_IMPLEMENT PRStatus
nssSlot_Destroy (
  NSSSlot *slot
)
{
    if (slot) {
	if (PR_AtomicDecrement(&slot->base.refCount) == 0) {
	    PZ_DestroyLock(slot->base.lock);
	    return nssArena_Destroy(slot->base.arena);
	}
    }
    return PR_SUCCESS;
}

void
nssSlot_EnterMonitor(NSSSlot *slot)
{
    if (slot->lock) {
	PZ_Lock(slot->lock);
    }
}

void
nssSlot_ExitMonitor(NSSSlot *slot)
{
    if (slot->lock) {
	PZ_Unlock(slot->lock);
    }
}

NSS_IMPLEMENT void
NSSSlot_Destroy (
  NSSSlot *slot
)
{
    (void)nssSlot_Destroy(slot);
}

NSS_IMPLEMENT NSSSlot *
nssSlot_AddRef (
  NSSSlot *slot
)
{
    PR_AtomicIncrement(&slot->base.refCount);
    return slot;
}

NSS_IMPLEMENT NSSUTF8 *
nssSlot_GetName (
  NSSSlot *slot
)
{
    return slot->base.name;
}

NSS_IMPLEMENT NSSUTF8 *
nssSlot_GetTokenName (
  NSSSlot *slot
)
{
    return nssToken_GetName(slot->token);
}

NSS_IMPLEMENT void
nssSlot_ResetDelay (
  NSSSlot *slot
)
{
    slot->lastTokenPing = 0;
}

static PRBool
within_token_delay_period(NSSSlot *slot)
{
    PRIntervalTime time, lastTime;
    
    if (s_token_delay_time == 0) {
	s_token_delay_time = PR_SecondsToInterval(NSSSLOT_TOKEN_DELAY_TIME);
    }
    time = PR_IntervalNow();
    lastTime = slot->lastTokenPing;
    if ((lastTime) && ((time - lastTime) < s_token_delay_time)) {
	return PR_TRUE;
    }
    slot->lastTokenPing = time;
    return PR_FALSE;
}

NSS_IMPLEMENT PRBool
nssSlot_IsTokenPresent (
  NSSSlot *slot
)
{
    CK_RV ckrv;
    PRStatus nssrv;
    
    nssSession *session;
    CK_SLOT_INFO slotInfo;
    void *epv;
    
    if (nssSlot_IsPermanent(slot)) {
	return !PK11_IsDisabled(slot->pk11slot);
    }
    
    if (within_token_delay_period(slot)) {
	return ((slot->ckFlags & CKF_TOKEN_PRESENT) != 0);
    }

    
    epv = slot->epv;
    if (!epv) {
	return PR_FALSE;
    }
    nssSlot_EnterMonitor(slot);
    ckrv = CKAPI(epv)->C_GetSlotInfo(slot->slotID, &slotInfo);
    nssSlot_ExitMonitor(slot);
    if (ckrv != CKR_OK) {
	slot->token->base.name[0] = 0; 
	return PR_FALSE;
    }
    slot->ckFlags = slotInfo.flags;
    
    if ((slot->ckFlags & CKF_TOKEN_PRESENT) == 0) {
	if (!slot->token) {
	    
	    return PR_FALSE;
	}
	session = nssToken_GetDefaultSession(slot->token);
	if (session) {
	    nssSession_EnterMonitor(session);
	    
	    if (session->handle != CK_INVALID_SESSION) {
		
		CKAPI(epv)->C_CloseSession(session->handle);
		session->handle = CK_INVALID_SESSION;
	    }
	    nssSession_ExitMonitor(session);
	}
	if (slot->token->base.name[0] != 0) {
	    
	    slot->token->base.name[0] = 0; 
	    nssToken_NotifyCertsNotVisible(slot->token);
	}
	slot->token->base.name[0] = 0; 
	
	nssToken_Remove(slot->token);
	return PR_FALSE;
    }
    


    session = nssToken_GetDefaultSession(slot->token);
    if (session) {
	nssSession_EnterMonitor(session);
	if (session->handle != CK_INVALID_SESSION) {
	    CK_SESSION_INFO sessionInfo;
	    ckrv = CKAPI(epv)->C_GetSessionInfo(session->handle, &sessionInfo);
	    if (ckrv != CKR_OK) {
		
		CKAPI(epv)->C_CloseSession(session->handle);
		session->handle = CK_INVALID_SESSION;
	    }
	}
	nssSession_ExitMonitor(session);
	
	if (session->handle != CK_INVALID_SESSION)
	    return PR_TRUE;
    } 
    



    nssToken_NotifyCertsNotVisible(slot->token);
    nssToken_Remove(slot->token);
    
    nssrv = nssSlot_Refresh(slot);
    if (nssrv != PR_SUCCESS) {
        slot->token->base.name[0] = 0; 
        slot->ckFlags &= ~CKF_TOKEN_PRESENT;
        return PR_FALSE;
    }
    return PR_TRUE;
}

NSS_IMPLEMENT void *
nssSlot_GetCryptokiEPV (
  NSSSlot *slot
)
{
    return slot->epv;
}

NSS_IMPLEMENT NSSToken *
nssSlot_GetToken (
  NSSSlot *slot
)
{
    if (nssSlot_IsTokenPresent(slot)) {
	return nssToken_AddRef(slot->token);
    }
    return (NSSToken *)NULL;
}

NSS_IMPLEMENT PRStatus
nssSession_EnterMonitor (
  nssSession *s
)
{
    if (s->lock) PZ_Lock(s->lock);
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
nssSession_ExitMonitor (
  nssSession *s
)
{
    return (s->lock) ? PZ_Unlock(s->lock) : PR_SUCCESS;
}

NSS_EXTERN PRBool
nssSession_IsReadWrite (
  nssSession *s
)
{
    return s->isRW;
}

