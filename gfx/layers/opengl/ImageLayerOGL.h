




#ifndef GFX_IMAGELAYEROGL_H
#define GFX_IMAGELAYEROGL_H

#include "GLContextTypes.h"             
#include "ImageContainer.h"             
#include "ImageLayers.h"                
#include "LayerManagerOGL.h"            
#include "mozilla/Assertions.h"         
#include "mozilla/Mutex.h"              
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"            
#include "nsTArray.h"                   
#include "opengl/LayerManagerOGLProgram.h"  

struct nsIntPoint;

namespace mozilla {
namespace layers {

class BlobYCbCrSurface;
class Layer;













class GLTexture
{
  typedef mozilla::gl::GLContext GLContext;

public:
  GLTexture();
  ~GLTexture();

  


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
                      const LayerIntSize& aSize);
  void GetTexture(TextureType aType, const LayerIntSize& aSize,
                  GLContext *aContext, GLTexture *aOutTexture);

private:
  typedef mozilla::Mutex Mutex;

  
  
  Mutex mLock;

  nsTArray<GLTexture> mRecycledTextures[2];
  LayerIntSize mRecycledTextureSizes[2];
};

class ImageLayerOGL : public ImageLayer,
                      public LayerOGL
{
public:
  ImageLayerOGL(LayerManagerOGL *aManager);
  ~ImageLayerOGL() { Destroy(); }

  
  virtual void Destroy() { mDestroyed = true; }
  virtual Layer* GetLayer();
  virtual bool LoadAsTexture(GLuint aTextureUnit, gfx::IntSize* aSize);

  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources() {}


  void AllocateTexturesYCbCr(PlanarYCbCrImage *aImage);
  void AllocateTexturesCairo(CairoImage *aImage);

protected:
  nsRefPtr<TextureRecycleBin> mTextureRecycleBin;
};

struct PlanarYCbCrOGLBackendData : public ImageBackendData
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
  LayerIntSize mYSize, mCbCrSize;
  nsRefPtr<TextureRecycleBin> mTextureRecycleBin;
};


struct CairoOGLBackendData : public ImageBackendData
{
  CairoOGLBackendData() : mLayerProgram(RGBALayerProgramType) {}
  GLTexture mTexture;
  ShaderProgramType mLayerProgram;
  gfx::IntSize mTextureSize;
};

} 
} 
#endif 
