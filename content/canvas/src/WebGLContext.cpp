






































#include "WebGLContext.h"

#include "nsIConsoleService.h"
#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsDOMError.h"

#include "gfxContext.h"
#include "gfxPattern.h"

#include "CanvasUtils.h"
#include "NativeJSContext.h"

#include "GLContextProvider.h"

using namespace mozilla;
using namespace mozilla::gl;

nsresult NS_NewCanvasRenderingContextWebGL(nsICanvasRenderingContextWebGL** aResult);

nsresult
NS_NewCanvasRenderingContextWebGL(nsICanvasRenderingContextWebGL** aResult)
{
    nsICanvasRenderingContextWebGL* ctx = new WebGLContext();
    if (!ctx)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = ctx);
    return NS_OK;
}

WebGLContext::WebGLContext()
    : mCanvasElement(nsnull),
      gl(nsnull),
      mWidth(0), mHeight(0),
      mInvalidated(PR_FALSE),
      mActiveTexture(0),
      mSynthesizedGLError(LOCAL_GL_NO_ERROR)
{
    mMapBuffers.Init();
    mMapTextures.Init();
    mMapPrograms.Init();
    mMapShaders.Init();
    mMapFramebuffers.Init();
    mMapRenderbuffers.Init();
}

WebGLContext::~WebGLContext()
{
}

void
WebGLContext::Invalidate()
{
    if (!mCanvasElement)
        return;

    if (mInvalidated)
        return;

    mInvalidated = true;
    mCanvasElement->InvalidateFrame();
}





NS_IMETHODIMP
WebGLContext::SetCanvasElement(nsHTMLCanvasElement* aParentCanvas)
{
    if (aParentCanvas == nsnull) {
        
        
        return NS_OK;
    }

    if (!SafeToCreateCanvas3DContext(aParentCanvas))
        return NS_ERROR_FAILURE;

    mCanvasElement = aParentCanvas;

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::SetDimensions(PRInt32 width, PRInt32 height)
{
    if (mWidth == width && mHeight == height)
        return NS_OK;

    LogMessage("Canvas 3D: creating PBuffer...");

    GLContextProvider::ContextFormat format(GLContextProvider::ContextFormat::BasicRGBA32);
    format.depth = 16;
    format.minDepth = 1;

    gl = gl::sGLContextProvider.CreatePBuffer(gfxIntSize(width, height), format);

    if (!gl) {
        LogMessage("Canvas 3D: can't get a native PBuffer, trying OSMesa...");
        gl = gl::GLContextProviderOSMesa::CreatePBuffer(gfxIntSize(width, height), format);
        if (!gl) {
            LogMessage("Canvas 3D: can't create a OSMesa pseudo-PBuffer.");
            return NS_ERROR_FAILURE;
        }
    }

    if (!ValidateGL()) {
        LogMessage("Canvas 3D: Couldn't validate OpenGL implementation; is everything needed present?");
        return NS_ERROR_FAILURE;
    }

    LogMessage("Canvas 3D: ready");

    mWidth = width;
    mHeight = height;

    
    
#if 0
    int err = glGetError();
    if (err) {
        printf ("error before MakeContextCurrent! 0x%04x\n", err);
    }
#endif

    MakeContextCurrent();
    gl->fViewport(0, 0, mWidth, mHeight);
    gl->fClearColor(0, 0, 0, 0);
    gl->fClear(LOCAL_GL_COLOR_BUFFER_BIT | LOCAL_GL_DEPTH_BUFFER_BIT | LOCAL_GL_STENCIL_BUFFER_BIT);

#if 0
    err = glGetError();
    if (err) {
        printf ("error after MakeContextCurrent! 0x%04x\n", err);
    }
#endif

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::Render(gfxContext *ctx, gfxPattern::GraphicsFilter f)
{
    if (!gl)
        return NS_OK;

    nsRefPtr<gfxImageSurface> surf = new gfxImageSurface(gfxIntSize(mWidth, mHeight),
                                                         gfxASurface::ImageFormatARGB32);
    if (surf->CairoStatus() != 0)
        return NS_ERROR_FAILURE;

    MakeContextCurrent();
    gl->fReadPixels(0, 0, mWidth, mHeight,
                    LOCAL_GL_BGRA,
                    LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV,
                    surf->Data());

    gfxUtils::PremultiplyImageSurface(surf);

    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);
    pat->SetFilter(f);

    ctx->NewPath();
    ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->Fill();

    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetInputStream(const char* aMimeType,
                             const PRUnichar* aEncoderOptions,
                             nsIInputStream **aStream)
{
    return NS_ERROR_FAILURE;

    
#if 0
    if (!mGLPbuffer ||
        !mGLPbuffer->ThebesSurface())
        return NS_ERROR_FAILURE;

    nsresult rv;
    const char encoderPrefix[] = "@mozilla.org/image/encoder;2?type=";
    nsAutoArrayPtr<char> conid(new (std::nothrow) char[strlen(encoderPrefix) + strlen(aMimeType) + 1]);

    if (!conid)
        return NS_ERROR_OUT_OF_MEMORY;

    strcpy(conid, encoderPrefix);
    strcat(conid, aMimeType);

    nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(conid);
    if (!encoder)
        return NS_ERROR_FAILURE;

    nsAutoArrayPtr<PRUint8> imageBuffer(new (std::nothrow) PRUint8[mWidth * mHeight * 4]);
    if (!imageBuffer)
        return NS_ERROR_OUT_OF_MEMORY;

    nsRefPtr<gfxImageSurface> imgsurf = new gfxImageSurface(imageBuffer.get(),
                                                            gfxIntSize(mWidth, mHeight),
                                                            mWidth * 4,
                                                            gfxASurface::ImageFormatARGB32);

    if (!imgsurf || imgsurf->CairoStatus())
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxContext> ctx = new gfxContext(imgsurf);

    if (!ctx || ctx->HasError())
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxASurface> surf = mGLPbuffer->ThebesSurface();
    nsRefPtr<gfxPattern> pat = CanvasGLThebes::CreatePattern(surf);
    gfxMatrix m;
    m.Translate(gfxPoint(0.0, mGLPbuffer->Height()));
    m.Scale(1.0, -1.0);
    pat->SetMatrix(m);

    
    
    ctx->NewPath();
    ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->Fill();

    rv = encoder->InitFromData(imageBuffer.get(),
                               mWidth * mHeight * 4, mWidth, mHeight, mWidth * 4,
                               imgIEncoder::INPUT_FORMAT_HOSTARGB,
                               nsDependentString(aEncoderOptions));
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(encoder, aStream);
#endif
}

NS_IMETHODIMP
WebGLContext::GetThebesSurface(gfxASurface **surface)
{
    return NS_ERROR_NOT_AVAILABLE;
}

already_AddRefed<layers::CanvasLayer>
WebGLContext::GetCanvasLayer(LayerManager *manager)
{
    nsRefPtr<CanvasLayer> canvasLayer = manager->CreateCanvasLayer();
    if (!canvasLayer) {
        NS_WARNING("CreateCanvasLayer returned null!");
        return nsnull;
    }

    CanvasLayer::Data data;

    
    
    

    void* native_pbuffer = gl->GetNativeData(gl::GLContext::NativePBuffer);
    void* native_surface = gl->GetNativeData(gl::GLContext::NativeImageSurface);

    if (native_pbuffer) {
        data.mGLContext = gl.get();
    }
    else if (native_surface) {
        data.mSurface = static_cast<gfxASurface*>(native_surface);
    }
    else {
        NS_WARNING("The GLContext has neither a native PBuffer nor a native surface!");
        return nsnull;
    }

    data.mSize = nsIntSize(mWidth, mHeight);
    data.mGLBufferIsPremultiplied = PR_FALSE;

    canvasLayer->Initialize(data);
    
    canvasLayer->SetIsOpaqueContent(PR_FALSE);
    canvasLayer->Updated(nsIntRect(0, 0, mWidth, mHeight));

    mInvalidated = PR_FALSE;

    return canvasLayer.forget().get();
}






NS_IMPL_ADDREF(WebGLContext)
NS_IMPL_RELEASE(WebGLContext)

DOMCI_DATA(CanvasRenderingContextWebGL, WebGLContext)

NS_INTERFACE_MAP_BEGIN(WebGLContext)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextWebGL)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICanvasRenderingContextWebGL)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasRenderingContextWebGL)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLBuffer)
NS_IMPL_RELEASE(WebGLBuffer)

DOMCI_DATA(WebGLBuffer, WebGLBuffer)

NS_INTERFACE_MAP_BEGIN(WebGLBuffer)
  NS_INTERFACE_MAP_ENTRY(WebGLBuffer)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLBuffer)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLBuffer)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLTexture)
NS_IMPL_RELEASE(WebGLTexture)

DOMCI_DATA(WebGLTexture, WebGLTexture)

NS_INTERFACE_MAP_BEGIN(WebGLTexture)
  NS_INTERFACE_MAP_ENTRY(WebGLTexture)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLTexture)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLTexture)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLProgram)
NS_IMPL_RELEASE(WebGLProgram)

DOMCI_DATA(WebGLProgram, WebGLProgram)

NS_INTERFACE_MAP_BEGIN(WebGLProgram)
  NS_INTERFACE_MAP_ENTRY(WebGLProgram)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLProgram)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLProgram)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLShader)
NS_IMPL_RELEASE(WebGLShader)

DOMCI_DATA(WebGLShader, WebGLShader)

NS_INTERFACE_MAP_BEGIN(WebGLShader)
  NS_INTERFACE_MAP_ENTRY(WebGLShader)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLShader)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLShader)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLFramebuffer)
NS_IMPL_RELEASE(WebGLFramebuffer)

DOMCI_DATA(WebGLFramebuffer, WebGLFramebuffer)

NS_INTERFACE_MAP_BEGIN(WebGLFramebuffer)
  NS_INTERFACE_MAP_ENTRY(WebGLFramebuffer)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLFramebuffer)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLFramebuffer)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLRenderbuffer)
NS_IMPL_RELEASE(WebGLRenderbuffer)

DOMCI_DATA(WebGLRenderbuffer, WebGLRenderbuffer)

NS_INTERFACE_MAP_BEGIN(WebGLRenderbuffer)
  NS_INTERFACE_MAP_ENTRY(WebGLRenderbuffer)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLRenderbuffer)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLRenderbuffer)
NS_INTERFACE_MAP_END


NS_IMETHODIMP WebGLTexture::GetName(GLuint *aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebGLTexture::SetName(GLuint aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebGLBuffer::GetName(GLuint *aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebGLBuffer::SetName(GLuint aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebGLProgram::GetName(GLuint *aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebGLProgram::SetName(GLuint aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebGLShader::GetName(GLuint *aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebGLShader::SetName(GLuint aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebGLFramebuffer::GetName(GLuint *aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebGLFramebuffer::SetName(GLuint aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP WebGLRenderbuffer::GetName(GLuint *aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP WebGLRenderbuffer::SetName(GLuint aName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
