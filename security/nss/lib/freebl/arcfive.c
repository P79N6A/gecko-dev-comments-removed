







































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "blapi.h"
#include "prerror.h"
















RC5Context *
RC5_CreateContext(const SECItem *key, unsigned int rounds,
                  unsigned int wordSize, const unsigned char *iv, int mode)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return NULL;
}






void 
RC5_DestroyContext(RC5Context *cx, PRBool freeit) 
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
}












SECStatus 
RC5_Encrypt(RC5Context *cx, unsigned char *output, unsigned int *outputLen, 
	    unsigned int maxOutputLen, 
	    const unsigned char *input, unsigned int inputLen)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return SECFailure;
}












SECStatus 
RC5_Decrypt(RC5Context *cx, unsigned char *output, unsigned int *outputLen, 
	    unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return SECFailure;
}

