






































#ifndef WEBGLCONTEXT_H_
#define WEBGLCONTEXT_H_

#include <stdarg.h>
#include <vector>

#include "nsTArray.h"
#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#include "nsIDocShell.h"

#include "nsIDOMWebGLRenderingContext.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsHTMLCanvasElement.h"
#include "nsWeakReference.h"
#include "nsIDOMHTMLElement.h"
#include "nsIMemoryReporter.h"
#include "nsIJSNativeInitializer.h"
#include "nsContentUtils.h"
#include "nsWrapperCache.h"
#include "nsIObserver.h"

#include "GLContextProvider.h"
#include "Layers.h"

#include "nsDataHashtable.h"

#include "mozilla/CheckedInt.h"
#include "mozilla/dom/ImageData.h"

#ifdef XP_MACOSX
#include "ForceDiscreteGPUHelperCGL.h"
#endif

#include "angle/ShaderLang.h"

#include "mozilla/dom/TypedArray.h"
#include "mozilla/dom/Nullable.h"
#include "mozilla/ErrorResult.h"








#define MINVALUE_GL_MAX_TEXTURE_SIZE                  1024  // Different from the spec, which sets it to 64 on page 162
#define MINVALUE_GL_MAX_CUBE_MAP_TEXTURE_SIZE         512   // Different from the spec, which sets it to 16 on page 162
#define MINVALUE_GL_MAX_VERTEX_ATTRIBS                8     // Page 164
#define MINVALUE_GL_MAX_FRAGMENT_UNIFORM_VECTORS      16    // Page 164
#define MINVALUE_GL_MAX_VERTEX_UNIFORM_VECTORS        128   // Page 164
#define MINVALUE_GL_MAX_VARYING_VECTORS               8     // Page 164
#define MINVALUE_GL_MAX_TEXTURE_IMAGE_UNITS           8     // Page 164
#define MINVALUE_GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS    0     // Page 164
#define MINVALUE_GL_MAX_RENDERBUFFER_SIZE             1024  // Different from the spec, which sets it to 1 on page 164
#define MINVALUE_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS  8     // Page 164

class nsIDocShell;
class nsIPropertyBag;

namespace mozilla {

class WebGLTexture;
class WebGLBuffer;
class WebGLProgram;
class WebGLShader;
class WebGLFramebuffer;
class WebGLRenderbuffer;
class WebGLUniformLocation;
class WebGLExtension;
class WebGLContext;
struct WebGLVertexAttribData;
class WebGLMemoryPressureObserver;
class WebGLRectangleObject;
class WebGLContextBoundObject;
class WebGLActiveInfo;
class WebGLShaderPrecisionFormat;

enum FakeBlackStatus { DoNotNeedFakeBlack, DoNeedFakeBlack, DontKnowIfNeedFakeBlack };

struct VertexAttrib0Status {
    enum { Default, EmulatedUninitializedArray, EmulatedInitializedArray };
};

struct BackbufferClearingStatus {
    enum { NotClearedSinceLastPresented, ClearedToDefaultValues, HasBeenDrawnTo };
};

namespace WebGLTexelConversions {









enum WebGLTexelFormat
{
    
    
    BadFormat,
    
    
    
    Auto,
    
    R8,
    A8,
    R32F, 
    A32F, 
    
    RA8,
    RA32F,
    
    RGB8,
    BGRX8, 
    RGB565,
    RGB32F, 
    
    RGBA8,
    BGRA8, 
    RGBA5551,
    RGBA4444,
    RGBA32F 
};

} 

using WebGLTexelConversions::WebGLTexelFormat;

WebGLTexelFormat GetWebGLTexelFormat(GLenum format, GLenum type);


inline bool is_pot_assuming_nonnegative(WebGLsizei x)
{
    return x && (x & (x-1)) == 0;
}













































































template<typename Derived>
class WebGLRefCountedObject
{
public:
    enum DeletionStatus { Default, DeleteRequested, Deleted };

    WebGLRefCountedObject()
      : mDeletionStatus(Default)
    { }

    ~WebGLRefCountedObject() {
        NS_ABORT_IF_FALSE(mWebGLRefCnt == 0, "destroying WebGL object still referenced by other WebGL objects");
        NS_ABORT_IF_FALSE(mDeletionStatus == Deleted, "Derived class destructor must call DeleteOnce()");
    }

    
    void WebGLAddRef() {
        ++mWebGLRefCnt;
    }

    
    void WebGLRelease() {
        NS_ABORT_IF_FALSE(mWebGLRefCnt > 0, "releasing WebGL object with WebGL refcnt already zero");
        --mWebGLRefCnt;
        MaybeDelete();
    }

    
    void RequestDelete() {
        if (mDeletionStatus == Default)
            mDeletionStatus = DeleteRequested;
        MaybeDelete();
    }

    bool IsDeleted() const {
        return mDeletionStatus == Deleted;
    }

    bool IsDeleteRequested() const {
        return mDeletionStatus != Default;
    }

    void DeleteOnce() {
        if (mDeletionStatus != Deleted) {
            static_cast<Derived*>(this)->Delete();
            mDeletionStatus = Deleted;
        }
    }

private:
    void MaybeDelete() {
        if (mWebGLRefCnt == 0 &&
            mDeletionStatus == DeleteRequested)
        {
            DeleteOnce();
        }
    }

protected:
    nsAutoRefCnt mWebGLRefCnt;
    DeletionStatus mDeletionStatus;
};















template<typename T>
class WebGLRefPtr
{
public:
    WebGLRefPtr()
        : mRawPtr(0)
    { }

    WebGLRefPtr(const WebGLRefPtr<T>& aSmartPtr)
        : mRawPtr(aSmartPtr.mRawPtr)
    {
        AddRefOnPtr(mRawPtr);
    }

    WebGLRefPtr(T *aRawPtr)
        : mRawPtr(aRawPtr)
    {
        AddRefOnPtr(mRawPtr);
    }

    ~WebGLRefPtr() {
        ReleasePtr(mRawPtr);
    }

    WebGLRefPtr<T>&
    operator=(const WebGLRefPtr<T>& rhs)
    {
        assign_with_AddRef(rhs.mRawPtr);
        return *this;
    }

    WebGLRefPtr<T>&
    operator=(T* rhs)
    {
        assign_with_AddRef(rhs);
        return *this;
    }

    T* get() const {
        return static_cast<T*>(mRawPtr);
    }

    operator T*() const {
        return get();
    }

    T* operator->() const {
        NS_ABORT_IF_FALSE(mRawPtr != 0, "You can't dereference a NULL WebGLRefPtr with operator->()!");
        return get();
    }

    T& operator*() const {
        NS_ABORT_IF_FALSE(mRawPtr != 0, "You can't dereference a NULL WebGLRefPtr with operator*()!");
        return *get();
    }

private:

    static void AddRefOnPtr(T* rawPtr) {
        if (rawPtr) {
            rawPtr->WebGLAddRef();
            rawPtr->AddRef();
        }
    }

    static void ReleasePtr(T* rawPtr) {
        if (rawPtr) {
            rawPtr->WebGLRelease(); 
            rawPtr->Release();
        }
    }

    void assign_with_AddRef(T* rawPtr) {
        AddRefOnPtr(rawPtr);
        assign_assuming_AddRef(rawPtr);
    }

    void assign_assuming_AddRef(T* newPtr) {
        T* oldPtr = mRawPtr;
        mRawPtr = newPtr;
        ReleasePtr(oldPtr);
    }

protected:
    T *mRawPtr;
};

typedef PRUint64 WebGLMonotonicHandle;






template<typename ElementType>
class WebGLFastArray
{
    struct Entry {
        ElementType mElement;
        WebGLMonotonicHandle mMonotonicHandle;

        Entry(ElementType elem, WebGLMonotonicHandle monotonicHandle)
            : mElement(elem), mMonotonicHandle(monotonicHandle)
        {}

        struct Comparator {
            bool Equals(const Entry& a, const Entry& b) const {
                return a.mMonotonicHandle == b.mMonotonicHandle;
            }
            bool LessThan(const Entry& a, const Entry& b) const {
                return a.mMonotonicHandle < b.mMonotonicHandle;
            }
        };
    };

public:
    WebGLFastArray()
        : mCurrentMonotonicHandle(0) 
    {}

    ElementType operator[](size_t index) const {
        return mArray[index].mElement;
    }

    size_t Length() const {
        return mArray.Length();
    }

    ElementType Last() const {
        return operator[](Length() - 1);
    }

    WebGLMonotonicHandle AppendElement(ElementType elem)
    {
        WebGLMonotonicHandle monotonicHandle = NextMonotonicHandle();
        mArray.AppendElement(Entry(elem, monotonicHandle));
        return monotonicHandle;
    }

    void RemoveElement(WebGLMonotonicHandle monotonicHandle)
    {
        mArray.RemoveElementSorted(Entry(ElementType(), monotonicHandle),
                                   typename Entry::Comparator());
    }

private:
    WebGLMonotonicHandle NextMonotonicHandle() {
        ++mCurrentMonotonicHandle;
        if (!mCurrentMonotonicHandle.isValid())
            NS_RUNTIMEABORT("ran out of monotonic ids!");
        return mCurrentMonotonicHandle.value();
    }

    nsTArray<Entry> mArray;
    CheckedInt<WebGLMonotonicHandle> mCurrentMonotonicHandle;
};



class WebGLRectangleObject
{
public:
    WebGLRectangleObject()
        : mWidth(0), mHeight(0) { }

    WebGLRectangleObject(WebGLsizei width, WebGLsizei height)
        : mWidth(width), mHeight(height) { }

    WebGLsizei Width() const { return mWidth; }
    void width(WebGLsizei value) { mWidth = value; }

    WebGLsizei Height() const { return mHeight; }
    void height(WebGLsizei value) { mHeight = value; }

    void setDimensions(WebGLsizei width, WebGLsizei height) {
        mWidth = width;
        mHeight = height;
    }

    void setDimensions(WebGLRectangleObject *rect) {
        if (rect) {
            mWidth = rect->Width();
            mHeight = rect->Height();
        } else {
            mWidth = 0;
            mHeight = 0;
        }
    }

    bool HasSameDimensionsAs(const WebGLRectangleObject& other) const {
        return Width() == other.Width() && Height() == other.Height(); 
    }

protected:
    WebGLsizei mWidth;
    WebGLsizei mHeight;
};

struct WebGLContextOptions {
    
    WebGLContextOptions()
        : alpha(true), depth(true), stencil(false),
          premultipliedAlpha(true), antialias(true),
          preserveDrawingBuffer(false)
    { }

    bool operator==(const WebGLContextOptions& other) const {
        return
            alpha == other.alpha &&
            depth == other.depth &&
            stencil == other.stencil &&
            premultipliedAlpha == other.premultipliedAlpha &&
            antialias == other.antialias &&
            preserveDrawingBuffer == other.preserveDrawingBuffer;
    }

    bool operator!=(const WebGLContextOptions& other) const {
        return !operator==(other);
    }

    bool alpha;
    bool depth;
    bool stencil;
    bool premultipliedAlpha;
    bool antialias;
    bool preserveDrawingBuffer;
};

class WebGLContext :
    public nsIDOMWebGLRenderingContext,
    public nsICanvasRenderingContextInternal,
    public nsSupportsWeakReference,
    public nsITimerCallback,
    public WebGLRectangleObject,
    public nsWrapperCache
{
    friend class WebGLMemoryMultiReporterWrapper;
    friend class WebGLExtensionLoseContext;
    friend class WebGLExtensionCompressedTextureS3TC;
    friend class WebGLContextUserData;
    friend class WebGLMemoryPressureObserver;

public:
    WebGLContext();
    virtual ~WebGLContext();

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS

    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(WebGLContext,
                                                           nsIDOMWebGLRenderingContext)

    nsINode* GetParentObject() {
        return HTMLCanvasElement();
    }

    NS_DECL_NSIDOMWEBGLRENDERINGCONTEXT

    NS_DECL_NSITIMERCALLBACK

    
    NS_IMETHOD SetCanvasElement(nsHTMLCanvasElement* aParentCanvas);
    nsHTMLCanvasElement* HTMLCanvasElement() const {
        return static_cast<nsHTMLCanvasElement*>(mCanvasElement.get());
    }

    NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
    NS_IMETHOD InitializeWithSurface(nsIDocShell *docShell, gfxASurface *surface, PRInt32 width, PRInt32 height)
        { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Reset()
        {  return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Render(gfxContext *ctx,
                      gfxPattern::GraphicsFilter f,
                      PRUint32 aFlags = RenderFlagPremultAlpha);
    NS_IMETHOD GetInputStream(const char* aMimeType,
                              const PRUnichar* aEncoderOptions,
                              nsIInputStream **aStream);
    NS_IMETHOD GetThebesSurface(gfxASurface **surface);
    mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetSurfaceSnapshot()
        { return nsnull; }

    NS_IMETHOD SetIsOpaque(bool b) { return NS_OK; };
    NS_IMETHOD SetContextOptions(nsIPropertyBag *aOptions);

    NS_IMETHOD SetIsIPC(bool b) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Redraw(const gfxRect&) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Swap(mozilla::ipc::Shmem& aBack,
                    PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h)
                    { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD Swap(PRUint32 nativeID,
                    PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h)
                    { return NS_ERROR_NOT_IMPLEMENTED; }

    bool LoseContext();
    bool RestoreContext();

    void SynthesizeGLError(WebGLenum err);
    void SynthesizeGLError(WebGLenum err, const char *fmt, ...);

    void ErrorInvalidEnum(const char *fmt = 0, ...);
    void ErrorInvalidOperation(const char *fmt = 0, ...);
    void ErrorInvalidValue(const char *fmt = 0, ...);
    void ErrorInvalidFramebufferOperation(const char *fmt = 0, ...);
    void ErrorInvalidEnumInfo(const char *info, PRUint32 enumvalue) {
        return ErrorInvalidEnum("%s: invalid enum value 0x%x", info, enumvalue);
    }
    void ErrorOutOfMemory(const char *fmt = 0, ...);

    const char *ErrorName(GLenum error);
    bool IsTextureFormatCompressed(GLenum format);

    void DummyFramebufferOperation(const char *info);

    WebGLTexture *activeBoundTextureForTarget(WebGLenum target) {
        return target == LOCAL_GL_TEXTURE_2D ? mBound2DTextures[mActiveTexture]
                                             : mBoundCubeMapTextures[mActiveTexture];
    }

    already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                                 CanvasLayer *aOldLayer,
                                                 LayerManager *aManager);
    void MarkContextClean() { mInvalidated = false; }

    
    
    PRUint32 Generation() { return mGeneration.value(); }

    const WebGLRectangleObject *FramebufferRectangleObject() const;

    
    
    
    void ForceClearFramebufferWithDefaultValues(PRUint32 mask, const nsIntRect& viewportRect);

    
    
    
    void EnsureBackbufferClearedAsNeeded();

    
    
    void UpdateWebGLErrorAndClearGLError(GLenum *currentGLError) {
        
        *currentGLError = gl->GetAndClearError();
        
        if (!mWebGLError)
            mWebGLError = *currentGLError;
    }
    
    
    
    void UpdateWebGLErrorAndClearGLError() {
        GLenum currentGLError;
        UpdateWebGLErrorAndClearGLError(&currentGLError);
    }
    
    bool MinCapabilityMode() const {
        return mMinCapability;
    }

    void SetupContextLossTimer() {
        
        
        
        
        if (mContextLossTimerRunning) {
            mDrawSinceContextLossTimerSet = true;
            return;
        }
        
        mContextRestorer->InitWithCallback(static_cast<nsITimerCallback*>(this),
                                           PR_MillisecondsToInterval(1000),
                                           nsITimer::TYPE_ONE_SHOT);
        mContextLossTimerRunning = true;
        mDrawSinceContextLossTimerSet = false;
    }

    void TerminateContextLossTimer() {
        if (mContextLossTimerRunning) {
            mContextRestorer->Cancel();
            mContextLossTimerRunning = false;
        }
    }

    
    nsHTMLCanvasElement* GetCanvas() const {
        return HTMLCanvasElement();
    }
    WebGLsizei GetDrawingBufferWidth() const {
        if (!IsContextStable())
            return 0;
        return mWidth;
    }
    WebGLsizei GetDrawingBufferHeight() const {
        if (!IsContextStable())
            return 0;
        return mHeight;
    }
        
    JSObject *GetContextAttributes(ErrorResult &rv);
    bool IsContextLost() const { return !IsContextStable(); }
    void GetSupportedExtensions(dom::Nullable< nsTArray<nsString> > &retval);
    nsIWebGLExtension* GetExtension(const nsAString& aName);
    void ActiveTexture(WebGLenum texture);
    void AttachShader(WebGLProgram* program, WebGLShader* shader);
    void BindAttribLocation(WebGLProgram* program, WebGLuint location,
                            const nsAString& name);
    void BindBuffer(WebGLenum target, WebGLBuffer* buf);
    void BindFramebuffer(WebGLenum target, WebGLFramebuffer* wfb);
    void BindRenderbuffer(WebGLenum target, WebGLRenderbuffer* wrb);
    void BindTexture(WebGLenum target, WebGLTexture *tex);
    void BlendColor(WebGLclampf r, WebGLclampf g, WebGLclampf b, WebGLclampf a) {
        if (!IsContextStable())
            return;
        MakeContextCurrent();
        gl->fBlendColor(r, g, b, a);
    }
    void BlendEquation(WebGLenum mode);
    void BlendEquationSeparate(WebGLenum modeRGB, WebGLenum modeAlpha);
    void BlendFunc(WebGLenum sfactor, WebGLenum dfactor);
    void BlendFuncSeparate(WebGLenum srcRGB, WebGLenum dstRGB,
                           WebGLenum srcAlpha, WebGLenum dstAlpha);
    void BufferData(WebGLenum target, WebGLsizeiptr size, WebGLenum usage);
    void BufferData(WebGLenum target, dom::ArrayBufferView &data,
                    WebGLenum usage);
    void BufferData(WebGLenum target, dom::ArrayBuffer *data, WebGLenum usage);
    void BufferSubData(WebGLenum target, WebGLsizeiptr byteOffset,
                       dom::ArrayBufferView &data);
    void BufferSubData(WebGLenum target, WebGLsizeiptr byteOffset,
                       dom::ArrayBuffer *data);
    WebGLenum CheckFramebufferStatus(WebGLenum target);
    void Clear(WebGLbitfield mask);
    void ClearColor(WebGLclampf r, WebGLclampf g, WebGLclampf b, WebGLclampf a);
    void ClearDepth(WebGLclampf v);
    void ClearStencil(WebGLint v);
    void ColorMask(WebGLboolean r, WebGLboolean g, WebGLboolean b, WebGLboolean a);
    void CompileShader(WebGLShader *shader);
    void CompressedTexImage2D(WebGLenum target, WebGLint level,
                              WebGLenum internalformat, WebGLsizei width,
                              WebGLsizei height, WebGLint border,
                              dom::ArrayBufferView& view);
    void CompressedTexSubImage2D(WebGLenum target, WebGLint level,
                                 WebGLint xoffset, WebGLint yoffset,
                                 WebGLsizei width, WebGLsizei height,
                                 WebGLenum format, dom::ArrayBufferView& view);
    void CopyTexImage2D(WebGLenum target, WebGLint level,
                        WebGLenum internalformat, WebGLint x, WebGLint y,
                        WebGLsizei width, WebGLsizei height, WebGLint border);
    void CopyTexSubImage2D(WebGLenum target, WebGLint level, WebGLint xoffset,
                           WebGLint yoffset, WebGLint x, WebGLint y,
                           WebGLsizei width, WebGLsizei height);
    already_AddRefed<WebGLBuffer> CreateBuffer();
    already_AddRefed<WebGLFramebuffer> CreateFramebuffer();
    already_AddRefed<WebGLProgram> CreateProgram();
    already_AddRefed<WebGLRenderbuffer> CreateRenderbuffer();
    already_AddRefed<WebGLTexture> CreateTexture();
    already_AddRefed<WebGLShader> CreateShader(WebGLenum type);
    void CullFace(WebGLenum face);
    void DeleteBuffer(WebGLBuffer *buf);
    void DeleteFramebuffer(WebGLFramebuffer *fbuf);
    void DeleteProgram(WebGLProgram *prog);
    void DeleteRenderbuffer(WebGLRenderbuffer *rbuf);
    void DeleteShader(WebGLShader *shader);
    void DeleteTexture(WebGLTexture *tex);
    void DepthFunc(WebGLenum func);
    void DepthMask(WebGLboolean b);
    void DepthRange(WebGLclampf zNear, WebGLclampf zFar);
    void DetachShader(WebGLProgram *program, WebGLShader *shader);
    void Disable(WebGLenum cap);
    void DisableVertexAttribArray(WebGLuint index);
    void DrawArrays(GLenum mode, WebGLint first, WebGLsizei count);
    void DrawElements(WebGLenum mode, WebGLsizei count, WebGLenum type,
                      WebGLintptr byteOffset);
    void Enable(WebGLenum cap);
    void EnableVertexAttribArray(WebGLuint index);
    void Flush() {
        if (!IsContextStable())
            return;
        MakeContextCurrent();
        gl->fFlush();
    }
    void Finish() {
        if (!IsContextStable())
            return;
        MakeContextCurrent();
        gl->fFinish();
    }
    void FramebufferRenderbuffer(WebGLenum target, WebGLenum attachment,
                                 WebGLenum rbtarget, WebGLRenderbuffer *wrb);
    void FramebufferTexture2D(WebGLenum target, WebGLenum attachment,
                              WebGLenum textarget, WebGLTexture *tobj,
                              WebGLint level);
    void FrontFace(WebGLenum mode);
    void GenerateMipmap(WebGLenum target);
    already_AddRefed<WebGLActiveInfo> GetActiveAttrib(WebGLProgram *prog,
                                                      WebGLuint index);
    already_AddRefed<WebGLActiveInfo> GetActiveUniform(WebGLProgram *prog,
                                                       WebGLuint index);
    void GetAttachedShaders(WebGLProgram* prog,
                            dom::Nullable< nsTArray<WebGLShader*> > &retval);
    WebGLint GetAttribLocation(WebGLProgram* prog, const nsAString& name);
    JS::Value GetBufferParameter(WebGLenum target, WebGLenum pname);
    JS::Value GetParameter(JSContext* cx, WebGLenum pname, ErrorResult& rv);
    WebGLenum GetError();
    JS::Value GetFramebufferAttachmentParameter(JSContext* cx,
                                                WebGLenum target,
                                                WebGLenum attachment,
                                                WebGLenum pname,
                                                ErrorResult& rv);
    JS::Value GetProgramParameter(WebGLProgram *prog, WebGLenum pname);
    void GetProgramInfoLog(WebGLProgram *prog, nsAString& retval, ErrorResult& rv);
    JS::Value GetRenderbufferParameter(WebGLenum target, WebGLenum pname);
    JS::Value GetShaderParameter(WebGLShader *shader, WebGLenum pname);
    already_AddRefed<WebGLShaderPrecisionFormat>
      GetShaderPrecisionFormat(WebGLenum shadertype, WebGLenum precisiontype);
    void GetShaderInfoLog(WebGLShader *shader, nsAString& retval, ErrorResult& rv);
    void GetShaderSource(WebGLShader *shader, nsAString& retval);
    JS::Value GetTexParameter(WebGLenum target, WebGLenum pname);
    JS::Value GetUniform(JSContext* cx, WebGLProgram *prog,
                         WebGLUniformLocation *location, ErrorResult& rv);
    already_AddRefed<WebGLUniformLocation>
      GetUniformLocation(WebGLProgram *prog, const nsAString& name);
    JS::Value GetVertexAttrib(JSContext* cx, WebGLuint index, WebGLenum pname,
                              ErrorResult& rv);
    WebGLsizeiptr GetVertexAttribOffset(WebGLuint index, WebGLenum pname);
    void Hint(WebGLenum target, WebGLenum mode);
    bool IsBuffer(WebGLBuffer *buffer);
    bool IsEnabled(WebGLenum cap);
    bool IsFramebuffer(WebGLFramebuffer *fb);
    bool IsProgram(WebGLProgram *prog);
    bool IsRenderbuffer(WebGLRenderbuffer *rb);
    bool IsShader(WebGLShader *shader);
    bool IsTexture(WebGLTexture *tex);
    void LineWidth(WebGLfloat width) {
        if (!IsContextStable())
            return;
        MakeContextCurrent();
        gl->fLineWidth(width);
    }
    void LinkProgram(WebGLProgram *program, ErrorResult& rv);
    void PixelStorei(WebGLenum pname, WebGLint param);
    void PolygonOffset(WebGLfloat factor, WebGLfloat units) {
        if (!IsContextStable())
            return;
        MakeContextCurrent();
        gl->fPolygonOffset(factor, units);
    }
    void ReadPixels(WebGLint x, WebGLint y, WebGLsizei width, WebGLsizei height,
                    WebGLenum format, WebGLenum type,
                    dom::ArrayBufferView* pixels, ErrorResult& rv);
    void RenderbufferStorage(WebGLenum target, WebGLenum internalformat,
                             WebGLsizei width, WebGLsizei height);
    void SampleCoverage(WebGLclampf value, WebGLboolean invert) {
        if (!IsContextStable())
            return;
        MakeContextCurrent();
        gl->fSampleCoverage(value, invert);
    }
    void Scissor(WebGLint x, WebGLint y, WebGLsizei width, WebGLsizei height);
    void ShaderSource(WebGLShader *shader, const nsAString& source);
    void StencilFunc(WebGLenum func, WebGLint ref, WebGLuint mask);
    void StencilFuncSeparate(WebGLenum face, WebGLenum func, WebGLint ref,
                             WebGLuint mask);
    void StencilMask(WebGLuint mask);
    void StencilMaskSeparate(WebGLenum face, WebGLuint mask);
    void StencilOp(WebGLenum sfail, WebGLenum dpfail, WebGLenum dppass);
    void StencilOpSeparate(WebGLenum face, WebGLenum sfail, WebGLenum dpfail,
                           WebGLenum dppass);
    void TexImage2D(JSContext* cx, WebGLenum target, WebGLint level,
                    WebGLenum internalformat, WebGLsizei width,
                    WebGLsizei height, WebGLint border, WebGLenum format,
                    WebGLenum type, dom::ArrayBufferView *pixels,
                    ErrorResult& rv);
    void TexImage2D(JSContext* cx, WebGLenum target, WebGLint level,
                    WebGLenum internalformat, WebGLenum format, WebGLenum type,
                    dom::ImageData* pixels, ErrorResult& rv);
    void TexImage2D(JSContext* , WebGLenum target, WebGLint level,
                    WebGLenum internalformat, WebGLenum format, WebGLenum type,
                    dom::Element* elt, ErrorResult& rv);
    void TexParameterf(WebGLenum target, WebGLenum pname, WebGLfloat param) {
        TexParameter_base(target, pname, nsnull, &param);
    }
    void TexParameteri(WebGLenum target, WebGLenum pname, WebGLint param) {
        TexParameter_base(target, pname, &param, nsnull);
    }
    
    void TexSubImage2D(JSContext* cx, WebGLenum target, WebGLint level,
                       WebGLint xoffset, WebGLint yoffset,
                       WebGLsizei width, WebGLsizei height, WebGLenum format,
                       WebGLenum type, dom::ArrayBufferView* pixels,
                       ErrorResult& rv);
    void TexSubImage2D(JSContext* cx, WebGLenum target, WebGLint level,
                       WebGLint xoffset, WebGLint yoffset, WebGLenum format,
                       WebGLenum type, dom::ImageData* pixels, ErrorResult& rv);
    void TexSubImage2D(JSContext* , WebGLenum target, WebGLint level,
                       WebGLint xoffset, WebGLint yoffset, WebGLenum format,
                       WebGLenum type, dom::Element* elt, ErrorResult& rv);

    void Uniform1i(WebGLUniformLocation* location, WebGLint x);
    void Uniform2i(WebGLUniformLocation* location, WebGLint x, WebGLint y);
    void Uniform3i(WebGLUniformLocation* location, WebGLint x, WebGLint y,
                   WebGLint z);
    void Uniform4i(WebGLUniformLocation* location, WebGLint x, WebGLint y,
                   WebGLint z, WebGLint w);

    void Uniform1f(WebGLUniformLocation* location, WebGLfloat x);
    void Uniform2f(WebGLUniformLocation* location, WebGLfloat x, WebGLfloat y);
    void Uniform3f(WebGLUniformLocation* location, WebGLfloat x, WebGLfloat y,
                   WebGLfloat z);
    void Uniform4f(WebGLUniformLocation* location, WebGLfloat x, WebGLfloat y,
                   WebGLfloat z, WebGLfloat w);
    
    void Uniform1iv(WebGLUniformLocation* location, dom::Int32Array& arr) {
        Uniform1iv_base(location, arr.mLength, arr.mData);
    }
    void Uniform1iv(WebGLUniformLocation* location, nsTArray<WebGLint>& arr) {
        Uniform1iv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform1iv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLint* data);

    void Uniform2iv(WebGLUniformLocation* location, dom::Int32Array& arr) {
        Uniform2iv_base(location, arr.mLength, arr.mData);
    }
    void Uniform2iv(WebGLUniformLocation* location, nsTArray<WebGLint>& arr) {
        Uniform2iv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform2iv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLint* data);

    void Uniform3iv(WebGLUniformLocation* location, dom::Int32Array& arr) {
        Uniform3iv_base(location, arr.mLength, arr.mData);
    }
    void Uniform3iv(WebGLUniformLocation* location, nsTArray<WebGLint>& arr) {
        Uniform3iv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform3iv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLint* data);
    
    void Uniform4iv(WebGLUniformLocation* location, dom::Int32Array& arr) {
        Uniform4iv_base(location, arr.mLength, arr.mData);
    }
    void Uniform4iv(WebGLUniformLocation* location, nsTArray<WebGLint>& arr) {
        Uniform4iv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform4iv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLint* data);

    void Uniform1fv(WebGLUniformLocation* location, dom::Float32Array& arr) {
        Uniform1fv_base(location, arr.mLength, arr.mData);
    }
    void Uniform1fv(WebGLUniformLocation* location, nsTArray<WebGLfloat>& arr) {
        Uniform1fv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform1fv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLfloat* data);

    void Uniform2fv(WebGLUniformLocation* location, dom::Float32Array& arr) {
        Uniform2fv_base(location, arr.mLength, arr.mData);
    }
    void Uniform2fv(WebGLUniformLocation* location, nsTArray<WebGLfloat>& arr) {
        Uniform2fv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform2fv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLfloat* data);

    void Uniform3fv(WebGLUniformLocation* location, dom::Float32Array& arr) {
        Uniform3fv_base(location, arr.mLength, arr.mData);
    }
    void Uniform3fv(WebGLUniformLocation* location, nsTArray<WebGLfloat>& arr) {
        Uniform3fv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform3fv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLfloat* data);
    
    void Uniform4fv(WebGLUniformLocation* location, dom::Float32Array& arr) {
        Uniform4fv_base(location, arr.mLength, arr.mData);
    }
    void Uniform4fv(WebGLUniformLocation* location, nsTArray<WebGLfloat>& arr) {
        Uniform4fv_base(location, arr.Length(), arr.Elements());
    }
    void Uniform4fv_base(WebGLUniformLocation* location, uint32_t arrayLength,
                         WebGLfloat* data);

    void UniformMatrix2fv(WebGLUniformLocation* location,
                          WebGLboolean transpose,
                          dom::Float32Array &value) {
        UniformMatrix2fv_base(location, transpose, value.mLength, value.mData);
    }
    void UniformMatrix2fv(WebGLUniformLocation* location,
                          WebGLboolean transpose,
                          nsTArray<float> &value) {
        UniformMatrix2fv_base(location, transpose, value.Length(),
                              value.Elements());
    }
    void UniformMatrix2fv_base(WebGLUniformLocation* location,
                               WebGLboolean transpose, uint32_t arrayLength,
                               float* data);

    void UniformMatrix3fv(WebGLUniformLocation* location,
                          WebGLboolean transpose,
                          dom::Float32Array &value) {
        UniformMatrix3fv_base(location, transpose, value.mLength, value.mData);
    }
    void UniformMatrix3fv(WebGLUniformLocation* location,
                          WebGLboolean transpose,
                          nsTArray<float> &value) {
        UniformMatrix3fv_base(location, transpose, value.Length(),
                              value.Elements());
    }
    void UniformMatrix3fv_base(WebGLUniformLocation* location,
                               WebGLboolean transpose, uint32_t arrayLength,
                               float* data);

    void UniformMatrix4fv(WebGLUniformLocation* location,
                          WebGLboolean transpose,
                          dom::Float32Array &value) {
        UniformMatrix4fv_base(location, transpose, value.mLength, value.mData);
    }
    void UniformMatrix4fv(WebGLUniformLocation* location,
                          WebGLboolean transpose,
                          nsTArray<float> &value) {
        UniformMatrix4fv_base(location, transpose, value.Length(),
                              value.Elements());
    }
    void UniformMatrix4fv_base(WebGLUniformLocation* location,
                               WebGLboolean transpose, uint32_t arrayLength,
                               float* data);

    void UseProgram(WebGLProgram *prog);
    void ValidateProgram(WebGLProgram *prog);

    void VertexAttrib1f(WebGLuint index, WebGLfloat x0);
    void VertexAttrib2f(WebGLuint index, WebGLfloat x0, WebGLfloat x1);
    void VertexAttrib3f(WebGLuint index, WebGLfloat x0, WebGLfloat x1,
                        WebGLfloat x2);
    void VertexAttrib4f(WebGLuint index, WebGLfloat x0, WebGLfloat x1,
                        WebGLfloat x2, WebGLfloat x3);

    void VertexAttrib1fv(WebGLuint idx, dom::Float32Array &arr) {
        VertexAttrib1fv_base(idx, arr.mLength, arr.mData);
    }
    void VertexAttrib1fv(WebGLuint idx, nsTArray<WebGLfloat>& arr) {
        VertexAttrib1fv_base(idx, arr.Length(), arr.Elements());
    }
    void VertexAttrib1fv_base(WebGLuint idx, uint32_t arrayLength,
                              WebGLfloat* ptr);

    void VertexAttrib2fv(WebGLuint idx, dom::Float32Array &arr) {
        VertexAttrib2fv_base(idx, arr.mLength, arr.mData);
    }
    void VertexAttrib2fv(WebGLuint idx, nsTArray<WebGLfloat>& arr) {
        VertexAttrib2fv_base(idx, arr.Length(), arr.Elements());
    }
    void VertexAttrib2fv_base(WebGLuint idx, uint32_t arrayLength,
                              WebGLfloat* ptr);

    void VertexAttrib3fv(WebGLuint idx, dom::Float32Array &arr) {
        VertexAttrib3fv_base(idx, arr.mLength, arr.mData);
    }
    void VertexAttrib3fv(WebGLuint idx, nsTArray<WebGLfloat>& arr) {
        VertexAttrib3fv_base(idx, arr.Length(), arr.Elements());
    }
    void VertexAttrib3fv_base(WebGLuint idx, uint32_t arrayLength,
                              WebGLfloat* ptr);

    void VertexAttrib4fv(WebGLuint idx, dom::Float32Array &arr) {
        VertexAttrib4fv_base(idx, arr.mLength, arr.mData);
    }
    void VertexAttrib4fv(WebGLuint idx, nsTArray<WebGLfloat>& arr) {
        VertexAttrib4fv_base(idx, arr.Length(), arr.Elements());
    }
    void VertexAttrib4fv_base(WebGLuint idx, uint32_t arrayLength,
                              WebGLfloat* ptr);
    
    void VertexAttribPointer(WebGLuint index, WebGLint size, WebGLenum type,
                             WebGLboolean normalized, WebGLsizei stride,
                             WebGLintptr byteOffset);
    void Viewport(WebGLint x, WebGLint y, WebGLsizei width, WebGLsizei height);

protected:
    void SetDontKnowIfNeedFakeBlack() {
        mFakeBlackStatus = DontKnowIfNeedFakeBlack;
    }

    bool NeedFakeBlack();
    void BindFakeBlackTextures();
    void UnbindFakeBlackTextures();

    int WhatDoesVertexAttrib0Need();
    bool DoFakeVertexAttrib0(WebGLuint vertexCount);
    void UndoFakeVertexAttrib0();
    void InvalidateFakeVertexAttrib0();

    static CheckedUint32 GetImageSize(WebGLsizei height, 
                                      WebGLsizei width, 
                                      PRUint32 pixelSize,
                                      PRUint32 alignment);

    
    static CheckedUint32 RoundedToNextMultipleOf(CheckedUint32 x, CheckedUint32 y) {
        return ((x + y - 1) / y) * y;
    }

    nsCOMPtr<nsIDOMHTMLCanvasElement> mCanvasElement;

    nsRefPtr<gl::GLContext> gl;

    CheckedUint32 mGeneration;

    WebGLContextOptions mOptions;

    bool mInvalidated;
    bool mResetLayer;
    bool mVerbose;
    bool mOptionsFrozen;
    bool mMinCapability;
    bool mDisableExtensions;
    bool mHasRobustness;

    template<typename WebGLObjectType>
    void DeleteWebGLObjectsArray(nsTArray<WebGLObjectType>& array);

    WebGLuint mActiveTexture;
    WebGLenum mWebGLError;

    
    bool mShaderValidation;

    
    PRInt32 mGLMaxVertexAttribs;
    PRInt32 mGLMaxTextureUnits;
    PRInt32 mGLMaxTextureSize;
    PRInt32 mGLMaxCubeMapTextureSize;
    PRInt32 mGLMaxTextureImageUnits;
    PRInt32 mGLMaxVertexTextureImageUnits;
    PRInt32 mGLMaxVaryingVectors;
    PRInt32 mGLMaxFragmentUniformVectors;
    PRInt32 mGLMaxVertexUniformVectors;

    
    
    
    
    enum ContextStatus {
        
        ContextStable,
        
        
        ContextLostAwaitingEvent,
        
        
        ContextLost,
        
        
        
        ContextLostAwaitingRestore
    };

    
    enum WebGLExtensionID {
        WebGL_OES_texture_float,
        WebGL_OES_standard_derivatives,
        WebGL_EXT_texture_filter_anisotropic,
        WebGL_WEBGL_lose_context,
        WebGL_WEBGL_compressed_texture_s3tc,
        WebGLExtensionID_Max
    };
    nsAutoTArray<nsRefPtr<WebGLExtension>, WebGLExtensionID_Max> mEnabledExtensions;
    bool IsExtensionEnabled(WebGLExtensionID ext) const {
        NS_ABORT_IF_FALSE(ext >= 0 && ext < WebGLExtensionID_Max, "bogus index!");
        return mEnabledExtensions[ext] != nsnull;
    }
    bool IsExtensionSupported(WebGLExtensionID ei);

    nsTArray<WebGLenum> mCompressedTextureFormats;

    bool InitAndValidateGL();
    bool ValidateBuffers(PRInt32* maxAllowedCount, const char *info);
    bool ValidateCapabilityEnum(WebGLenum cap, const char *info);
    bool ValidateBlendEquationEnum(WebGLenum cap, const char *info);
    bool ValidateBlendFuncDstEnum(WebGLenum mode, const char *info);
    bool ValidateBlendFuncSrcEnum(WebGLenum mode, const char *info);
    bool ValidateBlendFuncEnumsCompatibility(WebGLenum sfactor, WebGLenum dfactor, const char *info);
    bool ValidateTextureTargetEnum(WebGLenum target, const char *info);
    bool ValidateComparisonEnum(WebGLenum target, const char *info);
    bool ValidateStencilOpEnum(WebGLenum action, const char *info);
    bool ValidateFaceEnum(WebGLenum face, const char *info);
    bool ValidateBufferUsageEnum(WebGLenum target, const char *info);
    bool ValidateTexFormatAndType(WebGLenum format, WebGLenum type, int jsArrayType,
                                      PRUint32 *texelSize, const char *info);
    bool ValidateDrawModeEnum(WebGLenum mode, const char *info);
    bool ValidateAttribIndex(WebGLuint index, const char *info);
    bool ValidateStencilParamsForDrawCall();
    
    bool ValidateGLSLVariableName(const nsAString& name, const char *info);
    bool ValidateGLSLCharacter(PRUnichar c);
    bool ValidateGLSLString(const nsAString& string, const char *info);

    bool ValidateTexImage2DTarget(WebGLenum target, WebGLsizei width, WebGLsizei height, const char* info);
    bool ValidateCompressedTextureSize(WebGLint level, WebGLenum format, WebGLsizei width, WebGLsizei height, uint32_t byteLength, const char* info);
    bool ValidateLevelWidthHeightForTarget(WebGLenum target, WebGLint level, WebGLsizei width, WebGLsizei height, const char* info);

    static PRUint32 GetBitsPerTexel(WebGLenum format, WebGLenum type);

    void Invalidate();
    void DestroyResourcesAndContext();

    void MakeContextCurrent() { gl->MakeCurrent(); }

    
    void TexImage2D_base(WebGLenum target, WebGLint level, WebGLenum internalformat,
                         WebGLsizei width, WebGLsizei height, WebGLsizei srcStrideOrZero, WebGLint border,
                         WebGLenum format, WebGLenum type,
                         void *data, PRUint32 byteLength,
                         int jsArrayType,
                         WebGLTexelFormat srcFormat, bool srcPremultiplied);
    void TexSubImage2D_base(WebGLenum target, WebGLint level,
                            WebGLint xoffset, WebGLint yoffset,
                            WebGLsizei width, WebGLsizei height, WebGLsizei srcStrideOrZero,
                            WebGLenum format, WebGLenum type,
                            void *pixels, PRUint32 byteLength,
                            int jsArrayType,
                            WebGLTexelFormat srcFormat, bool srcPremultiplied);
    void TexParameter_base(WebGLenum target, WebGLenum pname,
                           WebGLint *intParamPtr, WebGLfloat *floatParamPtr);

    void ConvertImage(size_t width, size_t height, size_t srcStride, size_t dstStride,
                      const PRUint8*src, PRUint8 *dst,
                      WebGLTexelFormat srcFormat, bool srcPremultiplied,
                      WebGLTexelFormat dstFormat, bool dstPremultiplied,
                      size_t dstTexelSize);

    nsresult DOMElementToImageSurface(dom::Element* imageOrCanvas,
                                      gfxImageSurface **imageOut,
                                      WebGLTexelFormat *format);

    void CopyTexSubImage2D_base(WebGLenum target,
                                WebGLint level,
                                WebGLenum internalformat,
                                WebGLint xoffset,
                                WebGLint yoffset,
                                WebGLint x,
                                WebGLint y,
                                WebGLsizei width,
                                WebGLsizei height,
                                bool sub);

    
    template<class ObjectType>
    bool ValidateObject(const char* info, ObjectType *aObject);
    
    template<class ObjectType>
    bool ValidateObjectAllowNull(const char* info, ObjectType *aObject);
    
    
    template<class ObjectType>
    bool ValidateObjectAllowDeletedOrNull(const char* info, ObjectType *aObject);
    
    
    template<class ObjectType>
    bool ValidateObjectAllowDeleted(const char* info, ObjectType *aObject);
private:
    
    
    template<class ObjectType>
    bool ValidateObjectAssumeNonNull(const char* info, ObjectType *aObject);

protected:
    PRInt32 MaxTextureSizeForTarget(WebGLenum target) const {
        return target == LOCAL_GL_TEXTURE_2D ? mGLMaxTextureSize : mGLMaxCubeMapTextureSize;
    }
    
    

    GLenum CheckedBufferData(GLenum target,
                             GLsizeiptr size,
                             const GLvoid *data,
                             GLenum usage);
    

    GLenum CheckedTexImage2D(GLenum target,
                             GLint level,
                             GLenum internalFormat,
                             GLsizei width,
                             GLsizei height,
                             GLint border,
                             GLenum format,
                             GLenum type,
                             const GLvoid *data);

    void MaybeRestoreContext();
    bool IsContextStable() const {
        return mContextStatus == ContextStable;
    }
    void ForceLoseContext();
    void ForceRestoreContext();

    
    nsTArray<WebGLVertexAttribData> mAttribBuffers;

    nsTArray<WebGLRefPtr<WebGLTexture> > mBound2DTextures;
    nsTArray<WebGLRefPtr<WebGLTexture> > mBoundCubeMapTextures;

    WebGLRefPtr<WebGLBuffer> mBoundArrayBuffer;
    WebGLRefPtr<WebGLBuffer> mBoundElementArrayBuffer;

    WebGLRefPtr<WebGLProgram> mCurrentProgram;

    PRUint32 mMaxFramebufferColorAttachments;

    WebGLRefPtr<WebGLFramebuffer> mBoundFramebuffer;
    WebGLRefPtr<WebGLRenderbuffer> mBoundRenderbuffer;

    WebGLFastArray<WebGLTexture*> mTextures;
    WebGLFastArray<WebGLBuffer*> mBuffers;
    WebGLFastArray<WebGLProgram*> mPrograms;
    WebGLFastArray<WebGLShader*> mShaders;
    WebGLFastArray<WebGLRenderbuffer*> mRenderbuffers;
    WebGLFastArray<WebGLFramebuffer*> mFramebuffers;
    WebGLFastArray<WebGLUniformLocation*> mUniformLocations;

    
    PRUint32 mPixelStorePackAlignment, mPixelStoreUnpackAlignment, mPixelStoreColorspaceConversion;
    bool mPixelStoreFlipY, mPixelStorePremultiplyAlpha;

    FakeBlackStatus mFakeBlackStatus;

    WebGLuint mBlackTexture2D, mBlackTextureCubeMap;
    bool mBlackTexturesAreInitialized;

    WebGLfloat mVertexAttrib0Vector[4];
    WebGLfloat mFakeVertexAttrib0BufferObjectVector[4];
    size_t mFakeVertexAttrib0BufferObjectSize;
    GLuint mFakeVertexAttrib0BufferObject;
    int mFakeVertexAttrib0BufferStatus;

    WebGLint mStencilRefFront, mStencilRefBack;
    WebGLuint mStencilValueMaskFront, mStencilValueMaskBack,
              mStencilWriteMaskFront, mStencilWriteMaskBack;
    realGLboolean mColorWriteMask[4];
    realGLboolean mDepthWriteMask;
    realGLboolean mScissorTestEnabled;
    realGLboolean mDitherEnabled;
    WebGLfloat mColorClearValue[4];
    WebGLint mStencilClearValue;
    WebGLfloat mDepthClearValue;

    int mBackbufferClearingStatus;

    nsCOMPtr<nsITimer> mContextRestorer;
    bool mAllowRestore;
    bool mContextLossTimerRunning;
    bool mDrawSinceContextLossTimerSet;
    ContextStatus mContextStatus;
    bool mContextLostErrorSet;

#ifdef XP_MACOSX
    
    
    
    
    
    ForceDiscreteGPUHelperCGL mForceDiscreteGPUHelper;
#endif

    nsRefPtr<WebGLMemoryPressureObserver> mMemoryPressureObserver;

public:
    
    static void LogMessage(const char *fmt, ...);
    static void LogMessage(const char *fmt, va_list ap);
    void LogMessageIfVerbose(const char *fmt, ...);
    void LogMessageIfVerbose(const char *fmt, va_list ap);

    friend class WebGLTexture;
    friend class WebGLFramebuffer;
    friend class WebGLRenderbuffer;
    friend class WebGLProgram;
    friend class WebGLBuffer;
    friend class WebGLShader;
    friend class WebGLUniformLocation;
};




class WebGLContextBoundObject
{
public:
    WebGLContextBoundObject(WebGLContext *context) {
        mContext = context;
        mContextGeneration = context->Generation();
    }

    bool IsCompatibleWithContext(WebGLContext *other) {
        return mContext == other &&
            mContextGeneration == other->Generation();
    }

    WebGLContext *Context() const { return mContext; }

protected:
    WebGLContext *mContext;
    PRUint32 mContextGeneration;
};

struct WebGLVertexAttribData {
    
    WebGLVertexAttribData()
        : buf(0), stride(0), size(4), byteOffset(0),
          type(LOCAL_GL_FLOAT), enabled(false), normalized(false)
    { }

    WebGLRefPtr<WebGLBuffer> buf;
    WebGLuint stride;
    WebGLuint size;
    GLuint byteOffset;
    GLenum type;
    bool enabled;
    bool normalized;

    GLuint componentSize() const {
        switch(type) {
            case LOCAL_GL_BYTE:
                return sizeof(GLbyte);
                break;
            case LOCAL_GL_UNSIGNED_BYTE:
                return sizeof(GLubyte);
                break;
            case LOCAL_GL_SHORT:
                return sizeof(GLshort);
                break;
            case LOCAL_GL_UNSIGNED_SHORT:
                return sizeof(GLushort);
                break;
            
            case LOCAL_GL_FLOAT:
                return sizeof(GLfloat);
                break;
            default:
                NS_ERROR("Should never get here!");
                return 0;
        }
    }

    GLuint actualStride() const {
        if (stride) return stride;
        return size * componentSize();
    }
};



class WebGLBuffer MOZ_FINAL
    : public nsIWebGLBuffer
    , public WebGLRefCountedObject<WebGLBuffer>
    , public WebGLContextBoundObject
{
public:
    WebGLBuffer(WebGLContext *context)
        : WebGLContextBoundObject(context)
        , mHasEverBeenBound(false)
        , mByteLength(0)
        , mTarget(LOCAL_GL_NONE)
        , mData(nsnull)
    {
        mContext->MakeContextCurrent();
        mContext->gl->fGenBuffers(1, &mGLName);
        mMonotonicHandle = mContext->mBuffers.AppendElement(this);
    }

    ~WebGLBuffer() {
        DeleteOnce();
    }

    void Delete() {
        mContext->MakeContextCurrent();
        mContext->gl->fDeleteBuffers(1, &mGLName);
        free(mData);
        mData = nsnull;
        mByteLength = 0;
        mContext->mBuffers.RemoveElement(mMonotonicHandle);
    }

    size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
        return aMallocSizeOf(this) + aMallocSizeOf(mData);
    }
   
    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    GLuint GLName() const { return mGLName; }
    GLuint ByteLength() const { return mByteLength; }
    GLenum Target() const { return mTarget; }
    const void *Data() const { return mData; }

    void SetByteLength(GLuint byteLength) { mByteLength = byteLength; }
    void SetTarget(GLenum target) { mTarget = target; }

    
    
    bool CopyDataIfElementArray(const void* data) {
        if (mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
            mData = realloc(mData, mByteLength);
            if (!mData) {
                mByteLength = 0;
                return false;
            }
            memcpy(mData, data, mByteLength);
        }
        return true;
    }

    
    bool ZeroDataIfElementArray() {
        if (mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER) {
            mData = realloc(mData, mByteLength);
            if (!mData) {
                mByteLength = 0;
                return false;
            }
            memset(mData, 0, mByteLength);
        }
        return true;
    }

    
    void CopySubDataIfElementArray(GLuint byteOffset, GLuint byteLength, const void* data) {
        if (mTarget == LOCAL_GL_ELEMENT_ARRAY_BUFFER && mByteLength) {
            memcpy((void*) (size_t(mData)+byteOffset), data, byteLength);
        }
    }

    
    
    
    template<typename T>
    PRInt32 FindMaxElementInSubArray(GLuint count, GLuint byteOffset)
    {
        const T* start = reinterpret_cast<T*>(reinterpret_cast<size_t>(mData) + byteOffset);
        const T* stop = start + count;
        T result = 0;
        for(const T* ptr = start; ptr != stop; ++ptr) {
            if (*ptr > result) result = *ptr;
        }
        return result;
    }

    void InvalidateCachedMaxElements() {
      mHasCachedMaxUbyteElement = false;
      mHasCachedMaxUshortElement = false;
    }

    PRInt32 FindMaxUbyteElement() {
      if (mHasCachedMaxUbyteElement) {
        return mCachedMaxUbyteElement;
      } else {
        mHasCachedMaxUbyteElement = true;
        mCachedMaxUbyteElement = FindMaxElementInSubArray<GLubyte>(mByteLength, 0);
        return mCachedMaxUbyteElement;
      }
    }

    PRInt32 FindMaxUshortElement() {
      if (mHasCachedMaxUshortElement) {
        return mCachedMaxUshortElement;
      } else {
        mHasCachedMaxUshortElement = true;
        mCachedMaxUshortElement = FindMaxElementInSubArray<GLushort>(mByteLength>>1, 0);
        return mCachedMaxUshortElement;
      }
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLBUFFER

protected:

    WebGLuint mGLName;
    bool mHasEverBeenBound;
    GLuint mByteLength;
    GLenum mTarget;
    WebGLMonotonicHandle mMonotonicHandle;

    PRUint8 mCachedMaxUbyteElement;
    bool mHasCachedMaxUbyteElement;
    PRUint16 mCachedMaxUshortElement;
    bool mHasCachedMaxUshortElement;

    void* mData; 
};



class WebGLTexture MOZ_FINAL
    : public nsIWebGLTexture
    , public WebGLRefCountedObject<WebGLTexture>
    , public WebGLContextBoundObject
{
public:
    WebGLTexture(WebGLContext *context)
        : WebGLContextBoundObject(context)
        , mHasEverBeenBound(false)
        , mTarget(0)
        , mMinFilter(LOCAL_GL_NEAREST_MIPMAP_LINEAR)
        , mMagFilter(LOCAL_GL_LINEAR)
        , mWrapS(LOCAL_GL_REPEAT)
        , mWrapT(LOCAL_GL_REPEAT)
        , mFacesCount(0)
        , mMaxLevelWithCustomImages(0)
        , mHaveGeneratedMipmap(false)
        , mFakeBlackStatus(DoNotNeedFakeBlack)
    {
        mContext->MakeContextCurrent();
        mContext->gl->fGenTextures(1, &mGLName);
        mMonotonicHandle = mContext->mTextures.AppendElement(this);
    }

    ~WebGLTexture() {
        DeleteOnce();
    }

    void Delete() {
        mImageInfos.Clear();
        mContext->MakeContextCurrent();
        mContext->gl->fDeleteTextures(1, &mGLName);
        mContext->mTextures.RemoveElement(mMonotonicHandle);
    }

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    WebGLuint GLName() { return mGLName; }
    GLenum Target() const { return mTarget; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLTEXTURE

protected:

    friend class WebGLContext;
    friend class WebGLFramebuffer;

    bool mHasEverBeenBound;
    WebGLuint mGLName;

    
    

public:

    class ImageInfo : public WebGLRectangleObject {
    public:
        ImageInfo()
            : mFormat(0)
            , mType(0)
            , mIsDefined(false)
        {}

        ImageInfo(WebGLsizei width, WebGLsizei height,
                  WebGLenum format, WebGLenum type)
            : WebGLRectangleObject(width, height)
            , mFormat(format)
            , mType(type)
            , mIsDefined(true)
        {}

        bool operator==(const ImageInfo& a) const {
            return mIsDefined == a.mIsDefined &&
                   mWidth     == a.mWidth &&
                   mHeight    == a.mHeight &&
                   mFormat    == a.mFormat &&
                   mType      == a.mType;
        }
        bool operator!=(const ImageInfo& a) const {
            return !(*this == a);
        }
        bool IsSquare() const {
            return mWidth == mHeight;
        }
        bool IsPositive() const {
            return mWidth > 0 && mHeight > 0;
        }
        bool IsPowerOfTwo() const {
            return is_pot_assuming_nonnegative(mWidth) &&
                   is_pot_assuming_nonnegative(mHeight); 
        }
        PRInt64 MemoryUsage() const {
            if (!mIsDefined)
                return 0;
            PRInt64 texelSizeInBits = WebGLContext::GetBitsPerTexel(mFormat, mType);
            return PRInt64(mWidth) * PRInt64(mHeight) * texelSizeInBits / 8;
        }
        WebGLenum Format() const { return mFormat; }
        WebGLenum Type() const { return mType; }
    protected:
        WebGLenum mFormat, mType;
        bool mIsDefined;

        friend class WebGLTexture;
    };

    ImageInfo& ImageInfoAt(size_t level, size_t face = 0) {
#ifdef DEBUG
        if (face >= mFacesCount)
            NS_ERROR("wrong face index, must be 0 for TEXTURE_2D and at most 5 for cube maps");
#endif
        
        return mImageInfos.ElementAt(level * mFacesCount + face);
    }

    const ImageInfo& ImageInfoAt(size_t level, size_t face) const {
        return const_cast<WebGLTexture*>(this)->ImageInfoAt(level, face);
    }

    bool HasImageInfoAt(size_t level, size_t face) const {
        CheckedUint32 checked_index = CheckedUint32(level) * mFacesCount + face;
        return checked_index.isValid() &&
               checked_index.value() < mImageInfos.Length() &&
               ImageInfoAt(level, face).mIsDefined;
    }

    static size_t FaceForTarget(WebGLenum target) {
        return target == LOCAL_GL_TEXTURE_2D ? 0 : target - LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    }

    PRInt64 MemoryUsage() const {
        if (IsDeleted())
            return 0;
        PRInt64 result = 0;
        for(size_t face = 0; face < mFacesCount; face++) {
            if (mHaveGeneratedMipmap) {
                
                
                
                result += ImageInfoAt(0, face).MemoryUsage() * 4 / 3;
            } else {
                for(size_t level = 0; level <= mMaxLevelWithCustomImages; level++)
                    result += ImageInfoAt(level, face).MemoryUsage();
            }
        }
        return result;
    }

protected:

    WebGLenum mTarget;
    WebGLenum mMinFilter, mMagFilter, mWrapS, mWrapT;

    size_t mFacesCount, mMaxLevelWithCustomImages;
    nsTArray<ImageInfo> mImageInfos;

    bool mHaveGeneratedMipmap;
    FakeBlackStatus mFakeBlackStatus;

    WebGLMonotonicHandle mMonotonicHandle;

    void EnsureMaxLevelWithCustomImagesAtLeast(size_t aMaxLevelWithCustomImages) {
        mMaxLevelWithCustomImages = NS_MAX(mMaxLevelWithCustomImages, aMaxLevelWithCustomImages);
        mImageInfos.EnsureLengthAtLeast((mMaxLevelWithCustomImages + 1) * mFacesCount);
    }

    bool CheckFloatTextureFilterParams() const {
        
        return (mMagFilter == LOCAL_GL_NEAREST) &&
            (mMinFilter == LOCAL_GL_NEAREST || mMinFilter == LOCAL_GL_NEAREST_MIPMAP_NEAREST);
    }

    bool AreBothWrapModesClampToEdge() const {
        return mWrapS == LOCAL_GL_CLAMP_TO_EDGE && mWrapT == LOCAL_GL_CLAMP_TO_EDGE;
    }

    bool DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(size_t face) const {
        if (mHaveGeneratedMipmap)
            return true;

        ImageInfo expected = ImageInfoAt(0, face);

        
        
        for (size_t level = 0; level <= mMaxLevelWithCustomImages; ++level) {
            const ImageInfo& actual = ImageInfoAt(level, face);
            if (actual != expected)
                return false;
            expected.mWidth = NS_MAX(1, expected.mWidth >> 1);
            expected.mHeight = NS_MAX(1, expected.mHeight >> 1);

            
            
            if (actual.mWidth == 1 && actual.mHeight == 1)
                return true;
        }

        
        return false;
    }

public:

    void SetDontKnowIfNeedFakeBlack() {
        mFakeBlackStatus = DontKnowIfNeedFakeBlack;
        mContext->SetDontKnowIfNeedFakeBlack();
    }

    void Bind(WebGLenum aTarget) {
        
        

        bool firstTimeThisTextureIsBound = !mHasEverBeenBound;

        if (!firstTimeThisTextureIsBound && aTarget != mTarget) {
            mContext->ErrorInvalidOperation("bindTexture: this texture has already been bound to a different target");
            
            
            return;
        }

        mTarget = aTarget;

        mContext->gl->fBindTexture(mTarget, mGLName);

        if (firstTimeThisTextureIsBound) {
            mFacesCount = (mTarget == LOCAL_GL_TEXTURE_2D) ? 1 : 6;
            EnsureMaxLevelWithCustomImagesAtLeast(0);
            SetDontKnowIfNeedFakeBlack();

            
            
            
            if (mTarget == LOCAL_GL_TEXTURE_CUBE_MAP && !mContext->gl->IsGLES2())
                mContext->gl->fTexParameteri(mTarget, LOCAL_GL_TEXTURE_WRAP_R, LOCAL_GL_CLAMP_TO_EDGE);
        }

        mHasEverBeenBound = true;
    }

    void SetImageInfo(WebGLenum aTarget, WebGLint aLevel,
                      WebGLsizei aWidth, WebGLsizei aHeight,
                      WebGLenum aFormat, WebGLenum aType)
    {
        if ( (aTarget == LOCAL_GL_TEXTURE_2D) != (mTarget == LOCAL_GL_TEXTURE_2D) )
            return;

        size_t face = FaceForTarget(aTarget);

        EnsureMaxLevelWithCustomImagesAtLeast(aLevel);

        ImageInfoAt(aLevel, face) = ImageInfo(aWidth, aHeight, aFormat, aType);

        if (aLevel > 0)
            SetCustomMipmap();

        SetDontKnowIfNeedFakeBlack();
    }

    void SetMinFilter(WebGLenum aMinFilter) {
        mMinFilter = aMinFilter;
        SetDontKnowIfNeedFakeBlack();
    }
    void SetMagFilter(WebGLenum aMagFilter) {
        mMagFilter = aMagFilter;
        SetDontKnowIfNeedFakeBlack();
    }
    void SetWrapS(WebGLenum aWrapS) {
        mWrapS = aWrapS;
        SetDontKnowIfNeedFakeBlack();
    }
    void SetWrapT(WebGLenum aWrapT) {
        mWrapT = aWrapT;
        SetDontKnowIfNeedFakeBlack();
    }
    WebGLenum MinFilter() const { return mMinFilter; }

    bool DoesMinFilterRequireMipmap() const {
        return !(mMinFilter == LOCAL_GL_NEAREST || mMinFilter == LOCAL_GL_LINEAR);
    }

    void SetGeneratedMipmap() {
        if (!mHaveGeneratedMipmap) {
            mHaveGeneratedMipmap = true;
            SetDontKnowIfNeedFakeBlack();
        }
    }

    void SetCustomMipmap() {
        if (mHaveGeneratedMipmap) {
            
            

            
            
            ImageInfo imageInfo = ImageInfoAt(0, 0);
            NS_ASSERTION(imageInfo.IsPowerOfTwo(), "this texture is NPOT, so how could GenerateMipmap() ever accept it?");

            WebGLsizei size = NS_MAX(imageInfo.mWidth, imageInfo.mHeight);

            
            size_t maxLevel = 0;
            for (WebGLsizei n = size; n > 1; n >>= 1)
                ++maxLevel;

            EnsureMaxLevelWithCustomImagesAtLeast(maxLevel);

            for (size_t level = 1; level <= maxLevel; ++level) {
                
                imageInfo.mWidth >>= 1;
                imageInfo.mHeight >>= 1;
                for(size_t face = 0; face < mFacesCount; ++face)
                    ImageInfoAt(level, face) = imageInfo;
            }
        }
        mHaveGeneratedMipmap = false;
    }

    bool IsFirstImagePowerOfTwo() const {
        return ImageInfoAt(0, 0).IsPowerOfTwo();
    }

    bool AreAllLevel0ImageInfosEqual() const {
        for (size_t face = 1; face < mFacesCount; ++face) {
            if (ImageInfoAt(0, face) != ImageInfoAt(0, 0))
                return false;
        }
        return true;
    }

    bool IsMipmapTexture2DComplete() const {
        if (mTarget != LOCAL_GL_TEXTURE_2D)
            return false;
        if (!ImageInfoAt(0, 0).IsPositive())
            return false;
        if (mHaveGeneratedMipmap)
            return true;
        return DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(0);
    }

    bool IsCubeComplete() const {
        if (mTarget != LOCAL_GL_TEXTURE_CUBE_MAP)
            return false;
        const ImageInfo &first = ImageInfoAt(0, 0);
        if (!first.IsPositive() || !first.IsSquare())
            return false;
        return AreAllLevel0ImageInfosEqual();
    }

    bool IsMipmapCubeComplete() const {
        if (!IsCubeComplete()) 
            return false;
        for (size_t face = 0; face < mFacesCount; ++face) {
            if (!DoesTexture2DMipmapHaveAllLevelsConsistentlyDefined(face))
                return false;
        }
        return true;
    }

    bool NeedFakeBlack() {
        
        if (mFakeBlackStatus == DoNotNeedFakeBlack)
            return false;

        if (mFakeBlackStatus == DontKnowIfNeedFakeBlack) {
            
            

            for (size_t face = 0; face < mFacesCount; ++face) {
                if (!ImageInfoAt(0, face).mIsDefined) {
                    
                    
                    
                    
                    mFakeBlackStatus = DoNeedFakeBlack;
                    return true;
                }
            }

            const char *msg_rendering_as_black
                = "A texture is going to be rendered as if it were black, as per the OpenGL ES 2.0.24 spec section 3.8.2, "
                  "because it";

            if (mTarget == LOCAL_GL_TEXTURE_2D)
            {
                if (DoesMinFilterRequireMipmap())
                {
                    if (!IsMipmapTexture2DComplete()) {
                        mContext->LogMessageIfVerbose
                            ("%s is a 2D texture, with a minification filter requiring a mipmap, "
                             "and is not mipmap complete (as defined in section 3.7.10).", msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    } else if (!ImageInfoAt(0).IsPowerOfTwo()) {
                        mContext->LogMessageIfVerbose
                            ("%s is a 2D texture, with a minification filter requiring a mipmap, "
                             "and either its width or height is not a power of two.", msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    }
                }
                else 
                {
                    if (!ImageInfoAt(0).IsPositive()) {
                        mContext->LogMessageIfVerbose
                            ("%s is a 2D texture and its width or height is equal to zero.",
                             msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    } else if (!AreBothWrapModesClampToEdge() && !ImageInfoAt(0).IsPowerOfTwo()) {
                        mContext->LogMessageIfVerbose
                            ("%s is a 2D texture, with a minification filter not requiring a mipmap, "
                             "with its width or height not a power of two, and with a wrap mode "
                             "different from CLAMP_TO_EDGE.", msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    }
                }
            }
            else 
            {
                bool areAllLevel0ImagesPOT = true;
                for (size_t face = 0; face < mFacesCount; ++face)
                    areAllLevel0ImagesPOT &= ImageInfoAt(0, face).IsPowerOfTwo();

                if (DoesMinFilterRequireMipmap())
                {
                    if (!IsMipmapCubeComplete()) {
                        mContext->LogMessageIfVerbose("%s is a cube map texture, with a minification filter requiring a mipmap, "
                                   "and is not mipmap cube complete (as defined in section 3.7.10).",
                                   msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    } else if (!areAllLevel0ImagesPOT) {
                        mContext->LogMessageIfVerbose("%s is a cube map texture, with a minification filter requiring a mipmap, "
                                   "and either the width or the height of some level 0 image is not a power of two.",
                                   msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    }
                }
                else 
                {
                    if (!IsCubeComplete()) {
                        mContext->LogMessageIfVerbose("%s is a cube map texture, with a minification filter not requiring a mipmap, "
                                   "and is not cube complete (as defined in section 3.7.10).",
                                   msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    } else if (!AreBothWrapModesClampToEdge() && !areAllLevel0ImagesPOT) {
                        mContext->LogMessageIfVerbose("%s is a cube map texture, with a minification filter not requiring a mipmap, "
                                   "with some level 0 image having width or height not a power of two, and with a wrap mode "
                                   "different from CLAMP_TO_EDGE.", msg_rendering_as_black);
                        mFakeBlackStatus = DoNeedFakeBlack;
                    }
                }
            }

            
            
            if (mFakeBlackStatus == DontKnowIfNeedFakeBlack)
                mFakeBlackStatus = DoNotNeedFakeBlack;
        }

        return mFakeBlackStatus == DoNeedFakeBlack;
    }
};

struct WebGLMappedIdentifier {
    nsCString original, mapped; 
    WebGLMappedIdentifier(const nsACString& o, const nsACString& m) : original(o), mapped(m) {}
};

struct WebGLUniformInfo {
    PRUint32 arraySize;
    bool isArray;
    ShDataType type;

    WebGLUniformInfo(PRUint32 s = 0, bool a = false, ShDataType t = SH_NONE)
        : arraySize(s), isArray(a), type(t) {}

    int ElementSize() const {
        switch (type) {
            case SH_INT:
            case SH_FLOAT:
            case SH_BOOL:
            case SH_SAMPLER_2D:
            case SH_SAMPLER_CUBE:
                return 1;
            case SH_INT_VEC2:
            case SH_FLOAT_VEC2:
            case SH_BOOL_VEC2:
                return 2;
            case SH_INT_VEC3:
            case SH_FLOAT_VEC3:
            case SH_BOOL_VEC3:
                return 3;
            case SH_INT_VEC4:
            case SH_FLOAT_VEC4:
            case SH_BOOL_VEC4:
            case SH_FLOAT_MAT2:
                return 4;
            case SH_FLOAT_MAT3:
                return 9;
            case SH_FLOAT_MAT4:
                return 16;
            default:
                NS_ABORT(); 
                return 0;
        }
    }
};

class WebGLShader MOZ_FINAL
    : public nsIWebGLShader
    , public WebGLRefCountedObject<WebGLShader>
    , public WebGLContextBoundObject
{
    friend class WebGLContext;
    friend class WebGLProgram;

public:
    WebGLShader(WebGLContext *context, WebGLenum stype)
        : WebGLContextBoundObject(context)
        , mType(stype)
        , mNeedsTranslation(true)
        , mAttribMaxNameLength(0)
    {
        mContext->MakeContextCurrent();
        mGLName = mContext->gl->fCreateShader(mType);
        mMonotonicHandle = mContext->mShaders.AppendElement(this);
    }

    ~WebGLShader() {
        DeleteOnce();
    }
    
    size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) {
        return aMallocSizeOf(this) +
               mSource.SizeOfExcludingThisIfUnshared(aMallocSizeOf) +
               mTranslationLog.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
    }

    void Delete() {
        mSource.Truncate();
        mTranslationLog.Truncate();
        mContext->MakeContextCurrent();
        mContext->gl->fDeleteShader(mGLName);
        mContext->mShaders.RemoveElement(mMonotonicHandle);
    }

    WebGLuint GLName() { return mGLName; }
    WebGLenum ShaderType() { return mType; }

    void SetSource(const nsAString& src) {
        
        mSource.Assign(src);
    }

    const nsString& Source() const { return mSource; }

    void SetNeedsTranslation() { mNeedsTranslation = true; }
    bool NeedsTranslation() const { return mNeedsTranslation; }

    void SetTranslationSuccess() {
        mTranslationLog.SetIsVoid(true);
        mNeedsTranslation = false;
    }

    void SetTranslationFailure(const nsCString& msg) {
        mTranslationLog.Assign(msg); 
    }

    const nsCString& TranslationLog() const { return mTranslationLog; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLSHADER

protected:

    WebGLuint mGLName;
    WebGLenum mType;
    nsString mSource;
    nsCString mTranslationLog; 
    bool mNeedsTranslation;
    WebGLMonotonicHandle mMonotonicHandle;
    nsTArray<WebGLMappedIdentifier> mAttributes;
    nsTArray<WebGLMappedIdentifier> mUniforms;
    nsTArray<WebGLUniformInfo> mUniformInfos;
    int mAttribMaxNameLength;
};








static bool SplitLastSquareBracket(nsACString& string, nsCString& bracketPart)
{
    NS_ABORT_IF_FALSE(bracketPart.Length() == 0, "SplitLastSquareBracket must be called with empty bracketPart string");
    char *string_start = string.BeginWriting();
    char *s = string_start + string.Length() - 1;

    if (*s != ']')
        return false;

    while (*s != '[' && s != string_start)
        s--;

    if (*s != '[')
        return false;

    bracketPart.Assign(s);
    *s = 0;
    string.EndWriting();
    string.SetLength(s - string_start);
    return true;
}

typedef nsDataHashtable<nsCStringHashKey, nsCString> CStringMap;
typedef nsDataHashtable<nsCStringHashKey, WebGLUniformInfo> CStringToUniformInfoMap;



class WebGLProgram MOZ_FINAL
    : public nsIWebGLProgram
    , public WebGLRefCountedObject<WebGLProgram>
    , public WebGLContextBoundObject
{
public:
    WebGLProgram(WebGLContext *context)
        : WebGLContextBoundObject(context)
        , mLinkStatus(false)
        , mGeneration(0)
        , mAttribMaxNameLength(0)
    {
        mContext->MakeContextCurrent();
        mGLName = mContext->gl->fCreateProgram();
        mMonotonicHandle = mContext->mPrograms.AppendElement(this);
    }

    ~WebGLProgram() {
        DeleteOnce();
    }

    void Delete() {
        DetachShaders();
        mContext->MakeContextCurrent();
        mContext->gl->fDeleteProgram(mGLName);
        mContext->mPrograms.RemoveElement(mMonotonicHandle);
    }

    void DetachShaders() {
        mAttachedShaders.Clear();
    }

    WebGLuint GLName() { return mGLName; }
    const nsTArray<WebGLRefPtr<WebGLShader> >& AttachedShaders() const { return mAttachedShaders; }
    bool LinkStatus() { return mLinkStatus; }
    PRUint32 Generation() const { return mGeneration.value(); }
    void SetLinkStatus(bool val) { mLinkStatus = val; }

    bool ContainsShader(WebGLShader *shader) {
        return mAttachedShaders.Contains(shader);
    }

    
    bool AttachShader(WebGLShader *shader) {
        if (ContainsShader(shader))
            return false;
        mAttachedShaders.AppendElement(shader);

        mContext->MakeContextCurrent();
        mContext->gl->fAttachShader(GLName(), shader->GLName());

        return true;
    }

    
    bool DetachShader(WebGLShader *shader) {
        if (!mAttachedShaders.RemoveElement(shader))
            return false;

        mContext->MakeContextCurrent();
        mContext->gl->fDetachShader(GLName(), shader->GLName());

        return true;
    }

    bool HasAttachedShaderOfType(GLenum shaderType) {
        for (PRUint32 i = 0; i < mAttachedShaders.Length(); ++i) {
            if (mAttachedShaders[i] && mAttachedShaders[i]->ShaderType() == shaderType) {
                return true;
            }
        }
        return false;
    }

    bool HasBothShaderTypesAttached() {
        return
            HasAttachedShaderOfType(LOCAL_GL_VERTEX_SHADER) &&
            HasAttachedShaderOfType(LOCAL_GL_FRAGMENT_SHADER);
    }

    bool NextGeneration()
    {
        if (!(mGeneration + 1).isValid())
            return false; 
        ++mGeneration;
        return true;
    }

    
    bool UpdateInfo();

    
    bool IsAttribInUse(unsigned i) const { return mAttribsInUse[i]; }

    


    void MapIdentifier(const nsACString& name, nsCString *mappedName) {
        if (!mIdentifierMap) {
            
            mIdentifierMap = new CStringMap;
            mIdentifierMap->Init();
            for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
                for (size_t j = 0; j < mAttachedShaders[i]->mAttributes.Length(); j++) {
                    const WebGLMappedIdentifier& attrib = mAttachedShaders[i]->mAttributes[j];
                    mIdentifierMap->Put(attrib.original, attrib.mapped);
                }
                for (size_t j = 0; j < mAttachedShaders[i]->mUniforms.Length(); j++) {
                    const WebGLMappedIdentifier& uniform = mAttachedShaders[i]->mUniforms[j];
                    mIdentifierMap->Put(uniform.original, uniform.mapped);
                }
            }
        }

        nsCString mutableName(name);
        nsCString bracketPart;
        bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
        if (hadBracketPart)
            mutableName.AppendLiteral("[0]");

        if (mIdentifierMap->Get(mutableName, mappedName)) {
            if (hadBracketPart) {
                nsCString mappedBracketPart;
                bool mappedHadBracketPart = SplitLastSquareBracket(*mappedName, mappedBracketPart);
                if (mappedHadBracketPart)
                    mappedName->Append(bracketPart);
            }
            return;
        }

        
        
        mutableName.AppendLiteral("[0]");
        if (mIdentifierMap->Get(mutableName, mappedName))
            return;

        
        
        
        mappedName->Assign(name);
    }

    


    void ReverseMapIdentifier(const nsACString& name, nsCString *reverseMappedName) {
        if (!mIdentifierReverseMap) {
            
            mIdentifierReverseMap = new CStringMap;
            mIdentifierReverseMap->Init();
            for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
                for (size_t j = 0; j < mAttachedShaders[i]->mAttributes.Length(); j++) {
                    const WebGLMappedIdentifier& attrib = mAttachedShaders[i]->mAttributes[j];
                    mIdentifierReverseMap->Put(attrib.mapped, attrib.original);
                }
                for (size_t j = 0; j < mAttachedShaders[i]->mUniforms.Length(); j++) {
                    const WebGLMappedIdentifier& uniform = mAttachedShaders[i]->mUniforms[j];
                    mIdentifierReverseMap->Put(uniform.mapped, uniform.original);
                }
            }
        }

        nsCString mutableName(name);
        nsCString bracketPart;
        bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
        if (hadBracketPart)
            mutableName.AppendLiteral("[0]");

        if (mIdentifierReverseMap->Get(mutableName, reverseMappedName)) {
            if (hadBracketPart) {
                nsCString reverseMappedBracketPart;
                bool reverseMappedHadBracketPart = SplitLastSquareBracket(*reverseMappedName, reverseMappedBracketPart);
                if (reverseMappedHadBracketPart)
                    reverseMappedName->Append(bracketPart);
            }
            return;
        }

        
        
        mutableName.AppendLiteral("[0]");
        if (mIdentifierReverseMap->Get(mutableName, reverseMappedName))
            return;

        
        
        
        reverseMappedName->Assign(name);
    }

    




    WebGLUniformInfo GetUniformInfoForMappedIdentifier(const nsACString& name) {
        if (!mUniformInfoMap) {
            
            mUniformInfoMap = new CStringToUniformInfoMap;
            mUniformInfoMap->Init();
            for (size_t i = 0; i < mAttachedShaders.Length(); i++) {
                for (size_t j = 0; j < mAttachedShaders[i]->mUniforms.Length(); j++) {
                    const WebGLMappedIdentifier& uniform = mAttachedShaders[i]->mUniforms[j];
                    const WebGLUniformInfo& info = mAttachedShaders[i]->mUniformInfos[j];
                    mUniformInfoMap->Put(uniform.mapped, info);
                }
            }
        }

        nsCString mutableName(name);
        nsCString bracketPart;
        bool hadBracketPart = SplitLastSquareBracket(mutableName, bracketPart);
        
        if (hadBracketPart)
            mutableName.AppendLiteral("[0]");

        WebGLUniformInfo info;
        mUniformInfoMap->Get(mutableName, &info);
        

        
        if (hadBracketPart && !bracketPart.EqualsLiteral("[0]")) {
            info.isArray = false;
            info.arraySize = 1;
        }
        return info;
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLPROGRAM

protected:

    WebGLuint mGLName;
    bool mLinkStatus;
    
    nsTArray<WebGLRefPtr<WebGLShader> > mAttachedShaders;
    CheckedUint32 mGeneration;

    
    std::vector<bool> mAttribsInUse;
    WebGLMonotonicHandle mMonotonicHandle;
    nsAutoPtr<CStringMap> mIdentifierMap, mIdentifierReverseMap;
    nsAutoPtr<CStringToUniformInfoMap> mUniformInfoMap;
    int mAttribMaxNameLength;
};



class WebGLRenderbuffer MOZ_FINAL
    : public nsIWebGLRenderbuffer
    , public WebGLRefCountedObject<WebGLRenderbuffer>
    , public WebGLRectangleObject
    , public WebGLContextBoundObject
{
public:
    WebGLRenderbuffer(WebGLContext *context)
        : WebGLContextBoundObject(context)
        , mInternalFormat(0)
        , mInternalFormatForGL(0)
        , mHasEverBeenBound(false)
        , mInitialized(false)
    {

        mContext->MakeContextCurrent();
        mContext->gl->fGenRenderbuffers(1, &mGLName);
        mMonotonicHandle = mContext->mRenderbuffers.AppendElement(this);
    }

    ~WebGLRenderbuffer() {
        DeleteOnce();
    }

    void Delete() {
        mContext->MakeContextCurrent();
        mContext->gl->fDeleteRenderbuffers(1, &mGLName);
        mContext->mRenderbuffers.RemoveElement(mMonotonicHandle);
    }

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    WebGLuint GLName() const { return mGLName; }

    bool Initialized() const { return mInitialized; }
    void SetInitialized(bool aInitialized) { mInitialized = aInitialized; }

    WebGLenum InternalFormat() const { return mInternalFormat; }
    void SetInternalFormat(WebGLenum aInternalFormat) { mInternalFormat = aInternalFormat; }
    
    WebGLenum InternalFormatForGL() const { return mInternalFormatForGL; }
    void SetInternalFormatForGL(WebGLenum aInternalFormatForGL) { mInternalFormatForGL = aInternalFormatForGL; }
    
    PRInt64 MemoryUsage() const {
        PRInt64 pixels = PRInt64(Width()) * PRInt64(Height());
        switch (mInternalFormatForGL) {
            case LOCAL_GL_STENCIL_INDEX8:
                return pixels;
            case LOCAL_GL_RGBA4:
            case LOCAL_GL_RGB5_A1:
            case LOCAL_GL_RGB565:
            case LOCAL_GL_DEPTH_COMPONENT16:
                return 2 * pixels;
            case LOCAL_GL_RGB8:
            case LOCAL_GL_DEPTH_COMPONENT24:
                return 3*pixels;
            case LOCAL_GL_RGBA8:
            case LOCAL_GL_DEPTH24_STENCIL8:
                return 4*pixels;
            default:
                break;
        }
        NS_ABORT();
        return 0;
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLRENDERBUFFER

protected:

    WebGLuint mGLName;
    WebGLenum mInternalFormat;
    WebGLenum mInternalFormatForGL;
    WebGLMonotonicHandle mMonotonicHandle;
    bool mHasEverBeenBound;
    bool mInitialized;

    friend class WebGLFramebuffer;
};

class WebGLFramebufferAttachment
{
    
    WebGLRefPtr<WebGLTexture> mTexturePtr;
    WebGLRefPtr<WebGLRenderbuffer> mRenderbufferPtr;
    WebGLenum mAttachmentPoint;
    WebGLint mTextureLevel;
    WebGLenum mTextureCubeMapFace;

public:
    WebGLFramebufferAttachment(WebGLenum aAttachmentPoint)
        : mAttachmentPoint(aAttachmentPoint)
    {}

    bool IsDefined() const {
        return Texture() || Renderbuffer();
    }

    bool IsDeleteRequested() const {
        return Texture() ? Texture()->IsDeleteRequested()
             : Renderbuffer() ? Renderbuffer()->IsDeleteRequested()
             : false;
    }

    bool HasAlpha() const {
        WebGLenum format = 0;
        if (Texture() && Texture()->HasImageInfoAt(mTextureLevel, mTextureCubeMapFace))
            format = Texture()->ImageInfoAt(mTextureLevel, mTextureCubeMapFace).Format();
        else if (Renderbuffer())
            format = Renderbuffer()->InternalFormat();
        return format == LOCAL_GL_RGBA ||
               format == LOCAL_GL_LUMINANCE_ALPHA ||
               format == LOCAL_GL_ALPHA ||
               format == LOCAL_GL_RGBA4 ||
               format == LOCAL_GL_RGB5_A1;
    }

    void SetTexture(WebGLTexture *tex, WebGLint level, WebGLenum face) {
        mTexturePtr = tex;
        mRenderbufferPtr = nsnull;
        mTextureLevel = level;
        mTextureCubeMapFace = face;
    }
    void SetRenderbuffer(WebGLRenderbuffer *rb) {
        mTexturePtr = nsnull;
        mRenderbufferPtr = rb;
    }
    const WebGLTexture *Texture() const {
        return mTexturePtr;
    }
    WebGLTexture *Texture() {
        return mTexturePtr;
    }
    const WebGLRenderbuffer *Renderbuffer() const {
        return mRenderbufferPtr;
    }
    WebGLRenderbuffer *Renderbuffer() {
        return mRenderbufferPtr;
    }
    WebGLint TextureLevel() const {
        return mTextureLevel;
    }
    WebGLenum TextureCubeMapFace() const {
        return mTextureCubeMapFace;
    }

    bool HasUninitializedRenderbuffer() const {
        return mRenderbufferPtr && !mRenderbufferPtr->Initialized();
    }

    void Reset() {
        mTexturePtr = nsnull;
        mRenderbufferPtr = nsnull;
    }

    const WebGLRectangleObject* RectangleObject() const {
        if (Texture() && Texture()->HasImageInfoAt(mTextureLevel, mTextureCubeMapFace))
            return &Texture()->ImageInfoAt(mTextureLevel, mTextureCubeMapFace);
        else if (Renderbuffer())
            return Renderbuffer();
        else
            return nsnull;
    }
    bool HasSameDimensionsAs(const WebGLFramebufferAttachment& other) const {
        const WebGLRectangleObject *thisRect = RectangleObject();
        const WebGLRectangleObject *otherRect = other.RectangleObject();
        return thisRect &&
               otherRect &&
               thisRect->HasSameDimensionsAs(*otherRect);
    }

    bool IsComplete() const {
        const WebGLRectangleObject *thisRect = RectangleObject();

        if (!thisRect ||
            !thisRect->Width() ||
            !thisRect->Height())
            return false;

        if (mTexturePtr)
            return mAttachmentPoint == LOCAL_GL_COLOR_ATTACHMENT0;

        if (mRenderbufferPtr) {
            WebGLenum format = mRenderbufferPtr->InternalFormat();
            switch (mAttachmentPoint) {
                case LOCAL_GL_COLOR_ATTACHMENT0:
                    return format == LOCAL_GL_RGB565 ||
                           format == LOCAL_GL_RGB5_A1 ||
                           format == LOCAL_GL_RGBA4;
                case LOCAL_GL_DEPTH_ATTACHMENT:
                    return format == LOCAL_GL_DEPTH_COMPONENT16;
                case LOCAL_GL_STENCIL_ATTACHMENT:
                    return format == LOCAL_GL_STENCIL_INDEX8;
                case LOCAL_GL_DEPTH_STENCIL_ATTACHMENT:
                    return format == LOCAL_GL_DEPTH_STENCIL;
                default:
                    NS_ABORT(); 
            }
        }

        NS_ABORT(); 
        return false;
    }
};



class WebGLFramebuffer MOZ_FINAL
    : public nsIWebGLFramebuffer
    , public WebGLRefCountedObject<WebGLFramebuffer>
    , public WebGLContextBoundObject
{
public:
    WebGLFramebuffer(WebGLContext *context)
        : WebGLContextBoundObject(context)
        , mHasEverBeenBound(false)
        , mColorAttachment(LOCAL_GL_COLOR_ATTACHMENT0)
        , mDepthAttachment(LOCAL_GL_DEPTH_ATTACHMENT)
        , mStencilAttachment(LOCAL_GL_STENCIL_ATTACHMENT)
        , mDepthStencilAttachment(LOCAL_GL_DEPTH_STENCIL_ATTACHMENT)
    {
        mContext->MakeContextCurrent();
        mContext->gl->fGenFramebuffers(1, &mGLName);
        mMonotonicHandle = mContext->mFramebuffers.AppendElement(this);
    }

    ~WebGLFramebuffer() {
        DeleteOnce();
    }

    void Delete() {
        mColorAttachment.Reset();
        mDepthAttachment.Reset();
        mStencilAttachment.Reset();
        mDepthStencilAttachment.Reset();
        mContext->MakeContextCurrent();
        mContext->gl->fDeleteFramebuffers(1, &mGLName);
        mContext->mFramebuffers.RemoveElement(mMonotonicHandle);
    }

    bool HasEverBeenBound() { return mHasEverBeenBound; }
    void SetHasEverBeenBound(bool x) { mHasEverBeenBound = x; }
    WebGLuint GLName() { return mGLName; }

    void FramebufferRenderbuffer(WebGLenum target,
                                 WebGLenum attachment,
                                 WebGLenum rbtarget,
                                 WebGLRenderbuffer *wrb)
    {
        if (!mContext->ValidateObjectAllowNull("framebufferRenderbuffer: renderbuffer", wrb))
        {
            return;
        }

        if (target != LOCAL_GL_FRAMEBUFFER)
            return mContext->ErrorInvalidEnumInfo("framebufferRenderbuffer: target", target);

        if (rbtarget != LOCAL_GL_RENDERBUFFER)
            return mContext->ErrorInvalidEnumInfo("framebufferRenderbuffer: renderbuffer target:", rbtarget);

        switch (attachment) {
        case LOCAL_GL_DEPTH_ATTACHMENT:
            mDepthAttachment.SetRenderbuffer(wrb);
            break;
        case LOCAL_GL_STENCIL_ATTACHMENT:
            mStencilAttachment.SetRenderbuffer(wrb);
            break;
        case LOCAL_GL_DEPTH_STENCIL_ATTACHMENT:
            mDepthStencilAttachment.SetRenderbuffer(wrb);
            break;
        default:
            
            if (attachment != LOCAL_GL_COLOR_ATTACHMENT0)
                return mContext->ErrorInvalidEnumInfo("framebufferRenderbuffer: attachment", attachment);

            mColorAttachment.SetRenderbuffer(wrb);
            break;
        }

        mContext->MakeContextCurrent();
        WebGLuint renderbuffername = wrb ? wrb->GLName() : 0;
        if (attachment == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
            mContext->gl->fFramebufferRenderbuffer(target, LOCAL_GL_DEPTH_ATTACHMENT, rbtarget, renderbuffername);
            mContext->gl->fFramebufferRenderbuffer(target, LOCAL_GL_STENCIL_ATTACHMENT, rbtarget, renderbuffername);
        } else {
            mContext->gl->fFramebufferRenderbuffer(target, attachment, rbtarget, renderbuffername);
        }
    }

    void FramebufferTexture2D(WebGLenum target,
                              WebGLenum attachment,
                              WebGLenum textarget,
                              WebGLTexture *wtex,
                              WebGLint level)
    {
        if (!mContext->ValidateObjectAllowNull("framebufferTexture2D: texture",
                                               wtex))
        {
            return;
        }

        if (target != LOCAL_GL_FRAMEBUFFER)
            return mContext->ErrorInvalidEnumInfo("framebufferTexture2D: target", target);

        if (textarget != LOCAL_GL_TEXTURE_2D &&
            (textarget < LOCAL_GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
             textarget > LOCAL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
            return mContext->ErrorInvalidEnumInfo("framebufferTexture2D: invalid texture target", textarget);

        if (level != 0)
            return mContext->ErrorInvalidValue("framebufferTexture2D: level must be 0");

        size_t face = WebGLTexture::FaceForTarget(textarget);
        switch (attachment) {
        case LOCAL_GL_DEPTH_ATTACHMENT:
            mDepthAttachment.SetTexture(wtex, level, face);
            break;
        case LOCAL_GL_STENCIL_ATTACHMENT:
            mStencilAttachment.SetTexture(wtex, level, face);
            break;
        case LOCAL_GL_DEPTH_STENCIL_ATTACHMENT:
            mDepthStencilAttachment.SetTexture(wtex, level, face);
            break;
        default:
            if (attachment != LOCAL_GL_COLOR_ATTACHMENT0)
                return mContext->ErrorInvalidEnumInfo("framebufferTexture2D: attachment", attachment);

            mColorAttachment.SetTexture(wtex, level, face);
            break;
        }

        mContext->MakeContextCurrent();
        WebGLuint texturename = wtex ? wtex->GLName() : 0;
        if (attachment == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT) {
            mContext->gl->fFramebufferTexture2D(target, LOCAL_GL_DEPTH_ATTACHMENT, textarget, texturename, level);
            mContext->gl->fFramebufferTexture2D(target, LOCAL_GL_STENCIL_ATTACHMENT, textarget, texturename, level);
        } else {
            mContext->gl->fFramebufferTexture2D(target, attachment, textarget, texturename, level);
        }

        return;
    }

    bool HasIncompleteAttachment() const {
        return (mColorAttachment.IsDefined() && !mColorAttachment.IsComplete()) ||
               (mDepthAttachment.IsDefined() && !mDepthAttachment.IsComplete()) ||
               (mStencilAttachment.IsDefined() && !mStencilAttachment.IsComplete()) ||
               (mDepthStencilAttachment.IsDefined() && !mDepthStencilAttachment.IsComplete());
    }

    bool HasDepthStencilConflict() const {
        return int(mDepthAttachment.IsDefined()) +
               int(mStencilAttachment.IsDefined()) +
               int(mDepthStencilAttachment.IsDefined()) >= 2;
    }

    bool HasAttachmentsOfMismatchedDimensions() const {
        return (mDepthAttachment.IsDefined() && !mDepthAttachment.HasSameDimensionsAs(mColorAttachment)) ||
               (mStencilAttachment.IsDefined() && !mStencilAttachment.HasSameDimensionsAs(mColorAttachment)) ||
               (mDepthStencilAttachment.IsDefined() && !mDepthStencilAttachment.HasSameDimensionsAs(mColorAttachment));
    }

    const WebGLFramebufferAttachment& ColorAttachment() const {
        return mColorAttachment;
    }

    const WebGLFramebufferAttachment& DepthAttachment() const {
        return mDepthAttachment;
    }

    const WebGLFramebufferAttachment& StencilAttachment() const {
        return mStencilAttachment;
    }

    const WebGLFramebufferAttachment& DepthStencilAttachment() const {
        return mDepthStencilAttachment;
    }

    const WebGLFramebufferAttachment& GetAttachment(WebGLenum attachment) const {
        if (attachment == LOCAL_GL_DEPTH_STENCIL_ATTACHMENT)
            return mDepthStencilAttachment;
        if (attachment == LOCAL_GL_DEPTH_ATTACHMENT)
            return mDepthAttachment;
        if (attachment == LOCAL_GL_STENCIL_ATTACHMENT)
            return mStencilAttachment;

        NS_ASSERTION(attachment == LOCAL_GL_COLOR_ATTACHMENT0, "bad attachment!");
        return mColorAttachment;
    }

    void DetachTexture(const WebGLTexture *tex) {
        if (mColorAttachment.Texture() == tex)
            FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_COLOR_ATTACHMENT0, LOCAL_GL_TEXTURE_2D, nsnull, 0);
        if (mDepthAttachment.Texture() == tex)
            FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT, LOCAL_GL_TEXTURE_2D, nsnull, 0);
        if (mStencilAttachment.Texture() == tex)
            FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_STENCIL_ATTACHMENT, LOCAL_GL_TEXTURE_2D, nsnull, 0);
        if (mDepthStencilAttachment.Texture() == tex)
            FramebufferTexture2D(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_STENCIL_ATTACHMENT, LOCAL_GL_TEXTURE_2D, nsnull, 0);
    }

    void DetachRenderbuffer(const WebGLRenderbuffer *rb) {
        if (mColorAttachment.Renderbuffer() == rb)
            FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_COLOR_ATTACHMENT0, LOCAL_GL_RENDERBUFFER, nsnull);
        if (mDepthAttachment.Renderbuffer() == rb)
            FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_ATTACHMENT, LOCAL_GL_RENDERBUFFER, nsnull);
        if (mStencilAttachment.Renderbuffer() == rb)
            FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_STENCIL_ATTACHMENT, LOCAL_GL_RENDERBUFFER, nsnull);
        if (mDepthStencilAttachment.Renderbuffer() == rb)
            FramebufferRenderbuffer(LOCAL_GL_FRAMEBUFFER, LOCAL_GL_DEPTH_STENCIL_ATTACHMENT, LOCAL_GL_RENDERBUFFER, nsnull);
    }

    const WebGLRectangleObject *RectangleObject() {
        return mColorAttachment.RectangleObject();
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLFRAMEBUFFER

    bool CheckAndInitializeRenderbuffers()
    {
        
        
        if (HasDepthStencilConflict())
            return false;

        if (!mColorAttachment.HasUninitializedRenderbuffer() &&
            !mDepthAttachment.HasUninitializedRenderbuffer() &&
            !mStencilAttachment.HasUninitializedRenderbuffer() &&
            !mDepthStencilAttachment.HasUninitializedRenderbuffer())
            return true;

        
        const WebGLRectangleObject *rect = mColorAttachment.RectangleObject();
        if (!rect ||
            !rect->Width() ||
            !rect->Height())
            return false;

        mContext->MakeContextCurrent();

        WebGLenum status;
        mContext->CheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER, &status);
        if (status != LOCAL_GL_FRAMEBUFFER_COMPLETE)
            return false;

        PRUint32 mask = 0;

        if (mColorAttachment.HasUninitializedRenderbuffer())
            mask |= LOCAL_GL_COLOR_BUFFER_BIT;

        if (mDepthAttachment.HasUninitializedRenderbuffer() ||
            mDepthStencilAttachment.HasUninitializedRenderbuffer())
        {
            mask |= LOCAL_GL_DEPTH_BUFFER_BIT;
        }

        if (mStencilAttachment.HasUninitializedRenderbuffer() ||
            mDepthStencilAttachment.HasUninitializedRenderbuffer())
        {
            mask |= LOCAL_GL_STENCIL_BUFFER_BIT;
        }

        mContext->ForceClearFramebufferWithDefaultValues(mask, nsIntRect(0, 0, rect->Width(), rect->Height()));

        if (mColorAttachment.HasUninitializedRenderbuffer())
            mColorAttachment.Renderbuffer()->SetInitialized(true);

        if (mDepthAttachment.HasUninitializedRenderbuffer())
            mDepthAttachment.Renderbuffer()->SetInitialized(true);

        if (mStencilAttachment.HasUninitializedRenderbuffer())
            mStencilAttachment.Renderbuffer()->SetInitialized(true);

        if (mDepthStencilAttachment.HasUninitializedRenderbuffer())
            mDepthStencilAttachment.Renderbuffer()->SetInitialized(true);

        return true;
    }

    WebGLuint mGLName;
    bool mHasEverBeenBound;

    
    
    WebGLFramebufferAttachment mColorAttachment,
                               mDepthAttachment,
                               mStencilAttachment,
                               mDepthStencilAttachment;

    WebGLMonotonicHandle mMonotonicHandle;
};

class WebGLUniformLocation MOZ_FINAL
    : public nsIWebGLUniformLocation
    , public WebGLContextBoundObject
    , public WebGLRefCountedObject<WebGLUniformLocation>
{
public:
    WebGLUniformLocation(WebGLContext *context, WebGLProgram *program, GLint location, const WebGLUniformInfo& info)
        : WebGLContextBoundObject(context)
        , mProgram(program)
        , mProgramGeneration(program->Generation())
        , mLocation(location)
        , mInfo(info)
    {
        mElementSize = info.ElementSize();
        mMonotonicHandle = mContext->mUniformLocations.AppendElement(this);
    }

    ~WebGLUniformLocation() {
        DeleteOnce();
    }

    void Delete() {
        mProgram = nsnull;
        mContext->mUniformLocations.RemoveElement(mMonotonicHandle);
    }

    const WebGLUniformInfo &Info() const { return mInfo; }

    WebGLProgram *Program() const { return mProgram; }
    GLint Location() const { return mLocation; }
    PRUint32 ProgramGeneration() const { return mProgramGeneration; }
    int ElementSize() const { return mElementSize; }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLUNIFORMLOCATION
protected:
    
    
    nsRefPtr<WebGLProgram> mProgram;

    PRUint32 mProgramGeneration;
    GLint mLocation;
    WebGLUniformInfo mInfo;
    int mElementSize;
    WebGLMonotonicHandle mMonotonicHandle;
    friend class WebGLProgram;
};

class WebGLActiveInfo MOZ_FINAL
    : public nsIWebGLActiveInfo
{
public:
    WebGLActiveInfo(WebGLint size, WebGLenum type, const nsACString& name) :
        mSize(size),
        mType(type),
        mName(NS_ConvertASCIItoUTF16(name))
    {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLACTIVEINFO
protected:
    WebGLint mSize;
    WebGLenum mType;
    nsString mName;
};

class WebGLShaderPrecisionFormat MOZ_FINAL
    : public nsIWebGLShaderPrecisionFormat
{
public:
    WebGLShaderPrecisionFormat(WebGLint rangeMin, WebGLint rangeMax, WebGLint precision) :
        mRangeMin(rangeMin),
        mRangeMax(rangeMax),
        mPrecision(precision)
    {
    
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLSHADERPRECISIONFORMAT

protected:
    WebGLint mRangeMin;
    WebGLint mRangeMax;
    WebGLint mPrecision;
};

class WebGLExtension
    : public nsIWebGLExtension
    , public WebGLContextBoundObject
    , public nsWrapperCache
{
public:
    WebGLExtension(WebGLContext *baseContext)
        : WebGLContextBoundObject(baseContext)
    {}

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(WebGLExtension)
    NS_DECL_NSIWEBGLEXTENSION

    virtual ~WebGLExtension() {}
};

inline const WebGLRectangleObject *WebGLContext::FramebufferRectangleObject() const {
    return mBoundFramebuffer ? mBoundFramebuffer->RectangleObject()
                             : static_cast<const WebGLRectangleObject*>(this);
}





template<class ObjectType>
inline bool
WebGLContext::ValidateObjectAllowDeletedOrNull(const char* info,
                                               ObjectType *aObject)
{
    if (aObject && !aObject->IsCompatibleWithContext(this)) {
        ErrorInvalidOperation("%s: object from different WebGL context "
                              "(or older generation of this one) "
                              "passed as argument", info);
        return false;
    }

    return true;
}

template<class ObjectType>
inline bool
WebGLContext::ValidateObjectAssumeNonNull(const char* info, ObjectType *aObject)
{
    MOZ_ASSERT(aObject);

    if (!ValidateObjectAllowDeletedOrNull(info, aObject))
        return false;

    if (aObject->IsDeleted()) {
        ErrorInvalidValue("%s: deleted object passed as argument", info);
        return false;
    }

    return true;
}

template<class ObjectType>
inline bool
WebGLContext::ValidateObjectAllowNull(const char* info, ObjectType *aObject)
{
    if (!aObject) {
        return true;
    }

    return ValidateObjectAssumeNonNull(info, aObject);
}

template<class ObjectType>
inline bool
WebGLContext::ValidateObjectAllowDeleted(const char* info, ObjectType *aObject)
{
    if (!aObject) {
        ErrorInvalidValue("%s: null object passed as argument", info);
        return false;
    }

    return ValidateObjectAllowDeletedOrNull(info, aObject);
}

template<class ObjectType>
inline bool
WebGLContext::ValidateObject(const char* info, ObjectType *aObject)
{
    if (!aObject) {
        ErrorInvalidValue("%s: null object passed as argument", info);
        return false;
    }

    return ValidateObjectAssumeNonNull(info, aObject);
}

class WebGLMemoryMultiReporterWrapper
{
    WebGLMemoryMultiReporterWrapper();
    ~WebGLMemoryMultiReporterWrapper();
    static WebGLMemoryMultiReporterWrapper* sUniqueInstance;

    
    
    
    typedef nsTArray<const WebGLContext*> ContextsArrayType;
    ContextsArrayType mContexts;
    
    nsCOMPtr<nsIMemoryMultiReporter> mReporter;

    static WebGLMemoryMultiReporterWrapper* UniqueInstance();

    static ContextsArrayType & Contexts() { return UniqueInstance()->mContexts; }

  public:

    static void AddWebGLContext(const WebGLContext* c) {
        Contexts().AppendElement(c);
    }

    static void RemoveWebGLContext(const WebGLContext* c) {
        ContextsArrayType & contexts = Contexts();
        contexts.RemoveElement(c);
        if (contexts.IsEmpty()) {
            delete sUniqueInstance; 
            sUniqueInstance = nsnull;
        }
    }

    static PRInt64 GetTextureMemoryUsed() {
        const ContextsArrayType & contexts = Contexts();
        PRInt64 result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i)
            for (size_t j = 0; j < contexts[i]->mTextures.Length(); ++j)
              result += contexts[i]->mTextures[j]->MemoryUsage();
        return result;
    }

    static PRInt64 GetTextureCount() {
        const ContextsArrayType & contexts = Contexts();
        PRInt64 result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i)
            result += contexts[i]->mTextures.Length();
        return result;
    }

    static PRInt64 GetBufferMemoryUsed() {
        const ContextsArrayType & contexts = Contexts();
        PRInt64 result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i)
            for (size_t j = 0; j < contexts[i]->mBuffers.Length(); ++j)
                result += contexts[i]->mBuffers[j]->ByteLength();
        return result;
    }

    static PRInt64 GetBufferCacheMemoryUsed();

    static PRInt64 GetBufferCount() {
        const ContextsArrayType & contexts = Contexts();
        PRInt64 result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i)
            result += contexts[i]->mBuffers.Length();
        return result;
    }

    static PRInt64 GetRenderbufferMemoryUsed() {
        const ContextsArrayType & contexts = Contexts();
        PRInt64 result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i)
            for (size_t j = 0; j < contexts[i]->mRenderbuffers.Length(); ++j)
              result += contexts[i]->mRenderbuffers[j]->MemoryUsage();
        return result;
    }

    static PRInt64 GetRenderbufferCount() {
        const ContextsArrayType & contexts = Contexts();
        PRInt64 result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i)
            result += contexts[i]->mRenderbuffers.Length();
        return result;
    }

    static PRInt64 GetShaderSize();

    static PRInt64 GetShaderCount() {
        const ContextsArrayType & contexts = Contexts();
        PRInt64 result = 0;
        for(size_t i = 0; i < contexts.Length(); ++i)
            result += contexts[i]->mShaders.Length();
        return result;
    }

    static PRInt64 GetContextCount() {
        return Contexts().Length();
    }
};

class WebGLMemoryPressureObserver MOZ_FINAL
    : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  WebGLMemoryPressureObserver(WebGLContext *context)
    : mContext(context)
  {}

private:
  WebGLContext *mContext;
};

}

#endif
