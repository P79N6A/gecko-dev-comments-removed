



#ifndef SHAREDRGBIMAGE_H_
#define SHAREDRGBIMAGE_H_

#include "ImageContainer.h"
#include "ISurfaceAllocator.h"

namespace mozilla {
namespace ipc {
class Shmem;
}
namespace layers {
class BufferTextureClient;
class TextureClient;
class ImageClient;

already_AddRefed<Image> CreateSharedRGBImage(ImageContainer* aImageContainer,
                                             nsIntSize aSize,
                                             gfxASurface::gfxImageFormat aImageFormat);





class DeprecatedSharedRGBImage : public Image,
                                 public ISharedImage
{
friend already_AddRefed<Image> CreateSharedRGBImage(ImageContainer* aImageContainer,
                                                    nsIntSize aSize,
                                                    gfxASurface::gfxImageFormat aImageFormat);
public:
  typedef gfxASurface::gfxImageFormat gfxImageFormat;
  struct Header {
    gfxImageFormat mImageFormat;
  };

  DeprecatedSharedRGBImage(ISurfaceAllocator *aAllocator);
  ~DeprecatedSharedRGBImage();

  uint8_t *GetBuffer();

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
  typedef gfxASurface::gfxImageFormat gfxImageFormat;
public:
  SharedRGBImage(ImageClient* aCompositable);
  ~SharedRGBImage();

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
