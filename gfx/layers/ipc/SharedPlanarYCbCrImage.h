




#include <stdint.h>                     
#include "ImageContainer.h"             
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/ipc/Shmem.h"          
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsTraceRefcnt.h"              

class gfxASurface;

#ifndef MOZILLA_LAYERS_DeprecatedSharedPlanarYCbCrImage_H
#define MOZILLA_LAYERS_DeprecatedSharedPlanarYCbCrImage_H

namespace mozilla {
namespace layers {

class BufferTextureClient;
class ImageClient;
class ISurfaceAllocator;
class SurfaceDescriptor;
class TextureClient;


class DeprecatedSharedPlanarYCbCrImage : public PlanarYCbCrImage
{
public:
  DeprecatedSharedPlanarYCbCrImage(ISurfaceAllocator* aAllocator)
  : PlanarYCbCrImage(nullptr)
  , mSurfaceAllocator(aAllocator), mAllocated(false)
  {
    MOZ_COUNT_CTOR(DeprecatedSharedPlanarYCbCrImage);
  }

  ~DeprecatedSharedPlanarYCbCrImage();

  virtual DeprecatedSharedPlanarYCbCrImage* AsDeprecatedSharedPlanarYCbCrImage() MOZ_OVERRIDE
  {
    return this;
  }

  virtual already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE
  {
    if (!mAllocated) {
      NS_WARNING("Can't get as surface");
      return nullptr;
    }
    return PlanarYCbCrImage::GetAsSurface();
  }

  virtual void SetData(const PlanarYCbCrData& aData) MOZ_OVERRIDE;
  virtual void SetDataNoCopy(const Data &aData) MOZ_OVERRIDE;

  virtual bool Allocate(PlanarYCbCrData& aData);
  virtual uint8_t* AllocateBuffer(uint32_t aSize) MOZ_OVERRIDE;
  
  
  uint8_t* AllocateAndGetNewBuffer(uint32_t aSize) MOZ_OVERRIDE;

  virtual bool IsValid() MOZ_OVERRIDE {
    return mAllocated;
  }

  




  bool ToSurfaceDescriptor(SurfaceDescriptor& aResult);

  





  bool DropToSurfaceDescriptor(SurfaceDescriptor& aResult);

  



  static DeprecatedSharedPlanarYCbCrImage* FromSurfaceDescriptor(const SurfaceDescriptor& aDesc);

private:
  ipc::Shmem mShmem;
  ISurfaceAllocator* mSurfaceAllocator;
  bool mAllocated;
};


class SharedPlanarYCbCrImage : public PlanarYCbCrImage
                             , public ISharedImage
{
public:
  SharedPlanarYCbCrImage(ImageClient* aCompositable);
  ~SharedPlanarYCbCrImage();

  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }
  virtual TextureClient* GetTextureClient() MOZ_OVERRIDE;
  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  virtual already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;
  virtual void SetData(const PlanarYCbCrData& aData) MOZ_OVERRIDE;
  virtual void SetDataNoCopy(const Data &aData) MOZ_OVERRIDE;

  virtual bool Allocate(PlanarYCbCrData& aData);
  virtual uint8_t* AllocateBuffer(uint32_t aSize) MOZ_OVERRIDE;
  
  
  virtual uint8_t* AllocateAndGetNewBuffer(uint32_t aSize) MOZ_OVERRIDE;

  virtual bool IsValid() MOZ_OVERRIDE;

private:
  RefPtr<BufferTextureClient> mTextureClient;
  RefPtr<ImageClient> mCompositable;
};

} 
} 

#endif
