















#ifndef DOM_CAMERA_GONKCAMERAHWMGR_H
#define DOM_CAMERA_GONKCAMERAHWMGR_H

#include <binder/IMemory.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <utils/threads.h>

#include "GonkCameraControl.h"
#include "CameraCommon.h"

#include "GonkCameraListener.h"
#include "GonkNativeWindow.h"
#include "mozilla/ReentrantMonitor.h"


#define GIHM_TIMING_RECEIVEFRAME    0
#define GIHM_TIMING_OVERALL         1


namespace mozilla {
  class nsGonkCameraControl;
}

namespace android {

class GonkCameraHardware : public GonkNativeWindowNewFrameCallback
                         , public CameraListener
{
protected:
  GonkCameraHardware(mozilla::nsGonkCameraControl* aTarget, uint32_t aCameraId, const sp<Camera>& aCamera);
  virtual ~GonkCameraHardware();
  void Init();

public:
  static  sp<GonkCameraHardware>  Connect(mozilla::nsGonkCameraControl* aTarget, uint32_t aCameraId);
  void    Close();

  
  virtual void    OnNewFrame() MOZ_OVERRIDE;

  
  virtual void notify(int32_t aMsgType, int32_t ext1, int32_t ext2);
  virtual void postData(int32_t aMsgType, const sp<IMemory>& aDataPtr, camera_frame_metadata_t* metadata);
  virtual void postDataTimestamp(nsecs_t aTimestamp, int32_t aMsgType, const sp<IMemory>& aDataPtr);

  













  enum {
    RAW_SENSOR_ORIENTATION,
    OFFSET_SENSOR_ORIENTATION
  };
  int      GetSensorOrientation(uint32_t aType = OFFSET_SENSOR_ORIENTATION);

  int      AutoFocus();
  void     CancelAutoFocus();
  int      TakePicture();
  void     CancelTakePicture();
  int      StartPreview();
  void     StopPreview();
  int      PushParameters(const CameraParameters& aParams);
  void     PullParameters(CameraParameters& aParams);
  int      StartRecording();
  int      StopRecording();
  int      SetListener(const sp<GonkCameraListener>& aListener);
  void     ReleaseRecordingFrame(const sp<IMemory>& aFrame);
  int      StoreMetaDataInBuffers(bool aEnabled);

protected:

  uint32_t                      mCameraId;
  bool                          mClosing;
  mozilla::ReentrantMonitor     mMonitor;
  uint32_t                      mNumFrames;
  sp<Camera>                    mCamera;
  mozilla::nsGonkCameraControl* mTarget;
  sp<GonkNativeWindow>          mNativeWindow;
#if GIHM_TIMING_OVERALL
  struct timespec               mStart;
  struct timespec               mAutoFocusStart;
#endif
  sp<GonkCameraListener>        mListener;
  bool                          mInitialized;
  int                           mRawSensorOrientation;
  int                           mSensorOrientation;

  bool IsInitialized()
  {
    return mInitialized;
  }

private:
  GonkCameraHardware(const GonkCameraHardware&) MOZ_DELETE;
  GonkCameraHardware& operator=(const GonkCameraHardware&) MOZ_DELETE;
};

} 

#endif 
