




































#include "ThebesLayerOGL.h"
#include "ContainerLayerOGL.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#endif

namespace mozilla {
namespace layers {






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
{
  mImplData = static_cast<LayerOGL*>(this);
}

ThebesLayerOGL::~ThebesLayerOGL()
{
  static_cast<LayerManagerOGL*>(mManager)->MakeCurrent();
  if (mTexture) {
    gl()->fDeleteTextures(1, &mTexture);
  }
}

void
ThebesLayerOGL::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.GetBounds() == mVisibleRect) {
    return;
  }
  mVisibleRect = aRegion.GetBounds();

  static_cast<LayerManagerOGL*>(mManager)->MakeCurrent();

  if (!mTexture) {
    gl()->fGenTextures(1, &mTexture);
  }

  mInvalidatedRect = mVisibleRect;

  gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  gl()->fTexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

  gl()->fTexImage2D(LOCAL_GL_TEXTURE_2D,
                  0,
                  LOCAL_GL_RGBA,
                  mVisibleRect.width,
                  mVisibleRect.height,
                  0,
                  LOCAL_GL_BGRA,
                  LOCAL_GL_UNSIGNED_BYTE,
                  NULL);
}

void
ThebesLayerOGL::InvalidateRegion(const nsIntRegion &aRegion)
{
  nsIntRegion invalidatedRegion;
  invalidatedRegion.Or(aRegion, mInvalidatedRect);
  invalidatedRegion.And(invalidatedRegion, mVisibleRect);
  mInvalidatedRect = invalidatedRegion.GetBounds();
}

LayerOGL::LayerType
ThebesLayerOGL::GetType()
{
  return TYPE_THEBES;
}

const nsIntRect&
ThebesLayerOGL::GetVisibleRect()
{
  return mVisibleRect;
}

void
ThebesLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                            DrawThebesLayerCallback aCallback,
                            void* aCallbackData)
{
  if (!mTexture) {
    return;
  }

  if (!mInvalidatedRect.IsEmpty()) {
    gfxASurface::gfxImageFormat imageFormat;

    if (UseOpaqueSurface(this)) {
      imageFormat = gfxASurface::ImageFormatRGB24;
    } else {
      imageFormat = gfxASurface::ImageFormatARGB32;
    }

    nsRefPtr<gfxASurface> surface =
      gfxPlatform::GetPlatform()->
        CreateOffscreenSurface(gfxIntSize(mInvalidatedRect.width,
                                          mInvalidatedRect.height),
                               imageFormat);

    nsRefPtr<gfxContext> ctx = new gfxContext(surface);
    ctx->Translate(gfxPoint(-mInvalidatedRect.x, -mInvalidatedRect.y));
    aCallback(this, ctx, mInvalidatedRect, aCallbackData);

    static_cast<LayerManagerOGL*>(mManager)->MakeCurrent();

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
                         mInvalidatedRect.x - mVisibleRect.x,
                         mInvalidatedRect.y - mVisibleRect.y,
                         mInvalidatedRect.width,
                         mInvalidatedRect.height,
                         LOCAL_GL_BGRA,
                         LOCAL_GL_UNSIGNED_BYTE,
                         imageSurface->Data());
  }

  float quadTransform[4][4];
  



  memset(&quadTransform, 0, sizeof(quadTransform));
  quadTransform[0][0] = (float)GetVisibleRect().width;
  quadTransform[1][1] = (float)GetVisibleRect().height;
  quadTransform[2][2] = 1.0f;
  quadTransform[3][0] = (float)GetVisibleRect().x;
  quadTransform[3][1] = (float)GetVisibleRect().y;
  quadTransform[3][3] = 1.0f;

  RGBLayerProgram *program = 
    static_cast<LayerManagerOGL*>(mManager)->GetRGBLayerProgram();

  program->Activate();
  program->SetLayerQuadTransform(&quadTransform[0][0]);
  program->SetLayerOpacity(GetOpacity());
  program->SetLayerTransform(&mTransform._11);
  program->Apply();

  gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  gl()->fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);
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
