




































#include "BasicLayers.h"
#include "ImageLayers.h"

#include "nsTArray.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxPattern.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "ThebesLayerBuffer.h"

#include "GLContext.h"

namespace mozilla {
namespace layers {

class BasicContainerLayer;























class BasicImplData {
public:
  BasicImplData()
  {
    MOZ_COUNT_CTOR(BasicImplData);
  }
  ~BasicImplData()
  {
    MOZ_COUNT_DTOR(BasicImplData);
  }

  





  virtual void Paint(gfxContext* aContext,
                     LayerManager::DrawThebesLayerCallback aCallback,
                     void* aCallbackData) {}
};

static BasicImplData*
ToData(Layer* aLayer)
{
  return static_cast<BasicImplData*>(aLayer->ImplData());
}

class BasicContainerLayer : public ContainerLayer, BasicImplData {
public:
  BasicContainerLayer(BasicLayerManager* aManager) :
    ContainerLayer(aManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicContainerLayer);
  }
  virtual ~BasicContainerLayer();

  virtual void SetVisibleRegion(const nsIntRegion& aRegion)
  {
    NS_ASSERTION(BasicManager()->InConstruction(),
                 "Can only set properties in construction phase");
    ContainerLayer::SetVisibleRegion(aRegion);
  }
  virtual void InsertAfter(Layer* aChild, Layer* aAfter);
  virtual void RemoveChild(Layer* aChild);

protected:
  void RemoveChildInternal(Layer* aChild);

  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

BasicContainerLayer::~BasicContainerLayer()
{
  while (mFirstChild) {
    RemoveChildInternal(mFirstChild);
  }

  MOZ_COUNT_DTOR(BasicContainerLayer);
}

void
BasicContainerLayer::InsertAfter(Layer* aChild, Layer* aAfter)
{
  NS_ASSERTION(BasicManager()->InConstruction(),
               "Can only set properties in construction phase");
  NS_ASSERTION(aChild->Manager() == Manager(),
               "Child has wrong manager");
  NS_ASSERTION(!aChild->GetParent(),
               "aChild already in the tree");
  NS_ASSERTION(!aChild->GetNextSibling() && !aChild->GetPrevSibling(),
               "aChild already has siblings?");
  NS_ASSERTION(!aAfter ||
               (aAfter->Manager() == Manager() &&
                aAfter->GetParent() == this),
               "aAfter is not our child");

  NS_ADDREF(aChild);

  aChild->SetParent(this);
  if (!aAfter) {
    aChild->SetNextSibling(mFirstChild);
    if (mFirstChild) {
      mFirstChild->SetPrevSibling(aChild);
    }
    mFirstChild = aChild;
    return;
  }

  Layer* next = aAfter->GetNextSibling();
  aChild->SetNextSibling(next);
  aChild->SetPrevSibling(aAfter);
  if (next) {
    next->SetPrevSibling(aChild);
  }
  aAfter->SetNextSibling(aChild);
}

void
BasicContainerLayer::RemoveChild(Layer* aChild)
{
  NS_ASSERTION(BasicManager()->InConstruction(),
               "Can only set properties in construction phase");
  RemoveChildInternal(aChild);
}

void
BasicContainerLayer::RemoveChildInternal(Layer* aChild)
{
  NS_ASSERTION(aChild->Manager() == Manager(),
               "Child has wrong manager");
  NS_ASSERTION(aChild->GetParent() == this,
               "aChild not our child");

  Layer* prev = aChild->GetPrevSibling();
  Layer* next = aChild->GetNextSibling();
  if (prev) {
    prev->SetNextSibling(next);
  } else {
    mFirstChild = next;
  }
  if (next) {
    next->SetPrevSibling(prev);
  }

  aChild->SetNextSibling(nsnull);
  aChild->SetPrevSibling(nsnull);
  aChild->SetParent(nsnull);

  NS_RELEASE(aChild);
}






static PRBool
UseOpaqueSurface(Layer* aLayer)
{
  
  
  if (aLayer->IsOpaqueContent())
    return PR_TRUE;
  
  
  
  
  BasicContainerLayer* parent =
    static_cast<BasicContainerLayer*>(aLayer->GetParent());
  return parent && parent->GetFirstChild() == aLayer &&
         UseOpaqueSurface(parent);
}

class BasicThebesLayer : public ThebesLayer, BasicImplData {
public:
  BasicThebesLayer(BasicLayerManager* aLayerManager) :
    ThebesLayer(aLayerManager, static_cast<BasicImplData*>(this))
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

  virtual void Paint(gfxContext* aContext,
                     LayerManager::DrawThebesLayerCallback aCallback,
                     void* aCallbackData);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

  ThebesLayerBuffer mBuffer;
};

void
BasicThebesLayer::Paint(gfxContext* aContext,
                        LayerManager::DrawThebesLayerCallback aCallback,
                        void* aCallbackData)
{
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  gfxContext* target = BasicManager()->GetTarget();
  NS_ASSERTION(target, "We shouldn't be called if there's no target");

  if (!BasicManager()->IsRetained()) {
    mValidRegion.SetEmpty();
    mBuffer.Clear();
    aCallback(this, target, mVisibleRegion, nsIntRegion(), aCallbackData);
    return;
  }

  PRUint32 flags = 0;
  if (UseOpaqueSurface(this)) {
    flags |= ThebesLayerBuffer::OPAQUE_CONTENT;
  }

  {
    ThebesLayerBuffer::PaintState state =
      mBuffer.BeginPaint(this, aContext, flags);
    mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

    if (state.mContext) {
      
      
      
      
      state.mRegionToInvalidate.And(state.mRegionToInvalidate, mVisibleRegion);
      aCallback(this, state.mContext, state.mRegionToDraw,
                state.mRegionToInvalidate, aCallbackData);
      mValidRegion.Or(mValidRegion, state.mRegionToDraw);
    } else {
      NS_ASSERTION(state.mRegionToDraw.IsEmpty() &&
                   state.mRegionToInvalidate.IsEmpty(),
                   "If we need to draw, we should have a context");
    }
  }

  mBuffer.DrawTo(this, flags, target);
}

class BasicImageLayer : public ImageLayer, BasicImplData {
public:
  BasicImageLayer(BasicLayerManager* aLayerManager) :
    ImageLayer(aLayerManager, static_cast<BasicImplData*>(this))
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

  virtual void Paint(gfxContext* aContext,
                     LayerManager::DrawThebesLayerCallback aCallback,
                     void* aCallbackData);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

void
BasicImageLayer::Paint(gfxContext* aContext,
                       LayerManager::DrawThebesLayerCallback aCallback,
                       void* aCallbackData)
{
  if (!mContainer)
    return;

  gfxIntSize size;
  nsRefPtr<gfxASurface> surface = mContainer->GetCurrentAsSurface(&size);
  if (!surface) {
    return;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);
  if (!pat) {
    return;
  }

  pat->SetFilter(mFilter);

  
  
  gfxPattern::GraphicsExtend extend = gfxPattern::EXTEND_PAD;

  
  
  nsRefPtr<gfxASurface> target = aContext->CurrentSurface();
  gfxASurface::gfxSurfaceType type = target->GetType();
  if (type == gfxASurface::SurfaceTypeXlib ||
      type == gfxASurface::SurfaceTypeXcb ||
      type == gfxASurface::SurfaceTypeQuartz) {
    extend = gfxPattern::EXTEND_NONE;
  }

  pat->SetExtend(extend);

  
  aContext->NewPath();
  aContext->PixelSnappedRectangleAndSetPattern(
      gfxRect(0, 0, size.width, size.height), pat);
  aContext->Fill();
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

  virtual void Paint(gfxContext* aContext,
                     LayerManager::DrawThebesLayerCallback aCallback,
                     void* aCallbackData);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }
};

void
BasicColorLayer::Paint(gfxContext* aContext,
                       LayerManager::DrawThebesLayerCallback aCallback,
                       void* aCallbackData)
{
  aContext->SetColor(mColor);
  aContext->Paint();
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

  virtual void Initialize(const Data& aData);
  virtual void Updated(const nsIntRect& aRect);
  virtual void Paint(gfxContext* aContext,
                     LayerManager::DrawThebesLayerCallback aCallback,
                     void* aCallbackData);

protected:
  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<mozilla::gl::GLContext> mGLContext;

  nsIntRect mBounds;
  nsIntRect mUpdatedRect;

  PRPackedBool mGLBufferIsPremultiplied;
  PRPackedBool mNeedsYFlip;
};

void
BasicCanvasLayer::Initialize(const Data& aData)
{
  NS_ASSERTION(mSurface == nsnull, "BasicCanvasLayer::Initialize called twice!");

  mUpdatedRect.Empty();

  if (aData.mSurface) {
    mSurface = aData.mSurface;
    NS_ASSERTION(aData.mGLContext == nsnull,
                 "CanvasLayer can't have both surface and GLContext");
    mNeedsYFlip = PR_FALSE;
  } else if (aData.mGLContext) {
    mGLContext = aData.mGLContext;
    mGLBufferIsPremultiplied = aData.mGLBufferIsPremultiplied;
    mNeedsYFlip = PR_TRUE;
  } else {
    NS_ERROR("CanvasLayer created without mSurface or mGLContext?");
  }

  mBounds.SetRect(0, 0, aData.mSize.width, aData.mSize.height);
}

void
BasicCanvasLayer::Updated(const nsIntRect& aRect)
{
  NS_ASSERTION(mUpdatedRect.IsEmpty(),
               "CanvasLayer::Updated called more than once in a transaction!");

  mUpdatedRect.UnionRect(mUpdatedRect, aRect);

  if (mGLContext) {
    nsRefPtr<gfxImageSurface> isurf =
      new gfxImageSurface(gfxIntSize(mBounds.width, mBounds.height),
                          IsOpaqueContent()
                            ? gfxASurface::ImageFormatRGB24
                            : gfxASurface::ImageFormatARGB32);
    if (!isurf || isurf->CairoStatus() != 0) {
      return;
    }

    NS_ASSERTION(isurf->Stride() == mBounds.width * 4, "gfxImageSurface stride isn't what we expect!");

    
    mGLContext->MakeCurrent();

    
    
    mGLContext->fFlush();

    
    
    
#ifndef USE_GLES2
    mGLContext->fReadPixels(0, 0, mBounds.width, mBounds.height,
                            LOCAL_GL_BGRA, LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV,
                            isurf->Data());
#else
    mGLContext->fReadPixels(0, 0, mBounds.width, mBounds.height,
                            LOCAL_GL_RGBA, LOCAL_GL_UNSIGNED_BYTE,
                            isurf->Data());
#endif

    
    
    
    
    if (!mGLBufferIsPremultiplied)
      gfxUtils::PremultiplyImageSurface(isurf);

    
    mSurface = isurf;
  }

  
  NS_ASSERTION(mUpdatedRect.IsEmpty() || mBounds.Contains(mUpdatedRect),
               "CanvasLayer: Updated rect bigger than bounds!");
}

void
BasicCanvasLayer::Paint(gfxContext* aContext,
                        LayerManager::DrawThebesLayerCallback aCallback,
                        void* aCallbackData)
{
  nsRefPtr<gfxPattern> pat = new gfxPattern(mSurface);

  pat->SetFilter(mFilter);
  pat->SetExtend(gfxPattern::EXTEND_PAD);

  gfxRect r(0, 0, mBounds.width, mBounds.height);
  gfxMatrix m;
  if (mNeedsYFlip) {
    m = aContext->CurrentMatrix();
    aContext->Translate(gfxPoint(0.0, mBounds.height));
    aContext->Scale(1.0, -1.0);
  }

  aContext->NewPath();
  aContext->PixelSnappedRectangleAndSetPattern(r, pat);
  aContext->Fill();

  if (mNeedsYFlip) {
    aContext->SetMatrix(m);
  }

  mUpdatedRect.Empty();
}

BasicLayerManager::BasicLayerManager(gfxContext* aContext) :
  mDefaultTarget(aContext)
#ifdef DEBUG
  , mPhase(PHASE_NONE)
#endif
  , mDoubleBuffering(BUFFER_NONE), mUsingDefaultTarget(PR_FALSE),
    mRetain(PR_FALSE)
{
  MOZ_COUNT_CTOR(BasicLayerManager);
}

BasicLayerManager::~BasicLayerManager()
{
  NS_ASSERTION(!InTransaction(), "Died during transaction?");
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
BasicLayerManager::SetRetain(PRBool aRetain)
{
  NS_ASSERTION(!InTransaction(),
               "Must set retained mode outside transaction");
  mRetain = aRetain;
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

  if (mCachedSurface) {
    
    if (mCachedSurface->GetContentType() != aContent) {
      mCachedSurface = nsnull;
    } else {
      
      if (clip.size.width > mCachedSurfaceSize.width ||
          clip.size.height > mCachedSurfaceSize.height) {
        mCachedSurface = nsnull;
      }
    }
  }
  nsRefPtr<gfxContext> ctx;
  if (mCachedSurface) {
    ctx = new gfxContext(mCachedSurface);
    ctx->SetOperator(gfxContext::OPERATOR_CLEAR);
    ctx->Paint();
    ctx->SetOperator(gfxContext::OPERATOR_OVER);
  } else {
    mCachedSurfaceSize = gfxIntSize(clip.size.width, clip.size.height);
    mCachedSurface = currentSurf->CreateSimilarSurface(aContent,
                                                       mCachedSurfaceSize);
    if (!mCachedSurface)
      return nsnull;
    ctx = new gfxContext(mCachedSurface);
  }
  
  ctx->Translate(-clip.pos);
  *aSavedOffset = clip.pos;
  ctx->Multiply(saveMatrix.Matrix());
  return ctx.forget();
}

void
BasicLayerManager::PopGroupWithCachedSurface(gfxContext *aTarget,
                                             const gfxPoint& aSavedOffset)
{
  if (!mCachedSurface)
    return;

  gfxContextMatrixAutoSaveRestore saveMatrix(aTarget);
  aTarget->IdentityMatrix();

  aTarget->SetSource(mCachedSurface, aSavedOffset);
  aTarget->Paint();
}

void
BasicLayerManager::BeginTransactionWithTarget(gfxContext* aTarget)
{
  NS_ASSERTION(!InTransaction(), "Nested transactions not allowed");
#ifdef DEBUG
  mPhase = PHASE_CONSTRUCTION;
#endif
  mTarget = aTarget;
}

void
BasicLayerManager::EndTransaction(DrawThebesLayerCallback aCallback,
                                  void* aCallbackData)
{
  NS_ASSERTION(mRoot, "Root not set");
  NS_ASSERTION(InConstruction(), "Should be in construction phase");
#ifdef DEBUG
  mPhase = PHASE_DRAWING;
#endif

  if (mTarget) {
    nsRefPtr<gfxContext> finalTarget = mTarget;
    gfxPoint cachedSurfaceOffset;

    if (mUsingDefaultTarget && mDoubleBuffering != BUFFER_NONE) {
      nsRefPtr<gfxASurface> targetSurface = mTarget->CurrentSurface();
      mTarget = PushGroupWithCachedSurface(mTarget, targetSurface->GetContentType(),
                                           &cachedSurfaceOffset);
    }

    PaintLayer(mRoot, aCallback, aCallbackData);
    
    if (mUsingDefaultTarget && mDoubleBuffering != BUFFER_NONE) {
      finalTarget->SetOperator(gfxContext::OPERATOR_SOURCE);
      PopGroupWithCachedSurface(finalTarget, cachedSurfaceOffset);
    }

    mTarget = nsnull;
  }

#ifdef DEBUG
  mPhase = PHASE_NONE;
#endif
  mUsingDefaultTarget = PR_FALSE;
}

void
BasicLayerManager::SetRoot(Layer* aLayer)
{
  NS_ASSERTION(aLayer, "Root can't be null");
  NS_ASSERTION(aLayer->Manager() == this, "Wrong manager");
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  mRoot = aLayer;
}


static PRBool
NeedsGroup(Layer* aLayer)
{
  return aLayer->GetOpacity() != 1.0;
}




static PRBool
NeedsState(Layer* aLayer)
{
  return aLayer->GetClipRect() != nsnull ||
         !aLayer->GetTransform().IsIdentity();
}

void
BasicLayerManager::PaintLayer(Layer* aLayer,
                              DrawThebesLayerCallback aCallback,
                              void* aCallbackData)
{
  PRBool needsGroup = NeedsGroup(aLayer);
  PRBool needsSaveRestore = needsGroup || NeedsState(aLayer);

 if (needsSaveRestore) {
    mTarget->Save();

    if (aLayer->GetClipRect()) {
      const nsIntRect& r = *aLayer->GetClipRect();
      mTarget->NewPath();
      mTarget->Rectangle(gfxRect(r.x, r.y, r.width, r.height), PR_TRUE);
      mTarget->Clip();
    }

    gfxMatrix transform;
    
    
    NS_ASSERTION(aLayer->GetTransform().Is2D(),
                 "Only 2D transforms supported currently");
    aLayer->GetTransform().Is2D(&transform);
    mTarget->Multiply(transform);

    if (needsGroup) {
      
      
      nsIntRect bbox = aLayer->GetVisibleRegion().GetBounds();
      gfxRect deviceRect =
        mTarget->UserToDevice(gfxRect(bbox.x, bbox.y, bbox.width, bbox.height));
      deviceRect.RoundOut();

      gfxMatrix currentMatrix = mTarget->CurrentMatrix();
      mTarget->IdentityMatrix();
      mTarget->NewPath();
      mTarget->Rectangle(deviceRect);
      mTarget->Clip();
      mTarget->SetMatrix(currentMatrix);

      gfxASurface::gfxContentType type = UseOpaqueSurface(aLayer)
          ? gfxASurface::CONTENT_COLOR : gfxASurface::CONTENT_COLOR_ALPHA;
      mTarget->PushGroup(type);
    }
  }

  ToData(aLayer)->Paint(mTarget, aCallback, aCallbackData);
  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    PaintLayer(child, aCallback, aCallbackData);
  }

  if (needsSaveRestore) {
    if (needsGroup) {
      mTarget->PopGroupToSource();
      mTarget->Paint(aLayer->GetOpacity());
    }

    mTarget->Restore();
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

}
}
