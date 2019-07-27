



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
namespace ipc {
class Shmem;
}

namespace layers {

class BufferTextureClient;
class ImageClient;
class ISurfaceAllocator;
class TextureClient;
class SurfaceDescriptor;

already_AddRefed<Image> CreateSharedRGBImage(ImageContainer* aImageContainer,
                                             nsIntSize aSize,
                                             gfxImageFormat aImageFormat);





class SharedRGBImage : public Image
                     , public ISharedImage
{
public:
  explicit SharedRGBImage(ImageClient* aCompositable);

protected:
  ~SharedRGBImage();

public:
  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  gfx::IntSize GetSize();

  size_t GetBufferSize();

  TemporaryRef<gfx::SourceSurface> GetAsSourceSurface();

  bool Allocate(gfx::IntSize aSize, gfx::SurfaceFormat aFormat);
private:
  gfx::IntSize mSize;
  RefPtr<ImageClient> mCompositable;
  RefPtr<BufferTextureClient> mTextureClient;
};

} 
} 

#endif
