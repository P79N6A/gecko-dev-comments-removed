




#ifndef GFX_SHAREDTEXTUREIMAGE_H
#define GFX_SHAREDTEXTUREIMAGE_H

#include "GLContextProvider.h"          
#include "ImageContainer.h"             
#include "ImageTypes.h"                 
#include "gfxPoint.h"                   
#include "nsCOMPtr.h"                   
#include "nsSize.h"                     

class gfxASurface;




namespace mozilla {

namespace layers {

class SharedTextureImage : public Image {
public:
  struct Data {
    gl::SharedTextureHandle mHandle;
    gl::SharedTextureShareType mShareType;
    gfx::IntSize mSize;
    bool mInverted;
  };

  void SetData(const Data& aData) { mData = aData; }
  const Data* GetData() { return &mData; }

  gfx::IntSize GetSize() { return mData.mSize; }

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
