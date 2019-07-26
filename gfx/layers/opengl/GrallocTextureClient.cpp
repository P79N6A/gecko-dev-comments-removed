




#ifdef MOZ_WIDGET_GONK

#include "mozilla/gfx/2D.h"
#include "mozilla/layers/GrallocTextureClient.h"
#include "mozilla/layers/CompositableClient.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include "gfx2DGlue.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"            
#include "GrallocImages.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;
using namespace android;

class GraphicBufferLockedTextureClientData : public TextureClientData {
public:
  GraphicBufferLockedTextureClientData(GraphicBufferLocked* aBufferLocked)
    : mBufferLocked(aBufferLocked)
  {
    MOZ_COUNT_CTOR(GrallocTextureClientData);
  }

  ~GraphicBufferLockedTextureClientData()
  {
    MOZ_COUNT_DTOR(GrallocTextureClientData);
    MOZ_ASSERT(!mBufferLocked, "Forgot to unlock the GraphicBufferLocked?");
  }

  virtual void DeallocateSharedData(ISurfaceAllocator*) MOZ_OVERRIDE
  {
    mBufferLocked = nullptr;
  }

private:
  RefPtr<GraphicBufferLocked> mBufferLocked;
};

class GrallocTextureClientData : public TextureClientData {
public:
  GrallocTextureClientData(GrallocBufferActor* aActor)
    : mGrallocActor(aActor)
  {
    MOZ_COUNT_CTOR(GrallocTextureClientData);
  }

  ~GrallocTextureClientData()
  {
    MOZ_COUNT_DTOR(GrallocTextureClientData);
    MOZ_ASSERT(!mGrallocActor, "Forgot to unlock the GraphicBufferLocked?");
  }

  virtual void DeallocateSharedData(ISurfaceAllocator* allocator) MOZ_OVERRIDE
  {
    
    
    SurfaceDescriptor sd = SurfaceDescriptorGralloc(nullptr, mGrallocActor,
                                                    IntSize(0, 0),
                                                    false, false);
    allocator->DestroySharedSurface(&sd);
    mGrallocActor = nullptr;
  }

private:
  GrallocBufferActor* mGrallocActor;
};

TextureClientData*
GrallocTextureClientOGL::DropTextureData()
{
  if (mBufferLocked) {
    TextureClientData* result = new GraphicBufferLockedTextureClientData(mBufferLocked);
    mBufferLocked = nullptr;
    mGrallocActor = nullptr;
    mGraphicBuffer = nullptr;
    return result;
  } else {
    TextureClientData* result = new GrallocTextureClientData(mGrallocActor);
    mGrallocActor = nullptr;
    mGraphicBuffer = nullptr;
    return result;
  }
}

GrallocTextureClientOGL::GrallocTextureClientOGL(GrallocBufferActor* aActor,
                                                 gfx::IntSize aSize,
                                                 TextureFlags aFlags)
: BufferTextureClient(nullptr, gfx::SurfaceFormat::UNKNOWN, aFlags)
, mAllocator(nullptr)
, mMappedBuffer(nullptr)
{
  InitWith(aActor, aSize);
  MOZ_COUNT_CTOR(GrallocTextureClientOGL);
}

GrallocTextureClientOGL::GrallocTextureClientOGL(CompositableClient* aCompositable,
                                                 gfx::SurfaceFormat aFormat,
                                                 TextureFlags aFlags)
: BufferTextureClient(aCompositable, aFormat, aFlags)
, mAllocator(nullptr)
, mMappedBuffer(nullptr)
{
  MOZ_COUNT_CTOR(GrallocTextureClientOGL);
}

GrallocTextureClientOGL::GrallocTextureClientOGL(ISurfaceAllocator* aAllocator,
                                                 gfx::SurfaceFormat aFormat,
                                                 TextureFlags aFlags)
: BufferTextureClient(nullptr, aFormat, aFlags)
, mAllocator(aAllocator)
, mMappedBuffer(nullptr)
{
  MOZ_COUNT_CTOR(GrallocTextureClientOGL);
}

GrallocTextureClientOGL::~GrallocTextureClientOGL()
{
  MOZ_COUNT_DTOR(GrallocTextureClientOGL);
    if (ShouldDeallocateInDestructor()) {
    
    
    if (!mBufferLocked) {
      
      
      SurfaceDescriptor sd = SurfaceDescriptorGralloc(nullptr, mGrallocActor,
                                                      IntSize(0, 0),
                                                      false, false);

      ISurfaceAllocator* allocator = GetAllocator();
      allocator->DestroySharedSurface(&sd);
    }
  }
}

void
GrallocTextureClientOGL::InitWith(GrallocBufferActor* aActor, gfx::IntSize aSize)
{
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(!IsAllocated());
  MOZ_ASSERT(IsValid());
  mGrallocActor = aActor;
  mGraphicBuffer = aActor->GetGraphicBuffer();
  mSize = aSize;
}

void
GrallocTextureClientOGL::SetGraphicBufferLocked(GraphicBufferLocked* aBufferLocked)
{
  mBufferLocked = aBufferLocked;
}

bool
GrallocTextureClientOGL::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated()) {
    return false;
  }

  aOutDescriptor = NewSurfaceDescriptorGralloc(nullptr, mGrallocActor, mSize);
  return true;
}

 bool
GrallocTextureClientOGL::UpdateSurface(gfxASurface* aSurface)
{
  MOZ_ASSERT(aSurface);
  MOZ_ASSERT(!IsImmutable());
  MOZ_ASSERT(IsValid());
  if (!IsValid() || !IsAllocated()) {
    return false;
  }

  if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
    RefPtr<DrawTarget> dt = GetAsDrawTarget();
    RefPtr<SourceSurface> source = gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(dt, aSurface);

    dt->CopySurface(source, IntRect(IntPoint(), GetSize()), IntPoint());
  } else {
    nsRefPtr<gfxASurface> surf = GetAsSurface();
    nsRefPtr<gfxContext> tmpCtx = new gfxContext(surf.get());
    tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx->DrawSurface(aSurface, gfxSize(GetSize().width,
                                          GetSize().height));
  }

  return true;
}

bool
GrallocTextureClientOGL::Lock(OpenMode aMode)
{
  MOZ_ASSERT(IsValid());
  if (!IsValid() || !IsAllocated()) {
    return false;
  }

  if (mMappedBuffer) {
    return true;
  }
  uint32_t usage = 0;
  if (aMode & OPEN_READ) {
    usage |= GRALLOC_USAGE_SW_READ_OFTEN;
  }
  if (aMode & OPEN_WRITE) {
    usage |= GRALLOC_USAGE_SW_WRITE_OFTEN;
  }
  int32_t rv = mGraphicBuffer->lock(usage, reinterpret_cast<void**>(&mMappedBuffer));
  if (rv) {
    mMappedBuffer = nullptr;
    NS_WARNING("Couldn't lock graphic buffer");
    return false;
  }
  return BufferTextureClient::Lock(aMode);
}

void
GrallocTextureClientOGL::Unlock()
{
  BufferTextureClient::Unlock();
  mDrawTarget = nullptr;
  if (mMappedBuffer) {
    mMappedBuffer = nullptr;
    mGraphicBuffer->unlock();
  }
}

uint8_t*
GrallocTextureClientOGL::GetBuffer() const
{
  MOZ_ASSERT(IsValid());
  NS_WARN_IF_FALSE(mMappedBuffer, "Trying to get a gralloc buffer without getting the lock?");
  return mMappedBuffer;
}

static gfx::SurfaceFormat
SurfaceFormatForPixelFormat(android::PixelFormat aFormat)
{
  switch (aFormat) {
  case PIXEL_FORMAT_RGBA_8888:
    return gfx::SurfaceFormat::R8G8B8A8;
  case PIXEL_FORMAT_BGRA_8888:
    return gfx::SurfaceFormat::B8G8R8A8;
  case PIXEL_FORMAT_RGBX_8888:
    return gfx::SurfaceFormat::R8G8B8X8;
  case PIXEL_FORMAT_RGB_565:
    return gfx::SurfaceFormat::R5G6B5;
  default:
    MOZ_CRASH("Unknown gralloc pixel format");
  }
  return gfx::SurfaceFormat::R8G8B8A8;
}

TemporaryRef<gfx::DrawTarget>
GrallocTextureClientOGL::GetAsDrawTarget()
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(mMappedBuffer, "Calling TextureClient::GetAsDrawTarget without locking :(");

  if (mDrawTarget) {
    return mDrawTarget;
  }

  gfx::SurfaceFormat format = SurfaceFormatForPixelFormat(mGraphicBuffer->getPixelFormat());
  long pixelStride = mGraphicBuffer->getStride();
  long byteStride = pixelStride * BytesPerPixel(format);
  mDrawTarget = gfxPlatform::GetPlatform()->CreateDrawTargetForData(GetBuffer(),
                                                                    mSize,
                                                                    byteStride,
                                                                    mFormat);
  return mDrawTarget;
}

already_AddRefed<gfxASurface>
GrallocTextureClientOGL::GetAsSurface()
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(mMappedBuffer, "Calling TextureClient::GetAsSurface without locking :(");

  gfx::SurfaceFormat format = SurfaceFormatForPixelFormat(mGraphicBuffer->getPixelFormat());
  long pixelStride = mGraphicBuffer->getStride();
  long byteStride = pixelStride * BytesPerPixel(format);
  nsRefPtr<gfxImageSurface> surface =
                     new gfxImageSurface(GetBuffer(),
                                         gfxIntSize(mSize.width, mSize.height),
                                         byteStride,
                                         SurfaceFormatToImageFormat(mFormat));
  return surface.forget();
}

bool
GrallocTextureClientOGL::AllocateForSurface(gfx::IntSize aSize,
                                            TextureAllocationFlags)
{
  MOZ_ASSERT(IsValid());

  uint32_t format;
  uint32_t usage = android::GraphicBuffer::USAGE_SW_READ_OFTEN |
                   android::GraphicBuffer::USAGE_SW_WRITE_OFTEN |
                   android::GraphicBuffer::USAGE_HW_TEXTURE;

  switch (mFormat) {
  case gfx::SurfaceFormat::R8G8B8A8:
    format = android::PIXEL_FORMAT_RGBA_8888;
    break;
  case gfx::SurfaceFormat::B8G8R8A8:
     format = android::PIXEL_FORMAT_RGBA_8888;
     mFlags |= TEXTURE_RB_SWAPPED;
    break;
  case gfx::SurfaceFormat::R8G8B8X8:
    format = android::PIXEL_FORMAT_RGBX_8888;
    break;
  case gfx::SurfaceFormat::B8G8R8X8:
     format = android::PIXEL_FORMAT_RGBX_8888;
     mFlags |= TEXTURE_RB_SWAPPED;
    break;
  case gfx::SurfaceFormat::R5G6B5:
    format = android::PIXEL_FORMAT_RGB_565;
    break;
  case gfx::SurfaceFormat::A8:
    NS_WARNING("gralloc does not support gfx::SurfaceFormat::A8");
    return false;
  default:
    NS_WARNING("Unsupported surface format");
    return false;
  }

  return AllocateGralloc(aSize, format, usage);
}

bool
GrallocTextureClientOGL::AllocateForYCbCr(gfx::IntSize aYSize, gfx::IntSize aCbCrSize, StereoMode aStereoMode)
{
  MOZ_ASSERT(IsValid());
  return AllocateGralloc(aYSize,
                         HAL_PIXEL_FORMAT_YV12,
                         android::GraphicBuffer::USAGE_SW_READ_OFTEN);
}

bool
GrallocTextureClientOGL::AllocateForGLRendering(gfx::IntSize aSize)
{
  MOZ_ASSERT(IsValid());

  uint32_t format;
  uint32_t usage = android::GraphicBuffer::USAGE_HW_RENDER |
                   android::GraphicBuffer::USAGE_HW_TEXTURE;

  switch (mFormat) {
  case gfx::SurfaceFormat::R8G8B8A8:
  case gfx::SurfaceFormat::B8G8R8A8:
    format = android::PIXEL_FORMAT_RGBA_8888;
    break;
  case gfx::SurfaceFormat::R8G8B8X8:
  case gfx::SurfaceFormat::B8G8R8X8:
    
    format = android::PIXEL_FORMAT_RGBX_8888;
    break;
  case gfx::SurfaceFormat::R5G6B5:
    format = android::PIXEL_FORMAT_RGB_565;
    break;
  default:
    NS_WARNING("Unsupported surface format");
    return false;
  }

  return AllocateGralloc(aSize, format, usage);
}

bool
GrallocTextureClientOGL::AllocateGralloc(gfx::IntSize aSize,
                                         uint32_t aAndroidFormat,
                                         uint32_t aUsage)
{
  MOZ_ASSERT(IsValid());
  ISurfaceAllocator* allocator = GetAllocator();

  MaybeMagicGrallocBufferHandle handle;
  PGrallocBufferChild* actor =
    allocator->AllocGrallocBuffer(aSize,
                                  aAndroidFormat,
                                  aUsage,
                                  &handle);
  if (!actor) {
    return false;
  }
  GrallocBufferActor* gba = static_cast<GrallocBufferActor*>(actor);
  gba->InitFromHandle(handle.get_MagicGrallocBufferHandle());

  sp<GraphicBuffer> graphicBuffer = gba->GetGraphicBuffer();
  if (!graphicBuffer.get()) {
    return false;
  }

  if (graphicBuffer->initCheck() != NO_ERROR) {
    return false;
  }

  mGrallocActor = gba;
  mGraphicBuffer = graphicBuffer;
  mSize = aSize;
  return true;
}

bool
GrallocTextureClientOGL::IsAllocated() const
{
  return !!mGraphicBuffer.get();
}

bool
GrallocTextureClientOGL::Allocate(uint32_t aSize)
{
  
  MOZ_CRASH("This method should never be called.");
  return false;
}

size_t
GrallocTextureClientOGL::GetBufferSize() const
{
  
  MOZ_CRASH("This method should never be called.");
  return 0;
}

ISurfaceAllocator*
GrallocTextureClientOGL::GetAllocator()
{
  MOZ_ASSERT(mCompositable || mAllocator);
  return mCompositable ?
         mCompositable->GetForwarder() :
         mAllocator;
}

} 
} 

#endif 
