










#include "cert.h"
#include "ssl.h"
#include "keyhi.h"
#include "secitem.h"
#include "sslimpl.h"
#include "pkcs11t.h"
#include "preenc.h"
#include "pk11func.h"

PEHeader *SSL_PreencryptedStreamToFile(PRFileDesc *fd, PEHeader *inHeader, 
				       int *headerSize)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return NULL;
}

PEHeader *SSL_PreencryptedFileToStream(PRFileDesc *fd, PEHeader *header, 
							int *headerSize)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return NULL;
}


