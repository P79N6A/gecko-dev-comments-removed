




#include "TextureD3D9.h"
#include "CompositorD3D9.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "Effects.h"
#include "mozilla/layers/YCbCrImageDataSerializer.h"
#include "gfxWindowsPlatform.h"
#include "gfx2DGlue.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

static uint32_t GetRequiredTilesD3D9(uint32_t aSize, uint32_t aMaxSize)
{
  uint32_t requiredTiles = aSize / aMaxSize;
  if (aSize % aMaxSize) {
    requiredTiles++;
  }
  return requiredTiles;
}

TextureSourceD3D9::~TextureSourceD3D9()
{
  MOZ_ASSERT(!mCreatingDeviceManager ||
             mCreatingDeviceManager->IsInTextureHostList(this),
             "Inconsistency in list of texture hosts.");
  
  if (mPreviousHost) {
    MOZ_ASSERT(mPreviousHost->mNextHost == this);
    mPreviousHost->mNextHost = mNextHost;
  } else if (mCreatingDeviceManager) {
    mCreatingDeviceManager->RemoveTextureListHead(this);
  }
  if (mNextHost) {
    MOZ_ASSERT(mNextHost->mPreviousHost == this);
    mNextHost->mPreviousHost = mPreviousHost;
  }
}

TemporaryRef<TextureHost>
CreateTextureHostD3D9(const SurfaceDescriptor& aDesc,
                      ISurfaceAllocator* aDeallocator,
                      TextureFlags aFlags)
{
  RefPtr<TextureHost> result;
  switch (aDesc.type()) {
    case SurfaceDescriptor::TSurfaceDescriptorShmem:
    case SurfaceDescriptor::TSurfaceDescriptorMemory: {
      result = CreateBackendIndependentTextureHost(aDesc, aDeallocator, aFlags);
      break;
    }
    case SurfaceDescriptor::TSurfaceDescriptorD3D9: {
      result = new TextureHostD3D9(aFlags, aDesc);
      break;
    }
    case SurfaceDescriptor::TSurfaceDescriptorDIB: {
      result = new DIBTextureHostD3D9(aFlags, aDesc);
      break;
    }
    case SurfaceDescriptor::TSurfaceDescriptorD3D10: {
      result = new DXGITextureHostD3D9(aFlags, aDesc);
      break;
    }
    default: {
      NS_WARNING("Unsupported SurfaceDescriptor type");
    }
  }
  return result.forget();
}

static SurfaceFormat
D3D9FormatToSurfaceFormat(_D3DFORMAT format)
{
  switch (format) {
  case D3DFMT_X8R8G8B8:
    return SurfaceFormat::B8G8R8X8;
  case D3DFMT_A8R8G8B8:
    return SurfaceFormat::B8G8R8A8;
  case D3DFMT_A8:
    return SurfaceFormat::A8;
  default:
    NS_ERROR("Bad texture format");
  }
  return SurfaceFormat::UNKNOWN;
}

static _D3DFORMAT
SurfaceFormatToD3D9Format(SurfaceFormat format)
{
  switch (format) {
  case SurfaceFormat::B8G8R8X8:
    return D3DFMT_X8R8G8B8;
  case SurfaceFormat::B8G8R8A8:
    return D3DFMT_A8R8G8B8;
  case SurfaceFormat::A8:
    return D3DFMT_A8;
  default:
    NS_ERROR("Bad texture format");
  }
  return D3DFMT_A8R8G8B8;
}

CompositingRenderTargetD3D9::CompositingRenderTargetD3D9(IDirect3DTexture9* aTexture,
                                                         SurfaceInitMode aInit,
                                                         const gfx::IntRect& aRect)
  : CompositingRenderTarget(aRect.TopLeft())
  , mInitMode(aInit)
  , mInitialized(false)
{
  MOZ_COUNT_CTOR(CompositingRenderTargetD3D9);
  MOZ_ASSERT(aTexture);

  mTexture = aTexture;
  HRESULT hr = mTexture->GetSurfaceLevel(0, getter_AddRefs(mSurface));
  NS_ASSERTION(mSurface, "Couldn't create surface for texture");
  TextureSourceD3D9::SetSize(aRect.Size());
}

CompositingRenderTargetD3D9::CompositingRenderTargetD3D9(IDirect3DSurface9* aSurface,
                                                         SurfaceInitMode aInit,
                                                         const gfx::IntRect& aRect)
  : CompositingRenderTarget(aRect.TopLeft())
  , mSurface(aSurface)
  , mInitMode(aInit)
  , mInitialized(false)
{
  MOZ_COUNT_CTOR(CompositingRenderTargetD3D9);
  MOZ_ASSERT(mSurface);
  TextureSourceD3D9::SetSize(aRect.Size());
}

CompositingRenderTargetD3D9::~CompositingRenderTargetD3D9()
{
  MOZ_COUNT_DTOR(CompositingRenderTargetD3D9);
}

void
CompositingRenderTargetD3D9::BindRenderTarget(IDirect3DDevice9* aDevice)
{
  aDevice->SetRenderTarget(0, mSurface);
  if (!mInitialized &&
      mInitMode == INIT_MODE_CLEAR) {
    mInitialized = true;
    aDevice->Clear(0, 0, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 0, 0);
  }
}

IntSize
CompositingRenderTargetD3D9::GetSize() const
{
  return TextureSourceD3D9::GetSize();
}






TemporaryRef<IDirect3DTexture9>
TextureSourceD3D9::InitTextures(DeviceManagerD3D9* aDeviceManager,
                                const IntSize &aSize,
                                _D3DFORMAT aFormat,
                                RefPtr<IDirect3DSurface9>& aSurface,
                                D3DLOCKED_RECT& aLockedRect)
{
  if (!aDeviceManager) {
    return nullptr;
  }
  RefPtr<IDirect3DTexture9> result;
  
  
  
  
  result = aDeviceManager->CreateTexture(aSize, aFormat, D3DPOOL_DEFAULT, this);
  if (!result) {
    return nullptr;
  }

  RefPtr<IDirect3DTexture9> tmpTexture =
    aDeviceManager->CreateTexture(aSize, aFormat, D3DPOOL_SYSTEMMEM, this);
  if (!tmpTexture) {
    return nullptr;
  }

  tmpTexture->GetSurfaceLevel(0, byRef(aSurface));
  aSurface->LockRect(&aLockedRect, nullptr, 0);
  if (!aLockedRect.pBits) {
    NS_WARNING("Could not lock surface");
    return nullptr;
  }

  return result;
}




static void
FinishTextures(DeviceManagerD3D9* aDeviceManager,
               IDirect3DTexture9* aTexture,
               IDirect3DSurface9* aSurface)
{
  if (!aDeviceManager) {
    return;
  }

  aSurface->UnlockRect();
  nsRefPtr<IDirect3DSurface9> dstSurface;
  aTexture->GetSurfaceLevel(0, getter_AddRefs(dstSurface));
  aDeviceManager->device()->UpdateSurface(aSurface, nullptr, dstSurface,
                                          nullptr);
}

TemporaryRef<IDirect3DTexture9>
TextureSourceD3D9::DataToTexture(DeviceManagerD3D9* aDeviceManager,
                                 unsigned char *aData,
                                 int aStride,
                                 const IntSize &aSize,
                                 _D3DFORMAT aFormat,
                                 uint32_t aBPP)
{
  RefPtr<IDirect3DSurface9> surface;
  D3DLOCKED_RECT lockedRect;
  RefPtr<IDirect3DTexture9> texture = InitTextures(aDeviceManager, aSize, aFormat,
                                                   surface, lockedRect);
  if (!texture) {
    return nullptr;
  }

  uint32_t width = aSize.width * aBPP;

  for (int y = 0; y < aSize.height; y++) {
    memcpy((char*)lockedRect.pBits + lockedRect.Pitch * y,
            aData + aStride * y,
            width);
  }

  FinishTextures(aDeviceManager, texture, surface);

  return texture;
}

TemporaryRef<IDirect3DTexture9>
TextureSourceD3D9::TextureToTexture(DeviceManagerD3D9* aDeviceManager,
                                    IDirect3DTexture9* aTexture,
                                    const IntSize& aSize,
                                    _D3DFORMAT aFormat)
{
  if (!aDeviceManager) {
    return nullptr;
  }

  RefPtr<IDirect3DTexture9> texture =
    aDeviceManager->CreateTexture(aSize, aFormat, D3DPOOL_DEFAULT, this);
  if (!texture) {
    return nullptr;
  }

  HRESULT hr = aDeviceManager->device()->UpdateTexture(aTexture, texture);
  if (FAILED(hr)) {
    return nullptr;
  }

  return texture;
}

TemporaryRef<IDirect3DTexture9>
TextureSourceD3D9::SurfaceToTexture(DeviceManagerD3D9* aDeviceManager,
                                    gfxWindowsSurface* aSurface,
                                    const IntSize& aSize,
                                    _D3DFORMAT aFormat)
{
  RefPtr<IDirect3DSurface9> surface;
  D3DLOCKED_RECT lockedRect;

  RefPtr<IDirect3DTexture9> texture = InitTextures(aDeviceManager, aSize, aFormat,
                                                   surface, lockedRect);
  if (!texture) {
    return nullptr;
  }

  {
    RefPtr<DrawTarget> dt =
      Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                       reinterpret_cast<unsigned char*>(lockedRect.pBits),
                                       aSize, lockedRect.Pitch,
                                       gfxPlatform::GetPlatform()->Optimal2DFormatForContent(aSurface->GetContentType()));
    NativeSurface nativeSurf;
    nativeSurf.mSize = aSize;
    nativeSurf.mType = NativeSurfaceType::CAIRO_SURFACE;
    
    
    nativeSurf.mFormat = dt->GetFormat();
    nativeSurf.mSurface = aSurface->CairoSurface();

    RefPtr<SourceSurface> surf = dt->CreateSourceSurfaceFromNativeSurface(nativeSurf);

    dt->CopySurface(surf, IntRect(IntPoint(), aSize), IntPoint());
  }

  FinishTextures(aDeviceManager, texture, surface);

  return texture;
}

DataTextureSourceD3D9::DataTextureSourceD3D9(gfx::SurfaceFormat aFormat,
                                             CompositorD3D9* aCompositor,
                                             TextureFlags aFlags,
                                             StereoMode aStereoMode)
  : mFormat(aFormat)
  , mCompositor(aCompositor)
  , mCurrentTile(0)
  , mFlags(aFlags)
  , mIsTiled(false)
  , mIterating(false)
{
  mStereoMode = aStereoMode;
  MOZ_COUNT_CTOR(DataTextureSourceD3D9);
}

DataTextureSourceD3D9::DataTextureSourceD3D9(gfx::SurfaceFormat aFormat,
                                             gfx::IntSize aSize,
                                             CompositorD3D9* aCompositor,
                                             IDirect3DTexture9* aTexture,
                                             TextureFlags aFlags)
  : mFormat(aFormat)
  , mCompositor(aCompositor)
  , mCurrentTile(0)
  , mFlags(aFlags)
  , mIsTiled(false)
  , mIterating(false)
{
  mSize = aSize;
  mTexture = aTexture;
  mStereoMode = StereoMode::MONO;
  MOZ_COUNT_CTOR(DataTextureSourceD3D9);
}

DataTextureSourceD3D9::~DataTextureSourceD3D9()
{
  MOZ_COUNT_DTOR(DataTextureSourceD3D9);
}

IDirect3DTexture9*
DataTextureSourceD3D9::GetD3D9Texture()
{
  return mIterating ? mTileTextures[mCurrentTile]
                    : mTexture;
}

bool
DataTextureSourceD3D9::Update(gfx::DataSourceSurface* aSurface,
                              nsIntRegion* aDestRegion,
                              gfx::IntPoint* aSrcOffset)
{
  
  
  
  MOZ_ASSERT(!aSrcOffset);

  if (!mCompositor || !mCompositor->device()) {
    NS_WARNING("No D3D device to update the texture.");
    return false;
  }
  mSize = aSurface->GetSize();

  uint32_t bpp = 0;

  _D3DFORMAT format = D3DFMT_A8R8G8B8;
  mFormat = aSurface->GetFormat();
  switch (mFormat) {
  case SurfaceFormat::B8G8R8X8:
    format = D3DFMT_X8R8G8B8;
    bpp = 4;
    break;
  case SurfaceFormat::B8G8R8A8:
    format = D3DFMT_A8R8G8B8;
    bpp = 4;
    break;
  case SurfaceFormat::A8:
    format = D3DFMT_A8;
    bpp = 1;
    break;
  default:
    NS_WARNING("Bad image format");
    return false;
  }

  int32_t maxSize = mCompositor->GetMaxTextureSize();
  DeviceManagerD3D9* deviceManager = gfxWindowsPlatform::GetPlatform()->GetD3D9DeviceManager();
  if ((mSize.width <= maxSize && mSize.height <= maxSize) ||
      (mFlags & TextureFlags::DISALLOW_BIGIMAGE)) {
    mTexture = DataToTexture(deviceManager,
                             aSurface->GetData(), aSurface->Stride(),
                             IntSize(mSize), format, bpp);
    if (!mTexture) {
      NS_WARNING("Could not upload texture");
      Reset();
      return false;
    }
    mIsTiled = false;
  } else {
    mIsTiled = true;
    uint32_t tileCount = GetRequiredTilesD3D9(mSize.width, maxSize) *
                         GetRequiredTilesD3D9(mSize.height, maxSize);
    mTileTextures.resize(tileCount);
    mTexture = nullptr;

    for (uint32_t i = 0; i < tileCount; i++) {
      IntRect tileRect = GetTileRect(i);
      unsigned char* data = aSurface->GetData() +
                            tileRect.y * aSurface->Stride() +
                            tileRect.x * bpp;
      mTileTextures[i] = DataToTexture(deviceManager,
                                       data,
                                       aSurface->Stride(),
                                       IntSize(tileRect.width, tileRect.height),
                                       format,
                                       bpp);
      if (!mTileTextures[i]) {
        NS_WARNING("Could not upload texture");
        Reset();
        return false;
      }
    }
  }

  return true;
}

bool
DataTextureSourceD3D9::Update(gfxWindowsSurface* aSurface)
{
  MOZ_ASSERT(aSurface);
  if (!mCompositor || !mCompositor->device()) {
    NS_WARNING("No D3D device to update the texture.");
    return false;
  }
  mSize = ToIntSize(aSurface->GetSize());

  uint32_t bpp = 0;

  _D3DFORMAT format = D3DFMT_A8R8G8B8;
  mFormat = ImageFormatToSurfaceFormat(
    gfxPlatform::GetPlatform()->OptimalFormatForContent(aSurface->GetContentType()));
  switch (mFormat) {
  case SurfaceFormat::B8G8R8X8:
    format = D3DFMT_X8R8G8B8;
    bpp = 4;
    break;
  case SurfaceFormat::B8G8R8A8:
    format = D3DFMT_A8R8G8B8;
    bpp = 4;
    break;
  case SurfaceFormat::A8:
    format = D3DFMT_A8;
    bpp = 1;
    break;
  default:
    NS_WARNING("Bad image format");
    return false;
  }

  int32_t maxSize = mCompositor->GetMaxTextureSize();
  DeviceManagerD3D9* deviceManager = gfxWindowsPlatform::GetPlatform()->GetD3D9DeviceManager();
  if ((mSize.width <= maxSize && mSize.height <= maxSize) ||
      (mFlags & TextureFlags::DISALLOW_BIGIMAGE)) {
    mTexture = SurfaceToTexture(deviceManager, aSurface, mSize, format);

    if (!mTexture) {
      NS_WARNING("Could not upload texture");
      Reset();
      return false;
    }
    mIsTiled = false;
  } else {
    mIsTiled = true;
    uint32_t tileCount = GetRequiredTilesD3D9(mSize.width, maxSize) *
                         GetRequiredTilesD3D9(mSize.height, maxSize);
    mTileTextures.resize(tileCount);
    mTexture = nullptr;
    nsRefPtr<gfxImageSurface> imgSurface = aSurface->GetAsImageSurface();

    for (uint32_t i = 0; i < tileCount; i++) {
      IntRect tileRect = GetTileRect(i);
      unsigned char* data = imgSurface->Data() +
                            tileRect.y * imgSurface->Stride() +
                            tileRect.x * bpp;
      mTileTextures[i] = DataToTexture(deviceManager,
                                       data,
                                       imgSurface->Stride(),
                                       IntSize(tileRect.width, tileRect.height),
                                       format,
                                       bpp);
      if (!mTileTextures[i]) {
        NS_WARNING("Could not upload texture");
        Reset();
        return false;
      }
    }
  }

  return true;
}

void
DataTextureSourceD3D9::SetCompositor(Compositor* aCompositor)
{
  CompositorD3D9* d3dCompositor = static_cast<CompositorD3D9*>(aCompositor);
  if (mCompositor && mCompositor != d3dCompositor) {
    Reset();
  }
  mCompositor = d3dCompositor;
}

void
DataTextureSourceD3D9::Reset()
{
  mSize.width = 0;
  mSize.height = 0;
  mIsTiled = false;
  mTexture = nullptr;
  mTileTextures.clear();
}

IntRect
DataTextureSourceD3D9::GetTileRect(uint32_t aTileIndex) const
{
  uint32_t maxSize = mCompositor->GetMaxTextureSize();
  uint32_t horizontalTiles = GetRequiredTilesD3D9(mSize.width, maxSize);
  uint32_t verticalTiles = GetRequiredTilesD3D9(mSize.height, maxSize);

  uint32_t verticalTile = aTileIndex / horizontalTiles;
  uint32_t horizontalTile = aTileIndex % horizontalTiles;

  return IntRect(horizontalTile * maxSize,
                 verticalTile * maxSize,
                 horizontalTile < (horizontalTiles - 1) ? maxSize : mSize.width % maxSize,
                 verticalTile < (verticalTiles - 1) ? maxSize : mSize.height % maxSize);
}

nsIntRect
DataTextureSourceD3D9::GetTileRect()
{
  return ThebesIntRect(GetTileRect(mCurrentTile));
}

CairoTextureClientD3D9::CairoTextureClientD3D9(gfx::SurfaceFormat aFormat, TextureFlags aFlags)
  : TextureClient(aFlags)
  , mFormat(aFormat)
  , mIsLocked(false)
  , mNeedsClear(false)
  , mNeedsClearWhite(false)
  , mLockRect(false)
{
  MOZ_COUNT_CTOR(CairoTextureClientD3D9);
}

CairoTextureClientD3D9::~CairoTextureClientD3D9()
{
  MOZ_COUNT_DTOR(CairoTextureClientD3D9);
}

bool
CairoTextureClientD3D9::Lock(OpenMode aMode)
{
  MOZ_ASSERT(!mIsLocked);
  if (!IsValid() || !IsAllocated()) {
    return false;
  }

  if (!gfxWindowsPlatform::GetPlatform()->GetD3D9Device()) {
    
    
    mD3D9Surface = nullptr;
    return false;
  }

  if (!mD3D9Surface) {
    HRESULT hr = mTexture->GetSurfaceLevel(0, getter_AddRefs(mD3D9Surface));
    if (FAILED(hr)) {
      NS_WARNING("Failed to get texture surface level.");
      return false;
    }
  }

  mIsLocked = true;

  
  
  if (aMode & OpenMode::OPEN_WRITE) {
    mDrawTarget = BorrowDrawTarget();
    if (!mDrawTarget) {
      Unlock();
      return false;
    }
  }

  if (mNeedsClear) {
    mDrawTarget = BorrowDrawTarget();
    mDrawTarget->ClearRect(Rect(0, 0, GetSize().width, GetSize().height));
    mNeedsClear = false;
  }
  if (mNeedsClearWhite) {
    mDrawTarget = BorrowDrawTarget();
    mDrawTarget->FillRect(Rect(0, 0, GetSize().width, GetSize().height), ColorPattern(Color(1.0, 1.0, 1.0, 1.0)));
    mNeedsClearWhite = false;
  }

  return true;
}

void
CairoTextureClientD3D9::Unlock()
{
  MOZ_ASSERT(mIsLocked, "Unlocked called while the texture is not locked!");
  if (mDrawTarget) {
    mDrawTarget->Flush();
    mDrawTarget = nullptr;
  }

  if (mLockRect) {
    mD3D9Surface->UnlockRect();
    mLockRect = false;
  }

  if (mSurface) {
    mSurface = nullptr;
  }
  mIsLocked = false;
}

bool
CairoTextureClientD3D9::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated()) {
    return false;
  }

  mTexture->AddRef(); 
  aOutDescriptor = SurfaceDescriptorD3D9(reinterpret_cast<uintptr_t>(mTexture.get()));
  return true;
}

gfx::DrawTarget*
CairoTextureClientD3D9::BorrowDrawTarget()
{
  MOZ_ASSERT(mIsLocked && mD3D9Surface);
  if (mDrawTarget) {
    return mDrawTarget;
  }

  if (ContentForFormat(mFormat) == gfxContentType::COLOR) {
    mSurface = new gfxWindowsSurface(mD3D9Surface);
    if (!mSurface || mSurface->CairoStatus()) {
      NS_WARNING("Could not create surface for d3d9 surface");
      mSurface = nullptr;
      return nullptr;
    }
  } else {
    
    
    
    D3DLOCKED_RECT rect;
    mD3D9Surface->LockRect(&rect, nullptr, 0);
    mSurface = new gfxImageSurface((uint8_t*)rect.pBits, ThebesIntSize(mSize),
                                   rect.Pitch, SurfaceFormatToImageFormat(mFormat));
    mLockRect = true;
  }

  mDrawTarget =
    gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(mSurface, mSize);

  return mDrawTarget;
}

bool
CairoTextureClientD3D9::AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags aFlags)
{
  MOZ_ASSERT(!IsAllocated());
  mSize = aSize;
  _D3DFORMAT format = SurfaceFormatToD3D9Format(mFormat);

  DeviceManagerD3D9* deviceManager = gfxWindowsPlatform::GetPlatform()->GetD3D9DeviceManager();
  if (!deviceManager ||
      !(mTexture = deviceManager->CreateTexture(mSize, format, D3DPOOL_SYSTEMMEM, nullptr))) {
    NS_WARNING("Could not create d3d9 texture");
    return false;
  }

  mNeedsClear = aFlags & ALLOC_CLEAR_BUFFER;
  mNeedsClearWhite = aFlags & ALLOC_CLEAR_BUFFER_WHITE;

  MOZ_ASSERT(mTexture);
  return true;
}

DIBTextureClientD3D9::DIBTextureClientD3D9(gfx::SurfaceFormat aFormat, TextureFlags aFlags)
  : TextureClient(aFlags)
  , mFormat(aFormat)
  , mIsLocked(false)
{
  MOZ_COUNT_CTOR(DIBTextureClientD3D9);
}

DIBTextureClientD3D9::~DIBTextureClientD3D9()
{
  MOZ_COUNT_DTOR(DIBTextureClientD3D9);
}

bool
DIBTextureClientD3D9::Lock(OpenMode)
{
  MOZ_ASSERT(!mIsLocked);
  if (!IsValid()) {
    return false;
  }
  mIsLocked = true;
  return true;
}

void
DIBTextureClientD3D9::Unlock()
{
  MOZ_ASSERT(mIsLocked, "Unlocked called while the texture is not locked!");
  if (mDrawTarget) {
    mDrawTarget->Flush();
    mDrawTarget = nullptr;
  }

  mIsLocked = false;
}

bool
DIBTextureClientD3D9::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated()) {
    return false;
  }
  MOZ_ASSERT(mSurface);
  
  
  aOutDescriptor = SurfaceDescriptorDIB(reinterpret_cast<uintptr_t>(mSurface.get()));
  mSurface->AddRef();
  return true;
}

gfx::DrawTarget*
DIBTextureClientD3D9::BorrowDrawTarget()
{
  MOZ_ASSERT(mIsLocked && IsAllocated());

  if (!mDrawTarget) {
    mDrawTarget =
      gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(mSurface, mSize);
  }

  return mDrawTarget;
}

bool
DIBTextureClientD3D9::AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags aFlags)
{
  MOZ_ASSERT(!IsAllocated());
  mSize = aSize;

  mSurface = new gfxWindowsSurface(gfxIntSize(aSize.width, aSize.height),
                                   SurfaceFormatToImageFormat(mFormat));
  if (!mSurface || mSurface->CairoStatus())
  {
    NS_WARNING("Could not create surface");
    mSurface = nullptr;
    return false;
  }

  return true;
}

SharedTextureClientD3D9::SharedTextureClientD3D9(gfx::SurfaceFormat aFormat, TextureFlags aFlags)
  : TextureClient(aFlags)
  , mFormat(aFormat)
  , mHandle(0)
  , mIsLocked(false)
{
  MOZ_COUNT_CTOR(SharedTextureClientD3D9);
}

SharedTextureClientD3D9::~SharedTextureClientD3D9()
{
  MOZ_COUNT_DTOR(SharedTextureClientD3D9);
}

bool
SharedTextureClientD3D9::Lock(OpenMode)
{
  MOZ_ASSERT(!mIsLocked);
  if (!IsValid()) {
    return false;
  }
  mIsLocked = true;
  return true;
}

void
SharedTextureClientD3D9::Unlock()
{
  MOZ_ASSERT(mIsLocked, "Unlock called while the texture is not locked!");
}

bool
SharedTextureClientD3D9::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated()) {
    return false;
  }

  aOutDescriptor = SurfaceDescriptorD3D10((WindowsHandle)(mHandle), mFormat, GetSize());
  return true;
}

TextureHostD3D9::TextureHostD3D9(TextureFlags aFlags,
                                 const SurfaceDescriptorD3D9& aDescriptor)
  : TextureHost(aFlags)
  , mFormat(SurfaceFormat::UNKNOWN)
  , mIsLocked(false)
{
  mTexture = reinterpret_cast<IDirect3DTexture9*>(aDescriptor.texture());
  mTexture->Release(); 
  MOZ_ASSERT(mTexture);
  D3DSURFACE_DESC desc;
  HRESULT hr = mTexture->GetLevelDesc(0, &desc);
  if (!FAILED(hr)) {
    mFormat = D3D9FormatToSurfaceFormat(desc.Format);
    mSize.width = desc.Width;
    mSize.height = desc.Height;
  }
}

bool
DataTextureSourceD3D9::UpdateFromTexture(IDirect3DTexture9* aTexture,
                                         const nsIntRegion* aRegion)
{
  MOZ_ASSERT(aTexture);

  D3DSURFACE_DESC desc;
  HRESULT hr = aTexture->GetLevelDesc(0, &desc);
  if (FAILED(hr)) {
    return false;
  } else {
    
    
    MOZ_ASSERT(mFormat == D3D9FormatToSurfaceFormat(desc.Format));
    MOZ_ASSERT(!mSize.width || mSize.width == desc.Width);
    MOZ_ASSERT(!mSize.height || mSize.height == desc.Height);
    mSize = IntSize(desc.Width, desc.Height);
  }

  DeviceManagerD3D9* dm = gfxWindowsPlatform::GetPlatform()->GetD3D9DeviceManager();
  if (!mTexture) {
    mTexture = dm->CreateTexture(mSize, SurfaceFormatToD3D9Format(mFormat),
                                 D3DPOOL_DEFAULT, this);
    if (!mTexture) {
      NS_WARNING("Failed to create a texture");
      return false;
    }
  }

  RefPtr<IDirect3DSurface9> srcSurface;
  RefPtr<IDirect3DSurface9> dstSurface;

  hr = aTexture->GetSurfaceLevel(0, byRef(srcSurface));
  if (FAILED(hr)) {
    return false;
  }
  hr = mTexture->GetSurfaceLevel(0, byRef(dstSurface));
  if (FAILED(hr)) {
    return false;
  }

  if (aRegion) {
    nsIntRegionRectIterator iter(*aRegion);
    const nsIntRect *iterRect;
    while ((iterRect = iter.Next())) {
      RECT rect;
      rect.left = iterRect->x;
      rect.top = iterRect->y;
      rect.right = iterRect->XMost();
      rect.bottom = iterRect->YMost();

      POINT point;
      point.x = iterRect->x;
      point.y = iterRect->y;
      hr = dm->device()->UpdateSurface(srcSurface, &rect, dstSurface, &point);
      if (FAILED(hr)) {
        NS_WARNING("Failed Update the surface");
        return false;
      }
    }
  } else {
    hr = dm->device()->UpdateSurface(srcSurface, nullptr, dstSurface, nullptr);
    if (FAILED(hr)) {
      NS_WARNING("Failed Update the surface");
      return false;
    }
  }
  mIsTiled = false;
  return true;
}

void
TextureHostD3D9::Updated(const nsIntRegion* aRegion)
{
  if (!mTexture) {
    return;
  }

  if (!mTextureSource) {
    mTextureSource = new DataTextureSourceD3D9(mFormat, mSize, mCompositor,
                                               nullptr, mFlags);
  }

  mTextureSource->UpdateFromTexture(mTexture, aRegion);
}

IDirect3DDevice9*
TextureHostD3D9::GetDevice()
{
  return mCompositor ? mCompositor->device() : nullptr;
}

void
TextureHostD3D9::SetCompositor(Compositor* aCompositor)
{
  mCompositor = static_cast<CompositorD3D9*>(aCompositor);
  if (mTextureSource) {
    mTextureSource->SetCompositor(aCompositor);
  }
}

NewTextureSource*
TextureHostD3D9::GetTextureSources()
{
  MOZ_ASSERT(mIsLocked);
  return mTextureSource;
}

bool
TextureHostD3D9::Lock()
{
  MOZ_ASSERT(!mIsLocked);
  mIsLocked = true;
  return true;
}

void
TextureHostD3D9::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
}

void
TextureHostD3D9::DeallocateDeviceData()
{
  mTextureSource = nullptr;
}

DIBTextureHostD3D9::DIBTextureHostD3D9(TextureFlags aFlags,
                                       const SurfaceDescriptorDIB& aDescriptor)
  : TextureHost(aFlags)
  , mIsLocked(false)
{
  
  mSurface =
    dont_AddRef(reinterpret_cast<gfxWindowsSurface*>(aDescriptor.surface()));
  MOZ_ASSERT(mSurface);

  mSize = ToIntSize(mSurface->GetSize());
  mFormat = ImageFormatToSurfaceFormat(
    gfxPlatform::GetPlatform()->OptimalFormatForContent(mSurface->GetContentType()));
}

NewTextureSource*
DIBTextureHostD3D9::GetTextureSources()
{
  if (!mTextureSource) {
    Updated();
  }

  return mTextureSource;
}

void
DIBTextureHostD3D9::Updated(const nsIntRegion*)
{
  if (!mTextureSource) {
    mTextureSource = new DataTextureSourceD3D9(mFormat, mCompositor, mFlags);
  }

  if (!mTextureSource->Update(mSurface)) {
    mTextureSource = nullptr;
  }
}

bool
DIBTextureHostD3D9::Lock()
{
  MOZ_ASSERT(!mIsLocked);
  mIsLocked = true;
  return true;
}

void
DIBTextureHostD3D9::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
}

void
DIBTextureHostD3D9::SetCompositor(Compositor* aCompositor)
{
  mCompositor = static_cast<CompositorD3D9*>(aCompositor);
}

void
DIBTextureHostD3D9::DeallocateDeviceData()
{
  mTextureSource = nullptr;
}

DXGITextureHostD3D9::DXGITextureHostD3D9(TextureFlags aFlags,
  const SurfaceDescriptorD3D10& aDescriptor)
  : TextureHost(aFlags)
  , mHandle(aDescriptor.handle())
  , mFormat(aDescriptor.format())
  , mSize(aDescriptor.size())
  , mIsLocked(false)
{
  MOZ_ASSERT(mHandle);
}

NewTextureSource*
DXGITextureHostD3D9::GetTextureSources()
{
  return mTextureSource;
}

bool
DXGITextureHostD3D9::Lock()
{
  DeviceManagerD3D9* deviceManager = gfxWindowsPlatform::GetPlatform()->GetD3D9DeviceManager();
  if (!deviceManager) {
    NS_WARNING("trying to lock a TextureHost without a D3D device");
    return false;
  }

  if (!mTextureSource) {
    nsRefPtr<IDirect3DTexture9> tex;
    HRESULT hr = deviceManager->device()->CreateTexture(mSize.width,
                                                        mSize.height,
                                                        1,
                                                        D3DUSAGE_RENDERTARGET,
                                                        SurfaceFormatToD3D9Format(mFormat),
                                                        D3DPOOL_DEFAULT,
                                                        getter_AddRefs(tex),
                                                        (HANDLE*)&mHandle);
    if (FAILED(hr)) {
      NS_WARNING("Failed to open shared texture");
      return false;
    }

    mTextureSource = new DataTextureSourceD3D9(mFormat, mSize, mCompositor, tex);
  }

  MOZ_ASSERT(!mIsLocked);
  mIsLocked = true;
  return true;
}

void
DXGITextureHostD3D9::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
}

void
DXGITextureHostD3D9::SetCompositor(Compositor* aCompositor)
{
  mCompositor = static_cast<CompositorD3D9*>(aCompositor);
}

void
DXGITextureHostD3D9::DeallocateDeviceData()
{
  mTextureSource = nullptr;
}

}
}
