




































#ifndef _CANVASUTILS_H_
#define _CANVASUTILS_H_

#include "prtypes.h"
#include "nsContentUtils.h"
#include "nsICanvasElement.h"
#include "nsIPrincipal.h"
#include "nsIDOMElement.h"
#include "nsRect.h"

#include "gfxASurface.h"

namespace mozilla {

class CanvasUtils {
public:
    

    static PRBool CheckSaneSubrectSize(PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h,
                                       PRInt32 realWidth, PRInt32 realHeight)
    {
        return nsIntRect(0, 0, realWidth, realHeight).Contains(nsIntRect(x, y, w, h));
    }

    
    

    static void DoDrawImageSecurityCheck(nsICanvasElement *aCanvasElement,
                                         nsIPrincipal *aPrincipal,
                                         PRBool forceWriteOnly);

private:
    
    CanvasUtils() { }
};

}

#endif 
