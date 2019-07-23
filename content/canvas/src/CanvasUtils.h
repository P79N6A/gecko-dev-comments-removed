




































#ifndef _CANVASUTILS_H_
#define _CANVASUTILS_H_

#include "prtypes.h"

class nsICanvasElement;
class nsIPrincipal;

namespace mozilla {

class CanvasUtils {
public:
    

    static PRBool CheckSaneSubrectSize(PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h,
                                       PRInt32 realWidth, PRInt32 realHeight)
    {
        if (w <= 0 || h <= 0 || x < 0 || y < 0)
            return PR_FALSE;

        if (x >= realWidth  || w > (realWidth - x) ||
            y >= realHeight || h > (realHeight - y))
            return PR_FALSE;

        return PR_TRUE;
    }

    
    

    static void DoDrawImageSecurityCheck(nsICanvasElement *aCanvasElement,
                                         nsIPrincipal *aPrincipal,
                                         PRBool forceWriteOnly);

    static void LogMessage (const nsCString& errorString);
    static void LogMessagef (const char *fmt, ...);

private:
    
    CanvasUtils() { }
};

}

#endif 
