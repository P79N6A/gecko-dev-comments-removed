




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
#  include "gfxImageSurface.h"
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












class TextureChild : public PTextureChild
                   , public AtomicRefCounted<TextureChild>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(TextureChild)
  TextureChild()
  : mForwarder(nullptr)
  , mTextureData(nullptr)
  , mTextureClient(nullptr)
  , mIPCOpen(false)
  {
    MOZ_COUNT_CTOR(TextureChild);
  }

  ~TextureChild()
  {
    MOZ_COUNT_DTOR(TextureChild);
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
DisableGralloc(SurfaceFormat aFormat)
{
  if (aFormat == gfx::SurfaceFormat::A8) {
    return true;
  }
#if ANDROID_VERSION <= 15
  static bool checkedDevice = false;
  static bool disableGralloc = false;

  if (!checkedDevice) {
    char propValue[PROPERTY_VALUE_MAX];
    property_get("ro.product.device", propValue, "None");

    if (strcmp("crespo",propValue) == 0) {
      NS_WARNING("Nexus S has issues with gralloc, falling back to shmem");
      disableGralloc = true;
    }

    checkedDevice = true;
  }

  if (disableGralloc) {
    return true;
  }
  return false;
#else
  return false;
#endif
}
#endif


TemporaryRef<TextureClient>
TextureClient::CreateTextureClientForDrawing(ISurfaceAllocator* aAllocator,
                                             SurfaceFormat aFormat,
                                             TextureFlags aTextureFlags)
{
  RefPtr<TextureClient> result;

#ifdef XP_WIN
  LayersBackend parentBackend = aAllocator->GetCompositorBackendType();
  if (parentBackend == LayersBackend::LAYERS_D3D11 && gfxWindowsPlatform::GetPlatform()->GetD2DDevice() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
    result = new TextureClientD3D11(aFormat, aTextureFlags);
  }
  if (parentBackend == LayersBackend::LAYERS_D3D9 &&
      aAllocator->IsSameProcess() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
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
      type == gfxSurfaceType::Xlib &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK))
  {
    result = new TextureClientX11(aFormat, aTextureFlags);
  }
#ifdef GL_PROVIDER_GLX
#if 0
  
  if (parentBackend == LayersBackend::LAYERS_OPENGL &&
      type == gfxSurfaceType::Xlib &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK) &&
      aFormat != SurfaceFormat::A8 &&
      gl::sGLXLibrary.UseTextureFromPixmap())
  {
    result = new TextureClientX11(aFormat, aTextureFlags);
  }
#endif
#endif
#endif

#ifdef MOZ_WIDGET_GONK
  if (!DisableGralloc(aFormat)) {
    result = new GrallocTextureClientOGL(aAllocator, aFormat, aTextureFlags);
  }
#endif

  
  if (!result) {
    result = CreateBufferTextureClient(aAllocator, aFormat, aTextureFlags);
  }

  MOZ_ASSERT(!result || result->AsTextureClientDrawTarget(),
             "Not a TextureClientDrawTarget?");
  return result;
}


TemporaryRef<BufferTextureClient>
TextureClient::CreateBufferTextureClient(ISurfaceAllocator* aAllocator,
                                         SurfaceFormat aFormat,
                                         TextureFlags aTextureFlags)
{
  if (gfxPlatform::GetPlatform()->PreferMemoryOverShmem()) {
    RefPtr<BufferTextureClient> result = new MemoryTextureClient(aAllocator, aFormat,
                                                                 aTextureFlags);
    return result.forget();
  }
  RefPtr<BufferTextureClient> result = new ShmemTextureClient(aAllocator, aFormat,
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
    if (GetFlags() & TEXTURE_DEALLOCATE_CLIENT) {
      mActor->SetTextureData(DropTextureData());
      if (mActor->IPCOpen()) {
        mActor->SendRemoveTextureSync();
      }
      mActor->DeleteTextureData();
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

  if (!aTarget->AsTextureClientDrawTarget() || !AsTextureClientDrawTarget()) {
    return false;
  }

  RefPtr<DrawTarget> destinationTarget = aTarget->AsTextureClientDrawTarget()->GetAsDrawTarget();
  RefPtr<DrawTarget> sourceTarget = AsTextureClientDrawTarget()->GetAsDrawTarget();
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
  
  
  RefPtr<TextureChild> actor = mActor;

  if (actor) {
    
    if (actor->GetForwarder()) {
      actor->GetForwarder()->RemoveTexture(this);
    }
    
    actor->mTextureClient = nullptr;
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
                                       TextureFlags aFlags)
  : BufferTextureClient(aAllocator, aFormat, aFlags)
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
                                         TextureFlags aFlags)
  : BufferTextureClient(aAllocator, aFormat, aFlags)
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
    delete mBuffer;
  }
}

BufferTextureClient::BufferTextureClient(ISurfaceAllocator* aAllocator,
                                         gfx::SurfaceFormat aFormat,
                                         TextureFlags aFlags)
  : TextureClient(aFlags)
  , mAllocator(aAllocator)
  , mFormat(aFormat)
  , mUsingFallbackDrawTarget(false)
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
BufferTextureClient::UpdateSurface(gfxASurface* aSurface)
{
  MOZ_ASSERT(aSurface);
  MOZ_ASSERT(!IsImmutable());
  MOZ_ASSERT(IsValid());

  ImageDataSerializer serializer(GetBuffer(), GetBufferSize());
  if (!serializer.IsValid()) {
    return false;
  }

  if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
    RefPtr<DrawTarget> dt = GetAsDrawTarget();
    RefPtr<SourceSurface> source = gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(dt, aSurface);

    dt->CopySurface(source, IntRect(IntPoint(), serializer.GetSize()), IntPoint());
    
    
    
  } else {
    RefPtr<gfxImageSurface> surf = serializer.GetAsThebesSurface();
    if (!surf) {
      return false;
    }

    nsRefPtr<gfxContext> tmpCtx = new gfxContext(surf.get());
    tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
    tmpCtx->DrawSurface(aSurface, gfxSize(serializer.GetSize().width,
                                          serializer.GetSize().height));
  }

  if (TextureRequiresLocking(mFlags) && !ImplementsLocking()) {
    
    
    MarkImmutable();
  }
  return true;
}

already_AddRefed<gfxASurface>
BufferTextureClient::GetAsSurface()
{
  MOZ_ASSERT(IsValid());

  ImageDataSerializer serializer(GetBuffer(), GetBufferSize());
  if (!serializer.IsValid()) {
    return nullptr;
  }

  RefPtr<gfxImageSurface> surf = serializer.GetAsThebesSurface();
  nsRefPtr<gfxASurface> result = surf.get();
  return result.forget();
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
  
  NS_WARN_IF_FALSE(mLocked, "GetAsDrawTarget should be called on locked textures only");

  if (mDrawTarget) {
    return mDrawTarget;
  }

  ImageDataSerializer serializer(GetBuffer(), GetBufferSize());
  if (!serializer.IsValid()) {
    return nullptr;
  }

  MOZ_ASSERT(mUsingFallbackDrawTarget == false);
  mDrawTarget = serializer.GetAsDrawTarget();
  if (mDrawTarget) {
    return mDrawTarget;
  }

  
  
  mDrawTarget = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
    serializer.GetSize(), serializer.GetFormat());
  if (!mDrawTarget) {
    return nullptr;
  }

  mUsingFallbackDrawTarget = true;
  if (mOpenMode & OPEN_READ) {
    RefPtr<DataSourceSurface> surface = serializer.GetAsSurface();
    IntRect rect(0, 0, surface->GetSize().width, surface->GetSize().height);
    mDrawTarget->CopySurface(surface, rect, IntPoint(0,0));
  }
  return mDrawTarget;
}

bool
BufferTextureClient::Lock(OpenMode aMode)
{
  
  NS_WARN_IF_FALSE(!mLocked, "The TextureClient is already Locked!");
  mOpenMode = aMode;
  mLocked = IsValid() && IsAllocated();;
  return mLocked;
}

void
BufferTextureClient::Unlock()
{
  
  NS_WARN_IF_FALSE(mLocked, "The TextureClient is already Unlocked!");
  mLocked = false;
  if (!mDrawTarget) {
    mUsingFallbackDrawTarget = false;
    return;
  }

  mDrawTarget->Flush();
  if (mUsingFallbackDrawTarget && (mOpenMode & OPEN_WRITE)) {
    
    
    
    
    RefPtr<SourceSurface> snapshot = mDrawTarget->Snapshot();
    RefPtr<DataSourceSurface> surface = snapshot->GetDataSurface();
    ImageDataSerializer serializer(GetBuffer(), GetBufferSize());
    if (!serializer.IsValid() || serializer.GetSize() != surface->GetSize()) {
      NS_WARNING("Could not write the data back into the texture.");
      mDrawTarget = nullptr;
      mUsingFallbackDrawTarget = false;
      return;
    }
    MOZ_ASSERT(surface->GetSize() == serializer.GetSize());
    MOZ_ASSERT(surface->GetFormat() == serializer.GetFormat());
    int bpp = BytesPerPixel(surface->GetFormat());
    for (int i = 0; i < surface->GetSize().height; ++i) {
      memcpy(serializer.GetData() + i*serializer.GetStride(),
             surface->GetData() + i*surface->Stride(),
             surface->GetSize().width * bpp);
    }
  }
  mDrawTarget = nullptr;
  mUsingFallbackDrawTarget = false;
}

bool
BufferTextureClient::UpdateYCbCr(const PlanarYCbCrData& aData)
{
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









DeprecatedTextureClient::DeprecatedTextureClient(CompositableForwarder* aForwarder,
                             const TextureInfo& aTextureInfo)
  : mForwarder(aForwarder)
  , mTextureInfo(aTextureInfo)
  , mAccessMode(ACCESS_READ_WRITE)
{
  MOZ_COUNT_CTOR(DeprecatedTextureClient);
}

DeprecatedTextureClient::~DeprecatedTextureClient()
{
  MOZ_COUNT_DTOR(DeprecatedTextureClient);
  MOZ_ASSERT(mDescriptor.type() == SurfaceDescriptor::T__None, "Need to release surface!");
}

DeprecatedTextureClientShmem::DeprecatedTextureClientShmem(CompositableForwarder* aForwarder,
                                       const TextureInfo& aTextureInfo)
  : DeprecatedTextureClient(aForwarder, aTextureInfo)
{
}

DeprecatedTextureClientShmem::~DeprecatedTextureClientShmem()
{
  ReleaseResources();
}

void
DeprecatedTextureClientShmem::ReleaseResources()
{
  if (mSurface) {
    mSurface = nullptr;
    mSurfaceAsImage = nullptr;

    ShadowLayerForwarder::CloseDescriptor(mDescriptor);
  }

  if (!(mTextureInfo.mTextureFlags & TEXTURE_DEALLOCATE_CLIENT)) {
    mDescriptor = SurfaceDescriptor();
    return;
  }

  if (IsSurfaceDescriptorValid(mDescriptor)) {
    mForwarder->DestroySharedSurface(&mDescriptor);
    mDescriptor = SurfaceDescriptor();
  }
}

bool
DeprecatedTextureClientShmem::EnsureAllocated(gfx::IntSize aSize,
                                    gfxContentType aContentType)
{
  if (aSize != mSize ||
      aContentType != mContentType ||
      !IsSurfaceDescriptorValid(mDescriptor)) {
    ReleaseResources();

    mContentType = aContentType;
    mSize = aSize;

    if (!mForwarder->AllocSurfaceDescriptor(mSize, mContentType,
                                            &mDescriptor)) {
      NS_WARNING("creating SurfaceDescriptor failed!");
    }
    if (mContentType == gfxContentType::COLOR_ALPHA) {
      gfxASurface* surface = GetSurface();
      if (!surface) {
        return false;
      }
      nsRefPtr<gfxContext> context = new gfxContext(surface);
      context->SetColor(gfxRGBA(0, 0, 0, 0));
      context->SetOperator(gfxContext::OPERATOR_SOURCE);
      context->Paint();
    }
  }
  return true;
}

void
DeprecatedTextureClientShmem::SetDescriptor(const SurfaceDescriptor& aDescriptor)
{
  if (aDescriptor.type() == SurfaceDescriptor::Tnull_t) {
    EnsureAllocated(mSize, mContentType);
    return;
  }

  ReleaseResources();
  mDescriptor = aDescriptor;

  MOZ_ASSERT(!mSurface);
  NS_ASSERTION(mDescriptor.type() == SurfaceDescriptor::T__None ||
               mDescriptor.type() == SurfaceDescriptor::TSurfaceDescriptorGralloc ||
               mDescriptor.type() == SurfaceDescriptor::TShmem ||
               mDescriptor.type() == SurfaceDescriptor::TMemoryImage ||
               mDescriptor.type() == SurfaceDescriptor::TRGBImage,
               "Invalid surface descriptor");
}

gfxASurface*
DeprecatedTextureClientShmem::GetSurface()
{
  if (!mSurface) {
    if (!IsSurfaceDescriptorValid(mDescriptor)) {
      return nullptr;
    }
    MOZ_ASSERT(mAccessMode == ACCESS_READ_WRITE || mAccessMode == ACCESS_READ_ONLY);
    OpenMode mode = mAccessMode == ACCESS_READ_WRITE
                    ? OPEN_READ_WRITE
                    : OPEN_READ_ONLY;
    mSurface = ShadowLayerForwarder::OpenDescriptor(mode, mDescriptor);
    MOZ_ASSERT(!mSurface || mSurface->GetContentType() == mContentType);
  }

  return mSurface.get();
}


gfx::DrawTarget*
DeprecatedTextureClientShmem::LockDrawTarget()
{
  if (mDrawTarget) {
    return mDrawTarget;
  }

  gfxASurface* surface = GetSurface();
  if (!surface) {
    return nullptr;
  }

  mDrawTarget = gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(surface, mSize);

  return mDrawTarget;
}

void
DeprecatedTextureClientShmem::Unlock()
{
  if (mSurface) {
    mSurface = nullptr;
    mSurfaceAsImage = nullptr;
    ShadowLayerForwarder::CloseDescriptor(mDescriptor);
  }
  mDrawTarget = nullptr;
}

gfxImageSurface*
DeprecatedTextureClientShmem::LockImageSurface()
{
  if (!mSurfaceAsImage) {
    gfxASurface* surface = GetSurface();
    if (!surface) {
      return nullptr;
    }
    mSurfaceAsImage = surface->GetAsImageSurface();
  }

  return mSurfaceAsImage.get();
}

DeprecatedTextureClientTile::DeprecatedTextureClientTile(const DeprecatedTextureClientTile& aOther)
  : DeprecatedTextureClient(aOther.mForwarder, aOther.mTextureInfo)
  , mSurface(aOther.mSurface)
{}

DeprecatedTextureClientTile::~DeprecatedTextureClientTile()
{}

void
DeprecatedTextureClientShmemYCbCr::ReleaseResources()
{
  GetForwarder()->DestroySharedSurface(&mDescriptor);
}

void
DeprecatedTextureClientShmemYCbCr::SetDescriptor(const SurfaceDescriptor& aDescriptor)
{
  MOZ_ASSERT(aDescriptor.type() == SurfaceDescriptor::TYCbCrImage ||
             aDescriptor.type() == SurfaceDescriptor::T__None);

  if (IsSurfaceDescriptorValid(mDescriptor)) {
    GetForwarder()->DestroySharedSurface(&mDescriptor);
  }
  mDescriptor = aDescriptor;
}

void
DeprecatedTextureClientShmemYCbCr::SetDescriptorFromReply(const SurfaceDescriptor& aDescriptor)
{
  MOZ_ASSERT(aDescriptor.type() == SurfaceDescriptor::TYCbCrImage);
  DeprecatedSharedPlanarYCbCrImage* shYCbCr = DeprecatedSharedPlanarYCbCrImage::FromSurfaceDescriptor(aDescriptor);
  if (shYCbCr) {
    shYCbCr->Release();
    mDescriptor = SurfaceDescriptor();
  } else {
    SetDescriptor(aDescriptor);
  }
}

bool
DeprecatedTextureClientShmemYCbCr::EnsureAllocated(gfx::IntSize aSize,
                                         gfxContentType aType)
{
  NS_RUNTIMEABORT("not enough arguments to do this (need both Y and CbCr sizes)");
  return false;
}


DeprecatedTextureClientTile::DeprecatedTextureClientTile(CompositableForwarder* aForwarder,
                                                         const TextureInfo& aTextureInfo,
                                                         gfxReusableSurfaceWrapper* aSurface)
  : DeprecatedTextureClient(aForwarder, aTextureInfo)
  , mSurface(aSurface)
{
  mTextureInfo.mDeprecatedTextureHostFlags = TEXTURE_HOST_TILED;
}

bool
DeprecatedTextureClientTile::EnsureAllocated(gfx::IntSize aSize, gfxContentType aType)
{
  if (!mSurface ||
      mSurface->Format() != gfxPlatform::GetPlatform()->OptimalFormatForContent(aType)) {
#ifdef MOZ_ANDROID_OMTC
    
    
    
    gfxImageSurface* tmpTile = new gfxImageSurface(gfxIntSize(aSize.width, aSize.height),
                                                   gfxPlatform::GetPlatform()->OptimalFormatForContent(aType),
                                                   aType != gfxContentType::COLOR);
    mSurface = new gfxReusableImageSurfaceWrapper(tmpTile);
#else
    nsRefPtr<gfxSharedImageSurface> sharedImage =
      gfxSharedImageSurface::CreateUnsafe(mForwarder,
                                          gfxIntSize(aSize.width, aSize.height),
                                          gfxPlatform::GetPlatform()->OptimalFormatForContent(aType));
    mSurface = new gfxReusableSharedImageSurfaceWrapper(mForwarder, sharedImage);
#endif
    mContentType = aType;
  }
  return true;
}

gfxImageSurface*
DeprecatedTextureClientTile::LockImageSurface()
{
  
  
  
  gfxImageSurface* writableSurface = nullptr;
  mSurface = mSurface->GetWritable(&writableSurface);
  return writableSurface;
}





bool AutoLockShmemClient::Update(Image* aImage,
                                 uint32_t aContentFlags,
                                 gfxASurface *aSurface)
{
  if (!aImage) {
    return false;
  }

  gfx::IntSize size = aImage->GetSize();

  gfxContentType contentType = aSurface->GetContentType();
  bool isOpaque = (aContentFlags & Layer::CONTENT_OPAQUE);
  if (contentType != gfxContentType::ALPHA &&
      isOpaque) {
    contentType = gfxContentType::COLOR;
  }
  mDeprecatedTextureClient->EnsureAllocated(size, contentType);

  OpenMode mode = mDeprecatedTextureClient->GetAccessMode() == DeprecatedTextureClient::ACCESS_READ_WRITE
                  ? OPEN_READ_WRITE
                  : OPEN_READ_ONLY;
  nsRefPtr<gfxASurface> tmpASurface =
    ShadowLayerForwarder::OpenDescriptor(mode,
                                         *mDeprecatedTextureClient->LockSurfaceDescriptor());
  if (!tmpASurface) {
    return false;
  }
  nsRefPtr<gfxContext> tmpCtx = new gfxContext(tmpASurface.get());
  tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
  tmpCtx->DrawSurface(aSurface, gfxSize(size.width, size.height));

  return true;
}

bool
AutoLockYCbCrClient::Update(PlanarYCbCrImage* aImage)
{
  MOZ_ASSERT(aImage);
  MOZ_ASSERT(mDescriptor);

  const PlanarYCbCrData *data = aImage->GetData();
  NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");
  if (!data) {
    return false;
  }

  if (!EnsureDeprecatedTextureClient(aImage)) {
    return false;
  }

  ipc::Shmem& shmem = mDescriptor->get_YCbCrImage().data();

  YCbCrImageDataSerializer serializer(shmem.get<uint8_t>(), shmem.Size<uint8_t>());
  if (!serializer.CopyData(data->mYChannel, data->mCbChannel, data->mCrChannel,
                           data->mYSize, data->mYStride,
                           data->mCbCrSize, data->mCbCrStride,
                           data->mYSkip, data->mCbSkip)) {
    NS_WARNING("Failed to copy image data!");
    return false;
  }
  return true;
}

bool AutoLockYCbCrClient::EnsureDeprecatedTextureClient(PlanarYCbCrImage* aImage)
{
  MOZ_ASSERT(aImage);
  if (!aImage) {
    return false;
  }

  const PlanarYCbCrData *data = aImage->GetData();
  NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");
  if (!data) {
    return false;
  }

  bool needsAllocation = false;
  if (mDescriptor->type() != SurfaceDescriptor::TYCbCrImage) {
    needsAllocation = true;
  } else {
    ipc::Shmem& shmem = mDescriptor->get_YCbCrImage().data();
    YCbCrImageDataSerializer serializer(shmem.get<uint8_t>(), shmem.Size<uint8_t>());
    if (serializer.GetYSize() != data->mYSize ||
        serializer.GetCbCrSize() != data->mCbCrSize) {
      needsAllocation = true;
    }
  }

  if (!needsAllocation) {
    return true;
  }

  mDeprecatedTextureClient->ReleaseResources();

  ipc::SharedMemory::SharedMemoryType shmType = OptimalShmemType();
  size_t size = YCbCrImageDataSerializer::ComputeMinBufferSize(data->mYSize,
                                                               data->mCbCrSize);
  ipc::Shmem shmem;
  if (!mDeprecatedTextureClient->GetForwarder()->AllocUnsafeShmem(size, shmType, &shmem)) {
    return false;
  }

  YCbCrImageDataSerializer serializer(shmem.get<uint8_t>(), shmem.Size<uint8_t>());
  serializer.InitializeBufferInfo(data->mYSize,
                                  data->mCbCrSize,
                                  data->mStereoMode);

  *mDescriptor = YCbCrImage(shmem, 0);

  return true;
}


}
}
