




#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/GrallocTextureClient.h"
#include "mozilla/layers/CompositableClient.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include "GrallocImages.h"
#include "gfx2DGlue.h"

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
    mBufferLocked->Unlock();
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
: BufferTextureClient(nullptr, gfx::FORMAT_UNKNOWN, aFlags)
, mGrallocFlags(android::GraphicBuffer::USAGE_SW_READ_OFTEN)
, mMappedBuffer(nullptr)
{
  InitWith(aActor, aSize);
  MOZ_COUNT_CTOR(GrallocTextureClientOGL);
}

GrallocTextureClientOGL::GrallocTextureClientOGL(CompositableClient* aCompositable,
                                                 gfx::SurfaceFormat aFormat,
                                                 TextureFlags aFlags)
: BufferTextureClient(aCompositable, aFormat, aFlags)
, mGrallocFlags(android::GraphicBuffer::USAGE_SW_READ_OFTEN)
, mMappedBuffer(nullptr)
{
  MOZ_COUNT_CTOR(GrallocTextureClientOGL);
}

GrallocTextureClientOGL::~GrallocTextureClientOGL()
{
  MOZ_COUNT_DTOR(GrallocTextureClientOGL);
    if (ShouldDeallocateInDestructor()) {
    
    
    if (mBufferLocked) {
      mBufferLocked->Unlock();
    } else {
      MOZ_ASSERT(mCompositable);
      
      
      SurfaceDescriptor sd = SurfaceDescriptorGralloc(nullptr, mGrallocActor,
                                                      IntSize(0, 0),
                                                      false, false);
      mCompositable->GetForwarder()->DestroySharedSurface(&sd);
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
GrallocTextureClientOGL::Lock(OpenMode aMode)
{
  MOZ_ASSERT(IsValid());
  
  
  
  int32_t rv = mGraphicBuffer->lock(mGrallocFlags, reinterpret_cast<void**>(&mMappedBuffer));
  if (rv) {
    NS_WARNING("Couldn't lock graphic buffer");
    return false;
  }
  return BufferTextureClient::Lock(aMode);
}

void
GrallocTextureClientOGL::Unlock()
{
  BufferTextureClient::Unlock();
  mMappedBuffer = nullptr;
  mGraphicBuffer->unlock();
}

uint8_t*
GrallocTextureClientOGL::GetBuffer() const
{
  MOZ_ASSERT(IsValid());
  NS_WARN_IF_FALSE(mMappedBuffer, "Trying to get a gralloc buffer without getting the lock?");
  return mMappedBuffer;
}

bool
GrallocTextureClientOGL::AllocateForSurface(gfx::IntSize aSize,
                                            TextureAllocationFlags)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(mCompositable);
  ISurfaceAllocator* allocator = mCompositable->GetForwarder();

  uint32_t format;
  uint32_t usage = android::GraphicBuffer::USAGE_SW_READ_OFTEN;
  bool swapRB = GetFlags() & TEXTURE_RB_SWAPPED;

  switch (mFormat) {
  case gfx::FORMAT_R8G8B8A8:
    format = swapRB ? android::PIXEL_FORMAT_BGRA_8888 : android::PIXEL_FORMAT_RGBA_8888;
    break;
  case gfx::FORMAT_B8G8R8A8:
    format = swapRB ? android::PIXEL_FORMAT_RGBA_8888 : android::PIXEL_FORMAT_BGRA_8888;
    break;
  case gfx::FORMAT_R8G8B8X8:
  case gfx::FORMAT_B8G8R8X8:
    
    format = android::PIXEL_FORMAT_RGBX_8888;
    break;
  case gfx::FORMAT_R5G6B5:
    format = android::PIXEL_FORMAT_RGB_565;
    break;
  case gfx::FORMAT_A8:
    format = android::PIXEL_FORMAT_A_8;
    break;
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
GrallocTextureClientOGL::AllocateGralloc(gfx::IntSize aSize,
                                         uint32_t aAndroidFormat,
                                         uint32_t aUsage)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(mCompositable);
  ISurfaceAllocator* allocator = mCompositable->GetForwarder();

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

} 
} 

#endif 
