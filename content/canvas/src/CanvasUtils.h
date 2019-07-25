




































#ifndef _CANVASUTILS_H_
#define _CANVASUTILS_H_

#include "prtypes.h"

#include "CheckedInt.h"

class nsHTMLCanvasElement;
class nsIPrincipal;

namespace mozilla {

namespace gfx {
class Matrix;
}

namespace CanvasUtils {

using namespace gfx;



inline PRBool CheckSaneSubrectSize(PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h,
                            PRInt32 realWidth, PRInt32 realHeight) {
    CheckedInt32 checked_xmost  = CheckedInt32(x) + w;
    CheckedInt32 checked_ymost  = CheckedInt32(y) + h;

    return w >= 0 && h >= 0 && x >= 0 && y >= 0 &&
        checked_xmost.valid() &&
        checked_xmost.value() <= realWidth &&
        checked_ymost.valid() &&
        checked_ymost.value() <= realHeight;
}




void DoDrawImageSecurityCheck(nsHTMLCanvasElement *aCanvasElement,
                              nsIPrincipal *aPrincipal,
                              PRBool forceWriteOnly);

void LogMessage (const nsCString& errorString);
void LogMessagef (const char *fmt, ...);




bool CoerceDouble(jsval v, double* d);



bool JSValToMatrix(JSContext* cx, const jsval& val,
                   gfxMatrix* matrix, nsresult* rv);
bool JSValToMatrix(JSContext* cx, const jsval& val,
                   Matrix* matrix, nsresult* rv);

nsresult MatrixToJSVal(const gfxMatrix& matrix,
                       JSContext* cx, jsval* val);
nsresult MatrixToJSVal(const Matrix& matrix,
                       JSContext* cx, jsval* val);

    
#define VALIDATE(_f)  if (!NS_finite(_f)) return PR_FALSE

inline PRBool FloatValidate (double f1) {
    VALIDATE(f1);
    return PR_TRUE;
}

inline PRBool FloatValidate (double f1, double f2) {
    VALIDATE(f1); VALIDATE(f2);
    return PR_TRUE;
}

inline PRBool FloatValidate (double f1, double f2, double f3) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3);
    return PR_TRUE;
}

inline PRBool FloatValidate (double f1, double f2, double f3, double f4) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4);
    return PR_TRUE;
}

inline PRBool FloatValidate (double f1, double f2, double f3, double f4, double f5) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4); VALIDATE(f5);
    return PR_TRUE;
}

inline PRBool FloatValidate (double f1, double f2, double f3, double f4, double f5, double f6) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4); VALIDATE(f5); VALIDATE(f6);
    return PR_TRUE;
}

#undef VALIDATE

}
}

#endif 
