




#include "base/process.h"
#include "GLContext.h"
#include "gfx2DGlue.h"
#include <ui/GraphicBuffer.h>
#include "GrallocImages.h"  
#include "mozilla/layers/GrallocTextureHost.h"
#include "mozilla/layers/SharedBufferManagerParent.h"
#include "EGLImageHelpers.h"
#include "GLReadTexImageHelper.h"

namespace mozilla {
namespace layers {

using namespace android;

static gfx::SurfaceFormat
SurfaceFormatForAndroidPixelFormat(android::PixelFormat aFormat,
                                   TextureFlags aFlags)
{
  bool swapRB = bool(aFlags & TextureFlags::RB_SWAPPED);
  switch (aFormat) {
  case android::PIXEL_FORMAT_BGRA_8888:
    return swapRB ? gfx::SurfaceFormat::R8G8B8A8 : gfx::SurfaceFormat::B8G8R8A8;
  case android::PIXEL_FORMAT_RGBA_8888:
    return swapRB ? gfx::SurfaceFormat::B8G8R8A8 : gfx::SurfaceFormat::R8G8B8A8;
  case android::PIXEL_FORMAT_RGBX_8888:
    return swapRB ? gfx::SurfaceFormat::B8G8R8X8 : gfx::SurfaceFormat::R8G8B8X8;
  case android::PIXEL_FORMAT_RGB_565:
    return gfx::SurfaceFormat::R5G6B5;
  case HAL_PIXEL_FORMAT_YCbCr_422_SP:
  case HAL_PIXEL_FORMAT_YCrCb_420_SP:
  case HAL_PIXEL_FORMAT_YCbCr_422_I:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
  case HAL_PIXEL_FORMAT_YV12:
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
#endif
    return gfx::SurfaceFormat::R8G8B8A8; 
  default:
    if (aFormat >= 0x100 && aFormat <= 0x1FF) {
      
      return gfx::SurfaceFormat::R8G8B8A8;
    } else {
      
      
      
      
      
      printf_stderr(" xxxxx unknow android format %i\n", (int)aFormat);
      MOZ_ASSERT(false, "Unknown Android pixel format.");
      return gfx::SurfaceFormat::UNKNOWN;
    }
  }
}

static GLenum
TextureTargetForAndroidPixelFormat(android::PixelFormat aFormat)
{
  switch (aFormat) {
  case HAL_PIXEL_FORMAT_YCbCr_422_SP:
  case HAL_PIXEL_FORMAT_YCrCb_420_SP:
  case HAL_PIXEL_FORMAT_YCbCr_422_I:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
  case HAL_PIXEL_FORMAT_YV12:
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
#endif
    return LOCAL_GL_TEXTURE_EXTERNAL;
  case android::PIXEL_FORMAT_BGRA_8888:
  case android::PIXEL_FORMAT_RGBA_8888:
  case android::PIXEL_FORMAT_RGBX_8888:
  case android::PIXEL_FORMAT_RGB_565:
    return LOCAL_GL_TEXTURE_2D;
  default:
    if (aFormat >= 0x100 && aFormat <= 0x1FF) {
      
      return LOCAL_GL_TEXTURE_EXTERNAL;
    } else {
      
      
      
      
      
      MOZ_ASSERT(false, "Unknown Android pixel format.");
      return LOCAL_GL_TEXTURE_EXTERNAL;
    }
  }
}

GrallocTextureSourceOGL::GrallocTextureSourceOGL(CompositorOGL* aCompositor,
                                                 GrallocTextureHostOGL* aTextureHost,
                                                 android::GraphicBuffer* aGraphicBuffer,
                                                 gfx::SurfaceFormat aFormat)
  : mCompositor(aCompositor)
  , mTextureHost(aTextureHost)
  , mGraphicBuffer(aGraphicBuffer)
  , mEGLImage(0)
  , mFormat(aFormat)
  , mNeedsReset(true)
{
  MOZ_ASSERT(mGraphicBuffer.get());
}

GrallocTextureSourceOGL::~GrallocTextureSourceOGL()
{
  DeallocateDeviceData();
  mCompositor = nullptr;
}

void
GrallocTextureSourceOGL::BindTexture(GLenum aTextureUnit, gfx::Filter aFilter)
{
  







  MOZ_ASSERT(gl());
  if (!IsValid() || !gl()->MakeCurrent()) {
    return;
  }

  GLuint tex = GetGLTexture();
  GLuint textureTarget = GetTextureTarget();

  gl()->fActiveTexture(aTextureUnit);
  gl()->fBindTexture(textureTarget, tex);

  ApplyFilterToBoundTexture(gl(), aFilter, textureTarget);

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  if (mTextureHost) {
    
    mTextureHost->WaitAcquireFenceSyncComplete();
  }
#endif
}

bool GrallocTextureSourceOGL::Lock()
{
  MOZ_ASSERT(IsValid());
  if (!IsValid()) {
    return false;
  }
  if (!gl()->MakeCurrent()) {
    NS_WARNING("Failed to make the gl context current");
    return false;
  }

  mTexture = mCompositor->GetTemporaryTexture(GetTextureTarget(), LOCAL_GL_TEXTURE0);

  GLuint textureTarget = GetTextureTarget();

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(textureTarget, mTexture);
  if (!mEGLImage) {
    mEGLImage = EGLImageCreateFromNativeBuffer(gl(), mGraphicBuffer->getNativeBuffer());
  }
  gl()->fEGLImageTargetTexture2D(textureTarget, mEGLImage);
  return true;
}

bool
GrallocTextureSourceOGL::IsValid() const
{
  return !!gl() && !!mGraphicBuffer.get() && !!mCompositor;
}

gl::GLContext*
GrallocTextureSourceOGL::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

void
GrallocTextureSourceOGL::SetCompositor(Compositor* aCompositor)
{
  if (mCompositor && !aCompositor) {
    DeallocateDeviceData();
  }
  mCompositor = static_cast<CompositorOGL*>(aCompositor);
}


GLenum
GrallocTextureSourceOGL::GetTextureTarget() const
{
  MOZ_ASSERT(gl());
  MOZ_ASSERT(mGraphicBuffer.get());

  if (!gl() || !mGraphicBuffer.get()) {
    return LOCAL_GL_TEXTURE_EXTERNAL;
  }

  
  
  
  
  
  if (gl()->Renderer() == gl::GLRenderer::SGX530 ||
      gl()->Renderer() == gl::GLRenderer::SGX540) {
    return LOCAL_GL_TEXTURE_EXTERNAL;
  }

  return TextureTargetForAndroidPixelFormat(mGraphicBuffer->getPixelFormat());
}

gfx::IntSize
GrallocTextureSourceOGL::GetSize() const
{
  if (!IsValid()) {
    NS_WARNING("Trying to access the size of an invalid GrallocTextureSourceOGL");
    return gfx::IntSize(0, 0);
  }
  return gfx::IntSize(mGraphicBuffer->getWidth(), mGraphicBuffer->getHeight());
}

void
GrallocTextureSourceOGL::DeallocateDeviceData()
{
  if (mEGLImage) {
    MOZ_ASSERT(mCompositor);
    if (!gl() || !gl()->MakeCurrent()) {
      return;
    }
    EGLImageDestroy(gl(), mEGLImage);
    mEGLImage = EGL_NO_IMAGE;
  }
}

GrallocTextureHostOGL::GrallocTextureHostOGL(TextureFlags aFlags,
                                             const NewSurfaceDescriptorGralloc& aDescriptor)
  : TextureHost(aFlags)
  , mGrallocHandle(aDescriptor)
  , mSize(0, 0)
  , mDescriptorSize(aDescriptor.size())
  , mFormat(gfx::SurfaceFormat::UNKNOWN)
  , mEGLImage(EGL_NO_IMAGE)
{
  android::GraphicBuffer* graphicBuffer = GetGraphicBufferFromDesc(mGrallocHandle).get();
  MOZ_ASSERT(graphicBuffer);

  if (graphicBuffer) {
    mFormat =
      SurfaceFormatForAndroidPixelFormat(graphicBuffer->getPixelFormat(),
                                         aFlags & TextureFlags::RB_SWAPPED);
    mSize = gfx::IntSize(graphicBuffer->getWidth(), graphicBuffer->getHeight());
  } else {
    printf_stderr("gralloc buffer is nullptr");
  }
}

GrallocTextureHostOGL::~GrallocTextureHostOGL()
{}

void
GrallocTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  mCompositor = static_cast<CompositorOGL*>(aCompositor);
  if (mTilingTextureSource) {
    mTilingTextureSource->SetCompositor(mCompositor);
  }
  if (mGLTextureSource) {
    mGLTextureSource->SetCompositor(mCompositor);
  }

  if (mCompositor && aCompositor != mCompositor) {
    DestroyEGLImage();
  }
}

bool
GrallocTextureHostOGL::Lock()
{
  return IsValid();
}

void
GrallocTextureHostOGL::Unlock()
{
  
}

bool
GrallocTextureHostOGL::IsValid() const
{
  android::GraphicBuffer* graphicBuffer = GetGraphicBufferFromDesc(mGrallocHandle).get();
  return graphicBuffer != nullptr;
}

gfx::SurfaceFormat
GrallocTextureHostOGL::GetFormat() const
{
  return mFormat;
}

void
GrallocTextureHostOGL::DeallocateSharedData()
{
  if (mTilingTextureSource) {
    mTilingTextureSource->ForgetBuffer();
    mTilingTextureSource = nullptr;
  }
  if (mGLTextureSource) {
    mGLTextureSource = nullptr;
  }

  DestroyEGLImage();

  if (mGrallocHandle.buffer().type() != SurfaceDescriptor::Tnull_t) {
    MaybeMagicGrallocBufferHandle handle = mGrallocHandle.buffer();
    base::ProcessId owner;
    if (handle.type() == MaybeMagicGrallocBufferHandle::TGrallocBufferRef) {
      owner = handle.get_GrallocBufferRef().mOwner;
    }
    else {
      owner = handle.get_MagicGrallocBufferHandle().mRef.mOwner;
    }

    SharedBufferManagerParent::DropGrallocBuffer(owner, mGrallocHandle);
  }
}

void
GrallocTextureHostOGL::ForgetSharedData()
{
  if (mTilingTextureSource) {
    mTilingTextureSource->ForgetBuffer();
    mTilingTextureSource = nullptr;
  }
  if (mGLTextureSource) {
    mGLTextureSource = nullptr;
  }
}

void
GrallocTextureHostOGL::DeallocateDeviceData()
{
  if (mTilingTextureSource) {
    mTilingTextureSource->DeallocateDeviceData();
  }
  if (mGLTextureSource) {
    mGLTextureSource = nullptr;
  }
  DestroyEGLImage();
}

LayerRenderState
GrallocTextureHostOGL::GetRenderState()
{
  android::GraphicBuffer* graphicBuffer = GetGraphicBufferFromDesc(mGrallocHandle).get();

  if (graphicBuffer) {
    LayerRenderStateFlags flags = LayerRenderStateFlags::LAYER_RENDER_STATE_DEFAULT;
    if (mFlags & TextureFlags::NEEDS_Y_FLIP) {
      flags |= LayerRenderStateFlags::Y_FLIPPED;
    }
    if (mFlags & TextureFlags::RB_SWAPPED) {
      flags |= LayerRenderStateFlags::FORMAT_RB_SWAP;
    }
    return LayerRenderState(graphicBuffer,
                            gfx::ThebesIntSize(mDescriptorSize),
                            flags,
                            this);
  }

  return LayerRenderState();
}

TemporaryRef<gfx::DataSourceSurface>
GrallocTextureHostOGL::GetAsSurface() {
  return mTilingTextureSource ? mTilingTextureSource->GetAsSurface()
                              : nullptr;
}

TemporaryRef<gfx::DataSourceSurface>
GrallocTextureSourceOGL::GetAsSurface() {
  if (!IsValid() || !gl()->MakeCurrent()) {
    return nullptr;
  }

  GLuint tex = GetGLTexture();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(GetTextureTarget(), tex);
  if (!mEGLImage) {
    mEGLImage = EGLImageCreateFromNativeBuffer(gl(), mGraphicBuffer->getNativeBuffer());
  }
  BindEGLImage();

  RefPtr<gfx::DataSourceSurface> surf =
    IsValid() ? ReadBackSurface(gl(), tex, false, GetFormat())
              : nullptr;

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  return surf.forget();
}

GLuint
GrallocTextureSourceOGL::GetGLTexture()
{
  return mTexture;
}

void
GrallocTextureSourceOGL::BindEGLImage()
{
  gl()->fEGLImageTargetTexture2D(GetTextureTarget(), mEGLImage);
}

TextureSource*
GrallocTextureHostOGL::GetTextureSources()
{
  
  
  MOZ_ASSERT(!mGLTextureSource);
  if (!mTilingTextureSource) {
    android::GraphicBuffer* graphicBuffer = GetGraphicBufferFromDesc(mGrallocHandle).get();
    MOZ_ASSERT(graphicBuffer);
    if (!graphicBuffer) {
      return nullptr;
    }
    mTilingTextureSource = new GrallocTextureSourceOGL(mCompositor, this,
                                                 graphicBuffer, mFormat);
  }
  mTilingTextureSource->Lock();
  return mTilingTextureSource;
}

void
GrallocTextureHostOGL::UnbindTextureSource()
{
  
  
  
  
  
  
  
  
  
  
  mGLTextureSource = nullptr;
}

GLenum GetTextureTarget(gl::GLContext* aGL, android::PixelFormat aFormat) {
  MOZ_ASSERT(aGL);
  if (aGL->Renderer() == gl::GLRenderer::SGX530 ||
      aGL->Renderer() == gl::GLRenderer::SGX540) {
    
    
    
    
    
    return LOCAL_GL_TEXTURE_EXTERNAL;
  } else {
    return TextureTargetForAndroidPixelFormat(aFormat);
  }
}

void
GrallocTextureHostOGL::DestroyEGLImage()
{
  
  
  if (mEGLImage != EGL_NO_IMAGE && GetGLContext()) {
    EGLImageDestroy(GetGLContext(), mEGLImage);
    mEGLImage = EGL_NO_IMAGE;
  }
}

void
GrallocTextureHostOGL::PrepareTextureSource(CompositableTextureSourceRef& aTextureSource)
{
  
  
  
  

  
  
  
  
  
  
  
  

  
  
  
  

  
  
  
  

  MOZ_ASSERT(!mTilingTextureSource);

  android::GraphicBuffer* graphicBuffer = GetGraphicBufferFromDesc(mGrallocHandle).get();

  MOZ_ASSERT(graphicBuffer);
  if (!graphicBuffer) {
    mGLTextureSource = nullptr;
    return;
  }

  if (mGLTextureSource && !mGLTextureSource->IsValid()) {
    mGLTextureSource = nullptr;
  }

  if (mGLTextureSource) {
    
    
    aTextureSource = mGLTextureSource.get();
    return;
  }

  gl::GLContext* gl = GetGLContext();
  if (!gl || !gl->MakeCurrent()) {
    mGLTextureSource = nullptr;
    return;
  }

  if (mEGLImage == EGL_NO_IMAGE) {
    
    mEGLImage = EGLImageCreateFromNativeBuffer(gl, graphicBuffer->getNativeBuffer());
  }

  GLenum textureTarget = GetTextureTarget(gl, graphicBuffer->getPixelFormat());

  GLTextureSource* glSource = aTextureSource.get() ?
    aTextureSource->AsSourceOGL()->AsGLTextureSource() : nullptr;

  bool shouldCreateTextureSource = !glSource  || !glSource->IsValid()
                                 || glSource->NumCompositableRefs() > 1
                                 || glSource->GetTextureTarget() != textureTarget;

  if (shouldCreateTextureSource) {
    GLuint textureHandle;
    gl->fGenTextures(1, &textureHandle);
    gl->fBindTexture(textureTarget, textureHandle);
    gl->fTexParameteri(textureTarget, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);
    gl->fTexParameteri(textureTarget, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
    gl->fEGLImageTargetTexture2D(textureTarget, mEGLImage);

    mGLTextureSource = new GLTextureSource(mCompositor, textureHandle, textureTarget,
                                           mSize, mFormat);
    aTextureSource = mGLTextureSource.get();
  } else {
    gl->fBindTexture(textureTarget, glSource->GetTextureHandle());

    gl->fEGLImageTargetTexture2D(textureTarget, mEGLImage);
    glSource->SetSize(mSize);
    glSource->SetFormat(mFormat);
    mGLTextureSource = glSource;
  }
}

bool
GrallocTextureHostOGL::BindTextureSource(CompositableTextureSourceRef& aTextureSource)
{
  

  
  if (!mGLTextureSource) {
    return false;
  }

  
  
  
  MOZ_ASSERT(mGLTextureSource == aTextureSource);
  aTextureSource = mGLTextureSource.get();

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  
  WaitAcquireFenceSyncComplete();
#endif
  return true;
}

} 
} 
