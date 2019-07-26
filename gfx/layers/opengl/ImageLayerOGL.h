




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

  void Release();
private:

  nsRefPtr<GLContext> mContext;
  GLuint mTexture;
};








class TextureRecycleBin {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TextureRecycleBin)

  typedef mozilla::gl::GLContext GLContext;

public:
  TextureRecycleBin();

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

  nsTArray<GLTexture> mRecycledTextures[2];
  gfxIntSize mRecycledTextureSizes[2];
};

class THEBES_API ImageLayerOGL : public ImageLayer,
                                 public LayerOGL
{
public:
  ImageLayerOGL(LayerManagerOGL *aManager);
  ~ImageLayerOGL() { Destroy(); }

  
  virtual void Destroy() { mDestroyed = true; }
  virtual Layer* GetLayer();
  virtual bool LoadAsTexture(GLuint aTextureUnit, gfxIntSize* aSize);

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources() {}


  void AllocateTexturesYCbCr(PlanarYCbCrImage *aImage);
  void AllocateTexturesCairo(CairoImage *aImage);

protected:
  nsRefPtr<TextureRecycleBin> mTextureRecycleBin;
};

struct THEBES_API PlanarYCbCrOGLBackendData : public ImageBackendData
{
  ~PlanarYCbCrOGLBackendData()
  {
    if (HasTextures()) {
      mTextureRecycleBin->RecycleTexture(&mTextures[0], TextureRecycleBin::TEXTURE_Y, mYSize);
      mTextureRecycleBin->RecycleTexture(&mTextures[1], TextureRecycleBin::TEXTURE_C, mCbCrSize);
      mTextureRecycleBin->RecycleTexture(&mTextures[2], TextureRecycleBin::TEXTURE_C, mCbCrSize);
    }
  }

  bool HasTextures()
  {
    return mTextures[0].IsAllocated() && mTextures[1].IsAllocated() &&
           mTextures[2].IsAllocated();
  }

  GLTexture mTextures[3];
  gfxIntSize mYSize, mCbCrSize;
  nsRefPtr<TextureRecycleBin> mTextureRecycleBin;
};


struct CairoOGLBackendData : public ImageBackendData
{
  CairoOGLBackendData() : mLayerProgram(gl::RGBALayerProgramType) {}
  GLTexture mTexture;
  gl::ShaderProgramType mLayerProgram;
  gfxIntSize mTextureSize;
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
  virtual bool LoadAsTexture(GLuint aTextureUnit, gfxIntSize* aSize);

  virtual Layer* GetLayer();

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

  virtual void CleanupResources();

private:
  bool Init(const SharedImage& aFront);
  void UploadSharedYUVToTexture(const YUVImage& yuv);


  nsRefPtr<TextureImage> mTexImage;
  GLTexture mYUVTexture[3];
  gfxIntSize mSize;
  gfxIntSize mCbCrSize;
  nsIntRect mPictureRect;
};

} 
} 
#endif 
