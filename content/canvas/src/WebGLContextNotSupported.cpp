





































#include "nsICanvasRenderingContextWebGL.h"
#include "nsDOMClassInfoID.h"

#define DUMMY(func,rtype)  nsresult func (rtype ** aResult) { return NS_ERROR_FAILURE; }

DUMMY(NS_NewCanvasRenderingContextWebGL, nsICanvasRenderingContextWebGL)

DOMCI_DATA(CanvasRenderingContextWebGL, void)
