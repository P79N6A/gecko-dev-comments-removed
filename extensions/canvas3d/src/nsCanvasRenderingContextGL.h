





































#ifndef _NSCANVASRENDERINGCONTEXTGL_H_
#define _NSCANVASRENDERINGCONTEXTGL_H_

#include "nsICanvasRenderingContextGL.h"

#include <stdlib.h>
#include "prmem.h"

#include "nsICanvasRenderingContextGLBuffer.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsIDOMHTMLCanvasElement.h"

#include "nsICanvasGLPrivate.h"

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

#include "nsIServiceManager.h"
#include "nsIConsoleService.h"

#include "nsDOMError.h"

#include "nsContentUtils.h"

#include "nsIXPConnect.h"
#include "jsapi.h"

#include "cairo.h"
#include "glew.h"

#include "nsGLPbuffer.h"

extern nsIXPConnect *gXPConnect;
extern JSRuntime *gScriptRuntime;
extern nsIJSRuntimeService *gJSRuntimeService;

class nsICanvasRenderingContextGL;

class nsCanvasRenderingContextGLES11;
class nsCanvasRenderingContextGLWeb20;

class nsCanvasRenderingContextGLPrivate :
    public nsICanvasRenderingContextInternal,
    public nsSupportsWeakReference
{
    friend class nsGLPbuffer;

public:
    nsCanvasRenderingContextGLPrivate();
    virtual ~nsCanvasRenderingContextGLPrivate();

    virtual nsICanvasRenderingContextGL *GetSelf() = 0;

    virtual PRBool ValidateGL() { return PR_TRUE; }

    inline void MakeContextCurrent();
    static void LostCurrentContext(void *closure);

    inline GLEWContext *glewGetContext() {
        return mGLPbuffer->glewGetContext();
    }

#ifdef XP_WIN
    inline WGLEWContext *wglewGetContext() {
        return mGLPbuffer->wglewGetContext();
    }
#endif

    
    NS_IMETHOD SetCanvasElement(nsICanvasElement* aParentCanvas);
    NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
    NS_IMETHOD Render(nsIRenderingContext *rc);
    NS_IMETHOD RenderToSurface(cairo_surface_t *surf);
    NS_IMETHOD GetInputStream(const nsACString& aMimeType,
                              const nsAString& aEncoderOptions,
                              nsIInputStream **aStream);

protected:
    PRBool SafeToCreateCanvas3DContext();
    nsIFrame *GetCanvasLayoutFrame();
    nsresult DoSwapBuffers();
    nsresult CairoSurfaceFromElement(nsIDOMElement *imgElt,
                                     cairo_surface_t **aCairoSurface,
                                     PRUint8 **imgData,
                                     PRInt32 *widthOut, PRInt32 *heightOut,
                                     nsIURI **uriOut, PRBool *forceWriteOnlyOut);
    void DoDrawImageSecurityCheck(nsIURI* aURI, PRBool forceWriteOnly);

    nsGLPbuffer *mGLPbuffer;
    PRInt32 mWidth, mHeight;
    nsICanvasElement* mCanvasElement;

    static inline PRBool JSValToFloatArray (JSContext *ctx, jsval val,
                                            jsuint cnt, float *array)
    {
        JSObject *arrayObj;
        jsuint arrayLen;
        jsval jv;
        jsdouble dv;

        if (!::JS_ValueToObject(ctx, val, &arrayObj) ||
            !::JS_IsArrayObject(ctx, arrayObj) ||
            !::JS_GetArrayLength(ctx, arrayObj, &arrayLen) ||
            (arrayLen < cnt))
            return PR_FALSE;

        for (jsuint i = 0; i < cnt; i++) {
            ::JS_GetElement(ctx, arrayObj, i, &jv);
            if (!::JS_ValueToNumber(ctx, jv, &dv))
                return PR_FALSE;
            array[i] = (float) dv;
        }

        return PR_TRUE;
    }

    static inline PRBool JSValToDoubleArray (JSContext *ctx, jsval val,
                                             jsuint cnt, double *array)
    {
        JSObject *arrayObj;
        jsuint arrayLen;
        jsval jv;
        jsdouble dv;

        if (!::JS_ValueToObject(ctx, val, &arrayObj) ||
            !::JS_IsArrayObject(ctx, arrayObj) ||
            !::JS_GetArrayLength(ctx, arrayObj, &arrayLen) ||
            (arrayLen < cnt))
            return PR_FALSE;

        for (jsuint i = 0; i < cnt; i++) {
            ::JS_GetElement(ctx, arrayObj, i, &jv);
            if (!::JS_ValueToNumber(ctx, jv, &dv))
                return PR_FALSE;
            array[i] = dv;
        }

        return PR_TRUE;
    }

    static inline PRBool JSValToJSArrayAndLength (JSContext *ctx, jsval val,
                                                  JSObject **outObj, jsuint *outLen)
    {
        JSObject *obj = nsnull;
        jsuint len;
        if (!::JS_ValueToObject(ctx, val, &obj) ||
            !::JS_IsArrayObject(ctx, obj) ||
            !::JS_GetArrayLength(ctx, obj, &len))
        {
            return PR_FALSE;
        }

        *outObj = obj;
        *outLen = len;

        return PR_TRUE;
    }

    template<class T>
    static nsresult JSValToSpecificInterface(JSContext *ctx, jsval val, T **out)
    {
        if (JSVAL_IS_NULL(val)) {
            *out = nsnull;
            return NS_OK;
        }

        if (!JSVAL_IS_OBJECT(val))
            return NS_ERROR_DOM_SYNTAX_ERR;

        nsCOMPtr<nsISupports> isup;
        nsresult rv = gXPConnect->WrapJS(ctx, JSVAL_TO_OBJECT(val),
                                         NS_GET_IID(nsISupports),
                                         getter_AddRefs(isup));
        if (NS_FAILED(rv))
            return NS_ERROR_DOM_SYNTAX_ERR;

        nsCOMPtr<T> obj = do_QueryInterface(isup);
        if (!obj)
            return NS_ERROR_DOM_SYNTAX_ERR;

        NS_ADDREF(*out = obj.get());
        return NS_OK;
    }

    static inline JSObject *ArrayToJSArray (JSContext *ctx,
                                            const PRInt32 *vals,
                                            const PRUint32 len)
    {
        
        nsAutoArrayPtr<jsval> jsvector(new jsval[len]);
        for (PRUint32 i = 0; i < len; i++)
            jsvector[i] = INT_TO_JSVAL(vals[i]);
        return JS_NewArrayObject(ctx, len, jsvector);
    }

    static inline JSObject *ArrayToJSArray (JSContext *ctx,
                                            const PRUint32 *vals,
                                            const PRUint32 len)
    {
        
        nsAutoArrayPtr<jsval> jsvector(new jsval[len]);
        for (PRUint32 i = 0; i < len; i++)
            jsvector[i] = INT_TO_JSVAL(vals[i]);
        return JS_NewArrayObject(ctx, len, jsvector);
    }

    void LogMessage (const nsCString& errorString) {
        nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
        console->LogStringMessage(NS_ConvertUTF8toUTF16(errorString).get());
    }
};

class NativeJSContext {
public:
    NativeJSContext() {
        error = gXPConnect->GetCurrentNativeCallContext(getter_AddRefs(ncc));
        if (NS_FAILED(error))
            return;

        if (!ncc) {
            error = NS_ERROR_FAILURE;
            return;
        }

        ctx = nsnull;

        error = ncc->GetJSContext(&ctx);
        if (NS_FAILED(error))
            return;

        ncc->GetArgc(&argc);
        ncc->GetArgvPtr(&argv);
    }

    PRBool AddGCRoot (void *aPtr, const char *aName) {
        return JS_AddNamedRootRT(gScriptRuntime, aPtr, aName);
    }

    void ReleaseGCRoot (void *aPtr) {
        JS_RemoveRootRT(gScriptRuntime, aPtr);
    }

    void SetRetVal (PRInt32 val) {
        if (INT_FITS_IN_JSVAL(val))
            SetRetVal(INT_TO_JSVAL(val));
        else
            SetRetVal((double) val);
    }

    void SetRetVal (PRUint32 val) {
        if (INT_FITS_IN_JSVAL(val))
            SetRetVal(INT_TO_JSVAL((int) val));
        else
            SetRetVal((double) val);
    }

    void SetRetVal (double val) {
        jsval *vp;
        ncc->GetRetValPtr(&vp);
        JS_NewDoubleValue(ctx, val, vp);
    }

    void SetBoolRetVal (PRBool val) {
        if (val)
            SetRetVal(JSVAL_TRUE);
        else
            SetRetVal(JSVAL_FALSE);
    }

    void SetRetVal (PRInt32 *vp, PRUint32 len) {
        nsAutoArrayPtr<jsval> jsvector = new jsval[len];
        for (PRUint32 i = 0; i < len; i++)
            jsvector[i] = INT_TO_JSVAL(vp[i]);
        JSObject *jsarr = JS_NewArrayObject(ctx, len, jsvector.get());
        SetRetVal(OBJECT_TO_JSVAL(jsarr));
    }

    void SetRetVal (float *fp, PRUint32 len) {
        nsAutoArrayPtr<jsval> jsvector = new jsval[len];

        if (!JS_EnterLocalRootScope(ctx))
            return; 

        for (PRUint32 i = 0; i < len; i++)
            JS_NewDoubleValue(ctx, (jsdouble) fp[i], &jsvector[i]);
        JSObject *jsarr = JS_NewArrayObject(ctx, len, jsvector.get());
        SetRetVal(OBJECT_TO_JSVAL(jsarr));

        JS_LeaveLocalRootScope(ctx);
    }

    void SetRetVal (jsval val) {
        jsval *vp;
        ncc->GetRetValPtr(&vp);
        *vp = val;
        ncc->SetReturnValueWasSet(PR_TRUE);
    }

    nsCOMPtr<nsIXPCNativeCallContext> ncc;
    nsresult error;
    JSContext *ctx;
    PRUint32 argc;
    jsval *argv;
};

class JSObjectHelper {
public:
    JSObjectHelper(NativeJSContext *jsctx)
        : mCtx (jsctx)
    {
        mObject = JS_NewObject(mCtx->ctx, NULL, NULL, NULL);
        if (!mObject)
            return;

        if (!mCtx->AddGCRoot(&mObject, "JSObjectHelperCanvas3D"))
            mObject = nsnull;
    }

    ~JSObjectHelper() {
        if (mObject && mCtx)
            mCtx->ReleaseGCRoot(&mObject);
    }

    PRBool DefineProperty(const char *name, PRInt32 val) {
        
        if (!JS_DefineProperty(mCtx->ctx, mObject, name, INT_TO_JSVAL(val), NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    PRBool DefineProperty(const char *name, PRUint32 val) {
        
        if (!JS_DefineProperty(mCtx->ctx, mObject, name, INT_TO_JSVAL((int)val), NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    PRBool DefineProperty(const char *name, double val) {
        jsval dv;

        if (!JS_NewDoubleValue(mCtx->ctx, val, &dv))
            return PR_FALSE;

        if (!JS_DefineProperty(mCtx->ctx, mObject, name, dv, NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    PRBool DefineProperty(const char *name, JSObject *val) {
        if (!JS_DefineProperty(mCtx->ctx, mObject, name, OBJECT_TO_JSVAL(val), NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    
    PRBool DefineBoolProperty(const char *name, PRBool val) {
        if (!JS_DefineProperty(mCtx->ctx, mObject, name, val ? JS_TRUE : JS_FALSE, NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    PRBool DefineProperty(const char *name, const nsCSubstring& val) {
        JSString *jsstr = JS_NewStringCopyN(mCtx->ctx, val.BeginReading(), val.Length());
        if (!jsstr ||
            !JS_DefineProperty(mCtx->ctx, mObject, name, STRING_TO_JSVAL(jsstr), NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    PRBool DefineProperty(const char *name, const nsSubstring& val) {
        JSString *jsstr = JS_NewUCStringCopyN(mCtx->ctx, val.BeginReading(), val.Length());
        if (!jsstr ||
            !JS_DefineProperty(mCtx->ctx, mObject, name, STRING_TO_JSVAL(jsstr), NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    PRBool DefineProperty(const char *name, const char *val, PRUint32 len) {
        JSString *jsstr = JS_NewStringCopyN(mCtx->ctx, val, len);
        if (!jsstr ||
            !JS_DefineProperty(mCtx->ctx, mObject, name, STRING_TO_JSVAL(jsstr), NULL, NULL, JSPROP_ENUMERATE))
            return PR_FALSE;
        return PR_TRUE;
    }

    JSObject *Object() {
        return mObject;
    }

protected:
    NativeJSContext *mCtx;
    JSObject *mObject;
};

class SimpleBuffer {
public:
    SimpleBuffer() : type(GL_FLOAT), data(nsnull), length(0), capacity(0), sizePerVertex(0) {
    }

    ~SimpleBuffer() {
        Release();
    }

    inline PRUint32 ElementSize() {
        if (type == GL_FLOAT) return sizeof(float);
        if (type == GL_SHORT) return sizeof(short);
        if (type == GL_UNSIGNED_SHORT) return sizeof(unsigned short);
        if (type == GL_BYTE) return 1;
        if (type == GL_UNSIGNED_BYTE) return 1;
        if (type == GL_INT) return sizeof(int);
        if (type == GL_UNSIGNED_INT) return sizeof(unsigned int);
        if (type == GL_DOUBLE) return sizeof(double);
        return 0;
    }

    void Clear() {
        Release();
    }

    void Set(PRUint32 t, PRUint32 spv, PRUint32 count, void* vals) {
        Prepare(t, spv, count);

        if (count)
            memcpy(data, vals, count*ElementSize());
    }

    void Prepare(PRUint32 t, PRUint32 spv, PRUint32 count) {
        if (count == 0) {
            Release();
        } else {
            EnsureCapacity(PR_FALSE, count*ElementSize());
            type = t;
            length = count;
            sizePerVertex = spv;
        }
    }

    void Release() {
        if (data)
            PR_Free(data);
        length = 0;
        capacity = 0;
        data = nsnull;
    }

    void EnsureCapacity(PRBool preserve, PRUint32 cap) {
        if (capacity >= cap)
            return;

        void* newdata = PR_Malloc(cap);
        if (preserve && length)
            memcpy(newdata, data, length*ElementSize());
        PR_Free(data);
        data = newdata;
        capacity = cap;
    }

    PRUint32 type;
    void* data;
    PRUint32 length;        
    PRUint32 capacity;      
    PRUint32 sizePerVertex; 
};


nsresult JSArrayToSimpleBuffer (SimpleBuffer& sbuffer,
                                PRUint32 typeParam,
                                PRUint32 sizeParam,
                                JSContext *ctx,
                                JSObject *arrayObj,
                                jsuint arrayLen);

class CanvasGLTexture :
    public nsICanvasRenderingContextGLTexture,
    public nsICanvasGLTexture
{
    friend class nsCanvasRenderingContextGLES11;
    friend class nsCanvasRenderingContextGLWeb20;
public:
    CanvasGLTexture(nsCanvasRenderingContextGLPrivate *owner);
    ~CanvasGLTexture();

    NS_DECL_ISUPPORTS

    NS_DECL_NSICANVASRENDERINGCONTEXTGLTEXTURE

    nsresult Init();
    nsresult Dispose();

protected:
    PRBool mDisposed;
    nsCOMPtr<nsIWeakReference> mOwnerContext;

    
    
    PRUint32 mWidth;
    PRUint32 mHeight;
};

class CanvasGLBuffer :
    public nsICanvasRenderingContextGLBuffer,
    public nsISecurityCheckedComponent,
    public nsICanvasGLBuffer
{
    friend class nsCanvasRenderingContextGLES11;
    friend class nsCanvasRenderingContextGLWeb20;
public:

    CanvasGLBuffer(nsCanvasRenderingContextGLPrivate *owner);
    ~CanvasGLBuffer();

    
    
    nsresult Init (PRUint32 usage,
                   PRUint32 size,
                   PRUint32 type,
                   JSContext *ctx,
                   JSObject *arrayObj,
                   jsuint arrayLen);

    SimpleBuffer& GetSimpleBuffer() { return mSimpleBuffer; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASRENDERINGCONTEXTGLBUFFER
    NS_DECL_NSISECURITYCHECKEDCOMPONENT

protected:
    CanvasGLBuffer() { }

    inline GLEWContext *glewGetContext() {
        return mGlewContextPtr;
    }

    nsCOMPtr<nsIWeakReference> mOwnerContext;
    GLEWContext *mGlewContextPtr;

    PRBool mDisposed;

    PRUint32 mLength;
    PRUint32 mSize;
    PRUint32 mType;
    PRUint32 mUsage;

    SimpleBuffer mSimpleBuffer;
    GLuint mBufferID;
};






#define GL_SAME_METHOD_0(glname, name)                       \
NS_IMETHODIMP NSGL_CONTEXT_NAME::name() {       \
    MakeContextCurrent(); gl##glname(); return NS_OK;        \
}

#define GL_SAME_METHOD_1(glname, name, t1)                            \
NS_IMETHODIMP NSGL_CONTEXT_NAME::name(t1 a1) {           \
    MakeContextCurrent(); gl##glname(a1); return NS_OK;               \
}

#define GL_SAME_METHOD_2(glname, name, t1, t2)                          \
NS_IMETHODIMP NSGL_CONTEXT_NAME::name(t1 a1, t2 a2) {      \
    MakeContextCurrent(); gl##glname(a1,a2); return NS_OK;              \
}

#define GL_SAME_METHOD_3(glname, name, t1, t2, t3)                        \
NS_IMETHODIMP NSGL_CONTEXT_NAME::name(t1 a1, t2 a2, t3 a3) { \
    MakeContextCurrent(); gl##glname(a1,a2,a3); return NS_OK;             \
}

#define GL_SAME_METHOD_4(glname, name, t1, t2, t3, t4)                           \
NS_IMETHODIMP NSGL_CONTEXT_NAME::name(t1 a1, t2 a2, t3 a3, t4 a4) { \
    MakeContextCurrent(); gl##glname(a1,a2,a3,a4); return NS_OK;                 \
}

#define GL_SAME_METHOD_5(glname, name, t1, t2, t3, t4, t5)                              \
NS_IMETHODIMP NSGL_CONTEXT_NAME::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { \
    MakeContextCurrent(); gl##glname(a1,a2,a3,a4,a5); return NS_OK;                     \
}

#define GL_SAME_METHOD_6(glname, name, t1, t2, t3, t4, t5, t6)                                 \
NS_IMETHODIMP NSGL_CONTEXT_NAME::name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) { \
    MakeContextCurrent(); gl##glname(a1,a2,a3,a4,a5,a6); return NS_OK;                         \
}

#endif 
