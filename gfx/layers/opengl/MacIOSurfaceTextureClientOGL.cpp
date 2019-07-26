




#include "MacIOSurfaceTextureClientOGL.h"
#include "mozilla/gfx/MacIOSurface.h"

namespace mozilla {
namespace layers {

MacIOSurfaceTextureClientOGL::MacIOSurfaceTextureClientOGL(TextureFlags aFlags)
  : TextureClient(aFlags)
  , mIsLocked(false)
{}

MacIOSurfaceTextureClientOGL::~MacIOSurfaceTextureClientOGL()
{}

void
MacIOSurfaceTextureClientOGL::InitWith(MacIOSurface* aSurface)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(!IsAllocated());
  mSurface = aSurface;
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
                                                 mSurface->HasAlpha());
  return true;
}

gfx::IntSize
MacIOSurfaceTextureClientOGL::GetSize() const
{
  return gfx::IntSize(mSurface->GetDevicePixelWidth(), mSurface->GetDevicePixelHeight());
}

}
}
