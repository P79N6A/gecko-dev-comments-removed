




#include "WebGLContext.h"

#include "AccessCheck.h"
#include "CanvasUtils.h"
#include "gfxContext.h"
#include "gfxCrashReporterUtils.h"
#include "gfxPattern.h"
#include "gfxPrefs.h"
#include "gfxUtils.h"
#include "GLBlitHelper.h"
#include "GLContext.h"
#include "GLContextProvider.h"
#include "GLReadTexImageHelper.h"
#include "ImageContainer.h"
#include "ImageEncoder.h"
#include "Layers.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/HTMLVideoElement.h"
#include "mozilla/dom/ImageData.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "mozilla/EnumeratedArrayCycleCollection.h"
#include "mozilla/Preferences.h"
#include "mozilla/ProcessPriorityManager.h"
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsError.h"
#include "nsIClassInfoImpl.h"
#include "nsIConsoleService.h"
#include "nsIGfxInfo.h"
#include "nsIObserverService.h"
#include "nsIDOMEvent.h"
#include "nsIVariant.h"
#include "nsIWidget.h"
#include "nsIXPConnect.h"
#include "nsServiceManagerUtils.h"
#include "nsSVGEffects.h"
#include "prenv.h"
#include <queue>
#include "ScopedGLHelpers.h"
#include "WebGL1Context.h"
#include "WebGLBuffer.h"
#include "WebGLContextLossHandler.h"
#include "WebGLContextUtils.h"
#include "WebGLExtensions.h"
#include "WebGLFramebuffer.h"
#include "WebGLMemoryTracker.h"
#include "WebGLObjectModel.h"
#include "WebGLQuery.h"
#include "WebGLSampler.h"
#include "WebGLTransformFeedback.h"
#include "WebGLVertexArray.h"
#include "WebGLVertexAttribData.h"

#ifdef MOZ_WIDGET_GONK
#include "mozilla/layers/ShadowLayers.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;
using namespace mozilla::gl;
using namespace mozilla::layers;

WebGLObserver::WebGLObserver(WebGLContext* webgl)
    : mWebGL(webgl)
{
}

WebGLObserver::~WebGLObserver()
{
}

void
WebGLObserver::Destroy()
{
    UnregisterMemoryPressureEvent();
    UnregisterVisibilityChangeEvent();
    mWebGL = nullptr;
}

void
WebGLObserver::RegisterVisibilityChangeEvent()
{
    if (!mWebGL)
        return;

    HTMLCanvasElement* canvas = mWebGL->GetCanvas();
    MOZ_ASSERT(canvas);

    if (canvas) {
        nsIDocument* document = canvas->OwnerDoc();

        document->AddSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                         this, true, false);
    }
}

void
WebGLObserver::UnregisterVisibilityChangeEvent()
{
    if (!mWebGL)
        return;

    HTMLCanvasElement* canvas = mWebGL->GetCanvas();

    if (canvas) {
        nsIDocument* document = canvas->OwnerDoc();

        document->RemoveSystemEventListener(NS_LITERAL_STRING("visibilitychange"),
                                            this, true);
    }
}

void
WebGLObserver::RegisterMemoryPressureEvent()
{
    if (!mWebGL)
        return;

    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();

    MOZ_ASSERT(observerService);

    if (observerService)
        observerService->AddObserver(this, "memory-pressure", false);
}

void
WebGLObserver::UnregisterMemoryPressureEvent()
{
    if (!mWebGL)
        return;

    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();

    
    
    
    if (observerService)
        observerService->RemoveObserver(this, "memory-pressure");
}

NS_IMETHODIMP
WebGLObserver::Observe(nsISupports*, const char* topic, const char16_t*)
{
    if (!mWebGL || strcmp(topic, "memory-pressure")) {
        return NS_OK;
    }

    bool wantToLoseContext = mWebGL->mLoseContextOnMemoryPressure;

    if (!mWebGL->mCanLoseContextInForeground &&
        ProcessPriorityManager::CurrentProcessIsForeground())
    {
        wantToLoseContext = false;
    }

    if (wantToLoseContext)
        mWebGL->ForceLoseContext();

    return NS_OK;
}

NS_IMETHODIMP
WebGLObserver::HandleEvent(nsIDOMEvent* event)
{
    nsAutoString type;
    event->GetType(type);
    if (!mWebGL || !type.EqualsLiteral("visibilitychange"))
        return NS_OK;

    HTMLCanvasElement* canvas = mWebGL->GetCanvas();
    MOZ_ASSERT(canvas);

    if (canvas && !canvas->OwnerDoc()->Hidden())
        mWebGL->ForceRestoreContext();

    return NS_OK;
}

WebGLContextOptions::WebGLContextOptions()
    : alpha(true)
    , depth(true)
    , stencil(false)
    , premultipliedAlpha(true)
    , antialias(true)
    , preserveDrawingBuffer(false)
{
    
    if (Preferences::GetBool("webgl.default-no-alpha", false))
        alpha = false;
}

WebGLContext::WebGLContext()
    : WebGLContextUnchecked(nullptr)
    , mNeedsFakeNoAlpha(false)
{
    mGeneration = 0;
    mInvalidated = false;
    mShouldPresent = true;
    mResetLayer = true;
    mOptionsFrozen = false;

    mActiveTexture = 0;
    mPixelStoreFlipY = false;
    mPixelStorePremultiplyAlpha = false;
    mPixelStoreColorspaceConversion = BROWSER_DEFAULT_WEBGL;

    mShaderValidation = true;

    mFakeBlackStatus = WebGLContextFakeBlackStatus::NotNeeded;

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
    mFakeVertexAttrib0BufferStatus = WebGLVertexAttrib0Status::Default;

    mViewportX = 0;
    mViewportY = 0;
    mViewportWidth = 0;
    mViewportHeight = 0;

    mScissorTestEnabled = 0;
    mDitherEnabled = 1;
    mRasterizerDiscardEnabled = 0; 

    
    
    mGLMaxVertexAttribs = 0;
    mGLMaxTextureUnits = 0;
    mGLMaxTextureSize = 0;
    mGLMaxTextureSizeLog2 = 0;
    mGLMaxCubeMapTextureSize = 0;
    mGLMaxCubeMapTextureSizeLog2 = 0;
    mGLMaxRenderbufferSize = 0;
    mGLMaxTextureImageUnits = 0;
    mGLMaxVertexTextureImageUnits = 0;
    mGLMaxVaryingVectors = 0;
    mGLMaxFragmentUniformVectors = 0;
    mGLMaxVertexUniformVectors = 0;
    mGLMaxColorAttachments = 1;
    mGLMaxDrawBuffers = 1;
    mGLMaxTransformFeedbackSeparateAttribs = 0;
    mGLMaxUniformBufferBindings = 0;

    
    mPixelStorePackAlignment = 4;
    mPixelStoreUnpackAlignment = 4;

    WebGLMemoryTracker::AddWebGLContext(this);

    mAllowContextRestore = true;
    mLastLossWasSimulated = false;
    mContextLossHandler = new WebGLContextLossHandler(this);
    mContextStatus = ContextNotLost;
    mLoseContextOnMemoryPressure = false;
    mCanLoseContextInForeground = true;
    mRestoreWhenVisible = false;

    mAlreadyGeneratedWarnings = 0;
    mAlreadyWarnedAboutFakeVertexAttrib0 = false;
    mAlreadyWarnedAboutViewportLargerThanDest = false;

    mMaxWarnings = Preferences::GetInt("webgl.max-warnings-per-context", 32);
    if (mMaxWarnings < -1) {
        GenerateWarning("webgl.max-warnings-per-context size is too large (seems like a negative value wrapped)");
        mMaxWarnings = 0;
    }

    mContextObserver = new WebGLObserver(this);
    MOZ_RELEASE_ASSERT(mContextObserver, "Can't alloc WebGLContextObserver");

    mLastUseIndex = 0;

    InvalidateBufferFetching();

    mBackbufferNeedsClear = true;

    mDisableFragHighP = false;

    mDrawCallsSinceLastFlush = 0;
}

WebGLContext::~WebGLContext()
{
    RemovePostRefreshObserver();
    mContextObserver->Destroy();

    DestroyResourcesAndContext();
    WebGLMemoryTracker::RemoveWebGLContext(this);

    mContextLossHandler->DisableTimer();
    mContextLossHandler = nullptr;
}

void
WebGLContext::DestroyResourcesAndContext()
{
    mContextObserver->UnregisterMemoryPressureEvent();

    if (!gl)
        return;

    gl->MakeCurrent();

    mBound2DTextures.Clear();
    mBoundCubeMapTextures.Clear();
    mBound3DTextures.Clear();
    mBoundArrayBuffer = nullptr;
    mBoundCopyReadBuffer = nullptr;
    mBoundCopyWriteBuffer = nullptr;
    mBoundPixelPackBuffer = nullptr;
    mBoundPixelUnpackBuffer = nullptr;
    mBoundTransformFeedbackBuffer = nullptr;
    mBoundUniformBuffer = nullptr;
    mCurrentProgram = nullptr;
    mBoundFramebuffer = nullptr;
    mActiveOcclusionQuery = nullptr;
    mBoundRenderbuffer = nullptr;
    mBoundVertexArray = nullptr;
    mDefaultVertexArray = nullptr;
    mBoundTransformFeedback = nullptr;
    mDefaultTransformFeedback = nullptr;

    if (mBoundTransformFeedbackBuffers) {
        for (GLuint i = 0; i < mGLMaxTransformFeedbackSeparateAttribs; i++) {
            mBoundTransformFeedbackBuffers[i] = nullptr;
        }
    }

    for (GLuint i = 0; i < mGLMaxUniformBufferBindings; i++)
        mBoundUniformBuffers[i] = nullptr;

    while (!mTextures.isEmpty())
        mTextures.getLast()->DeleteOnce();
    while (!mVertexArrays.isEmpty())
        mVertexArrays.getLast()->DeleteOnce();
    while (!mBuffers.isEmpty())
        mBuffers.getLast()->DeleteOnce();
    while (!mRenderbuffers.isEmpty())
        mRenderbuffers.getLast()->DeleteOnce();
    while (!mFramebuffers.isEmpty())
        mFramebuffers.getLast()->DeleteOnce();
    while (!mShaders.isEmpty())
        mShaders.getLast()->DeleteOnce();
    while (!mPrograms.isEmpty())
        mPrograms.getLast()->DeleteOnce();
    while (!mQueries.isEmpty())
        mQueries.getLast()->DeleteOnce();
    while (!mSamplers.isEmpty())
        mSamplers.getLast()->DeleteOnce();
    while (!mTransformFeedbacks.isEmpty())
        mTransformFeedbacks.getLast()->DeleteOnce();

    mBlackOpaqueTexture2D = nullptr;
    mBlackOpaqueTextureCubeMap = nullptr;
    mBlackTransparentTexture2D = nullptr;
    mBlackTransparentTextureCubeMap = nullptr;

    if (mFakeVertexAttrib0BufferObject)
        gl->fDeleteBuffers(1, &mFakeVertexAttrib0BufferObject);

    
    
    for (size_t i = 0; i < size_t(WebGLExtensionID::Max); ++i) {
        WebGLExtensionID extension = WebGLExtensionID(i);

        if (!IsExtensionEnabled(extension) || (extension == WebGLExtensionID::WEBGL_lose_context))
            continue;

        mExtensions[extension]->MarkLost();
        mExtensions[extension] = nullptr;
    }

    
    
#ifdef DEBUG
    if (gl->DebugMode())
        printf_stderr("--- WebGL context destroyed: %p\n", gl.get());
#endif

    gl = nullptr;
}

void
WebGLContext::Invalidate()
{
    if (mInvalidated)
        return;

    if (!mCanvasElement)
        return;

    nsSVGEffects::InvalidateDirectRenderingObservers(mCanvasElement);

    mInvalidated = true;
    mCanvasElement->InvalidateCanvasContent(nullptr);
}





NS_IMETHODIMP
WebGLContext::SetContextOptions(JSContext* cx, JS::Handle<JS::Value> options)
{
    if (options.isNullOrUndefined() && mOptionsFrozen)
        return NS_OK;

    WebGLContextAttributes attributes;
    NS_ENSURE_TRUE(attributes.Init(cx, options), NS_ERROR_UNEXPECTED);

    WebGLContextOptions newOpts;

    newOpts.stencil = attributes.mStencil;
    newOpts.depth = attributes.mDepth;
    newOpts.premultipliedAlpha = attributes.mPremultipliedAlpha;
    newOpts.antialias = attributes.mAntialias;
    newOpts.preserveDrawingBuffer = attributes.mPreserveDrawingBuffer;

    if (attributes.mAlpha.WasPassed())
        newOpts.alpha = attributes.mAlpha.Value();

    
    if (!gfxPrefs::MSAALevel())
      newOpts.antialias = false;

#if 0
    GenerateWarning("aaHint: %d stencil: %d depth: %d alpha: %d premult: %d preserve: %d\n",
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

#ifdef DEBUG
int32_t
WebGLContext::GetWidth() const
{
    return mWidth;
}

int32_t
WebGLContext::GetHeight() const
{
    return mHeight;
}
#endif


















static bool
IsFeatureInBlacklist(const nsCOMPtr<nsIGfxInfo>& gfxInfo, int32_t feature)
{
    int32_t status;
    if (!NS_SUCCEEDED(gfxInfo->GetFeatureStatus(feature, &status)))
        return false;

    return status != nsIGfxInfo::FEATURE_STATUS_OK;
}

static already_AddRefed<GLContext>
CreateHeadlessNativeGL(bool forceEnabled, const nsCOMPtr<nsIGfxInfo>& gfxInfo,
                       WebGLContext* webgl)
{
    if (!forceEnabled &&
        IsFeatureInBlacklist(gfxInfo, nsIGfxInfo::FEATURE_WEBGL_OPENGL))
    {
        webgl->GenerateWarning("Refused to create native OpenGL context"
                               " because of blacklisting.");
        return nullptr;
    }

    nsRefPtr<GLContext> gl = gl::GLContextProvider::CreateHeadless();
    if (!gl) {
        webgl->GenerateWarning("Error during native OpenGL init.");
        return nullptr;
    }
    MOZ_ASSERT(!gl->IsANGLE());

    return gl.forget();
}




static already_AddRefed<GLContext>
CreateHeadlessANGLE(bool forceEnabled, const nsCOMPtr<nsIGfxInfo>& gfxInfo,
                    WebGLContext* webgl)
{
    nsRefPtr<GLContext> gl;

#ifdef XP_WIN
    if (!forceEnabled &&
        IsFeatureInBlacklist(gfxInfo, nsIGfxInfo::FEATURE_WEBGL_ANGLE))
    {
        webgl->GenerateWarning("Refused to create ANGLE OpenGL context"
                               " because of blacklisting.");
        return nullptr;
    }

    gl = gl::GLContextProviderEGL::CreateHeadless();
    if (!gl) {
        webgl->GenerateWarning("Error during ANGLE OpenGL init.");
        return nullptr;
    }
    MOZ_ASSERT(gl->IsANGLE());
#endif

    return gl.forget();
}

static already_AddRefed<GLContext>
CreateHeadlessEGL(bool forceEnabled, const nsCOMPtr<nsIGfxInfo>& gfxInfo,
                  WebGLContext* webgl)
{
    nsRefPtr<GLContext> gl;

#ifdef ANDROID
    gl = gl::GLContextProviderEGL::CreateHeadless();
    if (!gl) {
        webgl->GenerateWarning("Error during EGL OpenGL init.");
        return nullptr;
    }
    MOZ_ASSERT(!gl->IsANGLE());
#endif

    return gl.forget();
}


static already_AddRefed<GLContext>
CreateHeadlessGL(bool forceEnabled, const nsCOMPtr<nsIGfxInfo>& gfxInfo,
                 WebGLContext* webgl)
{
    bool preferEGL = PR_GetEnv("MOZ_WEBGL_PREFER_EGL");
    bool disableANGLE = Preferences::GetBool("webgl.disable-angle", false);

    if (PR_GetEnv("MOZ_WEBGL_FORCE_OPENGL"))
        disableANGLE = true;

    nsRefPtr<GLContext> gl;

    if (preferEGL)
        gl = CreateHeadlessEGL(forceEnabled, gfxInfo, webgl);

    if (!gl && !disableANGLE)
        gl = CreateHeadlessANGLE(forceEnabled, gfxInfo, webgl);

    if (!gl)
        gl = CreateHeadlessNativeGL(forceEnabled, gfxInfo, webgl);

    return gl.forget();
}


static bool
CreateOffscreenWithCaps(GLContext* gl, const SurfaceCaps& caps)
{
    gfx::IntSize dummySize(16, 16);
    return gl->InitOffscreen(dummySize, caps);
}

static void
PopulateCapFallbackQueue(const SurfaceCaps& baseCaps,
                         std::queue<SurfaceCaps>* out_fallbackCaps)
{
    out_fallbackCaps->push(baseCaps);

    
    
    
    if (baseCaps.antialias) {
        SurfaceCaps nextCaps(baseCaps);
        nextCaps.antialias = false;
        PopulateCapFallbackQueue(nextCaps, out_fallbackCaps);
    }

    
    
    
    if (baseCaps.stencil) {
        SurfaceCaps nextCaps(baseCaps);
        nextCaps.stencil = false;
        PopulateCapFallbackQueue(nextCaps, out_fallbackCaps);
    }

    if (baseCaps.depth) {
        SurfaceCaps nextCaps(baseCaps);
        nextCaps.depth = false;
        PopulateCapFallbackQueue(nextCaps, out_fallbackCaps);
    }
}

static bool
CreateOffscreen(GLContext* gl, const WebGLContextOptions& options,
                const nsCOMPtr<nsIGfxInfo>& gfxInfo, WebGLContext* webgl,
                layers::ISurfaceAllocator* surfAllocator)
{
    SurfaceCaps baseCaps;

    baseCaps.color = true;
    baseCaps.alpha = options.alpha;
    baseCaps.antialias = options.antialias;
    baseCaps.depth = options.depth;
    baseCaps.premultAlpha = options.premultipliedAlpha;
    baseCaps.preserve = options.preserveDrawingBuffer;
    baseCaps.stencil = options.stencil;

    if (!baseCaps.alpha)
        baseCaps.premultAlpha = true;

    if (gl->IsANGLE()) {
        
        
        baseCaps.alpha = true;
    }

    
    
    
    baseCaps.bpp16 = Preferences::GetBool("webgl.prefer-16bpp", false);

#ifdef MOZ_WIDGET_GONK
    baseCaps.surfaceAllocator = surfAllocator;
#endif

    

    bool forceAllowAA = Preferences::GetBool("webgl.msaa-force", false);
    if (!forceAllowAA &&
        IsFeatureInBlacklist(gfxInfo, nsIGfxInfo::FEATURE_WEBGL_MSAA))
    {
        webgl->GenerateWarning("Disallowing antialiased backbuffers due"
                               " to blacklisting.");
        baseCaps.antialias = false;
    }

    std::queue<SurfaceCaps> fallbackCaps;
    PopulateCapFallbackQueue(baseCaps, &fallbackCaps);

    bool created = false;
    while (!fallbackCaps.empty()) {
        SurfaceCaps& caps = fallbackCaps.front();

        created = CreateOffscreenWithCaps(gl, caps);
        if (created)
            break;

        fallbackCaps.pop();
    }

    return created;
}

bool
WebGLContext::CreateOffscreenGL(bool forceEnabled)
{
    nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");

    layers::ISurfaceAllocator* surfAllocator = nullptr;
#ifdef MOZ_WIDGET_GONK
    nsIWidget* docWidget = nsContentUtils::WidgetForDocument(mCanvasElement->OwnerDoc());
    if (docWidget) {
        layers::LayerManager* layerManager = docWidget->GetLayerManager();
        if (layerManager) {
            
            layers::ShadowLayerForwarder* forwarder = layerManager->AsShadowForwarder();
            if (forwarder)
                surfAllocator = static_cast<layers::ISurfaceAllocator*>(forwarder);
        }
    }
#endif

    gl = CreateHeadlessGL(forceEnabled, gfxInfo, this);

    do {
        if (!gl)
            break;

        if (!CreateOffscreen(gl, mOptions, gfxInfo, this, surfAllocator))
            break;

        if (!InitAndValidateGL())
            break;

        return true;
    } while (false);

    gl = nullptr;
    return false;
}


bool
WebGLContext::ResizeBackbuffer(uint32_t requestedWidth,
                               uint32_t requestedHeight)
{
    uint32_t width = requestedWidth;
    uint32_t height = requestedHeight;

    bool resized = false;
    while (width || height) {
      width = width ? width : 1;
      height = height ? height : 1;

      gfx::IntSize curSize(width, height);
      if (gl->ResizeOffscreen(curSize)) {
          resized = true;
          break;
      }

      width /= 2;
      height /= 2;
    }

    if (!resized)
        return false;

    mWidth = gl->OffscreenSize().width;
    mHeight = gl->OffscreenSize().height;
    MOZ_ASSERT((uint32_t)mWidth == width);
    MOZ_ASSERT((uint32_t)mHeight == height);

    if (width != requestedWidth ||
        height != requestedHeight)
    {
        GenerateWarning("Requested size %dx%d was too large, but resize"
                          " to %dx%d succeeded.",
                        requestedWidth, requestedHeight,
                        width, height);
    }
    return true;
}

NS_IMETHODIMP
WebGLContext::SetDimensions(int32_t signedWidth, int32_t signedHeight)
{
    
    if (!GetCanvas())
        return NS_ERROR_FAILURE;

    if (signedWidth < 0 || signedHeight < 0) {
        GenerateWarning("Canvas size is too large (seems like a negative value wrapped)");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    uint32_t width = signedWidth;
    uint32_t height = signedHeight;

    
    GetCanvas()->InvalidateCanvas();

    
    if (width == 0)
        width = 1;

    if (height == 0)
        height = 1;

    
    if (gl) {
        if ((uint32_t)mWidth == width &&
            (uint32_t)mHeight == height)
        {
            return NS_OK;
        }

        if (IsContextLost())
            return NS_OK;

        MakeContextCurrent();

        
        PresentScreenBuffer();

        
        if (!ResizeBackbuffer(width, height)) {
            GenerateWarning("WebGL context failed to resize.");
            ForceLoseContext();
            return NS_OK;
        }

        
        mResetLayer = true;
        mBackbufferNeedsClear = true;

        return NS_OK;
    }

    
    
    

    
    
    
    
    
    LoseOldestWebGLContextIfLimitExceeded();

    
    
    
    

    
    
    
    if (!(mGeneration + 1).isValid()) {
        GenerateWarning("Too many WebGL contexts created this run.");
        return NS_ERROR_FAILURE; 
    }

    
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), NS_ERROR_FAILURE);

    bool disabled = Preferences::GetBool("webgl.disabled", false);
    if (disabled) {
        GenerateWarning("WebGL creation is disabled, and so disallowed here.");
        return NS_ERROR_FAILURE;
    }

    
    bool forceEnabled = Preferences::GetBool("webgl.force-enabled", false);
    ScopedGfxFeatureReporter reporter("WebGL", forceEnabled);

    if (!CreateOffscreenGL(forceEnabled)) {
        GenerateWarning("WebGL creation failed.");
        return NS_ERROR_FAILURE;
    }
    MOZ_ASSERT(gl);

    if (!ResizeBackbuffer(width, height)) {
        GenerateWarning("Initializing WebGL backbuffer failed.");
        return NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    if (gl->DebugMode())
        printf_stderr("--- WebGL context created: %p\n", gl.get());
#endif

    mResetLayer = true;
    mOptionsFrozen = true;

    
    ++mGeneration;

    
    if (gl->WorkAroundDriverBugs()) {
        if (!mOptions.alpha && gl->Caps().alpha)
            mNeedsFakeNoAlpha = true;
    }

    
    mOptions.depth = gl->Caps().depth;
    mOptions.stencil = gl->Caps().stencil;
    mOptions.antialias = gl->Caps().antialias;

    MakeContextCurrent();

    gl->fViewport(0, 0, mWidth, mHeight);
    mViewportWidth = mWidth;
    mViewportHeight = mHeight;

    
    
    gl->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

    AssertCachedBindings();
    AssertCachedState();

    
    
    mBackbufferNeedsClear = true;
    ClearBackbufferIfNeeded();

    mShouldPresent = true;

    MOZ_ASSERT(gl->Caps().color);
    MOZ_ASSERT_IF(!mNeedsFakeNoAlpha, gl->Caps().alpha == mOptions.alpha);
    MOZ_ASSERT_IF(mNeedsFakeNoAlpha, !mOptions.alpha && gl->Caps().alpha);
    MOZ_ASSERT(gl->Caps().depth == mOptions.depth);
    MOZ_ASSERT(gl->Caps().stencil == mOptions.stencil);
    MOZ_ASSERT(gl->Caps().antialias == mOptions.antialias);
    MOZ_ASSERT(gl->Caps().preserve == mOptions.preserveDrawingBuffer);

    AssertCachedBindings();
    AssertCachedState();

    reporter.SetSuccessful();
    return NS_OK;
}

void
WebGLContext::ClearBackbufferIfNeeded()
{
    if (!mBackbufferNeedsClear)
        return;

#ifdef DEBUG
    gl->MakeCurrent();

    GLuint fb = 0;
    gl->GetUIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, &fb);
    MOZ_ASSERT(fb == 0);
#endif

    ClearScreen();

    mBackbufferNeedsClear = false;
}

void
WebGLContext::LoseOldestWebGLContextIfLimitExceeded()
{
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    
    const size_t kMaxWebGLContextsPerPrincipal = 2;
    const size_t kMaxWebGLContexts             = 4;
#else
    const size_t kMaxWebGLContextsPerPrincipal = 16;
    const size_t kMaxWebGLContexts             = 32;
#endif
    MOZ_ASSERT(kMaxWebGLContextsPerPrincipal < kMaxWebGLContexts);

    
    
    
    UpdateLastUseIndex();

    WebGLMemoryTracker::ContextsArrayType& contexts = WebGLMemoryTracker::Contexts();

    
    if (contexts.Length() <= kMaxWebGLContextsPerPrincipal)
        return;

    
    
    

    uint64_t oldestIndex = UINT64_MAX;
    uint64_t oldestIndexThisPrincipal = UINT64_MAX;
    const WebGLContext* oldestContext = nullptr;
    const WebGLContext* oldestContextThisPrincipal = nullptr;
    size_t numContexts = 0;
    size_t numContextsThisPrincipal = 0;

    for(size_t i = 0; i < contexts.Length(); ++i) {
        
        if (contexts[i] == this)
            continue;

        if (contexts[i]->IsContextLost())
            continue;

        if (!contexts[i]->GetCanvas()) {
            
            
            
            const_cast<WebGLContext*>(contexts[i])->LoseContext();
            continue;
        }

        numContexts++;
        if (contexts[i]->mLastUseIndex < oldestIndex) {
            oldestIndex = contexts[i]->mLastUseIndex;
            oldestContext = contexts[i];
        }

        nsIPrincipal* ourPrincipal = GetCanvas()->NodePrincipal();
        nsIPrincipal* theirPrincipal = contexts[i]->GetCanvas()->NodePrincipal();
        bool samePrincipal;
        nsresult rv = ourPrincipal->Equals(theirPrincipal, &samePrincipal);
        if (NS_SUCCEEDED(rv) && samePrincipal) {
            numContextsThisPrincipal++;
            if (contexts[i]->mLastUseIndex < oldestIndexThisPrincipal) {
                oldestIndexThisPrincipal = contexts[i]->mLastUseIndex;
                oldestContextThisPrincipal = contexts[i];
            }
        }
    }

    if (numContextsThisPrincipal > kMaxWebGLContextsPerPrincipal) {
        GenerateWarning("Exceeded %d live WebGL contexts for this principal, losing the "
                        "least recently used one.", kMaxWebGLContextsPerPrincipal);
        MOZ_ASSERT(oldestContextThisPrincipal); 
        const_cast<WebGLContext*>(oldestContextThisPrincipal)->LoseContext();
    } else if (numContexts > kMaxWebGLContexts) {
        GenerateWarning("Exceeded %d live WebGL contexts, losing the least recently used one.",
                        kMaxWebGLContexts);
        MOZ_ASSERT(oldestContext); 
        const_cast<WebGLContext*>(oldestContext)->LoseContext();
    }
}

void
WebGLContext::GetImageBuffer(uint8_t** out_imageBuffer, int32_t* out_format)
{
    *out_imageBuffer = nullptr;
    *out_format = 0;

    
    bool premult;
    RefPtr<SourceSurface> snapshot =
      GetSurfaceSnapshot(mOptions.premultipliedAlpha ? nullptr : &premult);
    if (!snapshot)
        return;

    MOZ_ASSERT(mOptions.premultipliedAlpha || !premult, "We must get unpremult when we ask for it!");

    RefPtr<DataSourceSurface> dataSurface = snapshot->GetDataSurface();

    DataSourceSurface::MappedSurface map;
    if (!dataSurface->Map(DataSourceSurface::MapType::READ, &map))
        return;

    static const fallible_t fallible = fallible_t();
    uint8_t* imageBuffer = new (fallible) uint8_t[mWidth * mHeight * 4];
    if (!imageBuffer) {
        dataSurface->Unmap();
        return;
    }
    memcpy(imageBuffer, map.mData, mWidth * mHeight * 4);

    dataSurface->Unmap();

    int32_t format = imgIEncoder::INPUT_FORMAT_HOSTARGB;
    if (!mOptions.premultipliedAlpha) {
        
        
        
        
        
        gfxUtils::ConvertBGRAtoRGBA(imageBuffer, mWidth * mHeight * 4);
        format = imgIEncoder::INPUT_FORMAT_RGBA;
    }

    *out_imageBuffer = imageBuffer;
    *out_format = format;
}

NS_IMETHODIMP
WebGLContext::GetInputStream(const char* mimeType,
                             const char16_t* encoderOptions,
                             nsIInputStream** out_stream)
{
    NS_ASSERTION(gl, "GetInputStream on invalid context?");
    if (!gl)
        return NS_ERROR_FAILURE;

    nsCString enccid("@mozilla.org/image/encoder;2?type=");
    enccid += mimeType;
    nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(enccid.get());
    if (!encoder)
        return NS_ERROR_FAILURE;

    nsAutoArrayPtr<uint8_t> imageBuffer;
    int32_t format = 0;
    GetImageBuffer(getter_Transfers(imageBuffer), &format);
    if (!imageBuffer)
        return NS_ERROR_FAILURE;

    return ImageEncoder::GetInputStream(mWidth, mHeight, imageBuffer, format,
                                        encoder, encoderOptions, out_stream);
}

void
WebGLContext::UpdateLastUseIndex()
{
    static CheckedInt<uint64_t> sIndex = 0;

    sIndex++;

    
    
    if (!sIndex.isValid())
        NS_RUNTIMEABORT("Can't believe it's been 2^64 transactions already!");

    mLastUseIndex = sIndex.value();
}

static uint8_t gWebGLLayerUserData;

namespace mozilla {

class WebGLContextUserData : public LayerUserData
{
public:
    explicit WebGLContextUserData(HTMLCanvasElement* canvas)
        : mCanvas(canvas)
    {}

    


    static void PreTransactionCallback(void* data) {
        WebGLContextUserData* userdata = static_cast<WebGLContextUserData*>(data);
        HTMLCanvasElement* canvas = userdata->mCanvas;
        WebGLContext* webgl = static_cast<WebGLContext*>(canvas->GetContextAtIndex(0));

        
        webgl->PresentScreenBuffer();
        webgl->mDrawCallsSinceLastFlush = 0;
    }

    


    static void DidTransactionCallback(void* data) {
        WebGLContextUserData* userdata = static_cast<WebGLContextUserData*>(data);
        HTMLCanvasElement* canvas = userdata->mCanvas;
        WebGLContext* webgl = static_cast<WebGLContext*>(canvas->GetContextAtIndex(0));

        
        webgl->MarkContextClean();

        webgl->UpdateLastUseIndex();
    }

private:
    nsRefPtr<HTMLCanvasElement> mCanvas;
};

} 

already_AddRefed<layers::CanvasLayer>
WebGLContext::GetCanvasLayer(nsDisplayListBuilder* builder,
                             CanvasLayer* oldLayer,
                             LayerManager* manager)
{
    if (IsContextLost())
        return nullptr;

    if (!mResetLayer && oldLayer &&
        oldLayer->HasUserData(&gWebGLLayerUserData)) {
        nsRefPtr<layers::CanvasLayer> ret = oldLayer;
        return ret.forget();
    }

    nsRefPtr<CanvasLayer> canvasLayer = manager->CreateCanvasLayer();
    if (!canvasLayer) {
        NS_WARNING("CreateCanvasLayer returned null!");
        return nullptr;
    }

    WebGLContextUserData* userData = nullptr;
    if (builder->IsPaintingToWindow()) {
      
      
      
      
      
      

      
      
      
      
      
      userData = new WebGLContextUserData(mCanvasElement);
      canvasLayer->SetDidTransactionCallback(
              WebGLContextUserData::DidTransactionCallback, userData);
      canvasLayer->SetPreTransactionCallback(
              WebGLContextUserData::PreTransactionCallback, userData);
    }
    canvasLayer->SetUserData(&gWebGLLayerUserData, userData);

    CanvasLayer::Data data;
    data.mGLContext = gl;
    data.mSize = nsIntSize(mWidth, mHeight);
    data.mHasAlpha = gl->Caps().alpha;
    data.mIsGLAlphaPremult = IsPremultAlpha() || !data.mHasAlpha;

    canvasLayer->Initialize(data);
    uint32_t flags = gl->Caps().alpha ? 0 : Layer::CONTENT_OPAQUE;
    canvasLayer->SetContentFlags(flags);
    canvasLayer->Updated();

    mResetLayer = false;

    return canvasLayer.forget();
}

void
WebGLContext::GetContextAttributes(Nullable<dom::WebGLContextAttributes>& retval)
{
    retval.SetNull();
    if (IsContextLost())
        return;

    dom::WebGLContextAttributes& result = retval.SetValue();

    result.mAlpha.Construct(mOptions.alpha);
    result.mDepth = mOptions.depth;
    result.mStencil = mOptions.stencil;
    result.mAntialias = mOptions.antialias;
    result.mPremultipliedAlpha = mOptions.premultipliedAlpha;
    result.mPreserveDrawingBuffer = mOptions.preserveDrawingBuffer;
}


NS_IMETHODIMP
WebGLContext::MozGetUnderlyingParamString(uint32_t pname, nsAString& retval)
{
    if (IsContextLost())
        return NS_OK;

    retval.SetIsVoid(true);

    MakeContextCurrent();

    switch (pname) {
    case LOCAL_GL_VENDOR:
    case LOCAL_GL_RENDERER:
    case LOCAL_GL_VERSION:
    case LOCAL_GL_SHADING_LANGUAGE_VERSION:
    case LOCAL_GL_EXTENSIONS:
        {
            const char* s = (const char*)gl->fGetString(pname);
            retval.Assign(NS_ConvertASCIItoUTF16(nsDependentCString(s)));
            break;
        }

    default:
        return NS_ERROR_INVALID_ARG;
    }

    return NS_OK;
}

void
WebGLContext::ClearScreen()
{
    bool colorAttachmentsMask[WebGLContext::kMaxColorAttachments] = {false};

    MakeContextCurrent();
    ScopedBindFramebuffer autoFB(gl, 0);

    GLbitfield clearMask = LOCAL_GL_COLOR_BUFFER_BIT;
    if (mOptions.depth)
        clearMask |= LOCAL_GL_DEPTH_BUFFER_BIT;
    if (mOptions.stencil)
        clearMask |= LOCAL_GL_STENCIL_BUFFER_BIT;

    colorAttachmentsMask[0] = true;

    ForceClearFramebufferWithDefaultValues(clearMask, colorAttachmentsMask);
}

void
WebGLContext::ForceClearFramebufferWithDefaultValues(GLbitfield mask,
                                                     const bool colorAttachmentsMask[kMaxColorAttachments])
{
    MakeContextCurrent();

    bool initializeColorBuffer = 0 != (mask & LOCAL_GL_COLOR_BUFFER_BIT);
    bool initializeDepthBuffer = 0 != (mask & LOCAL_GL_DEPTH_BUFFER_BIT);
    bool initializeStencilBuffer = 0 != (mask & LOCAL_GL_STENCIL_BUFFER_BIT);
    bool drawBuffersIsEnabled = IsExtensionEnabled(WebGLExtensionID::WEBGL_draw_buffers);
    bool shouldOverrideDrawBuffers = false;

    GLenum currentDrawBuffers[WebGLContext::kMaxColorAttachments];

    
    
    AssertCachedState(); 
                         

    
    gl->fDisable(LOCAL_GL_SCISSOR_TEST);

    if (initializeColorBuffer) {

        if (drawBuffersIsEnabled) {

            GLenum drawBuffersCommand[WebGLContext::kMaxColorAttachments] = { LOCAL_GL_NONE };

            for(int32_t i = 0; i < mGLMaxDrawBuffers; i++) {
                GLint temp;
                gl->fGetIntegerv(LOCAL_GL_DRAW_BUFFER0 + i, &temp);
                currentDrawBuffers[i] = temp;

                if (colorAttachmentsMask[i]) {
                    drawBuffersCommand[i] = LOCAL_GL_COLOR_ATTACHMENT0 + i;
                }
                if (currentDrawBuffers[i] != drawBuffersCommand[i])
                    shouldOverrideDrawBuffers = true;
            }
            
            
            if (shouldOverrideDrawBuffers)
                gl->fDrawBuffers(mGLMaxDrawBuffers, drawBuffersCommand);
        }

        gl->fColorMask(1, 1, 1, 1);

        if (mNeedsFakeNoAlpha) {
            gl->fClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        } else {
            gl->fClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    if (initializeDepthBuffer) {
        gl->fDepthMask(1);
        gl->fClearDepth(1.0f);
    }

    if (initializeStencilBuffer) {
        
        
        gl->fStencilMaskSeparate(LOCAL_GL_FRONT, 0xffffffff);
        gl->fStencilMaskSeparate(LOCAL_GL_BACK,  0xffffffff);
        gl->fClearStencil(0);
    }

    if (mRasterizerDiscardEnabled) {
        gl->fDisable(LOCAL_GL_RASTERIZER_DISCARD);
    }

    
    gl->fClear(mask);

    
    if (mScissorTestEnabled)
        gl->fEnable(LOCAL_GL_SCISSOR_TEST);

    if (mRasterizerDiscardEnabled) {
        gl->fEnable(LOCAL_GL_RASTERIZER_DISCARD);
    }

    
    if (initializeColorBuffer) {
        if (shouldOverrideDrawBuffers) {
            gl->fDrawBuffers(mGLMaxDrawBuffers, currentDrawBuffers);
        }

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
        gl->fStencilMaskSeparate(LOCAL_GL_BACK,  mStencilWriteMaskBack);
        gl->fClearStencil(mStencilClearValue);
    }
}



bool
WebGLContext::PresentScreenBuffer()
{
    if (IsContextLost()) {
        return false;
    }

    if (!mShouldPresent) {
        return false;
    }
    MOZ_ASSERT(!mBackbufferNeedsClear);

    gl->MakeCurrent();

    GLScreenBuffer* screen = gl->Screen();
    MOZ_ASSERT(screen);

    if (!screen->PublishFrame(screen->Size())) {
        ForceLoseContext();
        return false;
    }

    if (!mOptions.preserveDrawingBuffer) {
        mBackbufferNeedsClear = true;
    }

    mShouldPresent = false;

    return true;
}

void
WebGLContext::DummyFramebufferOperation(const char* funcName)
{
    FBStatus status = CheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (status != LOCAL_GL_FRAMEBUFFER_COMPLETE) {
        ErrorInvalidFramebufferOperation("%s: incomplete framebuffer",
                                         funcName);
    }
}

static bool
CheckContextLost(GLContext* gl, bool* const out_isGuilty)
{
    MOZ_ASSERT(gl);
    MOZ_ASSERT(out_isGuilty);

    bool isEGL = gl->GetContextType() == gl::GLContextType::EGL;

    GLenum resetStatus = LOCAL_GL_NO_ERROR;
    if (gl->HasRobustness()) {
        gl->MakeCurrent();
        resetStatus = gl->fGetGraphicsResetStatus();
    } else if (isEGL) {
        
        
        
        if (!gl->MakeCurrent(true) && gl->IsContextLost()) {
            resetStatus = LOCAL_GL_UNKNOWN_CONTEXT_RESET_ARB;
        }
    }

    if (resetStatus == LOCAL_GL_NO_ERROR) {
        *out_isGuilty = false;
        return false;
    }

    
    bool isGuilty = true;
    switch (resetStatus) {
    case LOCAL_GL_INNOCENT_CONTEXT_RESET_ARB:
        
        isGuilty = false;
        break;
    case LOCAL_GL_GUILTY_CONTEXT_RESET_ARB:
        NS_WARNING("WebGL content on the page definitely caused the graphics"
                   " card to reset.");
        break;
    case LOCAL_GL_UNKNOWN_CONTEXT_RESET_ARB:
        NS_WARNING("WebGL content on the page might have caused the graphics"
                   " card to reset");
        
        break;
    default:
        MOZ_ASSERT(false, "Unreachable.");
        
        break;
    }

    if (isGuilty) {
        NS_WARNING("WebGL context on this page is considered guilty, and will"
                   " not be restored.");
    }

    *out_isGuilty = isGuilty;
    return true;
}

bool
WebGLContext::TryToRestoreContext()
{
    if (NS_FAILED(SetDimensions(mWidth, mHeight)))
        return false;

    return true;
}

void
WebGLContext::RunContextLossTimer()
{
    mContextLossHandler->RunTimer();
}

class UpdateContextLossStatusTask : public nsRunnable
{
    nsRefPtr<WebGLContext> mWebGL;

public:
    explicit UpdateContextLossStatusTask(WebGLContext* webgl)
        : mWebGL(webgl)
    {
    }

    NS_IMETHOD Run() {
        mWebGL->UpdateContextLossStatus();

        return NS_OK;
    }
};

void
WebGLContext::EnqueueUpdateContextLossStatus()
{
    nsCOMPtr<nsIRunnable> task = new UpdateContextLossStatusTask(this);
    NS_DispatchToCurrentThread(task);
}















void
WebGLContext::UpdateContextLossStatus()
{
    if (!mCanvasElement) {
        
        
        return;
    }
    if (mContextStatus == ContextNotLost) {
        
        

        bool isGuilty = true;
        MOZ_ASSERT(gl); 
        bool isContextLost = CheckContextLost(gl, &isGuilty);

        if (isContextLost) {
            if (isGuilty)
                mAllowContextRestore = false;

            ForceLoseContext();
        }

        
    }

    if (mContextStatus == ContextLostAwaitingEvent) {
        
        

        bool useDefaultHandler;
        nsContentUtils::DispatchTrustedEvent(mCanvasElement->OwnerDoc(),
                                             static_cast<nsIDOMHTMLCanvasElement*>(mCanvasElement),
                                             NS_LITERAL_STRING("webglcontextlost"),
                                             true,
                                             true,
                                             &useDefaultHandler);
        
        mContextStatus = ContextLost;
        
        
        
        if (useDefaultHandler)
            mAllowContextRestore = false;

        
    }

    if (mContextStatus == ContextLost) {
        
        
        

        
        if (!mAllowContextRestore)
            return;

        
        
        if (mLastLossWasSimulated)
            return;

        
        if (mRestoreWhenVisible)
            return;

        ForceRestoreContext();
        return;
    }

    if (mContextStatus == ContextLostAwaitingRestore) {
        

        if (!mAllowContextRestore) {
            
            
            mContextStatus = ContextLost;
            return;
        }

        if (!TryToRestoreContext()) {
            
            mContextLossHandler->RunTimer();
            return;
        }

        
        mContextStatus = ContextNotLost;
        nsContentUtils::DispatchTrustedEvent(mCanvasElement->OwnerDoc(),
                                             static_cast<nsIDOMHTMLCanvasElement*>(mCanvasElement),
                                             NS_LITERAL_STRING("webglcontextrestored"),
                                             true,
                                             true);
        mEmitContextLostErrorOnce = true;
        return;
    }
}

void
WebGLContext::ForceLoseContext(bool simulateLosing)
{
    printf_stderr("WebGL(%p)::ForceLoseContext\n", this);
    MOZ_ASSERT(!IsContextLost());
    mContextStatus = ContextLostAwaitingEvent;
    mContextLostErrorSet = false;

    
    DestroyResourcesAndContext();
    mLastLossWasSimulated = simulateLosing;

    
    
    if (mRestoreWhenVisible && !mLastLossWasSimulated) {
        mContextObserver->RegisterVisibilityChangeEvent();
    }

    
    EnqueueUpdateContextLossStatus();
}

void
WebGLContext::ForceRestoreContext()
{
    printf_stderr("WebGL(%p)::ForceRestoreContext\n", this);
    mContextStatus = ContextLostAwaitingRestore;
    mAllowContextRestore = true; 

    mContextObserver->UnregisterVisibilityChangeEvent();

    
    EnqueueUpdateContextLossStatus();
}

void
WebGLContext::MakeContextCurrent() const
{
    gl->MakeCurrent();
}

mozilla::TemporaryRef<mozilla::gfx::SourceSurface>
WebGLContext::GetSurfaceSnapshot(bool* out_premultAlpha)
{
    if (!gl)
        return nullptr;

    bool hasAlpha = mOptions.alpha;
    SurfaceFormat surfFormat = hasAlpha ? SurfaceFormat::B8G8R8A8
                                        : SurfaceFormat::B8G8R8X8;
    RefPtr<DataSourceSurface> surf;
    surf = Factory::CreateDataSourceSurfaceWithStride(IntSize(mWidth, mHeight),
                                                      surfFormat,
                                                      mWidth * 4);
    if (NS_WARN_IF(!surf)) {
        return nullptr;
    }

    gl->MakeCurrent();
    {
        ScopedBindFramebuffer autoFB(gl, 0);
        ClearBackbufferIfNeeded();
        ReadPixelsIntoDataSurface(gl, surf);
    }

    if (out_premultAlpha) {
        *out_premultAlpha = true;
    }
    bool srcPremultAlpha = mOptions.premultipliedAlpha;
    if (!srcPremultAlpha) {
        if (out_premultAlpha) {
            *out_premultAlpha = false;
        } else {
            gfxUtils::PremultiplyDataSurface(surf, surf);
        }
    }

    RefPtr<DrawTarget> dt =
        Factory::CreateDrawTarget(BackendType::CAIRO,
                                  IntSize(mWidth, mHeight),
                                  SurfaceFormat::B8G8R8A8);

    if (!dt) {
        return nullptr;
    }

    dt->SetTransform(Matrix::Translation(0.0, mHeight).PreScale(1.0, -1.0));

    dt->DrawSurface(surf,
                    Rect(0, 0, mWidth, mHeight),
                    Rect(0, 0, mWidth, mHeight),
                    DrawSurfaceOptions(),
                    DrawOptions(1.0f, CompositionOp::OP_SOURCE));

    return dt->Snapshot();
}

void
WebGLContext::DidRefresh()
{
    if (gl) {
        gl->FlushIfHeavyGLCallsSinceLastFlush();
    }
}

bool
WebGLContext::TexImageFromVideoElement(const TexImageTarget texImageTarget,
                                       GLint level, GLenum internalFormat,
                                       GLenum format, GLenum type,
                                       mozilla::dom::Element& elt)
{
    if (type == LOCAL_GL_HALF_FLOAT_OES)
        type = LOCAL_GL_HALF_FLOAT;

    if (!ValidateTexImageFormatAndType(format, type,
                                       WebGLTexImageFunc::TexImage,
                                       WebGLTexDimensions::Tex2D))
    {
        return false;
    }

    HTMLVideoElement* video = HTMLVideoElement::FromContentOrNull(&elt);
    if (!video)
        return false;

    uint16_t readyState;
    if (NS_SUCCEEDED(video->GetReadyState(&readyState)) &&
        readyState < nsIDOMHTMLMediaElement::HAVE_CURRENT_DATA)
    {
        
        return false;
    }

    
    nsCOMPtr<nsIPrincipal> principal = video->GetCurrentPrincipal();
    if (!principal)
        return false;

    mozilla::layers::ImageContainer* container = video->GetImageContainer();
    if (!container)
        return false;

    if (video->GetCORSMode() == CORS_NONE) {
        bool subsumes;
        nsresult rv = mCanvasElement->NodePrincipal()->Subsumes(principal, &subsumes);
        if (NS_FAILED(rv) || !subsumes) {
            GenerateWarning("It is forbidden to load a WebGL texture from a cross-domain element that has not been validated with CORS. "
                                "See https://developer.mozilla.org/en/WebGL/Cross-Domain_Textures");
            return false;
        }
    }

    gl->MakeCurrent();
    nsRefPtr<mozilla::layers::Image> srcImage = container->LockCurrentImage();
    if (!srcImage)
        return false;

    WebGLTexture* tex = ActiveBoundTextureForTexImageTarget(texImageTarget);

    const WebGLTexture::ImageInfo& info = tex->ImageInfoAt(texImageTarget, 0);
    bool dimensionsMatch = info.Width() == srcImage->GetSize().width &&
                           info.Height() == srcImage->GetSize().height;
    if (!dimensionsMatch) {
        
        gl->fTexImage2D(texImageTarget.get(), level, internalFormat,
                        srcImage->GetSize().width, srcImage->GetSize().height,
                        0, format, type, nullptr);
    }

    bool ok = gl->BlitHelper()->BlitImageToTexture(srcImage.get(),
                                                   srcImage->GetSize(),
                                                   tex->GLName(),
                                                   texImageTarget.get(),
                                                   mPixelStoreFlipY);
    if (ok) {
        TexInternalFormat effectiveInternalFormat =
            EffectiveInternalFormatFromInternalFormatAndType(internalFormat,
                                                             type);
        MOZ_ASSERT(effectiveInternalFormat != LOCAL_GL_NONE);
        tex->SetImageInfo(texImageTarget, level, srcImage->GetSize().width,
                          srcImage->GetSize().height, 1,
                          effectiveInternalFormat,
                          WebGLImageDataStatus::InitializedImageData);
        tex->Bind(TexImageTargetToTexTarget(texImageTarget));
    }

    srcImage = nullptr;
    container->UnlockCurrentImage();
    return ok;
}



WebGLContext::ScopedMaskWorkaround::ScopedMaskWorkaround(WebGLContext& webgl)
    : mWebGL(webgl)
    , mNeedsChange(NeedsChange(webgl))
{
    if (mNeedsChange) {
        mWebGL.gl->fColorMask(mWebGL.mColorWriteMask[0],
                              mWebGL.mColorWriteMask[1],
                              mWebGL.mColorWriteMask[2],
                              false);
    }
}

WebGLContext::ScopedMaskWorkaround::~ScopedMaskWorkaround()
{
    if (mNeedsChange) {
        mWebGL.gl->fColorMask(mWebGL.mColorWriteMask[0],
                              mWebGL.mColorWriteMask[1],
                              mWebGL.mColorWriteMask[2],
                              mWebGL.mColorWriteMask[3]);
    }
}



NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLContext)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(WebGLContext,
  mCanvasElement,
  mExtensions,
  mBound2DTextures,
  mBoundCubeMapTextures,
  mBound3DTextures,
  mBoundArrayBuffer,
  mBoundCopyReadBuffer,
  mBoundCopyWriteBuffer,
  mBoundPixelPackBuffer,
  mBoundPixelUnpackBuffer,
  mBoundTransformFeedbackBuffer,
  mBoundUniformBuffer,
  mCurrentProgram,
  mBoundFramebuffer,
  mBoundRenderbuffer,
  mBoundVertexArray,
  mDefaultVertexArray,
  mActiveOcclusionQuery,
  mActiveTransformFeedbackQuery)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLContext)
    NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
    NS_INTERFACE_MAP_ENTRY(nsIDOMWebGLRenderingContext)
    NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
    
    
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMWebGLRenderingContext)
NS_INTERFACE_MAP_END
