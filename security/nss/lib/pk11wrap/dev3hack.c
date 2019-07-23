



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: dev3hack.c,v $ $Revision: 1.25 $ $Date: 2008/09/30 04:09:04 $";
#endif 

#ifndef PKIT_H
#include "pkit.h"
#endif 

#ifndef DEVM_H
#include "devm.h"
#endif 

#include "pki3hack.h"
#include "dev3hack.h"
#include "pkim.h"

#ifndef BASE_H
#include "base.h"
#endif 

#include "pk11func.h"
#include "secmodti.h"
#include "secerr.h"

NSS_IMPLEMENT nssSession *
nssSession_ImportNSS3Session(NSSArena *arenaOpt,
                             CK_SESSION_HANDLE session, 
                             PZLock *lock, PRBool rw)
{
    nssSession *rvSession = NULL;
    if (session != CK_INVALID_SESSION) {
	rvSession = nss_ZNEW(arenaOpt, nssSession);
	if (rvSession) {
	    rvSession->handle = session;
	    rvSession->lock = lock;
	    rvSession->ownLock = PR_FALSE;
	    rvSession->isRW = rw;
	}
    }
    return rvSession;
}

NSS_IMPLEMENT nssSession *
nssSlot_CreateSession
(
  NSSSlot *slot,
  NSSArena *arenaOpt,
  PRBool readWrite
)
{
    nssSession *rvSession;
    rvSession = nss_ZNEW(arenaOpt, nssSession);
    if (!rvSession) {
	return (nssSession *)NULL;
    }
    if (readWrite) {
	rvSession->handle = PK11_GetRWSession(slot->pk11slot);
	if (rvSession->handle == CK_INVALID_HANDLE) {
	    nss_ZFreeIf(rvSession);
	    return NULL;
	}
	rvSession->isRW = PR_TRUE;
	rvSession->slot = slot;
        














        rvSession->lock = NULL;
        rvSession->ownLock = PR_FALSE;
	return rvSession;
    } else {
	return NULL;
    }
}

NSS_IMPLEMENT PRStatus
nssSession_Destroy
(
  nssSession *s
)
{
    CK_RV ckrv = CKR_OK;
    if (s) {
	if (s->isRW) {
	    PK11_RestoreROSession(s->slot->pk11slot, s->handle);
	}
	nss_ZFreeIf(s);
    }
    return (ckrv == CKR_OK) ? PR_SUCCESS : PR_FAILURE;
}

static NSSSlot *
nssSlot_CreateFromPK11SlotInfo(NSSTrustDomain *td, PK11SlotInfo *nss3slot)
{
    NSSSlot *rvSlot;
    NSSArena *arena;
    arena = nssArena_Create();
    if (!arena) {
	return NULL;
    }
    rvSlot = nss_ZNEW(arena, NSSSlot);
    if (!rvSlot) {
	nssArena_Destroy(arena);
	return NULL;
    }
    rvSlot->base.refCount = 1;
    rvSlot->base.lock = PZ_NewLock(nssILockOther);
    rvSlot->base.arena = arena;
    rvSlot->pk11slot = nss3slot;
    rvSlot->epv = nss3slot->functionList;
    rvSlot->slotID = nss3slot->slotID;
    
    rvSlot->base.name = nssUTF8_Duplicate(nss3slot->slot_name,td->arena);
    rvSlot->lock = (nss3slot->isThreadSafe) ? NULL : nss3slot->sessionLock;
    return rvSlot;
}

NSSToken *
nssToken_CreateFromPK11SlotInfo(NSSTrustDomain *td, PK11SlotInfo *nss3slot)
{
    NSSToken *rvToken;
    NSSArena *arena;

    
    if (nss3slot->disabled) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
	return NULL;
    }
    arena = nssArena_Create();
    if (!arena) {
	return NULL;
    }
    rvToken = nss_ZNEW(arena, NSSToken);
    if (!rvToken) {
	nssArena_Destroy(arena);
	return NULL;
    }
    rvToken->base.refCount = 1;
    rvToken->base.lock = PZ_NewLock(nssILockOther);
    if (!rvToken->base.lock) {
	nssArena_Destroy(arena);
	return NULL;
    }
    rvToken->base.arena = arena;
    rvToken->pk11slot = nss3slot;
    rvToken->epv = nss3slot->functionList;
    rvToken->defaultSession = nssSession_ImportNSS3Session(td->arena,
                                                       nss3slot->session,
                                                       nss3slot->sessionLock,
                                                       nss3slot->defRWSession);
#if 0 
    if (!rvToken->defaultSession) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
    	goto loser;
    }
#endif
    if (!PK11_IsInternal(nss3slot) && PK11_IsHW(nss3slot)) {
	rvToken->cache = nssTokenObjectCache_Create(rvToken, 
	                                            PR_TRUE, PR_TRUE, PR_TRUE);
	if (!rvToken->cache)
	    goto loser;
    }
    rvToken->trustDomain = td;
    
    rvToken->base.name = nssUTF8_Duplicate(nss3slot->token_name,td->arena);
    rvToken->slot = nssSlot_CreateFromPK11SlotInfo(td, nss3slot);
    if (!rvToken->slot) {
        goto loser;
    }
    rvToken->slot->token = rvToken;
    if (rvToken->defaultSession)
	rvToken->defaultSession->slot = rvToken->slot;
    return rvToken;
loser:
    PZ_DestroyLock(rvToken->base.lock);
    nssArena_Destroy(arena);
    return NULL;
}

NSS_IMPLEMENT void
nssToken_UpdateName(NSSToken *token)
{
    if (!token) {
	return;
    }
    token->base.name = nssUTF8_Duplicate(token->pk11slot->token_name,token->base.arena);
}

NSS_IMPLEMENT PRBool
nssSlot_IsPermanent
(
  NSSSlot *slot
)
{
    return slot->pk11slot->isPerm;
}

NSS_IMPLEMENT PRBool
nssSlot_IsFriendly
(
  NSSSlot *slot
)
{
    return PK11_IsFriendly(slot->pk11slot);
}

NSS_IMPLEMENT PRStatus
nssToken_Refresh(NSSToken *token)
{
    PK11SlotInfo *nss3slot;

    if (!token) {
	return PR_SUCCESS;
    }
    nss3slot = token->pk11slot;
    token->defaultSession = 
    	nssSession_ImportNSS3Session(token->slot->base.arena,
				     nss3slot->session,
				     nss3slot->sessionLock,
				     nss3slot->defRWSession);
    return token->defaultSession ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT PRStatus
nssSlot_Refresh
(
  NSSSlot *slot
)
{
    PK11SlotInfo *nss3slot = slot->pk11slot;
    PRBool doit = PR_FALSE;
    if (slot->token && slot->token->base.name[0] == 0) {
	doit = PR_TRUE;
    }
    if (PK11_InitToken(nss3slot, PR_FALSE) != SECSuccess) {
	return PR_FAILURE;
    }
    if (doit) {
	nssTrustDomain_UpdateCachedTokenCerts(slot->token->trustDomain, 
	                                      slot->token);
    }
    return nssToken_Refresh(slot->token);
}

NSS_IMPLEMENT PRStatus
nssToken_GetTrustOrder
(
  NSSToken *tok
)
{
    PK11SlotInfo *slot;
    SECMODModule *module;
    slot = tok->pk11slot;
    module = PK11_GetModule(slot);
    return module->trustOrder;
}

NSS_IMPLEMENT PRBool
nssSlot_IsLoggedIn
(
  NSSSlot *slot
)
{
    if (!slot->pk11slot->needLogin) {
	return PR_TRUE;
    }
    return PK11_IsLoggedIn(slot->pk11slot, NULL);
}


NSSTrustDomain *
nssToken_GetTrustDomain(NSSToken *token)
{
    return token->trustDomain;
}

NSS_EXTERN PRStatus
nssTrustDomain_RemoveTokenCertsFromCache
(
  NSSTrustDomain *td,
  NSSToken *token
);

NSS_IMPLEMENT PRStatus
nssToken_NotifyCertsNotVisible
(
  NSSToken *tok
)
{
    return nssTrustDomain_RemoveTokenCertsFromCache(tok->trustDomain, tok);
}

