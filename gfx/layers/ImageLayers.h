




































#ifndef GFX_IMAGELAYER_H
#define GFX_IMAGELAYER_H

#include "Layers.h"

#include "gfxPattern.h"
#include "nsThreadUtils.h"
#include "nsCoreAnimationSupport.h"
#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"

namespace mozilla {
namespace layers {

enum StereoMode {
  STEREO_MODE_MONO,
  STEREO_MODE_LEFT_RIGHT,
  STEREO_MODE_RIGHT_LEFT,
  STEREO_MODE_BOTTOM_TOP,
  STEREO_MODE_TOP_BOTTOM
};















class THEBES_API Image {
  THEBES_INLINE_DECL_THREADSAFE_REFCOUNTING(Image)

public:
  virtual ~Image() {}

  enum Format {
    




    PLANAR_YCBCR,

    












    CAIRO_SURFACE,

    





    MAC_IO_SURFACE
  };

  Format GetFormat() { return mFormat; }
  void* GetImplData() { return mImplData; }

protected:
  Image(void* aImplData, Format aFormat) :
    mImplData(aImplData),
    mFormat(aFormat)
  {}

  void* mImplData;
  Format mFormat;
};








class THEBES_API ImageContainer {
  THEBES_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageContainer)

public:
  ImageContainer() :
    mMonitor("ImageContainer"),
    mPaintCount(0),
    mPreviousImagePainted(PR_FALSE)
  {}

  virtual ~ImageContainer() {}

  







  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats) = 0;

  







  virtual void SetCurrentImage(Image* aImage) = 0;

  









  virtual already_AddRefed<Image> GetCurrentImage() = 0;

  














  virtual already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSizeResult) = 0;

  




  LayerManager* Manager()
  {
    NS_PRECONDITION(NS_IsMainThread(), "Must be called on main thread");
    return mManager;
  }

  




  virtual gfxIntSize GetCurrentSize() = 0;

  




  virtual PRBool SetLayerManager(LayerManager *aManager) = 0;

  






  virtual void SetScaleHint(const gfxIntSize& ) { }

  




  virtual LayerManager::LayersBackend GetBackendType() = 0;

  





  TimeStamp GetPaintTime() {
    MonitorAutoEnter mon(mMonitor);
    return mPaintTime;
  }

  



  PRUint32 GetPaintCount() {
    MonitorAutoEnter mon(mMonitor);
    return mPaintCount;
  }

  




  void NotifyPaintedImage(Image* aPainted) {
    MonitorAutoEnter mon(mMonitor);
    nsRefPtr<Image> current = GetCurrentImage();
    if (aPainted == current) {
      if (mPaintTime.IsNull()) {
        mPaintTime = TimeStamp::Now();
        mPaintCount++;
      }
    } else if (!mPreviousImagePainted) {
      
      
      
      mPaintCount++;
      mPreviousImagePainted = PR_TRUE;
    }
  }

protected:
  typedef mozilla::Monitor Monitor;
  LayerManager* mManager;

  
  
  Monitor mMonitor;

  ImageContainer(LayerManager* aManager) :
    mManager(aManager),
    mMonitor("ImageContainer"),
    mPaintCount(0),
    mPreviousImagePainted(PR_FALSE)
  {}

  
  
  
  void CurrentImageChanged() {
    mMonitor.AssertCurrentThreadIn();
    mPreviousImagePainted = !mPaintTime.IsNull();
    mPaintTime = TimeStamp();
  }

  
  
  
  PRUint32 mPaintCount;

  
  
  TimeStamp mPaintTime;

  
  PRPackedBool mPreviousImagePainted;
};




class THEBES_API ImageLayer : public Layer {
public:
  




  void SetContainer(ImageContainer* aContainer) 
  {
    NS_ASSERTION(!aContainer->Manager() || aContainer->Manager() == Manager(), 
                 "ImageContainer must have the same manager as the ImageLayer");
    mContainer = aContainer;  
  }
  



  void SetFilter(gfxPattern::GraphicsFilter aFilter) { mFilter = aFilter; }

  ImageContainer* GetContainer() { return mContainer; }
  gfxPattern::GraphicsFilter GetFilter() { return mFilter; }

  MOZ_LAYER_DECL_NAME("ImageLayer", TYPE_IMAGE)

  virtual void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface)
  {
    
    gfxRect snap(0, 0, 0, 0);
    if (mContainer) {
      gfxIntSize size = mContainer->GetCurrentSize();
      snap.SizeTo(gfxSize(size.width, size.height));
    }
    
    
    
    
    mEffectiveTransform =
        SnapTransform(GetLocalTransform(), snap, nsnull)*
        SnapTransform(aTransformToSurface, gfxRect(0, 0, 0, 0), nsnull);
  }

protected:
  ImageLayer(LayerManager* aManager, void* aImplData)
    : Layer(aManager, aImplData), mFilter(gfxPattern::FILTER_GOOD) {}

  virtual nsACString& PrintInfo(nsACString& aTo, const char* aPrefix);

  nsRefPtr<ImageContainer> mContainer;
  gfxPattern::GraphicsFilter mFilter;
};




















class THEBES_API PlanarYCbCrImage : public Image {
public:
  struct Data {
    
    PRUint8* mYChannel;
    PRInt32 mYStride;
    gfxIntSize mYSize;
    
    PRUint8* mCbChannel;
    PRUint8* mCrChannel;
    PRInt32 mCbCrStride;
    gfxIntSize mCbCrSize;
    
    PRUint32 mPicX;
    PRUint32 mPicY;
    gfxIntSize mPicSize;
    StereoMode mStereoMode;
  };

  enum {
    MAX_DIMENSION = 16384
  };

  





  virtual void SetData(const Data& aData) = 0;

protected:
  PlanarYCbCrImage(void* aImplData) : Image(aImplData, PLANAR_YCBCR) {}
};





class THEBES_API CairoImage : public Image {
public:
  struct Data {
    gfxASurface* mSurface;
    gfxIntSize mSize;
  };

  




  virtual void SetData(const Data& aData) = 0;

protected:
  CairoImage(void* aImplData) : Image(aImplData, CAIRO_SURFACE) {}
};

#ifdef XP_MACOSX
class THEBES_API MacIOSurfaceImage : public Image {
public:
  struct Data {
    nsIOSurface* mIOSurface;
  };

 




  virtual void SetData(const Data& aData) = 0;

  




  typedef void (*UpdateSurfaceCallback)(ImageContainer* aContainer, void* aInstanceOwner);
  virtual void SetUpdateCallback(UpdateSurfaceCallback aCallback, void* aInstanceOwner) = 0;
  typedef void (*DestroyCallback)(void* aInstanceOwner);
  virtual void SetDestroyCallback(DestroyCallback aCallback) = 0;

protected:
  MacIOSurfaceImage(void* aImplData) : Image(aImplData, MAC_IO_SURFACE) {}
};
#endif

}
}

#endif 
