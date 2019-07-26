




#ifndef _CANVASUTILS_H_
#define _CANVASUTILS_H_

#include "mozilla/CheckedInt.h"

class nsIPrincipal;

namespace mozilla {

namespace gfx {
class Matrix;
}

namespace dom {
class HTMLCanvasElement;
}

namespace CanvasUtils {




inline bool CheckSaneSubrectSize(int32_t x, int32_t y, int32_t w, int32_t h,
                            int32_t realWidth, int32_t realHeight) {
    CheckedInt32 checked_xmost  = CheckedInt32(x) + w;
    CheckedInt32 checked_ymost  = CheckedInt32(y) + h;

    return w >= 0 && h >= 0 && x >= 0 && y >= 0 &&
        checked_xmost.isValid() &&
        checked_xmost.value() <= realWidth &&
        checked_ymost.isValid() &&
        checked_ymost.value() <= realHeight;
}




void DoDrawImageSecurityCheck(dom::HTMLCanvasElement *aCanvasElement,
                              nsIPrincipal *aPrincipal,
                              bool forceWriteOnly,
                              bool CORSUsed);




bool CoerceDouble(JS::Value v, double* d);

    
#define VALIDATE(_f)  if (!NS_finite(_f)) return false

inline bool FloatValidate (double f1) {
    VALIDATE(f1);
    return true;
}

inline bool FloatValidate (double f1, double f2) {
    VALIDATE(f1); VALIDATE(f2);
    return true;
}

inline bool FloatValidate (double f1, double f2, double f3) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3);
    return true;
}

inline bool FloatValidate (double f1, double f2, double f3, double f4) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4);
    return true;
}

inline bool FloatValidate (double f1, double f2, double f3, double f4, double f5) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4); VALIDATE(f5);
    return true;
}

inline bool FloatValidate (double f1, double f2, double f3, double f4, double f5, double f6) {
    VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4); VALIDATE(f5); VALIDATE(f6);
    return true;
}

#undef VALIDATE

template<typename T>
nsresult
JSValToDashArray(JSContext* cx, const JS::Value& val,
                 FallibleTArray<T>& dashArray);

template<typename T>
nsresult
DashArrayToJSVal(FallibleTArray<T>& dashArray,
                 JSContext* cx, JS::Value* val);

template<typename T>
nsresult
JSValToDashArray(JSContext* cx, const JS::Value& patternArray,
                 FallibleTArray<T>& dashes)
{
    
    
    static const uint32_t MAX_NUM_DASHES = 1 << 14;

    if (!JSVAL_IS_PRIMITIVE(patternArray)) {
        JSObject* obj = JSVAL_TO_OBJECT(patternArray);
        uint32_t length;
        if (!JS_GetArrayLength(cx, obj, &length)) {
            
            return NS_ERROR_INVALID_ARG;
        } else if (length > MAX_NUM_DASHES) {
            
            return NS_ERROR_ILLEGAL_VALUE;
        }

        bool haveNonzeroElement = false;
        for (uint32_t i = 0; i < length; ++i) {
            JS::Value elt;
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
                 JSContext* cx, JS::Value* val)
{
    if (dashes.IsEmpty()) {
        *val = JSVAL_NULL;
    } else {
        JSObject* obj = JS_NewArrayObject(cx, dashes.Length(), nullptr);
        if (!obj) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        for (uint32_t i = 0; i < dashes.Length(); ++i) {
            double d = dashes[i];
            JS::Value elt = DOUBLE_TO_JSVAL(d);
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
