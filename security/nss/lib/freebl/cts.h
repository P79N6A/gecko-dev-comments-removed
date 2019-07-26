



#ifndef CTS_H
#define CTS_H 1

#include "blapii.h"

typedef struct CTSContextStr CTSContext;








CTSContext *CTS_CreateContext(void *context, freeblCipherFunc cipher,
			const unsigned char *iv, unsigned int blocksize);

void CTS_DestroyContext(CTSContext *cts, PRBool freeit);

SECStatus CTS_EncryptUpdate(CTSContext *cts, unsigned char *outbuf,
			unsigned int *outlen, unsigned int maxout,
			const unsigned char *inbuf, unsigned int inlen,
			unsigned int blocksize);
SECStatus CTS_DecryptUpdate(CTSContext *cts, unsigned char *outbuf,
			unsigned int *outlen, unsigned int maxout,
			const unsigned char *inbuf, unsigned int inlen,
			unsigned int blocksize);

#endif
