





































#include "nsGlueLinking.h"
#include "nsXPCOMGlue.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(SUNOS4) || defined(NEXTSTEP) || \
    (defined(OPENBSD) || defined(NETBSD)) && !defined(__ELF__)
#define LEADING_UNDERSCORE "_"
#else
#define LEADING_UNDERSCORE
#endif

struct DependentLib
{
    void         *libHandle;
    DependentLib *next;
};

static DependentLib *sTop;
static void* sXULLibHandle;

static void
AppendDependentLib(void *libHandle)
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
    void *libHandle = dlopen(aDependentLib, RTLD_GLOBAL | RTLD_LAZY);
    if (!libHandle)
        return;

    AppendDependentLib(libHandle);
}

nsresult
XPCOMGlueLoad(const char *xpcomFile, GetFrozenFunctionsFunc *func)
{
    char xpcomDir[MAXPATHLEN];
    if (realpath(xpcomFile, xpcomDir)) {
        char *lastSlash = strrchr(xpcomDir, '/');
        if (lastSlash) {
            *lastSlash = '\0';

            XPCOMGlueLoadDependentLibs(xpcomDir, ReadDependentCB);

            snprintf(lastSlash, MAXPATHLEN - strlen(xpcomDir), "/" XUL_DLL);

            sXULLibHandle = dlopen(xpcomDir, RTLD_GLOBAL | RTLD_LAZY);
        }
    }

    
    

    void *libHandle = nsnull;

    if (xpcomFile[0] != '.' || xpcomFile[1] != '\0') {
        libHandle = dlopen(xpcomFile, RTLD_GLOBAL | RTLD_LAZY);
        if (libHandle) {
            AppendDependentLib(libHandle);
        }
    }

    GetFrozenFunctionsFunc sym =
        (GetFrozenFunctionsFunc) dlsym(libHandle,
                                       LEADING_UNDERSCORE "NS_GetFrozenFunctions");

    if (!sym) { 
        XPCOMGlueUnload();
        return NS_ERROR_NOT_AVAILABLE;
    }

    *func = sym;

    return NS_OK;
}

void
XPCOMGlueUnload()
{
    while (sTop) {
        dlclose(sTop->libHandle);

        DependentLib *temp = sTop;
        sTop = sTop->next;

        delete temp;
    }

    if (sXULLibHandle) {
        dlclose(sXULLibHandle);
        sXULLibHandle = nsnull;
    }
}

nsresult
XPCOMGlueLoadXULFunctions(const nsDynamicFunctionLoad *symbols)
{
    
    

    nsresult rv = NS_OK;
    while (symbols->functionName) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer),
                 LEADING_UNDERSCORE "%s", symbols->functionName);

        *symbols->function = (NSFuncPtr) dlsym(sXULLibHandle, buffer);
        if (!*symbols->function)
            rv = NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;

        ++symbols;
    }
    return rv;
}
