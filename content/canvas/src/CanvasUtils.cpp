




































#include <stdlib.h>
#include <stdarg.h>

#include "prmem.h"

#include "nsIServiceManager.h"

#include "nsIConsoleService.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsICanvasElement.h"
#include "nsIPrincipal.h"
#include "nsINode.h"

#include "nsGfxCIID.h"

#include "nsTArray.h"

#include "CanvasUtils.h"

using namespace mozilla;

void
CanvasUtils::DoDrawImageSecurityCheck(nsICanvasElement *aCanvasElement,
                                      nsIPrincipal *aPrincipal,
                                      PRBool forceWriteOnly)
{
    
    if (!aCanvasElement) {
        NS_WARNING("DoDrawImageSecurityCheck called without canvas element!");
        return;
    }

    if (aCanvasElement->IsWriteOnly())
        return;

    
    if (forceWriteOnly) {
        aCanvasElement->SetWriteOnly();
        return;
    }

    if (aPrincipal == nsnull)
        return;

    nsCOMPtr<nsINode> elem = do_QueryInterface(aCanvasElement);
    if (elem) { 
        PRBool subsumes;
        nsresult rv =
            elem->NodePrincipal()->Subsumes(aPrincipal, &subsumes);
            
        if (NS_SUCCEEDED(rv) && subsumes) {
            
            return;
        }
    }

    aCanvasElement->SetWriteOnly();
}

void
CanvasUtils::LogMessage (const nsCString& errorString)
{
    nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
    if (!console)
        return;

    console->LogStringMessage(NS_ConvertUTF8toUTF16(errorString).get());
    fprintf(stderr, "%s\n", errorString.get());
}

void
CanvasUtils::LogMessagef (const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[256];

    nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
    if (console) {
        vsnprintf(buf, 256, fmt, ap);
        console->LogStringMessage(NS_ConvertUTF8toUTF16(nsDependentCString(buf)).get());
        fprintf(stderr, "%s\n", buf);
    }

    va_end(ap);
}
