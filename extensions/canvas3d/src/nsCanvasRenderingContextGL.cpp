





































#include "prmem.h"
#include "prlog.h"

#include "nsCanvasRenderingContextGL.h"

#include "nsICanvasRenderingContextGL.h"

#include "nsIRenderingContext.h"

#include "nsICanvasRenderingContextInternal.h"
#include "nsIDOMHTMLCanvasElement.h"
#include "nsIView.h"
#include "nsIViewManager.h"

#ifndef MOZILLA_1_8_BRANCH
#include "nsIDocument.h"
#endif

#include "nsTransform2D.h"

#include "nsIScriptSecurityManager.h"
#include "nsISecurityCheckedComponent.h"

#include "imgIRequest.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIDOMHTMLCanvasElement.h"
#include "nsICanvasElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIImageLoadingContent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIImage.h"
#include "nsIFrame.h"
#include "nsDOMError.h"
#include "nsIJSRuntimeService.h"

#ifndef MOZILLA_1_8_BRANCH
#include "nsIClassInfoImpl.h"
#endif

#include "nsServiceManagerUtils.h"

#include "nsDOMError.h"

#include "nsContentUtils.h"

#include "nsIXPConnect.h"
#include "jsapi.h"


#include "glew.h"



#include "cairo.h"

#ifdef MOZ_CAIRO_GFX
#include "gfxContext.h"
#include "gfxASurface.h"
#endif

#ifdef XP_WIN
#ifdef MOZILLA_1_8_BRANCH
struct _cairo_surface_win32_hack {
    void *ptr;
    unsigned int refcnt;
    cairo_status_t st;
    cairo_bool_t finished;
    
    int sz;
    int num_el;
    int el_sz;
    void *elements;
    double dx, dy, dxs, dys;
    unsigned int a;
    unsigned int b;

    
    cairo_format_t format;
};
#endif
#endif

#ifdef MOZ_X11
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include "cairo-xlib.h"
#endif

nsIXPConnect *gXPConnect = nsnull;
JSRuntime *gScriptRuntime = nsnull;
nsIJSRuntimeService *gJSRuntimeService = nsnull;


NS_DECL_CLASSINFO(CanvasGLBuffer)
NS_IMPL_ADDREF(CanvasGLBuffer)
NS_IMPL_RELEASE(CanvasGLBuffer)

NS_IMPL_CI_INTERFACE_GETTER1(CanvasGLBuffer, nsICanvasRenderingContextGLBuffer)

NS_INTERFACE_MAP_BEGIN(CanvasGLBuffer)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextGLBuffer)
  NS_INTERFACE_MAP_ENTRY(nsISecurityCheckedComponent)
  NS_INTERFACE_MAP_ENTRY(nsICanvasGLBuffer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICanvasRenderingContextGLBuffer)
  NS_IMPL_QUERY_CLASSINFO(CanvasGLBuffer)
NS_INTERFACE_MAP_END


NS_DECL_CLASSINFO(CanvasGLTexture)
NS_IMPL_ADDREF(CanvasGLTexture)
NS_IMPL_RELEASE(CanvasGLTexture)

NS_IMPL_CI_INTERFACE_GETTER1(CanvasGLTexture, nsICanvasRenderingContextGLTexture)

NS_INTERFACE_MAP_BEGIN(CanvasGLTexture)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextGLTexture)
  NS_INTERFACE_MAP_ENTRY(nsICanvasGLTexture)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICanvasRenderingContextGLTexture)
  NS_IMPL_QUERY_CLASSINFO(CanvasGLTexture)
NS_INTERFACE_MAP_END





static int bufferCount = 0;

CanvasGLBuffer::CanvasGLBuffer(nsCanvasRenderingContextGLPrivate *owner)
    : mDisposed(PR_TRUE),
      mLength(0), mSize(0), mType(0), mUsage(GL_STATIC_DRAW),
      mBufferID(0)
{
    owner->GetWeakReference(getter_AddRefs(mOwnerContext));

    bufferCount++;
    
}

CanvasGLBuffer::~CanvasGLBuffer()
{
    Dispose();

    --bufferCount;
    
}



static char* cloneAllAccess()
{
    static const char allAccess[] = "allAccess";
    return (char*)nsMemory::Clone(allAccess, sizeof(allAccess));
}

NS_IMETHODIMP
CanvasGLBuffer::CanCreateWrapper(const nsIID* iid, char **_retval) {
    *_retval = cloneAllAccess();
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::CanCallMethod(const nsIID *iid, const PRUnichar *methodName, char **_retval) {
    *_retval = cloneAllAccess();
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::CanGetProperty(const nsIID *iid, const PRUnichar *propertyName, char **_retval) {
    *_retval = cloneAllAccess();
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::CanSetProperty(const nsIID *iid, const PRUnichar *propertyName, char **_retval) {
    *_retval = cloneAllAccess();
    return NS_OK;
}

nsresult
CanvasGLBuffer::Init(PRUint32 usage,
                     PRUint32 size,
                     PRUint32 type,
                     JSContext *ctx,
                     JSObject *arrayObj,
                     jsuint arrayLen)
{
    nsresult rv;

    

    if (!mDisposed)
        Dispose();

    if (usage != GL_STATIC_DRAW &&
        usage != GL_DYNAMIC_DRAW)
        return NS_ERROR_INVALID_ARG;

    rv = JSArrayToSimpleBuffer(mSimpleBuffer, type, size, ctx, arrayObj, arrayLen);
    if (NS_FAILED(rv))
        return rv;

    mUsage = usage;
    mSize = size;
    mType = type;
    mLength = arrayLen;

    mBufferID = 0;

    mDisposed = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::Dispose()
{
    if (mDisposed)
        return NS_OK;

    if (mBufferID) {
        nsCOMPtr<nsICanvasRenderingContextInternal> ctx = do_QueryReferent(mOwnerContext);
        if (ctx) {
            nsCanvasRenderingContextGLPrivate *priv = (nsCanvasRenderingContextGLPrivate*) ctx.get();
            mGlewContextPtr = priv->glewGetContext();
            priv->MakeContextCurrent();

            glDeleteBuffers(1, &mBufferID);
            mBufferID = 0;
        }
    }

    mSimpleBuffer.Release();

    mDisposed = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::GetOwnerContext(nsICanvasRenderingContextGL **retval)
{
    nsCOMPtr<nsICanvasRenderingContextInternal> ctx = do_QueryReferent(mOwnerContext);
    if (ctx) {
        nsCanvasRenderingContextGLPrivate *priv = (nsCanvasRenderingContextGLPrivate*) ctx.get();
        *retval = priv->GetSelf();
    } else {
        *retval = nsnull;
    }

    NS_IF_ADDREF(*retval);
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::GetDisposed(PRBool *retval)
{
    *retval = mDisposed;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::GetUsage(PRUint32 *usage)
{
    if (mDisposed)
        return NS_ERROR_FAILURE;

    *usage = mUsage;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::GetLength(PRUint32 *retval)
{
    if (mDisposed)
        return NS_ERROR_FAILURE;

    *retval = mLength;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLBuffer::GetType(PRUint32 *retval)
{
    if (mDisposed)
        return NS_ERROR_FAILURE;

    *retval = mType;
    return NS_OK;
}





CanvasGLTexture::CanvasGLTexture(nsCanvasRenderingContextGLPrivate *owner)
    : mDisposed(PR_FALSE),
      
      mWidth(0), mHeight(0)
{
    owner->GetWeakReference(getter_AddRefs(mOwnerContext));
}

CanvasGLTexture::~CanvasGLTexture()
{
    Dispose();
}

nsresult
CanvasGLTexture::Init()
{
    return NS_OK;
}

nsresult
CanvasGLTexture::Dispose()
{
    if (mDisposed)
        return NS_OK;

    mDisposed = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLTexture::GetDisposed(PRBool *retval)
{
    *retval = mDisposed;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLTexture::GetOwnerContext(nsICanvasRenderingContextGL **retval)
{
    nsCOMPtr<nsICanvasRenderingContextInternal> ctx = do_QueryReferent(mOwnerContext);
    if (ctx) {
        nsCanvasRenderingContextGLPrivate *priv = (nsCanvasRenderingContextGLPrivate*) ctx.get();
        *retval = priv->GetSelf();
    } else {
        *retval = nsnull;
    }

    NS_IF_ADDREF(*retval);
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLTexture::GetTarget(PRUint32 *aResult)
{
    
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLTexture::GetWidth(PRUint32 *aWidth)
{
    *aWidth = mWidth;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLTexture::GetHeight(PRUint32 *aHeight)
{
    *aHeight = mHeight;
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLTexture::SetFilter(PRUint32 filterType, PRUint32 filterMode)
{
    if (filterType < 0 || filterType > 1 ||
        filterMode < 0 || filterMode > 1)
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    
    return NS_OK;
}

NS_IMETHODIMP
CanvasGLTexture::SetWrap(PRUint32 wrapType, PRUint32 wrapMode)
{
    if (wrapType != GL_TEXTURE_WRAP_S &&
        wrapType != GL_TEXTURE_WRAP_T)
        return NS_ERROR_DOM_SYNTAX_ERR;

    if (wrapMode != GL_CLAMP_TO_EDGE &&
        wrapMode != GL_REPEAT &&
        wrapMode != GL_MIRRORED_REPEAT)
        return NS_ERROR_DOM_SYNTAX_ERR;

    
    return NS_OK;
}

nsresult
JSArrayToSimpleBuffer (SimpleBuffer& sbuffer,
                       PRUint32 typeParam,
                       PRUint32 sizeParam,
                       JSContext *ctx,
                       JSObject *arrayObj,
                       jsuint arrayLen)
{
    sbuffer.Prepare(typeParam, sizeParam, arrayLen);

    if (typeParam == GL_SHORT) {
        short *ptr = (short*) sbuffer.data;
        for (PRUint32 i = 0; i < arrayLen; i++) {
            jsval jv;
            int32 iv;
            ::JS_GetElement(ctx, arrayObj, i, &jv);
            ::JS_ValueToECMAInt32(ctx, jv, &iv);
            *ptr++ = (short) iv;
        }
    } else if (typeParam == GL_FLOAT) {
        float *ptr = (float*) sbuffer.data;
        for (PRUint32 i = 0; i < arrayLen; i++) {
            jsval jv;
            jsdouble dv;
            ::JS_GetElement(ctx, arrayObj, i, &jv);
            ::JS_ValueToNumber(ctx, jv, &dv);
            *ptr++ = (float) dv;
        }
    } else if (typeParam == GL_UNSIGNED_BYTE) {
        unsigned char *ptr = (unsigned char*) sbuffer.data;
        for (PRUint32 i = 0; i < arrayLen; i++) {
            jsval jv;
            uint32 iv;
            ::JS_GetElement(ctx, arrayObj, i, &jv);
            ::JS_ValueToECMAUint32(ctx, jv, &iv);
            *ptr++ = (unsigned char) iv;
        }
    } else if (typeParam == GL_UNSIGNED_INT) {
        PRUint32 *ptr = (PRUint32*) sbuffer.data;
        for (PRUint32 i = 0; i < arrayLen; i++) {
            jsval jv;
            uint32 iv;
            ::JS_GetElement(ctx, arrayObj, i, &jv);
            ::JS_ValueToECMAUint32(ctx, jv, &iv);
            *ptr++ = iv;
        }
    } else {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    return NS_OK;
}

void
nsCanvasRenderingContextGLPrivate::MakeContextCurrent()
{
    mGLPbuffer->MakeContextCurrent();
}

void
nsCanvasRenderingContextGLPrivate::LostCurrentContext(void *closure)
{
    nsCanvasRenderingContextGLPrivate* self = (nsCanvasRenderingContextGLPrivate*) closure;
    
    fflush (stderr);
}





NS_IMETHODIMP
nsCanvasRenderingContextGLPrivate::SetCanvasElement(nsICanvasElement* aParentCanvas)
{
    if (!SafeToCreateCanvas3DContext())
        return NS_ERROR_FAILURE;

    mGLPbuffer = new nsGLPbuffer();

    if (!mGLPbuffer->Init(this))
        return NS_ERROR_FAILURE;

    if (!ValidateGL()) {
        

        LogMessage(NS_LITERAL_CSTRING("Canvas 3D: Couldn't validate OpenGL implementation; is everything needed present?"));
        return NS_ERROR_FAILURE;
    }

    mCanvasElement = aParentCanvas;
    fprintf (stderr, "VVVV SetCanvasElement: %p\n", mCanvasElement);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLPrivate::SetDimensions(PRInt32 width, PRInt32 height)
{
    fprintf (stderr, "VVVV CanvasGLBuffer::SetDimensions %d %d\n", width, height);

    if (mWidth == width && mHeight == height)
        return NS_OK;

    if (!mGLPbuffer->Resize(width, height)) {
        LogMessage(NS_LITERAL_CSTRING("mGLPbuffer->Resize failed"));
        return NS_ERROR_FAILURE;
    }

    mWidth = width;
    mHeight = height;

    return NS_OK;
}






NS_IMETHODIMP
nsCanvasRenderingContextGLPrivate::Render(nsIRenderingContext *rc)
{
    nsresult rv = NS_OK;

    if (!mGLPbuffer)
        return NS_OK;

    if (!mGLPbuffer->ThebesSurface())
        return NS_OK;

#ifdef MOZ_CAIRO_GFX
    gfxContext* ctx = (gfxContext*) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
    nsRefPtr<gfxASurface> surf = mGLPbuffer->ThebesSurface();
    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);

    
    
    ctx->NewPath();
    ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->Fill();
#else

    
    cairo_surface_t *dest = nsnull;
    cairo_t *dest_cr = nsnull;

#ifdef XP_WIN
    void *ptr = nsnull;
#ifdef MOZILLA_1_8_BRANCH
    rv = rc->RetrieveCurrentNativeGraphicData(&ptr);
    if (NS_FAILED(rv) || !ptr)
        return NS_ERROR_FAILURE;
#else
    ptr = rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_WINDOWS_DC);
#endif
    HDC dc = (HDC) ptr;

    dest = cairo_win32_surface_create (dc);
    dest_cr = cairo_create (dest);
#endif

#ifdef MOZ_WIDGET_GTK2
    GdkDrawable *gdkdraw = nsnull;
#ifdef MOZILLA_1_8_BRANCH
    rv = rc->RetrieveCurrentNativeGraphicData((void**) &gdkdraw);
    if (NS_FAILED(rv) || !gdkdraw)
        return NS_ERROR_FAILURE;
#else
    gdkdraw = (GdkDrawable*) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_GDK_DRAWABLE);
    if (!gdkdraw)
        return NS_ERROR_FAILURE;
#endif

    gint w, h;
    gdk_drawable_get_size (gdkdraw, &w, &h);
    dest = cairo_xlib_surface_create (GDK_DRAWABLE_XDISPLAY(gdkdraw),
                                      GDK_DRAWABLE_XID(gdkdraw),
                                      GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(gdkdraw)),
                                      w, h);
    dest_cr = cairo_create (dest);
#endif

    nsTransform2D *tx = nsnull;
    rc->GetCurrentTransform(tx);

    nsCOMPtr<nsIDeviceContext> dctx;
    rc->GetDeviceContext(*getter_AddRefs(dctx));

    
    
#ifndef XP_MACOSX

    float x0 = 0.0, y0 = 0.0;
    float sx = 1.0, sy = 1.0;
    if (tx->GetType() & MG_2DTRANSLATION) {
        tx->Transform(&x0, &y0);
    }

    if (tx->GetType() & MG_2DSCALE) {
        sx = sy = dctx->DevUnitsToTwips();
        tx->TransformNoXLate(&sx, &sy);
    }

    cairo_translate (dest_cr, NSToIntRound(x0), NSToIntRound(y0));
    if (sx != 1.0 || sy != 1.0)
        cairo_scale (dest_cr, sx, sy);

    cairo_rectangle (dest_cr, 0, 0, mWidth, mHeight);
    cairo_clip (dest_cr);

    cairo_set_source_surface (dest_cr, mCairoImageSurface, 0, 0);
    cairo_paint (dest_cr);

    if (dest_cr)
        cairo_destroy (dest_cr);
    if (dest)
        cairo_surface_destroy (dest);

#else

    

    CGrafPtr port = nsnull;
#ifdef MOZILLA_1_8_BRANCH
    rv = rc->RetrieveCurrentNativeGraphicData((void**) &port);
    if (NS_FAILED(rv) || !port)
        return NS_ERROR_FAILURE;
#else
    port = (CGrafPtr) rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_MAC_THING);
    if (!port)
        return NS_ERROR_FAILURE;
#endif

    struct Rect portRect;
    GetPortBounds(port, &portRect);

    CGContextRef cgc;
    OSStatus status;
    status = QDBeginCGContext (port, &cgc);
    if (status != noErr)
        return NS_ERROR_FAILURE;

    CGDataProviderRef dataProvider;
    CGImageRef img;

    dataProvider = CGDataProviderCreateWithData (NULL, mImageBuffer,
                                                 mWidth * mHeight * 4,
                                                 NULL);
    CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
    img = CGImageCreate (mWidth, mHeight, 8, 32, mWidth * 4, rgb,
                         kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host,
                         dataProvider, NULL, false, kCGRenderingIntentDefault);
    CGColorSpaceRelease (rgb);
    CGDataProviderRelease (dataProvider);

    float x0 = 0.0, y0 = 0.0;
    float sx = 1.0, sy = 1.0;
    if (tx->GetType() & MG_2DTRANSLATION) {
        tx->Transform(&x0, &y0);
    }

    if (tx->GetType() & MG_2DSCALE) {
        float p2t = dctx->DevUnitsToTwips();
        sx = p2t, sy = p2t;
        tx->TransformNoXLate(&sx, &sy);
    }

    
    CGContextTranslateCTM (cgc, NSToIntRound(x0),
                           portRect.bottom - portRect.top - NSToIntRound(y0) - NSToIntRound(mHeight * sy));
    if (sx != 1.0 || sy != 1.0)
        CGContextScaleCTM (cgc, sx, sy);

    CGContextDrawImage (cgc, CGRectMake(0, 0, mWidth, mHeight), img);

    CGImageRelease (img);

    status = QDEndCGContext (port, &cgc);
    
#endif
#endif

    return rv;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLPrivate::RenderToSurface(cairo_surface_t *surf)
{
    return NS_OK;
}

nsIFrame*
nsCanvasRenderingContextGLPrivate::GetCanvasLayoutFrame()
{
    if (!mCanvasElement)
        return nsnull;

    nsIFrame *fr = nsnull;
    mCanvasElement->GetPrimaryCanvasFrame(&fr);
    return fr;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLPrivate::GetInputStream(const nsACString& aMimeType,
                                                  const nsAString& aEncoderOptions,
                                                  nsIInputStream **aStream)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





nsresult
nsCanvasRenderingContextGLPrivate::CairoSurfaceFromElement(nsIDOMElement *imgElt,
                                                           cairo_surface_t **aCairoSurface,
                                                           PRUint8 **imgData,
                                                           PRInt32 *widthOut, PRInt32 *heightOut,
                                                           nsIURI **uriOut, PRBool *forceWriteOnlyOut)
{
    nsresult rv;

    nsCOMPtr<imgIContainer> imgContainer;

    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(imgElt);
    if (imageLoader) {
        nsCOMPtr<imgIRequest> imgRequest;
        rv = imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                     getter_AddRefs(imgRequest));
        NS_ENSURE_SUCCESS(rv, rv);
        if (!imgRequest)
            
            return NS_ERROR_NOT_AVAILABLE;

        nsCOMPtr<nsIURI> uri;
        rv = imageLoader->GetCurrentURI(uriOut);
        NS_ENSURE_SUCCESS(rv, rv);
       
        *forceWriteOnlyOut = PR_FALSE;

        rv = imgRequest->GetImage(getter_AddRefs(imgContainer));
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        
        nsCOMPtr<nsICanvasElement> canvas = do_QueryInterface(imgElt);
        if (canvas) {
            PRUint32 w, h;
            rv = canvas->GetSize(&w, &h);
            NS_ENSURE_SUCCESS(rv, rv);

            PRUint8 *data = (PRUint8*) PR_Malloc(w * h * 4);
            cairo_surface_t *surf =
                cairo_image_surface_create_for_data (data, CAIRO_FORMAT_ARGB32,
                                                     w, h, w*4);
            cairo_t *cr = cairo_create (surf);
            cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
            cairo_paint (cr);
            cairo_destroy (cr);

            rv = canvas->RenderContextsToSurface(surf);
            if (NS_FAILED(rv)) {
                cairo_surface_destroy (surf);
                return rv;
            }

            *aCairoSurface = surf;
            *imgData = data;
            *widthOut = w;
            *heightOut = h;

            *uriOut = nsnull;
            *forceWriteOnlyOut = canvas->IsWriteOnly();

            return NS_OK;
        } else {
            NS_WARNING("No way to get surface from non-canvas, non-imageloader");
            return NS_ERROR_NOT_AVAILABLE;
        }
    }

    if (!imgContainer)
        return NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<gfxIImageFrame> frame;
    rv = imgContainer->GetCurrentFrame(getter_AddRefs(frame));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIImage> img(do_GetInterface(frame));

    PRInt32 imgWidth, imgHeight;
    rv = frame->GetWidth(&imgWidth);
    rv |= frame->GetHeight(&imgHeight);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    if (widthOut)
        *widthOut = imgWidth;
    if (heightOut)
        *heightOut = imgHeight;

#ifdef MOZ_CAIRO_GFX
    gfxASurface* gfxsurf = nsnull;
    rv = img->GetSurface(&gfxsurf);
    NS_ENSURE_SUCCESS(rv, rv);

    *aCairoSurface = gfxsurf->CairoSurface();
    cairo_surface_reference (*aCairoSurface);
    *imgData = nsnull;
#else
    
    
    
    

    PRUint8 *cairoImgData = (PRUint8 *)nsMemory::Alloc(imgHeight * imgWidth * 4);
    PRUint8 *outData = cairoImgData;

    gfx_format format;
    rv = frame->GetFormat(&format);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = frame->LockImageData();
    if (img->GetHasAlphaMask())
        rv |= frame->LockAlphaData();
    if (NS_FAILED(rv)) {
        nsMemory::Free(cairoImgData);
        return NS_ERROR_FAILURE;
    }

    PRUint8 *inPixBits, *inAlphaBits = nsnull;
    PRUint32 inPixStride, inAlphaStride = 0;
    inPixBits = img->GetBits();
    inPixStride = img->GetLineStride();
    if (img->GetHasAlphaMask()) {
        inAlphaBits = img->GetAlphaBits();
        inAlphaStride = img->GetAlphaLineStride();
    }

    PRBool topToBottom = img->GetIsRowOrderTopToBottom();
    PRBool useBGR;

    
    
    
    

    if ((format == gfxIFormats::RGB || format == gfxIFormats::BGR) ||
        (!(img->GetHasAlphaMask()) && (format == gfxIFormats::RGB_A8 || format == gfxIFormats::BGR_A8)))
    {
        useBGR = (format & 1);

#ifdef IS_BIG_ENDIAN
        useBGR = !useBGR;
#endif

        for (PRUint32 j = 0; j < (PRUint32) imgHeight; j++) {
            PRUint32 rowIndex;
            if (topToBottom)
                rowIndex = j;
            else
                rowIndex = imgHeight - j - 1;

            PRUint8 *inrowrgb = inPixBits + (inPixStride * rowIndex);

            for (PRUint32 i = 0; i < (PRUint32) imgWidth; i++) {
                
#ifdef XP_MACOSX
                
                inrowrgb++;
#endif
                PRUint8 r, g, b;
                if (useBGR) {
                    b = *inrowrgb++;
                    g = *inrowrgb++;
                    r = *inrowrgb++;
                } else {
                    r = *inrowrgb++;
                    g = *inrowrgb++;
                    b = *inrowrgb++;
                }

#ifdef IS_BIG_ENDIAN
                
                *outData++ = 0xff;
#endif

                *outData++ = r;
                *outData++ = g;
                *outData++ = b;

#ifdef IS_LITTLE_ENDIAN
                
                *outData++ = 0xff;
#endif
            }
        }
        rv = NS_OK;
    } else if (format == gfxIFormats::RGB_A1 || format == gfxIFormats::BGR_A1) {
        useBGR = (format & 1);

#ifdef IS_BIG_ENDIAN
        useBGR = !useBGR;
#endif

        for (PRUint32 j = 0; j < (PRUint32) imgHeight; j++) {
            PRUint32 rowIndex;
            if (topToBottom)
                rowIndex = j;
            else
                rowIndex = imgHeight - j - 1;

            PRUint8 *inrowrgb = inPixBits + (inPixStride * rowIndex);
            PRUint8 *inrowalpha = inAlphaBits + (inAlphaStride * rowIndex);

            for (PRUint32 i = 0; i < (PRUint32) imgWidth; i++) {
                
                PRInt32 bit = i % 8;
                PRInt32 byte = i / 8;

#ifdef IS_LITTLE_ENDIAN
                PRUint8 a = (inrowalpha[byte] >> (7-bit)) & 1;
#else
                PRUint8 a = (inrowalpha[byte] >> bit) & 1;
#endif

#ifdef XP_MACOSX
                
                inrowrgb++;
#endif

                
                
                
                if (a) {
                    PRUint8 r, g, b;

                    if (useBGR) {
                        b = *inrowrgb++;
                        g = *inrowrgb++;
                        r = *inrowrgb++;
                    } else {
                        r = *inrowrgb++;
                        g = *inrowrgb++;
                        b = *inrowrgb++;
                    }

#ifdef IS_BIG_ENDIAN
                    
                    *outData++ = 0xff;
#endif

                    *outData++ = r;
                    *outData++ = g;
                    *outData++ = b;

#ifdef IS_LITTLE_ENDIAN
                    
                    *outData++ = 0xff;
#endif
                } else {
                    
                    
                    inrowrgb += 3;
                    *outData++ = 0;
                    *outData++ = 0;
                    *outData++ = 0;
                    *outData++ = 0;
                }
            }
        }
        rv = NS_OK;
    } else if (format == gfxIFormats::RGB_A8 || format == gfxIFormats::BGR_A8) {
        useBGR = (format & 1);

#ifdef IS_BIG_ENDIAN
        useBGR = !useBGR;
#endif

        for (PRUint32 j = 0; j < (PRUint32) imgHeight; j++) {
            PRUint32 rowIndex;
            if (topToBottom)
                rowIndex = j;
            else
                rowIndex = imgHeight - j - 1;

            PRUint8 *inrowrgb = inPixBits + (inPixStride * rowIndex);
            PRUint8 *inrowalpha = inAlphaBits + (inAlphaStride * rowIndex);

            for (PRUint32 i = 0; i < (PRUint32) imgWidth; i++) {
                
                PRUint8 a = *inrowalpha++;

                
                
#ifdef XP_MACOSX
                
                inrowrgb++;
#endif

                
                
                
                
                
                
                
                
                
                

                
                
                
                

                PRUint8 r, g, b;
                if (useBGR) {
                    b = (*inrowrgb++ * a - a / 2) / 255;
                    g = (*inrowrgb++ * a - a / 2) / 255;
                    r = (*inrowrgb++ * a - a / 2) / 255;
                } else {
                    r = (*inrowrgb++ * a - a / 2) / 255;
                    g = (*inrowrgb++ * a - a / 2) / 255;
                    b = (*inrowrgb++ * a - a / 2) / 255;
                }

#ifdef IS_BIG_ENDIAN
                *outData++ = a;
#endif

                *outData++ = r;
                *outData++ = g;
                *outData++ = b;

#ifdef IS_LITTLE_ENDIAN
                *outData++ = a;
#endif
            }
        }
        rv = NS_OK;
    } else {
        rv = NS_ERROR_FAILURE;
    }

    if (img->GetHasAlphaMask())
        frame->UnlockAlphaData();
    frame->UnlockImageData();

    if (NS_FAILED(rv)) {
        nsMemory::Free(cairoImgData);
        return rv;
    }

    cairo_surface_t *imgSurf =
        cairo_image_surface_create_for_data(cairoImgData, CAIRO_FORMAT_ARGB32,
                                            imgWidth, imgHeight, imgWidth*4);

    *aCairoSurface = imgSurf;
    *imgData = cairoImgData;
#endif

    return NS_OK;
}

void
nsCanvasRenderingContextGLPrivate::DoDrawImageSecurityCheck(nsIURI* aURI, PRBool forceWriteOnly)
{
    
    return;

#if 0
    fprintf (stderr, "DoDrawImageSecuritycheck this 1: %p\n", this);
    if (mCanvasElement->IsWriteOnly())
        return;

    fprintf (stderr, "DoDrawImageSecuritycheck this 2: %p\n", this);
    if (!aURI)
        return;

    fprintf (stderr, "DoDrawImageSecuritycheck this 3: %p\n", this);
    if (forceWriteOnly) {
        mCanvasElement->SetWriteOnly();
        return;
    }

    fprintf (stderr, "DoDrawImageSecuritycheck this 4: %p\n", this);
    nsCOMPtr<nsIScriptSecurityManager> ssm =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
    if (!ssm) {
        mCanvasElement->SetWriteOnly();
        return;
    }

    fprintf (stderr, "DoDrawImageSecuritycheck this 5: %p\n", this);
#ifdef MOZILLA_1_8_BRANCH
#if 0
    nsCOMPtr<nsIDOMNode> elem = do_QueryInterface(mCanvasElement);
    if (elem && ssm) {
        nsCOMPtr<nsIPrincipal> elemPrincipal;
        nsCOMPtr<nsIPrincipal> uriPrincipal;
        nsCOMPtr<nsIDocument> elemDocument;
        nsContentUtils::GetDocumentAndPrincipal(elem, getter_AddRefs(elemDocument), getter_AddRefs(elemPrincipal));
        ssm->GetCodebasePrincipal(aURI, getter_AddRefs(uriPrincipal));

        if (uriPrincipal && elemPrincipal) {
            nsresult rv =
                ssm->CheckSameOriginPrincipal(elemPrincipal, uriPrincipal);
            if (NS_SUCCEEDED(rv)) {
                
                return;
            }
        }
    }
#endif
#else
    nsCOMPtr<nsINode> elem = do_QueryInterface(mCanvasElement);
    if (elem && ssm) {
        nsCOMPtr<nsIPrincipal> uriPrincipal;
        ssm->GetCodebasePrincipal(aURI, getter_AddRefs(uriPrincipal));

        if (uriPrincipal) {
            nsresult rv = ssm->CheckSameOriginPrincipal(elem->NodePrincipal(),
                                                        uriPrincipal);
            if (NS_SUCCEEDED(rv)) {
                
                return;
            }
        }
    }
#endif

    fprintf (stderr, "DoDrawImageSecuritycheck this 6: %p\n", this); fflush(stderr);
    mCanvasElement->SetWriteOnly();
#endif
}


nsCanvasRenderingContextGLPrivate::nsCanvasRenderingContextGLPrivate()
    : mGLPbuffer(nsnull), mWidth(0), mHeight(0), mCanvasElement(nsnull)
{
    
    if (!gXPConnect) {
        nsresult rv = CallGetService(nsIXPConnect::GetCID(), &gXPConnect);
        if (NS_FAILED(rv)) {
            NS_ERROR("Failed to get XPConnect!");
            return;
        }
    } else {
        NS_ADDREF(gXPConnect);
    }

    if (!gJSRuntimeService) {
        nsresult rv = CallGetService("@mozilla.org/js/xpc/RuntimeService;1",
                                     &gJSRuntimeService);
        if (NS_FAILED(rv)) {
            
            NS_ERROR("Failed to get JS RuntimeService!");
            return;
        }

        gJSRuntimeService->GetRuntime(&gScriptRuntime);
        if (!gScriptRuntime) {
            NS_RELEASE(gJSRuntimeService);
            gJSRuntimeService = nsnull;
            NS_ERROR("Unable to get JS runtime from JS runtime service");
        }
    } else {
        NS_ADDREF(gJSRuntimeService);
    }
}

nsCanvasRenderingContextGLPrivate::~nsCanvasRenderingContextGLPrivate()
{
    delete mGLPbuffer;
    mGLPbuffer = nsnull;
    
    
    if (gXPConnect && gXPConnect->Release() == 0)
        gXPConnect = nsnull;
    if (gJSRuntimeService && gJSRuntimeService->Release() == 0) {
        gJSRuntimeService = nsnull;
        gScriptRuntime = nsnull;
    }
}

nsresult
nsCanvasRenderingContextGLPrivate::DoSwapBuffers()
{
    mGLPbuffer->SwapBuffers();

    
    
    nsIFrame *frame = GetCanvasLayoutFrame();
    if (frame) {
        nsRect r = frame->GetRect();
        r.x = r.y = 0;

        
        

        
        
        
        nsIPresShell *shell = frame->PresContext()->GetPresShell();
        if (shell) {
            PRBool suppressed = PR_FALSE;
            shell->IsPaintingSuppressed(&suppressed);
            if (suppressed)
                return NS_OK;
        }

        
        
        PRUint32 flags = NS_VMREFRESH_NO_SYNC;
        if (frame->HasView()) {
            nsIView* view = frame->GetViewExternal();
            view->GetViewManager()->UpdateView(view, r, flags);
        } else {
            nsPoint offset;
            nsIView *view;
            frame->GetOffsetFromView(offset, &view);
            NS_ASSERTION(view, "no view");
            r += offset;
            view->GetViewManager()->UpdateView(view, r, flags);
        }
    }

    return NS_OK;
}

PRBool
nsCanvasRenderingContextGLPrivate::SafeToCreateCanvas3DContext()
{
    nsresult rv;

    
    PRBool is_caller_chrome = PR_FALSE;
    nsCOMPtr<nsIScriptSecurityManager> ssm =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    rv = ssm->SubjectPrincipalIsSystem(&is_caller_chrome);
    if (NS_SUCCEEDED(rv) && is_caller_chrome)
        return TRUE;

    

    
    nsCOMPtr<nsIPrefService> prefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    
    nsCOMPtr<nsIPrefBranch> prefBranch;
    rv = prefService->GetBranch("extensions.canvas3d.", getter_AddRefs(prefBranch));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    PRBool enabled;
    rv = prefBranch->GetBoolPref("enabledForWebContent", &enabled);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    if (enabled)
        return PR_TRUE;

    LogMessage(NS_LITERAL_CSTRING("Canvas 3D: Web content tried to create 3D Canvas Context, but pref extensions.canvas3d.enabledForWebContent is not set!"));

    return PR_FALSE;
}
