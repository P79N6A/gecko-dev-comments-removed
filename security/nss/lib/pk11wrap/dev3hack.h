



#ifndef DEVNSS3HACK_H
#define DEVNSS3HACK_H

#include "cert.h"

PR_BEGIN_EXTERN_C

NSS_EXTERN NSSToken *
nssToken_CreateFromPK11SlotInfo(NSSTrustDomain *td, PK11SlotInfo *nss3slot);

NSS_EXTERN void
nssToken_UpdateName(NSSToken *);

NSS_EXTERN PRStatus
nssToken_Refresh(NSSToken *);

NSSTrustDomain *
nssToken_GetTrustDomain(NSSToken *token);

void PK11Slot_SetNSSToken(PK11SlotInfo *sl, NSSToken *nsst);

NSSToken * PK11Slot_GetNSSToken(PK11SlotInfo *sl);

PR_END_EXTERN_C

#endif 
