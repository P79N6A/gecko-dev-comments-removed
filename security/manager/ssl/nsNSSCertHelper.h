



#ifndef _NSNSSCERTHELPER_H_
#define _NSNSSCERTHELPER_H_

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#include "certt.h"
#include "nsString.h"

uint32_t
getCertType(CERTCertificate *cert);

CERTCertNicknames *
getNSSCertNicknamesFromCertList(CERTCertList *certList);

nsresult
GetCertFingerprintByOidTag(CERTCertificate* nsscert,
                           SECOidTag aOidTag, 
                           nsCString &fp);

#endif
