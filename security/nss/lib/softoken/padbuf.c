


#include "blapit.h"
#include "secport.h"
#include "secerr.h"









unsigned char *
CBC_PadBuffer(PRArenaPool *arena, unsigned char *inbuf, unsigned int inlen,
	      unsigned int *outlen, int blockSize)
{
    unsigned char *outbuf;
    unsigned int   des_len;
    unsigned int   i;
    unsigned char  des_pad_len;

    




    des_len = (inlen + blockSize) & ~(blockSize - 1);

    if (arena != NULL) {
	outbuf = (unsigned char*)PORT_ArenaGrow (arena, inbuf, inlen, des_len);
    } else {
	outbuf = (unsigned char*)PORT_Realloc (inbuf, des_len);
    }

    if (outbuf == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    des_pad_len = des_len - inlen;
    for (i = inlen; i < des_len; i++)
	outbuf[i] = des_pad_len;

    *outlen = des_len;
    return outbuf;
}
