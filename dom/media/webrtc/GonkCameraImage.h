




#ifndef GONKCAMERAIMAGE_H
#define GONKCAMERAIMAGE_H

#include "mozilla/ReentrantMonitor.h"
#include "ImageLayers.h"
#include "ImageContainer.h"
#include "GrallocImages.h"

namespace android {
class MOZ_EXPORT MediaBuffer;
}

namespace mozilla {
























class GonkCameraImage : public layers::GrallocImage
{
public:
  GonkCameraImage();

  
  
  
  nsresult GetMediaBuffer(android::MediaBuffer** aBuffer);

  
  
  nsresult SetMediaBuffer(android::MediaBuffer* aBuffer);

  
  nsresult ClearMediaBuffer();

  bool HasMediaBuffer();

protected:
  virtual ~GonkCameraImage();

  
  ReentrantMonitor mMonitor;
  android::MediaBuffer* mMediaBuffer;
  
  
  DebugOnly<nsIThread*> mThread;
};

} 

#endif 
