




#include "MacIOSurfaceTextureHostOGL.h"
#include "mozilla/gfx/MacIOSurface.h"
#include "mozilla/layers/CompositorOGL.h"
#include "GLContextCGL.h"

namespace mozilla {
namespace layers {

MacIOSurfaceTextureSourceOGL::MacIOSurfaceTextureSourceOGL(
                                CompositorOGL* aCompositor,
                                MacIOSurface* aSurface)
  : mCompositor(aCompositor)
  , mSurface(aSurface)
{}

MacIOSurfaceTextureSourceOGL::~MacIOSurfaceTextureSourceOGL()
{}

gfx::IntSize
MacIOSurfaceTextureSourceOGL::GetSize() const
{
  return gfx::IntSize(mSurface->GetDevicePixelWidth(),
                      mSurface->GetDevicePixelHeight());
}

gfx::SurfaceFormat
MacIOSurfaceTextureSourceOGL::GetFormat() const
{
  return mSurface->HasAlpha() ? gfx::FORMAT_R8G8B8A8 : gfx::FORMAT_B8G8R8X8;
}

MacIOSurfaceTextureHostOGL::MacIOSurfaceTextureHostOGL(TextureFlags aFlags,
                                                       const SurfaceDescriptorMacIOSurface& aDescriptor)
  : TextureHost(aFlags)
{
  mSurface = MacIOSurface::LookupSurface(aDescriptor.surface(),
                                         aDescriptor.scaleFactor(),
                                         aDescriptor.hasAlpha());
}

bool
MacIOSurfaceTextureHostOGL::Lock()
{
  if (!mCompositor) {
    return false;
  }

  if (!mTextureSource) {
    mTextureSource = new MacIOSurfaceTextureSourceOGL(mCompositor, mSurface);
  }
  return true;
}

void
MacIOSurfaceTextureHostOGL::SetCompositor(Compositor* aCompositor)
{
  CompositorOGL* glCompositor = static_cast<CompositorOGL*>(aCompositor);
  mCompositor = glCompositor;
  if (mTextureSource) {
    mTextureSource->SetCompositor(glCompositor);
  }
}

void
MacIOSurfaceTextureSourceOGL::BindTexture(GLenum aTextureUnit)
{
  if (!gl()) {
    NS_WARNING("Trying to bind a texture without a GLContext");
    return;
  }
  GLuint tex = mCompositor->GetTemporaryTexture(aTextureUnit);

  gl()->fActiveTexture(aTextureUnit);
  gl()->fBindTexture(LOCAL_GL_TEXTURE_RECTANGLE_ARB, tex);
  mSurface->CGLTexImageIOSurface2D(gl::GLContextCGL::Cast(gl())->GetCGLContext());
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
}

void
MacIOSurfaceTextureSourceOGL::SetCompositor(Compositor* aCompositor)
{
  mCompositor = static_cast<CompositorOGL*>(aCompositor);
}

gl::GLContext*
MacIOSurfaceTextureSourceOGL::gl() const
{
  return mCompositor ? mCompositor->gl() : nullptr;
}

gfx::SurfaceFormat
MacIOSurfaceTextureHostOGL::GetFormat() const {
  return mSurface->HasAlpha() ? gfx::FORMAT_R8G8B8A8 : gfx::FORMAT_B8G8R8X8;
}

gfx::IntSize
MacIOSurfaceTextureHostOGL::GetSize() const {
  return gfx::IntSize(mSurface->GetDevicePixelWidth(),
                      mSurface->GetDevicePixelHeight());
}

}
}
