




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
class TextureClient;

class SharedPlanarYCbCrImage : public PlanarYCbCrImage
{
public:
  explicit SharedPlanarYCbCrImage(ImageClient* aCompositable);

protected:
  ~SharedPlanarYCbCrImage();

public:
  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;
  virtual uint8_t* GetBuffer() override;

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() override;
  virtual void SetData(const PlanarYCbCrData& aData) override;
  virtual void SetDataNoCopy(const Data &aData) override;

  virtual bool Allocate(PlanarYCbCrData& aData);
  virtual uint8_t* AllocateBuffer(uint32_t aSize) override;
  
  
  virtual uint8_t* AllocateAndGetNewBuffer(uint32_t aSize) override;

  virtual bool IsValid() override;

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const override;

private:
  RefPtr<BufferTextureClient> mTextureClient;
  RefPtr<ImageClient> mCompositable;
};

} 
} 

#endif
