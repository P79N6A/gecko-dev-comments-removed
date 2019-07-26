




#include "mozilla/layers/TextureClient.h"
#include <stdint.h>                     
#include "Layers.h"                     
#include "gfx2DGlue.h"
#include "gfxContext.h"                 
#include "gfxPlatform.h"                
#include "gfxPoint.h"                   
#include "gfxReusableSurfaceWrapper.h"  
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/ipc/SharedMemory.h"   
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/ImageDataSerializer.h"
#include "mozilla/layers/ShadowLayers.h"  
#include "mozilla/layers/SharedPlanarYCbCrImage.h"
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "ImageContainer.h"             
#include "mozilla/gfx/2D.h"
#include "mozilla/layers/TextureClientOGL.h"
#include "mozilla/layers/PTextureChild.h"

#ifdef XP_WIN
#include "mozilla/layers/TextureD3D9.h"
#include "mozilla/layers/TextureD3D11.h"
#include "gfxWindowsPlatform.h"
#include "gfx2DGlue.h"
#endif
#ifdef MOZ_X11
#include "mozilla/layers/TextureClientX11.h"
#ifdef GL_PROVIDER_GLX
#include "GLXLibrary.h"
#endif
#endif

#ifdef MOZ_WIDGET_GONK
#include <cutils/properties.h>
#include "mozilla/layers/GrallocTextureClient.h"
#endif

#ifdef MOZ_ANDROID_OMTC
#  include "gfxReusableImageSurfaceWrapper.h"
#else
#  include "gfxReusableSharedImageSurfaceWrapper.h"
#  include "gfxSharedImageSurface.h"
#endif

#if 0
#define RECYCLE_LOG(...) printf_stderr(__VA_ARGS__)
#else
#define RECYCLE_LOG(...) do { } while (0)
#endif

using namespace mozilla::gl;
using namespace mozilla::gfx;

namespace mozilla {
namespace layers {












class TextureChild MOZ_FINAL : public PTextureChild
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TextureChild)

  TextureChild()
  : mForwarder(nullptr)
  , mTextureData(nullptr)
  , mTextureClient(nullptr)
  , mIPCOpen(false)
  {
  }

  bool Recv__delete__() MOZ_OVERRIDE;

  bool RecvCompositorRecycle()
  {
    RECYCLE_LOG("Receive recycle %p (%p)\n", mTextureClient, mWaitForRecycle.get());
    mWaitForRecycle = nullptr;
    return true;
  }

  void WaitForCompositorRecycle()
  {
    mWaitForRecycle = mTextureClient;
    RECYCLE_LOG("Wait for recycle %p\n", mWaitForRecycle.get());
    SendClientRecycle();
  }

  




  void SetTextureData(TextureClientData* aData)
  {
    mTextureData = aData;
  }

  void DeleteTextureData();

  CompositableForwarder* GetForwarder() { return mForwarder; }

  ISurfaceAllocator* GetAllocator() { return mForwarder; }

  void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  bool IPCOpen() const { return mIPCOpen; }

private:

  
  
  
  
  void AddIPDLReference() {
    MOZ_ASSERT(mIPCOpen == false);
    mIPCOpen = true;
    AddRef();
  }
  void ReleaseIPDLReference() {
    MOZ_ASSERT(mIPCOpen == true);
    mIPCOpen = false;
    Release();
  }

  RefPtr<CompositableForwarder> mForwarder;
  RefPtr<TextureClient> mWaitForRecycle;
  TextureClientData* mTextureData;
  TextureClient* mTextureClient;
  bool mIPCOpen;

  friend class TextureClient;
};

void
TextureChild::DeleteTextureData()
{
  mWaitForRecycle = nullptr;
  if (mTextureData) {
    mTextureData->DeallocateSharedData(GetAllocator());
    delete mTextureData;
    mTextureData = nullptr;
  }
}

bool
TextureChild::Recv__delete__()
{
  DeleteTextureData();
  return true;
}

void
TextureChild::ActorDestroy(ActorDestroyReason why)
{
  if (mTextureClient) {
    mTextureClient->mActor = nullptr;
  }
  mWaitForRecycle = nullptr;
}


PTextureChild*
TextureClient::CreateIPDLActor()
{
  TextureChild* c = new TextureChild();
  c->AddIPDLReference();
  return c;
}


bool
TextureClient::DestroyIPDLActor(PTextureChild* actor)
{
  static_cast<TextureChild*>(actor)->ReleaseIPDLReference();
  return true;
}


TextureClient*
TextureClient::AsTextureClient(PTextureChild* actor)
{
  return actor ? static_cast<TextureChild*>(actor)->mTextureClient : nullptr;
}

void
TextureClient::WaitForCompositorRecycle()
{
  mActor->WaitForCompositorRecycle();
}

bool
TextureClient::InitIPDLActor(CompositableForwarder* aForwarder)
{
  MOZ_ASSERT(aForwarder);
  if (mActor && mActor->GetForwarder() == aForwarder) {
    return true;
  }
  MOZ_ASSERT(!mActor, "Cannot use a texture on several IPC channels.");

  SurfaceDescriptor desc;
  if (!ToSurfaceDescriptor(desc)) {
    return false;
  }

  mActor = static_cast<TextureChild*>(aForwarder->CreateTexture(desc, GetFlags()));
  MOZ_ASSERT(mActor);
  mActor->mForwarder = aForwarder;
  mActor->mTextureClient = this;
  mShared = true;
  return mActor->IPCOpen();
}

PTextureChild*
TextureClient::GetIPDLActor()
{
  return mActor;
}

#ifdef MOZ_WIDGET_GONK
static bool
DisableGralloc(SurfaceFormat aFormat, const gfx::IntSize& aSizeHint)
{
  if (aFormat == gfx::SurfaceFormat::A8) {
    return true;
  }

#if ANDROID_VERSION <= 15
  
  
  
  if (aSizeHint.width < 64 || aSizeHint.height < 32) {
    return true;
  }
#endif

  return false;
}
#endif


TemporaryRef<TextureClient>
TextureClient::CreateTextureClientForDrawing(ISurfaceAllocator* aAllocator,
                                             SurfaceFormat aFormat,
                                             TextureFlags aTextureFlags,
                                             gfx::BackendType aMoz2DBackend,
                                             const gfx::IntSize& aSizeHint)
{
  if (aMoz2DBackend == gfx::BackendType::NONE) {
    aMoz2DBackend = gfxPlatform::GetPlatform()->GetContentBackend();
  }

  RefPtr<TextureClient> result;

#if defined(MOZ_WIDGET_GONK) || defined(XP_WIN)
  int32_t maxTextureSize = aAllocator->GetMaxTextureSize();
#endif

#ifdef XP_WIN
  LayersBackend parentBackend = aAllocator->GetCompositorBackendType();
  if (parentBackend == LayersBackend::LAYERS_D3D11 &&
      (aMoz2DBackend == gfx::BackendType::DIRECT2D ||
        aMoz2DBackend == gfx::BackendType::DIRECT2D1_1) &&
      gfxWindowsPlatform::GetPlatform()->GetD2DDevice() &&
      aSizeHint.width <= maxTextureSize &&
      aSizeHint.height <= maxTextureSize &&
      !(aTextureFlags & TextureFlags::ALLOC_FALLBACK)) {
    result = new TextureClientD3D11(aFormat, aTextureFlags);
  }
  if (parentBackend == LayersBackend::LAYERS_D3D9 &&
      aMoz2DBackend == gfx::BackendType::CAIRO &&
      aAllocator->IsSameProcess() &&
      aSizeHint.width <= maxTextureSize &&
      aSizeHint.height <= maxTextureSize &&
      !(aTextureFlags & TextureFlags::ALLOC_FALLBACK)) {
    if (!gfxWindowsPlatform::GetPlatform()->GetD3D9Device()) {
      result = new DIBTextureClientD3D9(aFormat, aTextureFlags);
    } else {
      result = new CairoTextureClientD3D9(aFormat, aTextureFlags);
    }
  }
#endif

#ifdef MOZ_X11
  LayersBackend parentBackend = aAllocator->GetCompositorBackendType();
  gfxSurfaceType type =
    gfxPlatform::GetPlatform()->ScreenReferenceSurface()->GetType();

  if (parentBackend == LayersBackend::LAYERS_BASIC &&
      aMoz2DBackend == gfx::BackendType::CAIRO &&
      type == gfxSurfaceType::Xlib &&
      !(aTextureFlags & TextureFlags::ALLOC_FALLBACK))
  {
    result = new TextureClientX11(aFormat, aTextureFlags);
  }
#ifdef GL_PROVIDER_GLX
  if (parentBackend == LayersBackend::LAYERS_OPENGL &&
      type == gfxSurfaceType::Xlib &&
      !(aTextureFlags & TextureFlags::ALLOC_FALLBACK) &&
      aFormat != SurfaceFormat::A8 &&
      gl::sGLXLibrary.UseTextureFromPixmap())
  {
    result = new TextureClientX11(aFormat, aTextureFlags);
  }
#endif
#endif

#ifdef MOZ_WIDGET_GONK
  if (!DisableGralloc(aFormat, aSizeHint)) {
    
    
    if (aSizeHint.width <= maxTextureSize && aSizeHint.height <= maxTextureSize) {
      result = new GrallocTextureClientOGL(aAllocator, aFormat, aMoz2DBackend,
                                           aTextureFlags);
    }
  }
#endif

  
  if (!result) {
    result = CreateBufferTextureClient(aAllocator, aFormat, aTextureFlags, aMoz2DBackend);
  }

  MOZ_ASSERT(!result || result->CanExposeDrawTarget(), "texture cannot expose a DrawTarget?");
  return result;
}


TemporaryRef<BufferTextureClient>
TextureClient::CreateBufferTextureClient(ISurfaceAllocator* aAllocator,
                                         SurfaceFormat aFormat,
                                         TextureFlags aTextureFlags,
                                         gfx::BackendType aMoz2DBackend)
{
  if (gfxPlatform::GetPlatform()->PreferMemoryOverShmem()) {
    RefPtr<BufferTextureClient> result = new MemoryTextureClient(aAllocator, aFormat,
                                                                 aMoz2DBackend,
                                                                 aTextureFlags);
    return result.forget();
  }
  RefPtr<BufferTextureClient> result = new ShmemTextureClient(aAllocator, aFormat,
                                                              aMoz2DBackend,
                                                              aTextureFlags);
  return result.forget();
}


class ShmemTextureClientData : public TextureClientData
{
public:
  ShmemTextureClientData(ipc::Shmem& aShmem)
  : mShmem(aShmem)
  {
    MOZ_COUNT_CTOR(ShmemTextureClientData);
  }

  ~ShmemTextureClientData()
  {
    MOZ_COUNT_CTOR(ShmemTextureClientData);
  }

  virtual void DeallocateSharedData(ISurfaceAllocator* allocator)
  {
    allocator->DeallocShmem(mShmem);
    mShmem = ipc::Shmem();
  }

private:
  ipc::Shmem mShmem;
};

class MemoryTextureClientData : public TextureClientData
{
public:
  MemoryTextureClientData(uint8_t* aBuffer)
  : mBuffer(aBuffer)
  {
    MOZ_COUNT_CTOR(MemoryTextureClientData);
  }

  ~MemoryTextureClientData()
  {
    MOZ_ASSERT(!mBuffer, "Forgot to deallocate the shared texture data?");
    MOZ_COUNT_DTOR(MemoryTextureClientData);
  }

  virtual void DeallocateSharedData(ISurfaceAllocator*)
  {
    delete[] mBuffer;
    mBuffer = nullptr;
  }

private:
  uint8_t* mBuffer;
};

TextureClientData*
MemoryTextureClient::DropTextureData()
{
  if (!mBuffer) {
    return nullptr;
  }
  TextureClientData* result = new MemoryTextureClientData(mBuffer);
  MarkInvalid();
  mBuffer = nullptr;
  return result;
}

TextureClientData*
ShmemTextureClient::DropTextureData()
{
  if (!mShmem.IsReadable()) {
    return nullptr;
  }
  TextureClientData* result = new ShmemTextureClientData(mShmem);
  MarkInvalid();
  mShmem = ipc::Shmem();
  return result;
}

TextureClient::TextureClient(TextureFlags aFlags)
  : mFlags(aFlags)
  , mShared(false)
  , mValid(true)
{}

TextureClient::~TextureClient()
{
  
  
}

void TextureClient::ForceRemove()
{
  if (mValid && mActor) {
    if (GetFlags() & TextureFlags::DEALLOCATE_CLIENT) {
      if (mActor->IPCOpen()) {
        mActor->SendClearTextureHostSync();
        mActor->SendRemoveTexture();
      }
    } else {
      if (mActor->IPCOpen()) {
        mActor->SendRemoveTexture();
      }
    }
  }
  MarkInvalid();
}

bool TextureClient::CopyToTextureClient(TextureClient* aTarget,
                                        const gfx::IntRect* aRect,
                                        const gfx::IntPoint* aPoint)
{
  MOZ_ASSERT(IsLocked());
  MOZ_ASSERT(aTarget->IsLocked());

  if (!aTarget->CanExposeDrawTarget() || !CanExposeDrawTarget()) {
    return false;
  }

  RefPtr<DrawTarget> destinationTarget = aTarget->GetAsDrawTarget();
  RefPtr<DrawTarget> sourceTarget = GetAsDrawTarget();
  RefPtr<gfx::SourceSurface> source = sourceTarget->Snapshot();
  destinationTarget->CopySurface(source,
                                 aRect ? *aRect : gfx::IntRect(gfx::IntPoint(0, 0), GetSize()),
                                 aPoint ? *aPoint : gfx::IntPoint(0, 0));
  destinationTarget = nullptr;
  source = nullptr;
  sourceTarget = nullptr;

  return true;
}

void
TextureClient::Finalize()
{
  MOZ_ASSERT(!IsLocked());
  
  
  RefPtr<TextureChild> actor = mActor;

  if (actor) {
    
    
    
    actor->mTextureClient = nullptr;
    
    if (actor->GetForwarder()) {
      actor->GetForwarder()->RemoveTexture(this);
    }
  }
}

bool
TextureClient::ShouldDeallocateInDestructor() const
{
  if (!IsAllocated()) {
    return false;
  }

  
  
  
  return !IsSharedWithCompositor();
}

bool
ShmemTextureClient::ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated() || GetFormat() == gfx::SurfaceFormat::UNKNOWN) {
    return false;
  }

  aDescriptor = SurfaceDescriptorShmem(mShmem, GetFormat());

  return true;
}

bool
ShmemTextureClient::Allocate(uint32_t aSize)
{
  MOZ_ASSERT(mValid);
  ipc::SharedMemory::SharedMemoryType memType = OptimalShmemType();
  mAllocated = GetAllocator()->AllocUnsafeShmem(aSize, memType, &mShmem);
  return mAllocated;
}

uint8_t*
ShmemTextureClient::GetBuffer() const
{
  MOZ_ASSERT(IsValid());
  if (mAllocated) {
    return mShmem.get<uint8_t>();
  }
  return nullptr;
}

size_t
ShmemTextureClient::GetBufferSize() const
{
  MOZ_ASSERT(IsValid());
  return mShmem.Size<uint8_t>();
}

ShmemTextureClient::ShmemTextureClient(ISurfaceAllocator* aAllocator,
                                       gfx::SurfaceFormat aFormat,
                                       gfx::BackendType aMoz2DBackend,
                                       TextureFlags aFlags)
  : BufferTextureClient(aAllocator, aFormat, aMoz2DBackend, aFlags)
  , mAllocated(false)
{
  MOZ_COUNT_CTOR(ShmemTextureClient);
}

ShmemTextureClient::~ShmemTextureClient()
{
  MOZ_COUNT_DTOR(ShmemTextureClient);
  if (ShouldDeallocateInDestructor()) {
    
    
    GetAllocator()->DeallocShmem(mShmem);
  }
}

bool
MemoryTextureClient::ToSurfaceDescriptor(SurfaceDescriptor& aDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated() || GetFormat() == gfx::SurfaceFormat::UNKNOWN) {
    return false;
  }
  aDescriptor = SurfaceDescriptorMemory(reinterpret_cast<uintptr_t>(mBuffer),
                                        GetFormat());
  return true;
}

bool
MemoryTextureClient::Allocate(uint32_t aSize)
{
  MOZ_ASSERT(!mBuffer);
  static const fallible_t fallible = fallible_t();
  mBuffer = new(fallible) uint8_t[aSize];
  if (!mBuffer) {
    NS_WARNING("Failed to allocate buffer");
    return false;
  }
  GfxMemoryImageReporter::DidAlloc(mBuffer);
  mBufSize = aSize;
  return true;
}

MemoryTextureClient::MemoryTextureClient(ISurfaceAllocator* aAllocator,
                                         gfx::SurfaceFormat aFormat,
                                         gfx::BackendType aMoz2DBackend,
                                         TextureFlags aFlags)
  : BufferTextureClient(aAllocator, aFormat, aMoz2DBackend, aFlags)
  , mBuffer(nullptr)
  , mBufSize(0)
{
  MOZ_COUNT_CTOR(MemoryTextureClient);
}

MemoryTextureClient::~MemoryTextureClient()
{
  MOZ_COUNT_DTOR(MemoryTextureClient);
  if (mBuffer && ShouldDeallocateInDestructor()) {
    
    
    GfxMemoryImageReporter::WillFree(mBuffer);
    delete [] mBuffer;
  }
}

BufferTextureClient::BufferTextureClient(ISurfaceAllocator* aAllocator,
                                         gfx::SurfaceFormat aFormat,
                                         gfx::BackendType aMoz2DBackend,
                                         TextureFlags aFlags)
  : TextureClient(aFlags)
  , mAllocator(aAllocator)
  , mFormat(aFormat)
  , mBackend(aMoz2DBackend)
  , mOpenMode(OpenMode::OPEN_NONE)
  , mLocked(false)
{}

BufferTextureClient::~BufferTextureClient()
{}

ISurfaceAllocator*
BufferTextureClient::GetAllocator() const
{
  return mAllocator;
}

bool
BufferTextureClient::AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags aFlags)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(mFormat != gfx::SurfaceFormat::YUV, "This textureClient cannot use YCbCr data");
  MOZ_ASSERT(aSize.width * aSize.height);

  int bufSize
    = ImageDataSerializer::ComputeMinBufferSize(aSize, mFormat);
  if (!Allocate(bufSize)) {
    return false;
  }

  if (aFlags & ALLOC_CLEAR_BUFFER) {
    memset(GetBuffer(), 0, bufSize);
  }

  ImageDataSerializer serializer(GetBuffer(), GetBufferSize());
  serializer.InitializeBufferInfo(aSize, mFormat);
  mSize = aSize;
  return true;
}

TemporaryRef<gfx::DrawTarget>
BufferTextureClient::GetAsDrawTarget()
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(mLocked, "GetAsDrawTarget should be called on locked textures only");

  if (mDrawTarget) {
    return mDrawTarget;
  }

  ImageDataSerializer serializer(GetBuffer(), GetBufferSize());
  if (!serializer.IsValid()) {
    return nullptr;
  }

  mDrawTarget = serializer.GetAsDrawTarget(mBackend);
  if (mDrawTarget) {
    return mDrawTarget;
  }

  mDrawTarget = serializer.GetAsDrawTarget(BackendType::CAIRO);

  return mDrawTarget;
}

bool
BufferTextureClient::Lock(OpenMode aMode)
{
  MOZ_ASSERT(!mLocked, "The TextureClient is already Locked!");
  mOpenMode = aMode;
  mLocked = IsValid() && IsAllocated();;
  return mLocked;
}

void
BufferTextureClient::Unlock()
{
  MOZ_ASSERT(mLocked, "The TextureClient is already Unlocked!");
  mLocked = false;
  if (!mDrawTarget) {
    return;
  }

  
  
  
  
  MOZ_ASSERT(mDrawTarget->refCount() == 1);

  mDrawTarget->Flush();
  mDrawTarget = nullptr;
}

bool
BufferTextureClient::UpdateYCbCr(const PlanarYCbCrData& aData)
{
  MOZ_ASSERT(mLocked);
  MOZ_ASSERT(mFormat == gfx::SurfaceFormat::YUV, "This textureClient can only use YCbCr data");
  MOZ_ASSERT(!IsImmutable());
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(aData.mCbSkip == aData.mCrSkip);

  YCbCrImageDataSerializer serializer(GetBuffer(), GetBufferSize());
  MOZ_ASSERT(serializer.IsValid());
  if (!serializer.CopyData(aData.mYChannel, aData.mCbChannel, aData.mCrChannel,
                           aData.mYSize, aData.mYStride,
                           aData.mCbCrSize, aData.mCbCrStride,
                           aData.mYSkip, aData.mCbSkip)) {
    NS_WARNING("Failed to copy image data!");
    return false;
  }

  if (TextureRequiresLocking(mFlags)) {
    
    
    MarkImmutable();
  }
  return true;
}

bool
BufferTextureClient::AllocateForYCbCr(gfx::IntSize aYSize,
                                      gfx::IntSize aCbCrSize,
                                      StereoMode aStereoMode)
{
  MOZ_ASSERT(IsValid());

  size_t bufSize = YCbCrImageDataSerializer::ComputeMinBufferSize(aYSize,
                                                                  aCbCrSize);
  if (!Allocate(bufSize)) {
    return false;
  }
  YCbCrImageDataSerializer serializer(GetBuffer(), GetBufferSize());
  serializer.InitializeBufferInfo(aYSize,
                                  aCbCrSize,
                                  aStereoMode);
  mSize = aYSize;
  return true;
}

}
}
