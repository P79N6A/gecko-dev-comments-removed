






#ifndef mozilla_layers_ShadowLayers_h
#define mozilla_layers_ShadowLayers_h 1

#include "gfxASurface.h"
#include "GLDefs.h"

#include "ImageLayers.h"
#include "mozilla/layers/Compositor.h"
#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/WidgetUtils.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "mozilla/layers/CompositableForwarder.h"

class gfxSharedImageSurface;

namespace mozilla {

namespace gl {
class GLContext;
class TextureImage;
}

namespace layers {

class CompositableClient;
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
class SurfaceDescriptor;
class CanvasSurface;
class TextureClientShmem;
class ContentClientRemote;
class CompositableChild;
class ImageClient;
class CanvasClient;
class ContentClient;

enum OpenMode {
  OPEN_READ_ONLY,
  OPEN_READ_WRITE
};












































































class ShadowLayerForwarder : public CompositableForwarder
{
  friend class AutoOpenSurface;
  friend class TextureClientShmem;

public:
  typedef gfxASurface::gfxContentType gfxContentType;

  virtual ~ShadowLayerForwarder();

  



  void Connect(CompositableClient* aCompositable);

  virtual void CreatedSingleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aDescriptorOnWhite = nullptr) MOZ_OVERRIDE;
  virtual void CreatedDoubleBuffer(CompositableClient* aCompositable,
                                   const SurfaceDescriptor& aFrontDescriptor,
                                   const SurfaceDescriptor& aBackDescriptor,
                                   const TextureInfo& aTextureInfo,
                                   const SurfaceDescriptor* aFrontDescriptorOnWhite = nullptr,
                                   const SurfaceDescriptor* aBackDescriptorOnWhite = nullptr) MOZ_OVERRIDE;
  virtual void DestroyThebesBuffer(CompositableClient* aCompositable) MOZ_OVERRIDE;

  




  void Attach(CompositableClient* aCompositable,
              ShadowableLayer* aLayer);

  







  void AttachAsyncCompositable(uint64_t aCompositableID,
                               ShadowableLayer* aLayer);

  



  void BeginTransaction(const nsIntRect& aTargetBounds,
                        ScreenRotation aRotation,
                        const nsIntRect& aClientBounds,
                        mozilla::dom::ScreenOrientation aOrientation);

  





  




  void CreatedThebesLayer(ShadowableLayer* aThebes);
  void CreatedContainerLayer(ShadowableLayer* aContainer);
  void CreatedImageLayer(ShadowableLayer* aImage);
  void CreatedColorLayer(ShadowableLayer* aColor);
  void CreatedCanvasLayer(ShadowableLayer* aCanvas);
  void CreatedRefLayer(ShadowableLayer* aRef);

  







  virtual void DestroyedThebesBuffer(const SurfaceDescriptor& aBackBufferToDestroy) MOZ_OVERRIDE;

  




  void Mutated(ShadowableLayer* aMutant);

  void SetRoot(ShadowableLayer* aRoot);
  




  void InsertAfter(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild,
                   ShadowableLayer* aAfter=NULL);
  void RemoveChild(ShadowableLayer* aContainer,
                   ShadowableLayer* aChild);
  void RepositionChild(ShadowableLayer* aContainer,
                       ShadowableLayer* aChild,
                       ShadowableLayer* aAfter=NULL);

  






  void SetMask(ShadowableLayer* aLayer,
               ShadowableLayer* aMaskLayer);

  






  virtual void PaintedTiledLayerBuffer(CompositableClient* aCompositable,
                                       BasicTiledLayerBuffer* aTiledLayerBuffer) MOZ_OVERRIDE;

  




  void AttachAsyncCompositable(PLayersChild* aLayer, uint64_t aID);

  



  virtual void UpdateTexture(CompositableClient* aCompositable,
                             TextureIdentifier aTextureId,
                             SurfaceDescriptor* aDescriptor) MOZ_OVERRIDE;

  



  virtual void UpdateTextureRegion(CompositableClient* aCompositable,
                                   const ThebesBufferData& aThebesBufferData,
                                   const nsIntRegion& aUpdatedRegion) MOZ_OVERRIDE;

  


  void UpdatePictureRect(CompositableClient* aCompositable,
                         const nsIntRect& aRect);

  




  bool EndTransaction(InfallibleTArray<EditReply>* aReplies);

  


  void SetShadowManager(PLayersChild* aShadowManager)
  {
    mShadowManager = aShadowManager;
  }

  


  bool HasShadowManager() const { return !!mShadowManager; }
  PLayersChild* GetShadowManager() const { return mShadowManager; }

  
































  
  virtual bool AllocUnsafeShmem(size_t aSize,
                                ipc::SharedMemory::SharedMemoryType aType,
                                ipc::Shmem* aShmem) MOZ_OVERRIDE;
  virtual bool AllocShmem(size_t aSize,
                          ipc::SharedMemory::SharedMemoryType aType,
                          ipc::Shmem* aShmem) MOZ_OVERRIDE;
  virtual void DeallocShmem(ipc::Shmem& aShmem) MOZ_OVERRIDE;

  



  PLayerChild* ConstructShadowFor(ShadowableLayer* aLayer);

  


  void SetIsFirstPaint() { mIsFirstPaint = true; }

  static void PlatformSyncBeforeUpdate();

  static already_AddRefed<gfxASurface>
  OpenDescriptor(OpenMode aMode, const SurfaceDescriptor& aSurface);

protected:
  ShadowLayerForwarder();

  PLayersChild* mShadowManager;

#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  virtual PGrallocBufferChild* AllocGrallocBuffer(const gfxIntSize& aSize,
                                                  gfxASurface::gfxContentType aContent,
                                                  MaybeMagicGrallocBufferHandle* aHandle) MOZ_OVERRIDE;
#endif

private:
  



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
  PlatformOpenDescriptor(OpenMode aMode, const SurfaceDescriptor& aDescriptor);

  



  static void
  CloseDescriptor(const SurfaceDescriptor& aDescriptor);

  static bool
  PlatformCloseDescriptor(const SurfaceDescriptor& aDescriptor);

  bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);

  Transaction* mTxn;

  bool mIsFirstPaint;
};

class ShadowLayerManager : public LayerManager
{
public:
  virtual ~ShadowLayerManager() {}

  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Shadow"); }

  
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer() = 0;
  
  virtual already_AddRefed<ShadowContainerLayer> CreateShadowContainerLayer() = 0;
  
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer() = 0;
  
  virtual already_AddRefed<ShadowColorLayer> CreateShadowColorLayer() = 0;
  
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer() = 0;
  
  virtual already_AddRefed<ShadowRefLayer> CreateShadowRefLayer() { return nullptr; }

  virtual void NotifyShadowTreeTransaction() {}

  




  static already_AddRefed<gl::TextureImage>
  OpenDescriptorForDirectTexturing(gl::GLContext* aContext,
                                   const SurfaceDescriptor& aDescriptor,
                                   GLenum aWrapMode);

  



  static bool SupportsDirectTexturing();

  static void PlatformSyncBeforeReplyUpdate();

  void SetCompositorID(uint32_t aID)
  {
    NS_ASSERTION(mCompositor, "No compositor");
    mCompositor->SetCompositorID(aID);
  }

  Compositor* GetCompositor() const
  {
    return mCompositor;
  }

protected:
  ShadowLayerManager()
  : mCompositor(nullptr)
  {}

  bool PlatformDestroySharedSurface(SurfaceDescriptor* aSurface);
  RefPtr<Compositor> mCompositor;
};

class CompositableClient;








class ShadowableLayer
{
public:
  virtual ~ShadowableLayer() {}

  virtual Layer* AsLayer() = 0;

  


  bool HasShadow() { return !!mShadow; }

  



  PLayerChild* GetShadow() { return mShadow; }

  virtual CompositableClient* GetCompositableClient() { return nullptr; }
protected:
  ShadowableLayer() : mShadow(NULL) {}

  PLayerChild* mShadow;
};









class ShadowLayer
{
public:
  virtual ~ShadowLayer() {}

  virtual void DestroyFrontBuffer() { }

  






  void SetShadowVisibleRegion(const nsIntRegion& aRegion)
  {
    mShadowVisibleRegion = aRegion;
  }

  void SetShadowOpacity(float aOpacity)
  {
    mShadowOpacity = aOpacity;
  }

  void SetShadowClipRect(const nsIntRect* aRect)
  {
    mUseShadowClipRect = aRect != nullptr;
    if (aRect) {
      mShadowClipRect = *aRect;
    }
  }

  void SetShadowTransform(const gfx3DMatrix& aMatrix)
  {
    mShadowTransform = aMatrix;
  }

  
  float GetShadowOpacity() { return mShadowOpacity; }
  const nsIntRect* GetShadowClipRect() { return mUseShadowClipRect ? &mShadowClipRect : nullptr; }
  const nsIntRegion& GetShadowVisibleRegion() { return mShadowVisibleRegion; }
  const gfx3DMatrix& GetShadowTransform() { return mShadowTransform; }

protected:
  ShadowLayer()
    : mShadowOpacity(1.0f)
    , mUseShadowClipRect(false)
  {}

  nsIntRegion mShadowVisibleRegion;
  gfx3DMatrix mShadowTransform;
  nsIntRect mShadowClipRect;
  float mShadowOpacity;
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
    MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) ValidRegion", this));
    mValidRegion = aRegion;
    Mutated();
  }

  const nsIntRegion& GetValidRegion() { return mValidRegion; }

  virtual void
  Swap(const ThebesBuffer& aNewFront, const nsIntRegion& aUpdatedRegion,
       OptionalThebesBuffer* aNewBack, nsIntRegion* aNewBackValidRegion,
       OptionalThebesBuffer* aReadOnlyFront, nsIntRegion* aFrontUpdatedRegion) {
    NS_RUNTIMEABORT("should not use layer swap");
  };

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
  






  virtual void Swap(const SurfaceDescriptor& aNewFront, bool needYFlip,
                    SurfaceDescriptor* aNewBack) = 0;

  virtual ShadowLayer* AsShadowLayer() { return this; }

  void SetBounds(nsIntRect aBounds) { mBounds = aBounds; }

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
  



  virtual ShadowLayer* AsShadowLayer() { return this; }

  MOZ_LAYER_DECL_NAME("ShadowImageLayer", TYPE_SHADOW)

protected:
  ShadowImageLayer(LayerManager* aManager, void* aImplData)
    : ImageLayer(aManager, aImplData)
  {}
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

} 
} 

#endif 
