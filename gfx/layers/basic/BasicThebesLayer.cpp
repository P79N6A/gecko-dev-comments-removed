




#include "BasicThebesLayer.h"
#include "gfxUtils.h"
#include "nsIWidget.h"
#include "RenderTrace.h"
#include "sampler.h"
#include "gfxSharedImageSurface.h"

#include "prprf.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

already_AddRefed<gfxASurface>
BasicThebesLayer::CreateBuffer(Buffer::ContentType aType, const nsIntSize& aSize)
{
  nsRefPtr<gfxASurface> referenceSurface = mBuffer.GetBuffer();
  if (!referenceSurface) {
    gfxContext* defaultTarget = BasicManager()->GetDefaultTarget();
    if (defaultTarget) {
      referenceSurface = defaultTarget->CurrentSurface();
    } else {
      nsIWidget* widget = BasicManager()->GetRetainerWidget();
      if (!widget || !(referenceSurface = widget->GetThebesSurface())) {
        referenceSurface = BasicManager()->GetTarget()->CurrentSurface();
      }
    }
  }
  return referenceSurface->CreateSimilarSurface(
    aType, gfxIntSize(aSize.width, aSize.height));
}

TemporaryRef<DrawTarget>
BasicThebesLayer::CreateDrawTarget(const IntSize& aSize, SurfaceFormat aFormat)
{
  return gfxPlatform::GetPlatform()->CreateOffscreenDrawTarget(aSize, aFormat);
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
  if (!aTarget->IsCairo()) {
    RefPtr<DrawTarget> dt = aTarget->GetDrawTarget();

    if (dt->GetFormat() != FORMAT_B8G8R8A8) {
      return;
    }

    const nsIntRect& bounds = aLayer->GetVisibleRegion().GetBounds();
    gfx::Rect transformedBounds = dt->GetTransform().TransformBounds(gfx::Rect(Float(bounds.x), Float(bounds.y),
                                                                     Float(bounds.width), Float(bounds.height)));
    transformedBounds.RoundOut();
    IntRect intTransformedBounds;
    transformedBounds.ToIntRect(&intTransformedBounds);
    dt->SetPermitSubpixelAA(!(aLayer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA) ||
                            dt->GetOpaqueRect().Contains(intTransformedBounds));
  } else {
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
}

void
BasicThebesLayer::PaintThebes(gfxContext* aContext,
                              Layer* aMaskLayer,
                              LayerManager::DrawThebesLayerCallback aCallback,
                              void* aCallbackData,
                              ReadbackProcessor* aReadback)
{
  SAMPLE_LABEL("BasicThebesLayer", "PaintThebes");
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  nsRefPtr<gfxASurface> targetSurface = aContext->CurrentSurface();

  nsTArray<ReadbackProcessor::Update> readbackUpdates;
  if (aReadback && UsedForReadback()) {
    aReadback->GetThebesLayerUpdates(this, &readbackUpdates);
  }
  SyncFrontBufferToBackBuffer();

  bool canUseOpaqueSurface = CanUseOpaqueSurface();
  Buffer::ContentType contentType =
    canUseOpaqueSurface ? gfxASurface::CONTENT_COLOR :
                          gfxASurface::CONTENT_COLOR_ALPHA;
  float opacity = GetEffectiveOpacity();
  
  if (!BasicManager()->IsRetained()) {
    NS_ASSERTION(readbackUpdates.IsEmpty(), "Can't do readback for non-retained layer");

    mValidRegion.SetEmpty();
    mBuffer.Clear();

    nsIntRegion toDraw = IntersectWithClip(GetEffectiveVisibleRegion(), aContext);

    RenderTraceInvalidateStart(this, "FFFF00", toDraw.GetBounds());

    if (!toDraw.IsEmpty() && !IsHidden()) {
      if (!aCallback) {
        BasicManager()->SetTransactionIncomplete();
        return;
      }

      aContext->Save();

      bool needsClipToVisibleRegion = GetClipToVisibleRegion();
      bool needsGroup =
          opacity != 1.0 || GetOperator() != gfxContext::OPERATOR_OVER || aMaskLayer;
      nsRefPtr<gfxContext> groupContext;
      if (needsGroup) {
        groupContext =
          BasicManager()->PushGroupForLayer(aContext, this, toDraw,
                                            &needsClipToVisibleRegion);
        if (GetOperator() != gfxContext::OPERATOR_OVER) {
          needsClipToVisibleRegion = true;
        }
      } else {
        groupContext = aContext;
      }
      SetAntialiasingFlags(this, groupContext);
      aCallback(this, groupContext, toDraw, nsIntRegion(), aCallbackData);
      if (needsGroup) {
        BasicManager()->PopGroupToSourceWithCachedSurface(aContext, groupContext);
        if (needsClipToVisibleRegion) {
          gfxUtils::ClipToRegion(aContext, toDraw);
        }
        AutoSetOperator setOperator(aContext, GetOperator());
        PaintWithMask(aContext, opacity, aMaskLayer);
      }

      aContext->Restore();
    }

    RenderTraceInvalidateEnd(this, "FFFF00");
    return;
  }

  {
    uint32_t flags = 0;
#ifndef MOZ_WIDGET_ANDROID
    if (BasicManager()->CompositorMightResample()) {
      flags |= ThebesLayerBuffer::PAINT_WILL_RESAMPLE;
    }
    if (!(flags & ThebesLayerBuffer::PAINT_WILL_RESAMPLE)) {
      gfxMatrix transform;
      if (!GetEffectiveTransform().CanDraw2D(&transform) ||
          transform.HasNonIntegerTranslation()) {
        flags |= ThebesLayerBuffer::PAINT_WILL_RESAMPLE;
      }
    }
#endif
    if (mDrawAtomically) {
      flags |= ThebesLayerBuffer::PAINT_NO_ROTATION;
    }
    Buffer::PaintState state =
      mBuffer.BeginPaint(this, contentType, flags);
    mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

    if (state.mContext) {
      
      
      
      
      state.mRegionToInvalidate.And(state.mRegionToInvalidate,
                                    GetEffectiveVisibleRegion());
      nsIntRegion extendedDrawRegion = state.mRegionToDraw;
      SetAntialiasingFlags(this, state.mContext);

      RenderTraceInvalidateStart(this, "FFFF00", state.mRegionToDraw.GetBounds());

      PaintBuffer(state.mContext,
                  state.mRegionToDraw, extendedDrawRegion, state.mRegionToInvalidate,
                  state.mDidSelfCopy,
                  aCallback, aCallbackData);
      Mutated();

      RenderTraceInvalidateEnd(this, "FFFF00");
    } else {
      
      
      
      NS_WARN_IF_FALSE(state.mRegionToDraw.IsEmpty(),
                       "No context when we have something to draw; resource exhaustion?");
    }
  }

  if (BasicManager()->IsTransactionIncomplete())
    return;

  gfxRect clipExtents;
  clipExtents = aContext->GetClipExtents();
  if (!IsHidden() && !clipExtents.IsEmpty()) {
    AutoSetOperator setOperator(aContext, GetOperator());
    mBuffer.DrawTo(this, aContext, opacity, aMaskLayer);
  }

  for (uint32_t i = 0; i < readbackUpdates.Length(); ++i) {
    ReadbackProcessor::Update& update = readbackUpdates[i];
    nsIntPoint offset = update.mLayer->GetBackgroundLayerOffset();
    nsRefPtr<gfxContext> ctx =
      update.mLayer->GetSink()->BeginUpdate(update.mUpdateRect + offset,
                                            update.mSequenceCounter);
    if (ctx) {
      NS_ASSERTION(opacity == 1.0, "Should only read back opaque layers");
      ctx->Translate(gfxPoint(offset.x, offset.y));
      mBuffer.DrawTo(this, ctx, 1.0, aMaskLayer);
      update.mLayer->GetSink()->EndUpdate(ctx, update.mUpdateRect + offset);
    }
  }
}

















struct NS_STACK_CLASS AutoBufferTracker {
  AutoBufferTracker(BasicShadowableThebesLayer* aLayer)
    : mLayer(aLayer)
  {
    MOZ_ASSERT(!mLayer->mBufferTracker);

    mLayer->mBufferTracker = this;
    if (IsSurfaceDescriptorValid(mLayer->mBackBuffer)) {
      mInitialBuffer.construct(OPEN_READ_WRITE, mLayer->mBackBuffer);
      mLayer->mBuffer.ProvideBuffer(&mInitialBuffer.ref());
    }
  }

  ~AutoBufferTracker() {
    mLayer->mBufferTracker = nullptr;
    mLayer->mBuffer.RevokeBuffer();
    
    
  }

  gfxASurface* GetInitialBuffer() { return mInitialBuffer.ref().Get(); }

  DrawTarget* GetInitialDrawTarget() {
    return mInitialBuffer.ref().GetDrawTarget();
  }

  gfxASurface*
  CreatedBuffer(const SurfaceDescriptor& aDescriptor) {
    Maybe<AutoOpenSurface>* surface = mNewBuffers.AppendElement();
    surface->construct(OPEN_READ_WRITE, aDescriptor);
    return surface->ref().Get();
  }

  Maybe<AutoOpenSurface> mInitialBuffer;
  nsAutoTArray<Maybe<AutoOpenSurface>, 3> mNewBuffers;
  BasicShadowableThebesLayer* mLayer;

private:
  AutoBufferTracker(const AutoBufferTracker&) MOZ_DELETE;
  AutoBufferTracker& operator=(const AutoBufferTracker&) MOZ_DELETE;
};

BasicShadowableThebesLayer::~BasicShadowableThebesLayer()
{
  
  
  if (OptionalThebesBuffer::Tnull_t != mROFrontBuffer.type()) {
    ShadowLayerForwarder::PlatformSyncBeforeUpdate();
  }
  DestroyBackBuffer();
  MOZ_COUNT_DTOR(BasicShadowableThebesLayer);
}

void
BasicShadowableThebesLayer::PaintThebes(gfxContext* aContext,
                                        Layer* aMaskLayer,
                                        LayerManager::DrawThebesLayerCallback aCallback,
                                        void* aCallbackData,
                                        ReadbackProcessor* aReadback)
{
  if (!HasShadow()) {
    BasicThebesLayer::PaintThebes(aContext, aMaskLayer, aCallback, aCallbackData, aReadback);
    return;
  }

  AutoBufferTracker tracker(this);

  BasicThebesLayer::PaintThebes(aContext, nullptr, aCallback, aCallbackData, aReadback);
  if (aMaskLayer) {
    static_cast<BasicImplData*>(aMaskLayer->ImplData())
      ->Paint(aContext, nullptr);
  }
}


void
BasicShadowableThebesLayer::SetBackBufferAndAttrs(const OptionalThebesBuffer& aBuffer,
                                                  const nsIntRegion& aValidRegion,
                                                  const OptionalThebesBuffer& aReadOnlyFrontBuffer,
                                                  const nsIntRegion& aFrontUpdatedRegion)
{
  if (OptionalThebesBuffer::Tnull_t == aBuffer.type()) {
    mBackBuffer = SurfaceDescriptor();
  } else {
    mBackBuffer = aBuffer.get_ThebesBuffer().buffer();
    mBackBufferRect = aBuffer.get_ThebesBuffer().rect();
    mBackBufferRectRotation = aBuffer.get_ThebesBuffer().rotation();
  }
  mFrontAndBackBufferDiffer = true;
  mROFrontBuffer = aReadOnlyFrontBuffer;
  mFrontUpdatedRegion = aFrontUpdatedRegion;
  mFrontValidRegion = aValidRegion;
}

void
BasicShadowableThebesLayer::SyncFrontBufferToBackBuffer()
{
  if (!mFrontAndBackBufferDiffer) {
    return;
  }

  
  
  
  MOZ_ASSERT(mBufferTracker);

  nsRefPtr<gfxASurface> backBuffer = mBuffer.GetBuffer();
  DrawTarget* backDT = mBuffer.GetDT();

  if (!IsSurfaceDescriptorValid(mBackBuffer)) {
    MOZ_ASSERT(!backBuffer && !backDT);
    MOZ_ASSERT(mROFrontBuffer.type() == OptionalThebesBuffer::TThebesBuffer);
    const ThebesBuffer roFront = mROFrontBuffer.get_ThebesBuffer();
    AutoOpenSurface roFrontBuffer(OPEN_READ_ONLY, roFront.buffer());
    backBuffer = CreateBuffer(roFrontBuffer.ContentType(), roFrontBuffer.Size());
  }
  mFrontAndBackBufferDiffer = false;

  if (!backBuffer && !backDT) {
    if (gfxPlatform::GetPlatform()->SupportsAzureContent()) {
      backDT = mBufferTracker->GetInitialDrawTarget();
    } else {
      backBuffer = mBufferTracker->GetInitialBuffer();
    }
  }

  if (OptionalThebesBuffer::Tnull_t == mROFrontBuffer.type()) {
    
    
    
    
    mValidRegion = mFrontValidRegion;
    if (backDT) {
      mBuffer.SetBackingBuffer(backDT, mBackBufferRect, mBackBufferRectRotation);
    } else {
      mBuffer.SetBackingBuffer(backBuffer, mBackBufferRect, mBackBufferRectRotation);
    }
    return;
  }

  MOZ_LAYERS_LOG(("BasicShadowableThebes(%p): reading back <x=%d,y=%d,w=%d,h=%d>",
                  this,
                  mFrontUpdatedRegion.GetBounds().x,
                  mFrontUpdatedRegion.GetBounds().y,
                  mFrontUpdatedRegion.GetBounds().width,
                  mFrontUpdatedRegion.GetBounds().height));

  const ThebesBuffer roFront = mROFrontBuffer.get_ThebesBuffer();
  AutoOpenSurface autoROFront(OPEN_READ_ONLY, roFront.buffer());

  if (backDT) {
    mBuffer.SetBackingBufferAndUpdateFrom(
      backDT,
      autoROFront.GetDrawTarget(), roFront.rect(), roFront.rotation(),
      mFrontUpdatedRegion);
  } else {
    mBuffer.SetBackingBufferAndUpdateFrom(
      backBuffer,
      autoROFront.Get(), roFront.rect(), roFront.rotation(),
      mFrontUpdatedRegion);
  }

  mIsNewBuffer = false;
  
  
  
}

void
BasicShadowableThebesLayer::PaintBuffer(gfxContext* aContext,
                                        const nsIntRegion& aRegionToDraw,
                                        const nsIntRegion& aExtendedRegionToDraw,
                                        const nsIntRegion& aRegionToInvalidate,
                                        bool aDidSelfCopy,
                                        LayerManager::DrawThebesLayerCallback aCallback,
                                        void* aCallbackData)
{
  Base::PaintBuffer(aContext,
                    aRegionToDraw, aExtendedRegionToDraw, aRegionToInvalidate,
                    aDidSelfCopy,
                    aCallback, aCallbackData);
  if (!HasShadow() || BasicManager()->IsTransactionIncomplete()) {
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

  if (!BasicManager()->AllocBuffer(gfxIntSize(aSize.width, aSize.height),
                                   aType,
                                   &mBackBuffer)) {
    enum { buflen = 256 };
    char buf[buflen];
    PR_snprintf(buf, buflen,
                "creating ThebesLayer 'back buffer' failed! width=%d, height=%d, type=%x",
                aSize.width, aSize.height, int(aType));
    NS_RUNTIMEABORT(buf);
  }

  NS_ABORT_IF_FALSE(!mIsNewBuffer,
                    "Bad! Did we create a buffer twice without painting?");

  mIsNewBuffer = true;

  nsRefPtr<gfxASurface> buffer = mBufferTracker->CreatedBuffer(mBackBuffer);
  return buffer.forget();
}

TemporaryRef<DrawTarget>
BasicShadowableThebesLayer::CreateDrawTarget(const IntSize& aSize,
                                             SurfaceFormat aFormat)
{
  if (!HasShadow()) {
    return BasicThebesLayer::CreateDrawTarget(aSize, aFormat);
  }

  MOZ_LAYERS_LOG(("BasicShadowableThebes(%p): creating %d x %d buffer(x2)",
                  this,
                  aSize.width, aSize.height));

  if (IsSurfaceDescriptorValid(mBackBuffer)) {
    BasicManager()->DestroyedThebesBuffer(BasicManager()->Hold(this),
                                          mBackBuffer);
    mBackBuffer = SurfaceDescriptor();
  }

  
  if (!BasicManager()->AllocBuffer(gfxIntSize(aSize.width, aSize.height),
                                   mozilla::gfx::ContentForFormat(aFormat),
                                   &mBackBuffer)) {
    NS_RUNTIMEABORT("creating ThebesLayer 'back buffer' failed!");
  }

  NS_ABORT_IF_FALSE(!mIsNewBuffer,
                    "Bad! Did we create a buffer twice without painting?");

  mIsNewBuffer = true;

  mozilla::ipc::Shmem shm = mBackBuffer.get_Shmem();
  if (!shm.IsWritable()) {
    NS_RUNTIMEABORT("shmem not writable!");
    return nullptr;
  }
  SharedImageInfo* shmInfo = gfxSharedImageSurface::GetShmInfoPtr(shm);
  unsigned char* data = shm.get<unsigned char>();

  int stride = gfxASurface::FormatStrideForWidth(
      static_cast<gfxASurface::gfxImageFormat>(shmInfo->format), aSize.width);

  return gfxPlatform::GetPlatform()->CreateDrawTargetForData(data, aSize, stride, aFormat);
}

void
BasicShadowableThebesLayer::Disconnect()
{
  mBackBuffer = SurfaceDescriptor();
  BasicShadowableLayer::Disconnect();
}


class BasicShadowThebesLayer : public ShadowThebesLayer, public BasicImplData {
public:
  BasicShadowThebesLayer(BasicShadowLayerManager* aLayerManager)
    : ShadowThebesLayer(aLayerManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicShadowThebesLayer);
  }
  virtual ~BasicShadowThebesLayer()
  {
    
    
    
    MOZ_COUNT_DTOR(BasicShadowThebesLayer);
  }

  virtual void SetValidRegion(const nsIntRegion& aRegion)
  {
    mOldValidRegion = mValidRegion;
    ShadowThebesLayer::SetValidRegion(aRegion);
  }

  virtual void Disconnect()
  {
    DestroyFrontBuffer();
    ShadowThebesLayer::Disconnect();
  }

  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       OptionalThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       OptionalThebesBuffer* aReadOnlyFront, nsIntRegion* aFrontUpdatedRegion);

  virtual void DestroyFrontBuffer()
  {
    mFrontBuffer.Clear();
    mValidRegion.SetEmpty();
    mOldValidRegion.SetEmpty();

    if (IsSurfaceDescriptorValid(mFrontBufferDescriptor)) {
      mAllocator->DestroySharedSurface(&mFrontBufferDescriptor);
    }
  }

  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMaskLayer,
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
};

void
BasicShadowThebesLayer::Swap(const ThebesBuffer& aNewFront,
                             const nsIntRegion& aUpdatedRegion,
                             OptionalThebesBuffer* aNewBack,
                             nsIntRegion* aNewBackValidRegion,
                             OptionalThebesBuffer* aReadOnlyFront,
                             nsIntRegion* aFrontUpdatedRegion)
{
  if (IsSurfaceDescriptorValid(mFrontBufferDescriptor)) {
    AutoOpenSurface autoNewFrontBuffer(OPEN_READ_ONLY, aNewFront.buffer());
    AutoOpenSurface autoCurrentFront(OPEN_READ_ONLY, mFrontBufferDescriptor);
    if (autoCurrentFront.Size() != autoNewFrontBuffer.Size()) {
      
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

  nsIntRect backRect;
  nsIntPoint backRotation;
  mFrontBuffer.Swap(
    aNewFront.rect(), aNewFront.rotation(),
    &backRect, &backRotation);

  if (aNewBack->type() != OptionalThebesBuffer::Tnull_t) {
    aNewBack->get_ThebesBuffer().rect() = backRect;
    aNewBack->get_ThebesBuffer().rotation() = backRotation;
  }

  mFrontBufferDescriptor = aNewFront.buffer();

  *aReadOnlyFront = aNewFront;
  *aFrontUpdatedRegion = aUpdatedRegion;
}

void
BasicShadowThebesLayer::PaintThebes(gfxContext* aContext,
                                    Layer* aMaskLayer,
                                    LayerManager::DrawThebesLayerCallback aCallback,
                                    void* aCallbackData,
                                    ReadbackProcessor* aReadback)
{
  NS_ASSERTION(BasicManager()->InDrawing(),
               "Can only draw in drawing phase");
  NS_ASSERTION(BasicManager()->IsRetained(),
               "ShadowThebesLayer makes no sense without retained mode");

  if (!IsSurfaceDescriptorValid(mFrontBufferDescriptor)) {
    return;
  }

  AutoOpenSurface autoFrontBuffer(OPEN_READ_ONLY, mFrontBufferDescriptor);
  mFrontBuffer.ProvideBuffer(&autoFrontBuffer);

  mFrontBuffer.DrawTo(this, aContext, GetEffectiveOpacity(), aMaskLayer);

  mFrontBuffer.RevokeBuffer();
}

already_AddRefed<ThebesLayer>
BasicLayerManager::CreateThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ThebesLayer> layer = new BasicThebesLayer(this);
  return layer.forget();
}

already_AddRefed<ShadowThebesLayer>
BasicShadowLayerManager::CreateShadowThebesLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ShadowThebesLayer> layer = new BasicShadowThebesLayer(this);
  return layer.forget();
}

}
}
