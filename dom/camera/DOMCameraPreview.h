



#ifndef DOM_CAMERA_DOMCAMERAPREVIEW_H
#define DOM_CAMERA_DOMCAMERAPREVIEW_H

#include "nsCycleCollectionParticipant.h"
#include "MediaStreamGraph.h"
#include "StreamBuffer.h"
#include "ICameraControl.h"
#include "nsDOMMediaStream.h"
#include "CameraCommon.h"

using namespace mozilla;
using namespace mozilla::layers;

namespace mozilla {

typedef void (*FrameBuilder)(Image* aImage, void* aBuffer, uint32_t aWidth, uint32_t aHeight);





class DOMCameraPreview : public nsDOMMediaStream
{
protected:
  enum { TRACK_VIDEO = 1 };

public:
  DOMCameraPreview(ICameraControl* aCameraControl, uint32_t aWidth, uint32_t aHeight, uint32_t aFramesPerSecond = 30);
  bool ReceiveFrame(void* aBuffer, ImageFormat aFormat, FrameBuilder aBuilder);
  bool HaveEnoughBuffered();

  NS_IMETHODIMP
  GetCurrentTime(double* aCurrentTime) {
    return nsDOMMediaStream::GetCurrentTime(aCurrentTime);
  }

  void Start();   
  void Started(); 
  void Stop();    
  void Stopped(bool aForced = false);
                  
  void Error();   

  void SetStateStarted();
  void SetStateStopped();

protected:
  virtual ~DOMCameraPreview();

  enum {
    STOPPED,
    STARTING,
    STARTED,
    STOPPING
  };
  uint32_t mState;

  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mFramesPerSecond;
  SourceMediaStream* mInput;
  nsRefPtr<ImageContainer> mImageContainer;
  VideoSegment mVideoSegment;
  uint32_t mFrameCount;
  nsRefPtr<ICameraControl> mCameraControl;

  
  MediaStreamListener* mListener;

private:
  DOMCameraPreview(const DOMCameraPreview&) MOZ_DELETE;
  DOMCameraPreview& operator=(const DOMCameraPreview&) MOZ_DELETE;
};

} 

#endif
