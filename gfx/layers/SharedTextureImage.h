




#ifndef GFX_SHAREDTEXTUREIMAGE_H
#define GFX_SHAREDTEXTUREIMAGE_H

#include "ImageContainer.h"
#include "GLContext.h"
#include "GLContextProvider.h"




namespace mozilla {

namespace layers {

class SharedTextureImage : public Image {
public:
  struct Data {
    gl::SharedTextureHandle mHandle;
    gl::GLContext::SharedTextureShareType mShareType;
    gfxIntSize mSize;
    bool mInverted;
  };

  void SetData(const Data& aData) { mData = aData; }
  const Data* GetData() { return &mData; }

  gfxIntSize GetSize() { return mData.mSize; }

  virtual already_AddRefed<gfxASurface> GetAsSurface() { 
    return gl::GLContextProvider::GetSharedHandleAsSurface(mData.mShareType, mData.mHandle);
  }

  SharedTextureImage() : Image(nullptr, SHARED_TEXTURE) {}

private:
  Data mData;
};

} 
} 

#endif 
