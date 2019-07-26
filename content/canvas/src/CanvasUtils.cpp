




#include <stdlib.h>
#include <stdarg.h>

#include "prprf.h"

#include "nsIServiceManager.h"

#include "nsIConsoleService.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsIHTMLCollection.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsIPrincipal.h"

#include "nsGfxCIID.h"

#include "nsTArray.h"

#include "CanvasUtils.h"
#include "mozilla/gfx/Matrix.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace CanvasUtils {

void
DoDrawImageSecurityCheck(dom::HTMLCanvasElement *aCanvasElement,
                         nsIPrincipal *aPrincipal,
                         bool forceWriteOnly,
                         bool CORSUsed)
{
    NS_PRECONDITION(aPrincipal, "Must have a principal here");

    
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

    
    if (CORSUsed)
        return;

    if (aCanvasElement->NodePrincipal()->Subsumes(aPrincipal)) {
        
        return;
    }

    aCanvasElement->SetWriteOnly();
}

bool
CoerceDouble(JS::Value v, double* d)
{
    if (v.isDouble()) {
        *d = v.toDouble();
    } else if (v.isInt32()) {
        *d = double(v.toInt32());
    } else if (v.isUndefined()) {
        *d = 0.0;
    } else {
        return false;
    }
    return true;
}

} 
} 
