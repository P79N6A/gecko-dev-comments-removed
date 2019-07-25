




































#ifndef GFX_IMAGELAYEROGL_H
#define GFX_IMAGELAYEROGL_H

#include "LayerManagerOGL.h"
#include "ImageLayers.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace layers {













class GLTexture {
  typedef mozilla::gl::GLContext GLContext;

public:
  GLTexture() : mTexture(0) {}
  ~GLTexture() { Release(); }

  


  void Allocate(GLContext *aContext);
  



  void TakeFrom(GLTexture *aOther);

  PRBool IsAllocated() { return mTexture != 0; }
  GLuint GetTextureID() { return mTexture; }
  GLContext *GetGLContext() { return mContext; }

private:
  void Release();

  nsRefPtr<GLContext> mContext;
  GLuint mTexture;
};

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

  virtual gfxIntSize GetCurrentSize();

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
  typedef mozilla::gl::GLContext GLContext;

public:
  PlanarYCbCrImageOGL();

  virtual void SetData(const Data &aData);

  



  void AllocateTextures(LayerManagerOGL *aManager);
  PRBool HasData() { return mHasData; }
  PRBool HasTextures()
  {
    return mTextures[0].IsAllocated() && mTextures[1].IsAllocated() &&
           mTextures[2].IsAllocated();
  }

  nsAutoArrayPtr<PRUint8> mBuffer;
  GLTexture mTextures[3];
  Data mData;
  gfxIntSize mSize;
  PRPackedBool mHasData;
};


class THEBES_API CairoImageOGL : public CairoImage
{
  typedef mozilla::gl::GLContext GLContext;

public:
  CairoImageOGL(LayerManagerOGL *aManager);

  void SetData(const Data &aData);

  GLTexture mTexture;
  gfxIntSize mSize;
};

} 
} 
#endif 
