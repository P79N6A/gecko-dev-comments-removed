







































#include "prtypes.h"
#include "prinit.h"
#include "seccomon.h"
#include "secerr.h"
#include "ssl.h"
#include "sslerrstrs.h"

static int ssl_inited = 0;

SECStatus
ssl_Init(void)
{
    if (!ssl_inited) {
	if (ssl_InitializePRErrorTable() == PR_FAILURE) {
	   return (SEC_ERROR_NO_MEMORY);
	}
	ssl_inited = 1;
    }
    return SECSuccess;
}
