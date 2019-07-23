





































#include "prmem.h"
#include "prlog.h"

#include "nsIRenderingContext.h"

#define NSGL_CONTEXT_NAME nsCanvasRenderingContextGLES11

#include "nsCanvasRenderingContextGL.h"
#include "nsICanvasRenderingContextGLBuffer.h"
#include "nsICanvasRenderingContextGLES11.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsIDOMHTMLCanvasElement.h"
#include "nsIView.h"
#include "nsIViewManager.h"

#include "nsICanvasGLPrivate.h"

#ifndef MOZILLA_1_8_BRANCH
#include "nsIDocument.h"
#endif

#include "nsTransform2D.h"

#include "nsIScriptSecurityManager.h"
#include "nsISecurityCheckedComponent.h"

#include "nsWeakReference.h"

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

#ifdef MOZ_CAIRO_GFX
#include "gfxContext.h"
#include "gfxASurface.h"
#endif

#ifdef XP_WIN
#include <windows.h>
#endif


#include "glew.h"

#include "cairo.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gGLES11Log = nsnull;
#endif

#define MAX_TEXTURE_UNITS 16

class nsCanvasRenderingContextGLES11 :
    public nsICanvasRenderingContextGLES11,
    public nsCanvasRenderingContextGLPrivate
{
public:
    nsCanvasRenderingContextGLES11();
    virtual ~nsCanvasRenderingContextGLES11();

    NS_DECL_ISUPPORTS

    NS_DECL_NSICANVASRENDERINGCONTEXTGL

    NS_DECL_NSICANVASRENDERINGCONTEXTGLES11

    
    virtual nsICanvasRenderingContextGL *GetSelf() { return this; }

protected:
    inline PRBool CheckEnableMode(PRUint32 mode);

    nsRefPtr<CanvasGLBuffer> mVertexBuffer;
    nsRefPtr<CanvasGLBuffer> mNormalBuffer;
    nsRefPtr<CanvasGLBuffer> mColorBuffer;
    nsRefPtr<CanvasGLBuffer> mTexCoordBuffer[MAX_TEXTURE_UNITS];

    PRPackedBool mVertexArrayEnabled;
    PRPackedBool mNormalArrayEnabled;
    PRPackedBool mColorArrayEnabled;
    PRPackedBool mTexCoordArrayEnabled[MAX_TEXTURE_UNITS];

    PRUint16 mActiveTextureUnit;
};





NS_IMPL_ADDREF(nsCanvasRenderingContextGLES11)
NS_IMPL_RELEASE(nsCanvasRenderingContextGLES11)

NS_IMPL_CI_INTERFACE_GETTER4(nsCanvasRenderingContextGLES11,
                             nsICanvasRenderingContextGL,
                             nsICanvasRenderingContextGLES11,
                             nsICanvasRenderingContextInternal,
                             nsISupportsWeakReference)

NS_INTERFACE_MAP_BEGIN(nsCanvasRenderingContextGLES11)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextGL)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextGLES11)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICanvasRenderingContextGL)
  NS_IMPL_QUERY_CLASSINFO(nsCanvasRenderingContextGLES11)
NS_INTERFACE_MAP_END

nsresult
NS_NewCanvasRenderingContextGLES11(nsICanvasRenderingContextGLES11** aResult)
{
    nsICanvasRenderingContextGLES11* ctx = new nsCanvasRenderingContextGLES11();
    if (!ctx)
        return NS_ERROR_OUT_OF_MEMORY;

    fprintf (stderr, "VVVVV NS_New called\n"); fflush(stderr);
    NS_ADDREF(*aResult = ctx);
    return NS_OK;
}

nsCanvasRenderingContextGLES11::nsCanvasRenderingContextGLES11()
    : mVertexArrayEnabled(PR_FALSE),
      mNormalArrayEnabled(PR_FALSE),
      mColorArrayEnabled(PR_FALSE),
      mActiveTextureUnit(0)
{
#ifdef PR_LOGGING
    if (!gGLES11Log)
        gGLES11Log = PR_NewLogModule("canvasGLES11");
#endif

    PR_LOG(gGLES11Log, PR_LOG_DEBUG, ("##[%p] Creating GLES11 canvas context\n", this));

    for (int i = 0; i < MAX_TEXTURE_UNITS; i++)
        mTexCoordArrayEnabled[i] = PR_FALSE;
}

nsCanvasRenderingContextGLES11::~nsCanvasRenderingContextGLES11()
{
    
    
    
    mVertexBuffer = nsnull;
    mNormalBuffer = nsnull;
    mColorBuffer = nsnull;
    for (int i = 0; i < MAX_TEXTURE_UNITS; i++)
        mTexCoordBuffer[i] = nsnull;

    fprintf (stderr, "VVVVV ~nsCanvasRenderingContextGLES11\n");
    fflush (stderr);
}





NS_IMETHODIMP
nsCanvasRenderingContextGLES11::SwapBuffers()
{
    return DoSwapBuffers();
}





NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GetCanvas(nsIDOMHTMLCanvasElement **_retval)
{
    *_retval = nsnull;
    return NS_OK;
}

#define BETWEEN(_x,_a,_b) ((_x) >= (_a) && (_x) < (_b))

PRBool
nsCanvasRenderingContextGLES11::CheckEnableMode(PRUint32 mode) {
    if (BETWEEN(mode, CLIP_PLANE0, CLIP_PLANE0+5) ||
        BETWEEN(mode, LIGHT0, LIGHT0+32))
    {
        return PR_TRUE;
    } else {
        switch (mode) {
            case NORMALIZE:
            case RESCALE_NORMAL:
            case CLIP_PLANE0:
            case CLIP_PLANE1:
            case CLIP_PLANE2:
            case CLIP_PLANE3:
            case CLIP_PLANE4:
            case CLIP_PLANE5:
            case FOG:
            case LIGHTING:
            case COLOR_MATERIAL:
            case POINT_SMOOTH:
            case LINE_SMOOTH:
            case CULL_FACE:
            case POLYGON_OFFSET_FILL:
            case MULTISAMPLE:
            case SAMPLE_ALPHA_TO_COVERAGE:
            case SAMPLE_ALPHA_TO_ONE:
            case SAMPLE_COVERAGE:
            case TEXTURE_2D:
            case TEXTURE_RECTANGLE:
            case SCISSOR_TEST:
            case ALPHA_TEST:
            case STENCIL_TEST:
            case DEPTH_TEST:
            case BLEND:
            case DITHER:
            case COLOR_LOGIC_OP:
                return PR_TRUE;
                break;
            default:
                return PR_FALSE;
        }
    }

    return PR_FALSE;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::Enable(PRUint32 mode)
{
    if (!CheckEnableMode(mode))
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    MakeContextCurrent();
    glEnable(mode);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::Disable(PRUint32 mode)
{
    if (!CheckEnableMode(mode))
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    MakeContextCurrent();
    glDisable(mode);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::ClientActiveTexture(PRUint32 texture)
{
    if (!GLEW_ARB_multitexture) {
        if (texture == GL_TEXTURE0)
            return NS_OK;
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    if (texture < GL_TEXTURE0 || texture > (GL_TEXTURE0 + MAX_TEXTURE_UNITS))
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    mActiveTextureUnit = texture - GL_TEXTURE0;
    glClientActiveTexture(texture);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::EnableClientState(PRUint32 mode)
{
    switch(mode) {
        case VERTEX_ARRAY:
            mVertexArrayEnabled = PR_TRUE;
            break;
        case COLOR_ARRAY:
            mColorArrayEnabled = PR_TRUE;
            break;
        case NORMAL_ARRAY:
            mNormalArrayEnabled = PR_TRUE;
            break;
        case TEXTURE_COORD_ARRAY:
            mTexCoordArrayEnabled[mActiveTextureUnit] = PR_TRUE;
            break;
        default:
            return NS_ERROR_INVALID_ARG;
            break;
    }

    MakeContextCurrent();
    glEnableClientState(mode);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::DisableClientState(PRUint32 mode)
{
    switch(mode) {
        case VERTEX_ARRAY:
            mVertexArrayEnabled = PR_FALSE;
            break;
        case COLOR_ARRAY:
            mColorArrayEnabled = PR_FALSE;
            break;
        case NORMAL_ARRAY:
            mNormalArrayEnabled = PR_FALSE;
            break;
        case TEXTURE_COORD_ARRAY:
            mTexCoordArrayEnabled[mActiveTextureUnit] = PR_FALSE;
            break;
        default:
            return NS_ERROR_INVALID_ARG;
            break;
    }

    MakeContextCurrent();
    glDisableClientState(mode);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GetError(PRUint32 *_retval)
{
    MakeContextCurrent();
    *_retval = glGetError();
    return NS_OK;
}


GL_SAME_METHOD_3(Normal3f, Normal, float, float, float)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::MultiTexCoord(PRUint32 target, float s, float t, float r, float q)
{
    if (!GLEW_ARB_multitexture) {
        if (target == GL_TEXTURE0) {
            glTexCoord4f(s,t,r,q);
            return NS_OK;
        }
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    if (target < GL_TEXTURE0 || target > (GL_TEXTURE0+MAX_TEXTURE_UNITS))
        return NS_ERROR_DOM_SYNTAX_ERR;

    glMultiTexCoord4f(target, s, t, r, q);

    return NS_OK;
}


GL_SAME_METHOD_4(Color4f, Color, float, float, float, float)



NS_IMETHODIMP
nsCanvasRenderingContextGLES11::VertexPointer()
{
    {
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    nsRefPtr<CanvasGLBuffer> newBuffer;
    nsresult rv;

    if (js.argc == 1) {
        nsCOMPtr<nsICanvasRenderingContextGLBuffer> bufferBase;
        rv = JSValToSpecificInterface(js.ctx, js.argv[0], (nsICanvasRenderingContextGLBuffer**) getter_AddRefs(bufferBase));
        if (NS_FAILED(rv))
            return rv;

        newBuffer = (CanvasGLBuffer*) bufferBase.get();

        if (newBuffer->mType != GL_SHORT &&
            newBuffer->mType != GL_FLOAT)
            return NS_ERROR_DOM_SYNTAX_ERR;

        if (newBuffer->mSize < 2 || newBuffer->mSize > 4)
            return NS_ERROR_DOM_SYNTAX_ERR;
    } else if (js.argc == 3) {
        jsuint sizeParam;
        jsuint typeParam;

        JSObject *arrayObj;
        jsuint arrayLen;

        if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &sizeParam) ||
            !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &typeParam) ||
            !JSValToJSArrayAndLength(js.ctx, js.argv[2], &arrayObj, &arrayLen))
        {
            return NS_ERROR_DOM_SYNTAX_ERR;
        }

        if (typeParam != GL_SHORT &&
            typeParam != GL_FLOAT &&
            (sizeParam < 2 || sizeParam > 4))
            return NS_ERROR_DOM_SYNTAX_ERR;

        newBuffer = new CanvasGLBuffer(this);
        if (!newBuffer)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = newBuffer->Init (GL_STATIC_DRAW, sizeParam, typeParam, js.ctx, arrayObj, arrayLen);
        if (NS_FAILED(rv))
            return rv;

    } else {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    mVertexBuffer = newBuffer;

    MakeContextCurrent();
    glVertexPointer(newBuffer->GetSimpleBuffer().sizePerVertex,
                    newBuffer->GetSimpleBuffer().type,
                    0,
                    newBuffer->GetSimpleBuffer().data);

}
    (mVertexBuffer.get())->AddRef();
    nsrefcnt kk = (mVertexBuffer.get())->Release();
    fprintf (stderr, "VVVVV After VertexPointer: mVertexBuffer: %p refcount: %d\n", mVertexBuffer, kk);
    fflush(stderr);

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::NormalPointer()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    nsRefPtr<CanvasGLBuffer> newBuffer;
    nsresult rv;

    if (js.argc == 1) {
        nsCOMPtr<nsICanvasRenderingContextGLBuffer> bufferBase;
        rv = JSValToSpecificInterface(js.ctx, js.argv[0], (nsICanvasRenderingContextGLBuffer**) getter_AddRefs(bufferBase));
        if (NS_FAILED(rv))
            return rv;

        newBuffer = (CanvasGLBuffer*) bufferBase.get();

        if (newBuffer->mType != GL_SHORT &&
            newBuffer->mType != GL_FLOAT)
            return NS_ERROR_DOM_SYNTAX_ERR;

        if (newBuffer->mSize != 3)
            return NS_ERROR_DOM_SYNTAX_ERR;
    } else if (js.argc == 2) {
        jsuint typeParam;

        JSObject *arrayObj;
        jsuint arrayLen;

        if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &typeParam) ||
            !JSValToJSArrayAndLength(js.ctx, js.argv[1], &arrayObj, &arrayLen))
        {
            return NS_ERROR_DOM_SYNTAX_ERR;
        }

        if (typeParam != GL_SHORT && typeParam != GL_FLOAT)
            return NS_ERROR_DOM_SYNTAX_ERR;

        newBuffer = new CanvasGLBuffer(this);
        if (!newBuffer)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = newBuffer->Init (GL_STATIC_DRAW, 3, typeParam, js.ctx, arrayObj, arrayLen);
        if (NS_FAILED(rv))
            return NS_ERROR_DOM_SYNTAX_ERR;
    } else {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    mNormalBuffer = newBuffer;

    MakeContextCurrent();
    glNormalPointer(newBuffer->GetSimpleBuffer().type,
                    0,
                    newBuffer->GetSimpleBuffer().data);
    return NS_OK;
}



NS_IMETHODIMP
nsCanvasRenderingContextGLES11::TexCoordPointer()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    nsRefPtr<CanvasGLBuffer> newBuffer;
    nsresult rv;

    if (js.argc == 1) {
        nsCOMPtr<nsICanvasRenderingContextGLBuffer> bufferBase;
        rv = JSValToSpecificInterface(js.ctx, js.argv[0], (nsICanvasRenderingContextGLBuffer**) getter_AddRefs(bufferBase));
        if (NS_FAILED(rv))
            return rv;

        newBuffer = (CanvasGLBuffer*) bufferBase.get();

        if (newBuffer->mType != GL_SHORT &&
            newBuffer->mType != GL_FLOAT)
            return NS_ERROR_DOM_SYNTAX_ERR;

        if (newBuffer->mSize != 2)
            return NS_ERROR_DOM_SYNTAX_ERR;

    } else if (js.argc == 3) {
        jsuint sizeParam;
        jsuint typeParam;

        JSObject *arrayObj;
        jsuint arrayLen;

        if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &sizeParam) ||
            !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &typeParam) ||
            !JSValToJSArrayAndLength(js.ctx, js.argv[2], &arrayObj, &arrayLen))
        {
            return NS_ERROR_DOM_SYNTAX_ERR;
        }

        if (typeParam != GL_SHORT &&
            typeParam != GL_FLOAT)
            return NS_ERROR_DOM_SYNTAX_ERR;

        
        if (sizeParam != 2)
            return NS_ERROR_DOM_SYNTAX_ERR;

        newBuffer = new CanvasGLBuffer(this);
        if (!newBuffer)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = newBuffer->Init (GL_STATIC_DRAW, sizeParam, typeParam, js.ctx, arrayObj, arrayLen);
        if (NS_FAILED(rv))
            return rv;
    } else {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    mTexCoordBuffer[mActiveTextureUnit] = newBuffer;

    MakeContextCurrent();
    glTexCoordPointer(newBuffer->GetSimpleBuffer().sizePerVertex,
                      newBuffer->GetSimpleBuffer().type,
                      0,
                      newBuffer->GetSimpleBuffer().data);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::ColorPointer()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    nsRefPtr<CanvasGLBuffer> newBuffer;
    nsresult rv;

    if (js.argc == 1) {
        nsCOMPtr<nsICanvasRenderingContextGLBuffer> bufferBase;
        rv = JSValToSpecificInterface(js.ctx, js.argv[0], (nsICanvasRenderingContextGLBuffer**) getter_AddRefs(bufferBase));
        if (NS_FAILED(rv))
            return rv;

        newBuffer = (CanvasGLBuffer*) bufferBase.get();

        if (newBuffer->mType != GL_UNSIGNED_BYTE &&
            newBuffer->mType != GL_FLOAT)
            return NS_ERROR_DOM_SYNTAX_ERR;

        if (newBuffer->mSize != 4)
            return NS_ERROR_DOM_SYNTAX_ERR;
    } else if (js.argc == 3) {
        jsuint sizeParam;
        jsuint typeParam;

        JSObject *arrayObj;
        jsuint arrayLen;

        if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &sizeParam) ||
            !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &typeParam) ||
            !JSValToJSArrayAndLength(js.ctx, js.argv[2], &arrayObj, &arrayLen))
        {
            return NS_ERROR_DOM_SYNTAX_ERR;
        }

        
        
        if (typeParam != GL_UNSIGNED_BYTE &&
            typeParam != GL_FLOAT &&
            sizeParam != 4)
            return NS_ERROR_DOM_SYNTAX_ERR;

        newBuffer = new CanvasGLBuffer(this);
        if (!newBuffer)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = newBuffer->Init (GL_STATIC_DRAW, sizeParam, typeParam, js.ctx, arrayObj, arrayLen);
        if (NS_FAILED(rv))
            return rv;
    }

    mColorBuffer = newBuffer;

    MakeContextCurrent();
    glColorPointer(newBuffer->GetSimpleBuffer().sizePerVertex,
                   newBuffer->GetSimpleBuffer().type,
                   0,
                   newBuffer->GetSimpleBuffer().data);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::DrawElements()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint modeParam;
    jsuint countParam;

    JSObject *arrayObj;
    jsuint arrayLen;

    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &modeParam) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &countParam) ||
        !JSValToJSArrayAndLength(js.ctx, js.argv[2], &arrayObj, &arrayLen))
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    if (modeParam != GL_POINTS &&
        modeParam != GL_LINES &&
        modeParam != GL_LINE_STRIP &&
        modeParam != GL_LINE_LOOP &&
        modeParam != GL_TRIANGLES &&
        modeParam != GL_TRIANGLE_STRIP &&
        modeParam != GL_TRIANGLE_FAN)
        return NS_ERROR_DOM_SYNTAX_ERR;

    if (countParam > arrayLen)
        return NS_ERROR_DOM_SYNTAX_ERR;

    
    SimpleBuffer indexBuffer;
    JSArrayToSimpleBuffer(indexBuffer, GL_UNSIGNED_SHORT, 1, js.ctx, arrayObj, countParam);

    
    
    PRUint32 vLen, nLen, cLen;

    if (mVertexBuffer)
        vLen = mVertexBuffer->mLength / mVertexBuffer->mSize;
    if (mNormalBuffer)
        nLen = mNormalBuffer->mLength / 3;
    if (mColorBuffer)
        cLen = mColorBuffer->mLength / 4;

    PRUint16 *indices = (PRUint16*) indexBuffer.data;
    for (PRUint32 i = 0; i < countParam; i++) {
        if ((mVertexArrayEnabled && indices[i] >= vLen) ||
            (mNormalArrayEnabled && indices[i] >= nLen) ||
            (mColorArrayEnabled && indices[i] >= cLen))
            return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }

    MakeContextCurrent();
    glDrawElements(modeParam, countParam, indexBuffer.type, indexBuffer.data);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::DrawArrays(PRUint32 mode, PRUint32 first, PRUint32 count)
{
    
    MakeContextCurrent();
    glDrawArrays (mode, first, count);
    return NS_OK;
}


GL_SAME_METHOD_2(DepthRange, DepthRange, float, float)


GL_SAME_METHOD_4(Viewport, Viewport, PRInt32, PRInt32, PRInt32, PRInt32)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::MatrixMode(PRUint32 mode)
{
    MakeContextCurrent();
    glMatrixMode(mode);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::LoadMatrix()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 1)
        return NS_ERROR_DOM_SYNTAX_ERR;

    double mat[16];
    if (!JSValToDoubleArray(js.ctx, js.argv[0], 16, &mat[0]))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glLoadMatrixd(mat);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::MultMatrix()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 1)
        return NS_ERROR_DOM_SYNTAX_ERR;

    double mat[16];
    if (!JSValToDoubleArray(js.ctx, js.argv[0], 16, &mat[0]))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glMultMatrixd(mat);
    return NS_OK;
}


GL_SAME_METHOD_0(LoadIdentity, LoadIdentity)


GL_SAME_METHOD_4(Rotatef, Rotate, float, float, float, float)


GL_SAME_METHOD_3(Scalef, Scale, float, float, float)


GL_SAME_METHOD_3(Translatef, Translate, float, float, float)


GL_SAME_METHOD_6(Frustum, Frustum, float, float, float, float, float, float)


GL_SAME_METHOD_6(Ortho, Ortho, float, float, float, float, float, float)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::ActiveTexture(PRUint32 texture)
{
    if (!GLEW_ARB_multitexture) {
        if (texture == GL_TEXTURE0)
            return NS_OK;
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    if (texture < GL_TEXTURE0 || texture > GL_TEXTURE0+32)
        return NS_ERROR_DOM_SYNTAX_ERR;

    glActiveTexture(texture);
    return NS_OK;
}


GL_SAME_METHOD_0(PushMatrix, PushMatrix)


GL_SAME_METHOD_0(PopMatrix, PopMatrix)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::FrontFace(PRUint32 face)
{
    MakeContextCurrent();
    glFrontFace(face);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::Material()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint faceVal;
    jsuint pnameVal;
    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &faceVal) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &pnameVal))
        return NS_ERROR_DOM_SYNTAX_ERR;

    if (faceVal != GL_FRONT_AND_BACK)
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    switch (pnameVal) {
        case GL_SHININESS: {
            jsdouble dval;
            if (!::JS_ValueToNumber(js.ctx, js.argv[2], &dval))
                return NS_ERROR_DOM_SYNTAX_ERR;

            glMaterialf (faceVal, pnameVal, (float) dval);
        }
            break;
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_AMBIENT_AND_DIFFUSE: {
            float fv[4];
            if (!JSValToFloatArray(js.ctx, js.argv[2], 4, fv))
                return NS_ERROR_DOM_SYNTAX_ERR;

            glMaterialfv (faceVal, pnameVal, fv);
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GetMaterial()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::Light()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    jsuint lightIndex;
    jsuint lightParam;

    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &lightIndex) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &lightParam))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    switch (lightParam) {
        case SPOT_EXPONENT:
        case SPOT_CUTOFF:
        case CONSTANT_ATTENUATION:
        case LINEAR_ATTENUATION:
        case QUADRATIC_ATTENUATION: {
            jsdouble dv;
            if (!::JS_ValueToNumber(js.ctx, js.argv[2], &dv))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glLightf(lightIndex, lightParam, (float)dv);
        }
            break;
        case AMBIENT:
        case DIFFUSE:
        case SPECULAR:
        case POSITION:
        case SPOT_DIRECTION: {
            float fv[4];
            int cnt = 4;
            if (lightParam == GL_SPOT_DIRECTION)
                cnt = 3;

            if (!JSValToFloatArray(js.ctx, js.argv[2], cnt, fv))
                return NS_ERROR_DOM_SYNTAX_ERR;

            glLightfv(lightIndex, lightParam, fv);
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GetLight()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::LightModel()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 2)
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    jsuint lightParam;

    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &lightParam))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    switch (lightParam) {
        case LIGHT_MODEL_COLOR_CONTROL:
        case LIGHT_MODEL_LOCAL_VIEWER:
        case LIGHT_MODEL_TWO_SIDE: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[1], &ival))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glLightModeli(lightParam, (int) ival);
        }
            break;
        case LIGHT_MODEL_AMBIENT: {
            float fv[4];
            if (!JSValToFloatArray(js.ctx, js.argv[1], 4, fv))
                return NS_ERROR_DOM_SYNTAX_ERR;

            glLightModelfv(lightParam, fv);
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::ShadeModel(PRUint32 pname)
{
    MakeContextCurrent();
    glShadeModel(pname);
    return NS_OK;
}


GL_SAME_METHOD_1(PointSize, PointSize, float)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::PointParameter()
{
    if (!GLEW_ARB_point_parameters)
        return NS_ERROR_NOT_IMPLEMENTED;

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 2)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint pnameVal;
    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &pnameVal))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    switch (pnameVal) {
        case GL_POINT_SIZE_MIN:
        case GL_POINT_SIZE_MAX:
        case GL_POINT_FADE_THRESHOLD_SIZE: {
            jsdouble dval;
            if (!::JS_ValueToNumber(js.ctx, js.argv[1], &dval))
                return NS_ERROR_DOM_SYNTAX_ERR;

            glPointParameterf (pnameVal, (float) dval);
        }
            break;
        case GL_POINT_DISTANCE_ATTENUATION: {
            float fv[3];
            if (!JSValToFloatArray(js.ctx, js.argv[1], 3, fv))
                return NS_ERROR_DOM_SYNTAX_ERR;

            glPointParameterfv (pnameVal, fv);
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}


GL_SAME_METHOD_1(LineWidth, LineWidth, float)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::CullFace(PRUint32 mode)
{
    MakeContextCurrent();
    glCullFace(mode);
    return NS_OK;
}


GL_SAME_METHOD_2(PolygonOffset, PolygonOffset, float, float)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::CreateTextureObject(nsIDOMHTMLElement *imageOrCanvas,
                                                    nsICanvasRenderingContextGLTexture **aTextureObject)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::BindTextureObject(nsICanvasRenderingContextGLTexture *aTextureObject)
{
    if (!aTextureObject)
        return NS_ERROR_INVALID_ARG;

    CanvasGLTexture *texObj = (CanvasGLTexture*) aTextureObject;
    if (texObj->mDisposed)
        return NS_ERROR_INVALID_ARG;

    MakeContextCurrent();
    
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::DeleteTextureObject(nsICanvasRenderingContextGLTexture *aTextureObject)
{
    if (!aTextureObject)
        return NS_ERROR_INVALID_ARG;

    CanvasGLTexture *texObj = (CanvasGLTexture*) aTextureObject;
    if (texObj->mDisposed)
        return NS_ERROR_INVALID_ARG;

    MakeContextCurrent();
    texObj->Dispose();
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::TexImage2DHTML(PRUint32 target, nsIDOMHTMLElement *imageOrCanvas)
{
    nsresult rv;
    cairo_surface_t *cairo_surf = nsnull;
    PRUint8 *image_data = nsnull, *local_image_data = nsnull;
    PRInt32 width, height;
    nsCOMPtr<nsIURI> element_uri;
    PRBool force_write_only = PR_FALSE;

    rv = CairoSurfaceFromElement(imageOrCanvas, &cairo_surf, &image_data,
                                 &width, &height, getter_AddRefs(element_uri), &force_write_only);
    if (NS_FAILED(rv))
        return rv;

    DoDrawImageSecurityCheck(element_uri, force_write_only);

    if (target == GL_TEXTURE_2D) {
        if ((width & (width-1)) != 0 ||
            (height & (height-1)) != 0)
            return NS_ERROR_INVALID_ARG;
    } else if (GLEW_ARB_texture_rectangle && target == GL_TEXTURE_RECTANGLE_ARB) {
        if (width == 0 || height == 0)
            return NS_ERROR_INVALID_ARG;
    } else {
        return NS_ERROR_INVALID_ARG;
    }

    if (!image_data) {
        local_image_data = (PRUint8*) PR_Malloc(width * height * 4);
        if (!local_image_data)
            return NS_ERROR_FAILURE;

        cairo_surface_t *tmp = cairo_image_surface_create_for_data (local_image_data,
                                                                    CAIRO_FORMAT_ARGB32,
                                                                    width, height, width * 4);
        if (!tmp) {
            PR_Free(local_image_data);
            return NS_ERROR_FAILURE;
        }

        cairo_t *tmp_cr = cairo_create (tmp);
        cairo_set_source_surface (tmp_cr, cairo_surf, 0, 0);
        cairo_set_operator (tmp_cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint (tmp_cr);
        cairo_destroy (tmp_cr);
        cairo_surface_destroy (tmp);

        image_data = local_image_data;
    }

    
    
    
    
    PRUint8* ptr = image_data;
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
#ifdef IS_LITTLE_ENDIAN
            PRUint8 b = ptr[0];
            PRUint8 g = ptr[1];
            PRUint8 r = ptr[2];
            PRUint8 a = ptr[3];
#else
            PRUint8 a = ptr[0];
            PRUint8 r = ptr[1];
            PRUint8 g = ptr[2];
            PRUint8 b = ptr[3];
#endif
            ptr[0] = r;
            ptr[1] = g;
            ptr[2] = b;
            ptr[3] = a;
            ptr += 4;
        }
    }

    MakeContextCurrent();

    glTexImage2D(target, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    if (local_image_data)
        PR_Free(local_image_data);

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::TexParameter()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint targetVal;
    jsuint pnameVal;
    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &targetVal) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &pnameVal))
        return NS_ERROR_DOM_SYNTAX_ERR;

    if (targetVal != GL_TEXTURE_2D &&
        (!GLEW_ARB_texture_rectangle || targetVal != GL_TEXTURE_RECTANGLE_ARB))
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    MakeContextCurrent();
    switch (pnameVal) {
        case GL_TEXTURE_MIN_FILTER: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_NEAREST &&
                 ival != GL_LINEAR &&
                 ival != GL_NEAREST_MIPMAP_NEAREST &&
                 ival != GL_LINEAR_MIPMAP_NEAREST &&
                 ival != GL_NEAREST_MIPMAP_LINEAR &&
                 ival != GL_LINEAR_MIPMAP_LINEAR))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case GL_TEXTURE_MAG_FILTER: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_NEAREST &&
                 ival != GL_LINEAR))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_CLAMP &&
                 ival != GL_CLAMP_TO_EDGE &&
                 ival != GL_REPEAT))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case GL_GENERATE_MIPMAP: {
            jsuint ival;
            if (js.argv[2] == JSVAL_TRUE)
                ival = 1;
            else if (js.argv[2] == JSVAL_FALSE)
                ival = 0;
            else if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                     (ival != 0 && ival != 1))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexParameteri (targetVal, pnameVal, ival);
        }
            break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT: {
            if (GLEW_EXT_texture_filter_anisotropic) {
                jsdouble dval;
                if (!::JS_ValueToNumber(js.ctx, js.argv[2], &dval))
                    return NS_ERROR_DOM_SYNTAX_ERR;
                glTexParameterf (targetVal, pnameVal, (float) dval);
            } else {
                return NS_ERROR_NOT_IMPLEMENTED;
            }
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GetTexParameter()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::BindTexture(PRUint32 target, PRUint32 texid)
{
    if (target != GL_TEXTURE_2D &&
        (!GLEW_ARB_texture_rectangle || target != GL_TEXTURE_RECTANGLE_ARB))
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    MakeContextCurrent();
    glBindTexture(target, texid);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::DeleteTextures()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 1)
        return NS_ERROR_DOM_SYNTAX_ERR;

    JSObject *arrayObj;
    jsuint arrayLen;
    if (!JSValToJSArrayAndLength(js.ctx, js.argv[0], &arrayObj, &arrayLen)) {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    if (arrayLen == 0)
        return NS_OK;

    SimpleBuffer sbuffer;
    nsresult rv = JSArrayToSimpleBuffer(sbuffer, GL_UNSIGNED_INT, 1, js.ctx, arrayObj, arrayLen);
    if (NS_FAILED(rv))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glDeleteTextures(arrayLen, (GLuint*) sbuffer.data);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GenTextures(PRUint32 n)
{
    if (n == 0)
        return NS_OK;

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    nsAutoArrayPtr<PRUint32> textures(new PRUint32[n]);
    glGenTextures(n, textures.get());

    nsAutoArrayPtr<jsval> jsvector(new jsval[n]);
    for (PRUint32 i = 0; i < n; i++)
        jsvector[i] = INT_TO_JSVAL(textures[i]);

    JSObject *obj = JS_NewArrayObject(js.ctx, n, jsvector);
    jsval *retvalPtr;
    js.ncc->GetRetValPtr(&retvalPtr);
    *retvalPtr = OBJECT_TO_JSVAL(obj);
    js.ncc->SetReturnValueWasSet(PR_TRUE);
    
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::TexEnv()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint targetVal;
    jsuint pnameVal;
    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &targetVal) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &pnameVal))
        return NS_ERROR_DOM_SYNTAX_ERR;

    if (targetVal != GL_TEXTURE_ENV)
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    switch (pnameVal) {
        case GL_TEXTURE_ENV_MODE: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_MODULATE &&
                 ival != GL_REPLACE &&
                 ival != GL_DECAL &&
                 ival != GL_BLEND &&
                 ival != GL_ADD &&
                 ival != GL_COMBINE))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexEnvi (targetVal, pnameVal, ival);
        }
            break;
        case GL_TEXTURE_ENV_COLOR: {
            float fv[4];
            if (!JSValToFloatArray(js.ctx, js.argv[2], 4, fv))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexEnvfv (targetVal, pnameVal, fv);
        }
            break;
        case GL_COMBINE_RGB:
        case GL_COMBINE_ALPHA: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_MODULATE &&
                 ival != GL_REPLACE &&
                 ival != GL_ADD &&
                 ival != GL_ADD_SIGNED &&
                 ival != GL_INTERPOLATE))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexEnvi (targetVal, pnameVal, ival);
        }
            break;
        case GL_SRC0_RGB:
        case GL_SRC1_RGB:
        case GL_SRC2_RGB:
        case GL_SRC0_ALPHA:
        case GL_SRC1_ALPHA:
        case GL_SRC2_ALPHA: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_PRIMARY_COLOR &&
                 ival != GL_CONSTANT &&
                 ival != GL_PREVIOUS &&
                 (ival < GL_TEXTURE0 || ival > (GL_TEXTURE0+MAX_TEXTURE_UNITS))))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexEnvi (targetVal, pnameVal, ival);
        }
            break;
        case GL_OPERAND0_RGB:
        case GL_OPERAND1_RGB:
        case GL_OPERAND2_RGB: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_SRC_COLOR &&
                 ival != GL_ONE_MINUS_SRC_COLOR &&
                 ival != GL_SRC_ALPHA &&
                 ival != GL_ONE_MINUS_SRC_ALPHA))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexEnvi (targetVal, pnameVal, ival);
        }
                break;
        case GL_OPERAND0_ALPHA:
        case GL_OPERAND1_ALPHA:
        case GL_OPERAND2_ALPHA: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[2], &ival) ||
                (ival != GL_SRC_ALPHA &&
                 ival != GL_ONE_MINUS_SRC_ALPHA))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexEnvi (targetVal, pnameVal, ival);
        }
            break;
        case RGB_SCALE:
        case ALPHA_SCALE: {
            jsdouble dval;
            if (!::JS_ValueToNumber(js.ctx, js.argv[2], &dval))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glTexEnvf (targetVal, pnameVal, (float)dval);
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GetTexEnv()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::FogParameter()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 2)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint pnameVal;
    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &pnameVal))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    switch (pnameVal) {
        case FOG_MODE: {
            jsuint ival;
            if (!::JS_ValueToECMAUint32(js.ctx, js.argv[1], &ival) ||
                (ival != GL_LINEAR &&
                 ival != GL_EXP &&
                 ival != GL_EXP2))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glFogi (pnameVal, ival);
        }
            break;
        case FOG_DENSITY:
        case FOG_START:
        case FOG_END: {
            jsdouble dval;
            if (!::JS_ValueToNumber(js.ctx, js.argv[1], &dval))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glFogf (pnameVal, (float) dval);
        }
            break;
        case FOG_COLOR: {
            float fv[4];
            if (!JSValToFloatArray(js.ctx, js.argv[1], 4, fv))
                return NS_ERROR_DOM_SYNTAX_ERR;
            glFogfv (pnameVal, fv);
        }
            break;
        default:
            return NS_ERROR_DOM_SYNTAX_ERR;
    }

    return NS_OK;
}


GL_SAME_METHOD_4(Scissor, Scissor, PRInt32, PRInt32, PRInt32, PRInt32)


GL_SAME_METHOD_2(SampleCoverage, SampleCoverage, float, PRBool)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::AlphaFunc(PRUint32 func, float ref)
{
    MakeContextCurrent();
    glAlphaFunc(func, ref);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::StencilFunc(PRUint32 func, PRInt32 ref, PRUint32 mask)
{
    if (func != GL_NEVER &&
        func != GL_LESS &&
        func != GL_LEQUAL &&
        func != GL_GREATER &&
        func != GL_GEQUAL &&
        func != GL_EQUAL &&
        func != GL_NOTEQUAL &&
        func != GL_ALWAYS)
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glStencilFunc(func, ref, mask);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::StencilMask(PRUint32 mask)
{
    MakeContextCurrent();
    glStencilMask(mask);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::StencilOp(PRUint32 fail, PRUint32 zfail, PRUint32 zpass)
{
    MakeContextCurrent();
    glStencilOp(fail, zfail, zpass);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::DepthFunc(PRUint32 func)
{
    MakeContextCurrent();
    glDepthFunc(func);
    return NS_OK;
}


GL_SAME_METHOD_1(DepthMask, DepthMask, PRBool)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::BlendFunc(PRUint32 sfactor, PRUint32 dfactor)
{
    MakeContextCurrent();
    glBlendFunc(sfactor, dfactor);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::LogicOp(PRUint32 opcode)
{
    MakeContextCurrent();
    glLogicOp(opcode);
    return NS_OK;
}


GL_SAME_METHOD_4(ColorMask, ColorMask, PRBool, PRBool, PRBool, PRBool)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::Clear(PRUint32 mask)
{
    MakeContextCurrent();
    glClear(mask);
    return NS_OK;
}


GL_SAME_METHOD_4(ClearColor, ClearColor, float, float, float, float)


GL_SAME_METHOD_1(ClearDepth, ClearDepth, float)


GL_SAME_METHOD_1(ClearStencil, ClearStencil, PRInt32)


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::Hint(PRUint32 target, PRUint32 mode)
{
    
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GetParameter(PRUint32 pname)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    MakeContextCurrent();

    jsval result = JSVAL_VOID;
    JSObject *jsarr = nsnull;

    
    switch (pname) {
        case GL_ACCUM_ALPHA_BITS:
        case GL_ACCUM_BLUE_BITS:
        case GL_ACCUM_GREEN_BITS:
        case GL_ACCUM_RED_BITS:
        case GL_ALPHA_BIAS:
        case GL_ALPHA_BITS:
        case GL_ALPHA_SCALE:
        case GL_ALPHA_TEST_FUNC:
        case GL_ALPHA_TEST_REF:
        case GL_ATTRIB_STACK_DEPTH:
        case GL_AUX_BUFFERS:
        case GL_BLEND_DST:
        case GL_BLEND_SRC:
        case GL_BLUE_BIAS:
        case GL_BLUE_BITS:
        case GL_BLUE_SCALE:
        case GL_COLOR_MATERIAL_FACE:
        case GL_COLOR_MATERIAL_PARAMETER: {
            float fv;
            glGetFloatv (pname, &fv);
            if (!JS_NewDoubleValue(js.ctx, (jsdouble) fv, &result))
                return NS_ERROR_OUT_OF_MEMORY;
        }
            break;

        case GL_ALPHA_TEST:
        case GL_AUTO_NORMAL:
        case GL_BLEND:
        case GL_CLIP_PLANE0:
        case GL_CLIP_PLANE1:
        case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3:
        case GL_CLIP_PLANE4:
        case GL_CLIP_PLANE5:
        case GL_COLOR_MATERIAL: {
            GLboolean bv;
            glGetBooleanv (pname, &bv);
            if (bv)
                result = JSVAL_TRUE;
            else
                result = JSVAL_FALSE;
        }
            break;






        case GL_MODELVIEW_MATRIX:
        case GL_PROJECTION_MATRIX:
        case GL_TEXTURE_MATRIX: {
            float mat[16];
            glGetFloatv (pname, &mat[0]);
            nsAutoArrayPtr<jsval> jsvector(new jsval[16]);
            for (int i = 0; i < 16; i++) {
                if (!JS_NewDoubleValue(js.ctx, (jsdouble) mat[i], &jsvector[i]))
                    return NS_ERROR_OUT_OF_MEMORY;
            }

            jsarr = JS_NewArrayObject(js.ctx, 16, jsvector.get());
            if (!jsarr)
                return NS_ERROR_OUT_OF_MEMORY;

            js.AddGCRoot (&jsarr, "glGetParameter");

            result = OBJECT_TO_JSVAL(jsarr);
        }
            break;

        default:
            return NS_ERROR_NOT_IMPLEMENTED;
    }

    if (result == JSVAL_VOID)
        return NS_ERROR_NOT_IMPLEMENTED;

    js.SetRetVal(result);

    if (jsarr)
        js.ReleaseGCRoot(&jsarr);

    return NS_OK;
}

#if 0
NS_IMETHODIMP
nsCanvasRenderingContextGLES11::BindBufferObject(PRUint32 target,
                                                 nsICanvasRenderingContextGLBuffer *obj)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
#endif

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::CreateBuffer(nsICanvasRenderingContextGLBuffer **obj)
{
    nsresult rv;

    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 4)
        return NS_ERROR_DOM_SYNTAX_ERR;

    jsuint usageParam;
    jsuint sizeParam;
    jsuint typeParam;
    JSObject *arrayObj;
    jsuint arrayLen;

    if (!::JS_ValueToECMAUint32(js.ctx, js.argv[0], &usageParam) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[1], &sizeParam) ||
        !::JS_ValueToECMAUint32(js.ctx, js.argv[2], &typeParam) ||
        !JSValToJSArrayAndLength(js.ctx, js.argv[3], &arrayObj, &arrayLen))
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    nsRefPtr<CanvasGLBuffer> buffer = new CanvasGLBuffer(this);
    if (!buffer)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = buffer->Init(usageParam, sizeParam, typeParam, js.ctx, arrayObj, arrayLen);
    if (NS_FAILED(rv))
        return rv;

    NS_ADDREF(*obj = buffer.get());
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::BindBuffer(PRUint32 target, PRUint32 buffer)
{
    if (target != GL_ARRAY_BUFFER &&
        target != GL_ELEMENT_ARRAY_BUFFER)
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glBindBuffer (target, buffer);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GenBuffers(PRUint32 n)
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (n == 0)
        return NS_OK;

    MakeContextCurrent();

    nsAutoArrayPtr<PRUint32> buffers(new PRUint32[n]);
    glGenBuffers(n, buffers.get());

    nsAutoArrayPtr<jsval> jsvector(new jsval[n]);
    for (PRUint32 i = 0; i < n; i++)
        jsvector[i] = INT_TO_JSVAL(buffers[i]);

    JSObject *obj = JS_NewArrayObject(js.ctx, n, jsvector);
    jsval *retvalPtr;
    js.ncc->GetRetValPtr(&retvalPtr);
    *retvalPtr = OBJECT_TO_JSVAL(obj);
    js.ncc->SetReturnValueWasSet(PR_TRUE);
    
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::DeleteBuffers()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 1)
        return NS_ERROR_DOM_SYNTAX_ERR;

    JSObject *arrayObj;
    jsuint arrayLen;
    if (JSValToJSArrayAndLength(js.ctx, js.argv[0], &arrayObj, &arrayLen)) {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    if (arrayLen == 0)
        return NS_OK;

    SimpleBuffer sbuffer;
    nsresult rv = JSArrayToSimpleBuffer(sbuffer, GL_UNSIGNED_INT, 1, js.ctx, arrayObj, arrayLen);
    if (NS_FAILED(rv))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glDeleteBuffers(arrayLen, (GLuint*) sbuffer.data);

    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::BufferData()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    JSObject *arrayObj;
    jsuint arrayLen;
    jsuint target;
    jsuint type;
    jsuint usage;
    if (!::JS_ConvertArguments(js.ctx, js.argc, js.argv, "uouu", &target, &arrayObj, &type, &usage) ||
        !::JS_IsArrayObject(js.ctx, arrayObj) ||
        !::JS_GetArrayLength(js.ctx, arrayObj, &arrayLen))
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    SimpleBuffer sbuffer;
    nsresult rv = JSArrayToSimpleBuffer(sbuffer, type, 1, js.ctx, arrayObj, arrayLen);
    if (NS_FAILED(rv))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glBufferData(target, sbuffer.length * sbuffer.ElementSize(), sbuffer.data, usage);
    return NS_OK;
}


NS_IMETHODIMP
nsCanvasRenderingContextGLES11::BufferSubData()
{
    NativeJSContext js;
    if (NS_FAILED(js.error))
        return js.error;

    if (js.argc != 3)
        return NS_ERROR_DOM_SYNTAX_ERR;

    JSObject *arrayObj;
    jsuint arrayLen;
    jsuint target;
    jsuint offset;
    jsuint type;
    if (!::JS_ConvertArguments(js.ctx, js.argc, js.argv, "uuou", &target, &offset, &arrayObj, &type) ||
        !::JS_IsArrayObject(js.ctx, arrayObj) ||
        !::JS_GetArrayLength(js.ctx, arrayObj, &arrayLen))
    {
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    SimpleBuffer sbuffer;
    nsresult rv = JSArrayToSimpleBuffer(sbuffer, type, 1, js.ctx, arrayObj, arrayLen);
    if (NS_FAILED(rv))
        return NS_ERROR_DOM_SYNTAX_ERR;

    MakeContextCurrent();
    glBufferSubData(target, offset, sbuffer.length * sbuffer.ElementSize(), sbuffer.data);
    return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GluPerspective(float fovy, float apsect, float znear, float zfar)
{
    MakeContextCurrent();
    gluPerspective(fovy, apsect, znear, zfar);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContextGLES11::GluLookAt(float eyex, float eyey, float eyez,
                                          float ctrx, float ctry, float ctrz,
                                          float upx, float upy, float upz)
{
    MakeContextCurrent();
    gluLookAt(eyex, eyey, eyez,
              ctrx, ctry, ctrz,
              upx, upy, upz);
    return NS_OK;
}

