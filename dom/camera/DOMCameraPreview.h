



#ifndef DOM_CAMERA_DOMCAMERAPREVIEW_H
#define DOM_CAMERA_DOMCAMERAPREVIEW_H

#include "nsCycleCollectionParticipant.h"
#include "MediaStreamGraph.h"
#include "StreamBuffer.h"
#include "ICameraControl.h"
#include "DOMMediaStream.h"
#include "CameraPreviewMediaStream.h"
#include "CameraCommon.h"

class nsGlobalWindow;

namespace mozilla {

typedef void (*FrameBuilder)(mozilla::layers::Image* aImage, void* aBuffer, uint32_t aWidth, uint32_t aHeight);







class DOMCameraPreview : public DOMMediaStream
{
protected:
  enum { TRACK_VIDEO = 1 };

public:
  DOMCameraPreview(nsGlobalWindow* aWindow, ICameraControl* aCameraControl,
                   uint32_t aWidth, uint32_t aHeight, uint32_t aFramesPerSecond = 30);

  bool ReceiveFrame(void* aBuffer, ImageFormat aFormat, mozilla::FrameBuilder aBuilder);
  bool HaveEnoughBuffered();

  void Start();   
  void Started(); 
  void StopPreview(); 
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

  
  
  
  void
  SetState(uint32_t aNewState, const char* aFileOrFunc, int aLine)
  {
#ifdef PR_LOGGING
    const char* states[] = { "stopped", "starting", "started", "stopping" };
    MOZ_ASSERT(mState < sizeof(states) / sizeof(states[0]));
    MOZ_ASSERT(aNewState < sizeof(states) / sizeof(states[0]));
    DOM_CAMERA_LOGI("SetState: (this=%p) '%s' --> '%s' : %s:%d\n", this, states[mState], states[aNewState], aFileOrFunc, aLine);
#endif

    NS_ASSERTION(NS_IsMainThread(), "Preview state set OFF OF main thread!");
    mState = aNewState;
  }

  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mFramesPerSecond;
  CameraPreviewMediaStream* mInput;
  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;
  VideoSegment mVideoSegment;
  uint32_t mFrameCount;
  nsRefPtr<ICameraControl> mCameraControl;

  
  MediaStreamListener* mListener;

private:
  DOMCameraPreview(const DOMCameraPreview&) MOZ_DELETE;
  DOMCameraPreview& operator=(const DOMCameraPreview&) MOZ_DELETE;
};

} 

#define DOM_CAMERA_SETSTATE(newState)   SetState((newState), __func__, __LINE__)

#endif
