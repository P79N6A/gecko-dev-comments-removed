




































#ifndef GFX_IMAGELAYEROGL_H
#define GFX_IMAGELAYEROGL_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "LayerManagerOGL.h"
#include "ImageLayers.h"
#include "yuv_convert.h"
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

  bool IsAllocated() { return mTexture != 0; }
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
  
  PRUint8* GetBuffer(PRUint32 aSize);

  enum TextureType {
    TEXTURE_Y,
    TEXTURE_C
  };

  void RecycleTexture(GLTexture *aTexture, TextureType aType,
                      const gfxIntSize& aSize);
  void GetTexture(TextureType aType, const gfxIntSize& aSize,
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
  virtual ~ImageContainerOGL();

  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats);

  virtual void SetCurrentImage(Image* aImage);

  virtual already_AddRefed<Image> GetCurrentImage();

  virtual already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSize);

  virtual gfxIntSize GetCurrentSize();

  virtual bool SetLayerManager(LayerManager *aManager);

  virtual LayerManager::LayersBackend GetBackendType() { return LayerManager::LAYERS_OPENGL; }

private:

  nsRefPtr<RecycleBin> mRecycleBin;
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
  ~ImageLayerOGL() { Destroy(); }

  
  virtual void Destroy() { mDestroyed = true; }
  virtual Layer* GetLayer();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
};

class THEBES_API PlanarYCbCrImageOGL : public PlanarYCbCrImage
{
  typedef mozilla::gl::GLContext GLContext;

public:
  PlanarYCbCrImageOGL(LayerManagerOGL *aManager,
                      RecycleBin *aRecycleBin);
  ~PlanarYCbCrImageOGL();

  virtual void SetData(const Data &aData);

  



  void AllocateTextures(GLContext *gl);
  void UpdateTextures(GLContext *gl);

  bool HasData() { return mHasData; }
  bool HasTextures()
  {
    return mTextures[0].IsAllocated() && mTextures[1].IsAllocated() &&
           mTextures[2].IsAllocated();
  }

  PRUint8* AllocateBuffer(PRUint32 aSize) {
    return mRecycleBin->GetBuffer(aSize);
  }

  PRUint32 GetDataSize() { return mBuffer ? mBufferSize : 0; }

  nsAutoArrayPtr<PRUint8> mBuffer;
  PRUint32 mBufferSize;
  nsRefPtr<RecycleBin> mRecycleBin;
  GLTexture mTextures[3];
  Data mData;
  gfxIntSize mSize;
  bool mHasData;
};


class THEBES_API CairoImageOGL : public CairoImage
{
  typedef mozilla::gl::GLContext GLContext;

public:
  CairoImageOGL(LayerManagerOGL *aManager);

  void SetData(const Data &aData);

  GLTexture mTexture;
  gfxIntSize mSize;
  gl::ShaderProgramType mLayerProgram;
#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
  nsRefPtr<gfxASurface> mSurface;
#endif
  void SetTiling(bool aTiling);
private:
  bool mTiling;
};

class ShadowImageLayerOGL : public ShadowImageLayer,
                            public LayerOGL
{
  typedef gl::TextureImage TextureImage;

public:
  ShadowImageLayerOGL(LayerManagerOGL* aManager);
  virtual ~ShadowImageLayerOGL();

  
  virtual void Swap(const SharedImage& aFront,
                    SharedImage* aNewBack);

  virtual void Disconnect();

  
  virtual void Destroy();

  virtual Layer* GetLayer();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

private:
  bool Init(const SharedImage& aFront);

  nsRefPtr<TextureImage> mTexImage;
  GLTexture mYUVTexture[3];
  gfxIntSize mSize;
  gfxIntSize mCbCrSize;
  nsIntRect mPictureRect;
};

} 
} 
#endif 
