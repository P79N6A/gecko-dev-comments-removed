





































#include "nsICanvasRenderingContextWebGL.h"
#include "WebGLArray.h"

#define DUMMY(func,rtype)  nsresult func (rtype ** aResult) { return NS_ERROR_FAILURE; }

DUMMY(NS_NewCanvasRenderingContextWebGL, nsICanvasRenderingContextWebGL)
DUMMY(NS_NewWebGLFloatArray, nsISupports)
DUMMY(NS_NewWebGLByteArray, nsISupports)
DUMMY(NS_NewWebGLUnsignedByteArray, nsISupports)
DUMMY(NS_NewWebGLShortArray, nsISupports)
DUMMY(NS_NewWebGLUnsignedShortArray, nsISupports)
DUMMY(NS_NewWebGLIntArray, nsISupports)
DUMMY(NS_NewWebGLUnsignedIntArray, nsISupports)
DUMMY(NS_NewWebGLArrayBuffer, nsISupports)
