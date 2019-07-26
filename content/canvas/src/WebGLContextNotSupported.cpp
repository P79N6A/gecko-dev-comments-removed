




#include "nsIDOMWebGLRenderingContext.h"
#include "WebGL2Context.h"

#define DUMMY(func,rtype)  nsresult func (rtype ** aResult) { return NS_ERROR_FAILURE; }

DUMMY(NS_NewCanvasRenderingContextWebGL, nsIDOMWebGLRenderingContext)

WebGL2Context * WebGL2Context::Create()
{
    return nullptr;
}
