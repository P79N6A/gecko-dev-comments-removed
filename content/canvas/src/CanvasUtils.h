




































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

template<typename T>
nsresult
JSValToDashArray(JSContext* cx, const jsval& val,
                 FallibleTArray<T>& dashArray);

template<typename T>
nsresult
DashArrayToJSVal(FallibleTArray<T>& dashArray,
                 JSContext* cx, jsval* val);

template<typename T>
nsresult
JSValToDashArray(JSContext* cx, const jsval& patternArray,
                 FallibleTArray<T>& dashes)
{
    
    
    static const jsuint MAX_NUM_DASHES = 1 << 14;

    if (!JSVAL_IS_PRIMITIVE(patternArray)) {
        JSObject* obj = JSVAL_TO_OBJECT(patternArray);
        jsuint length;
        if (!JS_GetArrayLength(cx, obj, &length)) {
            
            return NS_ERROR_INVALID_ARG;
        } else if (length > MAX_NUM_DASHES) {
            
            return NS_ERROR_ILLEGAL_VALUE;
        }

        bool haveNonzeroElement = false;
        for (jsint i = 0; i < jsint(length); ++i) {
            jsval elt;
            double d;
            if (!JS_GetElement(cx, obj, i, &elt)) {
                return NS_ERROR_FAILURE;
            }
            if (!(CoerceDouble(elt, &d) &&
                  FloatValidate(d) &&
                  d >= 0.0)) {
                
                return NS_ERROR_INVALID_ARG;
            } else if (d > 0.0) {
                haveNonzeroElement = true;
            }
            if (!dashes.AppendElement(d)) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }

        if (dashes.Length() > 0 && !haveNonzeroElement) {
            
            return NS_ERROR_ILLEGAL_VALUE;
        }
    } else if (!(JSVAL_IS_VOID(patternArray) || JSVAL_IS_NULL(patternArray))) {
        
        
        return NS_ERROR_INVALID_ARG;
    }

    return NS_OK;
}

template<typename T>
nsresult
DashArrayToJSVal(FallibleTArray<T>& dashes,
                 JSContext* cx, jsval* val)
{
    if (dashes.IsEmpty()) {
        *val = JSVAL_NULL;
    } else {
        JSObject* obj = JS_NewArrayObject(cx, dashes.Length(), nsnull);
        if (!obj) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        for (PRUint32 i = 0; i < dashes.Length(); ++i) {
            double d = dashes[i];
            jsval elt = DOUBLE_TO_JSVAL(d);
            if (!JS_SetElement(cx, obj, i, &elt)) {
                return NS_ERROR_FAILURE;
            }
        }
        *val = OBJECT_TO_JSVAL(obj);
    }
    return NS_OK;
}

}
}

#endif 
