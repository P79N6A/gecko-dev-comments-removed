




#ifndef mozilla_psm_ExtendedValidation_h
#define mozilla_psm_ExtendedValidation_h

#include "certt.h"
#include "prtypes.h"

namespace mozilla { namespace psm {

#ifndef MOZ_NO_EV_CERTS
void EnsureIdentityInfoLoaded();
void CleanupIdentityInfo();
SECStatus GetFirstEVPolicy(CERTCertificate* cert, SECOidTag& outOidTag);



bool CertIsAuthoritativeForEVPolicy(const CERTCertificate* cert,
                                    SECOidTag policyOidTag);
#endif

#ifndef NSS_NO_LIBPKIX
CERTCertList* GetRootsForOid(SECOidTag oid_tag);
#endif

} } 

#endif 
