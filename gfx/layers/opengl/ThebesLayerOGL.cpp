





































#include "ThebesLayerOGL.h"
#include "ContainerLayerOGL.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#endif
#include "GLContextProvider.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gl;






static PRBool
UseOpaqueSurface(Layer* aLayer)
{
  
  
  if (aLayer->IsOpaqueContent())
    return PR_TRUE;
  
  
  
  
  ContainerLayerOGL* parent =
    static_cast<ContainerLayerOGL*>(aLayer->GetParent());
  return parent && parent->GetFirstChild() == aLayer &&
         UseOpaqueSurface(parent);
}


ThebesLayerOGL::ThebesLayerOGL(LayerManagerOGL *aManager)
  : ThebesLayer(aManager, NULL)
  , LayerOGL(aManager)
  , mTexture(0)
  , mOffscreenFormat(gfxASurface::ImageFormatUnknown)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ThebesLayerOGL::~ThebesLayerOGL()
{
  mOGLManager->MakeCurrent();
  if (mOffscreenSurfaceAsGLContext)
    mOffscreenSurfaceAsGLContext->ReleaseTexImage();
  if (mTexture) {
    gl()->fDeleteTextures(1, &mTexture);
  }
}

PRBool
ThebesLayerOGL::EnsureSurface()
{
  gfxASurface::gfxImageFormat imageFormat = gfxASurface::ImageFormatARGB32;
  if (UseOpaqueSurface(this))
    imageFormat = gfxASurface::ImageFormatRGB24;

  if (mInvalidatedRect.IsEmpty())
    return mOffScreenSurface ? PR_TRUE : PR_FALSE;

  if ((mOffscreenSize == gfxIntSize(mInvalidatedRect.width, mInvalidatedRect.height)
      && imageFormat == mOffscreenFormat)
      || mInvalidatedRect.IsEmpty())
    return mOffScreenSurface ? PR_TRUE : PR_FALSE;

  mOffScreenSurface =
    gfxPlatform::GetPlatform()->
      CreateOffscreenSurface(gfxIntSize(mInvalidatedRect.width, mInvalidatedRect.height),
                             imageFormat);
  if (!mOffScreenSurface)
    return PR_FALSE;

  if (mOffScreenSurface) {
    mOffscreenSize.width = mInvalidatedRect.width;
    mOffscreenSize.height = mInvalidatedRect.height;
    mOffscreenFormat = imageFormat;
  }


  mOGLManager->MakeCurrent();

  if (!mTexture)
    gl()->fGenTextures(1, &mTexture);

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

  
  mOffscreenSurfaceAsGLContext = sGLContextProvider.CreateForNativePixmapSurface(mOffScreenSurface);
  
  if (mOffscreenSurfaceAsGLContext)
    return mOffscreenSurfaceAsGLContext->BindTexImage();

  
  gl()->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                    0,
                    LOCAL_GL_RGBA,
                    mInvalidatedRect.width,
                    mInvalidatedRect.height,
                    0,
                    LOCAL_GL_RGBA,
                    LOCAL_GL_UNSIGNED_BYTE,
                    NULL);

  DEBUG_GL_ERROR_CHECK(gl());
  return PR_TRUE;
}

void
ThebesLayerOGL::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.IsEqual(mVisibleRegion))
    return;

  ThebesLayer::SetVisibleRegion(aRegion);

  mInvalidatedRect = mVisibleRegion.GetBounds();
}

void
ThebesLayerOGL::InvalidateRegion(const nsIntRegion &aRegion)
{
  nsIntRegion invalidatedRegion;
  invalidatedRegion.Or(aRegion, mInvalidatedRect);
  invalidatedRegion.And(invalidatedRegion, mVisibleRegion);
  mInvalidatedRect = invalidatedRegion.GetBounds();
}

LayerOGL::LayerType
ThebesLayerOGL::GetType()
{
  return TYPE_THEBES;
}

void
ThebesLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                            const nsIntPoint& aOffset)
{
  if (!EnsureSurface())
    return;

  if (!mTexture)
    return;

  mOGLManager->MakeCurrent();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  bool needsTextureBind = true;
  nsIntRect visibleRect = mVisibleRegion.GetBounds();

  nsRefPtr<gfxASurface> surface = mOffScreenSurface;
  gfxASurface::gfxImageFormat imageFormat = mOffscreenFormat;

  if (!mInvalidatedRect.IsEmpty()) {
    nsRefPtr<gfxContext> ctx = new gfxContext(surface);
    ctx->Translate(gfxPoint(-mInvalidatedRect.x, -mInvalidatedRect.y));

    
    mOGLManager->CallThebesLayerDrawCallback(this, ctx, mInvalidatedRect);
  }

  
  if (!mInvalidatedRect.IsEmpty() && !mOffscreenSurfaceAsGLContext) {
    

    nsRefPtr<gfxImageSurface> imageSurface;

    switch (surface->GetType()) {
      case gfxASurface::SurfaceTypeImage:
        imageSurface = static_cast<gfxImageSurface*>(surface.get());
        break;
#ifdef XP_WIN
      case gfxASurface::SurfaceTypeWin32:
        imageSurface =
          static_cast<gfxWindowsSurface*>(surface.get())->
            GetImageSurface();
        break;
#endif
      default:
        



        {
          imageSurface = new gfxImageSurface(gfxIntSize(mInvalidatedRect.width,
                                                        mInvalidatedRect.height),
                                             imageFormat);
          nsRefPtr<gfxContext> tmpContext = new gfxContext(imageSurface);
          tmpContext->SetSource(surface);
          tmpContext->SetOperator(gfxContext::OPERATOR_SOURCE);
          tmpContext->Paint();
        }
        break;
    }

    
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
    gl()->fTexSubImage2D(LOCAL_GL_TEXTURE_2D,
                         0,
                         mInvalidatedRect.x - visibleRect.x,
                         mInvalidatedRect.y - visibleRect.y,
                         mInvalidatedRect.width,
                         mInvalidatedRect.height,
                         LOCAL_GL_RGBA,
                         LOCAL_GL_UNSIGNED_BYTE,
                         imageSurface->Data());

    needsTextureBind = false;
  }

  if (needsTextureBind)
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  
  
  ColorTextureLayerProgram *program =
    UseOpaqueSurface(this)
    ? mOGLManager->GetBGRXLayerProgram()
    : mOGLManager->GetBGRALayerProgram();

  program->Activate();
  program->SetLayerQuadRect(visibleRect);
  program->SetLayerOpacity(GetOpacity());
  program->SetLayerTransform(mTransform);
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);

  mOGLManager->BindAndDrawQuad(program);

  DEBUG_GL_ERROR_CHECK(gl());
}

const nsIntRect&
ThebesLayerOGL::GetInvalidatedRect()
{
  return mInvalidatedRect;
}

Layer*
ThebesLayerOGL::GetLayer()
{
  return this;
}

PRBool
ThebesLayerOGL::IsEmpty()
{
  return !mTexture;
}

} 
} 
