






































#ifdef MOZ_IPC
# include "mozilla/layers/PLayers.h"
#endif  

#include "LayerManagerOGL.h"
#include "ThebesLayerOGL.h"
#include "ContainerLayerOGL.h"
#include "ImageLayerOGL.h"
#include "ColorLayerOGL.h"
#include "CanvasLayerOGL.h"

#include "LayerManagerOGLShaders.h"

#include "gfxContext.h"
#include "nsIWidget.h"

#include "GLContext.h"
#include "GLContextProvider.h"

#include "nsIServiceManager.h"
#include "nsIConsoleService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gl;

#ifdef CHECK_CURRENT_PROGRAM
int LayerManagerOGLProgram::sCurrentProgramKey = 0;
#endif




LayerManagerOGL::LayerManagerOGL(nsIWidget *aWidget)
  : mWidget(aWidget)
  , mWidgetSize(-1, -1)
  , mBackBufferFBO(0)
  , mBackBufferTexture(0)
  , mBackBufferSize(-1, -1)
  , mHasBGRA(0)
{
}

LayerManagerOGL::~LayerManagerOGL()
{
  Destroy();
}

void
LayerManagerOGL::Destroy()
{
  if (!mDestroyed) {
    if (mRoot) {
      RootLayer()->Destroy();
    }
    mRoot = nsnull;

    
    
    nsTArray<ImageContainer*> imageContainers(mImageContainers);
    for (PRUint32 i = 0; i < imageContainers.Length(); ++i) {
      ImageContainer *c = imageContainers[i];
      c->SetLayerManager(nsnull);
    }

    CleanupResources();

    mDestroyed = PR_TRUE;
  }
}

void
LayerManagerOGL::CleanupResources()
{
  if (!mGLContext)
    return;

  nsRefPtr<GLContext> ctx = mGLContext->GetSharedContext();
  if (!ctx) {
    ctx = mGLContext;
  }

  ctx->MakeCurrent();

  for (unsigned int i = 0; i < mPrograms.Length(); ++i)
    delete mPrograms[i];
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

  mGLContext = nsnull;
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

PRBool
LayerManagerOGL::Initialize(nsRefPtr<GLContext> aContext)
{
  
  NS_ABORT_IF_FALSE(mGLContext == nsnull, "Don't reiniailize layer managers");

  if (!aContext)
    return PR_FALSE;

  mGLContext = aContext;

  MakeCurrent();

  DEBUG_GL_ERROR_CHECK(mGLContext);

  mHasBGRA =
    mGLContext->IsExtensionSupported(gl::GLContext::EXT_texture_format_BGRA8888) ||
    mGLContext->IsExtensionSupported(gl::GLContext::EXT_bgra);

  mGLContext->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE_MINUS_SRC_ALPHA,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
  mGLContext->fEnable(LOCAL_GL_BLEND);

  
  
  
#define SHADER_PROGRAM(penum, ptype, vsstr, fsstr) do {                           \
    NS_ASSERTION(programIndex++ == penum, "out of order shader initialization!"); \
    ptype *p = new ptype(mGLContext);                                             \
    if (!p->Initialize(vsstr, fsstr)) {                                           \
      delete p;                                                                   \
      return PR_FALSE;                                                            \
    }                                                                             \
    mPrograms.AppendElement(p);                                                   \
  } while (0)


  
  
#ifdef DEBUG
  GLint programIndex = 0;
#endif

  
  SHADER_PROGRAM(RGBALayerProgramType, ColorTextureLayerProgram,
                 sLayerVS, sRGBATextureLayerFS);
  SHADER_PROGRAM(BGRALayerProgramType, ColorTextureLayerProgram,
                 sLayerVS, sBGRATextureLayerFS);
  SHADER_PROGRAM(RGBXLayerProgramType, ColorTextureLayerProgram,
                 sLayerVS, sRGBXTextureLayerFS);
  SHADER_PROGRAM(BGRXLayerProgramType, ColorTextureLayerProgram,
                 sLayerVS, sBGRXTextureLayerFS);
  SHADER_PROGRAM(RGBARectLayerProgramType, ColorTextureLayerProgram,
                 sLayerVS, sRGBARectTextureLayerFS);
  SHADER_PROGRAM(ColorLayerProgramType, SolidColorLayerProgram,
                 sLayerVS, sSolidColorLayerFS);
  SHADER_PROGRAM(YCbCrLayerProgramType, YCbCrTextureLayerProgram,
                 sLayerVS, sYCbCrTextureLayerFS);
  SHADER_PROGRAM(ComponentAlphaPass1ProgramType, ComponentAlphaTextureLayerProgram,
                 sLayerVS, sComponentPass1FS);
  SHADER_PROGRAM(ComponentAlphaPass2ProgramType, ComponentAlphaTextureLayerProgram,
                 sLayerVS, sComponentPass2FS);
  
  SHADER_PROGRAM(Copy2DProgramType, CopyProgram,
                 sCopyVS, sCopy2DFS);
  SHADER_PROGRAM(Copy2DRectProgramType, CopyProgram,
                 sCopyVS, sCopy2DRectFS);

#undef SHADER_PROGRAM

  NS_ASSERTION(programIndex == NumProgramTypes,
               "not all programs were initialized!");

  



  mGLContext->fGenFramebuffers(1, &mBackBufferFBO);

  GLenum textureTargets[] = {
    LOCAL_GL_TEXTURE_2D,
#ifndef USE_GLES2
    LOCAL_GL_TEXTURE_RECTANGLE_ARB
#endif
  };

  mFBOTextureTarget = LOCAL_GL_NONE;

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(textureTargets); i++) {
    GLenum target = textureTargets[i];
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
                            NULL);

    
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
    
    return false;
  }

  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

  if (mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB) {
    




    if (!mGLContext->IsExtensionSupported(gl::GLContext::ARB_texture_rectangle))
      return false;
  }

  
  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

  DEBUG_GL_ERROR_CHECK(mGLContext);

  

  mGLContext->fGenBuffers(1, &mQuadVBO);
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mQuadVBO);

  GLfloat vertices[] = {
    
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    
    0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
  };
  mGLContext->fBufferData(LOCAL_GL_ARRAY_BUFFER, sizeof(vertices), vertices, LOCAL_GL_STATIC_DRAW);

  DEBUG_GL_ERROR_CHECK(mGLContext);

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

  DEBUG_GL_ERROR_CHECK(mGLContext);

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
}

void
LayerManagerOGL::BeginTransactionWithTarget(gfxContext *aTarget)
{
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
LayerManagerOGL::EndEmptyTransaction()
{
  if (!mRoot)
    return false;

  EndTransaction(nsnull, nsnull);
  return true;
}

void
LayerManagerOGL::EndTransaction(DrawThebesLayerCallback aCallback,
                                void* aCallbackData)
{
#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  
  
  mRoot->ComputeEffectiveTransforms(gfx3DMatrix());

  mThebesLayerCallback = aCallback;
  mThebesLayerCallbackData = aCallbackData;

  Render();

  mThebesLayerCallback = nsnull;
  mThebesLayerCallbackData = nsnull;

  mTarget = NULL;

#ifdef MOZ_LAYERS_HAVE_LOG
  Log();
  MOZ_LAYERS_LOG(("]----- EndTransaction"));
#endif
}

already_AddRefed<ThebesLayer>
LayerManagerOGL::CreateThebesLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }

  nsRefPtr<ThebesLayer> layer = new ThebesLayerOGL(this);
  return layer.forget();
}

already_AddRefed<ContainerLayer>
LayerManagerOGL::CreateContainerLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }

  nsRefPtr<ContainerLayer> layer = new ContainerLayerOGL(this);
  return layer.forget();
}

already_AddRefed<ImageContainer>
LayerManagerOGL::CreateImageContainer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }

  nsRefPtr<ImageContainer> container = new ImageContainerOGL(this);
  RememberImageContainer(container);
  return container.forget();
}

already_AddRefed<ImageLayer>
LayerManagerOGL::CreateImageLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }

  nsRefPtr<ImageLayer> layer = new ImageLayerOGL(this);
  return layer.forget();
}

already_AddRefed<ColorLayer>
LayerManagerOGL::CreateColorLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }

  nsRefPtr<ColorLayer> layer = new ColorLayerOGL(this);
  return layer.forget();
}

already_AddRefed<CanvasLayer>
LayerManagerOGL::CreateCanvasLayer()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }

  nsRefPtr<CanvasLayer> layer = new CanvasLayerOGL(this);
  return layer.forget();
}

void
LayerManagerOGL::ForgetImageContainer(ImageContainer *aContainer)
{
  NS_ASSERTION(aContainer->Manager() == this,
               "ForgetImageContainer called on non-owned container!");

  if (!mImageContainers.RemoveElement(aContainer)) {
    NS_WARNING("ForgetImageContainer couldn't find container it was supposed to forget!");
    return;
  }
}

void
LayerManagerOGL::RememberImageContainer(ImageContainer *aContainer)
{
  NS_ASSERTION(aContainer->Manager() == this,
               "RememberImageContainer called on non-owned container!");
  mImageContainers.AppendElement(aContainer);
}

LayerOGL*
LayerManagerOGL::RootLayer() const
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }

  return static_cast<LayerOGL*>(mRoot->ImplData());
}

void
LayerManagerOGL::Render()
{
  if (mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return;
  }

  nsIntRect rect;
  mWidget->GetClientBounds(rect);

  GLint width = rect.width;
  GLint height = rect.height;

  
  
  if (width == 0 || height == 0)
    return;

  
  
  if (mWidgetSize.width != width ||
      mWidgetSize.height != height)
  {
    MakeCurrent(PR_TRUE);

    mWidgetSize.width = width;
    mWidgetSize.height = height;
  } else {
    MakeCurrent();
  }

  DEBUG_GL_ERROR_CHECK(mGLContext);

  SetupBackBuffer(width, height);
  SetupPipeline(width, height);

  
  mGLContext->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE_MINUS_SRC_ALPHA,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
  mGLContext->fEnable(LOCAL_GL_BLEND);

  DEBUG_GL_ERROR_CHECK(mGLContext);

  const nsIntRect *clipRect = mRoot->GetClipRect();

  if (clipRect) {
    nsIntRect r = *clipRect;
    if (!mGLContext->IsDoubleBuffered() && !mTarget)
      mGLContext->FixWindowCoordinateRect(r, mWidgetSize.height);
    mGLContext->fScissor(r.x, r.y, r.width, r.height);
  } else {
    mGLContext->fScissor(0, 0, width, height);
  }

  mGLContext->fEnable(LOCAL_GL_SCISSOR_TEST);

  DEBUG_GL_ERROR_CHECK(mGLContext);

  mGLContext->fClearColor(0.0, 0.0, 0.0, 0.0);
  mGLContext->fClear(LOCAL_GL_COLOR_BUFFER_BIT | LOCAL_GL_DEPTH_BUFFER_BIT);

  
  RootLayer()->RenderLayer(mGLContext->IsDoubleBuffered() && !mTarget ? 0 : mBackBufferFBO,
                           nsIntPoint(0, 0));
                           
  static_cast<nsIWidget_MOZILLA_2_0_BRANCH*>(mWidget)->DrawOver(this, rect);

  DEBUG_GL_ERROR_CHECK(mGLContext);

  if (mTarget) {
    CopyToTarget();
    return;
  }

  if (mGLContext->IsDoubleBuffered()) {
    mGLContext->SwapBuffers();
    return;
  }

  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, 0);

  mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);

  CopyProgram *copyprog = GetCopy2DProgram();

  if (mFBOTextureTarget == LOCAL_GL_TEXTURE_RECTANGLE_ARB) {
    copyprog = GetCopy2DRectProgram();
  }

  mGLContext->fBindTexture(mFBOTextureTarget, mBackBufferTexture);

  copyprog->Activate();
  copyprog->SetTextureUnit(0);

  if (copyprog->GetTexCoordMultiplierUniformLocation() != -1) {
    float f[] = { float(width), float(height) };
    copyprog->SetUniform(copyprog->GetTexCoordMultiplierUniformLocation(),
                         2, f);
  }

  DEBUG_GL_ERROR_CHECK(mGLContext);

  
  mGLContext->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);

  
  mGLContext->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ZERO,
                                 LOCAL_GL_ONE, LOCAL_GL_ZERO);

  
  
  GLint vcattr = copyprog->AttribLocation(CopyProgram::VertexCoordAttrib);
  GLint tcattr = copyprog->AttribLocation(CopyProgram::TexCoordAttrib);

  mGLContext->fEnableVertexAttribArray(vcattr);
  mGLContext->fEnableVertexAttribArray(tcattr);

  const nsIntRect *r;
  nsIntRegionRectIterator iter(mClippingRegion);

  while ((r = iter.Next()) != nsnull) {
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

    float coords[] = { left, top,
                       right, top,
                       left, bottom,
                       right, bottom };

    mGLContext->fVertexAttribPointer(vcattr,
                                     2, LOCAL_GL_FLOAT,
                                     LOCAL_GL_FALSE,
                                     0, vertices);

    mGLContext->fVertexAttribPointer(tcattr,
                                     2, LOCAL_GL_FLOAT,
                                     LOCAL_GL_FALSE,
                                     0, coords);

    mGLContext->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);
    DEBUG_GL_ERROR_CHECK(mGLContext);
  }

  mGLContext->fDisableVertexAttribArray(vcattr);
  mGLContext->fDisableVertexAttribArray(tcattr);

  DEBUG_GL_ERROR_CHECK(mGLContext);

  mGLContext->fFlush();

  DEBUG_GL_ERROR_CHECK(mGLContext);
}

void
LayerManagerOGL::SetupPipeline(int aWidth, int aHeight)
{
  
  
  
  
  
  
  mGLContext->fViewport(0, 0, aWidth, aHeight);

  
  
  
  
  
  
  
  
  
  
  
  
  gfx3DMatrix viewMatrix;
  if (mGLContext->IsDoubleBuffered() && !mTarget) {
    


    viewMatrix._11 = 2.0f / float(aWidth);
    viewMatrix._22 = -2.0f / float(aHeight);
    viewMatrix._41 = -1.0f;
    viewMatrix._42 = 1.0f;
  } else {
    viewMatrix._11 = 2.0f / float(aWidth);
    viewMatrix._22 = 2.0f / float(aHeight);
    viewMatrix._41 = -1.0f;
    viewMatrix._42 = -1.0f;
  }

  SetLayerProgramProjectionMatrix(viewMatrix);
}

void
LayerManagerOGL::SetupBackBuffer(int aWidth, int aHeight)
{
  if (mGLContext->IsDoubleBuffered() && !mTarget) {
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
                          NULL);
  mGLContext->fBindTexture(mFBOTextureTarget, 0);

  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mBackBufferFBO);
  mGLContext->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                                    LOCAL_GL_COLOR_ATTACHMENT0,
                                    mFBOTextureTarget,
                                    mBackBufferTexture,
                                    0);

  NS_ASSERTION(mGLContext->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER) ==
               LOCAL_GL_FRAMEBUFFER_COMPLETE, "Error setting up framebuffer.");

  mBackBufferSize.width = aWidth;
  mBackBufferSize.height = aHeight;
}

void
LayerManagerOGL::CopyToTarget()
{
  nsIntRect rect;
  mWidget->GetBounds(rect);
  GLint width = rect.width;
  GLint height = rect.height;

  if ((PRInt64(width) * PRInt64(height) * PRInt64(4)) > PR_INT32_MAX) {
    NS_ERROR("Widget size too big - integer overflow!");
    return;
  }

  nsRefPtr<gfxImageSurface> imageSurface =
    new gfxImageSurface(gfxIntSize(width, height),
                        gfxASurface::ImageFormatARGB32);

  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER,
                               mBackBufferFBO);
#ifndef USE_GLES2
  
  
    mGLContext->fReadBuffer(LOCAL_GL_COLOR_ATTACHMENT0);
#endif

  GLenum format = LOCAL_GL_RGBA;
  if (mHasBGRA)
    format = LOCAL_GL_BGRA;

  NS_ASSERTION(imageSurface->Stride() == width * 4,
               "Image Surfaces being created with weird stride!");

  PRUint32 currentPackAlignment = 0;
  mGLContext->fGetIntegerv(LOCAL_GL_PACK_ALIGNMENT, (GLint*)&currentPackAlignment);
  if (currentPackAlignment != 4) {
    mGLContext->fPixelStorei(LOCAL_GL_PACK_ALIGNMENT, 4);
  }

  mGLContext->fFinish();

  mGLContext->fReadPixels(0, 0,
                          width, height,
                          format,
                          LOCAL_GL_UNSIGNED_BYTE,
                          imageSurface->Data());

  if (currentPackAlignment != 4) {
    mGLContext->fPixelStorei(LOCAL_GL_PACK_ALIGNMENT, currentPackAlignment);
  }

  if (!mHasBGRA) {
    
    for (int j = 0; j < height; ++j) {
      PRUint32 *row = (PRUint32*) (imageSurface->Data() + imageSurface->Stride() * j);
      for (int i = 0; i < width; ++i) {
        *row = (*row & 0xff00ff00) | ((*row & 0xff) << 16) | ((*row & 0xff0000) >> 16);
        row++;
      }
    }
  }

  mTarget->SetOperator(gfxContext::OPERATOR_OVER);
  mTarget->SetSource(imageSurface);
  mTarget->Paint();
}

LayerManagerOGL::ProgramType LayerManagerOGL::sLayerProgramTypes[] = {
  gl::RGBALayerProgramType,
  gl::BGRALayerProgramType,
  gl::RGBXLayerProgramType,
  gl::BGRXLayerProgramType,
  gl::RGBARectLayerProgramType,
  gl::ColorLayerProgramType,
  gl::YCbCrLayerProgramType,
  gl::ComponentAlphaPass1ProgramType,
  gl::ComponentAlphaPass2ProgramType
};

#define FOR_EACH_LAYER_PROGRAM(vname)                       \
  for (size_t lpindex = 0;                                  \
       lpindex < NS_ARRAY_LENGTH(sLayerProgramTypes);       \
       ++lpindex)                                           \
  {                                                         \
    LayerProgram *vname = static_cast<LayerProgram*>        \
      (mPrograms[sLayerProgramTypes[lpindex]]);             \
    do

#define FOR_EACH_LAYER_PROGRAM_END              \
    while (0);                                  \
  }                                             \

void
LayerManagerOGL::SetLayerProgramProjectionMatrix(const gfx3DMatrix& aMatrix)
{
  FOR_EACH_LAYER_PROGRAM(lp) {
    lp->Activate();
    lp->SetProjectionMatrix(aMatrix);
  } FOR_EACH_LAYER_PROGRAM_END
}

void
LayerManagerOGL::CreateFBOWithTexture(const nsIntRect& aRect, InitMode aInit,
                                      GLuint *aFBO, GLuint *aTexture)
{
  GLuint tex, fbo;

  mGLContext->fActiveTexture(LOCAL_GL_TEXTURE0);
  mGLContext->fGenTextures(1, &tex);
  mGLContext->fBindTexture(mFBOTextureTarget, tex);
  if (aInit == InitModeCopy) {
    mGLContext->fCopyTexImage2D(mFBOTextureTarget,
                                0,
                                LOCAL_GL_RGBA,
                                aRect.x, aRect.y,
                                aRect.width, aRect.height,
                                0);
  } else {
    mGLContext->fTexImage2D(mFBOTextureTarget,
                            0,
                            LOCAL_GL_RGBA,
                            aRect.width, aRect.height,
                            0,
                            LOCAL_GL_RGBA,
                            LOCAL_GL_UNSIGNED_BYTE,
                            NULL);
  }
  mGLContext->fTexParameteri(mFBOTextureTarget, LOCAL_GL_TEXTURE_MIN_FILTER,
                             LOCAL_GL_LINEAR);
  mGLContext->fTexParameteri(mFBOTextureTarget, LOCAL_GL_TEXTURE_MAG_FILTER,
                             LOCAL_GL_LINEAR);
  mGLContext->fBindTexture(mFBOTextureTarget, 0);

  mGLContext->fGenFramebuffers(1, &fbo);
  mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, fbo);
  mGLContext->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                                    LOCAL_GL_COLOR_ATTACHMENT0,
                                    mFBOTextureTarget,
                                    tex,
                                    0);

  NS_ASSERTION(mGLContext->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER) ==
               LOCAL_GL_FRAMEBUFFER_COMPLETE, "Error setting up framebuffer.");

  if (aInit == InitModeClear) {
    mGLContext->fClearColor(0.0, 0.0, 0.0, 0.0);
    mGLContext->fClear(LOCAL_GL_COLOR_BUFFER_BIT);
  }

  *aFBO = fbo;
  *aTexture = tex;

  DEBUG_GL_ERROR_CHECK(gl());
}

void LayerOGL::ApplyFilter(gfxPattern::GraphicsFilter aFilter)
{
  if (aFilter == gfxPattern::FILTER_NEAREST) {
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_NEAREST);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_NEAREST);
  } else {
    if (aFilter != gfxPattern::FILTER_GOOD) {
      NS_WARNING("Unsupported filter type!");
    }
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
    gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
  }
}

#ifdef MOZ_IPC

already_AddRefed<ShadowThebesLayer>
LayerManagerOGL::CreateShadowThebesLayer()
{
  if (LayerManagerOGL::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }
  return nsRefPtr<ShadowThebesLayerOGL>(new ShadowThebesLayerOGL(this)).forget();
}

already_AddRefed<ShadowContainerLayer>
LayerManagerOGL::CreateShadowContainerLayer()
{
  if (LayerManagerOGL::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }
  return nsRefPtr<ShadowContainerLayerOGL>(new ShadowContainerLayerOGL(this)).forget();
}

already_AddRefed<ShadowImageLayer>
LayerManagerOGL::CreateShadowImageLayer()
{
  if (LayerManagerOGL::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }
  return nsRefPtr<ShadowImageLayerOGL>(new ShadowImageLayerOGL(this)).forget();
}

already_AddRefed<ShadowColorLayer>
LayerManagerOGL::CreateShadowColorLayer()
{
  if (LayerManagerOGL::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }
  return nsRefPtr<ShadowColorLayerOGL>(new ShadowColorLayerOGL(this)).forget();
}

already_AddRefed<ShadowCanvasLayer>
LayerManagerOGL::CreateShadowCanvasLayer()
{
  if (LayerManagerOGL::mDestroyed) {
    NS_WARNING("Call on destroyed layer manager");
    return nsnull;
  }
  return nsRefPtr<ShadowCanvasLayerOGL>(new ShadowCanvasLayerOGL(this)).forget();
}

#else

already_AddRefed<ShadowThebesLayer>
LayerManagerOGL::CreateShadowThebesLayer() { return nsnull; }
already_AddRefed<ShadowContainerLayer>
LayerManagerOGL::CreateShadowContainerLayer() { return nsnull; }
already_AddRefed<ShadowImageLayer>
LayerManagerOGL::CreateShadowImageLayer() { return nsnull; }
already_AddRefed<ShadowColorLayer>
LayerManagerOGL::CreateShadowColorLayer() { return nsnull; }
already_AddRefed<ShadowCanvasLayer>
LayerManagerOGL::CreateShadowCanvasLayer() { return nsnull; }

#endif  

} 
} 
