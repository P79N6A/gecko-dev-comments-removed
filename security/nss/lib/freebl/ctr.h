



#ifndef CTR_H
#define CTR_H 1

#include "blapii.h"



struct CTRContextStr {
   freeblCipherFunc cipher;
   void *context;
   unsigned char counter[MAX_BLOCK_SIZE];
   unsigned char buffer[MAX_BLOCK_SIZE];
   unsigned long counterBits;
   unsigned int bufPtr;
};

typedef struct CTRContextStr CTRContext;

SECStatus CTR_InitContext(CTRContext *ctr, void *context,
			freeblCipherFunc cipher, const unsigned char *param,
			unsigned int blocksize);








CTRContext * CTR_CreateContext(void *context, freeblCipherFunc cipher,
			const unsigned char *param, unsigned int blocksize);

void CTR_DestroyContext(CTRContext *ctr, PRBool freeit);

SECStatus CTR_Update(CTRContext *ctr, unsigned char *outbuf,
			unsigned int *outlen, unsigned int maxout,
			const unsigned char *inbuf, unsigned int inlen,
			unsigned int blocksize);

#endif
