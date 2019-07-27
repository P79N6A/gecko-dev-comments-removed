




#include "MacIOSurfaceTextureClientOGL.h"
#include "mozilla/gfx/MacIOSurface.h"

namespace mozilla {
namespace layers {

MacIOSurfaceTextureClientOGL::MacIOSurfaceTextureClientOGL(ISurfaceAllocator* aAllcator,
                                                           TextureFlags aFlags)
  : TextureClient(aAllcator, aFlags)
  , mIsLocked(false)
{}

MacIOSurfaceTextureClientOGL::~MacIOSurfaceTextureClientOGL()
{
  if (mActor && mSurface) {
    KeepUntilFullDeallocation(MakeUnique<TKeepAlive<MacIOSurface>>(mSurface));
  }
}


TemporaryRef<MacIOSurfaceTextureClientOGL>
MacIOSurfaceTextureClientOGL::Create(ISurfaceAllocator* aAllocator,
                                     TextureFlags aFlags,
                                     MacIOSurface* aSurface)
{
  RefPtr<MacIOSurfaceTextureClientOGL> texture =
      new MacIOSurfaceTextureClientOGL(aAllocator, aFlags);
  MOZ_ASSERT(texture->IsValid());
  MOZ_ASSERT(!texture->IsAllocated());
  texture->mSurface = aSurface;
  return texture.forget();
}

bool
MacIOSurfaceTextureClientOGL::Lock(OpenMode aMode)
{
  MOZ_ASSERT(!mIsLocked);
  mIsLocked = true;
  return IsValid() && IsAllocated();
}

void
MacIOSurfaceTextureClientOGL::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
}

bool
MacIOSurfaceTextureClientOGL::IsLocked() const
{
  return mIsLocked;
}

bool
MacIOSurfaceTextureClientOGL::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated()) {
    return false;
  }
  aOutDescriptor = SurfaceDescriptorMacIOSurface(mSurface->GetIOSurfaceID(),
                                                 mSurface->GetContentsScaleFactor(),
                                                 !mSurface->HasAlpha());
  return true;
}

gfx::IntSize
MacIOSurfaceTextureClientOGL::GetSize() const
{
  return gfx::IntSize(mSurface->GetDevicePixelWidth(), mSurface->GetDevicePixelHeight());
}

TemporaryRef<gfx::DataSourceSurface>
MacIOSurfaceTextureClientOGL::GetAsSurface()
{
  RefPtr<gfx::SourceSurface> surf = mSurface->GetAsSurface();
  return surf->GetDataSurface();
}
}
}
