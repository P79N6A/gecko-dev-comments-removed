




#ifndef GFX_GLIMAGES_H
#define GFX_GLIMAGES_H

#include "GLTypes.h"
#include "ImageContainer.h"             
#include "ImageTypes.h"                 
#include "nsCOMPtr.h"                   
#include "mozilla/gfx/Point.h"          

namespace mozilla {
namespace gl {
class AndroidSurfaceTexture;
}
namespace layers {

class EGLImageImage : public Image {
public:
  struct Data {
    EGLImage mImage;
    EGLSync mSync;
    gfx::IntSize mSize;
    bool mInverted;
    bool mOwns;

    Data() : mImage(nullptr), mSync(nullptr), mSize(0, 0), mInverted(false), mOwns(false)
    {
    }
  };

  void SetData(const Data& aData) { mData = aData; }
  const Data* GetData() { return &mData; }

  gfx::IntSize GetSize() { return mData.mSize; }

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE
  {
    return nullptr;
  }

  EGLImageImage() : Image(nullptr, ImageFormat::EGLIMAGE) {}

protected:
  virtual ~EGLImageImage();

private:
  Data mData;
};

#ifdef MOZ_WIDGET_ANDROID

class SurfaceTextureImage : public Image {
public:
  struct Data {
    mozilla::gl::AndroidSurfaceTexture* mSurfTex;
    gfx::IntSize mSize;
    bool mInverted;
  };

  void SetData(const Data& aData) { mData = aData; }
  const Data* GetData() { return &mData; }

  gfx::IntSize GetSize() { return mData.mSize; }

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE;

  SurfaceTextureImage() : Image(nullptr, ImageFormat::SURFACE_TEXTURE) {}

private:
  Data mData;
};

#endif 

} 
} 

#endif 
