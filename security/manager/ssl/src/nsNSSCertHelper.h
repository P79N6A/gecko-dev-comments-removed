





































#ifndef _NSNSSCERTHELPER_H_
#define _NSNSSCERTHELPER_H_

#include "nsNSSCertHeader.h"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

PRUint32
getCertType(CERTCertificate *cert);

CERTCertNicknames *
getNSSCertNicknamesFromCertList(CERTCertList *certList);

#endif
