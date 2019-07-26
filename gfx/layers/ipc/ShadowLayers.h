






#ifndef mozilla_layers_ShadowLayers_h
#define mozilla_layers_ShadowLayers_h 1

#include "gfxASurface.h"

#include "ImageLayers.h"
#include "Layers.h"
#include "mozilla/ipc/SharedMemory.h"

class gfxSharedImageSurface;

namespace mozilla {
namespace layers {

class Edit;
class EditReply;
class OptionalThebesBuffer;
class PLayerChild;
class PLayersChild;
class PLayersParent;
class ShadowableLayer;
class ShadowThebesLayer;
class ShadowContainerLayer;
class ShadowImageLayer;
class ShadowColorLayer;
class ShadowCanvasLayer;
class ShadowRefLayer;
class SurfaceDescriptor;
class ThebesBuffer;
class TiledLayerComposer;
class Transaction;
class SharedImage;
class CanvasSurface;
class BasicTiledLayerBuffer;

enum BufferCapabilities {
  DEFAULT_BUFFER_CAPS = 0,
  



  MAP_AS_IMAGE_SURFACE = 1 << 0
};

enum OpenMode {
  OPEN_READ_ONLY,
  OPEN_READ_WRITE
};










































class ShadowLayerForwarder
{
  friend class AutoOpenSurface;

public:
  typedef gfxASurface::gfxContentType gfxContentType;
  typedef LayerManager::LayersBackend LayersBackend;

  virtual ~ShadowLayerForwarder();

  



  void BeginTransaction();

  





  




  void CreatedThebesLayer(ShadowableLayer* aThebes);
  void CreatedContainerLayer(ShadowableLayer* aContainer);
  void CreatedImageLayer(ShadowableLayer* aImage);
  void CreatedColorLayer(ShadowableLayer* aColor);
  void CreatedCanvasLayer(ShadowableLayer* aCanvas);
  void CreatedRefLayer(ShadowableLayer* aRef);

  







  void DestroyedThebesBuffer(ShadowableLayer* aThebes,
                             const SurfaceDescriptor& aBackBufferToDestroy);

  




  void Mutated(ShadowableLayer* aMutant);

  void SetRoot(ShadowableLayer* aRoot);
  




  void InsertAfter(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild,
                   ShadowableLayer* aAfter=NULL);
  void RemoveChild(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild);

  






  void SetMask(ShadowableLayer* aLayer,
               ShadowableLayer* aMaskLayer);

  






  




  void PaintedThebesBuffer(ShadowableLayer* aThebes,
                           const nsIntRegion& aUpdatedRegion,
                           const nsIntRect& aBufferRect,
                           const nsIntPoint& aBufferRotation,
                           const SurfaceDescriptor& aNewFrontBuffer);

  






  void PaintedTiledLayerBuffer(ShadowableLayer* aThebes,
                               BasicTiledLayerBuffer* aTiledLayerBuffer);

  



  void PaintedImage(ShadowableLayer* aImage,
                    const SharedImage& aNewFrontImage);
  void PaintedCanvas(ShadowableLayer* aCanvas,
                     bool aNeedYFlip,
                     const SurfaceDescriptor& aNewFrontSurface);

  




  bool EndTransaction(InfallibleTArray<EditReply>* aReplies);

  


  bool ShadowDrawToTarget(gfxContext* aTarget);

  


  void SetShadowManager(PLayersChild* aShadowManager)
  {
    mShadowManager = aShadowManager;
  }

  void SetParentBackendType(LayersBackend aBackendType)
  {
    mParentBackend = aBackendType;
  }

  


  bool HasShadowManager() const { return !!mShadowManager; }
  PLayersChild* GetShadowManager() const { return mShadowManager; }

  
































  






  bool AllocBuffer(const gfxIntSize& aSize,
                   gfxASurface::gfxContentType aContent,
                   SurfaceDescriptor* aBuffer);

  bool AllocBufferWithCaps(const gfxIntSize& aSize,
                           gfxASurface::gfxContentType aContent,
                           uint32_t aCaps,
                           SurfaceDescriptor* aBuffer);

  void DestroySharedSurface(SurfaceDescriptor* aSurface);

  



  PLayerChild* ConstructShadowFor(ShadowableLayer* aLayer);

  LayersBackend GetParentBackendType()
  {
    return mParentBackend;
  }

  


  void SetIsFirstPaint() { mIsFirstPaint = true; }

  virtual PRInt32 GetMaxTextureSize() const { return mMaxTextureSize; }
  void SetMaxTextureSize(PRInt32 aMaxTextureSize) { mMaxTextureSize = aMaxTextureSize; }

protected:
  ShadowLayerForwarder();

  PLayersChild* mShadowManager;

private:
  bool AllocBuffer(const gfxIntSize& aSize,
                   gfxASurface::gfxContentType aContent,
                   gfxSharedImageSurface** aBuffer);

  bool PlatformAllocBuffer(const gfxIntSize& aSize,
                           gfxASurface::gfxContentType aContent,
                           uint32_t aCaps,
                           SurfaceDescriptor* aBuffer);

  



  static gfxContentType
  GetDescriptorSurfaceContentType(const SurfaceDescriptor& aDescriptor,
                                  OpenMode aMode,
                                  gfxASurface** aSurface);
  




  static bool
  PlatformGetDescriptorSurfaceContentType(const SurfaceDescriptor& aDescriptor,
                                          OpenMode aMode,
                                          gfxContentType* aContent,
                                          gfxASurface** aSurface);
  
  static gfxIntSize
  GetDescriptorSurfaceSize(const SurfaceDescriptor& aDescriptor,
                           OpenMode aMode,
                           gfxASurface** aSurface);
  static bool
  PlatformGetDescriptorSurfaceSize(const SurfaceDescriptor& aDescriptor,
                                   OpenMode aMode,
                                   gfxIntSize* aSize,
                                   gfxASurface** aSurface);

  static already_AddRefed<gfxASurface>
  OpenDescriptor(OpenMode aMode, const SurfaceDescriptor& aSurface);

  static already_AddRefed<gfxASurface>
  PlatformOpenDescriptor(OpenMode aMode, const SurfaceDescriptor& aDescriptor);

  

  static void
  CloseDescriptor(const SurfaceDescriptor& aDescriptor);

  static bool
  PlatformCloseDescriptor(const SurfaceDescriptor& aDescriptor);

  bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);

  static void PlatformSyncBeforeUpdate();

  Transaction* mTxn;
  PRInt32 mMaxTextureSize;
  LayersBackend mParentBackend;

  bool mIsFirstPaint;
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
  
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer() = 0;
  
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer() = 0;
  
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer() = 0;
  
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer() = 0;
  
  virtual already_AddRefed<ShadowRefLayer> CreateShadowRefLayer() { return nsnull; }

  static void PlatformSyncBeforeReplyUpdate();

  void SetCompositorID(PRUint32 aID)
  {
    NS_ASSERTION(mCompositorID==0, "The compositor ID must be set only once.");
    mCompositorID = aID;
  }
  PRUint32 GetCompositorID() const
  {
    return mCompositorID;
  }

protected:
  ShadowLayerManager()
  : mCompositorID(0) {}

  bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);
  PRUint32 mCompositorID;
};









class ShadowableLayer
{
public:
  virtual ~ShadowableLayer() {}

  virtual Layer* AsLayer() = 0;

  


  bool HasShadow() { return !!mShadow; }

  



  PLayerChild* GetShadow() { return mShadow; }

protected:
  ShadowableLayer() : mShadow(NULL) {}

  PLayerChild* mShadow;
};




class ISurfaceDeAllocator
{
public:
  virtual void DestroySharedSurface(gfxSharedImageSurface* aSurface) = 0;
  virtual void DestroySharedSurface(SurfaceDescriptor* aSurface) = 0;
protected:
  ~ISurfaceDeAllocator() {};
};









class ShadowLayer
{
public:
  virtual ~ShadowLayer() {}

  




  virtual void SetAllocator(ISurfaceDeAllocator* aAllocator)
  {
    NS_ASSERTION(!mAllocator || mAllocator == aAllocator, "Stomping allocator?");
    mAllocator = aAllocator;
  }

  virtual void DestroyFrontBuffer() { };

  






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

  virtual TiledLayerComposer* AsTiledLayerComposer() { return NULL; }

protected:
  ShadowLayer()
    : mAllocator(nsnull)
    , mUseShadowClipRect(false)
  {}

  ISurfaceDeAllocator* mAllocator;
  nsIntRegion mShadowVisibleRegion;
  gfx3DMatrix mShadowTransform;
  nsIntRect mShadowClipRect;
  bool mUseShadowClipRect;
};


class ShadowThebesLayer : public ShadowLayer,
                          public ThebesLayer
{
public:
  virtual void InvalidateRegion(const nsIntRegion& aRegion)
  {
    NS_RUNTIMEABORT("ShadowThebesLayers can't fill invalidated regions");
  }

  


  virtual void SetValidRegion(const nsIntRegion& aRegion)
  {
    mValidRegion = aRegion;
    Mutated();
  }

  






  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       OptionalThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       OptionalThebesBuffer* aReadOnlyFront, nsIntRegion* aFrontUpdatedRegion) = 0;

  




  virtual void DestroyFrontBuffer() = 0;

  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowThebesLayer", TYPE_SHADOW)

protected:
  ShadowThebesLayer(LayerManager* aManager, void* aImplData)
    : ThebesLayer(aManager, aImplData)
  {}
};


class ShadowContainerLayer : public ShadowLayer,
                             public ContainerLayer
{
public:
  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowContainerLayer", TYPE_SHADOW)

protected:
  ShadowContainerLayer(LayerManager* aManager, void* aImplData)
    : ContainerLayer(aManager, aImplData)
  {}
};


class ShadowCanvasLayer : public ShadowLayer,
                          public CanvasLayer
{
public:
  






  virtual void Swap(const CanvasSurface& aNewFront, bool needYFlip,
                    CanvasSurface* aNewBack) = 0;

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
  



  virtual void Swap(const SharedImage& aFront,
                    SharedImage* aNewBack) = 0;

  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowImageLayer", TYPE_SHADOW)

protected:
  ShadowImageLayer(LayerManager* aManager, void* aImplData)
    : ImageLayer(aManager, aImplData), 
      mImageContainerID(0),
      mImageVersion(0)
  {}

  
  PRUint32 mImageContainerID;
  PRUint32 mImageVersion;
};


class ShadowColorLayer : public ShadowLayer,
                         public ColorLayer
{
public:
  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowColorLayer", TYPE_SHADOW)

protected:
  ShadowColorLayer(LayerManager* aManager, void* aImplData)
    : ColorLayer(aManager, aImplData)
  {}
};

class ShadowRefLayer : public ShadowLayer,
                       public RefLayer
{
public:
  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowRefLayer", TYPE_SHADOW)

protected:
  ShadowRefLayer(LayerManager* aManager, void* aImplData)
    : RefLayer(aManager, aImplData)
  {}
};

bool IsSurfaceDescriptorValid(const SurfaceDescriptor& aSurface);

ipc::SharedMemory::SharedMemoryType OptimalShmemType();


} 
} 

#endif 
