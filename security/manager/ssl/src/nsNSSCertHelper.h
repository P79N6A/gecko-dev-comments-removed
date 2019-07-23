





































#ifndef _NSNSSCERTHELPER_H_
#define _NSNSSCERTHELPER_H_

#include "nsNSSCertHeader.h"

PRUint32
getCertType(CERTCertificate *cert);

CERTCertNicknames *
getNSSCertNicknamesFromCertList(CERTCertList *certList);

#endif
