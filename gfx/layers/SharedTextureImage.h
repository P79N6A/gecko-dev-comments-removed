




#ifndef GFX_SHAREDTEXTUREIMAGE_H
#define GFX_SHAREDTEXTUREIMAGE_H

#include "GLContextProvider.h"          
#include "ImageContainer.h"             
#include "ImageTypes.h"                 
#include "nsCOMPtr.h"                   
#include "mozilla/gfx/Point.h"          




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

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE
  {
    return nullptr;
  }

  SharedTextureImage() : Image(nullptr, ImageFormat::SHARED_TEXTURE) {}

private:
  Data mData;
};

} 
} 

#endif 
