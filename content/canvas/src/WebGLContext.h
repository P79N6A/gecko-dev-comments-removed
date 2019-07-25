






































#ifndef WEBGLCONTEXT_H_
#define WEBGLCONTEXT_H_

#include <stdarg.h>

#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#include "nsIDocShell.h"

#include "nsICanvasRenderingContextWebGL.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsWeakReference.h"
#include "nsIDOMHTMLElement.h"
#include "nsIJSNativeInitializer.h"

#include "nsGLPbuffer.h"

#include "localgl.h"

class nsIDocShell;

namespace mozilla {

class WebGLTexture;
class WebGLBuffer;
class WebGLProgram;
class WebGLShader;
class WebGLFramebuffer;
class WebGLRenderbuffer;

class WebGLZeroingObject;

class WebGLObjectBaseRefPtr
{
protected:
    friend class WebGLZeroingObject;

    WebGLObjectBaseRefPtr()
        : mRawPtr(0)
    {
    }

    WebGLObjectBaseRefPtr(nsISupports *rawPtr)
        : mRawPtr(rawPtr)
    {
    }

    void Zero() {
        if (mRawPtr) {
            
            
            mRawPtr->Release();
            mRawPtr = 0;
        }
    }

protected:
    nsISupports *mRawPtr;
};

template <class T>
class WebGLObjectRefPtr
    : public WebGLObjectBaseRefPtr
{
public:
    typedef T element_type;

    WebGLObjectRefPtr()
    { }

    WebGLObjectRefPtr(const WebGLObjectRefPtr<T>& aSmartPtr)
        : WebGLObjectBaseRefPtr(aSmartPtr.mRawPtr)
    {
        if (mRawPtr) {
            RawPtr()->AddRef();
            RawPtr()->AddRefOwner(this);
        }
    }

    WebGLObjectRefPtr(T *aRawPtr)
        : WebGLObjectBaseRefPtr(aRawPtr)
    {
        if (mRawPtr) {
            RawPtr()->AddRef();
            RawPtr()->AddRefOwner(this);
        }
    }

    WebGLObjectRefPtr(const already_AddRefed<T>& aSmartPtr)
        : WebGLObjectBaseRefPtr(aSmartPtr.mRawPtr)
          
    {
        if (mRawPtr) {
            RawPtr()->AddRef();
            RawPtr()->AddRefOwner(this);
        }
    }

    ~WebGLObjectRefPtr() {
        if (mRawPtr) {
            RawPtr()->RemoveRefOwner(this);
            RawPtr()->Release();
        }
    }

    WebGLObjectRefPtr<T>&
    operator=(const WebGLObjectRefPtr<T>& rhs)
    {
        assign_with_AddRef(static_cast<T*>(rhs.mRawPtr));
        return *this;
    }

    WebGLObjectRefPtr<T>&
    operator=(T* rhs)
    {
        assign_with_AddRef(rhs);
        return *this;
    }

    WebGLObjectRefPtr<T>&
    operator=(const already_AddRefed<T>& rhs)
    {
        assign_assuming_AddRef(static_cast<T*>(rhs.mRawPtr));
        return *this;
    }

    T* get() const {
        return const_cast<T*>(static_cast<T*>(mRawPtr));
    }

    operator T*() const {
        return get();
    }

    T* operator->() const {
        NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL WebGLObjectRefPtr with operator->()!");
        return get();
    }

    T& operator*() const {
        NS_PRECONDITION(mRawPtr != 0, "You can't dereference a NULL WebGLObjectRefPtr with operator*()!");
        return *get();
    }

private:
    T* RawPtr() { return static_cast<T*>(mRawPtr); }

    void assign_with_AddRef(T* rawPtr) {
        if (rawPtr) {
            rawPtr->AddRef();
            rawPtr->AddRefOwner(this);
        }

        assign_assuming_AddRef(rawPtr);
    }

    void assign_assuming_AddRef(T* newPtr) {
        T* oldPtr = RawPtr();
        mRawPtr = newPtr;
        if (oldPtr) {
            oldPtr->RemoveRefOwner(this);
            oldPtr->Release();
        }
    }
};

class WebGLBuffer;

struct WebGLVertexAttribData {
    WebGLVertexAttribData()
        : buf(0), stride(0), size(0), offset(0), enabled(PR_FALSE)
    { }

    WebGLObjectRefPtr<WebGLBuffer> buf;
    GLuint stride;
    GLuint size;
    GLuint offset;
    PRBool enabled;
};

class WebGLContext :
    public nsICanvasRenderingContextWebGL,
    public nsICanvasRenderingContextInternal,
    public nsSupportsWeakReference
{
public:
    WebGLContext();
    virtual ~WebGLContext();

    NS_DECL_ISUPPORTS
    NS_DECL_NSICANVASRENDERINGCONTEXTWEBGL

    
    NS_IMETHOD SetCanvasElement(nsICanvasElement* aParentCanvas);
    NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
    NS_IMETHOD InitializeWithSurface(nsIDocShell *docShell, gfxASurface *surface, PRInt32 width, PRInt32 height)
        { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Render(gfxContext *ctx, gfxPattern::GraphicsFilter f);
    NS_IMETHOD GetInputStream(const char* aMimeType,
                              const PRUnichar* aEncoderOptions,
                              nsIInputStream **aStream);
    NS_IMETHOD GetThebesSurface(gfxASurface **surface);
    NS_IMETHOD SetIsOpaque(PRBool b) { return NS_OK; };
    NS_IMETHOD SetIsShmem(PRBool b) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Redraw(const gfxRect&) { return NS_ERROR_NOT_IMPLEMENTED; }
#ifdef MOZ_IPC
    NS_IMETHOD Swap(mozilla::ipc::Shmem& aBack,
                    PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h)
                    { return NS_ERROR_NOT_IMPLEMENTED; }
#endif

protected:
    GLES20Wrap *gl;

    nsICanvasElement* mCanvasElement;

    nsGLPbuffer *mGLPbuffer;
    PRInt32 mWidth, mHeight;

    PRBool mInvalidated;

    PRBool SafeToCreateCanvas3DContext(nsICanvasElement *canvasElement);
    PRBool ValidateGL();
    PRBool ValidateBuffers(PRUint32 count);

    void Invalidate();

    void MakeContextCurrent() { mGLPbuffer->MakeContextCurrent(); }

    
    nsresult TexImage2D_base(GLenum target, GLint level, GLenum internalformat,
                             GLsizei width, GLsizei height, GLint border,
                             GLenum format, GLenum type,
                             void *data, PRUint32 byteLength);
    nsresult TexSubImage2D_base(GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,
                                GLsizei width, GLsizei height,
                                GLenum format, GLenum type,
                                void *pixels, PRUint32 byteLength);

    nsresult DOMElementToImageSurface(nsIDOMElement *imageOrCanvas,
                                      gfxImageSurface **imageOut,
                                      PRBool flipY, PRBool premultiplyAlpha);

    GLuint mActiveTexture;

    
    nsTArray<WebGLVertexAttribData> mAttribBuffers;

    
    nsTArray<WebGLObjectRefPtr<WebGLTexture> > mUniformTextures;

    
    nsTArray<WebGLObjectRefPtr<WebGLTexture> > mBound2DTextures;
    nsTArray<WebGLObjectRefPtr<WebGLTexture> > mBoundCubeMapTextures;

    WebGLObjectRefPtr<WebGLBuffer> mBoundArrayBuffer;
    WebGLObjectRefPtr<WebGLBuffer> mBoundElementArrayBuffer;
    WebGLObjectRefPtr<WebGLProgram> mCurrentProgram;

    
    nsTArray<WebGLObjectRefPtr<WebGLTexture> > mFramebufferColorAttachments;
    nsRefPtr<WebGLFramebuffer> mFramebufferDepthAttachment;
    nsRefPtr<WebGLFramebuffer> mFramebufferStencilAttachment;

    nsRefPtr<WebGLFramebuffer> mBoundFramebuffer;
    nsRefPtr<WebGLRenderbuffer> mBoundRenderbuffer;

    
    nsRefPtrHashtable<nsUint32HashKey, WebGLTexture> mMapTextures;
    nsRefPtrHashtable<nsUint32HashKey, WebGLBuffer> mMapBuffers;
    nsRefPtrHashtable<nsUint32HashKey, WebGLProgram> mMapPrograms;
    nsRefPtrHashtable<nsUint32HashKey, WebGLShader> mMapShaders;
    nsRefPtrHashtable<nsUint32HashKey, WebGLFramebuffer> mMapFramebuffers;
    nsRefPtrHashtable<nsUint32HashKey, WebGLRenderbuffer> mMapRenderbuffers;

    
    void LogMessage (const char *fmt, ...);
    nsresult ErrorMessage (const char *fmt, ...);
};





class WebGLZeroingObject
{
public:
    WebGLZeroingObject()
    { }

    void AddRefOwner(WebGLObjectBaseRefPtr *owner) {
        mRefOwners.AppendElement(owner);
    }

    void RemoveRefOwner(WebGLObjectBaseRefPtr *owner) {
        mRefOwners.RemoveElement(owner);
    }

    void ZeroOwners() {
        WebGLObjectBaseRefPtr **owners = mRefOwners.Elements();
        
        for (PRUint32 i = 0; i < mRefOwners.Length(); i++) {
            owners[i]->Zero();
        }

        mRefOwners.Clear();
    }

protected:
    nsTArray<WebGLObjectBaseRefPtr *> mRefOwners;
};

class WebGLBuffer :
    public nsIWebGLBuffer,
    public WebGLZeroingObject
{
public:
    WebGLBuffer(GLuint name)
        : mName(name), mDeleted(PR_FALSE), mByteLength(0)
    { }

    void Delete() {
        if (mDeleted)
            return;
        ZeroOwners();

        mDeleted = PR_TRUE;
        mByteLength = 0;
    }

    PRBool Deleted() { return mDeleted; }
    GLuint GLName() { return mName; }
    PRUint32 ByteLength() { return mByteLength; }

    void SetByteLength(GLuint len) {
        mByteLength = len;
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLBUFFER
protected:
    GLuint mName;
    PRBool mDeleted;
    PRUint32 mByteLength;
};

class WebGLTexture :
    public nsIWebGLTexture,
    public WebGLZeroingObject
{
public:
    WebGLTexture(GLuint name) :
        mName(name), mDeleted(PR_FALSE) { }

    void Delete() {
        if (mDeleted)
            return;
        ZeroOwners();
        mDeleted = PR_TRUE;
    }

    PRBool Deleted() { return mDeleted; }
    GLuint GLName() { return mName; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLTEXTURE
protected:
    GLuint mName;
    PRBool mDeleted;
};

class WebGLProgram :
    public nsIWebGLProgram,
    public WebGLZeroingObject
{
public:
    WebGLProgram(GLuint name) :
        mName(name), mDeleted(PR_FALSE) { }

    void Delete() {
        if (mDeleted)
            return;
        ZeroOwners();
        mDeleted = PR_TRUE;
    }
    PRBool Deleted() { return mDeleted; }
    GLuint GLName() { return mName; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLPROGRAM
protected:
    GLuint mName;
    PRBool mDeleted;
};

class WebGLShader :
    public nsIWebGLShader,
    public WebGLZeroingObject
{
public:
    WebGLShader(GLuint name) :
        mName(name), mDeleted(PR_FALSE) { }

    void Delete() {
        if (mDeleted)
            return;
        ZeroOwners();
        mDeleted = PR_TRUE;
    }
    PRBool Deleted() { return mDeleted; }
    GLuint GLName() { return mName; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLSHADER
protected:
    GLuint mName;
    PRBool mDeleted;
};

class WebGLFramebuffer :
    public nsIWebGLFramebuffer,
    public WebGLZeroingObject
{
public:
    WebGLFramebuffer(GLuint name) :
        mName(name), mDeleted(PR_FALSE) { }

    void Delete() {
        if (mDeleted)
            return;
        ZeroOwners();
        mDeleted = PR_TRUE;
    }
    PRBool Deleted() { return mDeleted; }
    GLuint GLName() { return mName; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLFRAMEBUFFER
protected:
    GLuint mName;
    PRBool mDeleted;
};

class WebGLRenderbuffer :
    public nsIWebGLRenderbuffer,
    public WebGLZeroingObject
{
public:
    WebGLRenderbuffer(GLuint name) :
        mName(name), mDeleted(PR_FALSE) { }

    void Delete() {
        if (mDeleted)
            return;
        ZeroOwners();
        mDeleted = PR_TRUE;
    }
    PRBool Deleted() { return mDeleted; }
    GLuint GLName() { return mName; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLRENDERBUFFER
protected:
    GLuint mName;
    PRBool mDeleted;
};

}

#endif
