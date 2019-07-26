



#ifndef SHAREDRGBIMAGE_H_
#define SHAREDRGBIMAGE_H_

#include <stddef.h>                     
#include <stdint.h>                     
#include "ImageContainer.h"             
#include "gfxTypes.h"
#include "gfxPoint.h"                   
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "nsCOMPtr.h"                   

class gfxASurface;

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





class DeprecatedSharedRGBImage : public Image,
                                 public ISharedImage
{
friend already_AddRefed<Image> CreateSharedRGBImage(ImageContainer* aImageContainer,
                                                    nsIntSize aSize,
                                                    gfxImageFormat aImageFormat);
public:
  struct Header {
    gfxImageFormat mImageFormat;
  };

  DeprecatedSharedRGBImage(ISurfaceAllocator *aAllocator);
  ~DeprecatedSharedRGBImage();

  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }

  virtual uint8_t *GetBuffer() MOZ_OVERRIDE;

  gfxIntSize GetSize();
  size_t GetBufferSize();

  static uint8_t BytesPerPixel(gfxImageFormat aImageFormat);
  already_AddRefed<gfxASurface> GetAsSurface();

  




  bool ToSurfaceDescriptor(SurfaceDescriptor& aResult);

  





  bool DropToSurfaceDescriptor(SurfaceDescriptor& aResult);

  



  static DeprecatedSharedRGBImage* FromSurfaceDescriptor(const SurfaceDescriptor& aDescriptor);

  bool AllocateBuffer(nsIntSize aSize, gfxImageFormat aImageFormat);

  TextureClient* GetTextureClient() MOZ_OVERRIDE { return nullptr; }

protected:
  gfxIntSize mSize;
  gfxImageFormat mImageFormat;
  ISurfaceAllocator* mSurfaceAllocator;

  bool mAllocated;
  ipc::Shmem *mShmem;
};





class SharedRGBImage : public Image
                     , public ISharedImage
{
public:
  SharedRGBImage(ImageClient* aCompositable);
  ~SharedRGBImage();

  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }

  virtual TextureClient* GetTextureClient() MOZ_OVERRIDE;

  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  gfxIntSize GetSize();

  size_t GetBufferSize();

  already_AddRefed<gfxASurface> GetAsSurface();

  bool Allocate(gfx::IntSize aSize, gfx::SurfaceFormat aFormat);
private:
  gfx::IntSize mSize;
  RefPtr<ImageClient> mCompositable;
  RefPtr<BufferTextureClient> mTextureClient;
};

} 
} 

#endif
