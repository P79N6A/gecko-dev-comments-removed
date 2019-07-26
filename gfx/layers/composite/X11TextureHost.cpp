




#include "X11TextureHost.h"
#include "mozilla/layers/BasicCompositor.h"
#include "mozilla/layers/X11TextureSourceBasic.h"
#include "gfxXlibSurface.h"
#include "gfx2DGlue.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::gfx;

X11TextureHost::X11TextureHost(TextureFlags aFlags,
                               const SurfaceDescriptorX11& aDescriptor)
 : TextureHost(aFlags)
{
  nsRefPtr<gfxXlibSurface> surface = aDescriptor.OpenForeign();
  mSurface = surface.get();

  
  MOZ_ASSERT(!(aFlags & TEXTURE_DEALLOCATE_CLIENT));
  mSurface->TakePixmap();
}

bool
X11TextureHost::Lock()
{
  if (!mCompositor) {
    return false;
  }

  if (!mTextureSource) {
    switch (mCompositor->GetBackendType()) {
      case LayersBackend::LAYERS_BASIC:
        mTextureSource =
          new X11TextureSourceBasic(static_cast<BasicCompositor*>(mCompositor),
                                    mSurface);
        break;
      default:
        return false;
    }
  }

  return true;
}

void
X11TextureHost::SetCompositor(Compositor* aCompositor)
{
  mCompositor = aCompositor;
  if (mTextureSource) {
    mTextureSource->SetCompositor(aCompositor);
  }
}

SurfaceFormat
X11TextureHost::GetFormat() const
{
  return X11TextureSourceBasic::ContentTypeToSurfaceFormat(mSurface->GetContentType());
}

IntSize
X11TextureHost::GetSize() const
{
  return ToIntSize(mSurface->GetSize());
}
