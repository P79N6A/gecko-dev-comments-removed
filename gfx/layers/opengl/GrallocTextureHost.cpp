




#include "base/process.h"
#include "GLContext.h"
#include "gfx2DGlue.h"
#include <ui/GraphicBuffer.h>
#include "GrallocImages.h"  
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/DataSurfaceHelpers.h"
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

GrallocTextureHostOGL::GrallocTextureHostOGL(TextureFlags aFlags,
                                             const NewSurfaceDescriptorGralloc& aDescriptor)
  : TextureHost(aFlags)
  , mGrallocHandle(aDescriptor)
  , mSize(0, 0)
  , mDescriptorSize(aDescriptor.size())
  , mFormat(gfx::SurfaceFormat::UNKNOWN)
  , mEGLImage(EGL_NO_IMAGE)
  , mIsOpaque(aDescriptor.isOpaque())
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
{
  DestroyEGLImage();
}

void
GrallocTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  mCompositor = static_cast<CompositorOGL*>(aCompositor);
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
  if (mGLTextureSource) {
    mGLTextureSource = nullptr;
  }

  DestroyEGLImage();

  if (mGrallocHandle.buffer().type() != MaybeMagicGrallocBufferHandle::Tnull_t) {
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
  if (mGLTextureSource) {
    mGLTextureSource = nullptr;
  }
}

void
GrallocTextureHostOGL::DeallocateDeviceData()
{
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
    if (mIsOpaque) {
      flags |= LayerRenderStateFlags::OPAQUE;
    }
    if (mFlags & TextureFlags::ORIGIN_BOTTOM_LEFT) {
      flags |= LayerRenderStateFlags::ORIGIN_BOTTOM_LEFT;
    }
    if (mFlags & TextureFlags::RB_SWAPPED) {
      flags |= LayerRenderStateFlags::FORMAT_RB_SWAP;
    }
    return LayerRenderState(graphicBuffer,
                            mDescriptorSize,
                            flags,
                            this);
  }

  return LayerRenderState();
}

TemporaryRef<gfx::DataSourceSurface>
GrallocTextureHostOGL::GetAsSurface() {
  android::GraphicBuffer* graphicBuffer = GetGraphicBufferFromDesc(mGrallocHandle).get();
  uint8_t* grallocData;
  graphicBuffer->lock(GRALLOC_USAGE_SW_READ_OFTEN, reinterpret_cast<void**>(&grallocData));
  RefPtr<gfx::DataSourceSurface> grallocTempSurf =
    gfx::Factory::CreateWrappingDataSourceSurface(grallocData,
                                                  graphicBuffer->getStride() * android::bytesPerPixel(graphicBuffer->getPixelFormat()),
                                                  GetSize(), GetFormat());
  RefPtr<gfx::DataSourceSurface> surf = CreateDataSourceSurfaceByCloning(grallocTempSurf);

  graphicBuffer->unlock();

  return surf.forget();
}

void
GrallocTextureHostOGL::UnbindTextureSource()
{
  
  
  
  
  
  
  
  
  
  
  mGLTextureSource = nullptr;
}

FenceHandle
GrallocTextureHostOGL::GetAndResetReleaseFenceHandle()
{
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  android::sp<android::Fence> fence = GetAndResetReleaseFence();
  if (fence.get() && fence->isValid()) {
    FenceHandle handle = FenceHandle(fence);
    return handle;
  }
#endif
  return FenceHandle();
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
