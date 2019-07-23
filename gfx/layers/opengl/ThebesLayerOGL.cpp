




































#include "ThebesLayerOGL.h"
#include "ContainerLayerOGL.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#endif

#include "glWrapper.h"

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


ThebesLayerOGL::ThebesLayerOGL(LayerManager *aManager)
  : ThebesLayer(aManager, NULL)
  , mTexture(0)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ThebesLayerOGL::~ThebesLayerOGL()
{
  static_cast<LayerManagerOGL*>(mManager)->MakeCurrent();
  if (mTexture) {
    sglWrapper.DeleteTextures(1, &mTexture);
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
    sglWrapper.GenTextures(1, &mTexture);
  }

  mInvalidatedRect = mVisibleRect;

  sglWrapper.BindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  sglWrapper.TexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MIN_FILTER, LOCAL_GL_LINEAR);
  sglWrapper.TexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_MAG_FILTER, LOCAL_GL_LINEAR);
  sglWrapper.TexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_S, LOCAL_GL_CLAMP_TO_EDGE);
  sglWrapper.TexParameteri(LOCAL_GL_TEXTURE_2D, LOCAL_GL_TEXTURE_WRAP_T, LOCAL_GL_CLAMP_TO_EDGE);

  sglWrapper.TexImage2D(LOCAL_GL_TEXTURE_2D,
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

gfxContext *
ThebesLayerOGL::BeginDrawing(nsIntRegion *aRegion)
{
  if (mInvalidatedRect.IsEmpty()) {
    aRegion->SetEmpty();
    return NULL;
  }
  if (!mTexture) {
    aRegion->SetEmpty();
    return NULL;
  }
  *aRegion = mInvalidatedRect;

  gfxASurface::gfxImageFormat imageFormat;

  if (UseOpaqueSurface(this)) {
    imageFormat = gfxASurface::ImageFormatRGB24;
  } else {
    imageFormat = gfxASurface::ImageFormatARGB32;
  }

  mDestinationSurface =
    gfxPlatform::GetPlatform()->
      CreateOffscreenSurface(gfxIntSize(mInvalidatedRect.width,
                                        mInvalidatedRect.height),
                             imageFormat);

  mContext = new gfxContext(mDestinationSurface);
  mContext->Translate(gfxPoint(-mInvalidatedRect.x, -mInvalidatedRect.y));
  return mContext.get();
}

void
ThebesLayerOGL::EndDrawing()
{
  static_cast<LayerManagerOGL*>(mManager)->MakeCurrent();

  nsRefPtr<gfxImageSurface> imageSurface;

  gfxASurface::gfxImageFormat imageFormat;

  if (UseOpaqueSurface(this)) {
    imageFormat = gfxASurface::ImageFormatRGB24;
  } else {
    imageFormat = gfxASurface::ImageFormatARGB32;
  }

  switch (mDestinationSurface->GetType()) {
    case gfxASurface::SurfaceTypeImage:
      imageSurface = static_cast<gfxImageSurface*>(mDestinationSurface.get());
      break;
#ifdef XP_WIN
    case gfxASurface::SurfaceTypeWin32:
      imageSurface =
        static_cast<gfxWindowsSurface*>(mDestinationSurface.get())->
          GetImageSurface();
      break;
#endif
    default:
      



      {
        imageSurface = new gfxImageSurface(gfxIntSize(mInvalidatedRect.width,
                                                      mInvalidatedRect.height),
                                           imageFormat);
        nsRefPtr<gfxContext> tmpContext = new gfxContext(imageSurface);
        tmpContext->SetSource(mDestinationSurface);
        tmpContext->SetOperator(gfxContext::OPERATOR_SOURCE);
        tmpContext->Paint();
      }
      break;
  }

  sglWrapper.BindTexture(LOCAL_GL_TEXTURE_2D, mTexture);
  sglWrapper.TexSubImage2D(LOCAL_GL_TEXTURE_2D,
                           0,
                           mInvalidatedRect.x - mVisibleRect.x,
                           mInvalidatedRect.y - mVisibleRect.y,
                           mInvalidatedRect.width,
                           mInvalidatedRect.height,
                           LOCAL_GL_BGRA,
                           LOCAL_GL_UNSIGNED_BYTE,
                           imageSurface->Data());

  mDestinationSurface = NULL;
  mContext = NULL;
}

void
ThebesLayerOGL::CopyFrom(ThebesLayer* aSource,
                           const nsIntRegion& aRegion,
                           const nsIntPoint& aDelta)
{
  
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
ThebesLayerOGL::RenderLayer(int aPreviousFrameBuffer)
{
  if (!mTexture) {
    return;
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

  sglWrapper.BindTexture(LOCAL_GL_TEXTURE_2D, mTexture);

  sglWrapper.DrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);
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
