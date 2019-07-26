




#include "WebGLContext.h"
#include "WebGLObjectModel.h"
#include "WebGLExtensions.h"
#include "WebGLContextUtils.h"

#include "AccessCheck.h"
#include "nsIConsoleService.h"
#include "nsServiceManagerUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsError.h"
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
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"

#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ipc/ProcessPriorityManager.h"

#include "Layers.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::ipc;
using namespace mozilla::gfx;
using namespace mozilla::gl;
using namespace mozilla::layers;

NS_IMETHODIMP
WebGLMemoryPressureObserver::Observe(nsISupports* aSubject,
                                     const char* aTopic,
                                     const PRUnichar* aSomeData)
{
    if (strcmp(aTopic, "memory-pressure"))
        return NS_OK;

    bool wantToLoseContext = true;

    if (!mContext->mCanLoseContextInForeground && CurrentProcessIsForeground())
        wantToLoseContext = false;
    else if (!nsCRT::strcmp(aSomeData,
                            NS_LITERAL_STRING("heap-minimize").get()))
        wantToLoseContext = mContext->mLoseContextOnHeapMinimize;

    if (wantToLoseContext)
        mContext->ForceLoseContext();

    return NS_OK;
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

WebGLContextOptions::WebGLContextOptions()
    : alpha(true), depth(true), stencil(false),
      premultipliedAlpha(true), antialias(true),
      preserveDrawingBuffer(false)
{
    
    if (Preferences::GetBool("webgl.default-no-alpha", false))
        alpha = false;
}

WebGLContext::WebGLContext()
    : gl(nullptr)
{
    SetIsDOMBinding();

    mGeneration = 0;
    mInvalidated = false;
    mShouldPresent = true;
    mResetLayer = true;
    mOptionsFrozen = false;

    mActiveTexture = 0;
    mWebGLError = LOCAL_GL_NO_ERROR;
    mPixelStoreFlipY = false;
    mPixelStorePremultiplyAlpha = false;
    mPixelStoreColorspaceConversion = BROWSER_DEFAULT_WEBGL;

    mShaderValidation = true;

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
    
    
    
    mGLMaxVertexAttribs = 0;
    mGLMaxTextureUnits = 0;
    mGLMaxTextureSize = 0;
    mGLMaxCubeMapTextureSize = 0;
    mGLMaxRenderbufferSize = 0;
    mGLMaxTextureImageUnits = 0;
    mGLMaxVertexTextureImageUnits = 0;
    mGLMaxVaryingVectors = 0;
    mGLMaxFragmentUniformVectors = 0;
    mGLMaxVertexUniformVectors = 0;

    
    mPixelStorePackAlignment = 4;
    mPixelStoreUnpackAlignment = 4;

    WebGLMemoryMultiReporterWrapper::AddWebGLContext(this);

    mAllowRestore = true;
    mContextLossTimerRunning = false;
    mDrawSinceContextLossTimerSet = false;
    mContextRestorer = do_CreateInstance("@mozilla.org/timer;1");
    mContextStatus = ContextStable;
    mContextLostErrorSet = false;
    mLoseContextOnHeapMinimize = false;
    mCanLoseContextInForeground = true;

    mAlreadyGeneratedWarnings = 0;
    mAlreadyWarnedAboutFakeVertexAttrib0 = false;

    mLastUseIndex = 0;

    mMinInUseAttribArrayLengthCached = false;
    mMinInUseAttribArrayLength = 0;

    mIsScreenCleared = false;
}

WebGLContext::~WebGLContext()
{
    DestroyResourcesAndContext();
    WebGLMemoryMultiReporterWrapper::RemoveWebGLContext(this);
    TerminateContextLossTimer();
    mContextRestorer = nullptr;
}

JSObject*
WebGLContext::WrapObject(JSContext *cx, JSObject *scope)
{
    return dom::WebGLRenderingContextBinding::Wrap(cx, scope, this);
}

void
WebGLContext::DestroyResourcesAndContext()
{
    if (mMemoryPressureObserver) {
        nsCOMPtr<nsIObserverService> observerService
            = mozilla::services::GetObserverService();
        if (observerService) {
            observerService->RemoveObserver(mMemoryPressureObserver,
                                            "memory-pressure");
        }
        mMemoryPressureObserver = nullptr;
    }

    if (!gl)
        return;

    gl->MakeCurrent();

    mBound2DTextures.Clear();
    mBoundCubeMapTextures.Clear();
    mBoundArrayBuffer = nullptr;
    mBoundElementArrayBuffer = nullptr;
    mCurrentProgram = nullptr;
    mBoundFramebuffer = nullptr;
    mBoundRenderbuffer = nullptr;

    mAttribBuffers.Clear();

    while (!mTextures.isEmpty())
        mTextures.getLast()->DeleteOnce();
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

    if (mBlackTexturesAreInitialized) {
        gl->fDeleteTextures(1, &mBlackTexture2D);
        gl->fDeleteTextures(1, &mBlackTextureCubeMap);
        mBlackTexturesAreInitialized = false;
    }

    if (mFakeVertexAttrib0BufferObject) {
        gl->fDeleteBuffers(1, &mFakeVertexAttrib0BufferObject);
    }

    
    
#ifdef DEBUG
    if (gl->DebugMode()) {
        printf_stderr("--- WebGL context destroyed: %p\n", gl.get());
    }
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
    GetBoolFromPropertyBag(aOptions, "premultipliedAlpha", &newOpts.premultipliedAlpha);
    GetBoolFromPropertyBag(aOptions, "antialias", &newOpts.antialias);
    GetBoolFromPropertyBag(aOptions, "preserveDrawingBuffer", &newOpts.preserveDrawingBuffer);
    GetBoolFromPropertyBag(aOptions, "alpha", &newOpts.alpha);

    
    newOpts.depth |= newOpts.stencil;

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

NS_IMETHODIMP
WebGLContext::SetDimensions(int32_t width, int32_t height)
{
    

    if (width < 0 || height < 0) {
        GenerateWarning("Canvas size is too large (seems like a negative value wrapped)");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!GetCanvas())
        return NS_ERROR_FAILURE;

    

    GetCanvas()->InvalidateCanvas();

    if (gl && mWidth == width && mHeight == height)
        return NS_OK;

    
    if (width == 0 || height == 0) {
        width = 1;
        height = 1;
    }

    
    if (gl) {
        MakeContextCurrent();

        gl->ResizeOffscreen(gfxIntSize(width, height)); 
        

        
        mWidth = gl->OffscreenSize().width;
        mHeight = gl->OffscreenSize().height;
        mResetLayer = true;

        ScopedBindFramebuffer autoFB(gl, 0);
        gl->ClearSafely();
        mShouldPresent = true;

        return NS_OK;
    }

    
    
    

    
    
    
    
    
    LoseOldestWebGLContextIfLimitExceeded();

    
    NS_ENSURE_TRUE(Preferences::GetRootBranch(), NS_ERROR_FAILURE);

#ifdef XP_WIN
    bool preferEGL =
        Preferences::GetBool("webgl.prefer-egl", false);
    bool preferOpenGL =
        Preferences::GetBool("webgl.prefer-native-gl", false);
#endif
    bool forceEnabled =
        Preferences::GetBool("webgl.force-enabled", false);
    bool useMesaLlvmPipe =
        Preferences::GetBool("gfx.prefer-mesa-llvmpipe", false);
    bool disabled =
        Preferences::GetBool("webgl.disabled", false);
    bool prefer16bit =
        Preferences::GetBool("webgl.prefer-16bpp", false);

    ScopedGfxFeatureReporter reporter("WebGL", forceEnabled);

    if (disabled)
        return NS_ERROR_FAILURE;

    
    
    
    

    
    
    
    if (!(mGeneration + 1).isValid())
        return NS_ERROR_FAILURE; 

    SurfaceCaps caps;

    caps.color = true;
    caps.alpha = mOptions.alpha;
    caps.depth = mOptions.depth;
    caps.stencil = mOptions.stencil;

    
    
    
    caps.bpp16 = prefer16bit;

    caps.preserve = mOptions.preserveDrawingBuffer;

    bool forceMSAA =
        Preferences::GetBool("webgl.msaa-force", false);

    int32_t status;
    nsCOMPtr<nsIGfxInfo> gfxInfo = do_GetService("@mozilla.org/gfx/info;1");
    if (mOptions.antialias &&
        gfxInfo &&
        NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_WEBGL_MSAA, &status))) {
        if (status == nsIGfxInfo::FEATURE_NO_INFO || forceMSAA) {
            caps.antialias = true;
        }
    }

#ifdef XP_WIN
    if (PR_GetEnv("MOZ_WEBGL_PREFER_EGL")) {
        preferEGL = true;
    }
#endif

    
    bool useOpenGL = true;

#ifdef XP_WIN
    bool useANGLE = true;
#endif

    if (gfxInfo && !forceEnabled) {
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_WEBGL_OPENGL, &status))) {
            if (status != nsIGfxInfo::FEATURE_NO_INFO) {
                useOpenGL = false;
            }
        }
#ifdef XP_WIN
        if (NS_SUCCEEDED(gfxInfo->GetFeatureStatus(nsIGfxInfo::FEATURE_WEBGL_ANGLE, &status))) {
            if (status != nsIGfxInfo::FEATURE_NO_INFO) {
                useANGLE = false;
            }
        }
#endif
    }

#ifdef XP_WIN
    
    if (useMesaLlvmPipe || PR_GetEnv("MOZ_WEBGL_FORCE_OPENGL")) {
        preferEGL = false;
        useANGLE = false;
        useOpenGL = true;
    }
#endif

    gfxIntSize size(width, height);

#ifdef XP_WIN
    
    if (!gl && (preferEGL || useANGLE) && !preferOpenGL) {
        gl = gl::GLContextProviderEGL::CreateOffscreen(size, caps);
        if (!gl || !InitAndValidateGL()) {
            GenerateWarning("Error during ANGLE OpenGL ES initialization");
            return NS_ERROR_FAILURE;
        }
    }
#endif

    
    if (!gl && useOpenGL) {
        GLContext::ContextFlags flag = useMesaLlvmPipe 
                                       ? GLContext::ContextFlagsMesaLLVMPipe
                                       : GLContext::ContextFlagsNone;
        gl = gl::GLContextProvider::CreateOffscreen(size, caps, flag);
        if (gl && !InitAndValidateGL()) {
            GenerateWarning("Error during %s initialization", 
                            useMesaLlvmPipe ? "Mesa LLVMpipe" : "OpenGL");
            return NS_ERROR_FAILURE;
        }
    }

    if (!gl) {
        GenerateWarning("Can't get a usable WebGL context");
        return NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    if (gl->DebugMode()) {
        printf_stderr("--- WebGL context created: %p\n", gl.get());
    }
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

    
    
    gl->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

    gl->fClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl->fClearDepth(1.0f);
    gl->fClearStencil(0);

    gl->ClearSafely();

    mShouldPresent = true;

    MOZ_ASSERT(gl->Caps().color == caps.color);
    MOZ_ASSERT(gl->Caps().alpha == caps.alpha);
    MOZ_ASSERT(gl->Caps().depth == caps.depth || !gl->Caps().depth);
    MOZ_ASSERT(gl->Caps().stencil == caps.stencil || !gl->Caps().stencil);
    MOZ_ASSERT(gl->Caps().antialias == caps.antialias || !gl->Caps().antialias);
    MOZ_ASSERT(gl->Caps().preserve == caps.preserve);

    reporter.SetSuccessful();
    return NS_OK;
}

NS_IMETHODIMP
WebGLContext::Render(gfxContext *ctx, gfxPattern::GraphicsFilter f, uint32_t aFlags)
{
    if (!gl)
        return NS_OK;

    nsRefPtr<gfxImageSurface> surf = new gfxImageSurface(gfxIntSize(mWidth, mHeight),
                                                         gfxASurface::ImageFormatARGB32);
    if (surf->CairoStatus() != 0)
        return NS_ERROR_FAILURE;

    gl->ReadPixelsIntoImageSurface(surf);

    bool srcPremultAlpha = mOptions.premultipliedAlpha;
    bool dstPremultAlpha = aFlags & RenderFlagPremultAlpha;

    if (!srcPremultAlpha && dstPremultAlpha) {
        gfxUtils::PremultiplyImageSurface(surf);
    } else if (srcPremultAlpha && !dstPremultAlpha) {
        gfxUtils::UnpremultiplyImageSurface(surf);
    }

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

void WebGLContext::LoseOldestWebGLContextIfLimitExceeded()
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

    WebGLMemoryMultiReporterWrapper::ContextsArrayType &contexts
      = WebGLMemoryMultiReporterWrapper::Contexts();

    
    if (contexts.Length() <= kMaxWebGLContextsPerPrincipal) {
        return;
    }

    
    
    

    uint64_t oldestIndex = UINT64_MAX;
    uint64_t oldestIndexThisPrincipal = UINT64_MAX;
    const WebGLContext *oldestContext = nullptr;
    const WebGLContext *oldestContextThisPrincipal = nullptr;
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

        nsIPrincipal *ourPrincipal = GetCanvas()->NodePrincipal();
        nsIPrincipal *theirPrincipal = contexts[i]->GetCanvas()->NodePrincipal();
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
    
    uint32_t flags = mOptions.premultipliedAlpha ? RenderFlagPremultAlpha : 0;
    nsresult rv = Render(tmpcx, gfxPattern::FILTER_NEAREST, flags);
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

    int format = imgIEncoder::INPUT_FORMAT_HOSTARGB;
    if (!mOptions.premultipliedAlpha) {
        
        
        
        
        
        gfxUtils::ConvertBGRAtoRGBA(surf);
        format = imgIEncoder::INPUT_FORMAT_RGBA;
    }

    rv = encoder->InitFromData(surf->Data(),
                               mWidth * mHeight * 4,
                               mWidth, mHeight,
                               surf->Stride(),
                               format,
                               nsDependentString(aEncoderOptions));
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(encoder, aStream);
}

NS_IMETHODIMP
WebGLContext::GetThebesSurface(gfxASurface **surface)
{
    return NS_ERROR_NOT_AVAILABLE;
}

void WebGLContext::UpdateLastUseIndex()
{
    static CheckedInt<uint64_t> sIndex = 0;

    sIndex++;

    
    
    if (!sIndex.isValid()) {
        NS_RUNTIMEABORT("Can't believe it's been 2^64 transactions already!");
    }

    mLastUseIndex = sIndex.value();
}

static uint8_t gWebGLLayerUserData;

namespace mozilla {

class WebGLContextUserData : public LayerUserData {
public:
    WebGLContextUserData(HTMLCanvasElement *aContent)
        : mContent(aContent)
    {}

    


    static void PreTransactionCallback(void* data)
    {
        WebGLContextUserData* userdata = static_cast<WebGLContextUserData*>(data);
        HTMLCanvasElement* canvas = userdata->mContent;
        WebGLContext* context = static_cast<WebGLContext*>(canvas->GetContextAtIndex(0));

        
        context->PresentScreenBuffer();
    }

    


    static void DidTransactionCallback(void* aData)
    {
        WebGLContextUserData *userdata = static_cast<WebGLContextUserData*>(aData);
        HTMLCanvasElement *canvas = userdata->mContent;
        WebGLContext *context = static_cast<WebGLContext*>(canvas->GetContextAtIndex(0));

        
        context->MarkContextClean();

        context->UpdateLastUseIndex();
    }

private:
    nsRefPtr<HTMLCanvasElement> mContent;
};

} 

already_AddRefed<layers::CanvasLayer>
WebGLContext::GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                             CanvasLayer *aOldLayer,
                             LayerManager *aManager)
{
    if (!IsContextStable())
        return nullptr;

    if (!mResetLayer && aOldLayer &&
        aOldLayer->HasUserData(&gWebGLLayerUserData)) {
        NS_ADDREF(aOldLayer);
        return aOldLayer;
    }

    nsRefPtr<CanvasLayer> canvasLayer = aManager->CreateCanvasLayer();
    if (!canvasLayer) {
        NS_WARNING("CreateCanvasLayer returned null!");
        return nullptr;
    }
    WebGLContextUserData *userData = nullptr;
    if (aBuilder->IsPaintingToWindow()) {
      
      
      
      
      
      

      
      
      
      
      
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
    data.mIsGLAlphaPremult = IsPremultAlpha();

    canvasLayer->Initialize(data);
    uint32_t flags = gl->Caps().alpha ? 0 : Layer::CONTENT_OPAQUE;
    canvasLayer->SetContentFlags(flags);
    canvasLayer->Updated();

    mResetLayer = false;

    return canvasLayer.forget().get();
}

void
WebGLContext::GetContextAttributes(Nullable<dom::WebGLContextAttributesInitializer> &retval)
{
    retval.SetNull();
    if (!IsContextStable())
        return;

    dom::WebGLContextAttributes& result = retval.SetValue();

    const PixelBufferFormat& format = gl->GetPixelFormat();

    result.mAlpha = format.alpha > 0;
    result.mDepth = format.depth > 0;
    result.mStencil = format.stencil > 0;
    result.mAntialias = format.samples > 1;
    result.mPremultipliedAlpha = mOptions.premultipliedAlpha;
    result.mPreserveDrawingBuffer = mOptions.preserveDrawingBuffer;
}

bool
WebGLContext::IsExtensionEnabled(WebGLExtensionID ext) const {
    return mExtensions.SafeElementAt(ext);
}


NS_IMETHODIMP
WebGLContext::MozGetUnderlyingParamString(uint32_t pname, nsAString& retval)
{
    if (!IsContextStable())
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

bool WebGLContext::IsExtensionSupported(JSContext *cx, WebGLExtensionID ext) const
{
    if (mDisableExtensions) {
        return false;
    }

    switch (ext) {
        case OES_standard_derivatives:
        case WEBGL_lose_context:
            
            return true;
        case OES_texture_float:
            return gl->IsExtensionSupported(gl->IsGLES2() ? GLContext::OES_texture_float
                                                          : GLContext::ARB_texture_float);
        case EXT_texture_filter_anisotropic:
            return gl->IsExtensionSupported(GLContext::EXT_texture_filter_anisotropic);
        case WEBGL_compressed_texture_s3tc:
            if (gl->IsExtensionSupported(GLContext::EXT_texture_compression_s3tc)) {
                return true;
            }
            else if (gl->IsExtensionSupported(GLContext::EXT_texture_compression_dxt1) &&
                       gl->IsExtensionSupported(GLContext::ANGLE_texture_compression_dxt3) &&
                       gl->IsExtensionSupported(GLContext::ANGLE_texture_compression_dxt5))
            {
                return true;
            }
            else
            {
                return false;
            }
        case WEBGL_compressed_texture_atc:
            return gl->IsExtensionSupported(GLContext::AMD_compressed_ATC_texture);
        case WEBGL_compressed_texture_pvrtc:
            return gl->IsExtensionSupported(GLContext::IMG_texture_compression_pvrtc);
        case WEBGL_depth_texture:
            if (gl->IsGLES2() && 
                gl->IsExtensionSupported(GLContext::OES_packed_depth_stencil) &&
                gl->IsExtensionSupported(GLContext::OES_depth_texture)) 
            {
                return true;
            }
            else if (!gl->IsGLES2() &&
                     gl->IsExtensionSupported(GLContext::EXT_packed_depth_stencil))
            {
                return true;
            }
            else
            {
                return false;
            }
        case WEBGL_debug_renderer_info:
            return xpc::AccessCheck::isChrome(js::GetContextCompartment(cx));
        default:
            MOZ_ASSERT(false, "should not get there.");
    }

    MOZ_ASSERT(false, "should not get there.");
    return false;
}

static bool
CompareWebGLExtensionName(const nsACString& name, const char *other)
{
    return name.Equals(other, nsCaseInsensitiveCStringComparator());
}

JSObject*
WebGLContext::GetExtension(JSContext *cx, const nsAString& aName, ErrorResult& rv)
{
    if (!IsContextStable())
        return nullptr;

    NS_LossyConvertUTF16toASCII name(aName);

    WebGLExtensionID ext = WebGLExtensionID_unknown_extension;

    
    if (CompareWebGLExtensionName(name, "OES_texture_float"))
    {
        ext = OES_texture_float;
    }
    else if (CompareWebGLExtensionName(name, "OES_standard_derivatives"))
    {
        ext = OES_standard_derivatives;
    }
    else if (CompareWebGLExtensionName(name, "EXT_texture_filter_anisotropic"))
    {
        ext = EXT_texture_filter_anisotropic;
    }
    else if (CompareWebGLExtensionName(name, "MOZ_WEBGL_lose_context"))
    {
        ext = WEBGL_lose_context;
    }
    else if (CompareWebGLExtensionName(name, "WEBGL_lose_context"))
    {
        ext = WEBGL_lose_context;
    }
    else if (CompareWebGLExtensionName(name, "MOZ_WEBGL_compressed_texture_s3tc"))
    {
        ext = WEBGL_compressed_texture_s3tc;
    }
    else if (CompareWebGLExtensionName(name, "WEBGL_compressed_texture_s3tc"))
    {
        ext = WEBGL_compressed_texture_s3tc;
    }
    else if (CompareWebGLExtensionName(name, "MOZ_WEBGL_compressed_texture_atc"))
    {
        ext = WEBGL_compressed_texture_atc;
    }
    else if (CompareWebGLExtensionName(name, "MOZ_WEBGL_compressed_texture_pvrtc"))
    {
        ext = WEBGL_compressed_texture_pvrtc;
    }
    else if (CompareWebGLExtensionName(name, "WEBGL_debug_renderer_info"))
    {
        ext = WEBGL_debug_renderer_info;
    }
    else if (CompareWebGLExtensionName(name, "MOZ_WEBGL_depth_texture"))
    {
        ext = WEBGL_depth_texture;
    }
    else if (CompareWebGLExtensionName(name, "WEBGL_depth_texture"))
    {
        ext = WEBGL_depth_texture;
    }

    if (ext == WebGLExtensionID_unknown_extension) {
      return nullptr;
    }

    
    if (!IsExtensionSupported(cx, ext)) {
        return nullptr;
    }

    
    if (!IsExtensionEnabled(ext)) {
        WebGLExtensionBase *obj = nullptr;
        switch (ext) {
            case OES_standard_derivatives:
                obj = new WebGLExtensionStandardDerivatives(this);
                break;
            case EXT_texture_filter_anisotropic:
                obj = new WebGLExtensionTextureFilterAnisotropic(this);
                break;
            case WEBGL_lose_context:
                obj = new WebGLExtensionLoseContext(this);
                break;
            case WEBGL_compressed_texture_s3tc:
                obj = new WebGLExtensionCompressedTextureS3TC(this);
                break;
            case WEBGL_compressed_texture_atc:
                obj = new WebGLExtensionCompressedTextureATC(this);
                break;
            case WEBGL_compressed_texture_pvrtc:
                obj = new WebGLExtensionCompressedTexturePVRTC(this);
                break;
            case WEBGL_debug_renderer_info:
                obj = new WebGLExtensionDebugRendererInfo(this);
                break;
            case WEBGL_depth_texture:
                obj = new WebGLExtensionDepthTexture(this);
                break;
            case OES_texture_float:
                obj = new WebGLExtensionTextureFloat(this);
                break;
            default:
                MOZ_ASSERT(false, "should not get there.");
        }
        mExtensions.EnsureLengthAtLeast(ext + 1);
        mExtensions[ext] = obj;
    }

    return WebGLObjectAsJSObject(cx, mExtensions[ext].get(), rv);
}

void
WebGLContext::ClearScreen()
{
    MakeContextCurrent();
    ScopedBindFramebuffer autoFB(gl, 0);

    GLbitfield clearMask = LOCAL_GL_COLOR_BUFFER_BIT;
    if (mOptions.depth)
        clearMask |= LOCAL_GL_DEPTH_BUFFER_BIT;
    if (mOptions.stencil)
        clearMask |= LOCAL_GL_STENCIL_BUFFER_BIT;

    ForceClearFramebufferWithDefaultValues(clearMask);
    mIsScreenCleared = true;
}

void
WebGLContext::ForceClearFramebufferWithDefaultValues(GLbitfield mask)
{
    MakeContextCurrent();

    bool initializeColorBuffer = 0 != (mask & LOCAL_GL_COLOR_BUFFER_BIT);
    bool initializeDepthBuffer = 0 != (mask & LOCAL_GL_DEPTH_BUFFER_BIT);
    bool initializeStencilBuffer = 0 != (mask & LOCAL_GL_STENCIL_BUFFER_BIT);

    
    

#ifdef DEBUG
    
    {
        
        

        
        MOZ_ASSERT(gl->fIsEnabled(LOCAL_GL_SCISSOR_TEST) == mScissorTestEnabled);

        realGLboolean colorWriteMask[4] = {2, 2, 2, 2};
        GLfloat colorClearValue[4] = {-1.0f, -1.0f, -1.0f, -1.0f};

        gl->fGetBooleanv(LOCAL_GL_COLOR_WRITEMASK, colorWriteMask);
        gl->fGetFloatv(LOCAL_GL_COLOR_CLEAR_VALUE, colorClearValue);

        MOZ_ASSERT(colorWriteMask[0] == mColorWriteMask[0] &&
                   colorWriteMask[1] == mColorWriteMask[1] &&
                   colorWriteMask[2] == mColorWriteMask[2] &&
                   colorWriteMask[3] == mColorWriteMask[3]);
        MOZ_ASSERT(colorClearValue[0] == mColorClearValue[0] &&
                   colorClearValue[1] == mColorClearValue[1] &&
                   colorClearValue[2] == mColorClearValue[2] &&
                   colorClearValue[3] == mColorClearValue[3]);


        realGLboolean depthWriteMask = 2;
        GLfloat depthClearValue = -1.0f;

        gl->fGetBooleanv(LOCAL_GL_DEPTH_WRITEMASK, &depthWriteMask);
        gl->fGetFloatv(LOCAL_GL_DEPTH_CLEAR_VALUE, &depthClearValue);

        MOZ_ASSERT(depthWriteMask  == mDepthWriteMask);
        MOZ_ASSERT(depthClearValue == mDepthClearValue);


        GLuint stencilWriteMaskFront = 0xdeadbad1;
        GLuint stencilWriteMaskBack  = 0xdeadbad1;
        GLuint stencilClearValue     = 0xdeadbad1;

        gl->GetUIntegerv(LOCAL_GL_STENCIL_WRITEMASK,      &stencilWriteMaskFront);
        gl->GetUIntegerv(LOCAL_GL_STENCIL_BACK_WRITEMASK, &stencilWriteMaskBack);
        gl->GetUIntegerv(LOCAL_GL_STENCIL_CLEAR_VALUE,    &stencilClearValue);

        GLuint stencilBits = 0;
        gl->GetUIntegerv(LOCAL_GL_STENCIL_BITS, &stencilBits);
        GLuint stencilMask = (GLuint(1) << stencilBits) - 1;

        MOZ_ASSERT( ( stencilWriteMaskFront & stencilMask) ==
                    (mStencilWriteMaskFront & stencilMask) );
        MOZ_ASSERT( ( stencilWriteMaskBack & stencilMask) ==
                    (mStencilWriteMaskBack & stencilMask) );
        MOZ_ASSERT( ( stencilClearValue & stencilMask) ==
                    (mStencilClearValue & stencilMask) );
    }
#endif

    
    gl->fDisable(LOCAL_GL_SCISSOR_TEST);

    if (initializeColorBuffer) {
        gl->fColorMask(1, 1, 1, 1);
        gl->fClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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

    
    gl->fClear(mask);

    
    if (mScissorTestEnabled)
        gl->fEnable(LOCAL_GL_SCISSOR_TEST);

    
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
        gl->fStencilMaskSeparate(LOCAL_GL_BACK,  mStencilWriteMaskBack);
        gl->fClearStencil(mStencilClearValue);
    }
}



bool
WebGLContext::PresentScreenBuffer()
{
    if (!mShouldPresent) {
        return false;
    }

    gl->MakeCurrent();
    if (!gl->PublishFrame()) {
        this->ForceLoseContext();
        return false;
    }

    if (!mOptions.preserveDrawingBuffer) {
        ClearScreen();
    }

    mShouldPresent = false;

    return true;
}

void
WebGLContext::DummyFramebufferOperation(const char *info)
{
    WebGLenum status = CheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (status == LOCAL_GL_FRAMEBUFFER_COMPLETE)
        return;
    else
        return ErrorInvalidFramebufferOperation("%s: incomplete framebuffer", info);
}















void
WebGLContext::RobustnessTimerCallback(nsITimer* timer)
{
    TerminateContextLossTimer();

    if (!mCanvasElement) {
        
        
        return;
    }

    
    
    if (mContextStatus == ContextLostAwaitingEvent) {
        bool defaultAction;
        nsContentUtils::DispatchTrustedEvent(mCanvasElement->OwnerDoc(),
                                             static_cast<nsIDOMHTMLCanvasElement*>(mCanvasElement),
                                             NS_LITERAL_STRING("webglcontextlost"),
                                             true,
                                             true,
                                             &defaultAction);

        
        if (defaultAction)
            mAllowRestore = false;

        
        
        
        if (!defaultAction && mAllowRestore) {
            ForceRestoreContext();
            
            
            SetupContextLossTimer();
        } else {
            mContextStatus = ContextLost;
        }
    } else if (mContextStatus == ContextLostAwaitingRestore) {
        
        if (NS_FAILED(SetDimensions(mWidth, mHeight))) {
            SetupContextLossTimer();
            return;
        }
        mContextStatus = ContextStable;
        nsContentUtils::DispatchTrustedEvent(mCanvasElement->OwnerDoc(),
                                             static_cast<nsIDOMHTMLCanvasElement*>(mCanvasElement),
                                             NS_LITERAL_STRING("webglcontextrestored"),
                                             true,
                                             true);
        
        
        mContextLostErrorSet = false;
        mAllowRestore = true;
    }

    MaybeRestoreContext();
    return;
}

void
WebGLContext::MaybeRestoreContext()
{
    
    if (mContextStatus != ContextStable || gl == nullptr)
        return;

    bool isEGL = gl->GetContextType() == GLContext::ContextTypeEGL,
         isANGLE = gl->IsANGLE();

    GLContext::ContextResetARB resetStatus = GLContext::CONTEXT_NO_ERROR;
    if (mHasRobustness) {
        gl->MakeCurrent();
        resetStatus = (GLContext::ContextResetARB) gl->fGetGraphicsResetStatus();
    } else if (isEGL) {
        
        
        
        
        if (!gl->MakeCurrent(true) && gl->IsContextLost()) {
            resetStatus = GLContext::CONTEXT_GUILTY_CONTEXT_RESET_ARB;
        }
    }
    
    if (resetStatus != GLContext::CONTEXT_NO_ERROR) {
        
        
        ForceLoseContext();
    }

    switch (resetStatus) {
        case GLContext::CONTEXT_NO_ERROR:
            
            
            
            if (mDrawSinceContextLossTimerSet)
                SetupContextLossTimer();
            break;
        case GLContext::CONTEXT_GUILTY_CONTEXT_RESET_ARB:
            NS_WARNING("WebGL content on the page caused the graphics card to reset; not restoring the context");
            mAllowRestore = false;
            break;
        case GLContext::CONTEXT_INNOCENT_CONTEXT_RESET_ARB:
            break;
        case GLContext::CONTEXT_UNKNOWN_CONTEXT_RESET_ARB:
            NS_WARNING("WebGL content on the page might have caused the graphics card to reset");
            if (isEGL && isANGLE) {
                
                
                
                
                mAllowRestore = false;
            }
            break;
    }
}

void
WebGLContext::ForceLoseContext()
{
    if (mContextStatus == ContextLostAwaitingEvent)
        return;

    mContextStatus = ContextLostAwaitingEvent;
    
    SetupContextLossTimer();
    DestroyResourcesAndContext();
}

void
WebGLContext::ForceRestoreContext()
{
    mContextStatus = ContextLostAwaitingRestore;
}

void
WebGLContext::GetSupportedExtensions(JSContext *cx, Nullable< nsTArray<nsString> > &retval)
{
    retval.SetNull();
    if (!IsContextStable())
        return;

    nsTArray<nsString>& arr = retval.SetValue();

    if (IsExtensionSupported(cx, OES_texture_float))
        arr.AppendElement(NS_LITERAL_STRING("OES_texture_float"));
    if (IsExtensionSupported(cx, OES_standard_derivatives))
        arr.AppendElement(NS_LITERAL_STRING("OES_standard_derivatives"));
    if (IsExtensionSupported(cx, EXT_texture_filter_anisotropic))
        arr.AppendElement(NS_LITERAL_STRING("EXT_texture_filter_anisotropic"));
    if (IsExtensionSupported(cx, WEBGL_lose_context))
        arr.AppendElement(NS_LITERAL_STRING("MOZ_WEBGL_lose_context"));
    if (IsExtensionSupported(cx, WEBGL_lose_context))
        arr.AppendElement(NS_LITERAL_STRING("WEBGL_lose_context"));
    if (IsExtensionSupported(cx, WEBGL_compressed_texture_s3tc))
        arr.AppendElement(NS_LITERAL_STRING("MOZ_WEBGL_compressed_texture_s3tc"));
    if (IsExtensionSupported(cx, WEBGL_compressed_texture_s3tc))
        arr.AppendElement(NS_LITERAL_STRING("WEBGL_compressed_texture_s3tc"));
    if (IsExtensionSupported(cx, WEBGL_compressed_texture_atc))
        arr.AppendElement(NS_LITERAL_STRING("MOZ_WEBGL_compressed_texture_atc"));
    if (IsExtensionSupported(cx, WEBGL_compressed_texture_pvrtc))
        arr.AppendElement(NS_LITERAL_STRING("MOZ_WEBGL_compressed_texture_pvrtc"));
    if (IsExtensionSupported(cx, WEBGL_debug_renderer_info))
        arr.AppendElement(NS_LITERAL_STRING("WEBGL_debug_renderer_info"));
    if (IsExtensionSupported(cx, WEBGL_depth_texture))
        arr.AppendElement(NS_LITERAL_STRING("MOZ_WEBGL_depth_texture"));
    if (IsExtensionSupported(cx, WEBGL_depth_texture))
        arr.AppendElement(NS_LITERAL_STRING("WEBGL_depth_texture"));
}





NS_IMPL_CYCLE_COLLECTING_ADDREF(WebGLContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(WebGLContext)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_10(WebGLContext,
  mCanvasElement,
  mExtensions,
  mBound2DTextures,
  mBoundCubeMapTextures,
  mBoundArrayBuffer,
  mBoundElementArrayBuffer,
  mCurrentProgram,
  mBoundFramebuffer,
  mBoundRenderbuffer,
  mAttribBuffers)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(WebGLContext)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMWebGLRenderingContext)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  
  
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports,
                                   nsICanvasRenderingContextInternal)
NS_INTERFACE_MAP_END
