





































#include "gfxSharedImageSurface.h"

#include "mozilla/layers/PLayerChild.h"
#include "mozilla/layers/PLayersChild.h"
#include "mozilla/layers/PLayersParent.h"
#include "ipc/ShadowLayerChild.h"

#include "BasicLayers.h"
#include "ImageLayers.h"

#include "nsTArray.h"
#include "nsGUIEvent.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxPattern.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "ThebesLayerBuffer.h"
#include "nsIWidget.h"
#include "ReadbackProcessor.h"
#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif

#include "GLContext.h"

namespace mozilla {
namespace layers {

class BasicContainerLayer;
class ShadowableLayer;























class BasicImplData {
public:
  BasicImplData()
  {
    MOZ_COUNT_CTOR(BasicImplData);
  }
  virtual ~BasicImplData()
  {
    MOZ_COUNT_DTOR(BasicImplData);
  }

  





  virtual void Paint(gfxContext* aContext) {}

  



  virtual void PaintThebes(gfxContext* aContext,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback) {}

  virtual ShadowableLayer* AsShadowableLayer() { return nsnull; }

  





  virtual bool MustRetainContent() { return false; }

  




  virtual void ClearCachedResources() {}

  





  void SetCoveredByOpaque(PRBool aCovered) { mCoveredByOpaque = aCovered; }
  PRBool IsCoveredByOpaque() const { return mCoveredByOpaque; }

protected:
  PRPackedBool mCoveredByOpaque;
};

static BasicImplData*
ToData(Layer* aLayer)
{
  return static_cast<BasicImplData*>(aLayer->ImplData());
}

template<class Container>
static void ContainerInsertAfter(Layer* aChild, Layer* aAfter, Container* aContainer);
template<class Container>
static void ContainerRemoveChild(Layer* aChild, Container* aContainer);

class BasicContainerLayer : public ContainerLayer, BasicImplData {
  template<class Container>
  friend void ContainerInsertAfter(Layer* aChild, Layer* aAfter, Container* aContainer);
  template<class Container>
  friend void ContainerRemoveChild(Layer* aChild, Container* aContainer);

public:
  BasicContainerLayer(BasicLayerManager* aManager) :
    ContainerLayer(aManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicContainerLayer);
    mSupportsComponentAlphaChildren = PR_TRUE;
  }
  virtual ~BasicContainerLayer();

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::SetVisibleRegion(aRegion);
  }
  virtual void InsertAfter(Layer* aChild, Layer* aAfter)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerInsertAfter(aChild, aAfter, this);
  }

  virtual void RemoveChild(Layer* aChild)
  { 
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerRemoveChild(aChild, this);
  }

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    
    
    
    gfx3DMatrix idealTransform = GetLocalTransform()*aTransformToSurface;
    mEffectiveTransform = SnapTransform(idealTransform, gfxRect(0, 0, 0, 0), nsnull);
    
    
    ComputeEffectiveTransformsForChildren(idealTransform);

    




    mUseIntermediateSurface = GetEffectiveOpacity() != 1.0 && HasMultipleChildren();
  }

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

BasicContainerLayer::~BasicContainerLayer()
{
  while (mFirstChild) {
    ContainerRemoveChild(mFirstChild, this);
  }

  MOZ_COUNT_DTOR(BasicContainerLayer);
}

template<class Container>
static void
ContainerInsertAfter(Layer* aChild, Layer* aAfter, Container* aContainer)
{
  NS_ASSERTION(aChild->Manager() == aContainer->Manager(),
               "Child has wrong manager");
  NS_ASSERTION(!aChild->GetParent(),
               "aChild already in the tree");
  NS_ASSERTION(!aChild->GetNextSibling() && !aChild->GetPrevSibling(),
               "aChild already has siblings?");
  NS_ASSERTION(!aAfter ||
               (aAfter->Manager() == aContainer->Manager() &&
                aAfter->GetParent() == aContainer),
               "aAfter is not our child");

  aChild->SetParent(aContainer);
  if (aAfter == aContainer->mLastChild) {
    aContainer->mLastChild = aChild;
  }
  if (!aAfter) {
    aChild->SetNextSibling(aContainer->mFirstChild);
    if (aContainer->mFirstChild) {
      aContainer->mFirstChild->SetPrevSibling(aChild);
    }
    aContainer->mFirstChild = aChild;
    NS_ADDREF(aChild);
    aContainer->DidInsertChild(aChild);
    return;
  }

  Layer* next = aAfter->GetNextSibling();
  aChild->SetNextSibling(next);
  aChild->SetPrevSibling(aAfter);
  if (next) {
    next->SetPrevSibling(aChild);
  }
  aAfter->SetNextSibling(aChild);
  NS_ADDREF(aChild);
  aContainer->DidInsertChild(aChild);
}

template<class Container>
static void
ContainerRemoveChild(Layer* aChild, Container* aContainer)
{
  NS_ASSERTION(aChild->Manager() == aContainer->Manager(),
               "Child has wrong manager");
  NS_ASSERTION(aChild->GetParent() == aContainer,
               "aChild not our child");

  Layer* prev = aChild->GetPrevSibling();
  Layer* next = aChild->GetNextSibling();
  if (prev) {
    prev->SetNextSibling(next);
  } else {
    aContainer->mFirstChild = next;
  }
  if (next) {
    next->SetPrevSibling(prev);
  } else {
    aContainer->mLastChild = prev;
  }

  aChild->SetNextSibling(nsnull);
  aChild->SetPrevSibling(nsnull);
  aChild->SetParent(nsnull);

  aContainer->DidRemoveChild(aChild);
  NS_RELEASE(aChild);
}

class BasicThebesLayer;
class BasicThebesLayerBuffer : public ThebesLayerBuffer {
  typedef ThebesLayerBuffer Base;

public:
  BasicThebesLayerBuffer(BasicThebesLayer* aLayer)
    : Base(ContainsVisibleBounds)
    , mLayer(aLayer)
  {
  }

  virtual ~BasicThebesLayerBuffer()
  {}

  using Base::BufferRect;
  using Base::BufferRotation;

  




  void DrawTo(ThebesLayer* aLayer, gfxContext* aTarget, float aOpacity);

  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize, PRUint32 aFlags);

  


  void SetBackingBuffer(gfxASurface* aBuffer,
                        const nsIntRect& aRect, const nsIntPoint& aRotation)
  {
    gfxIntSize prevSize = gfxIntSize(BufferDims().width, BufferDims().height);
    gfxIntSize newSize = aBuffer->GetSize();
    NS_ABORT_IF_FALSE(newSize == prevSize,
                      "Swapped-in buffer size doesn't match old buffer's!");
    nsRefPtr<gfxASurface> oldBuffer;
    oldBuffer = SetBuffer(aBuffer, nsIntSize(newSize.width, newSize.height),
                          aRect, aRotation);
  }

  void SetBackingBufferAndUpdateFrom(
    gfxASurface* aBuffer,
    gfxASurface* aSource, const nsIntRect& aRect, const nsIntPoint& aRotation,
    const nsIntRegion& aUpdateRegion, float aXResolution, float aYResolution);

private:
  BasicThebesLayerBuffer(gfxASurface* aBuffer,
                         const nsIntRect& aRect, const nsIntPoint& aRotation)
    
    
    : ThebesLayerBuffer(ContainsVisibleBounds)
  {
    gfxIntSize sz = aBuffer->GetSize();
    SetBuffer(aBuffer, nsIntSize(sz.width, sz.height), aRect, aRotation);
  }

  BasicThebesLayer* mLayer;
};

class BasicThebesLayer : public ThebesLayer, BasicImplData {
public:
  typedef BasicThebesLayerBuffer Buffer;

  BasicThebesLayer(BasicLayerManager* aLayerManager) :
    ThebesLayer(aLayerManager, static_cast<BasicImplData*>(this)),
    mBuffer(this)
  {
    MOZ_COUNT_CTOR(BasicThebesLayer);
  }
  virtual ~BasicThebesLayer()
  {
    MOZ_COUNT_DTOR(BasicThebesLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ThebesLayer::SetVisibleRegion(aRegion);
  }
  virtual void InvalidateRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    mValidRegion.Sub(mValidRegion, aRegion);
  }

  virtual void PaintThebes(gfxContext* aContext,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback);

  virtual void ClearCachedResources() { mBuffer.Clear(); mValidRegion.SetEmpty(); }
  
  virtual already_AddRefed<gfxASurface>
  CreateBuffer(Buffer::ContentType aType, const nsIntSize& aSize)
  {
    nsRefPtr<gfxASurface> referenceSurface = mBuffer.GetBuffer();
    if (!referenceSurface) {
      gfxContext* defaultTarget = BasicManager()->GetDefaultTarget();
      if (defaultTarget) {
        referenceSurface = defaultTarget->CurrentSurface();
      } else {
        nsIWidget* widget = BasicManager()->GetRetainerWidget();
        if (widget) {
          referenceSurface = widget->GetThebesSurface();
        } else {
          referenceSurface = BasicManager()->GetTarget()->CurrentSurface();
        }
      }
    }
    return referenceSurface->CreateSimilarSurface(
      aType, gfxIntSize(aSize.width, aSize.height));
  }

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

  virtual void
  PaintBuffer(gfxContext* aContext,
              const nsIntRegion& aRegionToDraw,
              const nsIntRegion& aExtendedRegionToDraw,
              const nsIntRegion& aRegionToInvalidate,
              PRBool aDidSelfCopy,
              LayerManager::DrawThebesLayerCallback aCallback,
              void* aCallbackData)
  {
    if (!aCallback) {
      BasicManager()->SetTransactionIncomplete();
      return;
    }
    aCallback(this, aContext, aExtendedRegionToDraw, aRegionToInvalidate,
              aCallbackData);
    
    
    
    
    nsIntRegion tmp;
    tmp.Or(mVisibleRegion, aExtendedRegionToDraw);
    mValidRegion.Or(mValidRegion, tmp);
  }

  Buffer mBuffer;
};







static PRBool
ClipToContain(gfxContext* aContext, const nsIntRect& aRect)
{
  gfxRect userRect(aRect.x, aRect.y, aRect.width, aRect.height);
  gfxRect deviceRect = aContext->UserToDevice(userRect);
  deviceRect.RoundOut();

  gfxMatrix currentMatrix = aContext->CurrentMatrix();
  aContext->IdentityMatrix();
  aContext->NewPath();
  aContext->Rectangle(deviceRect);
  aContext->Clip();
  aContext->SetMatrix(currentMatrix);

  return aContext->DeviceToUser(deviceRect).IsEqualInterior(userRect);
}

static nsIntRegion
IntersectWithClip(const nsIntRegion& aRegion, gfxContext* aContext)
{
  gfxRect clip = aContext->GetClipExtents();
  clip.RoundOut();
  nsIntRect r(clip.X(), clip.Y(), clip.Width(), clip.Height());
  nsIntRegion result;
  result.And(aRegion, r);
  return result;
}

static void
SetAntialiasingFlags(Layer* aLayer, gfxContext* aTarget)
{
  nsRefPtr<gfxASurface> surface = aTarget->CurrentSurface();
  if (surface->GetContentType() != gfxASurface::CONTENT_COLOR_ALPHA) {
    
    return;
  }

  const nsIntRect& bounds = aLayer->GetVisibleRegion().GetBounds();
  surface->SetSubpixelAntialiasingEnabled(
      !(aLayer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA) ||
      surface->GetOpaqueRect().Contains(
        aTarget->UserToDevice(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height))));
}

static PRBool
PushGroupForLayer(gfxContext* aContext, Layer* aLayer, const nsIntRegion& aRegion)
{
  
  
  PRBool didCompleteClip = ClipToContain(aContext, aRegion.GetBounds());

  gfxASurface::gfxContentType contentType = gfxASurface::CONTENT_COLOR_ALPHA;
  PRBool needsClipToVisibleRegion = PR_FALSE;
  if (aLayer->CanUseOpaqueSurface() &&
      ((didCompleteClip && aRegion.GetNumRects() == 1) ||
       !aContext->CurrentMatrix().HasNonIntegerTranslation())) {
    
    
    
    
    needsClipToVisibleRegion = !didCompleteClip || aRegion.GetNumRects() > 1;
    contentType = gfxASurface::CONTENT_COLOR;
  }
  aContext->PushGroupAndCopyBackground(contentType);
  return needsClipToVisibleRegion;
}

void
BasicThebesLayer::PaintThebes(gfxContext* aContext,
                              LayerManager::DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              ReadbackProcessor* aReadback)
{
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  nsRefPtr<gfxASurface> targetSurface = aContext->CurrentSurface();

  nsTArray<ReadbackProcessor::Update> readbackUpdates;
  if (aReadback && UsedForReadback()) {
    aReadback->GetThebesLayerUpdates(this, &readbackUpdates);
  }

  PRBool canUseOpaqueSurface = CanUseOpaqueSurface();
  Buffer::ContentType contentType =
    canUseOpaqueSurface ? gfxASurface::CONTENT_COLOR :
                          gfxASurface::CONTENT_COLOR_ALPHA;
  float opacity = GetEffectiveOpacity();

  if (!BasicManager()->IsRetained() ||
      (!canUseOpaqueSurface &&
       (mContentFlags & CONTENT_COMPONENT_ALPHA) &&
       !MustRetainContent())) {
    NS_ASSERTION(readbackUpdates.IsEmpty(), "Can't do readback for non-retained layer");

    mValidRegion.SetEmpty();
    mBuffer.Clear();

    nsIntRegion toDraw = IntersectWithClip(mVisibleRegion, aContext);
    if (!toDraw.IsEmpty()) {
      if (!aCallback) {
        BasicManager()->SetTransactionIncomplete();
        return;
      }

      aContext->Save();

      PRBool needsClipToVisibleRegion = PR_FALSE;
      if (opacity != 1.0) {
        needsClipToVisibleRegion = PushGroupForLayer(aContext, this, toDraw);
      }
      SetAntialiasingFlags(this, aContext);
      aCallback(this, aContext, toDraw, nsIntRegion(), aCallbackData);
      if (opacity != 1.0) {
        aContext->PopGroupToSource();
        if (needsClipToVisibleRegion) {
          gfxUtils::ClipToRegion(aContext, toDraw);
        }
        aContext->Paint(opacity);
      }

      aContext->Restore();
    }
    return;
  }

  {
    gfxSize scale = aContext->CurrentMatrix().ScaleFactors(PR_TRUE);
    float paintXRes = BasicManager()->XResolution() * gfxUtils::ClampToScaleFactor(scale.width);
    float paintYRes = BasicManager()->YResolution() * gfxUtils::ClampToScaleFactor(scale.height);
    PRUint32 flags = 0;
    gfxMatrix transform;
    if (!GetEffectiveTransform().Is2D(&transform) ||
        transform.HasNonIntegerTranslation() ||
        MustRetainContent() ) {
      flags |= ThebesLayerBuffer::PAINT_WILL_RESAMPLE;
    }
    Buffer::PaintState state =
      mBuffer.BeginPaint(this, contentType, paintXRes, paintYRes, flags);
    mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

    if (state.mContext) {
      
      
      
      
      state.mRegionToInvalidate.And(state.mRegionToInvalidate, mVisibleRegion);
      nsIntRegion extendedDrawRegion = state.mRegionToDraw;
      extendedDrawRegion.ExtendForScaling(paintXRes, paintYRes);
      mXResolution = paintXRes;
      mYResolution = paintYRes;
      SetAntialiasingFlags(this, state.mContext);
      PaintBuffer(state.mContext,
                  state.mRegionToDraw, extendedDrawRegion, state.mRegionToInvalidate,
                  state.mDidSelfCopy,
                  aCallback, aCallbackData);
      Mutated();
    } else {
      
      
      NS_ASSERTION(state.mRegionToDraw.IsEmpty(),
                   "If we need to draw, we should have a context");
    }
  }

  mBuffer.DrawTo(this, aContext, opacity);

  for (PRUint32 i = 0; i < readbackUpdates.Length(); ++i) {
    ReadbackProcessor::Update& update = readbackUpdates[i];
    nsIntPoint offset = update.mLayer->GetBackgroundLayerOffset();
    nsRefPtr<gfxContext> ctx =
      update.mLayer->GetSink()->BeginUpdate(update.mUpdateRect + offset,
                                            update.mSequenceCounter);
    if (ctx) {
      NS_ASSERTION(opacity == 1.0, "Should only read back opaque layers");
      ctx->Translate(gfxPoint(offset.x, offset.y));
      mBuffer.DrawTo(this, ctx, 1.0);
      update.mLayer->GetSink()->EndUpdate(ctx, update.mUpdateRect + offset);
    }
  }
}

static PRBool
IsClippingCheap(gfxContext* aTarget, const nsIntRegion& aRegion)
{
  
  
  return !aTarget->CurrentMatrix().HasNonIntegerTranslation() &&
         aRegion.GetNumRects() <= 1; 
}

void
BasicThebesLayerBuffer::DrawTo(ThebesLayer* aLayer,
                               gfxContext* aTarget,
                               float aOpacity)
{
  aTarget->Save();
  
  
  
  if (!aLayer->GetValidRegion().Contains(BufferRect()) ||
      IsClippingCheap(aTarget, aLayer->GetVisibleRegion())) {
    
    
    
    
    
    gfxUtils::ClipToRegionSnapped(aTarget, aLayer->GetVisibleRegion());
  }
  DrawBufferWithRotation(aTarget, aOpacity,
                         aLayer->GetXResolution(), aLayer->GetYResolution());
  aTarget->Restore();
}

already_AddRefed<gfxASurface>
BasicThebesLayerBuffer::CreateBuffer(ContentType aType, 
                                     const nsIntSize& aSize, PRUint32 aFlags)
{
  return mLayer->CreateBuffer(aType, aSize);
}

void
BasicThebesLayerBuffer::SetBackingBufferAndUpdateFrom(
  gfxASurface* aBuffer,
  gfxASurface* aSource, const nsIntRect& aRect, const nsIntPoint& aRotation,
  const nsIntRegion& aUpdateRegion, float aXResolution, float aYResolution)
{
  SetBackingBuffer(aBuffer, aRect, aRotation);
  nsRefPtr<gfxContext> destCtx =
    GetContextForQuadrantUpdate(aUpdateRegion.GetBounds(),
                                aXResolution, aYResolution);
  destCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
  if (IsClippingCheap(destCtx, aUpdateRegion)) {
    gfxUtils::ClipToRegion(destCtx, aUpdateRegion);
  }

  BasicThebesLayerBuffer srcBuffer(aSource, aRect, aRotation);
  srcBuffer.DrawBufferWithRotation(destCtx, 1.0, aXResolution, aYResolution);
}

class BasicImageLayer : public ImageLayer, BasicImplData {
public:
  BasicImageLayer(BasicLayerManager* aLayerManager) :
    ImageLayer(aLayerManager, static_cast<BasicImplData*>(this)),
    mSize(-1, -1)
  {
    MOZ_COUNT_CTOR(BasicImageLayer);
  }
  virtual ~BasicImageLayer()
  {
    MOZ_COUNT_DTOR(BasicImageLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ImageLayer::SetVisibleRegion(aRegion);
  }

  virtual void Paint(gfxContext* aContext);

  static void PaintContext(gfxPattern* aPattern,
                           const nsIntRegion& aVisible,
                           const nsIntRect* aTileSourceRect,
                           float aOpacity,
                           gfxContext* aContext);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

  already_AddRefed<gfxPattern>
  GetAndPaintCurrentImage(gfxContext* aContext,
                          float aOpacity);

  gfxIntSize mSize;
};

void
BasicImageLayer::Paint(gfxContext* aContext)
{
  nsRefPtr<gfxPattern> dontcare =
      GetAndPaintCurrentImage(aContext, GetEffectiveOpacity());
}

already_AddRefed<gfxPattern>
BasicImageLayer::GetAndPaintCurrentImage(gfxContext* aContext,
                                         float aOpacity)
{
  if (!mContainer)
    return nsnull;

  nsRefPtr<Image> image = mContainer->GetCurrentImage();

  nsRefPtr<gfxASurface> surface = mContainer->GetCurrentAsSurface(&mSize);
  if (!surface) {
    return nsnull;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);
  if (!pat) {
    return nsnull;
  }

  pat->SetFilter(mFilter);

  
  
  
  const nsIntRect* tileSrcRect = GetTileSourceRect();
  PaintContext(pat,
               tileSrcRect ? GetVisibleRegion() : nsIntRegion(nsIntRect(0, 0, mSize.width, mSize.height)),
               tileSrcRect,
               aOpacity, aContext);

  GetContainer()->NotifyPaintedImage(image);

  return pat.forget();
}

 void
BasicImageLayer::PaintContext(gfxPattern* aPattern,
                              const nsIntRegion& aVisible,
                              const nsIntRect* aTileSourceRect,
                              float aOpacity,
                              gfxContext* aContext)
{
  
  
  gfxPattern::GraphicsExtend extend = gfxPattern::EXTEND_PAD;

  
  
  nsRefPtr<gfxASurface> target = aContext->CurrentSurface();
  gfxASurface::gfxSurfaceType type = target->GetType();
  if (type == gfxASurface::SurfaceTypeXlib ||
      type == gfxASurface::SurfaceTypeXcb ||
      type == gfxASurface::SurfaceTypeQuartz) {
    extend = gfxPattern::EXTEND_NONE;
  }

  if (!aTileSourceRect) {
    aContext->NewPath();
    
    
    gfxUtils::PathFromRegion(aContext, aVisible);
    aPattern->SetExtend(extend);
    aContext->SetPattern(aPattern);
    aContext->FillWithOpacity(aOpacity);
  } else {
    nsRefPtr<gfxASurface> source = aPattern->GetSurface();
    NS_ABORT_IF_FALSE(source, "Expecting a surface pattern");
    gfxIntSize sourceSize = source->GetSize();
    nsIntRect sourceRect(0, 0, sourceSize.width, sourceSize.height);
    NS_ABORT_IF_FALSE(sourceRect == *aTileSourceRect,
                      "Cowardly refusing to create a temporary surface for tiling");

    gfxContextAutoSaveRestore saveRestore(aContext);

    aContext->NewPath();
    gfxUtils::PathFromRegion(aContext, aVisible);

    aPattern->SetExtend(gfxPattern::EXTEND_REPEAT);
    aContext->SetPattern(aPattern);
    aContext->FillWithOpacity(aOpacity);
  }

  
  aPattern->SetExtend(extend);
}

class BasicColorLayer : public ColorLayer, BasicImplData {
public:
  BasicColorLayer(BasicLayerManager* aLayerManager) :
    ColorLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicColorLayer);
  }
  virtual ~BasicColorLayer()
  {
    MOZ_COUNT_DTOR(BasicColorLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ColorLayer::SetVisibleRegion(aRegion);
  }

  virtual void Paint(gfxContext* aContext)
  {
    PaintColorTo(mColor, GetEffectiveOpacity(), aContext);
  }

  static void PaintColorTo(gfxRGBA aColor, float aOpacity,
                           gfxContext* aContext);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

 void
BasicColorLayer::PaintColorTo(gfxRGBA aColor, float aOpacity,
                              gfxContext* aContext)
{
  aContext->SetColor(aColor);
  aContext->Paint(aOpacity);
}

class BasicCanvasLayer : public CanvasLayer,
                         BasicImplData
{
public:
  BasicCanvasLayer(BasicLayerManager* aLayerManager) :
    CanvasLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicCanvasLayer);
  }
  virtual ~BasicCanvasLayer()
  {
    MOZ_COUNT_DTOR(BasicCanvasLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    CanvasLayer::SetVisibleRegion(aRegion);
  }

  virtual void Initialize(const Data& aData);
  virtual void Paint(gfxContext* aContext);

  virtual void PaintWithOpacity(gfxContext* aContext,
                                float aOpacity);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
  void UpdateSurface();

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<mozilla::gl::GLContext> mGLContext;
  PRUint32 mCanvasFramebuffer;

  PRPackedBool mGLBufferIsPremultiplied;
  PRPackedBool mNeedsYFlip;
};

void
BasicCanvasLayer::Initialize(const Data& aData)
{
  NS_ASSERTION(mSurface == nsnull, "BasicCanvasLayer::Initialize called twice!");

  if (aData.mSurface) {
    mSurface = aData.mSurface;
    NS_ASSERTION(aData.mGLContext == nsnull,
                 "CanvasLayer can't have both surface and GLContext");
    mNeedsYFlip = PR_FALSE;
  } else if (aData.mGLContext) {
    NS_ASSERTION(aData.mGLContext->IsOffscreen(), "canvas gl context isn't offscreen");
    mGLContext = aData.mGLContext;
    mGLBufferIsPremultiplied = aData.mGLBufferIsPremultiplied;
    mCanvasFramebuffer = mGLContext->GetOffscreenFBO();
    mNeedsYFlip = PR_TRUE;
  } else {
    NS_ERROR("CanvasLayer created without mSurface or mGLContext?");
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);
}

void
BasicCanvasLayer::UpdateSurface()
{
  if (!mDirty)
    return;
  mDirty = PR_FALSE;

  if (mGLContext) {
    nsRefPtr<gfxImageSurface> isurf =
      new gfxImageSurface(gfxIntSize(mBounds.width, mBounds.height),
                          (GetContentFlags() & CONTENT_OPAQUE)
                            ? gfxASurface::ImageFormatRGB24
                            : gfxASurface::ImageFormatARGB32);
    if (!isurf || isurf->CairoStatus() != 0) {
      return;
    }

    NS_ASSERTION(isurf->Stride() == mBounds.width * 4, "gfxImageSurface stride isn't what we expect!");

    
    mGLContext->MakeCurrent();

    
    
    mGLContext->fFlush();

    PRUint32 currentFramebuffer = 0;

    mGLContext->fGetIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, (GLint*)&currentFramebuffer);

    
    
    if (currentFramebuffer != mCanvasFramebuffer)
      mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mCanvasFramebuffer);

    mGLContext->ReadPixelsIntoImageSurface(0, 0,
                                           mBounds.width, mBounds.height,
                                           isurf);

    
    if (currentFramebuffer != mCanvasFramebuffer)
      mGLContext->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, currentFramebuffer);

    
    
    
    
    if (!mGLBufferIsPremultiplied)
      gfxUtils::PremultiplyImageSurface(isurf);

    
    mSurface = isurf;
  }
}

void
BasicCanvasLayer::Paint(gfxContext* aContext)
{
  UpdateSurface();
  FireDidTransactionCallback();
  PaintWithOpacity(aContext, GetEffectiveOpacity());
}

void
BasicCanvasLayer::PaintWithOpacity(gfxContext* aContext,
                                   float aOpacity)
{
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");

  nsRefPtr<gfxPattern> pat = new gfxPattern(mSurface);

  pat->SetFilter(mFilter);
  pat->SetExtend(gfxPattern::EXTEND_PAD);

  gfxMatrix m;
  if (mNeedsYFlip) {
    m = aContext->CurrentMatrix();
    aContext->Translate(gfxPoint(0.0, mBounds.height));
    aContext->Scale(1.0, -1.0);
  }

  aContext->NewPath();
  
  aContext->Rectangle(gfxRect(0, 0, mBounds.width, mBounds.height));
  aContext->SetPattern(pat);
  aContext->FillWithOpacity(aOpacity);

  if (mNeedsYFlip) {
    aContext->SetMatrix(m);
  }
}

class BasicReadbackLayer : public ReadbackLayer,
                           BasicImplData
{
public:
  BasicReadbackLayer(BasicLayerManager* aLayerManager) :
    ReadbackLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicReadbackLayer);
  }
  virtual ~BasicReadbackLayer()
  {
    MOZ_COUNT_DTOR(BasicReadbackLayer);
  }

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ReadbackLayer::SetVisibleRegion(aRegion);
  }

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

static nsIntRect
ToOutsideIntRect(const gfxRect &aRect)
{
  gfxRect r = aRect;
  r.RoundOut();
  return nsIntRect(r.X(), r.Y(), r.Width(), r.Height());
}

static nsIntRect
ToInsideIntRect(const gfxRect& aRect)
{
  gfxRect r = aRect;
  r.RoundIn();
  return nsIntRect(r.X(), r.Y(), r.Width(), r.Height());
}







static PRBool
MayHaveOverlappingOrTransparentLayers(Layer* aLayer,
                                      const nsIntRect& aBounds,
                                      nsIntRegion* aDirtyVisibleRegionInContainer)
{
  if (!(aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE)) {
    return PR_TRUE;
  }

  gfxMatrix matrix;
  if (!aLayer->GetTransform().Is2D(&matrix) ||
      matrix.HasNonIntegerTranslation()) {
    return PR_TRUE;
  }

  nsIntPoint translation = nsIntPoint(PRInt32(matrix.x0), PRInt32(matrix.y0));
  nsIntRect bounds = aBounds - translation;

  nsIntRect clippedDirtyRect = bounds;
  const nsIntRect* clipRect = aLayer->GetClipRect();
  if (clipRect) {
    clippedDirtyRect.IntersectRect(clippedDirtyRect, *clipRect - translation);
  }
  aDirtyVisibleRegionInContainer->And(aLayer->GetVisibleRegion(), clippedDirtyRect);
  aDirtyVisibleRegionInContainer->MoveBy(translation);

  
  if (aDirtyVisibleRegionInContainer->IsEmpty()) {
    return PR_FALSE;
  }

  nsIntRegion region;

  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    nsIntRegion childRegion;
    if (MayHaveOverlappingOrTransparentLayers(child, bounds, &childRegion)) {
      return PR_TRUE;
    }

    nsIntRegion tmp;
    tmp.And(region, childRegion);
    if (!tmp.IsEmpty()) {
      return PR_TRUE;
    }

    region.Or(region, childRegion);
  }

  return PR_FALSE;
}

BasicLayerManager::BasicLayerManager(nsIWidget* aWidget) :
#ifdef DEBUG
  mPhase(PHASE_NONE),
#endif
  mXResolution(1.0)
  , mYResolution(1.0)
  , mWidget(aWidget)
  , mDoubleBuffering(BUFFER_NONE), mUsingDefaultTarget(PR_FALSE)
  , mTransactionIncomplete(false)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
  NS_ASSERTION(aWidget, "Must provide a widget");
}

BasicLayerManager::BasicLayerManager() :
#ifdef DEBUG
  mPhase(PHASE_NONE),
#endif
  mWidget(nsnull)
  , mDoubleBuffering(BUFFER_NONE), mUsingDefaultTarget(PR_FALSE)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
}

BasicLayerManager::~BasicLayerManager()
{
  NS_ASSERTION(!InTransaction(), "Died during transaction?");

  ClearCachedResources();

  mRoot = nsnull;

  MOZ_COUNT_DTOR(BasicLayerManager);
}

void
BasicLayerManager::SetDefaultTarget(gfxContext* aContext,
                                    BufferMode aDoubleBuffering)
{
  NS_ASSERTION(!InTransaction(),
               "Must set default target outside transaction");
  mDefaultTarget = aContext;
  mDoubleBuffering = aDoubleBuffering;
}

void
BasicLayerManager::BeginTransaction()
{
  mUsingDefaultTarget = PR_TRUE;
  BeginTransactionWithTarget(mDefaultTarget);
}

already_AddRefed<gfxContext>
BasicLayerManager::PushGroupWithCachedSurface(gfxContext *aTarget,
                                              gfxASurface::gfxContentType aContent,
                                              gfxPoint *aSavedOffset)
{
  gfxContextMatrixAutoSaveRestore saveMatrix(aTarget);
  aTarget->IdentityMatrix();

  nsRefPtr<gfxASurface> currentSurf = aTarget->CurrentSurface();
  gfxRect clip = aTarget->GetClipExtents();
  clip.RoundOut();

  nsRefPtr<gfxContext> ctx =
    mCachedSurface.Get(aContent,
                       gfxIntSize(clip.Width(), clip.Height()),
                       currentSurf);
  
  ctx->Translate(-clip.TopLeft());
  *aSavedOffset = clip.TopLeft();
  ctx->Multiply(saveMatrix.Matrix());
  return ctx.forget();
}

void
BasicLayerManager::PopGroupWithCachedSurface(gfxContext *aTarget,
                                             const gfxPoint& aSavedOffset)
{
  if (!mTarget)
    return;

  gfxContextMatrixAutoSaveRestore saveMatrix(aTarget);
  aTarget->IdentityMatrix();

  aTarget->SetSource(mTarget->OriginalSurface(), aSavedOffset);
  aTarget->Paint();
}

void
BasicLayerManager::BeginTransactionWithTarget(gfxContext* aTarget)
{
#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("[----- BeginTransaction"));
  Log();
#endif

  NS_ASSERTION(!InTransaction(), "Nested transactions not allowed");
#ifdef DEBUG
  mPhase = PHASE_CONSTRUCTION;
#endif
  mTarget = aTarget;
}

static void
TransformIntRect(nsIntRect& aRect, const gfxMatrix& aMatrix,
                 nsIntRect (*aRoundMethod)(const gfxRect&))
{
  gfxRect gr = gfxRect(aRect.x, aRect.y, aRect.width, aRect.height);
  gr = aMatrix.TransformBounds(gr);
  aRect = (*aRoundMethod)(gr);
}





static void
MarkLeafLayersCoveredByOpaque(Layer* aLayer, const nsIntRect& aClipRect,
                              nsIntRegion& aRegion)
{
  Layer* child = aLayer->GetLastChild();
  BasicImplData* data = ToData(aLayer);
  data->SetCoveredByOpaque(PR_FALSE);

  nsIntRect newClipRect(aClipRect);

  
  
  
  if (aLayer->GetOpacity() != 1.0f) {
    newClipRect.SetRect(0, 0, 0, 0);
  }

  {
    const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
    if (clipRect) {
      nsIntRect cr = *clipRect;
      
      
      if (aLayer->GetParent()) {
        gfxMatrix tr;
        if (aLayer->GetParent()->GetEffectiveTransform().Is2D(&tr)) {
          TransformIntRect(cr, tr, ToInsideIntRect);
        } else {
          cr.SetRect(0, 0, 0, 0);
        }
      }
      newClipRect.IntersectRect(newClipRect, cr);
    }
  }

  if (!child) {
    gfxMatrix transform;
    if (!aLayer->GetEffectiveTransform().Is2D(&transform)) {
      return;
    }

    nsIntRegion region = aLayer->GetEffectiveVisibleRegion();
    nsIntRect r = region.GetBounds();
    TransformIntRect(r, transform, ToOutsideIntRect);
    data->SetCoveredByOpaque(aRegion.Contains(r));

    
    
    if (!(aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE)) {
      return;
    }

    nsIntRegionRectIterator it(region);
    while (const nsIntRect* sr = it.Next()) {
      r = *sr;
      TransformIntRect(r, transform, ToInsideIntRect);

      r.IntersectRect(r, newClipRect);
      if (!r.IsEmpty()) {
        aRegion.Or(aRegion, r);
      }
    }
  } else {
    for (; child; child = child->GetPrevSibling()) {
      MarkLeafLayersCoveredByOpaque(child, newClipRect, aRegion);
    }
  }
}

void
BasicLayerManager::EndTransaction(DrawThebesLayerCallback aCallback,
                                  void* aCallbackData)
{
  EndTransactionInternal(aCallback, aCallbackData);
}

bool
BasicLayerManager::EndTransactionInternal(DrawThebesLayerCallback aCallback,
                                          void* aCallbackData)
{
#ifdef MOZ_LAYERS_HAVE_LOG
  MOZ_LAYERS_LOG(("  ----- (beginning paint)"));
  Log();
#endif

  NS_ASSERTION(InConstruction(), "Should be in construction phase");
#ifdef DEBUG
  mPhase = PHASE_DRAWING;
#endif

  mTransactionIncomplete = false;

  if (mTarget && mRoot) {
    nsRefPtr<gfxContext> finalTarget = mTarget;
    gfxPoint cachedSurfaceOffset;

    nsIntRegion rootRegion;
    PRBool useDoubleBuffering = mUsingDefaultTarget &&
      mDoubleBuffering != BUFFER_NONE &&
      MayHaveOverlappingOrTransparentLayers(mRoot,
                                            ToOutsideIntRect(mTarget->GetClipExtents()),
                                            &rootRegion);
    if (useDoubleBuffering) {
      nsRefPtr<gfxASurface> targetSurface = mTarget->CurrentSurface();
      mTarget = PushGroupWithCachedSurface(mTarget, targetSurface->GetContentType(),
                                           &cachedSurfaceOffset);
    }

    mSnapEffectiveTransforms =
      !(mTarget->GetFlags() & gfxContext::FLAG_DISABLE_SNAPPING);
    mRoot->ComputeEffectiveTransforms(gfx3DMatrix::From2D(mTarget->CurrentMatrix()));

    nsIntRegion region;
    MarkLeafLayersCoveredByOpaque(mRoot,
                                  mRoot->GetEffectiveVisibleRegion().GetBounds(),
                                  region);
    PaintLayer(mRoot, aCallback, aCallbackData, nsnull);

    
    
    
    
    
    
    
    
    if (useDoubleBuffering && !mTransactionIncomplete) {
      finalTarget->SetOperator(gfxContext::OPERATOR_SOURCE);
      PopGroupWithCachedSurface(finalTarget, cachedSurfaceOffset);
    }

    if (!mTransactionIncomplete) {
      
      mTarget = nsnull;
    } else {
      
      mTarget = finalTarget;
    }
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  Log();
  MOZ_LAYERS_LOG(("]----- EndTransaction"));
#endif

#ifdef DEBUG
  
  
  mPhase = mTransactionIncomplete ? PHASE_CONSTRUCTION : PHASE_NONE;
#endif

  if (!mTransactionIncomplete) {
    
    mUsingDefaultTarget = PR_FALSE;
  }

  NS_ASSERTION(!aCallback || !mTransactionIncomplete,
               "If callback is not null, transaction must be complete");

  
  

  return !mTransactionIncomplete;
}

bool
BasicLayerManager::EndEmptyTransaction()
{
  if (!mRoot) {
    return false;
  }

  return EndTransactionInternal(nsnull, nsnull);
}

void
BasicLayerManager::SetRoot(Layer* aLayer)
{
  NS_ASSERTION(aLayer, "Root can't be null");
  NS_ASSERTION(aLayer->Manager() == this, "Wrong manager");
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  mRoot = aLayer;
}

void
BasicLayerManager::PaintLayer(Layer* aLayer,
                              DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              ReadbackProcessor* aReadback)
{
  const nsIntRect* clipRect = aLayer->GetEffectiveClipRect();
  const gfx3DMatrix& effectiveTransform = aLayer->GetEffectiveTransform();
  PRBool needsGroup = aLayer->GetFirstChild() &&
      static_cast<BasicContainerLayer*>(aLayer)->UseIntermediateSurface();
  
  
  PRBool needsSaveRestore = needsGroup || clipRect;

  gfxMatrix savedMatrix;
  if (needsSaveRestore) {
    mTarget->Save();

    if (clipRect) {
      mTarget->NewPath();
      mTarget->Rectangle(gfxRect(clipRect->x, clipRect->y, clipRect->width, clipRect->height), PR_TRUE);
      mTarget->Clip();
    }
  } else {
    savedMatrix = mTarget->CurrentMatrix();
  }

  gfxMatrix transform;
  
  
  NS_ASSERTION(effectiveTransform.Is2D(),
               "Only 2D transforms supported currently");
  effectiveTransform.Is2D(&transform);
  mTarget->SetMatrix(transform);

  PRBool pushedTargetOpaqueRect = PR_FALSE;
  const nsIntRegion& visibleRegion = aLayer->GetEffectiveVisibleRegion();
  nsRefPtr<gfxASurface> currentSurface = mTarget->CurrentSurface();
  const gfxRect& targetOpaqueRect = currentSurface->GetOpaqueRect();

  
  
  if (targetOpaqueRect.IsEmpty() && visibleRegion.GetNumRects() == 1 &&
      (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
      !transform.HasNonAxisAlignedTransform()) {
    const nsIntRect& bounds = visibleRegion.GetBounds();
    currentSurface->SetOpaqueRect(
        mTarget->UserToDevice(gfxRect(bounds.x, bounds.y, bounds.width, bounds.height)));
    pushedTargetOpaqueRect = PR_TRUE;
  }

  PRBool needsClipToVisibleRegion = PR_FALSE;
  if (needsGroup) {
    needsClipToVisibleRegion =
        PushGroupForLayer(mTarget, aLayer, aLayer->GetEffectiveVisibleRegion());
  }

  
  Layer* child = aLayer->GetFirstChild();
  if (!child) {
    BasicImplData* data = ToData(aLayer);
#ifdef MOZ_LAYERS_HAVE_LOG
    MOZ_LAYERS_LOG(("%s (0x%p) is covered: %i\n", __FUNCTION__,
                   (void*)aLayer, data->IsCoveredByOpaque()));
#endif
    if (!data->IsCoveredByOpaque()) {
      if (aLayer->AsThebesLayer()) {
        data->PaintThebes(mTarget, aCallback, aCallbackData, aReadback);
      } else {
        data->Paint(mTarget);
      }
    }
  } else {
    ReadbackProcessor readback;
    if (IsRetained()) {
      ContainerLayer* container = static_cast<ContainerLayer*>(aLayer);
      readback.BuildUpdates(container);
    }

    for (; child; child = child->GetNextSibling()) {
      PaintLayer(child, aCallback, aCallbackData, &readback);
      if (mTransactionIncomplete)
        break;
    }
  }

  if (needsGroup) {
    mTarget->PopGroupToSource();
    if (needsClipToVisibleRegion) {
      gfxUtils::ClipToRegion(mTarget, aLayer->GetEffectiveVisibleRegion());
    }
    mTarget->Paint(aLayer->GetEffectiveOpacity());
  }

  if (pushedTargetOpaqueRect) {
    currentSurface->SetOpaqueRect(gfxRect(0, 0, 0, 0));
  }

  if (needsSaveRestore) {
    mTarget->Restore();
  } else {
    mTarget->SetMatrix(savedMatrix);
  }
}

void
BasicLayerManager::ClearCachedResources()
{
  if (mRoot) {
    ClearLayer(mRoot);
  }

  mCachedSurface.Expire();
}
void
BasicLayerManager::ClearLayer(Layer* aLayer)
{
  ToData(aLayer)->ClearCachedResources();
  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    ClearLayer(child);
  }
}

already_AddRefed<ThebesLayer>
BasicLayerManager::CreateThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ThebesLayer> layer = new BasicThebesLayer(this);
  return layer.forget();
}

already_AddRefed<ContainerLayer>
BasicLayerManager::CreateContainerLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ContainerLayer> layer = new BasicContainerLayer(this);
  return layer.forget();
}

already_AddRefed<ImageLayer>
BasicLayerManager::CreateImageLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ImageLayer> layer = new BasicImageLayer(this);
  return layer.forget();
}

already_AddRefed<ColorLayer>
BasicLayerManager::CreateColorLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ColorLayer> layer = new BasicColorLayer(this);
  return layer.forget();
}

already_AddRefed<CanvasLayer>
BasicLayerManager::CreateCanvasLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<CanvasLayer> layer = new BasicCanvasLayer(this);
  return layer.forget();
}

already_AddRefed<ReadbackLayer>
BasicLayerManager::CreateReadbackLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ReadbackLayer> layer = new BasicReadbackLayer(this);
  return layer.forget();
}

class BasicShadowableThebesLayer;
class BasicShadowableLayer : public ShadowableLayer
{
public:
  BasicShadowableLayer()
  {
    MOZ_COUNT_CTOR(BasicShadowableLayer);
  }

  ~BasicShadowableLayer()
  {
    if (HasShadow()) {
      PLayerChild::Send__delete__(GetShadow());
    }
    MOZ_COUNT_DTOR(BasicShadowableLayer);
  }

  void SetShadow(PLayerChild* aShadow)
  {
    NS_ABORT_IF_FALSE(!mShadow, "can't have two shadows (yet)");
    mShadow = aShadow;
  }

  virtual void SetBackBuffer(const SurfaceDescriptor& aBuffer)
  {
    NS_RUNTIMEABORT("if this default impl is called, |aBuffer| leaks");
  }
  
  virtual void SetBackBufferYUVImage(gfxSharedImageSurface* aYBuffer,
                                     gfxSharedImageSurface* aUBuffer,
                                     gfxSharedImageSurface* aVBuffer)
  {
    NS_RUNTIMEABORT("if this default impl is called, |aBuffer| leaks");
  }

  virtual void Disconnect()
  {
    
    
    
    
    
    mShadow = nsnull;
  }

  virtual BasicShadowableThebesLayer* AsThebes() { return nsnull; }
};

static ShadowableLayer*
ToShadowable(Layer* aLayer)
{
  return ToData(aLayer)->AsShadowableLayer();
}



static bool
ShouldShadow(Layer* aLayer)
{
  if (!ToShadowable(aLayer)) {
    NS_ABORT_IF_FALSE(aLayer->GetType() == Layer::TYPE_READBACK,
                      "Only expect not to shadow ReadbackLayers");
    return false;
  }
  return true;
}

template<class OpT>
static BasicShadowableLayer*
GetBasicShadowable(const OpT& op)
{
  return static_cast<BasicShadowableLayer*>(
    static_cast<const ShadowLayerChild*>(op.layerChild())->layer());
}

class BasicShadowableContainerLayer : public BasicContainerLayer,
                                      public BasicShadowableLayer {
public:
  BasicShadowableContainerLayer(BasicShadowLayerManager* aManager) :
    BasicContainerLayer(aManager)
  {
    MOZ_COUNT_CTOR(BasicShadowableContainerLayer);
  }
  virtual ~BasicShadowableContainerLayer()
  {
    MOZ_COUNT_DTOR(BasicShadowableContainerLayer);
  }

  virtual void InsertAfter(Layer* aChild, Layer* aAfter);
  virtual void RemoveChild(Layer* aChild);

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ContainerLayerAttributes(GetFrameMetrics());
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect()
  {
    BasicShadowableLayer::Disconnect();
  }

private:
  BasicShadowLayerManager* ShadowManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }
};

void
BasicShadowableContainerLayer::InsertAfter(Layer* aChild, Layer* aAfter)
{
  if (HasShadow() && ShouldShadow(aChild)) {
    while (aAfter && !ShouldShadow(aAfter)) {
      aAfter = aAfter->GetPrevSibling();
    }
    ShadowManager()->InsertAfter(ShadowManager()->Hold(this),
                                 ShadowManager()->Hold(aChild),
                                 aAfter ? ShadowManager()->Hold(aAfter) : nsnull);
  }
  BasicContainerLayer::InsertAfter(aChild, aAfter);
}

void
BasicShadowableContainerLayer::RemoveChild(Layer* aChild)
{
  if (HasShadow() && ShouldShadow(aChild)) {
    ShadowManager()->RemoveChild(ShadowManager()->Hold(this),
                                 ShadowManager()->Hold(aChild));
  }
  BasicContainerLayer::RemoveChild(aChild);
}

class BasicShadowableThebesLayer : public BasicThebesLayer,
                                   public BasicShadowableLayer
{
  typedef BasicThebesLayer Base;

public:
  BasicShadowableThebesLayer(BasicShadowLayerManager* aManager)
    : BasicThebesLayer(aManager)
    , mIsNewBuffer(false)
  {
    MOZ_COUNT_CTOR(BasicShadowableThebesLayer);
  }
  virtual ~BasicShadowableThebesLayer()
  {
    if (IsSurfaceDescriptorValid(mBackBuffer))
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBuffer);
    MOZ_COUNT_DTOR(BasicShadowableThebesLayer);
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ThebesLayerAttributes(GetValidRegion(),
                                   mXResolution, mYResolution);
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }
  virtual bool MustRetainContent() { return HasShadow(); }

  void SetBackBufferAndAttrs(const ThebesBuffer& aBuffer,
                             const nsIntRegion& aValidRegion,
                             float aXResolution, float aYResolution,
                             const OptionalThebesBuffer& aReadOnlyFrontBuffer,
                             const nsIntRegion& aFrontUpdatedRegion);

  virtual void Disconnect()
  {
    mBackBuffer = SurfaceDescriptor();
    BasicShadowableLayer::Disconnect();
  }

  virtual BasicShadowableThebesLayer* AsThebes() { return this; }

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  NS_OVERRIDE virtual void
  PaintBuffer(gfxContext* aContext,
              const nsIntRegion& aRegionToDraw,
              const nsIntRegion& aExtendedRegionToDraw,
              const nsIntRegion& aRegionToInvalidate,
              PRBool aDidSelfCopy,
              LayerManager::DrawThebesLayerCallback aCallback,
              void* aCallbackData);

  NS_OVERRIDE virtual already_AddRefed<gfxASurface>
  CreateBuffer(Buffer::ContentType aType, const nsIntSize& aSize);

  
  
  
  SurfaceDescriptor mBackBuffer;

  PRPackedBool mIsNewBuffer;
};

void
BasicShadowableThebesLayer::SetBackBufferAndAttrs(const ThebesBuffer& aBuffer,
                                                  const nsIntRegion& aValidRegion,
                                                  float aXResolution,
                                                  float aYResolution,
                                                  const OptionalThebesBuffer& aReadOnlyFrontBuffer,
                                                  const nsIntRegion& aFrontUpdatedRegion)
{
  mBackBuffer = aBuffer.buffer();
  nsRefPtr<gfxASurface> backBuffer = BasicManager()->OpenDescriptor(mBackBuffer);

  if (OptionalThebesBuffer::Tnull_t == aReadOnlyFrontBuffer.type()) {
    
    
    
    
    mValidRegion = aValidRegion;
    mXResolution = aXResolution;
    mYResolution = aYResolution;
    mBuffer.SetBackingBuffer(backBuffer, aBuffer.rect(), aBuffer.rotation());
    return;
  }

  MOZ_LAYERS_LOG(("BasicShadowableThebes(%p): reading back <x=%d,y=%d,w=%d,h=%d>",
                  this,
                  aFrontUpdatedRegion.GetBounds().x,
                  aFrontUpdatedRegion.GetBounds().y,
                  aFrontUpdatedRegion.GetBounds().width,
                  aFrontUpdatedRegion.GetBounds().height));

  const ThebesBuffer roFront = aReadOnlyFrontBuffer.get_ThebesBuffer();
  nsRefPtr<gfxASurface> roFrontBuffer = BasicManager()->OpenDescriptor(roFront.buffer());
  mBuffer.SetBackingBufferAndUpdateFrom(
    backBuffer,
    roFrontBuffer, roFront.rect(), roFront.rotation(),
    aFrontUpdatedRegion, mXResolution, mYResolution);
  
  
  
}

void
BasicShadowableThebesLayer::PaintBuffer(gfxContext* aContext,
                                        const nsIntRegion& aRegionToDraw,
                                        const nsIntRegion& aExtendedRegionToDraw,
                                        const nsIntRegion& aRegionToInvalidate,
                                        PRBool aDidSelfCopy,
                                        LayerManager::DrawThebesLayerCallback aCallback,
                                        void* aCallbackData)
{
  Base::PaintBuffer(aContext,
                    aRegionToDraw, aExtendedRegionToDraw, aRegionToInvalidate,
                    aDidSelfCopy,
                    aCallback, aCallbackData);
  if (!HasShadow()) {
    return;
  }

  nsIntRegion updatedRegion;
  if (mIsNewBuffer || aDidSelfCopy) {
    
    
    
    
    
    
    
    updatedRegion = mVisibleRegion;
    mIsNewBuffer = false;
  } else {
    updatedRegion = aRegionToDraw;
  }

  NS_ASSERTION(mBuffer.BufferRect().Contains(aRegionToDraw.GetBounds()),
               "Update outside of buffer rect!");
  NS_ABORT_IF_FALSE(IsSurfaceDescriptorValid(mBackBuffer),
                    "should have a back buffer by now");
  BasicManager()->PaintedThebesBuffer(BasicManager()->Hold(this),
                                      updatedRegion,
                                      mBuffer.BufferRect(),
                                      mBuffer.BufferRotation(),
                                      mBackBuffer);
}

already_AddRefed<gfxASurface>
BasicShadowableThebesLayer::CreateBuffer(Buffer::ContentType aType,
                                         const nsIntSize& aSize)
{
  if (!HasShadow()) {
    return BasicThebesLayer::CreateBuffer(aType, aSize);
  }

  MOZ_LAYERS_LOG(("BasicShadowableThebes(%p): creating %d x %d buffer(x2)",
                  this,
                  aSize.width, aSize.height));

  if (IsSurfaceDescriptorValid(mBackBuffer)) {
    BasicManager()->DestroyedThebesBuffer(BasicManager()->Hold(this),
                                          mBackBuffer);
    mBackBuffer = SurfaceDescriptor();
  }

  
  SurfaceDescriptor tmpFront;
  if (BasicManager()->ShouldDoubleBuffer()) {
    if (!BasicManager()->AllocDoubleBuffer(gfxIntSize(aSize.width, aSize.height),
                                           aType,
                                           &tmpFront,
                                           &mBackBuffer)) {
      NS_RUNTIMEABORT("creating ThebesLayer 'back buffer' failed!");
    }
  } else {
    if (!BasicManager()->AllocBuffer(gfxIntSize(aSize.width, aSize.height),
                                     aType,
                                     &mBackBuffer)) {
      NS_RUNTIMEABORT("creating ThebesLayer 'back buffer' failed!");
    }
  }

  NS_ABORT_IF_FALSE(!mIsNewBuffer,
                    "Bad! Did we create a buffer twice without painting?");
  mIsNewBuffer = true;

  BasicManager()->CreatedThebesBuffer(BasicManager()->Hold(this),
                                      nsIntRegion(),
                                      1.0, 1.0,
                                      nsIntRect(),
                                      tmpFront);
  return BasicManager()->OpenDescriptor(mBackBuffer);
}


class BasicShadowableImageLayer : public BasicImageLayer,
                                  public BasicShadowableLayer
{
public:
  BasicShadowableImageLayer(BasicShadowLayerManager* aManager) :
    BasicImageLayer(aManager)
  {
    MOZ_COUNT_CTOR(BasicShadowableImageLayer);
  }
  virtual ~BasicShadowableImageLayer()
  {
    if (IsSurfaceDescriptorValid(mBackBuffer)) {
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBuffer);
    }
    MOZ_COUNT_DTOR(BasicShadowableImageLayer);
  }

  virtual void Paint(gfxContext* aContext);

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ImageLayerAttributes(mFilter);
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void SetBackBuffer(const SurfaceDescriptor& aBuffer)
  {
    mBackBuffer = aBuffer;
  }

  virtual void SetBackBufferYUVImage(gfxSharedImageSurface* aYBuffer,
                                     gfxSharedImageSurface* aUBuffer,
                                     gfxSharedImageSurface* aVBuffer)
  {
    mBackBufferY = aYBuffer;
    mBackBufferU = aUBuffer;
    mBackBufferV = aVBuffer;
  }

  virtual void Disconnect()
  {
    mBackBuffer = SurfaceDescriptor();
    BasicShadowableLayer::Disconnect();
  }

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  
  
  SurfaceDescriptor mBackBuffer;
  nsRefPtr<gfxSharedImageSurface> mBackBufferY;
  nsRefPtr<gfxSharedImageSurface> mBackBufferU;
  nsRefPtr<gfxSharedImageSurface> mBackBufferV;
  gfxIntSize mCbCrSize;
};
 
void
BasicShadowableImageLayer::Paint(gfxContext* aContext)
{
  if (!mContainer) {
    return;
  }

  nsRefPtr<Image> image = mContainer->GetCurrentImage();
  if (!image) {
    return;
  }

  if (image->GetFormat() == Image::PLANAR_YCBCR && BasicManager()->IsCompositingCheap()) {
    PlanarYCbCrImage *YCbCrImage = static_cast<PlanarYCbCrImage*>(image.get());
    const PlanarYCbCrImage::Data *data = YCbCrImage->GetData();
    NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");

    if (mSize != data->mYSize || mCbCrSize != data->mCbCrSize) {

      if (mBackBufferY) {
        BasicManager()->ShadowLayerForwarder::DestroySharedSurface(mBackBufferY);
        BasicManager()->ShadowLayerForwarder::DestroySharedSurface(mBackBufferU);
        BasicManager()->ShadowLayerForwarder::DestroySharedSurface(mBackBufferV);
        BasicManager()->DestroyedImageBuffer(BasicManager()->Hold(this));
      }
      mSize = data->mYSize;
      mCbCrSize = data->mCbCrSize;

      nsRefPtr<gfxSharedImageSurface> tmpYSurface;
      nsRefPtr<gfxSharedImageSurface> tmpUSurface;
      nsRefPtr<gfxSharedImageSurface> tmpVSurface;

      if (!BasicManager()->AllocDoubleBuffer(
            mSize,
            gfxASurface::CONTENT_ALPHA,
            getter_AddRefs(tmpYSurface), getter_AddRefs(mBackBufferY)))
        NS_RUNTIMEABORT("creating ImageLayer 'front buffer' failed!");
      
      if (!BasicManager()->AllocDoubleBuffer(
            mCbCrSize,
            gfxASurface::CONTENT_ALPHA,
            getter_AddRefs(tmpUSurface), getter_AddRefs(mBackBufferU)))
        NS_RUNTIMEABORT("creating ImageLayer 'front buffer' failed!");
      
      if (!BasicManager()->AllocDoubleBuffer(
            mCbCrSize,
            gfxASurface::CONTENT_ALPHA,
            getter_AddRefs(tmpVSurface), getter_AddRefs(mBackBufferV)))
        NS_RUNTIMEABORT("creating ImageLayer 'front buffer' failed!");

      YUVImage yuv(tmpYSurface->GetShmem(),
                   tmpUSurface->GetShmem(),
                   tmpVSurface->GetShmem());

      BasicManager()->CreatedImageBuffer(BasicManager()->Hold(this),
                                         nsIntSize(mSize.width, mSize.height),
                                         yuv);

    }
      
    memcpy(mBackBufferY->Data(), 
           data->mYChannel, 
           data->mYStride * mSize.height);
    memcpy(mBackBufferU->Data(), 
           data->mCbChannel, 
           data->mCbCrStride * mCbCrSize.height);
    memcpy(mBackBufferV->Data(), 
           data->mCrChannel, 
           data->mCbCrStride * mCbCrSize.height);
      
    YUVImage yuv(mBackBufferY->GetShmem(),
                 mBackBufferU->GetShmem(),
                 mBackBufferV->GetShmem());
  
    BasicManager()->PaintedImage(BasicManager()->Hold(this),
                                 yuv);

    return;
  }

  gfxIntSize oldSize = mSize;
  nsRefPtr<gfxPattern> pat = GetAndPaintCurrentImage(aContext, GetEffectiveOpacity());
  if (!pat || !HasShadow())
    return;

  if (oldSize != mSize) {
    if (IsSurfaceDescriptorValid(mBackBuffer)) {
      BasicManager()->DestroyedImageBuffer(BasicManager()->Hold(this));
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBuffer);
    }

    SurfaceDescriptor tmpFrontSurface;
    
    if (!BasicManager()->AllocDoubleBuffer(
          mSize,
          (GetContentFlags() & CONTENT_OPAQUE) ?
            gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA,
          &tmpFrontSurface, &mBackBuffer))
      NS_RUNTIMEABORT("creating ImageLayer 'front buffer' failed!");

    BasicManager()->CreatedImageBuffer(BasicManager()->Hold(this),
                                       nsIntSize(mSize.width, mSize.height),
                                       tmpFrontSurface);
  }

  nsRefPtr<gfxASurface> backSurface =
    BasicManager()->OpenDescriptor(mBackBuffer);
  nsRefPtr<gfxContext> tmpCtx = new gfxContext(backSurface);
  PaintContext(pat,
               nsIntRegion(nsIntRect(0, 0, mSize.width, mSize.height)),
               nsnull, 1.0, tmpCtx);

  BasicManager()->PaintedImage(BasicManager()->Hold(this),
                               mBackBuffer);
}


class BasicShadowableColorLayer : public BasicColorLayer,
                                  public BasicShadowableLayer
{
public:
  BasicShadowableColorLayer(BasicShadowLayerManager* aManager) :
    BasicColorLayer(aManager)
  {
    MOZ_COUNT_CTOR(BasicShadowableColorLayer);
  }
  virtual ~BasicShadowableColorLayer()
  {
    MOZ_COUNT_DTOR(BasicShadowableColorLayer);
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ColorLayerAttributes(GetColor());
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect()
  {
    BasicShadowableLayer::Disconnect();
  }
};

class BasicShadowableCanvasLayer : public BasicCanvasLayer,
                                   public BasicShadowableLayer
{
public:
  BasicShadowableCanvasLayer(BasicShadowLayerManager* aManager) :
    BasicCanvasLayer(aManager)
  {
    MOZ_COUNT_CTOR(BasicShadowableCanvasLayer);
  }
  virtual ~BasicShadowableCanvasLayer()
  {
    if (IsSurfaceDescriptorValid(mBackBuffer)) {
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBuffer);
    }
    MOZ_COUNT_DTOR(BasicShadowableCanvasLayer);
  }

  virtual void Initialize(const Data& aData);
  virtual void Paint(gfxContext* aContext);

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = CanvasLayerAttributes(mFilter);
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void SetBackBuffer(const SurfaceDescriptor& aBuffer)
  {
    mBackBuffer = aBuffer;
  }

  virtual void Disconnect()
  {
    mBackBuffer = SurfaceDescriptor();
    BasicShadowableLayer::Disconnect();
  }

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  SurfaceDescriptor mBackBuffer;
};

void
BasicShadowableCanvasLayer::Initialize(const Data& aData)
{
  BasicCanvasLayer::Initialize(aData);
  if (!HasShadow())
      return;

  
  

  if (IsSurfaceDescriptorValid(mBackBuffer)) {
    BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBuffer);

    BasicManager()->DestroyedCanvasBuffer(BasicManager()->Hold(this));
  }

  SurfaceDescriptor tmpFrontBuffer;
  
  if (!BasicManager()->AllocDoubleBuffer(
        gfxIntSize(aData.mSize.width, aData.mSize.height),
        (GetContentFlags() & CONTENT_OPAQUE) ?
          gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA,
        &tmpFrontBuffer, &mBackBuffer))
    NS_RUNTIMEABORT("creating CanvasLayer back buffer failed!");

  BasicManager()->CreatedCanvasBuffer(BasicManager()->Hold(this),
                                      aData.mSize,
                                      tmpFrontBuffer);
}

void
BasicShadowableCanvasLayer::Paint(gfxContext* aContext)
{
  BasicCanvasLayer::Paint(aContext);
  if (!HasShadow())
    return;

  
  
  
  
  
  nsRefPtr<gfxASurface> backSurface =
    BasicManager()->OpenDescriptor(mBackBuffer);
  nsRefPtr<gfxContext> tmpCtx = new gfxContext(backSurface);
  tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);

  
  
  
  
  BasicCanvasLayer::PaintWithOpacity(tmpCtx, 1.0f);

  BasicManager()->PaintedCanvas(BasicManager()->Hold(this),
                                mBackBuffer);
}

class ShadowThebesLayerBuffer : public BasicThebesLayerBuffer
{
  typedef BasicThebesLayerBuffer Base;

public:
  ShadowThebesLayerBuffer()
    : Base(NULL)
  {
    MOZ_COUNT_CTOR(ShadowThebesLayerBuffer);
  }

  ~ShadowThebesLayerBuffer()
  {
    MOZ_COUNT_DTOR(ShadowThebesLayerBuffer);
  }

  void Swap(gfxASurface* aNewBuffer,
            const nsIntRect& aNewRect, const nsIntPoint& aNewRotation,
            gfxASurface** aOldBuffer,
            nsIntRect* aOldRect, nsIntPoint* aOldRotation)
  {
    *aOldRect = BufferRect();
    *aOldRotation = BufferRotation();

    gfxIntSize newSize = aNewBuffer->GetSize();
    nsRefPtr<gfxASurface> oldBuffer;
    oldBuffer = SetBuffer(aNewBuffer,
                          nsIntSize(newSize.width, newSize.height),
                          aNewRect, aNewRotation);
    oldBuffer.forget(aOldBuffer);
  }

protected:
  virtual already_AddRefed<gfxASurface>
  CreateBuffer(ContentType aType, const nsIntSize& aSize)
  {
    NS_RUNTIMEABORT("ShadowThebesLayer can't paint content");
    return nsnull;
  }
};


class BasicShadowThebesLayer : public ShadowThebesLayer, BasicImplData {
public:
  BasicShadowThebesLayer(BasicShadowLayerManager* aLayerManager)
    : ShadowThebesLayer(aLayerManager, static_cast<BasicImplData*>(this))
    , mOldXResolution(1.0)
    , mOldYResolution(1.0)
  {
    MOZ_COUNT_CTOR(BasicShadowThebesLayer);
  }
  virtual ~BasicShadowThebesLayer()
  {
    
    
    
    MOZ_COUNT_DTOR(BasicShadowThebesLayer);
  }

  virtual void SetFrontBuffer(const OptionalThebesBuffer& aNewFront,
                              const nsIntRegion& aValidRegion,
                              float aXResolution, float aYResolution);

  virtual void SetValidRegion(const nsIntRegion& aRegion)
  {
    mOldValidRegion = mValidRegion;
    ShadowThebesLayer::SetValidRegion(aRegion);
  }

  virtual void SetResolution(float aXResolution, float aYResolution)
  {
    mOldXResolution = mXResolution;
    mOldYResolution = mYResolution;
    ShadowThebesLayer::SetResolution(aXResolution, aYResolution);
  }

  virtual void Disconnect()
  {
    DestroyFrontBuffer();
    ShadowThebesLayer::Disconnect();
  }

  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       ThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       float* aNewXResolution, float* aNewYResolution,
       OptionalThebesBuffer* aReadOnlyFront, nsIntRegion* aFrontUpdatedRegion);

  virtual void DestroyFrontBuffer()
  {
    mFrontBuffer.Clear();
    mValidRegion.SetEmpty();
    mOldValidRegion.SetEmpty();
    mOldXResolution = 1.0;
    mOldYResolution = 1.0;

    if (IsSurfaceDescriptorValid(mFrontBufferDescriptor)) {
      BasicManager()->ShadowLayerManager::DestroySharedSurface(&mFrontBufferDescriptor, mAllocator);
    }
  }

  virtual void PaintThebes(gfxContext* aContext,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback);

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  ShadowThebesLayerBuffer mFrontBuffer;
  
  SurfaceDescriptor mFrontBufferDescriptor;
  
  
  
  
  nsIntRegion mOldValidRegion;
  float mOldXResolution;
  float mOldYResolution;
};

void
BasicShadowThebesLayer::SetFrontBuffer(const OptionalThebesBuffer& aNewFront,
                                       const nsIntRegion& aValidRegion,
                                       float aXResolution, float aYResolution)
{
  mValidRegion = mOldValidRegion = aValidRegion;
  mXResolution = mOldXResolution = aXResolution;
  mYResolution = mOldYResolution = aYResolution;

  NS_ABORT_IF_FALSE(OptionalThebesBuffer::Tnull_t != aNewFront.type(),
                    "aNewFront must be valid here!");

  const ThebesBuffer newFront = aNewFront.get_ThebesBuffer();
  nsRefPtr<gfxASurface> newFrontBuffer =
    BasicManager()->OpenDescriptor(newFront.buffer());

  nsRefPtr<gfxASurface> unused;
  nsIntRect unusedRect;
  nsIntPoint unusedRotation;
  mFrontBuffer.Swap(newFrontBuffer, newFront.rect(), newFront.rotation(),
                    getter_AddRefs(unused), &unusedRect, &unusedRotation);
  mFrontBufferDescriptor = newFront.buffer();
}

void
BasicShadowThebesLayer::Swap(const ThebesBuffer& aNewFront,
                             const nsIntRegion& aUpdatedRegion,
                             ThebesBuffer* aNewBack,
                             nsIntRegion* aNewBackValidRegion,
                             float* aNewXResolution, float* aNewYResolution,
                             OptionalThebesBuffer* aReadOnlyFront,
                             nsIntRegion* aFrontUpdatedRegion)
{
  
  aNewBack->buffer() = mFrontBufferDescriptor;
  
  
  if (mOldXResolution == mXResolution && mOldYResolution == mYResolution) {
    aNewBackValidRegion->Sub(mOldValidRegion, aUpdatedRegion);
  } else {
    
    
    
    
    
    
    
    
    aNewBackValidRegion->SetEmpty();
    mOldXResolution = mXResolution;
    mOldYResolution = mYResolution;
  }
  NS_ASSERTION(mXResolution == mOldXResolution && mYResolution == mOldYResolution,
               "Uh-oh, buffer allocation thrash forthcoming!");
  *aNewXResolution = mXResolution;
  *aNewYResolution = mYResolution;

  nsRefPtr<gfxASurface> newFrontBuffer =
    BasicManager()->OpenDescriptor(aNewFront.buffer());

  nsRefPtr<gfxASurface> unused;
  mFrontBuffer.Swap(
    newFrontBuffer, aNewFront.rect(), aNewFront.rotation(),
    getter_AddRefs(unused), &aNewBack->rect(), &aNewBack->rotation());

  mFrontBufferDescriptor = aNewFront.buffer();

  *aReadOnlyFront = aNewFront;
  *aFrontUpdatedRegion = aUpdatedRegion;
}

void
BasicShadowThebesLayer::PaintThebes(gfxContext* aContext,
                                    LayerManager::DrawThebesLayerCallback aCallback,
                                    void* aCallbackData,
                                    ReadbackProcessor* aReadback)
{
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  NS_ASSERTION(BasicManager()->IsRetained(),
               "ShadowThebesLayer makes no sense without retained mode");

  if (!mFrontBuffer.GetBuffer()) {
    return;
  }

  gfxContext* target = BasicManager()->GetTarget();
  NS_ASSERTION(target, "We shouldn't be called if there's no target");

  mFrontBuffer.DrawTo(this, target, GetEffectiveOpacity());
}

class BasicShadowContainerLayer : public ShadowContainerLayer, BasicImplData {
  template<class Container>
  friend void ContainerInsertAfter(Layer* aChild, Layer* aAfter, Container* aContainer);
  template<class Container>
  friend void ContainerRemoveChild(Layer* aChild, Container* aContainer);

public:
  BasicShadowContainerLayer(BasicShadowLayerManager* aLayerManager) :
    ShadowContainerLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicShadowContainerLayer);
  }
  virtual ~BasicShadowContainerLayer()
  {
    while (mFirstChild) {
      ContainerRemoveChild(mFirstChild, this);
    }

    MOZ_COUNT_DTOR(BasicShadowContainerLayer);
  }

  virtual void InsertAfter(Layer* aChild, Layer* aAfter)
  { ContainerInsertAfter(aChild, aAfter, this); }
  virtual void RemoveChild(Layer* aChild)
  { ContainerRemoveChild(aChild, this); }

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    
    
    
    gfx3DMatrix idealTransform = GetLocalTransform()*aTransformToSurface;
    mEffectiveTransform = SnapTransform(idealTransform, gfxRect(0, 0, 0, 0), nsnull);
    
    
    ComputeEffectiveTransformsForChildren(idealTransform);

    




    mUseIntermediateSurface = GetEffectiveOpacity() != 1.0 && HasMultipleChildren();
  }
};

class BasicShadowImageLayer : public ShadowImageLayer, BasicImplData {
public:
  BasicShadowImageLayer(BasicShadowLayerManager* aLayerManager) :
    ShadowImageLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicShadowImageLayer);
  }
  virtual ~BasicShadowImageLayer()
  {
    MOZ_COUNT_DTOR(BasicShadowImageLayer);
  }

  virtual void Disconnect()
  {
    DestroyFrontBuffer();
    ShadowImageLayer::Disconnect();
  }

  virtual PRBool Init(const SharedImage& front, const nsIntSize& size);

  virtual void Swap(const SharedImage& aNewFront, SharedImage* aNewBack);

  virtual void DestroyFrontBuffer()
  {
    if (IsSurfaceDescriptorValid(mFrontBuffer)) {
      BasicManager()->ShadowLayerManager::DestroySharedSurface(&mFrontBuffer, mAllocator);
    }
  }

  virtual void Paint(gfxContext* aContext);

protected:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  SurfaceDescriptor mFrontBuffer;
  gfxIntSize mSize;
};

PRBool
BasicShadowImageLayer::Init(const SharedImage& front,
                            const nsIntSize& size)
{
  mFrontBuffer = front.get_SurfaceDescriptor();
  mSize = gfxIntSize(size.width, size.height);
  return PR_TRUE;
}

void
BasicShadowImageLayer::Swap(const SharedImage& aNewFront, SharedImage* aNewBack)
{
  *aNewBack = mFrontBuffer;
  mFrontBuffer = aNewFront.get_SurfaceDescriptor();
}

void
BasicShadowImageLayer::Paint(gfxContext* aContext)
{
  if (!IsSurfaceDescriptorValid(mFrontBuffer)) {
    return;
  }

  nsRefPtr<gfxASurface> surface =
    BasicManager()->OpenDescriptor(mFrontBuffer);
  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);
  pat->SetFilter(mFilter);

  
  
  
  const nsIntRect* tileSrcRect = GetTileSourceRect();
  BasicImageLayer::PaintContext(pat,
                                tileSrcRect ? GetEffectiveVisibleRegion() : nsIntRegion(nsIntRect(0, 0, mSize.width, mSize.height)),
                                tileSrcRect,
                                GetEffectiveOpacity(), aContext);
}

class BasicShadowColorLayer : public ShadowColorLayer,
                              BasicImplData
{
public:
  BasicShadowColorLayer(BasicShadowLayerManager* aLayerManager) :
    ShadowColorLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicShadowColorLayer);
  }
  virtual ~BasicShadowColorLayer()
  {
    MOZ_COUNT_DTOR(BasicShadowColorLayer);
  }

  virtual void Paint(gfxContext* aContext)
  {
    BasicColorLayer::PaintColorTo(mColor, GetEffectiveOpacity(), aContext);
  }
};

class BasicShadowCanvasLayer : public ShadowCanvasLayer,
                               BasicImplData
{
public:
  BasicShadowCanvasLayer(BasicShadowLayerManager* aLayerManager) :
    ShadowCanvasLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicShadowCanvasLayer);
  }
  virtual ~BasicShadowCanvasLayer()
  {
    MOZ_COUNT_DTOR(BasicShadowCanvasLayer);
  }

  virtual void Disconnect()
  {
    DestroyFrontBuffer();
    ShadowCanvasLayer::Disconnect();
  }

  virtual void Initialize(const Data& aData);
  virtual void Init(const SurfaceDescriptor& aNewFront, const nsIntSize& aSize);

  void Swap(const SurfaceDescriptor& aNewFront, SurfaceDescriptor* aNewBack);

  virtual void DestroyFrontBuffer()
  {
    if (IsSurfaceDescriptorValid(mFrontSurface)) {
      BasicManager()->ShadowLayerManager::DestroySharedSurface(&mFrontSurface, mAllocator);
    }
  }

  virtual void Paint(gfxContext* aContext);

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  SurfaceDescriptor mFrontSurface;
};


void
BasicShadowCanvasLayer::Initialize(const Data& aData)
{
  NS_RUNTIMEABORT("Incompatibe surface type");
}

void
BasicShadowCanvasLayer::Init(const SurfaceDescriptor& aNewFront, const nsIntSize& aSize)
{
  mFrontSurface = aNewFront;
  mBounds.SetRect(0, 0, aSize.width, aSize.height);
}

void
BasicShadowCanvasLayer::Swap(const SurfaceDescriptor& aNewFront, SurfaceDescriptor* aNewBack)
{
  *aNewBack = mFrontSurface;
  mFrontSurface = aNewFront;
}

void
BasicShadowCanvasLayer::Paint(gfxContext* aContext)
{
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");

  if (!IsSurfaceDescriptorValid(mFrontSurface)) {
    return;
  }

  nsRefPtr<gfxASurface> surface =
    BasicManager()->OpenDescriptor(mFrontSurface);
  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);

  pat->SetFilter(mFilter);
  pat->SetExtend(gfxPattern::EXTEND_PAD);

  gfxRect r(0, 0, mBounds.width, mBounds.height);
  aContext->NewPath();
  
  aContext->Rectangle(r);
  aContext->SetPattern(pat);
  aContext->FillWithOpacity(GetEffectiveOpacity());
}




template<typename CreatedMethod>
static void
MaybeCreateShadowFor(BasicShadowableLayer* aLayer,
                     BasicShadowLayerManager* aMgr,
                     CreatedMethod aMethod)
{
  if (!aMgr->HasShadowManager()) {
    return;
  }

  PLayerChild* shadow = aMgr->ConstructShadowFor(aLayer);
  
  NS_ABORT_IF_FALSE(shadow, "failed to create shadow");

  aLayer->SetShadow(shadow);
  (aMgr->*aMethod)(aLayer);
  aMgr->Hold(aLayer->AsLayer());
}
#define MAYBE_CREATE_SHADOW(_type)                                      \
  MaybeCreateShadowFor(layer, this,                                     \
                       &ShadowLayerForwarder::Created ## _type ## Layer)

already_AddRefed<ThebesLayer>
BasicShadowLayerManager::CreateThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<BasicShadowableThebesLayer> layer =
    new BasicShadowableThebesLayer(this);
  MAYBE_CREATE_SHADOW(Thebes);
  return layer.forget();
}

already_AddRefed<ContainerLayer>
BasicShadowLayerManager::CreateContainerLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<BasicShadowableContainerLayer> layer =
    new BasicShadowableContainerLayer(this);
  MAYBE_CREATE_SHADOW(Container);
  return layer.forget();
}

already_AddRefed<ImageLayer>
BasicShadowLayerManager::CreateImageLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<BasicShadowableImageLayer> layer =
    new BasicShadowableImageLayer(this);
  MAYBE_CREATE_SHADOW(Image);
  return layer.forget();
}

already_AddRefed<ColorLayer>
BasicShadowLayerManager::CreateColorLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<BasicShadowableColorLayer> layer =
    new BasicShadowableColorLayer(this);
  MAYBE_CREATE_SHADOW(Color);
  return layer.forget();
}

already_AddRefed<CanvasLayer>
BasicShadowLayerManager::CreateCanvasLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<BasicShadowableCanvasLayer> layer =
    new BasicShadowableCanvasLayer(this);
  MAYBE_CREATE_SHADOW(Canvas);
  return layer.forget();
}
already_AddRefed<ShadowThebesLayer>
BasicShadowLayerManager::CreateShadowThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ShadowThebesLayer> layer = new BasicShadowThebesLayer(this);
  return layer.forget();
}

already_AddRefed<ShadowContainerLayer>
BasicShadowLayerManager::CreateShadowContainerLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ShadowContainerLayer> layer = new BasicShadowContainerLayer(this);
  return layer.forget();
}

already_AddRefed<ShadowImageLayer>
BasicShadowLayerManager::CreateShadowImageLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ShadowImageLayer> layer = new BasicShadowImageLayer(this);
  return layer.forget();
}

already_AddRefed<ShadowColorLayer>
BasicShadowLayerManager::CreateShadowColorLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ShadowColorLayer> layer = new BasicShadowColorLayer(this);
  return layer.forget();
}

already_AddRefed<ShadowCanvasLayer>
BasicShadowLayerManager::CreateShadowCanvasLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ShadowCanvasLayer> layer = new BasicShadowCanvasLayer(this);
  return layer.forget();
}

BasicShadowLayerManager::BasicShadowLayerManager(nsIWidget* aWidget) :
  BasicLayerManager(aWidget)
{
  MOZ_COUNT_CTOR(BasicShadowLayerManager);
}

BasicShadowLayerManager::~BasicShadowLayerManager()
{
  MOZ_COUNT_DTOR(BasicShadowLayerManager);
}

void
BasicShadowLayerManager::SetRoot(Layer* aLayer)
{
  if (mRoot != aLayer) {
    if (HasShadowManager()) {
      
      
      
      
      if (mRoot) {
        Hold(mRoot);
      }
      ShadowLayerForwarder::SetRoot(Hold(aLayer));
    }
    BasicLayerManager::SetRoot(aLayer);
  }
}

void
BasicShadowLayerManager::Mutated(Layer* aLayer)
{
  BasicLayerManager::Mutated(aLayer);

  NS_ASSERTION(InConstruction() || InDrawing(), "wrong phase");
  if (HasShadowManager() && ShouldShadow(aLayer)) {
    ShadowLayerForwarder::Mutated(Hold(aLayer));
  }
}

void
BasicShadowLayerManager::BeginTransactionWithTarget(gfxContext* aTarget)
{
  NS_ABORT_IF_FALSE(mKeepAlive.IsEmpty(), "uncommitted txn?");
  
  
  
  if (HasShadowManager()) {
    ShadowLayerForwarder::BeginTransaction();
  }
  BasicLayerManager::BeginTransactionWithTarget(aTarget);
}

void
BasicShadowLayerManager::EndTransaction(DrawThebesLayerCallback aCallback,
                                        void* aCallbackData)
{
  BasicLayerManager::EndTransaction(aCallback, aCallbackData);
  ForwardTransaction();
}

bool
BasicShadowLayerManager::EndEmptyTransaction()
{
  if (!BasicLayerManager::EndEmptyTransaction()) {
    
    
    
    return false;
  }
  ForwardTransaction();
  return true;
}

void
BasicShadowLayerManager::ForwardTransaction()
{
#ifdef DEBUG
  mPhase = PHASE_FORWARD;
#endif

  
  AutoInfallibleTArray<EditReply, 10> replies;
  if (HasShadowManager() && ShadowLayerForwarder::EndTransaction(&replies)) {
    for (nsTArray<EditReply>::size_type i = 0; i < replies.Length(); ++i) {
      const EditReply& reply = replies[i];

      switch (reply.type()) {
      case EditReply::TOpThebesBufferSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] ThebesBufferSwap"));

        const OpThebesBufferSwap& obs = reply.get_OpThebesBufferSwap();
        BasicShadowableThebesLayer* thebes = GetBasicShadowable(obs)->AsThebes();
        thebes->SetBackBufferAndAttrs(
          obs.newBackBuffer(),
          obs.newValidRegion(), obs.newXResolution(), obs.newYResolution(),
          obs.readOnlyFrontBuffer(), obs.frontUpdatedRegion());
        break;
      }
      case EditReply::TOpBufferSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] BufferSwap"));

        const OpBufferSwap& obs = reply.get_OpBufferSwap();
        const SurfaceDescriptor& descr = obs.newBackBuffer();
        GetBasicShadowable(obs)->SetBackBuffer(descr);
        break;
      }

      case EditReply::TOpImageSwap: {
        MOZ_LAYERS_LOG(("[LayersForwarder] YUVBufferSwap"));

        const OpImageSwap& ois = reply.get_OpImageSwap();
        BasicShadowableLayer* layer = GetBasicShadowable(ois);
        const SharedImage& newBack = ois.newBackImage();

        if (newBack.type() == SharedImage::TSurfaceDescriptor) {
          layer->SetBackBuffer(newBack.get_SurfaceDescriptor());
        } else {
          const YUVImage& yuv = newBack.get_YUVImage();
          nsRefPtr<gfxSharedImageSurface> YSurf = gfxSharedImageSurface::Open(yuv.Ydata());
          nsRefPtr<gfxSharedImageSurface> USurf = gfxSharedImageSurface::Open(yuv.Udata());
          nsRefPtr<gfxSharedImageSurface> VSurf = gfxSharedImageSurface::Open(yuv.Vdata());
          layer->SetBackBufferYUVImage(YSurf, USurf, VSurf);
        }
        
        break;
      }

      default:
        NS_RUNTIMEABORT("not reached");
      }
    }
  } else if (HasShadowManager()) {
    NS_WARNING("failed to forward Layers transaction");
  }

#ifdef DEBUG
  mPhase = PHASE_NONE;
#endif

  
  
  mKeepAlive.Clear();
}

ShadowableLayer*
BasicShadowLayerManager::Hold(Layer* aLayer)
{
  NS_ABORT_IF_FALSE(HasShadowManager(),
                    "top-level tree, no shadow tree to remote to");

  ShadowableLayer* shadowable = ToShadowable(aLayer);
  NS_ABORT_IF_FALSE(shadowable, "trying to remote an unshadowable layer");

  mKeepAlive.AppendElement(aLayer);
  return shadowable;
}

PRBool
BasicShadowLayerManager::IsCompositingCheap()
{
  
  return mShadowManager &&
         LayerManager::IsCompositingCheap(GetParentBackendType());
}

}
}
