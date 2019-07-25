




































#ifndef _CANVASUTILS_H_
#define _CANVASUTILS_H_

#include "prtypes.h"

#include "CheckedInt.h"

class nsHTMLCanvasElement;
class nsIPrincipal;

namespace mozilla {

class CanvasUtils {
public:
    

    static PRBool CheckSaneSubrectSize(PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h,
                                       PRInt32 realWidth, PRInt32 realHeight) {
        CheckedInt32 checked_x_plus_w  = CheckedInt32(x) + w;
        CheckedInt32 checked_y_plus_h  = CheckedInt32(y) + h;

        return w >= 0 && h >= 0 && x >= 0 && y >= 0 &&
            checked_x_plus_w.valid() &&
            checked_x_plus_w.value() <= realWidth &&
            checked_y_plus_h.valid() &&
            checked_y_plus_h.value() <= realHeight;
    }

    
    

    static void DoDrawImageSecurityCheck(nsHTMLCanvasElement *aCanvasElement,
                                         nsIPrincipal *aPrincipal,
                                         PRBool forceWriteOnly);

    static void LogMessage (const nsCString& errorString);
    static void LogMessagef (const char *fmt, ...);

private:
    
    CanvasUtils() { }
};

}

#endif 
