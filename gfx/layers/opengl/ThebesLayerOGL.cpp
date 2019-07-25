




#include "ipc/AutoOpenSurface.h"
#include "mozilla/layers/PLayers.h"
#include "TiledLayerBuffer.h"


#include "mozilla/Util.h"

#include "mozilla/layers/ShadowLayers.h"

#include "ThebesLayerBuffer.h"
#include "ThebesLayerOGL.h"
#include "gfxUtils.h"
#include "gfxTeeSurface.h"

#include "base/message_loop.h"

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
    , mInitialised(true)
  {}
  virtual ~ThebesLayerBufferOGL() {}

  enum { PAINT_WILL_RESAMPLE = ThebesLayerBuffer::PAINT_WILL_RESAMPLE };
  virtual PaintState BeginPaint(ContentType aContentType,
                                PRUint32 aFlags) = 0;

  void RenderTo(const nsIntPoint& aOffset, LayerManagerOGL* aManager,
                PRUint32 aFlags);

  nsIntSize GetSize() {
    if (mTexImage)
      return mTexImage->GetSize();
    return nsIntSize(0, 0);
  }

  bool Initialised() { return mInitialised; }

protected:
  virtual nsIntPoint GetOriginOffset() = 0;

  GLContext* gl() const { return mOGLLayer->gl(); }

  ThebesLayer* mLayer;
  LayerOGL* mOGLLayer;
  nsRefPtr<TextureImage> mTexImage;
  nsRefPtr<TextureImage> mTexImageOnWhite;
  bool mInitialised;
};

void
ThebesLayerBufferOGL::RenderTo(const nsIntPoint& aOffset,
                               LayerManagerOGL* aManager,
                               PRUint32 aFlags)
{
  NS_ASSERTION(Initialised(), "RenderTo with uninitialised buffer!");

  if (!mTexImage || !Initialised())
    return;

  if (mTexImage->InUpdate()) {
    mTexImage->EndUpdate();
  }

  if (mTexImageOnWhite && mTexImageOnWhite->InUpdate()) {
    mTexImageOnWhite->EndUpdate();
  }

#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    nsRefPtr<gfxImageSurface> surf = 
      gl()->GetTexImage(mTexImage->GetTextureID(), false, mTexImage->GetShaderProgramType());
    
    WriteSnapshotToDumpFile(mLayer, surf);
  }
#endif

  PRInt32 passes = mTexImageOnWhite ? 2 : 1;
  for (PRInt32 pass = 1; pass <= passes; ++pass) {
    ShaderProgramOGL *program;

    if (passes == 2) {
      ShaderProgramOGL* alphaProgram;
      if (pass == 1) {
        alphaProgram = aManager->GetProgram(gl::ComponentAlphaPass1ProgramType,
                                            mLayer->GetMaskLayer());
        gl()->fBlendFuncSeparate(LOCAL_GL_ZERO, LOCAL_GL_ONE_MINUS_SRC_COLOR,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
      } else {
        alphaProgram = aManager->GetProgram(gl::ComponentAlphaPass2ProgramType,
                                            mLayer->GetMaskLayer());
        gl()->fBlendFuncSeparate(LOCAL_GL_ONE, LOCAL_GL_ONE,
                                 LOCAL_GL_ONE, LOCAL_GL_ONE);
      }

      alphaProgram->Activate();
      alphaProgram->SetBlackTextureUnit(0);
      alphaProgram->SetWhiteTextureUnit(1);
      program = alphaProgram;
    } else {
      
      
      ShaderProgramOGL* basicProgram =
        aManager->GetProgram(mTexImage->GetShaderProgramType(),
                             mLayer->GetMaskLayer());

      basicProgram->Activate();
      basicProgram->SetTextureUnit(0);
      program = basicProgram;
    }

    program->SetLayerOpacity(mLayer->GetEffectiveOpacity());
    program->SetLayerTransform(mLayer->GetEffectiveTransform());
    program->SetRenderOffset(aOffset);
    program->LoadMask(mLayer->GetMaskLayer());

    const nsIntRegion& visibleRegion = mLayer->GetEffectiveVisibleRegion();
    nsIntRegion tmpRegion;
    const nsIntRegion* renderRegion;
    if (aFlags & PAINT_WILL_RESAMPLE) {
      
      
      
      tmpRegion = visibleRegion.GetBounds();
      renderRegion = &tmpRegion;
    } else {
      renderRegion = &visibleRegion;
    }

    nsIntRegion region(*renderRegion);
    nsIntPoint origin = GetOriginOffset();
    region.MoveBy(-origin);           

    
    nsIntSize texSize = mTexImage->GetSize();
    nsIntRect textureRect = nsIntRect(0, 0, texSize.width, texSize.height);
    textureRect.MoveBy(region.GetBounds().TopLeft());
    nsIntRegion subregion;
    subregion.And(region, textureRect);
    if (subregion.IsEmpty())  
      return;

    nsIntRegion screenRects;
    nsIntRegion regionRects;

    
    nsIntRegionRectIterator iter(subregion);
    while (const nsIntRect* iterRect = iter.Next()) {
        nsIntRect regionRect = *iterRect;
        nsIntRect screenRect = regionRect;
        screenRect.MoveBy(origin);

        screenRects.Or(screenRects, screenRect);
        regionRects.Or(regionRects, regionRect);
    }

    mTexImage->BeginTileIteration();
    if (mTexImageOnWhite) {
      NS_ASSERTION(mTexImage->GetTileCount() == mTexImageOnWhite->GetTileCount(),
                   "Tile count mismatch on component alpha texture");
      mTexImageOnWhite->BeginTileIteration();
    }

    bool usingTiles = (mTexImage->GetTileCount() > 1);
    do {
      if (mTexImageOnWhite) {
        NS_ASSERTION(mTexImageOnWhite->GetTileRect() == mTexImage->GetTileRect(), "component alpha textures should be the same size.");
      }

      nsIntRect tileRect = mTexImage->GetTileRect();

      
      TextureImage::ScopedBindTexture texBind(mTexImage, LOCAL_GL_TEXTURE0);
      TextureImage::ScopedBindTexture texOnWhiteBind(mTexImageOnWhite, LOCAL_GL_TEXTURE1);

      
      
      
      
      for (int y = 0; y < (usingTiles ? 2 : 1); y++) {
        for (int x = 0; x < (usingTiles ? 2 : 1); x++) {
          nsIntRect currentTileRect(tileRect);
          currentTileRect.MoveBy(x * texSize.width, y * texSize.height);

          nsIntRegionRectIterator screenIter(screenRects);
          nsIntRegionRectIterator regionIter(regionRects);

          const nsIntRect* screenRect;
          const nsIntRect* regionRect;
          while ((screenRect = screenIter.Next()) &&
                 (regionRect = regionIter.Next())) {
              nsIntRect tileScreenRect(*screenRect);
              nsIntRect tileRegionRect(*regionRect);

              
              
              
              if (usingTiles) {
                  tileScreenRect.MoveBy(-origin);
                  tileScreenRect = tileScreenRect.Intersect(currentTileRect);
                  tileScreenRect.MoveBy(origin);

                  if (tileScreenRect.IsEmpty())
                    continue;

                  tileRegionRect = regionRect->Intersect(currentTileRect);
                  tileRegionRect.MoveBy(-currentTileRect.TopLeft());
              }

#ifdef ANDROID
              
              
              
              gfxMatrix matrix;
              bool is2D = mLayer->GetEffectiveTransform().Is2D(&matrix);
              if (is2D && !matrix.HasNonTranslationOrFlip()) {
                gl()->ApplyFilterToBoundTexture(gfxPattern::FILTER_NEAREST);
              } else {
                mTexImage->ApplyFilter();
              }
#endif
              program->SetLayerQuadRect(tileScreenRect);
              aManager->BindAndDrawQuadWithTextureRect(program, tileRegionRect,
                                                       tileRect.Size(),
                                                       mTexImage->GetWrapMode());
          }
        }
      }

      if (mTexImageOnWhite)
          mTexImageOnWhite->NextTile();
    } while (mTexImage->NextTile());
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
                                PRUint32 aFlags)
  {
    
    return ThebesLayerBuffer::BeginPaint(mLayer, 
                                         aContentType, 
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

BasicBufferOGL::PaintState
BasicBufferOGL::BeginPaint(ContentType aContentType,
                           PRUint32 aFlags)
{
  PaintState result;
  
  
  bool canHaveRotation =  !(aFlags & PAINT_WILL_RESAMPLE);

  nsIntRegion validRegion = mLayer->GetValidRegion();

  Layer::SurfaceMode mode;
  ContentType contentType;
  nsIntRegion neededRegion;
  bool canReuseBuffer;
  nsIntRect destBufferRect;

  while (true) {
    mode = mLayer->GetSurfaceMode();
    contentType = aContentType;
    neededRegion = mLayer->GetVisibleRegion();
    
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
    }

    if (mTexImage &&
        (mTexImage->GetContentType() != contentType ||
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

  if (destBufferRect.width > gl()->GetMaxTextureImageSize() ||
      destBufferRect.height > gl()->GetMaxTextureImageSize()) {
    return result;
  }

  nsIntRect drawBounds = result.mRegionToDraw.GetBounds();
  nsRefPtr<TextureImage> destBuffer;
  nsRefPtr<TextureImage> destBufferOnWhite;

  PRUint32 bufferFlags = canHaveRotation ? ALLOW_REPEAT : 0;
  if (canReuseBuffer) {
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
        destBuffer = CreateClampOrRepeatTextureImage(gl(), destBufferRect.Size(), contentType, bufferFlags);
        if (!destBuffer)
          return result;
        if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
          destBufferOnWhite =
            CreateClampOrRepeatTextureImage(gl(), destBufferRect.Size(), contentType, bufferFlags);
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
    
    destBuffer = CreateClampOrRepeatTextureImage(gl(), destBufferRect.Size(), contentType, bufferFlags);
    if (!destBuffer)
      return result;

    if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
      destBufferOnWhite = 
        CreateClampOrRepeatTextureImage(gl(), destBufferRect.Size(), contentType, bufferFlags);
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
        
        if (mBufferRotation != nsIntPoint(0, 0)) {
          
          
          
          
          
          
          
          nsIntPoint rotationPoint(mBufferRect.x + mBufferRect.width - mBufferRotation.x, 
                                   mBufferRect.y + mBufferRect.height - mBufferRotation.y);

          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          nsIntRect bottom(mBufferRect.x, rotationPoint.y, mBufferRect.width, mBufferRotation.y);
          
          nsIntRect topright(rotationPoint.x, mBufferRect.y, mBufferRotation.x, rotationPoint.y - mBufferRect.y);

          if (!bottom.IsEmpty()) {
            nsIntRegion temp;
            temp.And(destBufferRect, bottom);
            result.mRegionToDraw.Or(result.mRegionToDraw, temp);
          }
          if (!topright.IsEmpty()) {
            nsIntRegion temp;
            temp.And(destBufferRect, topright);
            result.mRegionToDraw.Or(result.mRegionToDraw, temp);
          }
        }

        destBuffer->Resize(destBufferRect.Size());

        gl()->BlitTextureImage(mTexImage, srcRect,
                               destBuffer, dstRect);
        destBuffer->MarkValid();
        if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
          destBufferOnWhite->Resize(destBufferRect.Size());
          gl()->BlitTextureImage(mTexImageOnWhite, srcRect,
                                 destBufferOnWhite, dstRect);
          destBufferOnWhite->MarkValid();
        }
      } else {
        
        destBuffer = CreateClampOrRepeatTextureImage(gl(), destBufferRect.Size(), contentType, bufferFlags);
        if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
          destBufferOnWhite = 
            CreateClampOrRepeatTextureImage(gl(), destBufferRect.Size(), contentType, bufferFlags);
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
  
  
  if (mode == Layer::SURFACE_COMPONENT_ALPHA) {
    nsIntRegion drawRegionCopy = result.mRegionToDraw;
    gfxASurface *onBlack = mTexImage->BeginUpdate(drawRegionCopy);
    gfxASurface *onWhite = mTexImageOnWhite->BeginUpdate(result.mRegionToDraw);
    NS_ASSERTION(result.mRegionToDraw == drawRegionCopy,
                 "BeginUpdate should always modify the draw region in the same way!");
    FillSurface(onBlack, result.mRegionToDraw, nsIntPoint(0,0), gfxRGBA(0.0, 0.0, 0.0, 1.0));
    FillSurface(onWhite, result.mRegionToDraw, nsIntPoint(0,0), gfxRGBA(1.0, 1.0, 1.0, 1.0));
    gfxASurface* surfaces[2] = { onBlack, onWhite };
    nsRefPtr<gfxTeeSurface> surf = new gfxTeeSurface(surfaces, ArrayLength(surfaces));

    
    
    gfxPoint deviceOffset = onBlack->GetDeviceOffset();
    onBlack->SetDeviceOffset(gfxPoint(0, 0));
    onWhite->SetDeviceOffset(gfxPoint(0, 0));
    surf->SetDeviceOffset(deviceOffset);

    
    
    
    surf->SetAllowUseAsSource(false);
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
  result.mContext->Translate(-gfxPoint(quadrantRect.x, quadrantRect.y));
  
  
  result.mRegionToDraw.MoveBy(-offset);

  
  
  
  
  
  
  
  gfxUtils::ClipToRegion(result.mContext, result.mRegionToDraw);

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
    mDestroyed = true;
  }
}

bool
ThebesLayerOGL::CreateSurface()
{
  NS_ASSERTION(!mBuffer, "buffer already created?");

  if (mVisibleRegion.IsEmpty()) {
    return false;
  }

  if (gl()->TextureImageSupportsGetBackingSurface()) {
    
    mBuffer = new SurfaceBufferOGL(this);
  } else {
    mBuffer = new BasicBufferOGL(this);
  }
  return true;
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

  PRUint32 flags = 0;
#ifndef MOZ_GFX_OPTIMIZE_MOBILE
  gfxMatrix transform2d;
  if (GetEffectiveTransform().Is2D(&transform2d)) {
    if (transform2d.HasNonIntegerTranslation()) {
      flags |= ThebesLayerBufferOGL::PAINT_WILL_RESAMPLE;
    }
  } else {
    flags |= ThebesLayerBufferOGL::PAINT_WILL_RESAMPLE;
  }
#endif

  Buffer::PaintState state = mBuffer->BeginPaint(contentType, flags);
  mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

  if (state.mContext) {
    state.mRegionToInvalidate.And(state.mRegionToInvalidate, mVisibleRegion);

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

bool
ThebesLayerOGL::IsEmpty()
{
  return !mBuffer;
}

void
ThebesLayerOGL::CleanupResources()
{
  mBuffer = nsnull;
}

class ShadowBufferOGL : public ThebesLayerBufferOGL
{
public:
  ShadowBufferOGL(ShadowThebesLayerOGL* aLayer)
    : ThebesLayerBufferOGL(aLayer, aLayer)
  {
    mInitialised = false;
  }

  virtual PaintState BeginPaint(ContentType aContentType, PRUint32) {
    NS_RUNTIMEABORT("can't BeginPaint for a shadow layer");
    return PaintState();
  }

  void EnsureTexture(gfxIntSize aSize, ContentType aContentType);

  void DirectUpdate(gfxASurface* aUpdate, nsIntRegion& aRegion);

  void Upload(gfxASurface* aUpdate, const nsIntRegion& aUpdated,
              const nsIntRect& aRect, const nsIntPoint& aRotation,
              bool aDelayUpload, nsIntRegion& aPendingUploadRegion);

  nsRefPtr<TextureImage> GetTextureImage() { return mTexImage; }

protected:
  virtual nsIntPoint GetOriginOffset() {
    return mBufferRect.TopLeft() - mBufferRotation;
  }

private:
  nsIntRect mBufferRect;
  nsIntPoint mBufferRotation;
};

void
ShadowBufferOGL::EnsureTexture(gfxIntSize aSize, ContentType aContentType)
{
  if (!mTexImage ||
      GetSize() != nsIntSize(aSize.width, aSize.height) ||
      mTexImage->GetContentType() != aContentType) {
    
    
    mTexImage = CreateClampOrRepeatTextureImage(gl(),
      nsIntSize(aSize.width, aSize.height), aContentType, ALLOW_REPEAT);
    mInitialised = false;
  }
}

void
ShadowBufferOGL::DirectUpdate(gfxASurface* aUpdate, nsIntRegion& aRegion)
{
  EnsureTexture(aUpdate->GetSize(), aUpdate->GetContentType());
  mInitialised = true;
  mTexImage->DirectUpdate(aUpdate, aRegion);
}

void
ShadowBufferOGL::Upload(gfxASurface* aUpdate, const nsIntRegion& aUpdated,
                        const nsIntRect& aRect, const nsIntPoint& aRotation,
                        bool aDelayUpload, nsIntRegion& aPendingUploadRegion)
{
  
  nsIntRegion destRegion(aUpdated);
  destRegion.MoveBy(-aRect.TopLeft());

  
  destRegion.MoveBy(aRotation);
  gfxIntSize size = aUpdate->GetSize();
  nsIntRect destBounds = destRegion.GetBounds();
  destRegion.MoveBy((destBounds.x >= size.width) ? -size.width : 0,
                    (destBounds.y >= size.height) ? -size.height : 0);

  
  
  NS_ASSERTION(((destBounds.x % size.width) + destBounds.width <= size.width) &&
               ((destBounds.y % size.height) + destBounds.height <= size.height),
               "Updated region lies across rotation boundaries!");

  if (aDelayUpload) {
    
    
    aPendingUploadRegion.Or(aPendingUploadRegion, destRegion).
      And(aPendingUploadRegion, nsIntRect(0, 0, size.width, size.height));
  } else {
    
    DirectUpdate(aUpdate, destRegion);
    aPendingUploadRegion.Sub(aPendingUploadRegion, destRegion);
  }

  mBufferRect = aRect;
  mBufferRotation = aRotation;
}

ShadowThebesLayerOGL::ShadowThebesLayerOGL(LayerManagerOGL *aManager)
  : ShadowThebesLayer(aManager, nsnull)
  , LayerOGL(aManager)
  , mUploadTask(nsnull)
{
#ifdef FORCE_BASICTILEDTHEBESLAYER
  NS_ABORT();
#endif
  mImplData = static_cast<LayerOGL*>(this);
}

ShadowThebesLayerOGL::~ShadowThebesLayerOGL()
{}

bool
ShadowThebesLayerOGL::ShouldDoubleBuffer()
{
#ifdef MOZ_JAVA_COMPOSITOR
  










  return gl()->WantsSmallTiles();
#else
  return false;
#endif
}

void
ShadowThebesLayerOGL::EnsureTextureUpdated()
{
  if (mRegionPendingUpload.IsEmpty() || !IsSurfaceDescriptorValid(mFrontBufferDescriptor))
    return;

  AutoOpenSurface frontSurface(OPEN_READ_ONLY, mFrontBuffer.Buffer());
  mBuffer->DirectUpdate(frontSurface.Get(), mRegionPendingUpload);
  mRegionPendingUpload.SetEmpty();
}

static bool
EnsureTextureUpdatedCallback(gl::TextureImage* aImage, int aTileNumber,
                             void *aData)
{
  
  
  
  nsIntRegion* updateRegion = (nsIntRegion*)aData;
  nsIntRect tileRect = aImage->GetTileRect();
  if (updateRegion->Intersects(tileRect))
    updateRegion->Or(*updateRegion, tileRect);
  return true;
}

void
ShadowThebesLayerOGL::EnsureTextureUpdated(nsIntRegion& aRegion)
{
  if (mRegionPendingUpload.IsEmpty() || !IsSurfaceDescriptorValid(mFrontBufferDescriptor))
    return;

  
  nsIntRegion updateRegion;
  nsIntRect bufferRect = mFrontBuffer.Rect();
  for (int i = 0; i < 4; i++) {
    switch(i) {
      case 0:
        
        
        aRegion.MoveBy(mFrontBuffer.Rotation());
        break;
      case 1:
      case 3:
        
        
        aRegion.MoveBy(-bufferRect.width, 0);
        break;
      case 2:
        
        
        aRegion.MoveBy(bufferRect.width, -bufferRect.height);
    }

    
    
    updateRegion.And(aRegion, mRegionPendingUpload);

    if (updateRegion.IsEmpty())
      continue;

    AutoOpenSurface surface(OPEN_READ_ONLY, mFrontBuffer.Buffer());
    nsRefPtr<TextureImage> texImage;
    if (!gl()->CanUploadSubTextures()) {
      
      
      
      
      gfxIntSize size = surface.Size();
      mBuffer->EnsureTexture(size, surface.ContentType());
      texImage = mBuffer->GetTextureImage().get();
      if (texImage->GetTileCount() > 1)
        texImage->SetIterationCallback(EnsureTextureUpdatedCallback, (void *)&updateRegion);
      else
        updateRegion = nsIntRect(0, 0, size.width, size.height);
    }

    
    mBuffer->DirectUpdate(surface.Get(), updateRegion);

    if (!gl()->CanUploadSubTextures())
      texImage->SetIterationCallback(nsnull, nsnull);

    
    mRegionPendingUpload.Sub(mRegionPendingUpload, updateRegion);
  }
}

static bool
ProgressiveUploadCallback(gl::TextureImage* aImage, int aTileNumber,
                          void *aData)
{
  nsIntRegion* regionPendingUpload = (nsIntRegion*)aData;

  
  nsIntRect tileRect = aImage->GetTileRect();
  if (!regionPendingUpload->Intersects(tileRect))
    return true;

  regionPendingUpload->Sub(*regionPendingUpload, tileRect);

  
  
  
  return false;
}

void
ShadowThebesLayerOGL::ProgressiveUpload()
{
  
  mUploadTask = nsnull;

  if (mRegionPendingUpload.IsEmpty() || mBuffer == nsnull)
    return;

  
  
  AutoOpenSurface frontSurface(OPEN_READ_ONLY, mFrontBuffer.Buffer());
  mBuffer->EnsureTexture(frontSurface.Size(),
                         frontSurface.ContentType());
  nsRefPtr<gl::TextureImage> tiledImage = mBuffer->GetTextureImage().get();
  if (tiledImage->GetTileCount() > 1)
    tiledImage->SetIterationCallback(ProgressiveUploadCallback, (void *)&mRegionPendingUpload);
  else
    mRegionPendingUpload.SetEmpty();

  
  mBuffer->DirectUpdate(frontSurface.Get(), mRegionPendingUpload);

  
  tiledImage->SetIterationCallback(nsnull, nsnull);

  if (!mRegionPendingUpload.IsEmpty()) {
    
    mUploadTask = NewRunnableMethod(this, &ShadowThebesLayerOGL::ProgressiveUpload);
    
    
    MessageLoop::current()->PostDelayedTask(FROM_HERE, mUploadTask, 5);
  }
}

void
ShadowThebesLayerOGL::Swap(const ThebesBuffer& aNewFront,
                           const nsIntRegion& aUpdatedRegion,
                           OptionalThebesBuffer* aNewBack,
                           nsIntRegion* aNewBackValidRegion,
                           OptionalThebesBuffer* aReadOnlyFront,
                           nsIntRegion* aFrontUpdatedRegion)
{
  
  if (ShouldDoubleBuffer()) {
    AutoOpenSurface newFrontBuffer(OPEN_READ_ONLY, aNewFront.buffer());

    if (IsSurfaceDescriptorValid(mFrontBufferDescriptor)) {
      AutoOpenSurface currentFront(OPEN_READ_ONLY, mFrontBufferDescriptor);
      if (currentFront.Size() != newFrontBuffer.Size()) {
        
        DestroyFrontBuffer();
      }
    }

    
    if (IsSurfaceDescriptorValid(mFrontBufferDescriptor)) {
      *aNewBack = ThebesBuffer();
      aNewBack->get_ThebesBuffer().buffer() = mFrontBufferDescriptor;
    } else {
      *aNewBack = null_t();
    }

    
    
    aNewBackValidRegion->Sub(mOldValidRegion, aUpdatedRegion);

    SurfaceDescriptor unused;
    nsIntRect backRect;
    nsIntPoint backRotation;
    mFrontBuffer.Swap(
      aNewFront.buffer(), aNewFront.rect(), aNewFront.rotation(),
      &unused, &backRect, &backRotation);

    if (aNewBack->type() != OptionalThebesBuffer::Tnull_t) {
      aNewBack->get_ThebesBuffer().rect() = backRect;
      aNewBack->get_ThebesBuffer().rotation() = backRotation;
    }

    mFrontBufferDescriptor = aNewFront.buffer();

    
    if (!mDestroyed) {
      if (!mBuffer) {
        mBuffer = new ShadowBufferOGL(this);
      }
      AutoOpenSurface frontSurface(OPEN_READ_ONLY, mFrontBufferDescriptor);
      mBuffer->Upload(frontSurface.Get(), aUpdatedRegion, aNewFront.rect(), aNewFront.rotation(), true, mRegionPendingUpload);

      
      if (!mUploadTask) {
        mUploadTask = NewRunnableMethod(this, &ShadowThebesLayerOGL::ProgressiveUpload);
        
        MessageLoop::current()->PostDelayedTask(FROM_HERE, mUploadTask, 5);
      }
    }

    *aReadOnlyFront = aNewFront;
    *aFrontUpdatedRegion = aUpdatedRegion;

    return;
  }

  
  if (!mDestroyed) {
    if (!mBuffer) {
      mBuffer = new ShadowBufferOGL(this);
    }
    AutoOpenSurface frontSurface(OPEN_READ_ONLY, aNewFront.buffer());
    mBuffer->Upload(frontSurface.Get(), aUpdatedRegion, aNewFront.rect(), aNewFront.rotation(), false, mRegionPendingUpload);
  }

  *aNewBack = aNewFront;
  *aNewBackValidRegion = mValidRegion;
  *aReadOnlyFront = null_t();
  aFrontUpdatedRegion->SetEmpty();
}

void
ShadowThebesLayerOGL::DestroyFrontBuffer()
{
  if (ShouldDoubleBuffer()) {
    
    if (mUploadTask) {
      mUploadTask->Cancel();
      mUploadTask = nsnull;
    }

    mFrontBuffer.Clear();
    mOldValidRegion.SetEmpty();

    if (IsSurfaceDescriptorValid(mFrontBufferDescriptor)) {
      mAllocator->DestroySharedSurface(&mFrontBufferDescriptor);
    }
  }

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
    mDestroyed = true;
    DestroyFrontBuffer();
  }
}

Layer*
ShadowThebesLayerOGL::GetLayer()
{
  return this;
}

bool
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

  if (ShouldDoubleBuffer()) {
    
    gfxMatrix transform2d;
    const gfx3DMatrix& transform = GetLayer()->GetEffectiveTransform();

    if (transform.Is2D(&transform2d) && transform2d.PreservesAxisAlignedRectangles()) {
      
      
      nsIntRect bufferRect = mFrontBuffer.Rect();
      gfxRect layerRect = transform2d.Transform(gfxRect(bufferRect));
      layerRect.MoveBy(gfxPoint(aOffset));

      
      
      nsIntSize widgetSize = mOGLManager->GetWidgetSize();
      gfxRect clippedLayerRect = layerRect.Intersect(gfxRect(0, 0, widgetSize.width, widgetSize.height));

      
      
      gfxPoint scaleFactor = gfxPoint(bufferRect.width / (float)layerRect.width,
                                      bufferRect.height / (float)layerRect.height);
      float x1 = (clippedLayerRect.x - layerRect.x) * scaleFactor.x;
      float y1 = (clippedLayerRect.y - layerRect.y) * scaleFactor.y;
      float x2 = (clippedLayerRect.XMost() - layerRect.x) * scaleFactor.x;
      float y2 = (clippedLayerRect.YMost() - layerRect.y) * scaleFactor.y;

      
      nsIntRect updateRect = nsIntRect(NS_MAX(0.0f, x1), NS_MAX(0.0f, y1),
                                       NS_MAX(0.0f, x2 - x1), NS_MAX(0.0f, y2 - y1));

      nsIntRegion updateRegion = updateRect;
      EnsureTextureUpdated(updateRegion);
    } else {
      
      
      
      EnsureTextureUpdated();
    }
  }

  gl()->fActiveTexture(LOCAL_GL_TEXTURE0);

  gl()->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, aPreviousFrameBuffer);
  mBuffer->RenderTo(aOffset, mOGLManager, 0);
}

void
ShadowThebesLayerOGL::CleanupResources()
{
  DestroyFrontBuffer();
}

} 
} 
