





































#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "ThebesLayerBuffer.h"
#include "ThebesLayerOGL.h"
#include "gfxUtils.h"
#include "gfxTeeSurface.h"

namespace mozilla {
namespace layers {

using gl::GLContext;
using gl::TextureImage;

static const int ALLOW_REPEAT = ThebesLayerBuffer::ALLOW_REPEAT;







static already_AddRefed<TextureImage>
CreateClampOrRepeatTextureImage(GLContext *aGl,
                                const nsIntSize& aSize,
                                TextureImage::ContentType aContentType,
                                PRUint32 aFlags)
{
  GLenum wrapMode = LOCAL_GL_CLAMP_TO_EDGE;
  if ((aFlags & ALLOW_REPEAT) &&
      (aGl->IsExtensionSupported(GLContext::ARB_texture_non_power_of_two) ||
       aGl->IsExtensionSupported(GLContext::OES_texture_npot)))
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
                            rects.vertexPointer());

  
  aGl->fVertexAttribPointer(texCoordAttribIndex, 2,
                            LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0,
                            rects.texCoordPointer());

  {
    aGl->fEnableVertexAttribArray(texCoordAttribIndex);
    {
      aGl->fEnableVertexAttribArray(vertAttribIndex);

      aGl->fDrawArrays(LOCAL_GL_TRIANGLES, 0, rects.elements());

      aGl->fDisableVertexAttribArray(vertAttribIndex);
    }
    aGl->fDisableVertexAttribArray(texCoordAttribIndex);
  }
}

static void
SetAntialiasingFlags(Layer* aLayer, gfxContext* aTarget)
{
  nsRefPtr<gfxASurface> surface = aTarget->CurrentSurface();
  if (surface->GetContentType() != gfxASurface::CONTENT_COLOR_ALPHA) {
    
    return;
  }

  surface->SetSubpixelAntialiasingEnabled(
      !(aLayer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA));
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

  enum { PAINT_WILL_RESAMPLE = ThebesLayerBuffer::PAINT_WILL_RESAMPLE };
  virtual PaintState BeginPaint(ContentType aContentType,
                                float aXResolution,
                                float aYResolution,
                                PRUint32 aFlags) = 0;

  void RenderTo(const nsIntPoint& aOffset, LayerManagerOGL* aManager,
                PRUint32 aFlags);

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
  nsRefPtr<TextureImage> mTexImageOnWhite;
};

void
ThebesLayerBufferOGL::RenderTo(const nsIntPoint& aOffset,
                               LayerManagerOGL* aManager,
                               PRUint32 aFlags)
{
  if (!mTexImage)
    return;

  if (mTexImage->InUpdate()) {
    mTexImage->EndUpdate();
  }

  if (mTexImageOnWhite && mTexImageOnWhite->InUpdate()) {
    mTexImageOnWhite->EndUpdate();
  }

  
  TextureImage::ScopedBindTexture texBind(mTexImage, LOCAL_GL_TEXTURE0);
  TextureImage::ScopedBindTexture texOnWhiteBind(mTexImageOnWhite, LOCAL_GL_TEXTURE1);

  float xres = mLayer->GetXResolution();
  float yres = mLayer->GetYResolution();

  PRInt32 passes = mTexImageOnWhite ? 2 : 1;
  for (PRInt32 pass = 1; pass <= passes; ++pass) {
    LayerProgram *program;

    if (passes == 2) {
      ComponentAlphaTextureLayerProgram *alphaProgram;
      NS_ASSERTION(!mTexImage->IsRGB() && !mTexImageOnWhite->IsRGB(),
                   "Only BGR image surported with component alpha (currently!)");
      if (pass == 1) {
        alphaProgram = aManager->GetComponentAlphaPass1LayerProgram();
        gl()->fBlendFuncSeparate(LOCAL_GL_ZERO, LOCAL_GL_ONE_MINUS_SRC_COLOR,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
      } else {
        alphaProgram = aManager->GetComponentAlphaPass2LayerProgram();
        gl()->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
      }

      alphaProgram->Activate();
      alphaProgram->SetBlackTextureUnit(0);
      alphaProgram->SetWhiteTextureUnit(1);
      program = alphaProgram;
    } else {
      
      
      ColorTextureLayerProgram *basicProgram =
        aManager->GetColorTextureLayerProgram(mTexImage->GetShaderProgramType());

      basicProgram->Activate();
      basicProgram->SetTextureUnit(0);
      program = basicProgram;
    }

    program->SetLayerOpacity(mLayer->GetEffectiveOpacity());
    program->SetLayerTransform(mLayer->GetEffectiveTransform());
    program->SetRenderOffset(aOffset);

    const nsIntRegion& visibleRegion = mLayer->GetEffectiveVisibleRegion();
    nsIntRegion tmpRegion;
    const nsIntRegion* renderRegion;
    if (aFlags & PAINT_WILL_RESAMPLE) {
      
      
      
      tmpRegion = visibleRegion.GetBounds();
      renderRegion = &tmpRegion;
    } else {
      renderRegion = &visibleRegion;
    }
    nsIntRegionRectIterator iter(*renderRegion);
    while (const nsIntRect *iterRect = iter.Next()) {
      nsIntRect quadRect = *iterRect;
      program->SetLayerQuadRect(quadRect);

      quadRect.MoveBy(-GetOriginOffset());

      
      
      
      
      quadRect.ScaleRoundOut(xres, yres);

      BindAndDrawQuadWithTextureRect(gl(), program, quadRect,
                                     mTexImage->GetSize(),
                                     mTexImage->GetWrapMode());
    }
  }

  if (mTexImageOnWhite) {
    
    gl()->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE_MINUS_SRC_ALPHA,
                             LOCAL_GL_ONE, LOCAL_GL_ONE);
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

  
  virtual PaintState BeginPaint(ContentType aContentType, 
                                float aXResolution, 
                                float aYResolution,
                                PRUint32 aFlags)
  {
    
    return ThebesLayerBuffer::BeginPaint(mLayer, 
                                         aContentType, 
                                         aXResolution, 
                                         aYResolution,
                                         aFlags);
  }

  
  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize, PRUint32 aFlags)
  {
    NS_ASSERTION(gfxASurface::CONTENT_ALPHA != aType,"ThebesBuffer has color");

    mTexImage = CreateClampOrRepeatTextureImage(gl(), aSize, aType, aFlags);
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

  virtual PaintState BeginPaint(ContentType aContentType,
                                float aXResolution,
                                float aYResolution,
                                PRUint32 aFlags);

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

static void
FillSurface(gfxASurface* aSurface, const nsIntRegion& aRegion,
            const nsIntPoint& aOffset, const gfxRGBA& aColor)
{
  nsRefPtr<gfxContext> ctx = new gfxContext(aSurface);
  ctx->Translate(-gfxPoint(aOffset.x, aOffset.y));
  gfxUtils::ClipToRegion(ctx, aRegion);
  ctx->SetColor(aColor);
  ctx->Paint();
}

static nsIntSize
ScaledSize(const nsIntSize& aSize, float aXScale, float aYScale)
{
  if (aXScale == 1.0 && aYScale == 1.0) {
    return aSize;
  }

  nsIntRect rect(0, 0, aSize.width, aSize.height);
  rect.ScaleRoundOut(aXScale, aYScale);
  return rect.Size();
}

BasicBufferOGL::PaintState
BasicBufferOGL::BeginPaint(ContentType aContentType,
                           float aXResolution,
                           float aYResolution,
                           PRUint32 aFlags)
{
  PaintState result;
  float curXRes = mLayer->GetXResolution();
  float curYRes = mLayer->GetYResolution();
  
  
  
  
  
  
  PRBool canHaveRotation =
    !(aFlags & PAINT_WILL_RESAMPLE) && aXResolution == 1.0 && aYResolution == 1.0;

  nsIntRegion validRegion = mLayer->GetValidRegion();

  Layer::SurfaceMode mode;
  ContentType contentType;
  nsIntRegion neededRegion;
  nsIntSize destBufferDims;
  PRBool canReuseBuffer;
  nsIntRect destBufferRect;

  while (PR_TRUE) {
    mode = mLayer->GetSurfaceMode();
    contentType = aContentType;
    neededRegion = mLayer->GetVisibleRegion();
    destBufferDims = ScaledSize(neededRegion.GetBounds().Size(),
                                aXResolution, aYResolution);
    
    canReuseBuffer = neededRegion.GetBounds().Size() <= mBufferRect.Size() &&
      mTexImage &&
      (!(aFlags & PAINT_WILL_RESAMPLE) ||
       mTexImage->GetWrapMode() == LOCAL_GL_CLAMP_TO_EDGE);

    if (canReuseBuffer) {
      if (mBufferRect.Contains(neededRegion.GetBounds())) {
        
        destBufferRect = mBufferRect;
      } else {
        
        
        destBufferRect = nsIntRect(neededRegion.GetBounds().TopLeft(), mBufferRect.Size());
      }
    } else {
      destBufferRect = neededRegion.GetBounds();
    }

    if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
      mode = Layer::SURFACE_SINGLE_CHANNEL_ALPHA;
#else
      if (!mLayer->GetParent() || !mLayer->GetParent()->SupportsComponentAlphaChildren()) {
        mode = Layer::SURFACE_SINGLE_CHANNEL_ALPHA;
      } else {
        contentType = gfxASurface::CONTENT_COLOR;
      }
 #endif
    }
 
    if ((aFlags & PAINT_WILL_RESAMPLE) &&
        (!neededRegion.GetBounds().IsEqualInterior(destBufferRect) ||
         neededRegion.GetNumRects() > 1)) {
      
      if (mode == Layer::SURFACE_OPAQUE) {
        contentType = gfxASurface::CONTENT_COLOR_ALPHA;
        mode = Layer::SURFACE_SINGLE_CHANNEL_ALPHA;
      }
      

      
      
      neededRegion = destBufferRect;
      destBufferDims = ScaledSize(neededRegion.GetBounds().Size(),
                                  aXResolution, aYResolution);
    }

    if (mTexImage &&
        (mTexImage->GetContentType() != contentType ||
         aXResolution != curXRes || aYResolution != curYRes ||
         (mode == Layer::SURFACE_COMPONENT_ALPHA) != (mTexImageOnWhite != nsnull))) {
      
      
      
      
      
      
      
      
      
      result.mRegionToInvalidate = mLayer->GetValidRegion();
      validRegion.SetEmpty();
      mTexImage = nsnull;
      mTexImageOnWhite = nsnull;
      mBufferRect.SetRect(0, 0, 0, 0);
      mBufferRotation.MoveTo(0, 0);
      
      
      continue;
    }

    break;
  }

  result.mRegionToDraw.Sub(neededRegion, validRegion);
  if (result.mRegionToDraw.IsEmpty())
    return result;

  if (destBufferDims.width > gl()->GetMaxTextureSize() ||
      destBufferDims.height > gl()->GetMaxTextureSize()) {
    return result;
  }

  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();
  nsRefPtr<TextureImage> destBuffer;
  nsRefPtr<TextureImage> destBufferOnWhite;

  PRUint32 bufferFlags = canHaveRotation ? ALLOW_REPEAT : 0;
  if (canReuseBuffer) {
    NS_ASSERTION(curXRes == aXResolution && curYRes == aYResolution,
                 "resolution changes must clear the buffer!");

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
          (drawBounds.y < yBoundary && yBoundary < drawBounds.YMost()) ||
          (newRotation != nsIntPoint(0,0) && !canHaveRotation)) {
        
        
        
        
        
        
        destBufferRect = neededRegion.GetBounds();
        destBuffer = CreateClampOrRepeatTextureImage(gl(), destBufferDims, contentType, bufferFlags);
        if (!destBuffer)
          return result;
        if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
          destBufferOnWhite =
            CreateClampOrRepeatTextureImage(gl(), destBufferDims, contentType, bufferFlags);
          if (!destBufferOnWhite)
            return result;
        }
      } else {
        mBufferRect = destBufferRect;
        mBufferRotation = newRotation;
      }
    } else {
      
      
      
      mBufferRect = destBufferRect;
      mBufferRotation = nsIntPoint(0,0);
    }
  } else {
    
    destBuffer = CreateClampOrRepeatTextureImage(gl(), destBufferDims, contentType, bufferFlags);
    if (!destBuffer)
      return result;

    if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
      destBufferOnWhite = 
        CreateClampOrRepeatTextureImage(gl(), destBufferDims, contentType, bufferFlags);
      if (!destBufferOnWhite)
        return result;
    }
  }
  NS_ASSERTION(!(aFlags & PAINT_WILL_RESAMPLE) || destBufferRect == neededRegion.GetBounds(),
               "If we're resampling, we need to validate the entire buffer");

  if (!destBuffer && !mTexImage) {
    return result;
  }

  if (destBuffer) {
    if (mTexImage && (mode != Layer::SURFACE_COMPONENT_ALPHA || mTexImageOnWhite)) {
      
      
      if (mOGLLayer->OGLManager()->FBOTextureTarget() == LOCAL_GL_TEXTURE_2D) {
        nsIntRect overlap;
        overlap.IntersectRect(mBufferRect, destBufferRect);

        nsIntRect srcRect(overlap), dstRect(overlap);
        srcRect.MoveBy(- mBufferRect.TopLeft() + mBufferRotation);
        dstRect.MoveBy(- destBufferRect.TopLeft());

        nsIntSize size = ScaledSize(destBufferRect.Size(), aXResolution, aYResolution);
        destBuffer->Resize(size);
        srcRect.ScaleRoundOut(aXResolution, aYResolution);
        dstRect.ScaleRoundOut(aXResolution, aYResolution);

        gl()->BlitTextureImage(mTexImage, srcRect,
                               destBuffer, dstRect);
        if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
          destBufferOnWhite->Resize(size);
          gl()->BlitTextureImage(mTexImageOnWhite, srcRect,
                                 destBufferOnWhite, dstRect);
        }
      } else {
        
        destBuffer = CreateClampOrRepeatTextureImage(gl(), destBufferDims, contentType, bufferFlags);
        if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
          destBufferOnWhite = 
            CreateClampOrRepeatTextureImage(gl(), destBufferDims, contentType, bufferFlags);
        }
      }
    }

    mTexImage = destBuffer.forget();
    if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
      mTexImageOnWhite = destBufferOnWhite.forget();
    }
    mBufferRect = destBufferRect;
    mBufferRotation = nsIntPoint(0,0);
  }
  NS_ASSERTION(canHaveRotation || mBufferRotation == nsIntPoint(0,0),
               "Rotation disabled, but we have nonzero rotation?");

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
  result.mRegionToDraw.ScaleRoundOut(aXResolution, aYResolution);
  
  
  if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
    nsIntRegion drawRegionCopy = result.mRegionToDraw;
    gfxASurface *onBlack = mTexImage->BeginUpdate(drawRegionCopy);
    gfxASurface *onWhite = mTexImageOnWhite->BeginUpdate(result.mRegionToDraw);
    NS_ASSERTION(result.mRegionToDraw == drawRegionCopy,
                 "BeginUpdate should always modify the draw region in the same way!");
    FillSurface(onBlack, result.mRegionToDraw, nsIntPoint(0,0), gfxRGBA(0.0, 0.0, 0.0, 1.0));
    FillSurface(onWhite, result.mRegionToDraw, nsIntPoint(0,0), gfxRGBA(1.0, 1.0, 1.0, 1.0));
    gfxASurface* surfaces[2] = { onBlack, onWhite };
    nsRefPtr<gfxTeeSurface> surf = new gfxTeeSurface(surfaces, NS_ARRAY_LENGTH(surfaces));

    
    
    gfxPoint deviceOffset = onBlack->GetDeviceOffset();
    onBlack->SetDeviceOffset(gfxPoint(0, 0));
    onWhite->SetDeviceOffset(gfxPoint(0, 0));
    surf->SetDeviceOffset(deviceOffset);

    
    
    
    surf->SetAllowUseAsSource(PR_FALSE);
    result.mContext = new gfxContext(surf);
  } else {
    result.mContext = new gfxContext(mTexImage->BeginUpdate(result.mRegionToDraw));
    if (mTexImage->GetContentType() == gfxASurface::CONTENT_COLOR_ALPHA) {
      gfxUtils::ClipToRegion(result.mContext, result.mRegionToDraw);
      result.mContext->SetOperator(gfxContext::OPERATOR_CLEAR);
      result.mContext->Paint();
      result.mContext->SetOperator(gfxContext::OPERATOR_OVER);
    }
  }
  if (!result.mContext) {
    NS_WARNING("unable to get context for update");
    return result;
  }
  result.mContext->Scale(aXResolution, aYResolution);
  result.mContext->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));
  
  
  result.mRegionToDraw.ScaleRoundOut(1/aXResolution, 1/aYResolution);
  result.mRegionToDraw.MoveBy(-offset);
  
  
  result.mRegionToDraw.ExtendForScaling(aXResolution, aYResolution);
  
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

  gfxMatrix transform2d;
  gfxSize scale(1.0, 1.0);
  float paintXRes = 1.0;
  float paintYRes = 1.0;
  PRUint32 flags = 0;
  if (GetEffectiveTransform().Is2D(&transform2d)) {
    scale = transform2d.ScaleFactors(PR_TRUE);
    paintXRes = gfxUtils::ClampToScaleFactor(scale.width);
    paintYRes = gfxUtils::ClampToScaleFactor(scale.height);
    transform2d.Scale(1.0/paintXRes, 1.0/paintYRes);
    if (transform2d.HasNonIntegerTranslation()) {
      flags |= ThebesLayerBufferOGL::PAINT_WILL_RESAMPLE;
    }
  } else {
    flags |= ThebesLayerBufferOGL::PAINT_WILL_RESAMPLE;
  }

  Buffer::PaintState state =
    mBuffer->BeginPaint(contentType, paintXRes, paintYRes, flags);
  mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

  if (state.mContext) {
    state.mRegionToInvalidate.And(state.mRegionToInvalidate, mVisibleRegion);
    mXResolution = paintXRes;
    mYResolution = paintYRes;

    LayerManager::DrawThebesLayerCallback callback =
      mOGLManager->GetThebesLayerCallback();
    if (!callback) {
      NS_ERROR("GL should never need to update ThebesLayers in an empty transaction");
    } else {
      void* callbackData = mOGLManager->GetThebesLayerCallbackData();
      SetAntialiasingFlags(this, state.mContext);
      callback(this, state.mContext, state.mRegionToDraw,
               state.mRegionToInvalidate, callbackData);
      
      
      
      
      nsIntRegion tmp;
      tmp.Or(mVisibleRegion, state.mRegionToDraw);
      mValidRegion.Or(mValidRegion, tmp);
    }
  }

  
  gl()->MakeCurrent();

  gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
  mBuffer->RenderTo(aOffset, mOGLManager, flags);
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


class ShadowBufferOGL : public ThebesLayerBufferOGL
{
public:
  ShadowBufferOGL(ShadowThebesLayerOGL* aLayer)
    : ThebesLayerBufferOGL(aLayer, aLayer)
  {}

  virtual PaintState BeginPaint(ContentType aContentType,
                                float, float, PRUint32) {
    NS_RUNTIMEABORT("can't BeginPaint for a shadow layer");
    return PaintState();
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
  gfxIntSize size = aUpdate->GetSize();
  if (GetSize() != nsIntSize(size.width, size.height)) {
    
    
    mTexImage = CreateClampOrRepeatTextureImage(gl(),
      nsIntSize(size.width, size.height), aUpdate->GetContentType(), ALLOW_REPEAT);
  }

  nsIntRegion destRegion(aUpdated);
  
  
  nsIntPoint visTopLeft = mLayer->GetVisibleRegion().GetBounds().TopLeft();
  destRegion.MoveBy(-visTopLeft);

  
  
  
  
  
  
  
  
  
  
  
  nsIntRect destBounds = destRegion.GetBounds();
  gfxRect destRect(destBounds.x, destBounds.y, destBounds.width, destBounds.height);
  destRect.Scale(mLayer->GetXResolution(), mLayer->GetYResolution());
  destRect.RoundOut();

  
  nsIntRegion scaledDestRegion(nsIntRect(destRect.X(), destRect.Y(),
                                         destRect.Width(), destRect.Height()));
  mTexImage->DirectUpdate(aUpdate, scaledDestRegion);

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
ShadowThebesLayerOGL::SetFrontBuffer(const OptionalThebesBuffer& aNewFront,
                                     const nsIntRegion& aValidRegion,
                                     float aXResolution, float aYResolution)
{
  if (mDestroyed) {
    return;
  }

  if (!mBuffer) {
    mBuffer = new ShadowBufferOGL(this);
  }

  NS_ASSERTION(OptionalThebesBuffer::Tnull_t == aNewFront.type(),
               "Only one system-memory buffer expected");
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
}

void
ShadowThebesLayerOGL::Disconnect()
{
  Destroy();
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

  gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
  mBuffer->RenderTo(aOffset, mOGLManager, 0);
}

} 
} 
