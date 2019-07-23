









































#ifndef _BLAPIT_H_
#define _BLAPIT_H_

#include "base/third_party/nspr/prtypes.h"












typedef enum _SECStatus {
    SECWouldBlock = -2,
    SECFailure = -1,
    SECSuccess = 0
} SECStatus;

#define SHA256_LENGTH 		32 	/* bytes */
#define SHA384_LENGTH 		48 	/* bytes */
#define SHA512_LENGTH 		64 	/* bytes */
#define HASH_LENGTH_MAX         SHA512_LENGTH





#define SHA256_BLOCK_LENGTH      64     /* bytes */
#define SHA384_BLOCK_LENGTH     128     /* bytes */
#define SHA512_BLOCK_LENGTH     128     /* bytes */
#define HASH_BLOCK_LENGTH_MAX   SHA512_BLOCK_LENGTH





struct SHA256ContextStr     ;
struct SHA512ContextStr     ;

typedef struct SHA256ContextStr     SHA256Context;
typedef struct SHA512ContextStr     SHA512Context;

typedef struct SHA512ContextStr     SHA384Context;

#endif 
