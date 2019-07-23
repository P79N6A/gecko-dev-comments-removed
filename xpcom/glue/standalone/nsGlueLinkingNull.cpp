





































#include "nsGlueLinking.h"

#include <stdio.h>

GetFrozenFunctionsFunc
XPCOMGlueLoad(const char *xpcomFile)
{
    fprintf(stderr, "XPCOM glue dynamic linking is not implemented on this platform!");
    return nsnull;
}

void
XPCOMGlueUnload()
{
}
