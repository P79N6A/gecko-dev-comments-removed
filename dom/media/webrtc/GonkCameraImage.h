




#ifndef GONKCAMERAIMAGE_H
#define GONKCAMERAIMAGE_H

#include "mozilla/ReentrantMonitor.h"
#include "ImageLayers.h"
#include "ImageContainer.h"

namespace android {
class MOZ_EXPORT MediaBuffer;
}

namespace mozilla {
























class GonkCameraImage : public layers::PlanarYCbCrImage
{
public:
  GonkCameraImage(layers::BufferRecycleBin* aRecycleBin);

  
  
  
  nsresult GetBuffer(android::MediaBuffer** aBuffer);

  
  
  nsresult SetBuffer(android::MediaBuffer* aBuffer);

  
  nsresult ClearBuffer();

  bool HasMediaBuffer();

protected:
  virtual ~GonkCameraImage();

  
  ReentrantMonitor mMonitor;
  android::MediaBuffer* mMediaBuffer;
  
  
  DebugOnly<nsIThread*> mThread;
};

} 

#endif 
