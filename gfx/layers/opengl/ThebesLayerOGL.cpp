





































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

ThebesLayerOGL::ThebesLayerOGL(LayerManagerOGL *aManager)
  : ThebesLayer(aManager, nsnull)
  , LayerOGL(aManager)
  , mTexImage(nsnull)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ThebesLayerOGL::~ThebesLayerOGL()
{
  mTexImage = nsnull;
  DEBUG_GL_ERROR_CHECK(gl());
}

PRBool
ThebesLayerOGL::EnsureSurface()
{
  nsIntSize visibleSize = mVisibleRegion.GetBounds().Size();
  TextureImage::ContentType contentType =
    CanUseOpaqueSurface() ? gfxASurface::CONTENT_COLOR :
                            gfxASurface::CONTENT_COLOR_ALPHA;
  if (!mTexImage ||
      mTexImage->GetSize() != visibleSize ||
      mTexImage->GetContentType() != contentType)
  {
    mValidRegion.SetEmpty();
    mTexImage = nsnull;
    DEBUG_GL_ERROR_CHECK(gl());
  }

  if (!mTexImage && !mVisibleRegion.IsEmpty())
  {
    mTexImage = gl()->CreateTextureImage(visibleSize,
                                         contentType,
                                         LOCAL_GL_CLAMP_TO_EDGE);
    DEBUG_GL_ERROR_CHECK(gl());
  }
  return !!mTexImage;
}

void
ThebesLayerOGL::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.IsEqual(mVisibleRegion))
    return;
  ThebesLayer::SetVisibleRegion(aRegion);
  
  mValidRegion.SetEmpty();
}

void
ThebesLayerOGL::InvalidateRegion(const nsIntRegion &aRegion)
{
  mValidRegion.Sub(mValidRegion, aRegion);
}

void
ThebesLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                            const nsIntPoint& aOffset)
{
  if (!EnsureSurface())
    return;

  mOGLManager->MakeCurrent();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  nsIntRegion rgnToPaint = mVisibleRegion;
  rgnToPaint.Sub(rgnToPaint, mValidRegion);
  PRBool textureBound = PR_FALSE;
  if (!rgnToPaint.IsEmpty()) {
    nsIntRect visibleRect = mVisibleRegion.GetBounds();

    
    
    
    
    
    
    rgnToPaint.MoveBy(-visibleRect.TopLeft());

    
    
    nsRefPtr<gfxContext> ctx = mTexImage->BeginUpdate(rgnToPaint);
    if (!ctx) {
      NS_WARNING("unable to get context for update");
      return;
    }

    
    
    rgnToPaint.MoveBy(visibleRect.TopLeft());

    
    
    ctx->Translate(-gfxPoint(visibleRect.x, visibleRect.y));

    TextureImage::ContentType contentType = mTexImage->GetContentType();
    
    if (gfxASurface::CONTENT_COLOR_ALPHA == contentType) {
      ctx->SetOperator(gfxContext::OPERATOR_CLEAR);
      ctx->Paint();
      ctx->SetOperator(gfxContext::OPERATOR_OVER);
    }

    mOGLManager->CallThebesLayerDrawCallback(this, ctx, rgnToPaint);

    textureBound = mTexImage->EndUpdate();
    mValidRegion.Or(mValidRegion, rgnToPaint);
  }

  if (!textureBound)
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexImage->Texture());

  
  
  ColorTextureLayerProgram *program =
    CanUseOpaqueSurface()
    ? mOGLManager->GetBGRXLayerProgram()
    : mOGLManager->GetBGRALayerProgram();

  program->Activate();
  program->SetLayerQuadRect(mVisibleRegion.GetBounds());
  program->SetLayerOpacity(GetOpacity());
  program->SetLayerTransform(mTransform);
  program->SetRenderOffset(aOffset);
  program->SetTextureUnit(0);

  mOGLManager->BindAndDrawQuad(program);

  DEBUG_GL_ERROR_CHECK(gl());
}

Layer*
ThebesLayerOGL::GetLayer()
{
  return this;
}

PRBool
ThebesLayerOGL::IsEmpty()
{
  return !mTexImage;
}

} 
} 
