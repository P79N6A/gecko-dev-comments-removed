









#ifndef _CERTXUTL_H_
#define _CERTXUTL_H_

#include "nspr.h"

#ifdef OLD
typedef enum {
    CertificateExtensions,
    CrlExtensions,
    OCSPRequestExtensions,
    OCSPSingleRequestExtensions,
    OCSPResponseSingleExtensions
} ExtensionsType;
#endif

extern PRBool
cert_HasCriticalExtension (CERTCertExtension **extensions);

extern SECStatus
CERT_FindBitStringExtension (CERTCertExtension **extensions,
			     int tag, SECItem *retItem);
extern void *
cert_StartExtensions (void *owner, PLArenaPool *arena,
                      void (*setExts)(void *object, CERTCertExtension **exts));

extern SECStatus
cert_FindExtension (CERTCertExtension **extensions, int tag, SECItem *value);

extern SECStatus
cert_FindExtensionByOID (CERTCertExtension **extensions,
			 SECItem *oid, SECItem *value);

extern SECStatus
cert_GetExtenCriticality (CERTCertExtension **extensions,
			  int tag, PRBool *isCritical);

extern PRBool
cert_HasUnknownCriticalExten (CERTCertExtension **extensions);

#endif
