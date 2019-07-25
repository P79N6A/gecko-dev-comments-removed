



































#ifndef PKINSS3HACK_H
#define PKINSS3HACK_H

#ifdef DEBUG
static const char PKINSS3HACK_CVS_ID[] = "@(#) $RCSfile: pki3hack.h,v $ $Revision: 1.19.192.1 $ $Date: 2011/03/26 16:55:01 $";
#endif 

#ifndef NSSDEVT_H
#include "nssdevt.h"
#endif 

#ifndef DEVT_H
#include "devt.h"
#endif 

#ifndef NSSPKIT_H
#include "nsspkit.h"
#endif 

#include "base.h"

#include "cert.h"

PR_BEGIN_EXTERN_C

#define NSSITEM_FROM_SECITEM(nssit, secit)  \
    (nssit)->data = (void *)(secit)->data;  \
    (nssit)->size = (PRUint32)(secit)->len;

#define SECITEM_FROM_NSSITEM(secit, nssit)          \
    (secit)->data = (unsigned char *)(nssit)->data; \
    (secit)->len  = (unsigned int)(nssit)->size;

NSS_EXTERN NSSTrustDomain *
STAN_GetDefaultTrustDomain();

NSS_EXTERN NSSCryptoContext *
STAN_GetDefaultCryptoContext();

NSS_EXTERN PRStatus
STAN_InitTokenForSlotInfo(NSSTrustDomain *td, PK11SlotInfo *slot);

NSS_EXTERN PRStatus
STAN_ResetTokenInterator(NSSTrustDomain *td);

NSS_EXTERN PRStatus
STAN_LoadDefaultNSS3TrustDomain(void);

NSS_EXTERN PRStatus
STAN_Shutdown();

NSS_EXTERN SECStatus
STAN_AddModuleToDefaultTrustDomain(SECMODModule *module);

NSS_EXTERN SECStatus
STAN_RemoveModuleFromDefaultTrustDomain(SECMODModule *module);

NSS_EXTERN CERTCertificate *
STAN_ForceCERTCertificateUpdate(NSSCertificate *c);

NSS_EXTERN CERTCertificate *
STAN_GetCERTCertificate(NSSCertificate *c);

NSS_EXTERN CERTCertificate *
STAN_GetCERTCertificateOrRelease(NSSCertificate *c);

NSS_EXTERN NSSCertificate *
STAN_GetNSSCertificate(CERTCertificate *c);

NSS_EXTERN CERTCertTrust * 
nssTrust_GetCERTCertTrustForCert(NSSCertificate *c, CERTCertificate *cc);

NSS_EXTERN PRStatus
STAN_DeleteCertTrustMatchingSlot(NSSCertificate *c);

NSS_EXTERN PRStatus
STAN_ChangeCertTrust(CERTCertificate *cc, CERTCertTrust *trust);

NSS_EXTERN PRStatus
nssPKIX509_GetIssuerAndSerialFromDER(NSSDER *der, NSSArena *arena, 
                                     NSSDER *issuer, NSSDER *serial);

NSS_EXTERN char *
STAN_GetCERTCertificateName(PLArenaPool *arenaOpt, NSSCertificate *c);

NSS_EXTERN char *
STAN_GetCERTCertificateNameForInstance(PLArenaPool *arenaOpt,
                                       NSSCertificate *c,
                                       nssCryptokiInstance *instance);


NSS_EXTERN NSSCertificate *
NSSCertificate_Create
(
  NSSArena *arenaOpt
);




NSS_EXTERN NSSCertificate *
nssTrustDomain_FindBestCertificateByNicknameForToken
(
  NSSTrustDomain *td,
  NSSToken *token,
  NSSUTF8 *name,
  NSSTime *timeOpt, 
  NSSUsage *usage,
  NSSPolicies *policiesOpt 
);




NSS_EXTERN NSSCertificate **
nssTrustDomain_FindCertificatesByNicknameForToken
(
  NSSTrustDomain *td,
  NSSToken *token,
  NSSUTF8 *name,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt, 
  NSSArena *arenaOpt
);


NSS_EXTERN PRStatus
nssTrustDomain_TraverseCertificatesBySubject
(
  NSSTrustDomain *td,
  NSSDER *subject,
  PRStatus (*callback)(NSSCertificate *c, void *arg),
  void *arg
);


NSS_EXTERN PRStatus
nssTrustDomain_TraverseCertificatesByNickname
(
  NSSTrustDomain *td,
  NSSUTF8 *nickname,
  PRStatus (*callback)(NSSCertificate *c, void *arg),
  void *arg
);


NSS_EXTERN PRStatus
nssTrustDomain_TraverseCertificates
(
  NSSTrustDomain *td,
  PRStatus (*callback)(NSSCertificate *c, void *arg),
  void *arg
);


NSS_EXTERN PRStatus
nssTrustDomain_AddTempCertToPerm
(
  NSSCertificate *c
);

PR_END_EXTERN_C

#endif 
