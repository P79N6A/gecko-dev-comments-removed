





































#ifdef MOZ_IPC
# include "mozilla/layers/PLayers.h"
# include "mozilla/layers/ShadowLayers.h"
#endif

#include "ThebesLayerBuffer.h"
#include "ThebesLayerOGL.h"

namespace mozilla {
namespace layers {

using gl::GLContext;
using gl::TextureImage;





static already_AddRefed<TextureImage>
CreateClampOrRepeatTextureImage(GLContext *aGl,
                                const nsIntSize& aSize,
                                TextureImage::ContentType aContentType)
{
  GLenum wrapMode = LOCAL_GL_CLAMP_TO_EDGE;
  if (aGl->IsExtensionSupported(GLContext::ARB_texture_non_power_of_two) ||
      aGl->IsExtensionSupported(GLContext::OES_texture_npot))
  {
    wrapMode = LOCAL_GL_REPEAT;
  }

  return aGl->CreateTextureImage(aSize, aContentType, wrapMode);
}








static void
BindAndDrawQuadWithTextureRect(GLContext* aGl,
                               LayerProgram *aProg,
                               const nsIntRect& aTexCoordRect,
                               const nsIntSize& aTexSize,
                               GLenum aWrapMode)
{
  GLuint vertAttribIndex =
    aProg->AttribLocation(LayerProgram::VertexAttrib);
  GLuint texCoordAttribIndex =
    aProg->AttribLocation(LayerProgram::TexCoordAttrib);
  NS_ASSERTION(texCoordAttribIndex != GLuint(-1), "no texture coords?");

  
  
  aGl->fBindBuffer(LOCAL_GL_ARRAY_BUFFER, 0);

  
  
  
  
  
  

  GLContext::RectTriangles rects;

  if (aWrapMode == LOCAL_GL_REPEAT) {
    rects.addRect(
                  0.0f, 0.0f, 1.0f, 1.0f,
                  
                  aTexCoordRect.x / GLfloat(aTexSize.width),
                  aTexCoordRect.y / GLfloat(aTexSize.height),
                  aTexCoordRect.XMost() / GLfloat(aTexSize.width),
                  aTexCoordRect.YMost() / GLfloat(aTexSize.height));
  } else {
    GLContext::DecomposeIntoNoRepeatTriangles(aTexCoordRect, aTexSize, rects);
  }

  aGl->fVertexAttribPointer(vertAttribIndex, 2,
                            LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                            rects.vertexCoords);

  aGl->fVertexAttribPointer(texCoordAttribIndex, 2,
                            LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                            rects.texCoords);

  DEBUG_GL_ERROR_CHECK(aGl);

  {
    aGl->fEnableVertexAttribArray(texCoordAttribIndex);
    {
      aGl->fEnableVertexAttribArray(vertAttribIndex);

      aGl->fDrawArrays(LOCAL_GL_TRIANGLES, 0, rects.numRects * 6);
      DEBUG_GL_ERROR_CHECK(aGl);

      aGl->fDisableVertexAttribArray(vertAttribIndex);
    }
    aGl->fDisableVertexAttribArray(texCoordAttribIndex);
  }

  DEBUG_GL_ERROR_CHECK(aGl);
}


class ThebesLayerBufferOGL
{
  NS_INLINE_DECL_REFCOUNTING(ThebesLayerBufferOGL)
public:
  typedef TextureImage::ContentType ContentType;
  typedef ThebesLayerBuffer::PaintState PaintState;

  ThebesLayerBufferOGL(ThebesLayer* aLayer, LayerOGL* aOGLLayer)
    : mLayer(aLayer)
    , mOGLLayer(aOGLLayer)
  {}
  virtual ~ThebesLayerBufferOGL() {}

  virtual PaintState BeginPaint(ContentType aContentType) = 0;

  void RenderTo(const nsIntPoint& aOffset, LayerManagerOGL* aManager);

  nsIntSize GetSize() {
    if (mTexImage)
      return mTexImage->GetSize();
    return nsIntSize(0, 0);
  }

protected:
  virtual nsIntPoint GetOriginOffset() = 0;

  GLContext* gl() const { return mOGLLayer->gl(); }

  ThebesLayer* mLayer;
  LayerOGL* mOGLLayer;
  nsRefPtr<TextureImage> mTexImage;
};

void
ThebesLayerBufferOGL::RenderTo(const nsIntPoint& aOffset,
                               LayerManagerOGL* aManager)
{
  if (!mTexImage)
    return;

  
  
  ColorTextureLayerProgram *program =
    aManager->GetBasicLayerProgram(mLayer->CanUseOpaqueSurface(),
                                   mTexImage->IsRGB());

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  if (!mTexImage->InUpdate() || !mTexImage->EndUpdate()) {
    gl()->fBindTexture(LOCAL_GL_TEXTURE_2D, mTexImage->Texture());
  }

  float xres = mLayer->GetXResolution();
  float yres = mLayer->GetYResolution();

  nsIntRegionRectIterator iter(mLayer->GetEffectiveVisibleRegion());
  while (const nsIntRect *iterRect = iter.Next()) {
    nsIntRect quadRect = *iterRect;
    program->Activate();
    program->SetLayerQuadRect(quadRect);
    program->SetLayerOpacity(mLayer->GetEffectiveOpacity());
    program->SetLayerTransform(mLayer->GetEffectiveTransform());
    program->SetRenderOffset(aOffset);
    program->SetTextureUnit(0);
    DEBUG_GL_ERROR_CHECK(gl());

    quadRect.MoveBy(-GetOriginOffset());

    
    
    
    
    gfxRect sqr(quadRect.x, quadRect.y, quadRect.width, quadRect.height);
    sqr.Scale(xres, yres);
    sqr.RoundOut();
    nsIntRect scaledQuadRect(sqr.pos.x, sqr.pos.y, sqr.size.width, sqr.size.height);

    BindAndDrawQuadWithTextureRect(gl(), program, scaledQuadRect,
                                   mTexImage->GetSize(),
                                   mTexImage->GetWrapMode());
    DEBUG_GL_ERROR_CHECK(gl());
  }
}





class SurfaceBufferOGL : public ThebesLayerBufferOGL, private ThebesLayerBuffer
{
public:
  typedef ThebesLayerBufferOGL::ContentType ContentType;
  typedef ThebesLayerBufferOGL::PaintState PaintState;

  SurfaceBufferOGL(ThebesLayerOGL* aLayer)
    : ThebesLayerBufferOGL(aLayer, aLayer)
    , ThebesLayerBuffer(SizedToVisibleBounds)
  {
  }
  virtual ~SurfaceBufferOGL() {}

  
  virtual PaintState BeginPaint(ContentType aContentType)
  {
    
    return ThebesLayerBuffer::BeginPaint(mLayer, aContentType, 1.0, 1.0);
  }

  
  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize)
  {
    NS_ASSERTION(gfxASurface::CONTENT_ALPHA != aType,"ThebesBuffer has color");

    mTexImage = CreateClampOrRepeatTextureImage(gl(), aSize, aType);
    return mTexImage ? mTexImage->GetBackingSurface() : nsnull;
  }

protected:
  virtual nsIntPoint GetOriginOffset() {
    return BufferRect().TopLeft() - BufferRotation();
  }
};






class BasicBufferOGL : public ThebesLayerBufferOGL
{
public:
  BasicBufferOGL(ThebesLayerOGL* aLayer)
    : ThebesLayerBufferOGL(aLayer, aLayer)
    , mBufferRect(0,0,0,0)
    , mBufferRotation(0,0)
  {}
  virtual ~BasicBufferOGL() {}

  virtual PaintState BeginPaint(ContentType aContentType);

protected:
  enum XSide {
    LEFT, RIGHT
  };
  enum YSide {
    TOP, BOTTOM
  };
  nsIntRect GetQuadrantRectangle(XSide aXSide, YSide aYSide);

  virtual nsIntPoint GetOriginOffset() {
    return mBufferRect.TopLeft() - mBufferRotation;
  }

private:
  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
};

static void
WrapRotationAxis(PRInt32* aRotationPoint, PRInt32 aSize)
{
  if (*aRotationPoint < 0) {
    *aRotationPoint += aSize;
  } else if (*aRotationPoint >= aSize) {
    *aRotationPoint -= aSize;
  }
}

nsIntRect
BasicBufferOGL::GetQuadrantRectangle(XSide aXSide, YSide aYSide)
{
  
  
  nsIntPoint quadrantTranslation = -mBufferRotation;
  quadrantTranslation.x += aXSide == LEFT ? mBufferRect.width : 0;
  quadrantTranslation.y += aYSide == TOP ? mBufferRect.height : 0;
  return mBufferRect + quadrantTranslation;
}

BasicBufferOGL::PaintState
BasicBufferOGL::BeginPaint(ContentType aContentType)
{
  PaintState result;

  result.mRegionToDraw.Sub(mLayer->GetVisibleRegion(), mLayer->GetValidRegion());

  if (!mTexImage || mTexImage->GetContentType() != aContentType) {
    
    
    
    
    
    
    
    
    
    result.mRegionToDraw = mLayer->GetVisibleRegion();
    result.mRegionToInvalidate = mLayer->GetValidRegion();
    mTexImage = nsnull;
    mBufferRect.SetRect(0, 0, 0, 0);
    mBufferRotation.MoveTo(0, 0);
  }

  if (result.mRegionToDraw.IsEmpty())
    return result;

  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();
  nsIntRect visibleBounds = mLayer->GetVisibleRegion().GetBounds();
  nsRefPtr<TextureImage> destBuffer;
  nsIntRect destBufferRect;

  if (visibleBounds.Size() <= mBufferRect.Size()) {
    
    if (mBufferRect.Contains(visibleBounds)) {
      
      destBufferRect = mBufferRect;
    } else {
      
      
      destBufferRect = nsIntRect(visibleBounds.TopLeft(), mBufferRect.Size());
    }
    nsIntRect keepArea;
    if (keepArea.IntersectRect(destBufferRect, mBufferRect)) {
      
      
      
      nsIntPoint newRotation = mBufferRotation +
        (destBufferRect.TopLeft() - mBufferRect.TopLeft());
      WrapRotationAxis(&newRotation.x, mBufferRect.width);
      WrapRotationAxis(&newRotation.y, mBufferRect.height);
      NS_ASSERTION(nsIntRect(nsIntPoint(0,0), mBufferRect.Size()).Contains(newRotation),
                   "newRotation out of bounds");
      PRInt32 xBoundary = destBufferRect.XMost() - newRotation.x;
      PRInt32 yBoundary = destBufferRect.YMost() - newRotation.y;
      if ((drawBounds.x < xBoundary && xBoundary < drawBounds.XMost()) ||
          (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost())) {
        
        
        
        
        
        
        destBufferRect = visibleBounds;
        destBuffer = CreateClampOrRepeatTextureImage(gl(), visibleBounds.Size(), aContentType);
        DEBUG_GL_ERROR_CHECK(gl());
        if (!destBuffer)
          return result;
      } else {
        mBufferRect = destBufferRect;
        mBufferRotation = newRotation;
      }
    } else {
      
      
      
      mBufferRect = destBufferRect;
      mBufferRotation = nsIntPoint(0,0);
    }
  } else {
    
    destBufferRect = visibleBounds;
    destBuffer = CreateClampOrRepeatTextureImage(gl(), visibleBounds.Size(), aContentType);
    DEBUG_GL_ERROR_CHECK(gl());
    if (!destBuffer)
      return result;
  }

  if (!destBuffer && !mTexImage) {
    return result;
  }

  if (destBuffer) {
    if (mTexImage) {
      
      
      if (mOGLLayer->OGLManager()->FBOTextureTarget() == LOCAL_GL_TEXTURE_2D) {
        nsIntRect overlap;
        overlap.IntersectRect(mBufferRect, destBufferRect);

        nsIntRect srcRect(overlap), dstRect(overlap);
        srcRect.MoveBy(- mBufferRect.TopLeft() + mBufferRotation);
        dstRect.MoveBy(- destBufferRect.TopLeft());

        destBuffer->Resize(destBufferRect.Size());

        gl()->BlitTextureImage(mTexImage, srcRect,
                               destBuffer, dstRect);
      } else {
        
        destBufferRect = visibleBounds;
        destBuffer = CreateClampOrRepeatTextureImage(gl(), visibleBounds.Size(), aContentType);
      }
    }

    mTexImage = destBuffer.forget();
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  }

  nsIntRegion invalidate;
  invalidate.Sub(mLayer->GetValidRegion(), destBufferRect);
  result.mRegionToInvalidate.Or(result.mRegionToInvalidate, invalidate);

  
  PRInt32 xBoundary = mBufferRect.XMost() - mBufferRotation.x;
  PRInt32 yBoundary = mBufferRect.YMost() - mBufferRotation.y;
  XSide sideX = drawBounds.XMost() <= xBoundary ? RIGHT : LEFT;
  YSide sideY = drawBounds.YMost() <= yBoundary ? BOTTOM : TOP;
  nsIntRect quadrantRect = GetQuadrantRectangle(sideX, sideY);
  NS_ASSERTION(quadrantRect.Contains(drawBounds), "Messed up quadrants");

  nsIntPoint offset = -nsIntPoint(quadrantRect.x, quadrantRect.y);
  
  
  
  result.mRegionToDraw.MoveBy(offset);
  
  
  result.mContext = mTexImage->BeginUpdate(result.mRegionToDraw);
  result.mContext->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));
  if (!result.mContext) {
    NS_WARNING("unable to get context for update");
    return result;
  }
  
  
  result.mRegionToDraw.MoveBy(-offset);
  
  return result;
}

ThebesLayerOGL::ThebesLayerOGL(LayerManagerOGL *aManager)
  : ThebesLayer(aManager, nsnull)
  , LayerOGL(aManager)
  , mBuffer(nsnull)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ThebesLayerOGL::~ThebesLayerOGL()
{
  Destroy();
}

void
ThebesLayerOGL::Destroy()
{
  if (!mDestroyed) {
    mBuffer = nsnull;
    DEBUG_GL_ERROR_CHECK(gl());

    mDestroyed = PR_TRUE;
  }
}

PRBool
ThebesLayerOGL::CreateSurface()
{
  NS_ASSERTION(!mBuffer, "buffer already created?");

  if (mVisibleRegion.IsEmpty()) {
    return PR_FALSE;
  }

  if (gl()->TextureImageSupportsGetBackingSurface()) {
    
    mBuffer = new SurfaceBufferOGL(this);
  } else {
    mBuffer = new BasicBufferOGL(this);
  }
  return PR_TRUE;
}

void
ThebesLayerOGL::SetVisibleRegion(const nsIntRegion &aRegion)
{
  if (aRegion.IsEqual(mVisibleRegion))
    return;
  ThebesLayer::SetVisibleRegion(aRegion);
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
  if (!mBuffer && !CreateSurface()) {
    return;
  }
  NS_ABORT_IF_FALSE(mBuffer, "should have a buffer here");

  mOGLManager->MakeCurrent();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  TextureImage::ContentType contentType =
    CanUseOpaqueSurface() ? gfxASurface::CONTENT_COLOR :
                            gfxASurface::CONTENT_COLOR_ALPHA;
  Buffer::PaintState state = mBuffer->BeginPaint(contentType);
  mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

  if (state.mContext) {
    state.mRegionToInvalidate.And(state.mRegionToInvalidate, mVisibleRegion);

    LayerManager::DrawThebesLayerCallback callback =
      mOGLManager->GetThebesLayerCallback();
    void* callbackData = mOGLManager->GetThebesLayerCallbackData();
    callback(this, state.mContext, state.mRegionToDraw,
             state.mRegionToInvalidate, callbackData);
    mValidRegion.Or(mValidRegion, state.mRegionToDraw);
  }

  DEBUG_GL_ERROR_CHECK(gl());

  gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
  mBuffer->RenderTo(aOffset, mOGLManager);
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
  return !mBuffer;
}


#ifdef MOZ_IPC

class ShadowBufferOGL : public ThebesLayerBufferOGL
{
public:
  ShadowBufferOGL(ShadowThebesLayerOGL* aLayer)
    : ThebesLayerBufferOGL(aLayer, aLayer)
  {}

  virtual PaintState BeginPaint(ContentType aContentType) {
    NS_RUNTIMEABORT("can't BeginPaint for a shadow layer");
    return PaintState();
  }

  void
  CreateTexture(ContentType aType, const nsIntSize& aSize)
  {
    NS_ASSERTION(gfxASurface::CONTENT_ALPHA != aType,"ThebesBuffer has color");

    mTexImage = CreateClampOrRepeatTextureImage(gl(), aSize, aType);
  }

  void Upload(gfxASurface* aUpdate, const nsIntRegion& aUpdated,
              const nsIntRect& aRect, const nsIntPoint& aRotation);

protected:
  virtual nsIntPoint GetOriginOffset() {
    return mBufferRect.TopLeft() - mBufferRotation;
  }

private:
  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
};

void
ShadowBufferOGL::Upload(gfxASurface* aUpdate, const nsIntRegion& aUpdated,
                        const nsIntRect& aRect, const nsIntPoint& aRotation)
{
  nsIntRegion destRegion(aUpdated);
  
  
  nsIntPoint visTopLeft = mLayer->GetVisibleRegion().GetBounds().TopLeft();
  destRegion.MoveBy(-visTopLeft);
  
  nsRefPtr<gfxContext> dest = mTexImage->BeginUpdate(destRegion);

  dest->SetOperator(gfxContext::OPERATOR_SOURCE);
  dest->DrawSurface(aUpdate, aUpdate->GetSize());

  mTexImage->EndUpdate();

  mBufferRect = aRect;
  mBufferRotation = aRotation;
}

ShadowThebesLayerOGL::ShadowThebesLayerOGL(LayerManagerOGL *aManager)
  : ShadowThebesLayer(aManager, nsnull)
  , LayerOGL(aManager)
{
  mImplData = static_cast<LayerOGL*>(this);
}

ShadowThebesLayerOGL::~ShadowThebesLayerOGL()
{}

void
ShadowThebesLayerOGL::SetFrontBuffer(const ThebesBuffer& aNewFront,
                                     const nsIntRegion& aValidRegion,
                                     float aXResolution, float aYResolution)
{
  if (mDestroyed) {
    return;
  }

  if (!mBuffer) {
    mBuffer = new ShadowBufferOGL(this);
  }

  nsRefPtr<gfxASurface> surf = ShadowLayerForwarder::OpenDescriptor(aNewFront.buffer());
  gfxIntSize size = surf->GetSize();
  mBuffer->CreateTexture(surf->GetContentType(),
                         nsIntSize(size.width, size.height));



  mDeadweight = aNewFront.buffer();
}

void
ShadowThebesLayerOGL::Swap(const ThebesBuffer& aNewFront,
                           const nsIntRegion& aUpdatedRegion,
                           ThebesBuffer* aNewBack,
                           nsIntRegion* aNewBackValidRegion,
                           float* aNewXResolution, float* aNewYResolution,
                           OptionalThebesBuffer* aReadOnlyFront,
                           nsIntRegion* aFrontUpdatedRegion)
{
  if (!mDestroyed && mBuffer) {
    nsRefPtr<gfxASurface> surf = ShadowLayerForwarder::OpenDescriptor(aNewFront.buffer());
    mBuffer->Upload(surf, aUpdatedRegion, aNewFront.rect(), aNewFront.rotation());
  }

  *aNewBack = aNewFront;
  *aNewBackValidRegion = mValidRegion;
  *aNewXResolution = mXResolution;
  *aNewYResolution = mYResolution;
  *aReadOnlyFront = null_t();
  aFrontUpdatedRegion->SetEmpty();
}

void
ShadowThebesLayerOGL::DestroyFrontBuffer()
{
  mBuffer = nsnull;
  if (SurfaceDescriptor::T__None != mDeadweight.type()) {
    mOGLManager->DestroySharedSurface(&mDeadweight, mAllocator);
  }
}

void
ShadowThebesLayerOGL::Destroy()
{
  if (!mDestroyed) {
    mDestroyed = PR_TRUE;
    mBuffer = nsnull;
  }
}

Layer*
ShadowThebesLayerOGL::GetLayer()
{
  return this;
}

PRBool
ShadowThebesLayerOGL::IsEmpty()
{
  return !mBuffer;
}

void
ShadowThebesLayerOGL::RenderLayer(int aPreviousFrameBuffer,
                                  const nsIntPoint& aOffset)
{
  if (!mBuffer) {
    return;
  }
  NS_ABORT_IF_FALSE(mBuffer, "should have a buffer here");

  mOGLManager->MakeCurrent();
  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);
  DEBUG_GL_ERROR_CHECK(gl());

  gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
  mBuffer->RenderTo(aOffset, mOGLManager);
  DEBUG_GL_ERROR_CHECK(gl());
}

#endif  


} 
} 
