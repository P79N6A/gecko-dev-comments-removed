




#ifndef mozilla_psm_ExtendedValidation_h
#define mozilla_psm_ExtendedValidation_h

#include "certt.h"
#include "prtypes.h"

namespace mozilla { namespace psm {

#ifndef NSS_NO_LIBPKIX
void EnsureIdentityInfoLoaded();
SECStatus GetFirstEVPolicy(CERTCertificate *cert, SECOidTag &outOidTag);
CERTCertList* GetRootsForOid(SECOidTag oid_tag);
void CleanupIdentityInfo();
#endif

} } 

#endif 
