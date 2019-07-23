





































#include "nsICanvasRenderingContextWebGL.h"
#include "WebGLArray.h"

#define DUMMY(func,rtype)  nsresult func (rtype ** aResult) { return NS_ERROR_FAILURE; }

DUMMY(NS_NewCanvasRenderingContextWebGL, nsICanvasRenderingContextWebGL)
DUMMY(NS_NewCanvasFloatArray, nsISupports)
DUMMY(NS_NewCanvasByteArray, nsISupports)
DUMMY(NS_NewCanvasUnsignedByteArray, nsISupports)
DUMMY(NS_NewCanvasShortArray, nsISupports)
DUMMY(NS_NewCanvasUnsignedShortArray, nsISupports)
DUMMY(NS_NewCanvasIntArray, nsISupports)
DUMMY(NS_NewCanvasUnsignedIntArray, nsISupports)
