




#ifndef GFX_SHAREDTEXTUREIMAGE_H
#define GFX_SHAREDTEXTUREIMAGE_H

#include "ImageContainer.h"
#include "GLContext.h"




namespace mozilla {

namespace layers {

class THEBES_API SharedTextureImage : public Image {
public:
  struct Data {
    gl::SharedTextureHandle mHandle;
    gl::TextureImage::TextureShareType mShareType;
    gfxIntSize mSize;
    bool mInverted;
  };

  void SetData(const Data& aData) { mData = aData; }
  const Data* GetData() { return &mData; }

  gfxIntSize GetSize() { return mData.mSize; }

  virtual already_AddRefed<gfxASurface> GetAsSurface() { return NULL; }

  SharedTextureImage() : Image(NULL, SHARED_TEXTURE) {}

private:
  Data mData;
};

} 
} 

#endif 