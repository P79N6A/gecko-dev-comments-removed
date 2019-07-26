



#ifndef SHAREDRGBIMAGE_H_
#define SHAREDRGBIMAGE_H_

#include "ImageContainer.h"
#include "ISurfaceAllocator.h"

namespace mozilla {
namespace ipc {
class Shmem;
}
namespace layers {





class SharedRGBImage : public Image
{
  typedef gfxASurface::gfxImageFormat gfxImageFormat;
public:
  struct Header {
    gfxImageFormat mImageFormat;
  };

  SharedRGBImage(ISurfaceAllocator *aAllocator);
  ~SharedRGBImage();

  static already_AddRefed<SharedRGBImage> Create(ImageContainer* aImageContainer,
                                                 nsIntSize aSize,
                                                 gfxImageFormat aImageFormat);
  uint8_t *GetBuffer();

  gfxIntSize GetSize();
  size_t GetBufferSize();

  static uint8_t BytesPerPixel(gfxImageFormat aImageFormat);
  already_AddRefed<gfxASurface> GetAsSurface();

  




  bool ToSurfaceDescriptor(SurfaceDescriptor& aResult);

  





  bool DropToSurfaceDescriptor(SurfaceDescriptor& aResult);

  



  static SharedRGBImage* FromSurfaceDescriptor(const SurfaceDescriptor& aDescriptor);

private:
  bool AllocateBuffer(nsIntSize aSize, gfxImageFormat aImageFormat);

  gfxIntSize mSize;
  gfxImageFormat mImageFormat;
  ISurfaceAllocator* mSurfaceAllocator;

  bool mAllocated;
  ipc::Shmem *mShmem;
};

} 
} 

#endif
