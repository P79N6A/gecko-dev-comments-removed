




































#include "prmem.h"

#include "nsIServiceManager.h"

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
