




#include "mozilla/layers/TextureClientOGL.h"
#include "GLContext.h"

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

TextureClientSharedOGL::TextureClientSharedOGL(CompositableForwarder* aForwarder,
                                               CompositableType aCompositableType)
  : TextureClient(aForwarder, aCompositableType)
  , mGL(nullptr)
{
}

void
TextureClientSharedOGL::ReleaseResources()
{
  if (!IsSurfaceDescriptorValid(mDescriptor)) {
    return;
  }
  MOZ_ASSERT(mDescriptor.type() == SurfaceDescriptor::TSharedTextureDescriptor);
  SharedTextureDescriptor handle = mDescriptor.get_SharedTextureDescriptor();
  if (mGL && handle.handle()) {
    mGL->ReleaseSharedHandle(handle.shareType(), handle.handle());
  }
}

void
TextureClientSharedOGL::EnsureAllocated(gfx::IntSize aSize,
                                        gfxASurface::gfxContentType aContentType)
{
  mSize = aSize;
}


} 
} 
