













#ifndef _SECCOMMON_H_
#define _SECCOMMON_H_

#include "utilrename.h"
#include "prtypes.h"


#ifdef __cplusplus 
# define SEC_BEGIN_PROTOS extern "C" {
# define SEC_END_PROTOS }
#else
# define SEC_BEGIN_PROTOS
# define SEC_END_PROTOS
#endif

#include "secport.h"

typedef enum {
    siBuffer = 0,
    siClearDataBuffer = 1,
    siCipherDataBuffer = 2,
    siDERCertBuffer = 3,
    siEncodedCertBuffer = 4,
    siDERNameBuffer = 5,
    siEncodedNameBuffer = 6,
    siAsciiNameString = 7,
    siAsciiString = 8,
    siDEROID = 9,
    siUnsignedInteger = 10,
    siUTCTime = 11,
    siGeneralizedTime = 12,
    siVisibleString = 13,
    siUTF8String = 14,
    siBMPString = 15
} SECItemType;

typedef struct SECItemStr SECItem;

struct SECItemStr {
    SECItemType type;
    unsigned char *data;
    unsigned int len;
};












typedef enum _SECStatus {
    SECWouldBlock = -2,
    SECFailure = -1,
    SECSuccess = 0
} SECStatus;






typedef enum _SECComparison {
    SECLessThan = -1,
    SECEqual = 0,
    SECGreaterThan = 1
} SECComparison;

#endif 
