







































#include "prtypes.h"
#include "prinit.h"
#include "seccomon.h"
#include "secerr.h"
#include "ssl.h"
#include "sslimpl.h"

static int ssl_inited = 0;

SECStatus
ssl_Init(void)
{
    if (!ssl_inited) {
	if (ssl_InitializePRErrorTable() != SECSuccess) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    return (SECFailure);
	}
	ssl_inited = 1;
    }
    return SECSuccess;
}
