




#include "mozilla/layers/TextureHost.h"
#include "CompositableHost.h"           
#include "LayersLogging.h"              
#include "gfx2DGlue.h"                  
#include "gfxImageSurface.h"            
#include "mozilla/gfx/2D.h"             
#include "mozilla/ipc/Shmem.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/ImageDataSerializer.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "nsAString.h"
#include "nsAutoPtr.h"                  
#include "nsPrintfCString.h"            
#include "mozilla/layers/PTextureParent.h"

struct nsIntPoint;

namespace mozilla {
namespace layers {





class TextureParent : public PTextureParent
{
public:
  TextureParent(ISurfaceAllocator* aAllocator);

  ~TextureParent();

  bool RecvInit(const SurfaceDescriptor& aSharedData,
                const TextureFlags& aFlags) MOZ_OVERRIDE;

  virtual bool RecvRemoveTexture() MOZ_OVERRIDE;

  virtual bool RecvRemoveTextureSync() MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  TextureHost* GetTextureHost() { return mTextureHost; }

  ISurfaceAllocator* mAllocator;
  RefPtr<TextureHost> mTextureHost;
};


PTextureParent*
TextureHost::CreateIPDLActor(ISurfaceAllocator* aAllocator)
{
  return new TextureParent(aAllocator);
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


TemporaryRef<DeprecatedTextureHost> CreateDeprecatedTextureHostOGL(SurfaceDescriptorType aDescriptorType,
                                                           uint32_t aDeprecatedTextureHostFlags,
                                                           uint32_t aTextureFlags);

TemporaryRef<DeprecatedTextureHost> CreateBasicDeprecatedTextureHost(SurfaceDescriptorType aDescriptorType,
                                                             uint32_t aDeprecatedTextureHostFlags,
                                                             uint32_t aTextureFlags);

#ifdef XP_WIN
TemporaryRef<DeprecatedTextureHost> CreateDeprecatedTextureHostD3D9(SurfaceDescriptorType aDescriptorType,
                                                            uint32_t aDeprecatedTextureHostFlags,
                                                            uint32_t aTextureFlags);

TemporaryRef<DeprecatedTextureHost> CreateDeprecatedTextureHostD3D11(SurfaceDescriptorType aDescriptorType,
                                                             uint32_t aDeprecatedTextureHostFlags,
                                                             uint32_t aTextureFlags);
#endif

 TemporaryRef<DeprecatedTextureHost>
DeprecatedTextureHost::CreateDeprecatedTextureHost(SurfaceDescriptorType aDescriptorType,
                                           uint32_t aDeprecatedTextureHostFlags,
                                           uint32_t aTextureFlags,
                                           CompositableHost* aCompositableHost)
{
  switch (Compositor::GetBackend()) {
    case LAYERS_OPENGL:
      {
      RefPtr<DeprecatedTextureHost> result;
      result = CreateDeprecatedTextureHostOGL(aDescriptorType,
                                        aDeprecatedTextureHostFlags,
                                        aTextureFlags);
      if (aCompositableHost) {
        result->SetCompositableBackendSpecificData(aCompositableHost->GetCompositableBackendSpecificData());
      }
      return result;
      }
#ifdef XP_WIN
    case LAYERS_D3D9:
      return CreateDeprecatedTextureHostD3D9(aDescriptorType,
                                         aDeprecatedTextureHostFlags,
                                         aTextureFlags);
    case LAYERS_D3D11:
      return CreateDeprecatedTextureHostD3D11(aDescriptorType,
                                          aDeprecatedTextureHostFlags,
                                          aTextureFlags);
#endif
    case LAYERS_BASIC:
      return CreateBasicDeprecatedTextureHost(aDescriptorType,
                                          aDeprecatedTextureHostFlags,
                                          aTextureFlags);
    default:
      MOZ_CRASH("Couldn't create texture host");
  }
}


TemporaryRef<TextureHost> CreateTextureHostOGL(const SurfaceDescriptor& aDesc,
                                               ISurfaceAllocator* aDeallocator,
                                               TextureFlags aFlags);


TemporaryRef<TextureHost> CreateTextureHostBasic(const SurfaceDescriptor& aDesc,
                                                 ISurfaceAllocator* aDeallocator,
                                                 TextureFlags aFlags);


TemporaryRef<TextureHost>
TextureHost::Create(const SurfaceDescriptor& aDesc,
                    ISurfaceAllocator* aDeallocator,
                    TextureFlags aFlags)
{
  switch (Compositor::GetBackend()) {
    case LAYERS_OPENGL:
      return CreateTextureHostOGL(aDesc, aDeallocator, aFlags);
    case LAYERS_BASIC:
      return CreateTextureHostBasic(aDesc, aDeallocator, aFlags);
#ifdef MOZ_WIDGET_GONK
    case LAYERS_NONE:
      
      
      
      return CreateTextureHostOGL(aDesc, aDeallocator, aFlags);
#endif
#ifdef XP_WIN
    case LAYERS_D3D11:
    case LAYERS_D3D9:
      
#endif
    default:
      MOZ_CRASH("Couldn't create texture host");
      return nullptr;
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
    default: {
      NS_WARNING("No backend independent TextureHost for this descriptor type");
    }
  }
  return result;
}

void
TextureHost::SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData)
{
    mCompositableBackendData = aBackendData;
}


TextureHost::TextureHost(TextureFlags aFlags)
    : mFlags(aFlags)
{}

TextureHost::~TextureHost()
{
}

void
TextureHost::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("%s (0x%p)", Name(), this);
  AppendToString(aTo, GetSize(), " [size=", "]");
  AppendToString(aTo, GetFormat(), " [format=", "]");
  AppendToString(aTo, mFlags, " [flags=", "]");
}

void
TextureSource::SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData)
{
    mCompositableBackendData = aBackendData;
}

TextureSource::TextureSource()
{
    MOZ_COUNT_CTOR(TextureSource);
}
TextureSource::~TextureSource()
{
    MOZ_COUNT_DTOR(TextureSource);
}

DeprecatedTextureHost::DeprecatedTextureHost()
  : mFlags(0)
  , mBuffer(nullptr)
  , mDeAllocator(nullptr)
  , mFormat(gfx::FORMAT_UNKNOWN)
{
  MOZ_COUNT_CTOR(DeprecatedTextureHost);
}

DeprecatedTextureHost::~DeprecatedTextureHost()
{
  if (mBuffer) {
    if (!(mFlags & TEXTURE_DEALLOCATE_CLIENT)) {
      if (mDeAllocator) {
        mDeAllocator->DestroySharedSurface(mBuffer);
      } else {
        MOZ_ASSERT(mBuffer->type() == SurfaceDescriptor::Tnull_t);
      }
    }
    delete mBuffer;
  }
  MOZ_COUNT_DTOR(DeprecatedTextureHost);
}

void
DeprecatedTextureHost::Update(const SurfaceDescriptor& aImage,
                          nsIntRegion* aRegion,
                          nsIntPoint* aOffset)
{
  UpdateImpl(aImage, aRegion, aOffset);
}

void
DeprecatedTextureHost::SwapTextures(const SurfaceDescriptor& aImage,
                                SurfaceDescriptor* aResult,
                                nsIntRegion* aRegion)
{
  SwapTexturesImpl(aImage, aRegion);

  MOZ_ASSERT(mBuffer, "trying to swap a non-buffered texture host?");
  if (aResult) {
    *aResult = *mBuffer;
  }
  *mBuffer = aImage;
  
  
  
  
  
  
  
  
  SetBuffer(mBuffer, mDeAllocator);
}

void
DeprecatedTextureHost::OnActorDestroy()
{
  if (ISurfaceAllocator::IsShmem(mBuffer)) {
    *mBuffer = SurfaceDescriptor();
    mBuffer = nullptr;
  }
}

void
DeprecatedTextureHost::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("%s (0x%p)", Name(), this);
  AppendToString(aTo, GetSize(), " [size=", "]");
  AppendToString(aTo, GetFormat(), " [format=", "]");
  AppendToString(aTo, mFlags, " [flags=", "]");
}

void
DeprecatedTextureHost::SetBuffer(SurfaceDescriptor* aBuffer, ISurfaceAllocator* aAllocator)
{
  MOZ_ASSERT(!mBuffer || mBuffer == aBuffer, "Will leak the old mBuffer");
  mBuffer = aBuffer;
  mDeAllocator = aAllocator;
}

BufferTextureHost::BufferTextureHost(gfx::SurfaceFormat aFormat,
                                     TextureFlags aFlags)
: TextureHost(aFlags)
, mCompositor(nullptr)
, mFormat(aFormat)
, mUpdateSerial(1)
, mLocked(false)
, mPartialUpdate(false)
{}

BufferTextureHost::~BufferTextureHost()
{}

void
BufferTextureHost::Updated(const nsIntRegion* aRegion)
{
  ++mUpdateSerial;
  if (aRegion) {
    mPartialUpdate = true;
    mMaybeUpdatedRegion = *aRegion;
  } else {
    mPartialUpdate = false;
  }
  if (GetFlags() & TEXTURE_IMMEDIATE_UPLOAD) {
    DebugOnly<bool> result = MaybeUpload(mPartialUpdate ? &mMaybeUpdatedRegion : nullptr);
    MOZ_ASSERT(result);
  }
}

void
BufferTextureHost::SetCompositor(Compositor* aCompositor)
{
  if (mCompositor == aCompositor) {
    return;
  }
  DeallocateDeviceData();
  mCompositor = aCompositor;
}

void
BufferTextureHost::DeallocateDeviceData()
{
  RefPtr<NewTextureSource> it = mFirstSource;
  while (it) {
    it->DeallocateDeviceData();
    it = it->GetNextSibling();
  }
}

bool
BufferTextureHost::Lock()
{
  mLocked = true;
  return true;
}

void
BufferTextureHost::Unlock()
{
  mLocked = false;
}

NewTextureSource*
BufferTextureHost::GetTextureSources()
{
  MOZ_ASSERT(mLocked, "should never be called while not locked");
  if (!MaybeUpload(mPartialUpdate ? &mMaybeUpdatedRegion : nullptr)) {
      return nullptr;
  }
  return mFirstSource;
}

gfx::SurfaceFormat
BufferTextureHost::GetFormat() const
{
  
  
  
  
  
  if (mFormat == gfx::FORMAT_YUV &&
    mCompositor &&
    !mCompositor->SupportsEffect(EFFECT_YCBCR)) {
    return gfx::FORMAT_R8G8B8X8;
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
  mFirstSource->SetUpdateSerial(mUpdateSerial);
  return true;
}

bool
BufferTextureHost::Upload(nsIntRegion *aRegion)
{
  if (!mCompositor) {
    NS_WARNING("Tried to upload without a compositor. Skipping texture upload...");
    
    
    
    return false;
  }
  if (mFormat == gfx::FORMAT_UNKNOWN) {
    NS_WARNING("BufferTextureHost: unsupported format!");
    return false;
  } else if (mFormat == gfx::FORMAT_YUV) {
    YCbCrImageDataDeserializer yuvDeserializer(GetBuffer());
    MOZ_ASSERT(yuvDeserializer.IsValid());

    if (!mCompositor->SupportsEffect(EFFECT_YCBCR)) {
      RefPtr<gfx::DataSourceSurface> surf = yuvDeserializer.ToDataSourceSurface();
      if (!mFirstSource) {
        mFirstSource = mCompositor->CreateDataTextureSource(mFlags);
      }
      mFirstSource->Update(surf, mFlags, aRegion);
      return true;
    }

    RefPtr<DataTextureSource> srcY;
    RefPtr<DataTextureSource> srcU;
    RefPtr<DataTextureSource> srcV;
    if (!mFirstSource) {
      
      srcY = mCompositor->CreateDataTextureSource(mFlags|TEXTURE_DISALLOW_BIGIMAGE);
      srcU = mCompositor->CreateDataTextureSource(mFlags|TEXTURE_DISALLOW_BIGIMAGE);
      srcV = mCompositor->CreateDataTextureSource(mFlags|TEXTURE_DISALLOW_BIGIMAGE);
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
                                                    gfx::ToIntSize(yuvDeserializer.GetYSize()),
                                                    gfx::FORMAT_A8);
    RefPtr<gfx::DataSourceSurface> tempCb =
      gfx::Factory::CreateWrappingDataSourceSurface(yuvDeserializer.GetCbData(),
                                                    yuvDeserializer.GetCbCrStride(),
                                                    gfx::ToIntSize(yuvDeserializer.GetCbCrSize()),
                                                    gfx::FORMAT_A8);
    RefPtr<gfx::DataSourceSurface> tempCr =
      gfx::Factory::CreateWrappingDataSourceSurface(yuvDeserializer.GetCrData(),
                                                    yuvDeserializer.GetCbCrStride(),
                                                    gfx::ToIntSize(yuvDeserializer.GetCbCrSize()),
                                                    gfx::FORMAT_A8);
    
    NS_ASSERTION(!aRegion, "Unsupported partial updates for YCbCr textures");
    if (!srcY->Update(tempY, mFlags) ||
        !srcU->Update(tempCb, mFlags) ||
        !srcV->Update(tempCr, mFlags)) {
      NS_WARNING("failed to update the DataTextureSource");
      return false;
    }
  } else {
    
    if (!mFirstSource) {
      mFirstSource = mCompositor->CreateDataTextureSource();
    }
    ImageDataDeserializer deserializer(GetBuffer());
    if (!deserializer.IsValid()) {
      NS_WARNING("failed to open shmem surface");
      return false;
    }

    RefPtr<gfx::DataSourceSurface> surf = deserializer.GetAsSurface();
    if (!surf) {
      return false;
    }

    if (!mFirstSource->Update(surf.get(), mFlags, aRegion)) {
      NS_WARNING("failed to update the DataTextureSource");
      return false;
    }
  }
  return true;
}

TemporaryRef<gfx::DataSourceSurface>
BufferTextureHost::GetAsSurface()
{
  RefPtr<gfx::DataSourceSurface> result;
  if (mFormat == gfx::FORMAT_UNKNOWN) {
    NS_WARNING("BufferTextureHost: unsupported format!");
    return nullptr;
  } else if (mFormat == gfx::FORMAT_YUV) {
    YCbCrImageDataDeserializer yuvDeserializer(GetBuffer());
    if (!yuvDeserializer.IsValid()) {
      return nullptr;
    }
    result = yuvDeserializer.ToDataSourceSurface();
  } else {
    ImageDataDeserializer deserializer(GetBuffer());
    if (!deserializer.IsValid()) {
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
, mShmem(new ipc::Shmem(aShmem))
, mDeallocator(aDeallocator)
{
  MOZ_COUNT_CTOR(ShmemTextureHost);
}

ShmemTextureHost::~ShmemTextureHost()
{
  DeallocateDeviceData();
  delete mShmem;
  MOZ_COUNT_DTOR(ShmemTextureHost);
}

void
ShmemTextureHost::DeallocateSharedData()
{
  if (mShmem) {
    MOZ_ASSERT(mDeallocator,
               "Shared memory would leak without a ISurfaceAllocator");
    mDeallocator->DeallocShmem(*mShmem);
    delete mShmem;
    mShmem = nullptr;
  }
}

void
ShmemTextureHost::ForgetSharedData()
{
  if (mShmem) {
    delete mShmem;
    mShmem = nullptr;
  }
}

void
ShmemTextureHost::OnActorDestroy()
{
  delete mShmem;
  mShmem = nullptr;
}

uint8_t* ShmemTextureHost::GetBuffer()
{
  return mShmem ? mShmem->get<uint8_t>() : nullptr;
}

MemoryTextureHost::MemoryTextureHost(uint8_t* aBuffer,
                                     gfx::SurfaceFormat aFormat,
                                     TextureFlags aFlags)
: BufferTextureHost(aFormat, aFlags)
, mBuffer(aBuffer)
{
  MOZ_COUNT_CTOR(MemoryTextureHost);
}

MemoryTextureHost::~MemoryTextureHost()
{
  DeallocateDeviceData();
  NS_ASSERTION(!mBuffer || (mFlags & TEXTURE_DEALLOCATE_CLIENT),
               "Leaking our buffer");
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

TextureParent::TextureParent(ISurfaceAllocator* aAllocator)
: mAllocator(aAllocator)
{
  MOZ_COUNT_CTOR(TextureParent);
}

TextureParent::~TextureParent()
{
  MOZ_COUNT_DTOR(TextureParent);
  mTextureHost = nullptr;
}

bool
TextureParent::RecvInit(const SurfaceDescriptor& aSharedData,
                        const TextureFlags& aFlags)
{
  mTextureHost = TextureHost::Create(aSharedData,
                                     mAllocator,
                                     aFlags);
  return !!mTextureHost;
}

bool
TextureParent::RecvRemoveTexture()
{
  return PTextureParent::Send__delete__(this);
}

bool
TextureParent::RecvRemoveTextureSync()
{
  
  
  return PTextureParent::Send__delete__(this);
}

void
TextureParent::ActorDestroy(ActorDestroyReason why)
{
  switch (why) {
  case AncestorDeletion:
    NS_WARNING("PTexture deleted after ancestor");
    
  case Deletion:
    if (mTextureHost && mTextureHost->GetFlags() & !TEXTURE_DEALLOCATE_CLIENT) {
      mTextureHost->DeallocateSharedData();
    }
    break;

  case NormalShutdown:
  case AbnormalShutdown:
    if (mTextureHost) {
      mTextureHost->OnActorDestroy();
    }
    break;

  case FailedConstructor:
    NS_RUNTIMEABORT("FailedConstructor isn't possible in PTexture");
  }
  mTextureHost->ForgetSharedData();
  mTextureHost = nullptr;
}

} 
} 
