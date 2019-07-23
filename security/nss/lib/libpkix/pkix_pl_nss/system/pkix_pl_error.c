










































#include "pkix_pl_common.h"

#undef PKIX_ERRORENTRY

#define PKIX_ERRORENTRY(name,desc,plerr) plerr

const SECErrorCodes PKIX_PLErrorIndex[] =
{
#include "pkix_errorstrings.h"
};

int
PKIX_PL_GetPLErrorCode()
{
    return PORT_GetError();
}
