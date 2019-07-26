















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
#include "GonkCameraParameters.h"
#include "mozilla/ReentrantMonitor.h"

namespace mozilla {
  class nsGonkCameraControl;
  class GonkCameraParameters;
}

namespace android {

class GonkCameraHardware : public GonkNativeWindowNewFrameCallback
                         , public CameraListener
{
protected:
  GonkCameraHardware(mozilla::nsGonkCameraControl* aTarget, uint32_t aCameraId, const sp<Camera>& aCamera);
  virtual ~GonkCameraHardware();

  
  
  
  
  
  virtual nsresult Init();

public:
  static sp<GonkCameraHardware> Connect(mozilla::nsGonkCameraControl* aTarget, uint32_t aCameraId);
  virtual void Close();

  
  virtual void OnNewFrame() MOZ_OVERRIDE;

  
  virtual void notify(int32_t aMsgType, int32_t ext1, int32_t ext2);
  virtual void postData(int32_t aMsgType, const sp<IMemory>& aDataPtr, camera_frame_metadata_t* metadata);
  virtual void postDataTimestamp(nsecs_t aTimestamp, int32_t aMsgType, const sp<IMemory>& aDataPtr);

  













  enum {
    RAW_SENSOR_ORIENTATION,
    OFFSET_SENSOR_ORIENTATION
  };
  virtual int      GetSensorOrientation(uint32_t aType = RAW_SENSOR_ORIENTATION);

  






  enum { MIN_UNDEQUEUED_BUFFERS = 4};

  virtual int      AutoFocus();
  virtual int      CancelAutoFocus();
  virtual int      StartFaceDetection();
  virtual int      StopFaceDetection();
  virtual int      TakePicture();
  virtual void     CancelTakePicture();
  virtual int      StartPreview();
  virtual void     StopPreview();
  virtual int      PushParameters(const mozilla::GonkCameraParameters& aParams);
  virtual int      PushParameters(const CameraParameters& aParams);
  virtual nsresult PullParameters(mozilla::GonkCameraParameters& aParams);
  virtual void     PullParameters(CameraParameters& aParams);
  virtual int      StartRecording();
  virtual int      StopRecording();
  virtual int      SetListener(const sp<GonkCameraListener>& aListener);
  virtual void     ReleaseRecordingFrame(const sp<IMemory>& aFrame);
  virtual int      StoreMetaDataInBuffers(bool aEnabled);

protected:
  uint32_t                      mCameraId;
  bool                          mClosing;
  uint32_t                      mNumFrames;
  sp<Camera>                    mCamera;
  mozilla::nsGonkCameraControl* mTarget;
  sp<GonkNativeWindow>          mNativeWindow;
  sp<GonkCameraListener>        mListener;
  int                           mRawSensorOrientation;
  int                           mSensorOrientation;

private:
  GonkCameraHardware(const GonkCameraHardware&) MOZ_DELETE;
  GonkCameraHardware& operator=(const GonkCameraHardware&) MOZ_DELETE;
};

} 

#endif 
