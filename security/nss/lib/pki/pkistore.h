



































#ifndef PKISTORE_H
#define PKISTORE_H

#ifdef DEBUG
static const char PKISTORE_CVS_ID[] = "@(#) $RCSfile: pkistore.h,v $ $Revision: 1.12 $ $Date: 2008/06/06 01:19:31 $";
#endif 

#ifndef NSSPKIT_H
#include "nsspkit.h"
#endif 

#ifndef BASE_H
#include "base.h"
#endif 

PR_BEGIN_EXTERN_C



















NSS_EXTERN nssCertificateStore *
nssCertificateStore_Create
(
  NSSArena *arenaOpt
);

NSS_EXTERN PRStatus
nssCertificateStore_Destroy
(
  nssCertificateStore *store
);




NSS_EXTERN NSSCertificate *
nssCertificateStore_FindOrAdd 
(
  nssCertificateStore *store,
  NSSCertificate *c
);

NSS_EXTERN void
nssCertificateStore_RemoveCertLOCKED
(
  nssCertificateStore *store,
  NSSCertificate *cert
);

struct nssCertificateStoreTraceStr {
    nssCertificateStore* store;
    PZLock* lock;
    PRBool locked;
    PRBool unlocked;
};

typedef struct nssCertificateStoreTraceStr nssCertificateStoreTrace;

NSS_EXTERN void
nssCertificateStore_Lock (
  nssCertificateStore *store, nssCertificateStoreTrace* out
);

NSS_EXTERN void
nssCertificateStore_Unlock (
  nssCertificateStore *store, const nssCertificateStoreTrace* in,
  nssCertificateStoreTrace* out
);

NSS_EXTERN NSSCertificate **
nssCertificateStore_FindCertificatesBySubject
(
  nssCertificateStore *store,
  NSSDER *subject,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCertificate **
nssCertificateStore_FindCertificatesByNickname
(
  nssCertificateStore *store,
  const NSSUTF8 *nickname,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCertificate **
nssCertificateStore_FindCertificatesByEmail
(
  nssCertificateStore *store,
  NSSASCII7 *email,
  NSSCertificate *rvOpt[],
  PRUint32 maximumOpt,
  NSSArena *arenaOpt
);

NSS_EXTERN NSSCertificate *
nssCertificateStore_FindCertificateByIssuerAndSerialNumber
(
  nssCertificateStore *store,
  NSSDER *issuer,
  NSSDER *serial
);

NSS_EXTERN NSSCertificate *
nssCertificateStore_FindCertificateByEncodedCertificate
(
  nssCertificateStore *store,
  NSSDER *encoding
);

NSS_EXTERN PRStatus
nssCertificateStore_AddTrust
(
  nssCertificateStore *store,
  NSSTrust *trust
);

NSS_EXTERN NSSTrust *
nssCertificateStore_FindTrustForCertificate
(
  nssCertificateStore *store,
  NSSCertificate *cert
);

NSS_EXTERN PRStatus
nssCertificateStore_AddSMIMEProfile
(
  nssCertificateStore *store,
  nssSMIMEProfile *profile
);

NSS_EXTERN nssSMIMEProfile *
nssCertificateStore_FindSMIMEProfileForCertificate
(
  nssCertificateStore *store,
  NSSCertificate *cert
);

NSS_EXTERN void
nssCertificateStore_DumpStoreInfo
(
  nssCertificateStore *store,
  void (* cert_dump_iter)(const void *, void *, void *),
  void *arg
);

PR_END_EXTERN_C

#endif 
