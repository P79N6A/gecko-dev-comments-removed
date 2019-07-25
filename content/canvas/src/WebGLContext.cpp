






































#include "WebGLContext.h"
#include "WebGLExtensions.h"

#include "nsIConsoleService.h"
#include "nsServiceManagerUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsDOMError.h"
#include "nsIGfxInfo.h"

#include "nsIPropertyBag.h"
#include "nsIVariant.h"

#include "imgIEncoder.h"

#include "gfxContext.h"
#include "gfxPattern.h"
#include "gfxUtils.h"

#include "CanvasUtils.h"
#include "nsDisplayList.h"

#include "GLContextProvider.h"

#include "gfxCrashReporterUtils.h"

#include "nsSVGEffects.h"

#include "prenv.h"

#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;
using namespace mozilla::gl;
using namespace mozilla::layers;

WebGLMemoryReporter* WebGLMemoryReporter::sUniqueInstance = nsnull;

NS_MEMORY_REPORTER_IMPLEMENT(WebGLTextureMemoryUsed,
                             "webgl-texture-memory",
                             KIND_OTHER,
                             UNITS_BYTES,
                             WebGLMemoryReporter::GetTextureMemoryUsed,
                             "Memory used by WebGL textures. The OpenGL implementation is free to store these textures in either video memory or main memory. This measurement is only a lower bound, actual memory usage may be higher for example if the storage is strided.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLTextureCount,
                             "webgl-texture-count",
                             KIND_OTHER,
                             UNITS_COUNT,
                             WebGLMemoryReporter::GetTextureCount,
                             "Number of WebGL textures.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLBufferMemoryUsed,
                             "webgl-buffer-memory",
                             KIND_OTHER,
                             UNITS_BYTES,
                             WebGLMemoryReporter::GetBufferMemoryUsed,
                             "Memory used by WebGL buffers. The OpenGL implementation is free to store these buffers in either video memory or main memory. This measurement is only a lower bound, actual memory usage may be higher for example if the storage is strided.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLBufferCacheMemoryUsed,
                             "explicit/webgl/buffer-cache-memory",
                             KIND_HEAP,
                             UNITS_BYTES,
                             WebGLMemoryReporter::GetBufferCacheMemoryUsed,
                             "Memory used by WebGL buffer caches. The WebGL implementation caches the contents of element array buffers only. This adds up with the webgl-buffer-memory value, but contrary to it, this one represents bytes on the heap, not managed by OpenGL.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLBufferCount,
                             "webgl-buffer-count",
                             KIND_OTHER,
                             UNITS_COUNT,
                             WebGLMemoryReporter::GetBufferCount,
                             "Number of WebGL buffers.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLRenderbufferMemoryUsed,
                             "webgl-renderbuffer-memory",
                             KIND_OTHER,
                             UNITS_BYTES,
                             WebGLMemoryReporter::GetRenderbufferMemoryUsed,
                             "Memory used by WebGL renderbuffers. The OpenGL implementation is free to store these renderbuffers in either video memory or main memory. This measurement is only a lower bound, actual memory usage may be higher for example if the storage is strided.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLRenderbufferCount,
                             "webgl-renderbuffer-count",
                             KIND_OTHER,
                             UNITS_COUNT,
                             WebGLMemoryReporter::GetRenderbufferCount,
                             "Number of WebGL renderbuffers.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLShaderSourcesSize,
                             "explicit/webgl/shader-sources-size",
                             KIND_HEAP,
                             UNITS_BYTES,
                             WebGLMemoryReporter::GetShaderSourcesSize,
                             "Combined size of WebGL shader ASCII sources, cached on the heap. This should always be at most a few kilobytes, or dozen kilobytes for very shader-intensive WebGL demos.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLShaderTranslationLogsSize,
                             "explicit/webgl/shader-translationlogs-size",
                             KIND_HEAP,
                             UNITS_BYTES,
                             WebGLMemoryReporter::GetShaderTranslationLogsSize,
                             "Combined size of WebGL shader ASCII translation logs, cached on the heap.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLShaderCount,
                             "webgl-shader-count",
                             KIND_OTHER,
                             UNITS_COUNT,
                             WebGLMemoryReporter::GetShaderCount,
                             "Number of WebGL shaders.")

NS_MEMORY_REPORTER_IMPLEMENT(WebGLContextCount,
                             "webgl-context-count",
                             KIND_OTHER,
                             UNITS_COUNT,
                             WebGLMemoryReporter::GetContextCount,
                             "Number of WebGL contexts.")

WebGLMemoryReporter* WebGLMemoryReporter::UniqueInstance()
{
    if (!sUniqueInstance) {
        sUniqueInstance = new WebGLMemoryReporter;
    }
    return sUniqueInstance;
}

WebGLMemoryReporter::WebGLMemoryReporter()
    : mTextureMemoryUsageReporter(new NS_MEMORY_REPORTER_NAME(WebGLTextureMemoryUsed))
    , mTextureCountReporter(new NS_MEMORY_REPORTER_NAME(WebGLTextureCount))
    , mBufferMemoryUsageReporter(new NS_MEMORY_REPORTER_NAME(WebGLBufferMemoryUsed))
    , mBufferCacheMemoryUsageReporter(new NS_MEMORY_REPORTER_NAME(WebGLBufferCacheMemoryUsed))
    , mBufferCountReporter(new NS_MEMORY_REPORTER_NAME(WebGLBufferCount))
    , mRenderbufferMemoryUsageReporter(new NS_MEMORY_REPORTER_NAME(WebGLRenderbufferMemoryUsed))
    , mRenderbufferCountReporter(new NS_MEMORY_REPORTER_NAME(WebGLRenderbufferCount))
    , mShaderSourcesSizeReporter(new NS_MEMORY_REPORTER_NAME(WebGLShaderSourcesSize))
    , mShaderTranslationLogsSizeReporter(new NS_MEMORY_REPORTER_NAME(WebGLShaderTranslationLogsSize))
    , mShaderCountReporter(new NS_MEMORY_REPORTER_NAME(WebGLShaderCount))
    , mContextCountReporter(new NS_MEMORY_REPORTER_NAME(WebGLContextCount))
{
    NS_RegisterMemoryReporter(mTextureMemoryUsageReporter);
    NS_RegisterMemoryReporter(mTextureCountReporter);
    NS_RegisterMemoryReporter(mBufferMemoryUsageReporter);
    NS_RegisterMemoryReporter(mBufferCacheMemoryUsageReporter);    
    NS_RegisterMemoryReporter(mBufferCountReporter);
    NS_RegisterMemoryReporter(mRenderbufferMemoryUsageReporter);
    NS_RegisterMemoryReporter(mRenderbufferCountReporter);
    NS_RegisterMemoryReporter(mShaderSourcesSizeReporter);
    NS_RegisterMemoryReporter(mShaderTranslationLogsSizeReporter);
    NS_RegisterMemoryReporter(mShaderCountReporter);
    NS_RegisterMemoryReporter(mContextCountReporter);
}

WebGLMemoryReporter::~WebGLMemoryReporter()
{
    NS_UnregisterMemoryReporter(mTextureMemoryUsageReporter);
    NS_UnregisterMemoryReporter(mTextureCountReporter);
    NS_UnregisterMemoryReporter(mBufferMemoryUsageReporter);
    NS_UnregisterMemoryReporter(mBufferCacheMemoryUsageReporter);
    NS_UnregisterMemoryReporter(mBufferCountReporter);
    NS_UnregisterMemoryReporter(mRenderbufferMemoryUsageReporter);
    NS_UnregisterMemoryReporter(mRenderbufferCountReporter);
    NS_UnregisterMemoryReporter(mShaderSourcesSizeReporter);
    NS_UnregisterMemoryReporter(mShaderTranslationLogsSizeReporter);
    NS_UnregisterMemoryReporter(mShaderCountReporter);
    NS_UnregisterMemoryReporter(mContextCountReporter);
}

nsresult NS_NewCanvasRenderingContextWebGL(nsIDOMWebGLRenderingContext** aResult);

nsresult
NS_NewCanvasRenderingContextWebGL(nsIDOMWebGLRenderingContext** aResult)
{
    Telemetry::Accumulate(Telemetry::CANVAS_WEBGL_USED, 1);
    nsIDOMWebGLRenderingContext* ctx = new WebGLContext();
    if (!ctx)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aResult = ctx);
    return NS_OK;
}

WebGLContext::WebGLContext()
    : mCanvasElement(nsnull),
      gl(nsnull)
{
    mWidth = mHeight = 0;
    mGeneration = 0;
    mInvalidated = false;
    mResetLayer = true;
    mVerbose = false;
    mOptionsFrozen = false;

    mActiveTexture = 0;
    mWebGLError = LOCAL_GL_NO_ERROR;
    mPixelStoreFlipY = false;
    mPixelStorePremultiplyAlpha = false;
    mPixelStoreColorspaceConversion = BROWSER_DEFAULT_WEBGL;

    mShaderValidation = true;

    mMapBuffers.Init();
    mMapTextures.Init();
    mMapPrograms.Init();
    mMapShaders.Init();
    mMapFramebuffers.Init();
    mMapRenderbuffers.Init();

    mBlackTexturesAreInitialized = false;
    mFakeBlackStatus = DoNotNeedFakeBlack;

    mVertexAttrib0Vector[0] = 0;
    mVertexAttrib0Vector[1] = 0;
    mVertexAttrib0Vector[2] = 0;
    mVertexAttrib0Vector[3] = 1;
    mFakeVertexAttrib0BufferObjectVector[0] = 0;
    mFakeVertexAttrib0BufferObjectVector[1] = 0;
    mFakeVertexAttrib0BufferObjectVector[2] = 0;
    mFakeVertexAttrib0BufferObjectVector[3] = 1;
    mFakeVertexAttrib0BufferObjectSize = 0;
    mFakeVertexAttrib0BufferObject = 0;
    mFakeVertexAttrib0BufferStatus = VertexAttrib0Status::Default;

    
    mColorWriteMask[0] = 1;
    mColorWriteMask[1] = 1;
    mColorWriteMask[2] = 1;
    mColorWriteMask[3] = 1;
    mDepthWriteMask = 1;
    mColorClearValue[0] = 0.f;
    mColorClearValue[1] = 0.f;
    mColorClearValue[2] = 0.f;
    mColorClearValue[3] = 0.f;
    mDepthClearValue = 1.f;
    mStencilClearValue = 0;
    mStencilRefFront = 0;
    mStencilRefBack = 0;
    mStencilValueMaskFront = 0xffffffff;
    mStencilValueMaskBack  = 0xffffffff;
    mStencilWriteMaskFront = 0xffffffff;
    mStencilWriteMaskBack  = 0xffffffff;

    mScissorTestEnabled = 0;
    mDitherEnabled = 1;
    mBackbufferClearingStatus = BackbufferClearingStatus::NotClearedSinceLastPresented;
    
    
    
    mGLMaxVertexAttribs = 0;
    mGLMaxTextureUnits = 0;
    mGLMaxTextureSize = 0;
    mGLMaxCubeMapTextureSize = 0;
    mGLMaxTextureImageUnits = 0;
    mGLMaxVertexTextureImageUnits = 0;
    mGLMaxVaryingVectors = 0;
    mGLMaxFragmentUniformVectors = 0;
    mGLMaxVertexUniformVectors = 0;
    
    
    mPixelStorePackAlignment = 4;
    mPixelStoreUnpackAlignment = 4;

    WebGLMemoryReporter::AddWebGLContext(this);

    mContextLost = false;
    mAllowRestore = false;
    mRobustnessTimerRunning = false;
    mDrawSinceRobustnessTimerSet = false;
    mContextRestorer = do_CreateInstance("@mozilla.org/timer;1");
}

WebGLContext::~WebGLContext()
{
    DestroyResourcesAndContext();
    WebGLMemoryReporter::RemoveWebGLContext(this);
    TerminateRobustnessTimer();
    mContextRestorer = nsnull;
}

static PLDHashOperator
DeleteTextureFunction(const PRUint32& aKey, WebGLTexture *aValue, void *aData)
{
    gl::GLContext *gl = (gl::GLContext *) aData;
    NS_ASSERTION(!aValue->Deleted(), "Texture is still in mMapTextures, but is deleted?");
    GLuint name = aValue->GLName();
    gl->fDeleteTextures(1, &name);
    aValue->Delete();
    return PL_DHASH_NEXT;
}

static PLDHashOperator
DeleteBufferFunction(const PRUint32& aKey, WebGLBuffer *aValue, void *aData)
{
    gl::GLContext *gl = (gl::GLContext *) aData;
    NS_ASSERTION(!aValue->Deleted(), "Buffer is still in mMapBuffers, but is deleted?");
    GLuint name = aValue->GLName();
    gl->fDeleteBuffers(1, &name);
    aValue->Delete();
    return PL_DHASH_NEXT;
}

static PLDHashOperator
DeleteFramebufferFunction(const PRUint32& aKey, WebGLFramebuffer *aValue, void *aData)
{
    gl::GLContext *gl = (gl::GLContext *) aData;
    NS_ASSERTION(!aValue->Deleted(), "Framebuffer is still in mMapFramebuffers, but is deleted?");
    GLuint name = aValue->GLName();
    gl->fDeleteFramebuffers(1, &name);
    aValue->Delete();
    return PL_DHASH_NEXT;
}

static PLDHashOperator
DeleteRenderbufferFunction(const PRUint32& aKey, WebGLRenderbuffer *aValue, void *aData)
{
    gl::GLContext *gl = (gl::GLContext *) aData;
    NS_ASSERTION(!aValue->Deleted(), "Renderbuffer is still in mMapRenderbuffers, but is deleted?");
    GLuint name = aValue->GLName();
    gl->fDeleteRenderbuffers(1, &name);
    aValue->Delete();
    return PL_DHASH_NEXT;
}

static PLDHashOperator
DeleteProgramFunction(const PRUint32& aKey, WebGLProgram *aValue, void *aData)
{
    gl::GLContext *gl = (gl::GLContext *) aData;
    NS_ASSERTION(!aValue->Deleted(), "Program is still in mMapPrograms, but is deleted?");
    GLuint name = aValue->GLName();
    gl->fDeleteProgram(name);
    aValue->Delete();
    return PL_DHASH_NEXT;
}

static PLDHashOperator
DeleteShaderFunction(const PRUint32& aKey, WebGLShader *aValue, void *aData)
{
    gl::GLContext *gl = (gl::GLContext *) aData;
    NS_ASSERTION(!aValue->Deleted(), "Shader is still in mMapShaders, but is deleted?");
    GLuint name = aValue->GLName();
    gl->fDeleteShader(name);
    aValue->Delete();
    return PL_DHASH_NEXT;
}

void
WebGLContext::DestroyResourcesAndContext()
{
    if (!gl)
        return;

    gl->MakeCurrent();

    mMapTextures.EnumerateRead(DeleteTextureFunction, gl);
    mMapTextures.Clear();

    mMapBuffers.EnumerateRead(DeleteBufferFunction, gl);
    mMapBuffers.Clear();

    mMapPrograms.EnumerateRead(DeleteProgramFunction, gl);
    mMapPrograms.Clear();

    mMapShaders.EnumerateRead(DeleteShaderFunction, gl);
    mMapShaders.Clear();

    mMapFramebuffers.EnumerateRead(DeleteFramebufferFunction, gl);
    mMapFramebuffers.Clear();

    mMapRenderbuffers.EnumerateRead(DeleteRenderbufferFunction, gl);
    mMapRenderbuffers.Clear();

    if (mBlackTexturesAreInitialized) {
        gl->fDeleteTextures(1, &mBlackTexture2D);
        gl->fDeleteTextures(1, &mBlackTextureCubeMap);
        mBlackTexturesAreInitialized = false;
    }

    if (mFakeVertexAttrib0BufferObject) {
        gl->fDeleteBuffers(1, &mFakeVertexAttrib0BufferObject);
    }

    
    
#ifdef DEBUG
    printf_stderr("--- WebGL context destroyed: %p\n", gl.get());
#endif

    gl = nsnull;
}

void
WebGLContext::Invalidate()
{
    if (mInvalidated)
        return;

    if (!mCanvasElement)
        return;

    nsSVGEffects::InvalidateDirectRenderingObservers(HTMLCanvasElement());

    mInvalidated = true;
    HTMLCanvasElement()->InvalidateCanvasContent(nsnull);
}


NS_IMETHODIMP
WebGLContext::GetCanvas(nsIDOMHTMLCanvasElement **canvas)
{
    NS_IF_ADDREF(*canvas = mCanvasElement);

    return NS_OK;
}





NS_IMETHODIMP
WebGLContext::SetCanvasElement(nsHTMLCanvasElement* aParentCanvas)
{
    mCanvasElement = aParentCanvas;

    return NS_OK;
}

static bool
GetBoolFromPropertyBag(nsIPropertyBag *bag, const char *propName, bool *boolResult)
{
    nsCOMPtr<nsIVariant> vv;
    bool bv;

    nsresult rv = bag->GetProperty(NS_ConvertASCIItoUTF16(propName), getter_AddRefs(vv));
    if (NS_FAILED(rv) || !vv)
        return false;

    rv = vv->GetAsBool(&bv);
    if (NS_FAILED(rv))
        return false;

    *boolResult = bv ? true : false;
    return true;
}

NS_IMETHODIMP
WebGLContext::SetContextOptions(nsIPropertyBag *aOptions)
{
    if (!aOptions)
        return NS_OK;

    WebGLContextOptions newOpts;

    GetBoolFromPropertyBag(aOptions, "stencil", &newOpts.stencil);
    GetBoolFromPropertyBag(aOptions, "depth", &newOpts.depth);
    GetBoolFromPropertyBag(aOptions, "alpha", &newOpts.alpha);
    GetBoolFromPropertyBag(aOptions, "premultipliedAlpha", &newOpts.premultipliedAlpha);
    GetBoolFromPropertyBag(aOptions, "antialias", &newOpts.antialias);
    GetBoolFromPropertyBag(aOptions, "preserveDrawingBuffer", &newOpts.preserveDrawingBuffer);

    
    newOpts.depth |= newOpts.stencil;

#if 0
    LogMessage("aaHint: %d stencil: %d depth: %d alpha: %d premult: %d preserve: %d\n",
               newOpts.antialias ? 1 : 0,
               newOpts.stencil ? 1 : 0,
               newOpts.depth ? 1 : 0,
               newOpts.alpha ? 1 : 0,
               newOpts.premultipliedAlpha ? 1 : 0,
               newOpts.preserveDrawingBuffer ? 1 : 0);
#endif

    if (mOptionsFrozen && newOpts != mOptions) {
        
        
        return NS_ERROR_FAILURE;
    }

    mOptions = newOpts;
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::SetDimensions(PRInt32 width, PRInt32 height)
{
    
  
    if (mCanvasElement) {
        HTMLCanvasElement()->InvalidateCanvas();
    }

    if (gl && mWidth == width && mHeight == height)
        return NS_OK;

    
    if (width == 0 || height == 0) {
        width = 1;
        height = 1;
    }

    
    if (gl) {
        gl->ResizeOffscreen(gfxIntSize(width, height)); 
        

        
        mWidth = gl->OffscreenActualSize().width;
        mHeight = gl->OffscreenActualSize().height;
        mResetLayer = true;
        return NS_OK;
    }

    

    ScopedGfxFeatureReporter reporter("WebGL");

    
    
    DestroyResourcesAndContext();

    
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), NS_ERROR_FAILURE);

    bool forceOSMesa =
        Preferences::GetBool("webgl.force_osmesa", false);
    bool preferEGL =
        Preferences::GetBool("webgl.prefer-egl", false);
    bool preferOpenGL =
        Preferences::GetBool("webgl.prefer-native-gl", false);
    bool forceEnabled =
        Preferences::GetBool("webgl.force-enabled", false);
    bool disabled =
        Preferences::GetBool("webgl.disabled", false);
    bool verbose =
        Preferences::GetBool("webgl.verbose", false);

    if (disabled)
        return NS_ERROR_FAILURE;

    mVerbose = verbose;

    
    
    
    

    
    
    
    if (!(mGeneration+1).valid())
        return NS_ERROR_FAILURE; 

    gl::ContextFormat format(gl::ContextFormat::BasicRGBA32);
    if (mOptions.depth) {
        format.depth = 24;
        format.minDepth = 16;
    }

    if (mOptions.stencil) {
        format.stencil = 8;
        format.minStencil = 8;
    }

    if (!mOptions.alpha) {
        
        
        format.red = 5;
        format.green = 6;
        format.blue = 5;

        format.alpha = 0;
        format.minAlpha = 0;
    }

    bool forceMSAA =
        Preferences::GetBool("webgl.msaa-force", false);

    PRInt32 status;
    nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
    if (mOptions.antialias &&
        gfxInfo &&
        NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_WEBGL_MSAA, &status))) {
        if (status == nsIGfxInfo::FEATURE_NO_INFO || forceMSAA) {
            PRUint32 msaaLevel = Preferences::GetUint("webgl.msaa-level", 2);
            format.samples = msaaLevel*msaaLevel;
        }
    }

    if (PR_GetEnv("MOZ_WEBGL_PREFER_EGL")) {
        preferEGL = true;
    }

    
    bool useOpenGL = true;
    bool useANGLE = true;

    if (gfxInfo && !forceEnabled) {
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_WEBGL_OPENGL, &status))) {
            if (status != nsIGfxInfo::FEATURE_NO_INFO) {
                useOpenGL = false;
            }
        }
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_WEBGL_ANGLE, &status))) {
            if (status != nsIGfxInfo::FEATURE_NO_INFO) {
                useANGLE = false;
            }
        }
    }

    
    if (PR_GetEnv("MOZ_WEBGL_FORCE_OPENGL")) {
        preferEGL = false;
        useANGLE = false;
        useOpenGL = true;
    }

    
    if (forceOSMesa) {
        gl = gl::GLContextProviderOSMesa::CreateOffscreen(gfxIntSize(width, height), format);
        if (!gl || !InitAndValidateGL()) {
            LogMessage("OSMesa forced, but creating context failed -- aborting!");
            return NS_ERROR_FAILURE;
        }
        LogMessage("Using software rendering via OSMesa (THIS WILL BE SLOW)");
    }

#ifdef XP_WIN
    
    if (!gl && (preferEGL || useANGLE) && !preferOpenGL) {
        gl = gl::GLContextProviderEGL::CreateOffscreen(gfxIntSize(width, height), format);
        if (gl && !InitAndValidateGL()) {
            gl = nsnull;
        }
    }

    
    if (!gl && useOpenGL) {
        gl = gl::GLContextProvider::CreateOffscreen(gfxIntSize(width, height), format);
        if (gl && !InitAndValidateGL()) {
            gl = nsnull;
        }
    }
#else
    
    if (!gl && useOpenGL) {
        gl = gl::GLContextProvider::CreateOffscreen(gfxIntSize(width, height), format);
        if (gl && !InitAndValidateGL()) {
            gl = nsnull;
        }
    }
#endif

    
    if (!gl) {
        gl = gl::GLContextProviderOSMesa::CreateOffscreen(gfxIntSize(width, height), format);
        if (!gl || !InitAndValidateGL()) {
            gl = nsnull;
        } else {
            LogMessage("Using software rendering via OSMesa (THIS WILL BE SLOW)");
        }
    }

    if (!gl) {
        LogMessage("Can't get a usable WebGL context");
        return NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    printf_stderr ("--- WebGL context created: %p\n", gl.get());
#endif

    mWidth = width;
    mHeight = height;
    mResetLayer = true;
    mOptionsFrozen = true;

    mHasRobustness = gl->HasRobustness();

    
    ++mGeneration;

#if 0
    if (mGeneration > 0) {
        
    }
#endif

    MakeContextCurrent();

    
    
    gl->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, gl->GetOffscreenFBO());
    gl->fViewport(0, 0, mWidth, mHeight);
    gl->fClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl->fClearDepth(1.0f);
    gl->fClearStencil(0);
    gl->fClear(LOCAL_GL_COLOR_BUFFER_BIT | LOCAL_GL_DEPTH_BUFFER_BIT | LOCAL_GL_STENCIL_BUFFER_BIT);

    reporter.SetSuccessful();
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

    gl->ReadPixelsIntoImageSurface(0, 0, mWidth, mHeight, surf);
    gfxUtils::PremultiplyImageSurface(surf);

    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);
    pat->SetFilter(f);

    
    
    
    gfxMatrix m;
    m.Translate(gfxPoint(0.0, mHeight));
    m.Scale(1.0, -1.0);
    pat->SetMatrix(m);

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
    NS_ASSERTION(gl, "GetInputStream on invalid context?");
    if (!gl)
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxImageSurface> surf = new gfxImageSurface(gfxIntSize(mWidth, mHeight),
                                                         gfxASurface::ImageFormatARGB32);
    if (surf->CairoStatus() != 0)
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxContext> tmpcx = new gfxContext(surf);
    
    nsresult rv = Render(tmpcx, gfxPattern::FILTER_NEAREST);
    if (NS_FAILED(rv))
        return rv;

    const char encoderPrefix[] = "@mozilla.org/image/encoder;2?type=";
    nsAutoArrayPtr<char> conid(new char[strlen(encoderPrefix) + strlen(aMimeType) + 1]);

    if (!conid)
        return NS_ERROR_OUT_OF_MEMORY;

    strcpy(conid, encoderPrefix);
    strcat(conid, aMimeType);

    nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(conid);
    if (!encoder)
        return NS_ERROR_FAILURE;

    rv = encoder->InitFromData(surf->Data(),
                               mWidth * mHeight * 4,
                               mWidth, mHeight,
                               surf->Stride(),
                               imgIEncoder::INPUT_FORMAT_HOSTARGB,
                               nsDependentString(aEncoderOptions));
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(encoder, aStream);
}

NS_IMETHODIMP
WebGLContext::GetThebesSurface(gfxASurface **surface)
{
    return NS_ERROR_NOT_AVAILABLE;
}

static PRUint8 gWebGLLayerUserData;

namespace mozilla {

class WebGLContextUserData : public LayerUserData {
public:
    WebGLContextUserData(nsHTMLCanvasElement *aContent)
    : mContent(aContent) {}

  


  static void DidTransactionCallback(void* aData)
  {
    WebGLContextUserData *userdata = static_cast<WebGLContextUserData*>(aData);
    nsHTMLCanvasElement *canvas = userdata->mContent;
    WebGLContext *context = static_cast<WebGLContext*>(canvas->GetContextAtIndex(0));

    context->mBackbufferClearingStatus = BackbufferClearingStatus::NotClearedSinceLastPresented;
    canvas->MarkContextClean();
  }

private:
  nsRefPtr<nsHTMLCanvasElement> mContent;
};

} 

already_AddRefed<layers::CanvasLayer>
WebGLContext::GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                             CanvasLayer *aOldLayer,
                             LayerManager *aManager)
{
    if (mContextLost)
        return nsnull;

    if (!mResetLayer && aOldLayer &&
        aOldLayer->HasUserData(&gWebGLLayerUserData)) {
        NS_ADDREF(aOldLayer);
        return aOldLayer;
    }

    nsRefPtr<CanvasLayer> canvasLayer = aManager->CreateCanvasLayer();
    if (!canvasLayer) {
        NS_WARNING("CreateCanvasLayer returned null!");
        return nsnull;
    }
    WebGLContextUserData *userData = nsnull;
    if (aBuilder->IsPaintingToWindow()) {
      
      
      
      
      
      

      
      
      
      
      
      userData = new WebGLContextUserData(HTMLCanvasElement());
      canvasLayer->SetDidTransactionCallback(
              WebGLContextUserData::DidTransactionCallback, userData);
    }
    canvasLayer->SetUserData(&gWebGLLayerUserData, userData);

    CanvasLayer::Data data;

    
    
    

    void* native_surface = gl->GetNativeData(gl::GLContext::NativeImageSurface);

    if (native_surface) {
        data.mSurface = static_cast<gfxASurface*>(native_surface);
    } else {
        data.mGLContext = gl.get();
    }

    data.mSize = nsIntSize(mWidth, mHeight);
    data.mGLBufferIsPremultiplied = mOptions.premultipliedAlpha ? true : false;

    canvasLayer->Initialize(data);
    PRUint32 flags = gl->CreationFormat().alpha == 0 ? Layer::CONTENT_OPAQUE : 0;
    canvasLayer->SetContentFlags(flags);
    canvasLayer->Updated();

    mResetLayer = false;

    return canvasLayer.forget().get();
}

NS_IMETHODIMP
WebGLContext::GetContextAttributes(jsval *aResult)
{
    if (mContextLost)
    {
        *aResult = OBJECT_TO_JSVAL(NULL);
        return NS_OK;
    }

    JSContext *cx = nsContentUtils::GetCurrentJSContext();
    if (!cx)
        return NS_ERROR_FAILURE;

    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    if (!obj)
        return NS_ERROR_FAILURE;

    *aResult = OBJECT_TO_JSVAL(obj);

    gl::ContextFormat cf = gl->ActualFormat();

    if (!JS_DefineProperty(cx, obj, "alpha", cf.alpha > 0 ? JSVAL_TRUE : JSVAL_FALSE,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, obj, "depth", cf.depth > 0 ? JSVAL_TRUE : JSVAL_FALSE,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, obj, "stencil", cf.stencil > 0 ? JSVAL_TRUE : JSVAL_FALSE,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, obj, "antialias", cf.samples > 0 ? JSVAL_TRUE : JSVAL_FALSE,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, obj, "premultipliedAlpha",
                           mOptions.premultipliedAlpha ? JSVAL_TRUE : JSVAL_FALSE,
                           NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, obj, "preserveDrawingBuffer",
                           mOptions.preserveDrawingBuffer ? JSVAL_TRUE : JSVAL_FALSE,
                           NULL, NULL, JSPROP_ENUMERATE))
    {
        *aResult = JSVAL_VOID;
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::MozGetUnderlyingParamString(PRUint32 pname, nsAString& retval)
{
    if (mContextLost)
        return NS_OK;

    retval.SetIsVoid(true);

    MakeContextCurrent();

    switch (pname) {
    case LOCAL_GL_VENDOR:
    case LOCAL_GL_RENDERER:
    case LOCAL_GL_VERSION:
    case LOCAL_GL_SHADING_LANGUAGE_VERSION:
    case LOCAL_GL_EXTENSIONS: {
        const char *s = (const char *) gl->fGetString(pname);
        retval.Assign(NS_ConvertASCIItoUTF16(nsDependentCString(s)));
    }
        break;

    default:
        return NS_ERROR_INVALID_ARG;
    }

    return NS_OK;
}

bool WebGLContext::IsExtensionSupported(WebGLExtensionID ei)
{
    bool isSupported;

    switch (ei) {
        case WebGL_OES_texture_float:
            MakeContextCurrent();
            isSupported = gl->IsExtensionSupported(gl->IsGLES2() ? GLContext::OES_texture_float 
                                                                 : GLContext::ARB_texture_float);
	    break;
        case WebGL_OES_standard_derivatives:
            
            isSupported = true;
            break;
        case WebGL_WEBGL_EXT_lose_context:
            
            isSupported = true;
            break;
        default:
            isSupported = false;
    }

    return isSupported;
}

NS_IMETHODIMP
WebGLContext::GetExtension(const nsAString& aName, nsIWebGLExtension **retval)
{
    *retval = nsnull;
    if (mContextLost)
        return NS_OK;
    
    if (mDisableExtensions) {
        return NS_OK;
    }

    
    WebGLExtensionID ei = WebGLExtensionID_Max;
    if (aName.EqualsLiteral("OES_texture_float")) {
        if (IsExtensionSupported(WebGL_OES_texture_float))
            ei = WebGL_OES_texture_float;
    }
    else if (aName.EqualsLiteral("OES_standard_derivatives")) {
        if (IsExtensionSupported(WebGL_OES_standard_derivatives))
            ei = WebGL_OES_standard_derivatives;
    }
    else if (aName.EqualsLiteral("WEBGL_EXT_lose_context")) {
        if (IsExtensionSupported(WebGL_WEBGL_EXT_lose_context))
            ei = WebGL_WEBGL_EXT_lose_context;
    }

    if (ei != WebGLExtensionID_Max) {
        if (!IsExtensionEnabled(ei)) {
            switch (ei) {
                case WebGL_OES_standard_derivatives:
                    mEnabledExtensions[ei] = new WebGLExtensionStandardDerivatives(this);
                    break;
                case WebGL_WEBGL_EXT_lose_context:
                    mEnabledExtensions[ei] = new WebGLExtensionLoseContext(this);
                    break;
                
                
                default:
                    mEnabledExtensions[ei] = new WebGLExtension(this);
                    break;
            }
        }
        NS_ADDREF(*retval = mEnabledExtensions[ei]);
    }

    return NS_OK;
}

void
WebGLContext::ForceClearFramebufferWithDefaultValues(PRUint32 mask, const nsIntRect& viewportRect)
{
    MakeContextCurrent();

    bool initializeColorBuffer = 0 != (mask & LOCAL_GL_COLOR_BUFFER_BIT);
    bool initializeDepthBuffer = 0 != (mask & LOCAL_GL_DEPTH_BUFFER_BIT);
    bool initializeStencilBuffer = 0 != (mask & LOCAL_GL_STENCIL_BUFFER_BIT);

    
    gl->fDisable(LOCAL_GL_SCISSOR_TEST);
    gl->fDisable(LOCAL_GL_DITHER);
    gl->PushViewportRect(viewportRect);

    if (initializeColorBuffer) {
        gl->fColorMask(1, 1, 1, 1);
        gl->fClearColor(0.f, 0.f, 0.f, 0.f);
    }

    if (initializeDepthBuffer) {
        gl->fDepthMask(1);
        gl->fClearDepth(1.0f);
    }

    if (initializeStencilBuffer) {
        gl->fStencilMask(0xffffffff);
        gl->fClearStencil(0);
    }

    
    gl->fClear(mask);

    
    if (initializeColorBuffer) {
        gl->fColorMask(mColorWriteMask[0],
                       mColorWriteMask[1],
                       mColorWriteMask[2],
                       mColorWriteMask[3]);
        gl->fClearColor(mColorClearValue[0],
                        mColorClearValue[1],
                        mColorClearValue[2],
                        mColorClearValue[3]);
    }

    if (initializeDepthBuffer) {
        gl->fDepthMask(mDepthWriteMask);
        gl->fClearDepth(mDepthClearValue);
    }

    if (initializeStencilBuffer) {
        gl->fStencilMaskSeparate(LOCAL_GL_FRONT, mStencilWriteMaskFront);
        gl->fStencilMaskSeparate(LOCAL_GL_BACK, mStencilWriteMaskBack);
        gl->fClearStencil(mStencilClearValue);
    }

    gl->PopViewportRect();

    if (mDitherEnabled)
        gl->fEnable(LOCAL_GL_DITHER);
    else
        gl->fDisable(LOCAL_GL_DITHER);

    if (mScissorTestEnabled)
        gl->fEnable(LOCAL_GL_SCISSOR_TEST);
    else
        gl->fDisable(LOCAL_GL_SCISSOR_TEST);
}

void
WebGLContext::EnsureBackbufferClearedAsNeeded()
{
    if (mOptions.preserveDrawingBuffer)
        return;

    NS_ABORT_IF_FALSE(!mBoundFramebuffer,
                      "EnsureBackbufferClearedAsNeeded must not be called when a FBO is bound");

    if (mBackbufferClearingStatus != BackbufferClearingStatus::NotClearedSinceLastPresented)
        return;

    mBackbufferClearingStatus = BackbufferClearingStatus::ClearedToDefaultValues;

    ForceClearFramebufferWithDefaultValues(LOCAL_GL_COLOR_BUFFER_BIT |
                                           LOCAL_GL_DEPTH_BUFFER_BIT |
                                           LOCAL_GL_STENCIL_BUFFER_BIT,
                                           nsIntRect(0, 0, mWidth, mHeight));

    Invalidate();
}

NS_IMETHODIMP
WebGLContext::Notify(nsITimer* timer)
{
    TerminateRobustnessTimer();
    MaybeRestoreContext();
    return NS_OK;
}

void
WebGLContext::MaybeRestoreContext()
{
    if (mContextLost || mAllowRestore)
        return;

    GLContext::ContextResetARB resetStatus = GLContext::CONTEXT_NO_ERROR;
    if (mHasRobustness) {
        gl->MakeCurrent();
        resetStatus = (GLContext::ContextResetARB) gl->fGetGraphicsResetStatus();
    
    
    } else if (gl->GetContextType() == GLContext::ContextTypeEGL) {
        
        
        
        
        if (!gl->MakeCurrent(true) && gl->IsContextLost()) {
            resetStatus = GLContext::CONTEXT_GUILTY_CONTEXT_RESET_ARB;
        }
    }
    
    if (resetStatus != GLContext::CONTEXT_NO_ERROR) {
        
        
        ForceLoseContext();
    }

    switch (resetStatus) {
        case GLContext::CONTEXT_NO_ERROR:
            
            
            
            if (mDrawSinceRobustnessTimerSet)
                SetupRobustnessTimer();
            return;
        case GLContext::CONTEXT_GUILTY_CONTEXT_RESET_ARB:
            NS_WARNING("WebGL content on the page caused the graphics card to reset; not restoring the context");
            return;
        case GLContext::CONTEXT_INNOCENT_CONTEXT_RESET_ARB:
            break;
        case GLContext::CONTEXT_UNKNOWN_CONTEXT_RESET_ARB:
            NS_WARNING("WebGL content on the page might have caused the graphics card to reset");
            break;
    }

    ForceRestoreContext();
}

void
WebGLContext::ForceLoseContext()
{
    TerminateRobustnessTimer();

    mWebGLError = LOCAL_GL_CONTEXT_LOST;

    bool defaultAction;
    mContextLost = true;
    DestroyResourcesAndContext();
    nsContentUtils::DispatchTrustedEvent(HTMLCanvasElement()->OwnerDoc(), 
                                         (nsIDOMHTMLCanvasElement*) HTMLCanvasElement(), 
                                         NS_LITERAL_STRING("webglcontextlost"), 
                                         PR_TRUE, 
                                         PR_TRUE,
                                         &defaultAction);
    if (defaultAction)
        mAllowRestore = false;
}

void
WebGLContext::ForceRestoreContext()
{
    mContextLost = false;
    mAllowRestore = false;
    SetDimensions(mHeight, mWidth);
    nsContentUtils::DispatchTrustedEvent(HTMLCanvasElement()->OwnerDoc(), 
                                         (nsIDOMHTMLCanvasElement*) HTMLCanvasElement(), 
                                         NS_LITERAL_STRING("webglcontextrestored"), 
                                         PR_TRUE, 
                                         PR_TRUE);
}





NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLContext)

NS_IMPL_CYCLE_COLLECTION_CLASS(WebGLContext)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(WebGLContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCanvasElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(WebGLContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCanvasElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(WebGLRenderingContext, WebGLContext)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLContext)
  NS_INTERFACE_MAP_ENTRY(nsIDOMWebGLRenderingContext)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMWebGLRenderingContext)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLRenderingContext)
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

NS_IMPL_ADDREF(WebGLUniformLocation)
NS_IMPL_RELEASE(WebGLUniformLocation)

DOMCI_DATA(WebGLUniformLocation, WebGLUniformLocation)

NS_INTERFACE_MAP_BEGIN(WebGLUniformLocation)
  NS_INTERFACE_MAP_ENTRY(WebGLUniformLocation)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLUniformLocation)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLUniformLocation)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLActiveInfo)
NS_IMPL_RELEASE(WebGLActiveInfo)

DOMCI_DATA(WebGLActiveInfo, WebGLActiveInfo)

NS_INTERFACE_MAP_BEGIN(WebGLActiveInfo)
  NS_INTERFACE_MAP_ENTRY(WebGLActiveInfo)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLActiveInfo)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLActiveInfo)
NS_INTERFACE_MAP_END

#define NAME_NOT_SUPPORTED(base) \
NS_IMETHODIMP base::GetName(WebGLuint *aName) \
{ return NS_ERROR_NOT_IMPLEMENTED; } \
NS_IMETHODIMP base::SetName(WebGLuint aName) \
{ return NS_ERROR_NOT_IMPLEMENTED; }

NAME_NOT_SUPPORTED(WebGLTexture)
NAME_NOT_SUPPORTED(WebGLBuffer)
NAME_NOT_SUPPORTED(WebGLProgram)
NAME_NOT_SUPPORTED(WebGLShader)
NAME_NOT_SUPPORTED(WebGLFramebuffer)
NAME_NOT_SUPPORTED(WebGLRenderbuffer)

NS_IMPL_ADDREF(WebGLExtension)
NS_IMPL_RELEASE(WebGLExtension)

DOMCI_DATA(WebGLExtension, WebGLExtension)

NS_INTERFACE_MAP_BEGIN(WebGLExtension)
  NS_INTERFACE_MAP_ENTRY(WebGLExtension)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtension)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtension)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(WebGLExtensionStandardDerivatives)
NS_IMPL_RELEASE(WebGLExtensionStandardDerivatives)

DOMCI_DATA(WebGLExtensionStandardDerivatives, WebGLExtensionStandardDerivatives)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionStandardDerivatives)
  
  
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionStandardDerivatives)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionStandardDerivatives)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)

NS_IMPL_ADDREF(WebGLExtensionLoseContext)
NS_IMPL_RELEASE(WebGLExtensionLoseContext)

DOMCI_DATA(WebGLExtensionLoseContext, WebGLExtensionLoseContext)

NS_INTERFACE_MAP_BEGIN(WebGLExtensionLoseContext)
  NS_INTERFACE_MAP_ENTRY(nsIWebGLExtensionLoseContext)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, WebGLExtension)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(WebGLExtensionLoseContext)
NS_INTERFACE_MAP_END_INHERITING(WebGLExtension)


NS_IMETHODIMP
WebGLContext::GetDrawingBufferWidth(WebGLsizei *aWidth)
{
    if (mContextLost)
        return NS_OK;

    *aWidth = mWidth;
    return NS_OK;
}


NS_IMETHODIMP
WebGLContext::GetDrawingBufferHeight(WebGLsizei *aHeight)
{
    if (mContextLost)
        return NS_OK;

    *aHeight = mHeight;
    return NS_OK;
}


NS_IMETHODIMP
WebGLUniformLocation::GetLocation(WebGLint *aLocation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
WebGLUniformLocation::SetLocation(WebGLint aLocation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
WebGLActiveInfo::GetSize(WebGLint *aSize)
{
    *aSize = mSize;
    return NS_OK;
}


NS_IMETHODIMP
WebGLActiveInfo::GetType(WebGLenum *aType)
{
    *aType = mType;
    return NS_OK;
}


NS_IMETHODIMP
WebGLActiveInfo::GetName(nsAString & aName)
{
    aName = mName;
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::GetSupportedExtensions(nsIVariant **retval)
{
    *retval = nsnull;
    if (mContextLost)
        return NS_OK;
    
    if (mDisableExtensions) {
        return NS_OK;
    }
    
    nsCOMPtr<nsIWritableVariant> wrval = do_CreateInstance("@mozilla.org/variant;1");
    NS_ENSURE_TRUE(wrval, NS_ERROR_FAILURE);

    nsTArray<const char *> extList;

    if (IsExtensionSupported(WebGL_OES_texture_float))
        extList.InsertElementAt(extList.Length(), "OES_texture_float");
    if (IsExtensionSupported(WebGL_OES_standard_derivatives))
        extList.InsertElementAt(extList.Length(), "OES_standard_derivatives");
    if (IsExtensionSupported(WebGL_WEBGL_EXT_lose_context))
        extList.InsertElementAt(extList.Length(), "WEBGL_EXT_lose_context");

    nsresult rv;
    if (extList.Length() > 0) {
        rv = wrval->SetAsArray(nsIDataType::VTYPE_CHAR_STR, nsnull,
                               extList.Length(), &extList[0]);
    } else {
        rv = wrval->SetAsEmptyArray();
    }
    if (NS_FAILED(rv))
        return rv;

    *retval = wrval.forget().get();
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::IsContextLost(WebGLboolean *retval)
{
    *retval = mContextLost;
    return NS_OK;
}

