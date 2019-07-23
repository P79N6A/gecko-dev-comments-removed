





































#include "nsGlueLinking.h"

#include <stdio.h>

nsresult
XPCOMGlueLoad(const char *xpcomFile, GetFrozenFunctionsFunc *func)
{
    fprintf(stderr, "XPCOM glue dynamic linking is not implemented on this platform!");

    *func = nsnull;

    return NS_ERROR_NOT_IMPLEMENTED;
}

void
XPCOMGlueUnload()
{
}
