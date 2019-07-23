



































#ifndef _PK11SDR_H_
#define _PK11SDR_H_

#include "seccomon.h"

SEC_BEGIN_PROTOS





SECStatus
PK11SDR_Encrypt(SECItem *keyid, SECItem *data, SECItem *result, void *cx);





SECStatus
PK11SDR_Decrypt(SECItem *data, SECItem *result, void *cx);

SEC_END_PROTOS

#endif
