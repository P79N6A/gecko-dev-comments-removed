



#include "mozilla/layers/TextureClientX11.h"
#include "mozilla/layers/CompositableClient.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/ShadowLayerUtilsX11.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Logging.h"
#include "gfxXlibSurface.h"
#include "gfx2DGlue.h"

#include "mozilla/X11Util.h"
#include <X11/Xlib.h>

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;

TextureClientX11::TextureClientX11(ISurfaceAllocator* aAllocator, SurfaceFormat aFormat, TextureFlags aFlags)
  : TextureClient(aFlags),
    mFormat(aFormat),
    mAllocator(aAllocator),
    mLocked(false)
{
  MOZ_COUNT_CTOR(TextureClientX11);
}

TextureClientX11::~TextureClientX11()
{
  MOZ_COUNT_DTOR(TextureClientX11);
}

TemporaryRef<TextureClient>
TextureClientX11::CreateSimilar(TextureFlags aFlags,
                                TextureAllocationFlags aAllocFlags) const
{
  RefPtr<TextureClient> tex = new TextureClientX11(mAllocator, mFormat, mFlags);

  
  MOZ_ASSERT(mSize.width >= 0 && mSize.height >= 0);
  if (!tex->AllocateForSurface(mSize, aAllocFlags)) {
    return nullptr;
  }

  return tex;
}

bool
TextureClientX11::IsAllocated() const
{
  return !!mSurface;
}

bool
TextureClientX11::Lock(OpenMode aMode)
{
  MOZ_ASSERT(!mLocked, "The TextureClient is already Locked!");
  mLocked = IsValid() && IsAllocated();
  return mLocked;
}

void
TextureClientX11::Unlock()
{
  MOZ_ASSERT(mLocked, "The TextureClient is already Unlocked!");
  mLocked = false;

  if (mDrawTarget) {
    
    
    
    
    MOZ_ASSERT(mDrawTarget->refCount() == 1);

    mDrawTarget->Flush();
    mDrawTarget = nullptr;
  }

  if (mSurface && !mAllocator->IsSameProcess()) {
    FinishX(DefaultXDisplay());
  }
}

bool
TextureClientX11::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!mSurface) {
    return false;
  }

  if (!(mFlags & TextureFlags::DEALLOCATE_CLIENT)) {
    
    
    
    mSurface->ReleasePixmap();
  }

  aOutDescriptor = SurfaceDescriptorX11(mSurface);
  return true;
}

bool
TextureClientX11::AllocateForSurface(IntSize aSize, TextureAllocationFlags aTextureFlags)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(!IsAllocated());
  

  MOZ_ASSERT(aSize.width >= 0 && aSize.height >= 0);
  if (aSize.width <= 0 || aSize.height <= 0) {
    gfxDebug() << "Asking for X11 surface of invalid size " << aSize.width << "x" << aSize.height;
    return false;
  }
  gfxContentType contentType = ContentForFormat(mFormat);
  nsRefPtr<gfxASurface> surface = gfxPlatform::GetPlatform()->CreateOffscreenSurface(aSize, contentType);
  if (!surface || surface->GetType() != gfxSurfaceType::Xlib) {
    NS_ERROR("creating Xlib surface failed!");
    return false;
  }

  mSize = aSize;
  mSurface = static_cast<gfxXlibSurface*>(surface.get());

  if (!mAllocator->IsSameProcess()) {
    FinishX(DefaultXDisplay());
  }

  return true;
}

DrawTarget*
TextureClientX11::BorrowDrawTarget()
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(mLocked);

  if (!mSurface) {
    return nullptr;
  }

  if (!mDrawTarget) {
    IntSize size = ToIntSize(mSurface->GetSize());
    mDrawTarget = Factory::CreateDrawTargetForCairoSurface(mSurface->CairoSurface(), size);
  }

  return mDrawTarget;
}
