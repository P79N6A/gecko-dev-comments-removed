







































#ifndef mozilla_layers_ShadowLayers_h
#define mozilla_layers_ShadowLayers_h 1

#include "gfxASurface.h"

#include "ImageLayers.h"
#include "Layers.h"

class gfxSharedImageSurface;

namespace mozilla {
namespace layers {

struct Edit;
struct EditReply;
class PLayerChild;
class PLayersChild;
class PLayersParent;
class ShadowableLayer;
class ShadowThebesLayer;
class ShadowImageLayer;
class ShadowCanvasLayer;
class SurfaceDescriptor;
class ThebesBuffer;
class Transaction;










































class ShadowLayerForwarder
{
public:
  virtual ~ShadowLayerForwarder();

  



  void BeginTransaction();

  





  




  void CreatedThebesLayer(ShadowableLayer* aThebes);
  void CreatedContainerLayer(ShadowableLayer* aContainer);
  void CreatedImageLayer(ShadowableLayer* aImage);
  void CreatedColorLayer(ShadowableLayer* aColor);
  void CreatedCanvasLayer(ShadowableLayer* aCanvas);

  












  


  void CreatedThebesBuffer(ShadowableLayer* aThebes,
                           const nsIntRegion& aFrontValidRegion,
                           float aXResolution, float aYResolution,
                           const nsIntRect& aBufferRect,
                           const SurfaceDescriptor& aInitialFrontBuffer);
  



  void CreatedImageBuffer(ShadowableLayer* aImage,
                          nsIntSize aSize,
                          gfxSharedImageSurface* aInitialFrontSurface);
  void CreatedCanvasBuffer(ShadowableLayer* aCanvas,
                           nsIntSize aSize,
                           gfxSharedImageSurface* aInitialFrontSurface);

  







  void DestroyedThebesBuffer(ShadowableLayer* aThebes,
                             const SurfaceDescriptor& aBackBufferToDestroy);
  void DestroyedImageBuffer(ShadowableLayer* aImage);
  void DestroyedCanvasBuffer(ShadowableLayer* aCanvas);


  




  void Mutated(ShadowableLayer* aMutant);

  void SetRoot(ShadowableLayer* aRoot);
  




  void InsertAfter(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild,
                   ShadowableLayer* aAfter=NULL);
  void RemoveChild(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild);

  






  




  void PaintedThebesBuffer(ShadowableLayer* aThebes,
                           const nsIntRegion& aUpdatedRegion,
                           const nsIntRect& aBufferRect,
                           const nsIntPoint& aBufferRotation,
                           const SurfaceDescriptor& aNewFrontBuffer);
  



  void PaintedImage(ShadowableLayer* aImage,
                    gfxSharedImageSurface* aNewFrontSurface);
  void PaintedCanvas(ShadowableLayer* aCanvas,
                     gfxSharedImageSurface* aNewFrontSurface);

  




  PRBool EndTransaction(nsTArray<EditReply>* aReplies);

  


  PRBool HasShadowManager() const { return !!mShadowManager; }

  
































  






  PRBool AllocDoubleBuffer(const gfxIntSize& aSize,
                           gfxASurface::gfxContentType aContent,
                           gfxSharedImageSurface** aFrontBuffer,
                           gfxSharedImageSurface** aBackBuffer);
  void DestroySharedSurface(gfxSharedImageSurface* aSurface);

  



  PRBool AllocDoubleBuffer(const gfxIntSize& aSize,
                           gfxASurface::gfxContentType aContent,
                           SurfaceDescriptor* aFrontBuffer,
                           SurfaceDescriptor* aBackBuffer);

  static already_AddRefed<gfxASurface>
  OpenDescriptor(const SurfaceDescriptor& aSurface);

  void DestroySharedSurface(SurfaceDescriptor* aSurface);

  



  PLayerChild* ConstructShadowFor(ShadowableLayer* aLayer);

protected:
  ShadowLayerForwarder();

  PLayersChild* mShadowManager;

private:
  PRBool PlatformAllocDoubleBuffer(const gfxIntSize& aSize,
                                   gfxASurface::gfxContentType aContent,
                                   SurfaceDescriptor* aFrontBuffer,
                                   SurfaceDescriptor* aBackBuffer);

  static already_AddRefed<gfxASurface>
  PlatformOpenDescriptor(const SurfaceDescriptor& aDescriptor);

  PRBool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);

  static void PlatformSyncBeforeUpdate();

  Transaction* mTxn;
};


class ShadowLayerManager : public LayerManager
{
public:
  virtual ~ShadowLayerManager() {}

  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Shadow"); }

  void DestroySharedSurface(gfxSharedImageSurface* aSurface,
                            PLayersParent* aDeallocator);

  void DestroySharedSurface(SurfaceDescriptor* aSurface,
                            PLayersParent* aDeallocator);

  
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer() = 0;
  
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer() = 0;
  
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer() = 0;

  static void PlatformSyncBeforeReplyUpdate();

protected:
  ShadowLayerManager() {}

  PRBool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);
};









class ShadowableLayer
{
public:
  virtual ~ShadowableLayer() {}

  virtual Layer* AsLayer() = 0;

  


  PRBool HasShadow() { return !!mShadow; }

  



  PLayerChild* GetShadow() { return mShadow; }

protected:
  ShadowableLayer() : mShadow(NULL) {}

  PLayerChild* mShadow;
};










class ShadowLayer
{
public:
  virtual ~ShadowLayer() {}

  


  void SetAllocator(PLayersParent* aAllocator)
  {
    NS_ABORT_IF_FALSE(!mAllocator, "Stomping allocator?");
    mAllocator = aAllocator;
  }

  






  void SetShadowVisibleRegion(const nsIntRegion& aRegion)
  {
    mShadowVisibleRegion = aRegion;
  }

  void SetShadowClipRect(const nsIntRect* aRect)
  {
    mUseShadowClipRect = aRect != nsnull;
    if (aRect) {
      mShadowClipRect = *aRect;
    }
  }

  void SetShadowTransform(const gfx3DMatrix& aMatrix)
  {
    mShadowTransform = aMatrix;
  }

  
  const nsIntRect* GetShadowClipRect() { return mUseShadowClipRect ? &mShadowClipRect : nsnull; }
  const nsIntRegion& GetShadowVisibleRegion() { return mShadowVisibleRegion; }
  const gfx3DMatrix& GetShadowTransform() { return mShadowTransform; }

protected:
  ShadowLayer()
    : mAllocator(nsnull)
    , mUseShadowClipRect(PR_FALSE)
  {}

  PLayersParent* mAllocator;
  nsIntRegion mShadowVisibleRegion;
  gfx3DMatrix mShadowTransform;
  nsIntRect mShadowClipRect;
  PRPackedBool mUseShadowClipRect;
};


class ShadowThebesLayer : public ShadowLayer,
                          public ThebesLayer
{
public:
  





  virtual void SetFrontBuffer(const ThebesBuffer& aNewFront,
                              const nsIntRegion& aValidRegion,
                              float aXResolution, float aYResolution) = 0;

  virtual void InvalidateRegion(const nsIntRegion& aRegion)
  {
    NS_RUNTIMEABORT("ShadowThebesLayers can't fill invalidated regions");
  }

  


  virtual void SetValidRegion(const nsIntRegion& aRegion)
  {
    mValidRegion = aRegion;
    Mutated();
  }

  


  virtual void SetResolution(float aXResolution, float aYResolution)
  {
    mXResolution = aXResolution;
    mYResolution = aYResolution;
    Mutated();
  }

  






  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       ThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       float* aNewXResolution, float* aNewYResolution) = 0;

  




  virtual void DestroyFrontBuffer() = 0;

  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowThebesLayer", TYPE_SHADOW)

protected:
  ShadowThebesLayer(LayerManager* aManager, void* aImplData)
    : ThebesLayer(aManager, aImplData)
  {}
};


class ShadowCanvasLayer : public ShadowLayer,
                          public CanvasLayer
{
public:
  






  virtual already_AddRefed<gfxSharedImageSurface>
  Swap(gfxSharedImageSurface* aNewFront) = 0;

  




  virtual void DestroyFrontBuffer() = 0;

  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowCanvasLayer", TYPE_SHADOW)

protected:
  ShadowCanvasLayer(LayerManager* aManager, void* aImplData)
    : CanvasLayer(aManager, aImplData)
  {}
};


class ShadowImageLayer : public ShadowLayer,
                         public ImageLayer
{
public:
  







  virtual PRBool Init(gfxSharedImageSurface* aFront, const nsIntSize& aSize) = 0;

  



  virtual already_AddRefed<gfxSharedImageSurface>
  Swap(gfxSharedImageSurface* newFront) = 0;

  




  virtual void DestroyFrontBuffer() = 0;

  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowImageLayer", TYPE_SHADOW)

protected:
  ShadowImageLayer(LayerManager* aManager, void* aImplData)
    : ImageLayer(aManager, aImplData)
  {}
};


} 
} 

#endif 
