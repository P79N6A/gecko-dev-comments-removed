




#include "GLContext.h"
#include "gfxImageSurface.h"
#include "gfx2DGlue.h"
#include <ui/GraphicBuffer.h>
#include "GrallocImages.h"  
#include "mozilla/layers/GrallocTextureHost.h"
#include "mozilla/layers/CompositorOGL.h"

namespace mozilla {
namespace layers {

using namespace android;

static gfx::SurfaceFormat
SurfaceFormatForAndroidPixelFormat(android::PixelFormat aFormat,
                                   bool swapRB = false)
{
  switch (aFormat) {
  case android::PIXEL_FORMAT_BGRA_8888:
    return swapRB ? gfx::FORMAT_R8G8B8A8 : gfx::FORMAT_B8G8R8A8;
  case android::PIXEL_FORMAT_RGBA_8888:
    return swapRB ? gfx::FORMAT_B8G8R8A8 : gfx::FORMAT_R8G8B8A8;
  case android::PIXEL_FORMAT_RGBX_8888:
    return swapRB ? gfx::FORMAT_B8G8R8X8 : gfx::FORMAT_R8G8B8X8;
  case android::PIXEL_FORMAT_RGB_565:
    return gfx::FORMAT_R5G6B5;
  case android::PIXEL_FORMAT_A_8:
    return gfx::FORMAT_A8;
  case HAL_PIXEL_FORMAT_YCbCr_422_SP:
  case HAL_PIXEL_FORMAT_YCrCb_420_SP:
  case HAL_PIXEL_FORMAT_YCbCr_422_I:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
  case GrallocImage::HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
  case HAL_PIXEL_FORMAT_YV12:
    return gfx::FORMAT_B8G8R8A8; 
  default:
    if (aFormat >= 0x100 && aFormat <= 0x1FF) {
      
      return gfx::FORMAT_B8G8R8A8;
    } else {
      
      
      
      
      
      printf_stderr(" xxxxx unknow android format %i\n", (int)aFormat);
      MOZ_ASSERT(false, "Unknown Android pixel format.");
      return gfx::FORMAT_UNKNOWN;
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
    return LOCAL_GL_TEXTURE_EXTERNAL;
  case android::PIXEL_FORMAT_RGBA_8888:
  case android::PIXEL_FORMAT_RGBX_8888:
  case android::PIXEL_FORMAT_RGB_565:
  case android::PIXEL_FORMAT_A_8:
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
                                                 android::GraphicBuffer* aGraphicBuffer,
                                                 gfx::SurfaceFormat aFormat)
  : mCompositor(aCompositor)
  , mGraphicBuffer(aGraphicBuffer)
  , mEGLImage(0)
  , mFormat(aFormat)
{
  MOZ_ASSERT(mGraphicBuffer.get());
}

GrallocTextureSourceOGL::~GrallocTextureSourceOGL()
{
  DeallocateDeviceData();
  mCompositor = nullptr;
}

void GrallocTextureSourceOGL::BindTexture(GLenum aTextureUnit)
{
  







  MOZ_ASSERT(gl());
  gl()->MakeCurrent();

  GLuint tex = GetGLTexture();
  GLuint textureTarget = GetTextureTarget();

  gl()->fActiveTexture(aTextureUnit);
  gl()->fBindTexture(textureTarget, tex);
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
}

bool
GrallocTextureSourceOGL::IsValid() const
{
  return !!gl() && !!mGraphicBuffer.get();
}

gl::GLContext*
GrallocTextureSourceOGL::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

void
GrallocTextureSourceOGL::SetCompositor(CompositorOGL* aCompositor)
{
  if (mCompositor && !aCompositor) {
    DeallocateDeviceData();
  }
  mCompositor = aCompositor;
}


GLenum
GrallocTextureSourceOGL::GetTextureTarget() const
{
  MOZ_ASSERT(mGraphicBuffer.get());
  return TextureTargetForAndroidPixelFormat(mGraphicBuffer->getPixelFormat());
}

gfx::SurfaceFormat
GrallocTextureSourceOGL::GetFormat() const {
  if (!mGraphicBuffer.get()) {
    return gfx::FORMAT_UNKNOWN;
  }
  if (GetTextureTarget() == LOCAL_GL_TEXTURE_EXTERNAL) {
    return gfx::FORMAT_R8G8B8A8;
  }
  return mFormat;
}

void
GrallocTextureSourceOGL::SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData)
{
  mCompositableBackendData = aBackendData;

  if (!mCompositor) {
    return;
  }

  
  DeallocateDeviceData();

  gl()->MakeCurrent();
  GLuint tex = GetGLTexture();
  GLuint textureTarget = GetTextureTarget();

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(textureTarget, tex);
  
  mEGLImage = gl()->CreateEGLImageForNativeBuffer(mGraphicBuffer->getNativeBuffer());
  gl()->fEGLImageTargetTexture2D(textureTarget, mEGLImage);
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
    MOZ_ASSERT(gl());
    gl()->MakeCurrent();
    gl()->DestroyEGLImage(mEGLImage);
    mEGLImage = 0;
  }
}

GrallocTextureHostOGL::GrallocTextureHostOGL(uint64_t aID,
                                             TextureFlags aFlags,
                                             const NewSurfaceDescriptorGralloc& aDescriptor)
  : TextureHost(aID, aFlags)
{
  mGrallocActor =
    static_cast<GrallocBufferActor*>(aDescriptor.bufferParent());

  android::GraphicBuffer* graphicBuffer = mGrallocActor->GetGraphicBuffer();

  mSize = aDescriptor.size();
  gfx::SurfaceFormat format =
    SurfaceFormatForAndroidPixelFormat(graphicBuffer->getPixelFormat(),
                                     aFlags & TEXTURE_RB_SWAPPED);
  mTextureSource = new GrallocTextureSourceOGL(nullptr,
                                               graphicBuffer,
                                               format);
}

GrallocTextureHostOGL::~GrallocTextureHostOGL()
{
  mTextureSource = nullptr;
}

void
GrallocTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  mTextureSource->SetCompositor(static_cast<CompositorOGL*>(aCompositor));
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
  return mTextureSource->IsValid();
}

gfx::SurfaceFormat
GrallocTextureHostOGL::GetFormat() const
{
  return mTextureSource->GetFormat();
}

void
GrallocTextureHostOGL::DeallocateSharedData()
{
  if (mTextureSource) {
    mTextureSource->ForgetBuffer();
  }
  PGrallocBufferParent::Send__delete__(mGrallocActor);
}

void
GrallocTextureHostOGL::DeallocateDeviceData()
{
  mTextureSource->DeallocateDeviceData();
}

LayerRenderState
GrallocTextureHostOGL::GetRenderState()
{
  if (IsValid()) {
    uint32_t flags = 0;
    if (mFlags & TEXTURE_NEEDS_Y_FLIP) {
      flags |= LAYER_RENDER_STATE_Y_FLIPPED;
    }
    if (mFlags & TEXTURE_RB_SWAPPED) {
      flags |= LAYER_RENDER_STATE_FORMAT_RB_SWAP;
    }
    return LayerRenderState(mTextureSource->mGraphicBuffer.get(),
                            gfx::ThebesIntSize(mSize),
                            flags);
  }

  return LayerRenderState();
}

already_AddRefed<gfxImageSurface>
GrallocTextureHostOGL::GetAsSurface() {
  return mTextureSource ? mTextureSource->GetAsSurface()
                        : nullptr;
}

already_AddRefed<gfxImageSurface>
GrallocTextureSourceOGL::GetAsSurface() {
  MOZ_ASSERT(gl());
  gl()->MakeCurrent();

  GLuint tex = GetGLTexture();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(GetTextureTarget(), tex);
  if (!mEGLImage) {
    mEGLImage = gl()->CreateEGLImageForNativeBuffer(mGraphicBuffer->getNativeBuffer());
  }
  gl()->fEGLImageTargetTexture2D(GetTextureTarget(), mEGLImage);

  nsRefPtr<gfxImageSurface> surf = IsValid() ? gl()->GetTexImage(tex, false, GetFormat())
                                             : nullptr;

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  return surf.forget();
}

GLuint
GrallocTextureSourceOGL::GetGLTexture()
{
  mCompositableBackendData->SetCompositor(mCompositor);
  return static_cast<CompositableDataGonkOGL*>(mCompositableBackendData.get())->GetTexture();
}

void
GrallocTextureHostOGL::SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData)
{
  mCompositableBackendData = aBackendData;
  if (mTextureSource) {
    mTextureSource->SetCompositableBackendSpecificData(aBackendData);
  }
}

} 
} 
