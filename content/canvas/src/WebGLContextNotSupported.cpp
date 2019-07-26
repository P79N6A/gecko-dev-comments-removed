




#include "nsIDOMWebGLRenderingContext.h"
#include "nsDOMClassInfoID.h"

#define DUMMY(func,rtype)  nsresult func (rtype ** aResult) { return NS_ERROR_FAILURE; }

DUMMY(NS_NewCanvasRenderingContextWebGL, nsIDOMWebGLRenderingContext)

DOMCI_DATA(WebGLBuffer, void)
DOMCI_DATA(WebGLTexture, void)
DOMCI_DATA(WebGLProgram, void)
DOMCI_DATA(WebGLShader, void)
DOMCI_DATA(WebGLFramebuffer, void)
DOMCI_DATA(WebGLRenderbuffer, void)
DOMCI_DATA(WebGLUniformLocation, void)
DOMCI_DATA(WebGLShaderPrecisionFormat, void)
DOMCI_DATA(WebGLActiveInfo, void)
DOMCI_DATA(WebGLExtension, void)
DOMCI_DATA(WebGLExtensionStandardDerivatives, void)
DOMCI_DATA(WebGLExtensionTextureFilterAnisotropic, void)
DOMCI_DATA(WebGLExtensionLoseContext, void)
DOMCI_DATA(WebGLExtensionCompressedTextureS3TC, void)
DOMCI_DATA(WebGLExtensionCompressedTextureATC, void)
DOMCI_DATA(WebGLExtensionCompressedTexturePVRTC, void)
DOMCI_DATA(WebGLExtensionDepthTexture, void)
