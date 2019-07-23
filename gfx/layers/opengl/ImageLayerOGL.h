




































#ifndef GFX_IMAGELAYEROGL_H
#define GFX_IMAGELAYEROGL_H

#include "LayerManagerOGL.h"
#include "ImageLayers.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace layers {

class THEBES_API ImageContainerOGL : public ImageContainer
{
public:
  ImageContainerOGL(LayerManagerOGL *aManager);
  virtual ~ImageContainerOGL() {}

  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats);

  virtual void SetCurrentImage(Image* aImage);

  virtual already_AddRefed<Image> GetCurrentImage();

  virtual already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSize);
private:
  typedef mozilla::Mutex Mutex;

  nsRefPtr<Image> mActiveImage;

  Mutex mActiveImageLock;
};

class THEBES_API ImageLayerOGL : public ImageLayer,
                                 public LayerOGL
{
public:
  ImageLayerOGL(LayerManagerOGL *aManager)
    : ImageLayer(aManager, NULL)
    , LayerOGL(aManager)
  { 
    mImplData = static_cast<LayerOGL*>(this);
  }

  
  virtual LayerType GetType();

  virtual Layer* GetLayer();

  virtual void RenderLayer(int aPreviousDestination);
};

class THEBES_API PlanarYCbCrImageOGL : public PlanarYCbCrImage
{
public:
  PlanarYCbCrImageOGL(LayerManagerOGL *aManager);
  virtual ~PlanarYCbCrImageOGL();

  virtual void SetData(const Data &aData);

  



  virtual void AllocateTextures();
  






  virtual void FreeTextures();
  virtual PRBool HasData() { return mHasData; }

  Data mData;
  PRBool mLoaded;
  PRBool mHasData;
  GLuint mTextures[3];
  gfxIntSize mSize;
  LayerManagerOGL *mManager;
};


class THEBES_API CairoImageOGL : public CairoImage
{
public:
  CairoImageOGL(LayerManagerOGL *aManager)
    : CairoImage(NULL)
    , mManager(aManager)
  { }
  ~CairoImageOGL();

  virtual void SetData(const Data &aData);

  GLuint mTexture;
  gfxIntSize mSize;
  LayerManagerOGL *mManager;
};

} 
} 
#endif 
