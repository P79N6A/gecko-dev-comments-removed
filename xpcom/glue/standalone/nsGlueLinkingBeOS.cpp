





































#include "nsGlueLinking.h"
#include "nsXPCOMGlue.h"

#include <kernel/image.h>
#include <stdlib.h>
#include <stdio.h>
#include <Entry.h>
#include <Path.h>

#define LEADING_UNDERSCORE


struct DependentLib
{
    image_id         libHandle;
    DependentLib *next;
};

static DependentLib *sTop;
static image_id sXULLibHandle;

static void
AppendDependentLib(image_id libHandle)
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
    image_id libHandle = load_add_on(aDependentLib);
    if (!libHandle)
        return;

    AppendDependentLib(libHandle);
}

GetFrozenFunctionsFunc
XPCOMGlueLoad(const char *xpcomFile)
{
    char xpcomDir[MAXPATHLEN];
	BPath libpath;
    if (libpath.SetTo(xpcomFile, 0, true) == B_OK) {
        sprintf(xpcomDir, libpath.Path());
        char *lastSlash = strrchr(xpcomDir, '/');
        if (lastSlash) {
            *lastSlash = '\0';

            XPCOMGlueLoadDependentLibs(xpcomDir, ReadDependentCB);

            snprintf(lastSlash, MAXPATHLEN - strlen(xpcomDir), "/" XUL_DLL);

            sXULLibHandle = load_add_on(xpcomDir);
        }
    }


    image_id libHandle = nsnull;

    if (xpcomFile[0] != '.' || xpcomFile[1] != '\0') {
        libHandle = load_add_on(xpcomFile);
        if (libHandle) {
            AppendDependentLib(libHandle);
        }
    }

    GetFrozenFunctionsFunc sym = 0;
    status_t result;
    
    result = get_image_symbol(libHandle,
                              LEADING_UNDERSCORE "NS_GetFrozenFunctions", B_SYMBOL_TYPE_TEXT, (void **)sym);

    if (!sym || B_OK != result)
        XPCOMGlueUnload();

    return sym;
}

void
XPCOMGlueUnload()
{
    while (sTop) {
        unload_add_on(sTop->libHandle);

        DependentLib *temp = sTop;
        sTop = sTop->next;

        delete temp;
    }

    if (sXULLibHandle) {
        unload_add_on(sXULLibHandle);
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

        *symbols->function = 0;
        status_t result;
        result = get_image_symbol(sXULLibHandle, buffer, B_SYMBOL_TYPE_TEXT, (void **)*symbols->function );
        if (!*symbols->function || B_OK != result)
            rv = NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;

        ++symbols;
    }
    return rv;
}
