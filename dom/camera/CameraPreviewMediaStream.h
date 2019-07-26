



#ifndef DOM_CAMERA_CAMERAPREVIEWMEDIASTREAM_H
#define DOM_CAMERA_CAMERAPREVIEWMEDIASTREAM_H

#include "VideoFrameContainer.h"
#include "MediaStreamGraph.h"
#include "mozilla/Mutex.h"

namespace mozilla {

class CameraPreviewFrameCallback {
public:
  virtual void OnNewFrame(const gfxIntSize& aIntrinsicSize, layers::Image* aImage) = 0;
};








class CameraPreviewMediaStream : public MediaStream
{
  typedef mozilla::layers::Image Image;

public:
  CameraPreviewMediaStream(DOMMediaStream* aWrapper);

  virtual void AddAudioOutput(void* aKey) MOZ_OVERRIDE;
  virtual void SetAudioOutputVolume(void* aKey, float aVolume) MOZ_OVERRIDE;
  virtual void RemoveAudioOutput(void* aKey) MOZ_OVERRIDE;
  virtual void AddVideoOutput(VideoFrameContainer* aContainer) MOZ_OVERRIDE;
  virtual void RemoveVideoOutput(VideoFrameContainer* aContainer) MOZ_OVERRIDE;
  virtual void ChangeExplicitBlockerCount(int32_t aDelta) MOZ_OVERRIDE;
  virtual void AddListener(MediaStreamListener* aListener) MOZ_OVERRIDE;
  virtual void RemoveListener(MediaStreamListener* aListener) MOZ_OVERRIDE;
  virtual void Destroy();

  
  void SetCurrentFrame(const gfxIntSize& aIntrinsicSize, Image* aImage);

  void SetFrameCallback(CameraPreviewFrameCallback* aCallback) {
    mFrameCallback = aCallback;
  }

protected:
  
  
  
  Mutex mMutex;
  CameraPreviewFrameCallback* mFrameCallback;
};

}

#endif 
