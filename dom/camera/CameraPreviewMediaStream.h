



#ifndef DOM_CAMERA_CAMERAPREVIEWMEDIASTREAM_H
#define DOM_CAMERA_CAMERAPREVIEWMEDIASTREAM_H

#include "VideoFrameContainer.h"
#include "MediaStreamGraph.h"
#include "mozilla/Mutex.h"

namespace mozilla {








class CameraPreviewMediaStream : public MediaStream {
  typedef mozilla::layers::Image Image;

public:
  CameraPreviewMediaStream(DOMMediaStream* aWrapper) :
    MediaStream(aWrapper),
    mMutex("mozilla::camera::CameraPreviewMediaStream")
  {
    mIsConsumed = false;
  }

  virtual void AddAudioOutput(void* aKey);
  virtual void SetAudioOutputVolume(void* aKey, float aVolume);
  virtual void RemoveAudioOutput(void* aKey);
  virtual void AddVideoOutput(VideoFrameContainer* aContainer);
  virtual void RemoveVideoOutput(VideoFrameContainer* aContainer);
  virtual void ChangeExplicitBlockerCount(int32_t aDelta);
  virtual void AddListener(MediaStreamListener* aListener);
  virtual void RemoveListener(MediaStreamListener* aListener);
  virtual void Destroy();

  
  void SetCurrentFrame(const gfxIntSize& aIntrinsicSize, Image* aImage);

protected:
  
  
  
  Mutex mMutex;
};


}

#endif 
