






#ifndef _BLAPII_H_
#define _BLAPII_H_

#include "blapit.h"


#define MAX_BLOCK_SIZE 16

typedef SECStatus (*freeblCipherFunc)(void *cx, unsigned char *output,
                          unsigned int *outputLen, unsigned int maxOutputLen,
                          const unsigned char *input, unsigned int inputLen,
			  unsigned int blocksize);
typedef void (*freeblDestroyFunc)(void *cx, PRBool freeit);

SEC_BEGIN_PROTOS

#if defined(XP_UNIX) && !defined(NO_FORK_CHECK)

extern PRBool bl_parentForkedAfterC_Initialize;

#define SKIP_AFTER_FORK(x) if (!bl_parentForkedAfterC_Initialize) x

#else

#define SKIP_AFTER_FORK(x) x

#endif

SEC_END_PROTOS

#endif 

