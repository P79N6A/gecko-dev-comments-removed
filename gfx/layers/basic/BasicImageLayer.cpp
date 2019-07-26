




#include "mozilla/layers/PLayersParent.h"
#include "BasicLayersImpl.h"
#include "SharedTextureImage.h"
#include "gfxUtils.h"
#include "gfxSharedImageSurface.h"
#include "mozilla/layers/ImageContainerChild.h"
#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class BasicImageLayer : public ImageLayer, public BasicImplData {
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

  virtual void Paint(gfxContext* aContext, Layer* aMaskLayer);

  static void PaintContext(gfxPattern* aPattern,
                           const nsIntRegion& aVisible,
                           float aOpacity,
                           gfxContext* aContext,
                           Layer* aMaskLayer);

  virtual bool GetAsSurface(gfxASurface** aSurface,
                            SurfaceDescriptor* aDescriptor);

protected:
  BasicLayerManager* BasicManager()
  {
    return static_cast<BasicLayerManager*>(mManager);
  }

  
  already_AddRefed<gfxPattern>
  GetAndPaintCurrentImage(gfxContext* aContext,
                          float aOpacity,
                          Layer* aMaskLayer);

  gfxIntSize mSize;
};

void
BasicImageLayer::Paint(gfxContext* aContext, Layer* aMaskLayer)
{
  if (IsHidden())
    return;
  nsRefPtr<gfxPattern> dontcare =
    GetAndPaintCurrentImage(aContext, GetEffectiveOpacity(), aMaskLayer);
}

already_AddRefed<gfxPattern>
BasicImageLayer::GetAndPaintCurrentImage(gfxContext* aContext,
                                         float aOpacity,
                                         Layer* aMaskLayer)
{
  if (!mContainer)
    return nullptr;

  mContainer->SetImageFactory(mManager->IsCompositingCheap() ? nullptr : BasicManager()->GetImageFactory());

  nsRefPtr<gfxASurface> surface;
  AutoLockImage autoLock(mContainer, getter_AddRefs(surface));
  Image *image = autoLock.GetImage();
  gfxIntSize size = mSize = autoLock.GetSize();

  if (!surface || surface->CairoStatus()) {
    return nullptr;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);
  if (!pat) {
    return nullptr;
  }

  pat->SetFilter(mFilter);

  
  
  if (aContext) {
    AutoSetOperator setOperator(aContext, GetOperator());
    PaintContext(pat,
                 nsIntRegion(nsIntRect(0, 0, size.width, size.height)),
                 aOpacity, aContext, aMaskLayer);

    GetContainer()->NotifyPaintedImage(image);
  }

  return pat.forget();
}

 void
BasicImageLayer::PaintContext(gfxPattern* aPattern,
                              const nsIntRegion& aVisible,
                              float aOpacity,
                              gfxContext* aContext,
                              Layer* aMaskLayer)
{
  
  
  gfxPattern::GraphicsExtend extend = gfxPattern::EXTEND_PAD;

#ifdef MOZ_X11
  
  
  if (aContext->IsCairo()) {
    nsRefPtr<gfxASurface> target = aContext->CurrentSurface();
    if (target->GetType() == gfxASurface::SurfaceTypeXlib &&
        static_cast<gfxXlibSurface*>(target.get())->IsPadSlow()) {
      extend = gfxPattern::EXTEND_NONE;
    }
  }
#endif

  aContext->NewPath();
  
  
  gfxUtils::PathFromRegion(aContext, aVisible);
  aPattern->SetExtend(extend);
  aContext->SetPattern(aPattern);
  FillWithMask(aContext, aOpacity, aMaskLayer);

  
  aPattern->SetExtend(extend);
}

bool
BasicImageLayer::GetAsSurface(gfxASurface** aSurface,
                              SurfaceDescriptor* aDescriptor)
{
  if (!mContainer) {
    return false;
  }

  gfxIntSize dontCare;
  nsRefPtr<gfxASurface> surface = mContainer->GetCurrentAsSurface(&dontCare);
  *aSurface = surface.forget().get();
  return true;
}

class BasicShadowableImageLayer : public BasicImageLayer,
                                  public BasicShadowableLayer
{
public:
  BasicShadowableImageLayer(BasicShadowLayerManager* aManager) :
    BasicImageLayer(aManager),
    mBufferIsOpaque(false),
    mLastPaintedImageSerial(0)
  {
    MOZ_COUNT_CTOR(BasicShadowableImageLayer);
  }
  virtual ~BasicShadowableImageLayer()
  {
    DestroyBackBuffer();
    MOZ_COUNT_DTOR(BasicShadowableImageLayer);
  }

  virtual void Paint(gfxContext* aContext, Layer* aMaskLayer);

  virtual void ClearCachedResources() MOZ_OVERRIDE
  {
    DestroyBackBuffer();
  }

  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs)
  {
    aAttrs = ImageLayerAttributes(mFilter, mForceSingleTile);
  }

  virtual Layer* AsLayer() { return this; }
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void SetBackBuffer(const SurfaceDescriptor& aBuffer)
  {
    mBackBuffer = aBuffer;
  }

  virtual void SetBackBufferYUVImage(const SurfaceDescriptor& aYBuffer,
                                     const SurfaceDescriptor& aUBuffer,
                                     const SurfaceDescriptor& aVBuffer)
  {
    mBackBufferY = aYBuffer;
    mBackBufferU = aUBuffer;
    mBackBufferV = aVBuffer;
  }

  virtual void Disconnect()
  {
    mBackBufferY = SurfaceDescriptor();
    mBackBufferU = SurfaceDescriptor();
    mBackBufferV = SurfaceDescriptor();
    mBackBuffer = SurfaceDescriptor();
    BasicShadowableLayer::Disconnect();
  }

  void DestroyBackBuffer()
  {
    if (IsSurfaceDescriptorValid(mBackBuffer)) {
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBuffer);
    }
    if (IsSurfaceDescriptorValid(mBackBufferY)) {
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBufferY);
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBufferU);
      BasicManager()->ShadowLayerForwarder::DestroySharedSurface(&mBackBufferV);
}
  }

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  
  
  SurfaceDescriptor mBackBuffer;
  bool mBufferIsOpaque;
  SurfaceDescriptor mBackBufferY;
  SurfaceDescriptor mBackBufferU;
  SurfaceDescriptor mBackBufferV;
  gfxIntSize mCbCrSize;
  int32_t mLastPaintedImageSerial;
};
 
void
BasicShadowableImageLayer::Paint(gfxContext* aContext, Layer* aMaskLayer)
{
  if (!HasShadow()) {
    BasicImageLayer::Paint(aContext, aMaskLayer);
    return;
  }

  if (!mContainer) {
    return;
  }

  if (mContainer->IsAsync()) {
    uint32_t containerID = mContainer->GetAsyncContainerID();
    BasicManager()->PaintedImage(BasicManager()->Hold(this), 
                                 SharedImageID(containerID));
    AutoLockImage autoLock(mContainer);
    mContainer->NotifyPaintedImage(autoLock.GetImage());
    return;
  }

  nsRefPtr<gfxASurface> surface;
  AutoLockImage autoLock(mContainer, getter_AddRefs(surface));

  Image *image = autoLock.GetImage();

  if (!image) {
    return;
  }

  if (aMaskLayer) {
    static_cast<BasicImplData*>(aMaskLayer->ImplData())
      ->Paint(aContext, nullptr);
  }

  if (image->GetFormat() == SHARED_TEXTURE &&
      BasicManager()->GetParentBackendType() == mozilla::layers::LAYERS_OPENGL) {
    SharedTextureImage *sharedImage = static_cast<SharedTextureImage*>(image);
    const SharedTextureImage::Data *data = sharedImage->GetData();

    SharedTextureDescriptor texture(data->mShareType, data->mHandle, data->mSize, data->mInverted);
    SurfaceDescriptor descriptor(texture);
    BasicManager()->PaintedImage(BasicManager()->Hold(this), descriptor);
    mContainer->NotifyPaintedImage(image);
    return;
  }

  if (image->GetFormat() == PLANAR_YCBCR && BasicManager()->IsCompositingCheap()) {
    PlanarYCbCrImage *YCbCrImage = static_cast<PlanarYCbCrImage*>(image);
    const PlanarYCbCrImage::Data *data = YCbCrImage->GetData();
    NS_ASSERTION(data, "Must be able to retrieve yuv data from image!");

    if (mSize != data->mYSize || mCbCrSize != data->mCbCrSize || !IsSurfaceDescriptorValid(mBackBufferY)) {
      DestroyBackBuffer();
      mSize = data->mYSize;
      mCbCrSize = data->mCbCrSize;

      
      if (!BasicManager()->AllocBufferWithCaps(mSize,
                                               gfxASurface::CONTENT_ALPHA,
                                               MAP_AS_IMAGE_SURFACE,
                                               &mBackBufferY) ||
          !BasicManager()->AllocBufferWithCaps(mCbCrSize,
                                               gfxASurface::CONTENT_ALPHA,
                                               MAP_AS_IMAGE_SURFACE,
                                               &mBackBufferU) ||
          !BasicManager()->AllocBufferWithCaps(mCbCrSize,
                                               gfxASurface::CONTENT_ALPHA,
                                               MAP_AS_IMAGE_SURFACE,
                                               &mBackBufferV)) {
        NS_RUNTIMEABORT("creating ImageLayer 'front buffer' failed!");
      }
    }

    AutoOpenSurface dyas(OPEN_READ_WRITE, mBackBufferY);
    gfxImageSurface* dy = dyas.GetAsImage();

    for (int i = 0; i < data->mYSize.height; i++) {
      memcpy(dy->Data() + i * dy->Stride(),
             data->mYChannel + i * data->mYStride,
             data->mYSize.width);
    }

    AutoOpenSurface duas(OPEN_READ_WRITE, mBackBufferU);
    gfxImageSurface* du = duas.GetAsImage();
    AutoOpenSurface dvas(OPEN_READ_WRITE, mBackBufferV);
    gfxImageSurface* dv = dvas.GetAsImage();

    for (int i = 0; i < data->mCbCrSize.height; i++) {
      memcpy(du->Data() + i * du->Stride(),
             data->mCbChannel + i * data->mCbCrStride,
             data->mCbCrSize.width);
      memcpy(dv->Data() + i * dv->Stride(),
             data->mCrChannel + i * data->mCbCrStride,
             data->mCbCrSize.width);
    }

    YUVImage yuv(mBackBufferY, mBackBufferU, mBackBufferV,
                 data->GetPictureRect());

    BasicManager()->PaintedImage(BasicManager()->Hold(this),
                                 yuv);
    mContainer->NotifyPaintedImage(image);
    return;
  }

  gfxIntSize oldSize = mSize;
  nsRefPtr<gfxPattern> pat = GetAndPaintCurrentImage
    (aContext, GetEffectiveOpacity(), nullptr);
  if (!pat)
    return;

  bool isOpaque = (GetContentFlags() & CONTENT_OPAQUE);
  if (oldSize != mSize || 
      !IsSurfaceDescriptorValid(mBackBuffer) ||
      isOpaque != mBufferIsOpaque) {
    DestroyBackBuffer();
    mBufferIsOpaque = isOpaque;

    gfxASurface::gfxContentType type = gfxASurface::CONTENT_COLOR_ALPHA;
    if (surface) {
      type = surface->GetContentType();
    }
    if (type != gfxASurface::CONTENT_ALPHA &&
        isOpaque) {
      type = gfxASurface::CONTENT_COLOR;
    }

    if (!BasicManager()->AllocBuffer(mSize, type, &mBackBuffer))
      NS_RUNTIMEABORT("creating ImageLayer 'front buffer' failed!");
  } else if (mLastPaintedImageSerial == image->GetSerial()) {
    return;
  }

  AutoOpenSurface backSurface(OPEN_READ_WRITE, mBackBuffer);
  nsRefPtr<gfxContext> tmpCtx = new gfxContext(backSurface.Get());
  tmpCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
  PaintContext(pat,
               nsIntRegion(nsIntRect(0, 0, mSize.width, mSize.height)),
               1.0, tmpCtx, nullptr);

  BasicManager()->PaintedImage(BasicManager()->Hold(this),
                               mBackBuffer);
  mLastPaintedImageSerial = image->GetSerial();
}

class BasicShadowImageLayer : public ShadowImageLayer, public BasicImplData {
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

  virtual void Swap(const SharedImage& aNewFront,
                    SharedImage* aNewBack);

  virtual void DestroyFrontBuffer()
  {
    if (mAllocator && IsSurfaceDescriptorValid(mFrontBuffer)) {
      mAllocator->DestroySharedSurface(&mFrontBuffer);
    }
  }

  virtual void Paint(gfxContext* aContext, Layer* aMaskLayer);
  virtual bool GetAsSurface(gfxASurface** aSurface,
                            SurfaceDescriptor* aDescriptor);

protected:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  SurfaceDescriptor mFrontBuffer;
  gfxIntSize mSize;
};

void
BasicShadowImageLayer::Swap(const SharedImage& aNewFront,
                            SharedImage* aNewBack)
{
  AutoOpenSurface autoSurface(OPEN_READ_ONLY, aNewFront);
  
  bool surfaceConfigChanged = autoSurface.Size() != mSize;
  if (IsSurfaceDescriptorValid(mFrontBuffer)) {
    AutoOpenSurface autoFront(OPEN_READ_ONLY, mFrontBuffer);
    surfaceConfigChanged = surfaceConfigChanged ||
                           autoSurface.ContentType() != autoFront.ContentType();
  }
  if (surfaceConfigChanged) {
    DestroyFrontBuffer();
    mSize = autoSurface.Size();
  }

  
  if (IsSurfaceDescriptorValid(mFrontBuffer)) {
    *aNewBack = mFrontBuffer;
  } else {
    *aNewBack = null_t();
  }
  mFrontBuffer = aNewFront;
}

void
BasicShadowImageLayer::Paint(gfxContext* aContext, Layer* aMaskLayer)
{
  if (!IsSurfaceDescriptorValid(mFrontBuffer)) {
    return;
  }

  AutoOpenSurface autoSurface(OPEN_READ_ONLY, mFrontBuffer);
  nsRefPtr<gfxPattern> pat = new gfxPattern(autoSurface.Get());
  pat->SetFilter(mFilter);

  
  
  AutoSetOperator setOperator(aContext, GetOperator());
  BasicImageLayer::PaintContext(pat,
                                nsIntRegion(nsIntRect(0, 0, mSize.width, mSize.height)),
                                GetEffectiveOpacity(), aContext,
                                aMaskLayer);
}

bool
BasicShadowImageLayer::GetAsSurface(gfxASurface** aSurface,
                                    SurfaceDescriptor* aDescriptor)
{
  if (!IsSurfaceDescriptorValid(mFrontBuffer)) {
    return false;
  }

  *aDescriptor = mFrontBuffer;
  return true;
 }

already_AddRefed<ImageLayer>
BasicLayerManager::CreateImageLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ImageLayer> layer = new BasicImageLayer(this);
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

already_AddRefed<ShadowImageLayer>
BasicShadowLayerManager::CreateShadowImageLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ShadowImageLayer> layer = new BasicShadowImageLayer(this);
  return layer.forget();
}

}
}
