




































#include <stdlib.h>
#include <stdarg.h>

#include "prmem.h"
#include "prprf.h"

#include "nsIServiceManager.h"

#include "nsIConsoleService.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsHTMLCanvasElement.h"
#include "nsIPrincipal.h"
#include "nsINode.h"

#include "nsGfxCIID.h"

#include "nsTArray.h"

#include "CanvasUtils.h"
#include "mozilla/gfx/Matrix.h"

namespace mozilla {
namespace CanvasUtils {

void
DoDrawImageSecurityCheck(nsHTMLCanvasElement *aCanvasElement,
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

    PRBool subsumes;
    nsresult rv =
        aCanvasElement->NodePrincipal()->Subsumes(aPrincipal, &subsumes);

    if (NS_SUCCEEDED(rv) && subsumes) {
        
        return;
    }

    aCanvasElement->SetWriteOnly();
}

void
LogMessage (const nsCString& errorString)
{
    nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
    if (!console)
        return;

    console->LogStringMessage(NS_ConvertUTF8toUTF16(errorString).get());
    fprintf(stderr, "%s\n", errorString.get());
}

void
LogMessagef (const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[256];

    nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
    if (console) {
        PR_vsnprintf(buf, 256, fmt, ap);
        console->LogStringMessage(NS_ConvertUTF8toUTF16(nsDependentCString(buf)).get());
        fprintf(stderr, "%s\n", buf);
    }

    va_end(ap);
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
    jsuint length;

    if (JSVAL_IS_PRIMITIVE(val) ||
        !(obj = JSVAL_TO_OBJECT(val)) ||
        !JS_GetArrayLength(cx, obj, &length) ||
        N != length) {
        
        *rv = NS_ERROR_INVALID_ARG;
        return false;
    }

    for (PRUint32 i = 0; i < N; ++i) {
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
    *matrix = Matrix(m.xx, m.yx, m.xy, m.yy, m.x0, m.y0);
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
