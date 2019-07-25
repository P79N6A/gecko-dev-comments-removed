







































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
                           nsIntRect aBufferRect,
                           gfxSharedImageSurface* aInitialFrontBuffer);
  



  void CreatedImageBuffer(ShadowableLayer* aImage,
                          nsIntSize aSize,
                          gfxSharedImageSurface* aInitialFrontSurface);
  void CreatedCanvasBuffer(ShadowableLayer* aCanvas,
                           nsIntSize aSize,
                           gfxSharedImageSurface* aInitialFrontSurface);

  




  void DestroyedThebesBuffer(ShadowableLayer* aThebes);
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
                           nsIntRect aBufferRect,
                           nsIntPoint aBufferRotation,
                           gfxSharedImageSurface* aNewFrontBuffer);
  



  void PaintedImage(ShadowableLayer* aImage,
                    gfxSharedImageSurface* aNewFrontSurface);
  void PaintedCanvas(ShadowableLayer* aCanvas,
                     gfxSharedImageSurface* aNewFrontSurface);

  




  PRBool EndTransaction(nsTArray<EditReply>* aReplies);

  


  PRBool HasShadowManager() const { return !!mShadowManager; }

  PRBool AllocDoubleBuffer(const gfxIntSize& aSize,
                           gfxASurface::gfxImageFormat aFormat,
                           gfxSharedImageSurface** aFrontBuffer,
                           gfxSharedImageSurface** aBackBuffer);

  void DestroySharedSurface(gfxSharedImageSurface* aSurface);

  



  PLayerChild* ConstructShadowFor(ShadowableLayer* aLayer);

protected:
  ShadowLayerForwarder();

  PLayersChild* mShadowManager;

private:
  Transaction* mTxn;
};


class ShadowLayerManager : public LayerManager
{
public:
  virtual ~ShadowLayerManager() {}

  PRBool HasForwarder() { return !!mForwarder; }

  void SetForwarder(PLayersParent* aForwarder)
  {
    NS_ASSERTION(!aForwarder || !HasForwarder(), "stomping live forwarder?");
    mForwarder = aForwarder;
  }

  virtual void GetBackendName(nsAString& name) { name.AssignLiteral("Shadow"); }

  void DestroySharedSurface(gfxSharedImageSurface* aSurface);

  
  virtual already_AddRefed<ShadowThebesLayer> CreateShadowThebesLayer() = 0;
  
  virtual already_AddRefed<ShadowImageLayer> CreateShadowImageLayer() = 0;
  
  virtual already_AddRefed<ShadowCanvasLayer> CreateShadowCanvasLayer() = 0;

protected:
  ShadowLayerManager() : mForwarder(NULL) {}

  PLayersParent* mForwarder;
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


class ShadowThebesLayer : public ThebesLayer
{
public:
  virtual void InvalidateRegion(const nsIntRegion& aRegion)
  {
    NS_RUNTIMEABORT("ShadowThebesLayers can't fill invalidated regions");
  }

  


  void SetValidRegion(const nsIntRegion& aRegion)
  {
    mValidRegion = aRegion;
    Mutated();
  }

  


  void SetResolution(float aXResolution, float aYResolution)
  {
    mXResolution = aXResolution;
    mYResolution = aYResolution;
    Mutated();
  }

  




















  virtual already_AddRefed<gfxSharedImageSurface>
  Swap(gfxSharedImageSurface* aNewFront,
       const nsIntRect& aBufferRect,
       const nsIntPoint& aRotation) = 0;

  




  virtual void DestroyFrontBuffer() = 0;

  MOZ_LAYER_DECL_NAME("ShadowThebesLayer", TYPE_SHADOW)

protected:
  ShadowThebesLayer(LayerManager* aManager, void* aImplData) :
    ThebesLayer(aManager, aImplData) {}
};


class ShadowCanvasLayer : public CanvasLayer
{
public:
  






  virtual already_AddRefed<gfxSharedImageSurface>
  Swap(gfxSharedImageSurface* aNewFront) = 0;

  




  virtual void DestroyFrontBuffer() = 0;

  MOZ_LAYER_DECL_NAME("ShadowCanvasLayer", TYPE_SHADOW)

protected:
  ShadowCanvasLayer(LayerManager* aManager, void* aImplData) :
    CanvasLayer(aManager, aImplData) {}
};


class ShadowImageLayer : public ImageLayer
{
public:
  







  virtual PRBool Init(gfxSharedImageSurface* aFront, const nsIntSize& aSize) = 0;

  



  virtual already_AddRefed<gfxSharedImageSurface>
  Swap(gfxSharedImageSurface* newFront) = 0;

  




  virtual void DestroyFrontBuffer() = 0;

  MOZ_LAYER_DECL_NAME("ShadowImageLayer", TYPE_SHADOW)

protected:
  ShadowImageLayer(LayerManager* aManager, void* aImplData) :
    ImageLayer(aManager, aImplData) {}
};


} 
} 

#endif 
