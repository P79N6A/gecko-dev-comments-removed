




































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








class RecycleBin {
  THEBES_INLINE_DECL_THREADSAFE_REFCOUNTING(RecycleBin)

  typedef mozilla::gl::GLContext GLContext;

public:
  RecycleBin();

  void RecycleBuffer(PRUint8* aBuffer, PRUint32 aSize);
  
  PRUint8* TakeBuffer(PRUint32 aSize);

  enum TextureType {
    TEXTURE_Y,
    TEXTURE_C
  };

  void RecycleTexture(GLTexture *aTexture, TextureType aType,
                      const gfxIntSize& aSize);
  void TakeTexture(TextureType aType, const gfxIntSize& aSize,
                   GLContext *aContext, GLTexture *aOutTexture);

private:
  typedef mozilla::Mutex Mutex;

  
  
  Mutex mLock;

  
  
  nsTArray<nsAutoArrayPtr<PRUint8> > mRecycledBuffers;
  
  PRUint32 mRecycledBufferSize;

  nsTArray<GLTexture> mRecycledTextures[2];
  gfxIntSize mRecycledTextureSizes[2];
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

  nsRefPtr<RecycleBin> mRecycleBin;

  
  Mutex mActiveImageLock;

  nsRefPtr<Image> mActiveImage;
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

  
  virtual Layer* GetLayer();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
};

class THEBES_API PlanarYCbCrImageOGL : public PlanarYCbCrImage
{
  typedef mozilla::gl::GLContext GLContext;

public:
  PlanarYCbCrImageOGL(RecycleBin *aRecycleBin);
  ~PlanarYCbCrImageOGL();

  virtual void SetData(const Data &aData);

  



  void AllocateTextures(LayerManagerOGL *aManager);
  PRBool HasData() { return mHasData; }
  PRBool HasTextures()
  {
    return mTextures[0].IsAllocated() && mTextures[1].IsAllocated() &&
           mTextures[2].IsAllocated();
  }

  nsAutoArrayPtr<PRUint8> mBuffer;
  PRUint32 mBufferSize;
  nsRefPtr<RecycleBin> mRecycleBin;
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
  nsRefPtr<GLContext> mASurfaceAsGLContext;
};

} 
} 
#endif 
