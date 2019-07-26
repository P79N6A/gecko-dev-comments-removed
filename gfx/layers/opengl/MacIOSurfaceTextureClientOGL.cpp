




#include "MacIOSurfaceTextureClientOGL.h"
#include "mozilla/gfx/MacIOSurface.h"

namespace mozilla {
namespace layers {

MacIOSurfaceTextureClientOGL::MacIOSurfaceTextureClientOGL(TextureFlags aFlags)
  : TextureClient(aFlags)
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

TextureClientData*
MacIOSurfaceTextureClientOGL::DropTextureData()
{
  
  
  
  mSurface = nullptr;
  MarkInvalid();
  return nullptr;
}

}
}
