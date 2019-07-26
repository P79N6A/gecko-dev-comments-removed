




#include <stdlib.h>
#include <stdarg.h>

#include "prprf.h"

#include "nsIServiceManager.h"

#include "nsIConsoleService.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsHTMLCanvasElement.h"
#include "nsIPrincipal.h"

#include "nsGfxCIID.h"

#include "nsTArray.h"

#include "CanvasUtils.h"
#include "mozilla/gfx/Matrix.h"

namespace mozilla {
namespace CanvasUtils {

void
DoDrawImageSecurityCheck(nsHTMLCanvasElement *aCanvasElement,
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

    
    bool subsumes;
    nsresult rv =
        aCanvasElement->NodePrincipal()->SubsumesIgnoringDomain(aPrincipal,
                                                                &subsumes);

    if (NS_SUCCEEDED(rv) && subsumes) {
        
        return;
    }

    aCanvasElement->SetWriteOnly();
}

bool
CoerceDouble(jsval v, double* d)
{
    if (JSVAL_IS_DOUBLE(v)) {
        *d = JSVAL_TO_DOUBLE(v);
    } else if (JSVAL_IS_INT(v)) {
        *d = double(JSVAL_TO_INT(v));
    } else if (JSVAL_IS_VOID(v)) {
        *d = 0.0;
    } else {
        return false;
    }
    return true;
}

template<size_t N>
static bool
JSValToMatrixElts(JSContext* cx, const jsval& val,
                  double* (&elts)[N], nsresult* rv)
{
    JSObject* obj;
    uint32_t length;

    if (JSVAL_IS_PRIMITIVE(val) ||
        !(obj = JSVAL_TO_OBJECT(val)) ||
        !JS_GetArrayLength(cx, obj, &length) ||
        N != length) {
        
        *rv = NS_ERROR_INVALID_ARG;
        return false;
    }

    for (uint32_t i = 0; i < N; ++i) {
        jsval elt;
        double d;
        if (!JS_GetElement(cx, obj, i, &elt)) {
            *rv = NS_ERROR_FAILURE;
            return false;
        }
        if (!CoerceDouble(elt, &d)) {
            *rv = NS_ERROR_INVALID_ARG;
            return false;
        }
        if (!FloatValidate(d)) {
            
            *rv = NS_OK;
            return false;
        }
        *elts[i] = d;
    }

    *rv = NS_OK;
    return true;
}

bool
JSValToMatrix(JSContext* cx, const jsval& val, gfxMatrix* matrix, nsresult* rv)
{
    double* elts[] = { &matrix->xx, &matrix->yx, &matrix->xy, &matrix->yy,
                       &matrix->x0, &matrix->y0 };
    return JSValToMatrixElts(cx, val, elts, rv);
}

bool
JSValToMatrix(JSContext* cx, const jsval& val, Matrix* matrix, nsresult* rv)
{
    gfxMatrix m;
    if (!JSValToMatrix(cx, val, &m, rv))
        return false;
    *matrix = Matrix(Float(m.xx), Float(m.yx), Float(m.xy), Float(m.yy),
                     Float(m.x0), Float(m.y0));
    return true;
}

template<size_t N>
static nsresult
MatrixEltsToJSVal( jsval (&elts)[N], JSContext* cx, jsval* val)
{
    JSObject* obj = JS_NewArrayObject(cx, N, elts);
    if  (!obj) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    *val = OBJECT_TO_JSVAL(obj);

    return NS_OK;
}

nsresult
MatrixToJSVal(const gfxMatrix& matrix, JSContext* cx, jsval* val)
{
    jsval elts[] = {
        DOUBLE_TO_JSVAL(matrix.xx), DOUBLE_TO_JSVAL(matrix.yx),
        DOUBLE_TO_JSVAL(matrix.xy), DOUBLE_TO_JSVAL(matrix.yy),
        DOUBLE_TO_JSVAL(matrix.x0), DOUBLE_TO_JSVAL(matrix.y0)
    };
    return MatrixEltsToJSVal(elts, cx, val);
}

nsresult
MatrixToJSVal(const Matrix& matrix, JSContext* cx, jsval* val)
{
    jsval elts[] = {
        DOUBLE_TO_JSVAL(matrix._11), DOUBLE_TO_JSVAL(matrix._12),
        DOUBLE_TO_JSVAL(matrix._21), DOUBLE_TO_JSVAL(matrix._22),
        DOUBLE_TO_JSVAL(matrix._31), DOUBLE_TO_JSVAL(matrix._32)
    };
    return MatrixEltsToJSVal(elts, cx, val);
}

} 
} 
