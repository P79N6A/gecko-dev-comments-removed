




#include "X11TextureHost.h"
#include "mozilla/layers/BasicCompositor.h"
#include "mozilla/layers/X11TextureSourceBasic.h"
#ifdef GL_PROVIDER_GLX
#include "mozilla/layers/CompositorOGL.h"
#include "mozilla/layers/X11TextureSourceOGL.h"
#endif
#include "gfxXlibSurface.h"
#include "gfx2DGlue.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

X11TextureHost::X11TextureHost(TextureFlags aFlags,
                               const SurfaceDescriptorX11& aDescriptor)
 : TextureHost(aFlags)
{
  nsRefPtr<gfxXlibSurface> surface = aDescriptor.OpenForeign();
  mSurface = surface.get();

  if (!(aFlags & TextureFlags::DEALLOCATE_CLIENT)) {
    mSurface->TakePixmap();
  }
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
          new X11TextureSourceBasic(static_cast<BasicCompositor*>(mCompositor.get()),
                                    mSurface);
        break;
#ifdef GL_PROVIDER_GLX
      case LayersBackend::LAYERS_OPENGL:
        mTextureSource =
          new X11TextureSourceOGL(static_cast<CompositorOGL*>(mCompositor.get()),
                                  mSurface);
        break;
#endif
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
  gfxContentType type = mSurface->GetContentType();
#ifdef GL_PROVIDER_GLX
  if (mCompositor->GetBackendType() == LayersBackend::LAYERS_OPENGL) {
    return X11TextureSourceOGL::ContentTypeToSurfaceFormat(type);
  }
#endif
  return X11TextureSourceBasic::ContentTypeToSurfaceFormat(type);
}

IntSize
X11TextureHost::GetSize() const
{
  return mSurface->GetSize();
}

already_AddRefed<gfx::DataSourceSurface>
X11TextureHost::GetAsSurface()
{
  if (!mTextureSource || !mTextureSource->AsSourceBasic()) {
    return nullptr;
  }
  RefPtr<DrawTarget> tempDT =
    gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
      GetSize(), GetFormat());
  if (!tempDT) {
    return nullptr;
  }
  RefPtr<SourceSurface> surf = mTextureSource->AsSourceBasic()->GetSurface(tempDT);
  if (!surf) {
    return nullptr;
  }
  return surf->GetDataSurface();
}

}
}
