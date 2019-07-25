





































#include "nsIDOMWebGLRenderingContext.h"
#include "nsDOMClassInfoID.h"

#define DUMMY(func,rtype)  nsresult func (rtype ** aResult) { return NS_ERROR_FAILURE; }

DUMMY(NS_NewCanvasRenderingContextWebGL, nsIDOMWebGLRenderingContext)

DOMCI_DATA(WebGLRenderingContext, void)
DOMCI_DATA(WebGLBuffer, void)
DOMCI_DATA(WebGLTexture, void)
DOMCI_DATA(WebGLProgram, void)
DOMCI_DATA(WebGLShader, void)
DOMCI_DATA(WebGLFramebuffer, void)
DOMCI_DATA(WebGLRenderbuffer, void)
DOMCI_DATA(WebGLUniformLocation, void)
DOMCI_DATA(WebGLActiveInfo, void)
DOMCI_DATA(WebGLExtension, void)
DOMCI_DATA(WebGLExtensionLoseContext, void)
