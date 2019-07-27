



#ifndef SHAREDRGBIMAGE_H_
#define SHAREDRGBIMAGE_H_

#include <stddef.h>                     
#include <stdint.h>                     
#include "ImageContainer.h"             
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "nsCOMPtr.h"                   

namespace mozilla {
namespace layers {

class BufferTextureClient;
class ImageClient;
class TextureClient;

already_AddRefed<Image> CreateSharedRGBImage(ImageContainer* aImageContainer,
                                             nsIntSize aSize,
                                             gfxImageFormat aImageFormat);





class SharedRGBImage : public Image
{
public:
  explicit SharedRGBImage(ImageClient* aCompositable);

protected:
  ~SharedRGBImage();

public:
  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;

  virtual uint8_t* GetBuffer() override;

  gfx::IntSize GetSize() override;

  size_t GetBufferSize();

  TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() override;

  bool Allocate(gfx::IntSize aSize, gfx::SurfaceFormat aFormat);
private:
  gfx::IntSize mSize;
  RefPtr<ImageClient> mCompositable;
  RefPtr<BufferTextureClient> mTextureClient;
};

} 
} 

#endif
