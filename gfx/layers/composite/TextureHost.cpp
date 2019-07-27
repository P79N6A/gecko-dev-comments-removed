




#include "TextureHost.h"

#include "CompositableHost.h"           
#include "LayersLogging.h"              
#include "mozilla/gfx/2D.h"             
#include "mozilla/ipc/Shmem.h"          
#include "mozilla/layers/CompositableTransactionParent.h" 
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/ImageDataSerializer.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureHostOGL.h"  
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "nsAString.h"
#include "nsRefPtr.h"                   
#include "nsPrintfCString.h"            
#include "mozilla/layers/PTextureParent.h"
#include "mozilla/unused.h"
#include <limits>
#include "SharedSurface.h"
#include "SharedSurfaceEGL.h"
#include "SharedSurfaceGL.h"
#include "../opengl/CompositorOGL.h"
#include "gfxUtils.h"

#ifdef MOZ_ENABLE_D3D10_LAYER
#include "../d3d11/CompositorD3D11.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "../opengl/GrallocTextureClient.h"
#include "../opengl/GrallocTextureHost.h"
#include "SharedSurfaceGralloc.h"
#endif

#ifdef MOZ_X11
#include "mozilla/layers/X11TextureHost.h"
#endif

#ifdef XP_MACOSX
#include "SharedSurfaceIO.h"
#include "../opengl/MacIOSurfaceTextureHostOGL.h"
#endif

#ifdef XP_WIN
#include "SharedSurfaceANGLE.h"
#include "mozilla/layers/TextureDIB.h"
#endif

#if 0
#define RECYCLE_LOG(...) printf_stderr(__VA_ARGS__)
#else
#define RECYCLE_LOG(...) do { } while (0)
#endif

struct nsIntPoint;

namespace mozilla {
namespace layers {





class TextureParent : public PTextureParent
{
public:
  explicit TextureParent(CompositableParentManager* aManager);

  ~TextureParent();

  bool Init(const SurfaceDescriptor& aSharedData,
            const TextureFlags& aFlags);

  void CompositorRecycle();

  virtual bool RecvClientRecycle() override;

  virtual bool RecvClearTextureHostSync() override;

  virtual bool RecvRemoveTexture() override;

  virtual bool RecvRecycleTexture(const TextureFlags& aTextureFlags) override;

  TextureHost* GetTextureHost() { return mTextureHost; }

  void ActorDestroy(ActorDestroyReason why) override;

  void ClearTextureHost();

  CompositableParentManager* mCompositableManager;
  RefPtr<TextureHost> mWaitForClientRecycle;
  RefPtr<TextureHost> mTextureHost;
};


PTextureParent*
TextureHost::CreateIPDLActor(CompositableParentManager* aManager,
                             const SurfaceDescriptor& aSharedData,
                             TextureFlags aFlags)
{
  if (aSharedData.type() == SurfaceDescriptor::TSurfaceDescriptorMemory &&
      !aManager->IsSameProcess())
  {
    NS_ERROR("A client process is trying to peek at our address space using a MemoryTexture!");
    return nullptr;
  }
  TextureParent* actor = new TextureParent(aManager);
  if (!actor->Init(aSharedData, aFlags)) {
    delete actor;
    return nullptr;
  }
  return actor;
}


bool
TextureHost::DestroyIPDLActor(PTextureParent* actor)
{
  delete actor;
  return true;
}


bool
TextureHost::SendDeleteIPDLActor(PTextureParent* actor)
{
  return PTextureParent::Send__delete__(actor);
}


TextureHost*
TextureHost::AsTextureHost(PTextureParent* actor)
{
  return actor? static_cast<TextureParent*>(actor)->mTextureHost : nullptr;
}

PTextureParent*
TextureHost::GetIPDLActor()
{
  return mActor;
}

FenceHandle
TextureHost::GetAndResetReleaseFenceHandle()
{
  return FenceHandle();
}


TemporaryRef<TextureHost> CreateTextureHostOGL(const SurfaceDescriptor& aDesc,
                                               ISurfaceAllocator* aDeallocator,
                                               TextureFlags aFlags);


TemporaryRef<TextureHost> CreateTextureHostBasic(const SurfaceDescriptor& aDesc,
                                                 ISurfaceAllocator* aDeallocator,
                                                 TextureFlags aFlags);


TemporaryRef<TextureHost> CreateTextureHostD3D11(const SurfaceDescriptor& aDesc,
                                                 ISurfaceAllocator* aDeallocator,
                                                 TextureFlags aFlags);


TemporaryRef<TextureHost> CreateTextureHostD3D9(const SurfaceDescriptor& aDesc,
                                                ISurfaceAllocator* aDeallocator,
                                                TextureFlags aFlags);


TemporaryRef<TextureHost>
TextureHost::Create(const SurfaceDescriptor& aDesc,
                    ISurfaceAllocator* aDeallocator,
                    TextureFlags aFlags)
{
  switch (aDesc.type()) {
    case SurfaceDescriptor::TSurfaceDescriptorShmem:
    case SurfaceDescriptor::TSurfaceDescriptorMemory:
    case SurfaceDescriptor::TSurfaceDescriptorDIB:
      return CreateBackendIndependentTextureHost(aDesc, aDeallocator, aFlags);

    case SurfaceDescriptor::TEGLImageDescriptor:
    case SurfaceDescriptor::TNewSurfaceDescriptorGralloc:
    case SurfaceDescriptor::TSurfaceTextureDescriptor:
      return CreateTextureHostOGL(aDesc, aDeallocator, aFlags);

    case SurfaceDescriptor::TSharedSurfaceDescriptor:
      return new SharedSurfaceTextureHost(aFlags, aDesc.get_SharedSurfaceDescriptor());

    case SurfaceDescriptor::TSurfaceDescriptorMacIOSurface:
      if (Compositor::GetBackend() == LayersBackend::LAYERS_OPENGL) {
        return CreateTextureHostOGL(aDesc, aDeallocator, aFlags);
      } else {
        return CreateTextureHostBasic(aDesc, aDeallocator, aFlags);
      }

#ifdef MOZ_X11
    case SurfaceDescriptor::TSurfaceDescriptorX11: {
      const SurfaceDescriptorX11& desc = aDesc.get_SurfaceDescriptorX11();
      RefPtr<TextureHost> result = new X11TextureHost(aFlags, desc);
      return result;
    }
#endif

#ifdef XP_WIN
    case SurfaceDescriptor::TSurfaceDescriptorD3D9:
      return CreateTextureHostD3D9(aDesc, aDeallocator, aFlags);

    case SurfaceDescriptor::TSurfaceDescriptorD3D10:
    case SurfaceDescriptor::TSurfaceDescriptorDXGIYCbCr:
      if (Compositor::GetBackend() == LayersBackend::LAYERS_D3D9) {
        return CreateTextureHostD3D9(aDesc, aDeallocator, aFlags);
      } else {
        return CreateTextureHostD3D11(aDesc, aDeallocator, aFlags);
      }
#endif
    default:
      MOZ_CRASH("Unsupported Surface type");
  }
}

TemporaryRef<TextureHost>
CreateBackendIndependentTextureHost(const SurfaceDescriptor& aDesc,
                                    ISurfaceAllocator* aDeallocator,
                                    TextureFlags aFlags)
{
  RefPtr<TextureHost> result;
  switch (aDesc.type()) {
    case SurfaceDescriptor::TSurfaceDescriptorShmem: {
      const SurfaceDescriptorShmem& descriptor = aDesc.get_SurfaceDescriptorShmem();
      result = new ShmemTextureHost(descriptor.data(),
                                    descriptor.format(),
                                    aDeallocator,
                                    aFlags);
      break;
    }
    case SurfaceDescriptor::TSurfaceDescriptorMemory: {
      const SurfaceDescriptorMemory& descriptor = aDesc.get_SurfaceDescriptorMemory();
      result = new MemoryTextureHost(reinterpret_cast<uint8_t*>(descriptor.data()),
                                     descriptor.format(),
                                     aFlags);
      break;
    }
#ifdef XP_WIN
    case SurfaceDescriptor::TSurfaceDescriptorDIB: {
      result = new DIBTextureHost(aFlags, aDesc);
      break;
    }
#endif
    default: {
      NS_WARNING("No backend independent TextureHost for this descriptor type");
    }
  }
  return result;
}

void
TextureHost::CompositorRecycle()
{
  if (!mActor) {
    return;
  }
  static_cast<TextureParent*>(mActor)->CompositorRecycle();
}

TextureHost::TextureHost(TextureFlags aFlags)
    : mActor(nullptr)
    , mFlags(aFlags)
    , mCompositableCount(0)
{}

TextureHost::~TextureHost()
{
}

void TextureHost::Finalize()
{
  if (!(GetFlags() & TextureFlags::DEALLOCATE_CLIENT)) {
    DeallocateSharedData();
    DeallocateDeviceData();
  }
}

void
TextureHost::RecycleTexture(TextureFlags aFlags)
{
  MOZ_ASSERT(GetFlags() & TextureFlags::RECYCLE);
  MOZ_ASSERT(aFlags & TextureFlags::RECYCLE);
  MOZ_ASSERT(!HasRecycleCallback());
  mFlags = aFlags;
}

void
TextureHost::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("%s (0x%p)", Name(), this).get();
  
  
  if (Lock()) {
    AppendToString(aStream, GetSize(), " [size=", "]");
    AppendToString(aStream, GetFormat(), " [format=", "]");
    Unlock();
  }
  AppendToString(aStream, mFlags, " [flags=", "]");
#ifdef MOZ_DUMP_PAINTING
  if (gfxPrefs::LayersDumpTexture() || profiler_feature_active("layersdump")) {
    nsAutoCString pfx(aPrefix);
    pfx += "  ";

    aStream << "\n" << pfx.get() << "Surface: ";
    RefPtr<gfx::DataSourceSurface> dSurf = GetAsSurface();
    if (dSurf) {
      aStream << gfxUtils::GetAsLZ4Base64Str(dSurf).get();
    }
  }
#endif
}

TextureSource::TextureSource()
: mCompositableCount(0)
{
    MOZ_COUNT_CTOR(TextureSource);
}

TextureSource::~TextureSource()
{
    MOZ_COUNT_DTOR(TextureSource);
}

BufferTextureHost::BufferTextureHost(gfx::SurfaceFormat aFormat,
                                     TextureFlags aFlags)
: TextureHost(aFlags)
, mCompositor(nullptr)
, mFormat(aFormat)
, mUpdateSerial(1)
, mLocked(false)
, mNeedsFullUpdate(false)
{
  if (aFlags & TextureFlags::COMPONENT_ALPHA) {
    
    
    
    mNeedsFullUpdate = true;
  }
}

void
BufferTextureHost::InitSize()
{
  if (mFormat == gfx::SurfaceFormat::YUV) {
    YCbCrImageDataDeserializer yuvDeserializer(GetBuffer(), GetBufferSize());
    if (yuvDeserializer.IsValid()) {
      mSize = yuvDeserializer.GetYSize();
    }
  } else if (mFormat != gfx::SurfaceFormat::UNKNOWN) {
    ImageDataDeserializer deserializer(GetBuffer(), GetBufferSize());
    if (deserializer.IsValid()) {
      mSize = deserializer.GetSize();
    }
  }
}

BufferTextureHost::~BufferTextureHost()
{}

void
BufferTextureHost::Updated(const nsIntRegion* aRegion)
{
  ++mUpdateSerial;
  
  
  if (aRegion && !mNeedsFullUpdate) {
    mMaybeUpdatedRegion = mMaybeUpdatedRegion.Or(mMaybeUpdatedRegion, *aRegion);
  } else {
    mNeedsFullUpdate = true;
  }
  if (GetFlags() & TextureFlags::IMMEDIATE_UPLOAD) {
    DebugOnly<bool> result = MaybeUpload(!mNeedsFullUpdate ? &mMaybeUpdatedRegion : nullptr);
    NS_WARN_IF_FALSE(result, "Failed to upload a texture");
  }
}

void
BufferTextureHost::SetCompositor(Compositor* aCompositor)
{
  MOZ_ASSERT(aCompositor);
  if (mCompositor == aCompositor) {
    return;
  }
  RefPtr<TextureSource> it = mFirstSource;
  while (it) {
    it->SetCompositor(aCompositor);
    it = it->GetNextSibling();
  }
  mFirstSource = nullptr;
  mCompositor = aCompositor;
}

void
BufferTextureHost::DeallocateDeviceData()
{
  RefPtr<TextureSource> it = mFirstSource;
  while (it) {
    it->DeallocateDeviceData();
    it = it->GetNextSibling();
  }
}

bool
BufferTextureHost::Lock()
{
  MOZ_ASSERT(!mLocked);
  if (!MaybeUpload(!mNeedsFullUpdate ? &mMaybeUpdatedRegion : nullptr)) {
      return false;
  }
  mLocked = !!mFirstSource;
  return mLocked;
}

void
BufferTextureHost::Unlock()
{
  MOZ_ASSERT(mLocked);
  mLocked = false;
}

bool
BufferTextureHost::BindTextureSource(CompositableTextureSourceRef& aTexture)
{
  MOZ_ASSERT(mLocked);
  MOZ_ASSERT(mFirstSource);
  aTexture = mFirstSource;
  return !!aTexture;
}

gfx::SurfaceFormat
BufferTextureHost::GetFormat() const
{
  
  
  
  
  
  if (mFormat == gfx::SurfaceFormat::YUV &&
    mCompositor &&
    !mCompositor->SupportsEffect(EffectTypes::YCBCR)) {
    return gfx::SurfaceFormat::R8G8B8X8;
  }
  return mFormat;
}

bool
BufferTextureHost::MaybeUpload(nsIntRegion *aRegion)
{
  if (mFirstSource && mFirstSource->GetUpdateSerial() == mUpdateSerial) {
    return true;
  }
  if (!Upload(aRegion)) {
    return false;
  }

  
  mNeedsFullUpdate = false;
  mMaybeUpdatedRegion.SetEmpty();

  
  mFirstSource->SetUpdateSerial(mUpdateSerial);
  return true;
}

bool
BufferTextureHost::Upload(nsIntRegion *aRegion)
{
  if (!GetBuffer()) {
    
    
    
    return false;
  }
  if (!mCompositor) {
    
    
    return false;
  }
  if (mFormat == gfx::SurfaceFormat::UNKNOWN) {
    NS_WARNING("BufferTextureHost: unsupported format!");
    return false;
  } else if (mFormat == gfx::SurfaceFormat::YUV) {
    YCbCrImageDataDeserializer yuvDeserializer(GetBuffer(), GetBufferSize());
    MOZ_ASSERT(yuvDeserializer.IsValid());

    if (!mCompositor->SupportsEffect(EffectTypes::YCBCR)) {
      RefPtr<gfx::DataSourceSurface> surf = yuvDeserializer.ToDataSourceSurface();
      if (NS_WARN_IF(!surf)) {
        return false;
      }
      if (!mFirstSource) {
        mFirstSource = mCompositor->CreateDataTextureSource(mFlags);
      }
      mFirstSource->Update(surf, aRegion);
      return true;
    }

    RefPtr<DataTextureSource> srcY;
    RefPtr<DataTextureSource> srcU;
    RefPtr<DataTextureSource> srcV;
    if (!mFirstSource) {
      
      srcY = mCompositor->CreateDataTextureSource(mFlags|TextureFlags::DISALLOW_BIGIMAGE);
      srcU = mCompositor->CreateDataTextureSource(mFlags|TextureFlags::DISALLOW_BIGIMAGE);
      srcV = mCompositor->CreateDataTextureSource(mFlags|TextureFlags::DISALLOW_BIGIMAGE);
      mFirstSource = srcY;
      srcY->SetNextSibling(srcU);
      srcU->SetNextSibling(srcV);
    } else {
      
      
      
      
      MOZ_ASSERT(mFirstSource->GetNextSibling());
      MOZ_ASSERT(mFirstSource->GetNextSibling()->GetNextSibling());
      srcY = mFirstSource;
      srcU = mFirstSource->GetNextSibling()->AsDataTextureSource();
      srcV = mFirstSource->GetNextSibling()->GetNextSibling()->AsDataTextureSource();
    }


    RefPtr<gfx::DataSourceSurface> tempY =
      gfx::Factory::CreateWrappingDataSourceSurface(yuvDeserializer.GetYData(),
                                                    yuvDeserializer.GetYStride(),
                                                    yuvDeserializer.GetYSize(),
                                                    gfx::SurfaceFormat::A8);
    RefPtr<gfx::DataSourceSurface> tempCb =
      gfx::Factory::CreateWrappingDataSourceSurface(yuvDeserializer.GetCbData(),
                                                    yuvDeserializer.GetCbCrStride(),
                                                    yuvDeserializer.GetCbCrSize(),
                                                    gfx::SurfaceFormat::A8);
    RefPtr<gfx::DataSourceSurface> tempCr =
      gfx::Factory::CreateWrappingDataSourceSurface(yuvDeserializer.GetCrData(),
                                                    yuvDeserializer.GetCbCrStride(),
                                                    yuvDeserializer.GetCbCrSize(),
                                                    gfx::SurfaceFormat::A8);
    
    NS_ASSERTION(!aRegion, "Unsupported partial updates for YCbCr textures");
    if (!tempY ||
        !tempCb ||
        !tempCr ||
        !srcY->Update(tempY) ||
        !srcU->Update(tempCb) ||
        !srcV->Update(tempCr)) {
      NS_WARNING("failed to update the DataTextureSource");
      return false;
    }
  } else {
    
    if (!mFirstSource) {
      mFirstSource = mCompositor->CreateDataTextureSource();
    }
    ImageDataDeserializer deserializer(GetBuffer(), GetBufferSize());
    if (!deserializer.IsValid()) {
      NS_ERROR("Failed to deserialize image!");
      return false;
    }

    RefPtr<gfx::DataSourceSurface> surf = deserializer.GetAsSurface();
    if (!surf) {
      return false;
    }

    if (!mFirstSource->Update(surf.get(), aRegion)) {
      NS_WARNING("failed to update the DataTextureSource");
      return false;
    }
  }
  MOZ_ASSERT(mFirstSource);
  return true;
}

TemporaryRef<gfx::DataSourceSurface>
BufferTextureHost::GetAsSurface()
{
  RefPtr<gfx::DataSourceSurface> result;
  if (mFormat == gfx::SurfaceFormat::UNKNOWN) {
    NS_WARNING("BufferTextureHost: unsupported format!");
    return nullptr;
  } else if (mFormat == gfx::SurfaceFormat::YUV) {
    YCbCrImageDataDeserializer yuvDeserializer(GetBuffer(), GetBufferSize());
    if (!yuvDeserializer.IsValid()) {
      return nullptr;
    }
    result = yuvDeserializer.ToDataSourceSurface();
    if (NS_WARN_IF(!result)) {
      return nullptr;
    }
  } else {
    ImageDataDeserializer deserializer(GetBuffer(), GetBufferSize());
    if (!deserializer.IsValid()) {
      NS_ERROR("Failed to deserialize image!");
      return nullptr;
    }
    result = deserializer.GetAsSurface();
  }
  return result.forget();
}

ShmemTextureHost::ShmemTextureHost(const ipc::Shmem& aShmem,
                                   gfx::SurfaceFormat aFormat,
                                   ISurfaceAllocator* aDeallocator,
                                   TextureFlags aFlags)
: BufferTextureHost(aFormat, aFlags)
, mShmem(MakeUnique<ipc::Shmem>(aShmem))
, mDeallocator(aDeallocator)
{
  MOZ_COUNT_CTOR(ShmemTextureHost);
  InitSize();
}

ShmemTextureHost::~ShmemTextureHost()
{
  MOZ_ASSERT(!mShmem || (mFlags & TextureFlags::DEALLOCATE_CLIENT),
             "Leaking our buffer");
  DeallocateDeviceData();
  MOZ_COUNT_DTOR(ShmemTextureHost);
}

void
ShmemTextureHost::DeallocateSharedData()
{
  if (mShmem) {
    MOZ_ASSERT(mDeallocator,
               "Shared memory would leak without a ISurfaceAllocator");
    mDeallocator->DeallocShmem(*mShmem);
    mShmem = nullptr;
  }
}

void
ShmemTextureHost::ForgetSharedData()
{
  if (mShmem) {
    mShmem = nullptr;
  }
}

void
ShmemTextureHost::OnShutdown()
{
  mShmem = nullptr;
}

uint8_t* ShmemTextureHost::GetBuffer()
{
  return mShmem ? mShmem->get<uint8_t>() : nullptr;
}

size_t ShmemTextureHost::GetBufferSize()
{
  return mShmem ? mShmem->Size<uint8_t>() : 0;
}

MemoryTextureHost::MemoryTextureHost(uint8_t* aBuffer,
                                     gfx::SurfaceFormat aFormat,
                                     TextureFlags aFlags)
: BufferTextureHost(aFormat, aFlags)
, mBuffer(aBuffer)
{
  MOZ_COUNT_CTOR(MemoryTextureHost);
  InitSize();
}

MemoryTextureHost::~MemoryTextureHost()
{
  MOZ_ASSERT(!mBuffer || (mFlags & TextureFlags::DEALLOCATE_CLIENT),
             "Leaking our buffer");
  DeallocateDeviceData();
  MOZ_COUNT_DTOR(MemoryTextureHost);
}

void
MemoryTextureHost::DeallocateSharedData()
{
  if (mBuffer) {
    GfxMemoryImageReporter::WillFree(mBuffer);
  }
  delete[] mBuffer;
  mBuffer = nullptr;
}

void
MemoryTextureHost::ForgetSharedData()
{
  mBuffer = nullptr;
}

uint8_t* MemoryTextureHost::GetBuffer()
{
  return mBuffer;
}

size_t MemoryTextureHost::GetBufferSize()
{
  
  
  
  
  return std::numeric_limits<size_t>::max();
}

TextureParent::TextureParent(CompositableParentManager* aCompositableManager)
: mCompositableManager(aCompositableManager)
{
  MOZ_COUNT_CTOR(TextureParent);
}

TextureParent::~TextureParent()
{
  MOZ_COUNT_DTOR(TextureParent);
  if (mTextureHost) {
    mTextureHost->ClearRecycleCallback();
  }
}

static void RecycleCallback(TextureHost* textureHost, void* aClosure) {
  TextureParent* tp = reinterpret_cast<TextureParent*>(aClosure);
  tp->CompositorRecycle();
}

void
TextureParent::CompositorRecycle()
{
  mTextureHost->ClearRecycleCallback();

  if (mTextureHost->GetFlags() & TextureFlags::RECYCLE) {
    mozilla::unused << SendCompositorRecycle();
    
    
    mWaitForClientRecycle = mTextureHost;
  }
}

bool
TextureParent::RecvClientRecycle()
{
  
  
  mTextureHost->SetRecycleCallback(RecycleCallback, this);
  if (!mWaitForClientRecycle) {
    RECYCLE_LOG("Not a recycable tile");
  }
  mWaitForClientRecycle = nullptr;
  return true;
}

bool
TextureParent::Init(const SurfaceDescriptor& aSharedData,
                    const TextureFlags& aFlags)
{
  mTextureHost = TextureHost::Create(aSharedData,
                                     mCompositableManager,
                                     aFlags);
  if (mTextureHost) {
    mTextureHost->mActor = this;
    if (aFlags & TextureFlags::RECYCLE) {
      mWaitForClientRecycle = mTextureHost;
      RECYCLE_LOG("Setup recycling for tile %p\n", this);
    }
  }

  return !!mTextureHost;
}

bool
TextureParent::RecvRemoveTexture()
{
  return PTextureParent::Send__delete__(this);
}

bool
TextureParent::RecvClearTextureHostSync()
{
  ClearTextureHost();
  return true;
}

void
TextureParent::ActorDestroy(ActorDestroyReason why)
{
  switch (why) {
  case AncestorDeletion:
  case Deletion:
  case NormalShutdown:
  case AbnormalShutdown:
    break;
  case FailedConstructor:
    NS_RUNTIMEABORT("FailedConstructor isn't possible in PTexture");
  }

  ClearTextureHost();
}

void
TextureParent::ClearTextureHost()
{
  if (!mTextureHost) {
    return;
  }

  if (mTextureHost->GetFlags() & TextureFlags::RECYCLE) {
    RECYCLE_LOG("clear recycling for tile %p\n", this);
    mTextureHost->ClearRecycleCallback();
  }
  if (mTextureHost->GetFlags() & TextureFlags::DEALLOCATE_CLIENT) {
    mTextureHost->ForgetSharedData();
  }

  
  mTextureHost->ClearRecycleCallback();
  mWaitForClientRecycle = nullptr;

  mTextureHost->mActor = nullptr;
  mTextureHost = nullptr;
}

bool
TextureParent::RecvRecycleTexture(const TextureFlags& aTextureFlags)
{
  if (!mTextureHost) {
    return true;
  }
  mTextureHost->RecycleTexture(aTextureFlags);
  return true;
}



static RefPtr<TextureSource>
SharedSurfaceToTexSource(gl::SharedSurface* abstractSurf, Compositor* compositor)
{
  MOZ_ASSERT(abstractSurf);
  MOZ_ASSERT(abstractSurf->mType != gl::SharedSurfaceType::Basic);
  MOZ_ASSERT(abstractSurf->mType != gl::SharedSurfaceType::Gralloc);

  if (!compositor) {
    return nullptr;
  }

  gfx::SurfaceFormat format = abstractSurf->mHasAlpha ? gfx::SurfaceFormat::R8G8B8A8
                                                      : gfx::SurfaceFormat::R8G8B8X8;

  RefPtr<TextureSource> texSource;
  switch (abstractSurf->mType) {
#ifdef XP_WIN
    case gl::SharedSurfaceType::EGLSurfaceANGLE: {
      auto surf = gl::SharedSurface_ANGLEShareHandle::Cast(abstractSurf);

      MOZ_ASSERT(compositor->GetBackendType() == LayersBackend::LAYERS_D3D11);
      CompositorD3D11* compositorD3D11 = static_cast<CompositorD3D11*>(compositor);
      RefPtr<ID3D11Texture2D> tex = surf->GetConsumerTexture();

      if (!tex) {
        NS_WARNING("Failed to open shared resource.");
        break;
      }
      texSource = new DataTextureSourceD3D11(format, compositorD3D11, tex);
      break;
    }
#endif
    case gl::SharedSurfaceType::GLTextureShare: {
      auto surf = gl::SharedSurface_GLTexture::Cast(abstractSurf);

      MOZ_ASSERT(compositor->GetBackendType() == LayersBackend::LAYERS_OPENGL);
      CompositorOGL* compositorOGL = static_cast<CompositorOGL*>(compositor);
      gl::GLContext* gl = compositorOGL->gl();

      GLenum target = surf->ConsTextureTarget();
      GLuint tex = surf->ConsTexture(gl);
      texSource = new GLTextureSource(compositorOGL, tex, target,
                                      surf->mSize, format,
                                      true);
      break;
    }
    case gl::SharedSurfaceType::EGLImageShare: {
      auto surf = gl::SharedSurface_EGLImage::Cast(abstractSurf);

      MOZ_ASSERT(compositor->GetBackendType() == LayersBackend::LAYERS_OPENGL);
      CompositorOGL* compositorOGL = static_cast<CompositorOGL*>(compositor);
      gl::GLContext* gl = compositorOGL->gl();
      MOZ_ASSERT(gl->IsCurrent());

      GLenum target = 0;
      GLuint tex = 0;
      surf->AcquireConsumerTexture(gl, &tex, &target);

      texSource = new GLTextureSource(compositorOGL, tex, target,
                                      surf->mSize, format,
                                      true);
      break;
    }
#ifdef XP_MACOSX
    case gl::SharedSurfaceType::IOSurface: {
      auto surf = gl::SharedSurface_IOSurface::Cast(abstractSurf);
      MacIOSurface* ioSurf = surf->GetIOSurface();

      MOZ_ASSERT(compositor->GetBackendType() == LayersBackend::LAYERS_OPENGL);
      CompositorOGL* compositorOGL = static_cast<CompositorOGL*>(compositor);

      texSource = new MacIOSurfaceTextureSourceOGL(compositorOGL, ioSurf);
      break;
    }
#endif
    default:
      break;
  }

  MOZ_ASSERT(texSource.get(), "TextureSource creation failed.");
  return texSource;
}




SharedSurfaceTextureHost::SharedSurfaceTextureHost(TextureFlags aFlags,
                                                   const SharedSurfaceDescriptor& aDesc)
  : TextureHost(aFlags)
  , mIsLocked(false)
  , mSurf((gl::SharedSurface*)aDesc.surf())
  , mCompositor(nullptr)
{
  MOZ_ASSERT(mSurf);
}

gfx::SurfaceFormat
SharedSurfaceTextureHost::GetFormat() const
{
  MOZ_ASSERT(mTexSource);
  return mTexSource->GetFormat();
}

gfx::IntSize
SharedSurfaceTextureHost::GetSize() const
{
  MOZ_ASSERT(mTexSource);
  return mTexSource->GetSize();
}

void
SharedSurfaceTextureHost::EnsureTexSource()
{
  MOZ_ASSERT(mIsLocked);

  if (mTexSource)
    return;

  mTexSource = SharedSurfaceToTexSource(mSurf, mCompositor);
  MOZ_ASSERT(mTexSource);
}

bool
SharedSurfaceTextureHost::Lock()
{
  MOZ_ASSERT(!mIsLocked);

  mSurf->ConsumerAcquire();

  mIsLocked = true;

  EnsureTexSource();

  return true;
}

void
SharedSurfaceTextureHost::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mSurf->ConsumerRelease();
  mIsLocked = false;
}




} 
} 
