




#include "LayerManagerOGL.h"
#include <stddef.h>                     
#include <stdint.h>                     
#include "CanvasLayerOGL.h"             
#include "ColorLayerOGL.h"              
#include "Composer2D.h"                 
#include "ContainerLayerOGL.h"          
#include "FPSCounter.h"                 
#include "GLContext.h"                  
#include "GLContextProvider.h"          
#include "GeckoProfiler.h"              
#include "ImageLayerOGL.h"              
#include "ImageLayers.h"                
#include "ThebesLayerOGL.h"             
#include "gfx3DMatrix.h"                
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "gfxCrashReporterUtils.h"      
#include "gfxImageSurface.h"            
#include "gfxPlatform.h"                
#include "gfxRect.h"                    
#include "gfxUtils.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/TimeStamp.h"          
#include "mozilla/Util.h"               
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/mozalloc.h"           
#include "nsIConsoleService.h"          
#include "nsIWidget.h"                  
#include "nsLiteralString.h"            
#include "nsPoint.h"                    
#include "nsServiceManagerUtils.h"      
#include "nsString.h"                   
#include "LayerManagerOGLProgram.h"     
#ifdef XP_WIN
#include "prenv.h"                      
#endif
#ifdef MOZ_WIDGET_ANDROID
#include <android/log.h>
#include "TexturePoolOGL.h"
#endif
#ifdef XP_MACOSX
#include "gfxPlatformMac.h"
#endif

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;
using namespace mozilla::gl;

int32_t
LayerManagerOGL::GetMaxTextureSize() const
{
  int32_t maxSize;
  mGLContext->MakeCurrent();
  mGLContext->fGetIntegerv(LOCAL_GL_MAX_TEXTURE_SIZE, &maxSize);
  return maxSize;
}

void
LayerManagerOGL::MakeCurrent(bool aForce)
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }
  mGLContext->MakeCurrent(aForce);
}

void*
LayerManagerOGL::GetNSOpenGLContext() const
{
  return gl()->GetNativeData(GLContext::NativeGLContext);
}


void
LayerManagerOGL::BindQuadVBO() {
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mQuadVBO);
}

void
LayerManagerOGL::QuadVBOVerticesAttrib(GLuint aAttribIndex) {
  mGLContext->fVertexAttribPointer(aAttribIndex, 2,
                                   LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                   (GLvoid*) QuadVBOVertexOffset());
}

void
LayerManagerOGL::QuadVBOTexCoordsAttrib(GLuint aAttribIndex) {
  mGLContext->fVertexAttribPointer(aAttribIndex, 2,
                                   LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                   (GLvoid*) QuadVBOTexCoordOffset());
}

void
LayerManagerOGL::QuadVBOFlippedTexCoordsAttrib(GLuint aAttribIndex) {
  mGLContext->fVertexAttribPointer(aAttribIndex, 2,
                                   LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                   (GLvoid*) QuadVBOFlippedTexCoordOffset());
}



void
LayerManagerOGL::BindAndDrawQuad(GLuint aVertAttribIndex,
                     GLuint aTexCoordAttribIndex,
                     bool aFlipped)
{
  BindQuadVBO();
  QuadVBOVerticesAttrib(aVertAttribIndex);

  if (aTexCoordAttribIndex != GLuint(-1)) {
    if (aFlipped)
      QuadVBOFlippedTexCoordsAttrib(aTexCoordAttribIndex);
    else
      QuadVBOTexCoordsAttrib(aTexCoordAttribIndex);

    mGLContext->fEnableVertexAttribArray(aTexCoordAttribIndex);
  }

  mGLContext->fEnableVertexAttribArray(aVertAttribIndex);

  mGLContext->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);

  mGLContext->fDisableVertexAttribArray(aVertAttribIndex);

  if (aTexCoordAttribIndex != GLuint(-1)) {
    mGLContext->fDisableVertexAttribArray(aTexCoordAttribIndex);
  }
}




LayerManagerOGL::LayerManagerOGL(nsIWidget *aWidget, int aSurfaceWidth, int aSurfaceHeight,
                                 bool aIsRenderingToEGLSurface)
  : mWidget(aWidget)
  , mWidgetSize(-1, -1)
  , mSurfaceSize(aSurfaceWidth, aSurfaceHeight)
  , mBackBufferFBO(0)
  , mBackBufferTexture(0)
  , mBackBufferSize(-1, -1)
  , mHasBGRA(0)
  , mIsRenderingToEGLSurface(aIsRenderingToEGLSurface)
#ifdef DEBUG
  , mMaybeInvalidTree(false)
#endif
{
}

LayerManagerOGL::~LayerManagerOGL()
{
  Destroy();
}

void
LayerManagerOGL::Destroy()
{
  if (mDestroyed)
    return;

  if (mRoot) {
    RootLayer()->Destroy();
    mRoot = nullptr;
  }

  mWidget->CleanupWindowEffects();

  if (!mGLContext)
    return;

  nsRefPtr<GLContext> ctx = mGLContext->GetSharedContext();
  if (!ctx) {
    ctx = mGLContext;
  }

  ctx->MakeCurrent();

  for (uint32_t i = 0; i < mPrograms.Length(); ++i) {
    for (uint32_t type = MaskNone; type < NumMaskTypes; ++type) {
      delete mPrograms[i].mVariations[type];
    }
  }
  mPrograms.Clear();

  ctx->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

  if (mBackBufferFBO) {
    ctx->fDeleteFramebuffers(1, &mBackBufferFBO);
    mBackBufferFBO = 0;
  }

  if (mBackBufferTexture) {
    ctx->fDeleteTextures(1, &mBackBufferTexture);
    mBackBufferTexture = 0;
  }

  if (mQuadVBO) {
    ctx->fDeleteBuffers(1, &mQuadVBO);
    mQuadVBO = 0;
  }

  mGLContext = nullptr;

  mDestroyed = true;
}

already_AddRefed<mozilla::gl::GLContext>
LayerManagerOGL::CreateContext()
{
  nsRefPtr<GLContext> context;

#ifdef XP_WIN
  if (PR_GetEnv("MOZ_LAYERS_PREFER_EGL")) {
    printf_stderr("Trying GL layers...\n");
    context = gl::GLContextProviderEGL::CreateForWindow(mWidget);
  }
#endif

  if (!context)
    context = gl::GLContextProvider::CreateForWindow(mWidget);

  if (!context) {
    NS_WARNING("Failed to create LayerManagerOGL context");
  }
  return context.forget();
}

void
LayerManagerOGL::AddPrograms(ShaderProgramType aType)
{
  for (uint32_t maskType = MaskNone; maskType < NumMaskTypes; ++maskType) {
    if (ProgramProfileOGL::ProgramExists(aType, static_cast<MaskType>(maskType))) {
      mPrograms[aType].mVariations[maskType] = new ShaderProgramOGL(this->gl(),
        ProgramProfileOGL::GetProfileFor(aType, static_cast<MaskType>(maskType)));
    } else {
      mPrograms[aType].mVariations[maskType] = nullptr;
    }
  }
}


NS_IMETHODIMP
LayerManagerOGL::ReadDrawFPSPref::Run()
{
  
  Preferences::AddBoolVarCache(&sDrawFPS, "layers.acceleration.draw-fps");
  return NS_OK;
}

bool
LayerManagerOGL::Initialize(bool force)
{
  ScopedGfxFeatureReporter reporter("GL Layers", force);

  
  NS_ABORT_IF_FALSE(mGLContext == nullptr, "Don't reinitialize layer managers");

  nsRefPtr<GLContext> ctx = CreateContext();

#ifdef MOZ_WIDGET_ANDROID
  if (!ctx) {
    NS_RUNTIMEABORT("We need a context on Android");
  }
#endif

  if (!ctx) {
    return false;
  }

  mGLContext = ctx;
  mGLContext->SetFlipped(true);

  MakeCurrent();

  mHasBGRA =
    mGLContext->IsExtensionSupported(gl::GLContext::EXT_texture_format_BGRA8888) ||
    mGLContext->IsExtensionSupported(gl::GLContext::EXT_bgra);

  mGLContext->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE_MINUS_SRC_ALPHA,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
  mGLContext->fEnable(LOCAL_GL_BLEND);

  mPrograms.AppendElements(NumProgramTypes);
  for (int type = 0; type < NumProgramTypes; ++type) {
    AddPrograms(static_cast<ShaderProgramType>(type));
  }

  
  if (!mPrograms[RGBALayerProgramType].mVariations[MaskNone]->Initialize()) {
#ifdef MOZ_WIDGET_ANDROID
    NS_RUNTIMEABORT("Shader initialization failed");
#endif
    return false;
  }


  mGLContext->fGenFramebuffers(1, &mBackBufferFBO);

  if (mGLContext->WorkAroundDriverBugs()) {

    




    GLenum textureTargets[] = {
      LOCAL_GL_TEXTURE_2D,
      LOCAL_GL_NONE
    };

    if (mGLContext->IsGLES2()) {
        textureTargets[1] = LOCAL_GL_TEXTURE_RECTANGLE_ARB;
    }

    mFBOTextureTarget = LOCAL_GL_NONE;

    for (uint32_t i = 0; i < ArrayLength(textureTargets); i++) {
      GLenum target = textureTargets[i];
      if (!target)
          continue;

      mGLContext->fGenTextures(1, &mBackBufferTexture);
      mGLContext->fBindTexture(target, mBackBufferTexture);
      mGLContext->fTexParameteri(target,
                                LOCAL_GL_TEXTURE_MIN_FILTER,
                                LOCAL_GL_NEAREST);
      mGLContext->fTexParameteri(target,
                                LOCAL_GL_TEXTURE_MAG_FILTER,
                                LOCAL_GL_NEAREST);
      mGLContext->fTexImage2D(target,
                              0,
                              LOCAL_GL_RGBA,
                              5, 3, 
                              0,
                              LOCAL_GL_RGBA,
                              LOCAL_GL_UNSIGNED_BYTE,
                              nullptr);

      
      mGLContext->fBindTexture(target, 0);

      mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mBackBufferFBO);
      mGLContext->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                                        LOCAL_GL_COLOR_ATTACHMENT0,
                                        target,
                                        mBackBufferTexture,
                                        0);

      if (mGLContext->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER) ==
          LOCAL_GL_FRAMEBUFFER_COMPLETE)
      {
        mFBOTextureTarget = target;
        break;
      }

      
      
      mGLContext->fDeleteTextures(1, &mBackBufferTexture);
    }

    if (mFBOTextureTarget == LOCAL_GL_NONE) {
      
#ifdef MOZ_WIDGET_ANDROID
      NS_RUNTIMEABORT("No texture target");
#endif
      return false;
    }
  } else {
    
    mFBOTextureTarget = LOCAL_GL_TEXTURE_2D;
  }

  
  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

  if (mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB) {
    




    if (!mGLContext->IsExtensionSupported(gl::GLContext::ARB_texture_rectangle))
#ifdef MOZ_WIDGET_ANDROID
      NS_RUNTIMEABORT("No texture rectangle");
#endif
      return false;
  }

  
  if (mGLContext->IsDoubleBuffered()) {
    mGLContext->fDeleteFramebuffers(1, &mBackBufferFBO);
    mBackBufferFBO = 0;
  }

  

  mGLContext->fGenBuffers(1, &mQuadVBO);
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mQuadVBO);

  GLfloat vertices[] = {
    
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    
    0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  };
  mGLContext->fBufferData(LOCAL_GL_ARRAY_BUFFER, sizeof(vertices), vertices, LOCAL_GL_STATIC_DRAW);
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);

  nsCOMPtr<nsIConsoleService>
    console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));

  if (console) {
    nsString msg;
    msg +=
      NS_LITERAL_STRING("OpenGL LayerManager Initialized Succesfully.\nVersion: ");
    msg += NS_ConvertUTF8toUTF16(
      nsDependentCString((const char*)mGLContext->fGetString(LOCAL_GL_VERSION)));
    msg += NS_LITERAL_STRING("\nVendor: ");
    msg += NS_ConvertUTF8toUTF16(
      nsDependentCString((const char*)mGLContext->fGetString(LOCAL_GL_VENDOR)));
    msg += NS_LITERAL_STRING("\nRenderer: ");
    msg += NS_ConvertUTF8toUTF16(
      nsDependentCString((const char*)mGLContext->fGetString(LOCAL_GL_RENDERER)));
    msg += NS_LITERAL_STRING("\nFBO Texture Target: ");
    if (mFBOTextureTarget == LOCAL_GL_TEXTURE_2D)
      msg += NS_LITERAL_STRING("TEXTURE_2D");
    else
      msg += NS_LITERAL_STRING("TEXTURE_RECTANGLE");
    console->LogStringMessage(msg.get());
  }

  if (NS_IsMainThread()) {
    
    Preferences::AddBoolVarCache(&sDrawFPS, "layers.acceleration.draw-fps");
  } else {
    
    NS_DispatchToMainThread(new ReadDrawFPSPref());
  }

  mComposer2D = mWidget->GetComposer2D();

  reporter.SetSuccessful();
  return true;
}

void
LayerManagerOGL::SetClippingRegion(const nsIntRegion& aClippingRegion)
{
  mClippingRegion = aClippingRegion;
}

void
LayerManagerOGL::BeginTransaction()
{
  mInTransaction = true;
#ifdef DEBUG
  mMaybeInvalidTree = false;
#endif
}

void
LayerManagerOGL::BeginTransactionWithTarget(gfxContext *aTarget)
{
  mInTransaction = true;
#ifdef DEBUG
  mMaybeInvalidTree = false;
#endif

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("[----- BeginTransaction"));
  Log();
#endif

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  mTarget = aTarget;
}

bool
LayerManagerOGL::EndEmptyTransaction(EndTransactionFlags aFlags)
{
  
  
  
  MOZ_ASSERT(!mMaybeInvalidTree);
  mInTransaction = false;

  if (!mRoot)
    return false;

  EndTransaction(nullptr, nullptr, aFlags);
  return true;
}

void
LayerManagerOGL::EndTransaction(DrawThebesLayerCallback aCallback,
                                void* aCallbackData,
                                EndTransactionFlags aFlags)
{
  mInTransaction = false;

#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  if (mRoot && !(aFlags & END_NO_IMMEDIATE_REDRAW)) {
    if (aFlags & END_NO_COMPOSITE) {
      
      
      mRoot->ApplyPendingUpdatesToSubtree();
    }

    
    
    mRoot->ComputeEffectiveTransforms(gfx3DMatrix());

    mThebesLayerCallback = aCallback;
    mThebesLayerCallbackData = aCallbackData;
    SetCompositingDisabled(aFlags & END_NO_COMPOSITE);

    bool needGLRender = true;
    if (mComposer2D && mComposer2D->TryRender(mRoot, mWorldMatrix)) {
      needGLRender = false;

      if (sDrawFPS) {
        if (!mFPS) {
          mFPS = new FPSState();
        }
        double fps = mFPS->mCompositionFps.AddFrameAndGetFps(TimeStamp::Now());
        printf_stderr("HWComposer: FPS is %g\n", fps);
      }

      
      
      if (mTarget) {
        MakeCurrent();
        CopyToTarget(mTarget);
        mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);
      }

      MOZ_ASSERT(!needGLRender);
    }
    if (needGLRender) {
      Render();
    }

    mThebesLayerCallback = nullptr;
    mThebesLayerCallbackData = nullptr;
  }

  mTarget = nullptr;

#ifdef MOZ_LAYERS_HAVE_LOG
  Log();
  MOZ_LAYERS_LOG(("]----- EndTransaction"));
#endif
}

already_AddRefed<gfxASurface>
LayerManagerOGL::CreateOptimalMaskSurface(const gfxIntSize &aSize)
{
  return gfxPlatform::GetPlatform()->
    CreateOffscreenImageSurface(aSize, GFX_CONTENT_ALPHA);
}

already_AddRefed<ThebesLayer>
LayerManagerOGL::CreateThebesLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }

  nsRefPtr<ThebesLayer> layer = new ThebesLayerOGL(this);
  return layer.forget();
}

already_AddRefed<ContainerLayer>
LayerManagerOGL::CreateContainerLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }

  nsRefPtr<ContainerLayer> layer = new ContainerLayerOGL(this);
  return layer.forget();
}

already_AddRefed<ImageLayer>
LayerManagerOGL::CreateImageLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }

  nsRefPtr<ImageLayer> layer = new ImageLayerOGL(this);
  return layer.forget();
}

already_AddRefed<ColorLayer>
LayerManagerOGL::CreateColorLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }

  nsRefPtr<ColorLayer> layer = new ColorLayerOGL(this);
  return layer.forget();
}

already_AddRefed<CanvasLayer>
LayerManagerOGL::CreateCanvasLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }

  nsRefPtr<CanvasLayer> layer = new CanvasLayerOGL(this);
  return layer.forget();
}

static LayerOGL*
ToLayerOGL(Layer* aLayer)
{
  return static_cast<LayerOGL*>(aLayer->ImplData());
}

static void ClearSubtree(Layer* aLayer)
{
  ToLayerOGL(aLayer)->CleanupResources();
  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    ClearSubtree(child);
  }
}

void
LayerManagerOGL::ClearCachedResources(Layer* aSubtree)
{
  MOZ_ASSERT(!aSubtree || aSubtree->Manager() == this);
  Layer* subtree = aSubtree ? aSubtree : mRoot.get();
  if (!subtree) {
    return;
  }

  ClearSubtree(subtree);
#ifdef DEBUG
  
  
  
  
  for(; subtree && subtree != mRoot; subtree = subtree->GetParent());
  mMaybeInvalidTree = (subtree == mRoot);
#endif  
}

LayerOGL*
LayerManagerOGL::RootLayer() const
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nullptr;
  }

  return ToLayerOGL(mRoot);
}

bool LayerManagerOGL::sDrawFPS = false;








void 
LayerManagerOGL::BindAndDrawQuadWithTextureRect(ShaderProgramOGL *aProg,
                                                const nsIntRect& aTexCoordRect,
                                                const nsIntSize& aTexSize,
                                                GLenum aWrapMode ,
                                                bool aFlipped )
{
  NS_ASSERTION(aProg->HasInitialized(), "Shader program not correctly initialized");
  GLuint vertAttribIndex =
    aProg->AttribLocation(ShaderProgramOGL::VertexCoordAttrib);
  GLuint texCoordAttribIndex =
    aProg->AttribLocation(ShaderProgramOGL::TexCoordAttrib);
  NS_ASSERTION(texCoordAttribIndex != GLuint(-1), "no texture coords?");

  
  
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);

  
  
  
  
  
  

  GLContext::RectTriangles rects;

  nsIntSize realTexSize = aTexSize;
  if (!mGLContext->CanUploadNonPowerOfTwo()) {
    realTexSize = nsIntSize(NextPowerOfTwo(aTexSize.width),
                            NextPowerOfTwo(aTexSize.height));
  }

  if (aWrapMode == LOCAL_GL_REPEAT) {
    rects.addRect(
                  0.0f, 0.0f, 1.0f, 1.0f,
                  
                  aTexCoordRect.x / GLfloat(realTexSize.width),
                  aTexCoordRect.y / GLfloat(realTexSize.height),
                  aTexCoordRect.XMost() / GLfloat(realTexSize.width),
                  aTexCoordRect.YMost() / GLfloat(realTexSize.height),
                  aFlipped);
  } else {
    GLContext::DecomposeIntoNoRepeatTriangles(aTexCoordRect, realTexSize,
                                              rects, aFlipped);
  }

  mGLContext->fVertexAttribPointer(vertAttribIndex, 2,
                                   LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                   rects.vertexPointer());

  mGLContext->fVertexAttribPointer(texCoordAttribIndex, 2,
                                   LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                                   rects.texCoordPointer());

  {
    mGLContext->fEnableVertexAttribArray(texCoordAttribIndex);
    {
      mGLContext->fEnableVertexAttribArray(vertAttribIndex);

      mGLContext->fDrawArrays(LOCAL_GL_TRIANGLES, 0, rects.elements());

      mGLContext->fDisableVertexAttribArray(vertAttribIndex);
    }
    mGLContext->fDisableVertexAttribArray(texCoordAttribIndex);
  }
}

void
LayerManagerOGL::Render()
{
  PROFILER_LABEL("LayerManagerOGL", "Render");
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  nsIntRect rect;
  if (mIsRenderingToEGLSurface) {
    rect = nsIntRect(0, 0, mSurfaceSize.width, mSurfaceSize.height);
  } else {
    rect = mRenderBounds;
    
    if (rect.width == 0 || rect.height == 0) {
      
      
      
      mWidget->GetClientBounds(rect);
      rect.x = rect.y = 0;
    }
  }
  WorldTransformRect(rect);

  GLint width = rect.width;
  GLint height = rect.height;

  
  
  if (width == 0 || height == 0)
    return;

  
  
  if (mWidgetSize.width != width ||
      mWidgetSize.height != height)
  {
    MakeCurrent(true);

    mWidgetSize.width = width;
    mWidgetSize.height = height;
  } else {
    MakeCurrent();
  }

#if MOZ_WIDGET_ANDROID
  TexturePoolOGL::Fill(gl());
#endif

  SetupBackBuffer(width, height);
  SetupPipeline(width, height, ApplyWorldTransform);

  
  mGLContext->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE_MINUS_SRC_ALPHA,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
  mGLContext->fEnable(LOCAL_GL_BLEND);

  const nsIntRect *clipRect = mRoot->GetClipRect();

  if (clipRect) {
    nsIntRect r = *clipRect;
    WorldTransformRect(r);
    mGLContext->fScissor(r.x, r.y, r.width, r.height);
  } else {
    mGLContext->fScissor(0, 0, width, height);
  }

  if (CompositingDisabled()) {
    RootLayer()->RenderLayer(mGLContext->IsDoubleBuffered() ? 0 : mBackBufferFBO,
                             nsIntPoint(0, 0));
    mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);
    return;
  }

  mGLContext->fEnable(LOCAL_GL_SCISSOR_TEST);

  
  
  
#ifndef MOZ_ANDROID_OMTC
  mGLContext->fClearColor(0.0, 0.0, 0.0, 0.0);
  mGLContext->fClear(LOCAL_GL_COLOR_BUFFER_BIT | LOCAL_GL_DEPTH_BUFFER_BIT);
#endif

  
  mWidget->PrepareWindowEffects();
  mWidget->DrawWindowUnderlay(this, rect);

  
  mGLContext->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE_MINUS_SRC_ALPHA,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);

  
  RootLayer()->RenderLayer(mGLContext->IsDoubleBuffered() ? 0 : mBackBufferFBO,
                           nsIntPoint(0, 0));

  
  mWidget->DrawWindowOverlay(this, rect);

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    nsIntRect rect;
    if (mIsRenderingToEGLSurface) {
      rect = nsIntRect(0, 0, mSurfaceSize.width, mSurfaceSize.height);
    } else {
      mWidget->GetClientBounds(rect);
    }
    nsRefPtr<gfxASurface> surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(rect.Size(), GFX_CONTENT_COLOR_ALPHA);
    nsRefPtr<gfxContext> ctx = new gfxContext(surf);
    CopyToTarget(ctx);

    WriteSnapshotToDumpFile(this, surf);
  }
#endif

  if (mTarget) {
    CopyToTarget(mTarget);
    mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);
    return;
  }

  if (sDrawFPS && !mFPS) {
    mFPS = new FPSState();
  } else if (!sDrawFPS && mFPS) {
    mFPS = nullptr;
  }

  if (mFPS) {
    mFPS->DrawFPS(TimeStamp::Now(), 0, mGLContext, GetProgram(Copy2DProgramType));
  }

  if (mGLContext->IsDoubleBuffered()) {
    mGLContext->SwapBuffers();
    LayerManager::PostPresent();
    mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);
    return;
  }

  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

  mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);

  ShaderProgramOGL *copyprog = GetProgram(Copy2DProgramType);

  if (mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB) {
    copyprog = GetProgram(Copy2DRectProgramType);
  }

  mGLContext->fBindTexture(mFBOTextureTarget, mBackBufferTexture);

  copyprog->Activate();
  copyprog->SetTextureUnit(0);

  if (copyprog->GetTexCoordMultiplierUniformLocation() != -1) {
    copyprog->SetTexCoordMultiplier(width, height);
  }

  
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);

  
  mGLContext->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ZERO,
                                 LOCAL_GL_ONE, LOCAL_GL_ZERO);

  
  
  GLint vcattr = copyprog->AttribLocation(ShaderProgramOGL::VertexCoordAttrib);
  GLint tcattr = copyprog->AttribLocation(ShaderProgramOGL::TexCoordAttrib);

  mGLContext->fEnableVertexAttribArray(vcattr);
  mGLContext->fEnableVertexAttribArray(tcattr);

  const nsIntRect *r;
  nsIntRegionRectIterator iter(mClippingRegion);

  while ((r = iter.Next()) != nullptr) {
    nsIntRect cRect = *r; r = &cRect;
    WorldTransformRect(cRect);
    float left = (GLfloat)r->x / width;
    float right = (GLfloat)r->XMost() / width;
    float top = (GLfloat)r->y / height;
    float bottom = (GLfloat)r->YMost() / height;

    float vertices[] = { left * 2.0f - 1.0f,
                         -(top * 2.0f - 1.0f),
                         right * 2.0f - 1.0f,
                         -(top * 2.0f - 1.0f),
                         left * 2.0f - 1.0f,
                         -(bottom * 2.0f - 1.0f),
                         right * 2.0f - 1.0f,
                         -(bottom * 2.0f - 1.0f) };

    
    
    float coords[] = { left, 1 - top,
                       right, 1 - top,
                       left, 1 - bottom,
                       right, 1 - bottom };

    mGLContext->fVertexAttribPointer(vcattr,
                                     2, LOCAL_GL_FLOAT,
                                     LOCAL_GL_FALSE,
                                     0, vertices);

    mGLContext->fVertexAttribPointer(tcattr,
                                     2, LOCAL_GL_FLOAT,
                                     LOCAL_GL_FALSE,
                                     0, coords);

    mGLContext->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);
  }

  mGLContext->fDisableVertexAttribArray(vcattr);
  mGLContext->fDisableVertexAttribArray(tcattr);

  mGLContext->fFlush();
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);
}

void
LayerManagerOGL::SetWorldTransform(const gfxMatrix& aMatrix)
{
  NS_ASSERTION(aMatrix.PreservesAxisAlignedRectangles(),
               "SetWorldTransform only accepts matrices that satisfy PreservesAxisAlignedRectangles");
  NS_ASSERTION(!aMatrix.HasNonIntegerScale(),
               "SetWorldTransform only accepts matrices with integer scale");

  mWorldMatrix = aMatrix;
}

gfxMatrix&
LayerManagerOGL::GetWorldTransform(void)
{
  return mWorldMatrix;
}

void
LayerManagerOGL::WorldTransformRect(nsIntRect& aRect)
{
  gfxRect grect(aRect.x, aRect.y, aRect.width, aRect.height);
  grect = mWorldMatrix.TransformBounds(grect);
  aRect.SetRect(grect.X(), grect.Y(), grect.Width(), grect.Height());
}

void
LayerManagerOGL::UpdateRenderBounds(const nsIntRect& aRect)
{
  mRenderBounds = aRect;
}

void
LayerManagerOGL::SetSurfaceSize(int width, int height)
{
  mSurfaceSize.width = width;
  mSurfaceSize.height = height;
}

void
LayerManagerOGL::SetupPipeline(int aWidth, int aHeight, WorldTransforPolicy aTransformPolicy)
{
  
  mGLContext->fViewport(0, 0, aWidth, aHeight);

  
  
  
  
  
  
  
  

  
  
  gfxMatrix viewMatrix; 
  viewMatrix.Translate(-gfxPoint(1.0, -1.0));
  viewMatrix.Scale(2.0f / float(aWidth), 2.0f / float(aHeight));
  viewMatrix.Scale(1.0f, -1.0f);

  if (aTransformPolicy == ApplyWorldTransform) {
    viewMatrix = mWorldMatrix * viewMatrix;
  }

  gfx3DMatrix matrix3d = gfx3DMatrix::From2D(viewMatrix);
  matrix3d._33 = 0.0f;

  SetLayerProgramProjectionMatrix(matrix3d);
}

void
LayerManagerOGL::SetupBackBuffer(int aWidth, int aHeight)
{
  if (mGLContext->IsDoubleBuffered()) {
    mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);
    return;
  }

  
  if (mBackBufferSize.width == aWidth &&
      mBackBufferSize.height == aHeight)
  {
    mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mBackBufferFBO);
    return;
  }

  
  mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
  mGLContext->fBindTexture(mFBOTextureTarget, mBackBufferTexture);
  mGLContext->fTexImage2D(mFBOTextureTarget,
                          0,
                          LOCAL_GL_RGBA,
                          aWidth, aHeight,
                          0,
                          LOCAL_GL_RGBA,
                          LOCAL_GL_UNSIGNED_BYTE,
                          nullptr);
  mGLContext->fBindTexture(mFBOTextureTarget, 0);

  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mBackBufferFBO);
  mGLContext->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                                    LOCAL_GL_COLOR_ATTACHMENT0,
                                    mFBOTextureTarget,
                                    mBackBufferTexture,
                                    0);

  GLenum result = mGLContext->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
  if (result != LOCAL_GL_FRAMEBUFFER_COMPLETE) {
    nsAutoCString msg;
    msg.Append("Framebuffer not complete -- error 0x");
    msg.AppendInt(result, 16);
    NS_RUNTIMEABORT(msg.get());
  }

  mBackBufferSize.width = aWidth;
  mBackBufferSize.height = aHeight;
}

void
LayerManagerOGL::CopyToTarget(gfxContext *aTarget)
{
  nsIntRect rect;
  if (mIsRenderingToEGLSurface) {
    rect = nsIntRect(0, 0, mSurfaceSize.width, mSurfaceSize.height);
  } else {
    rect = nsIntRect(0, 0, mWidgetSize.width, mWidgetSize.height);
  }
  GLint width = rect.width;
  GLint height = rect.height;

  if ((int64_t(width) * int64_t(height) * int64_t(4)) > INT32_MAX) {
    NS_ERROR("Widget size too big - integer overflow!");
    return;
  }

  nsRefPtr<gfxImageSurface> imageSurface =
    new gfxImageSurface(gfxIntSize(width, height),
                        gfxImageFormatARGB32);

  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER,
                               mGLContext->IsDoubleBuffered() ? 0 : mBackBufferFBO);

  if (!mGLContext->IsGLES2()) {
    
    
    if (mGLContext->IsDoubleBuffered()) {
      mGLContext->fReadBuffer(LOCAL_GL_BACK);
    }
    else {
      mGLContext->fReadBuffer(LOCAL_GL_COLOR_ATTACHMENT0);
    }
  }

  NS_ASSERTION(imageSurface->Stride() == width * 4,
               "Image Surfaces being created with weird stride!");

  mGLContext->ReadPixelsIntoImageSurface(imageSurface);

  
  gfxMatrix glToCairoTransform = mWorldMatrix;
  glToCairoTransform.Invert();
  glToCairoTransform.Scale(1.0, -1.0);
  glToCairoTransform.Translate(-gfxPoint(0.0, height));

  gfxContextAutoSaveRestore restore(aTarget);
  aTarget->SetOperator(gfxContext::OPERATOR_SOURCE);
  aTarget->SetMatrix(glToCairoTransform);
  aTarget->SetSource(imageSurface);
  aTarget->Paint();
}

void
LayerManagerOGL::SetLayerProgramProjectionMatrix(const gfx3DMatrix& aMatrix)
{
  for (unsigned int i = 0; i < mPrograms.Length(); ++i) {
    for (uint32_t mask = MaskNone; mask < NumMaskTypes; ++mask) {
      if (mPrograms[i].mVariations[mask]) {
        mPrograms[i].mVariations[mask]->CheckAndSetProjectionMatrix(aMatrix);
      }
    }
  }
}

static GLenum
GetFrameBufferInternalFormat(GLContext* gl,
                             GLuint aCurrentFrameBuffer,
                             nsIWidget* aWidget)
{
  if (aCurrentFrameBuffer == 0) { 
    return aWidget->GetGLFrameBufferFormat();
  }
  return LOCAL_GL_RGBA;
}

bool
LayerManagerOGL::CreateFBOWithTexture(const nsIntRect& aRect, InitMode aInit,
                                      GLuint aCurrentFrameBuffer,
                                      GLuint *aFBO, GLuint *aTexture)
{
  GLuint tex, fbo;

  mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
  mGLContext->fGenTextures(1, &tex);
  mGLContext->fBindTexture(mFBOTextureTarget, tex);

  if (aInit == InitModeCopy) {
    
    
    
    
    
    GLenum format =
      GetFrameBufferInternalFormat(gl(), aCurrentFrameBuffer, mWidget);
 
    bool isFormatCompatibleWithRGBA
        = gl()->IsGLES2() ? (format == LOCAL_GL_RGBA)
                          : true;

    if (isFormatCompatibleWithRGBA) {
      mGLContext->fCopyTexImage2D(mFBOTextureTarget,
                                  0,
                                  LOCAL_GL_RGBA,
                                  aRect.x, aRect.y,
                                  aRect.width, aRect.height,
                                  0);
    } else {
      

      
      size_t bufferSize = aRect.width * aRect.height * 4;
      nsAutoArrayPtr<uint8_t> buf(new uint8_t[bufferSize]);

      mGLContext->fReadPixels(aRect.x, aRect.y,
                              aRect.width, aRect.height,
                              LOCAL_GL_RGBA,
                              LOCAL_GL_UNSIGNED_BYTE,
                              buf);
      mGLContext->fTexImage2D(mFBOTextureTarget,
                              0,
                              LOCAL_GL_RGBA,
                              aRect.width, aRect.height,
                              0,
                              LOCAL_GL_RGBA,
                              LOCAL_GL_UNSIGNED_BYTE,
                              buf);
    }
  } else {
    mGLContext->fTexImage2D(mFBOTextureTarget,
                            0,
                            LOCAL_GL_RGBA,
                            aRect.width, aRect.height,
                            0,
                            LOCAL_GL_RGBA,
                            LOCAL_GL_UNSIGNED_BYTE,
                            nullptr);
  }
  mGLContext->fTexParameteri(mFBOTextureTarget, LOCAL_GL_TEXTURE_MIN_FILTER,
                             LOCAL_GL_LINEAR);
  mGLContext->fTexParameteri(mFBOTextureTarget, LOCAL_GL_TEXTURE_MAG_FILTER,
                             LOCAL_GL_LINEAR);
  mGLContext->fTexParameteri(mFBOTextureTarget, LOCAL_GL_TEXTURE_WRAP_S, 
                             LOCAL_GL_CLAMP_TO_EDGE);
  mGLContext->fTexParameteri(mFBOTextureTarget, LOCAL_GL_TEXTURE_WRAP_T, 
                             LOCAL_GL_CLAMP_TO_EDGE);
  mGLContext->fBindTexture(mFBOTextureTarget, 0);

  mGLContext->fGenFramebuffers(1, &fbo);
  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, fbo);
  mGLContext->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                                    LOCAL_GL_COLOR_ATTACHMENT0,
                                    mFBOTextureTarget,
                                    tex,
                                    0);

  
  
  GLenum result = mGLContext->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
  if (result != LOCAL_GL_FRAMEBUFFER_COMPLETE) {
    nsAutoCString msg;
    msg.Append("Framebuffer not complete -- error 0x");
    msg.AppendInt(result, 16);
    msg.Append(", mFBOTextureTarget 0x");
    msg.AppendInt(mFBOTextureTarget, 16);
    msg.Append(", aRect.width ");
    msg.AppendInt(aRect.width);
    msg.Append(", aRect.height ");
    msg.AppendInt(aRect.height);
    NS_WARNING(msg.get());

    mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);
    mGLContext->fDeleteFramebuffers(1, &fbo);
    mGLContext->fDeleteTextures(1, &tex);
    return false;
  }

  SetupPipeline(aRect.width, aRect.height, DontApplyWorldTransform);
  mGLContext->fScissor(0, 0, aRect.width, aRect.height);

  if (aInit == InitModeClear) {
    mGLContext->fClearColor(0.0, 0.0, 0.0, 0.0);
    mGLContext->fClear(LOCAL_GL_COLOR_BUFFER_BIT);
  }

  *aFBO = fbo;
  *aTexture = tex;
  return true;
}

TemporaryRef<DrawTarget>
LayerManagerOGL::CreateDrawTarget(const IntSize &aSize,
                                  SurfaceFormat aFormat)
{
#ifdef XP_MACOSX
  
  
  
  
  bool useAcceleration = aSize.width <= 4096 && aSize.height <= 4096 &&
                         aSize.width > 64 && aSize.height > 64 &&
                         gfxPlatformMac::GetPlatform()->UseAcceleratedCanvas();
  if (useAcceleration) {
    return Factory::CreateDrawTarget(BACKEND_COREGRAPHICS_ACCELERATED,
                                     aSize, aFormat);
  }
#endif
  return LayerManager::CreateDrawTarget(aSize, aFormat);
}

} 
} 
