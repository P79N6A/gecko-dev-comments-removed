




#include <stdint.h>                     
#include "ImageContainer.h"             
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/ipc/Shmem.h"          
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            

#ifndef MOZILLA_LAYERS_SHAREDPLANARYCBCRIMAGE_H
#define MOZILLA_LAYERS_SHAREDPLANARYCBCRIMAGE_H

namespace mozilla {
namespace layers {

class BufferTextureClient;
class ImageClient;
class ISurfaceAllocator;
class SurfaceDescriptor;
class TextureClient;

class SharedPlanarYCbCrImage : public PlanarYCbCrImage
                             , public ISharedImage
{
public:
  explicit SharedPlanarYCbCrImage(ImageClient* aCompositable);

protected:
  ~SharedPlanarYCbCrImage();

public:
  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }
  virtual TextureClient* GetTextureClient(CompositableClient* aClient) MOZ_OVERRIDE;
  virtual uint8_t* GetBuffer() MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE;
  virtual void SetData(const PlanarYCbCrData& aData) MOZ_OVERRIDE;
  virtual void SetDataNoCopy(const Data &aData) MOZ_OVERRIDE;

  virtual bool Allocate(PlanarYCbCrData& aData);
  virtual uint8_t* AllocateBuffer(uint32_t aSize) MOZ_OVERRIDE;
  
  
  virtual uint8_t* AllocateAndGetNewBuffer(uint32_t aSize) MOZ_OVERRIDE;

  virtual bool IsValid() MOZ_OVERRIDE;

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

private:
  RefPtr<BufferTextureClient> mTextureClient;
  RefPtr<ImageClient> mCompositable;
};

} 
} 

#endif
