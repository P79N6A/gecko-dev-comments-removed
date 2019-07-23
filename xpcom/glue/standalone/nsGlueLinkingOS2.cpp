





































#include "nsGlueLinking.h"
#include "nsXPCOMGlue.h"

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>

struct DependentLib
{
    HMODULE     libHandle;
    DependentLib *next;
};

static DependentLib *sTop;
HMODULE sXULLibrary = NULLHANDLE;

static void
AppendDependentLib(HMODULE libHandle)
{
    DependentLib *d = new DependentLib;
    if (!d)
        return;

    d->next = sTop;
    d->libHandle = libHandle;

    sTop = d;
}

static void
ReadDependentCB(const char *aDependentLib)
{
    CHAR pszError[_MAX_PATH];
    ULONG ulrc = NO_ERROR;
    HMODULE h;

    ulrc = DosLoadModule(pszError, _MAX_PATH, aDependentLib, &h);

    if (ulrc != NO_ERROR)
        return;

    AppendDependentLib(h);
}


static char*
ns_strrpbrk(char *string, const char *strCharSet)
{
    char *found = NULL;
    for (; *string; ++string) {
        for (const char *search = strCharSet; *search; ++search) {
            if (*search == *string) {
                found = string;
                
                
            }
        }
    }

    return found;
}

GetFrozenFunctionsFunc
XPCOMGlueLoad(const char *xpcomFile)
{
    CHAR pszError[_MAX_PATH];
    ULONG ulrc = NO_ERROR;
    HMODULE h;

    if (xpcomFile[0] == '.' && xpcomFile[1] == '\0') {
        xpcomFile = XPCOM_DLL;
    }
    else {
        char xpcomDir[MAXPATHLEN];

        _fullpath(xpcomDir, xpcomFile, sizeof(xpcomDir));
        char *lastSlash = ns_strrpbrk(xpcomDir, "/\\");
        if (lastSlash) {
            *lastSlash = '\0';

            XPCOMGlueLoadDependentLibs(xpcomDir, ReadDependentCB);

            snprintf(lastSlash, MAXPATHLEN - strlen(xpcomDir), "\\" XUL_DLL);

            DosLoadModule(pszError, _MAX_PATH, xpcomDir, &sXULLibrary);
        }
    }

    ulrc = DosLoadModule(pszError, _MAX_PATH, xpcomFile, &h);

    if (ulrc != NO_ERROR)
        return nsnull;

    AppendDependentLib(h);

    GetFrozenFunctionsFunc sym;

    ulrc = DosQueryProcAddr(h, 0, "_NS_GetFrozenFunctions", (PFN*)&sym);

    if (ulrc != NO_ERROR)
        XPCOMGlueUnload();

    return sym;
}

void
XPCOMGlueUnload()
{
    while (sTop) {
        DosFreeModule(sTop->libHandle);

        DependentLib *temp = sTop;
        sTop = sTop->next;

        delete temp;
    }

    if (sXULLibrary) {
        DosFreeModule(sXULLibrary);
        sXULLibrary = nsnull;
    }
}

nsresult
XPCOMGlueLoadXULFunctions(const nsDynamicFunctionLoad *symbols)
{
    ULONG ulrc = NO_ERROR;

    if (!sXULLibrary)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv = NS_OK;
    while (symbols->functionName) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "_%s", symbols->functionName);
        ulrc = DosQueryProcAddr(sXULLibrary, 0, buffer, (PFN*)symbols->function);

        if (ulrc != NO_ERROR)
            rv = NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;

        ++symbols;
    }

    return rv;
}
