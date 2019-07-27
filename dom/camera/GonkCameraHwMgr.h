















#ifndef DOM_CAMERA_GONKCAMERAHWMGR_H
#define DOM_CAMERA_GONKCAMERAHWMGR_H

#include "GonkCameraControl.h"
#include "CameraCommon.h"
#include "GonkCameraParameters.h"
#include "mozilla/ReentrantMonitor.h"

#ifdef MOZ_WIDGET_GONK
#include <binder/IMemory.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <utils/threads.h>
#include "GonkCameraListener.h"
#include "GonkNativeWindow.h"
#else
#include "FallbackCameraPlatform.h"
#endif

namespace mozilla {
  class nsGonkCameraControl;
  class GonkCameraParameters;
}

namespace android {

class GonkCameraHardware
#ifdef MOZ_WIDGET_GONK
  : public GonkNativeWindowNewFrameCallback
  , public CameraListener
#else
  : public nsISupports
#endif
{
#ifndef MOZ_WIDGET_GONK
  NS_DECL_ISUPPORTS
#endif

protected:
  GonkCameraHardware(mozilla::nsGonkCameraControl* aTarget, uint32_t aCameraId, const sp<Camera>& aCamera);
  virtual ~GonkCameraHardware();

  
  
  
  
  
  virtual nsresult Init();

public:
  static sp<GonkCameraHardware> Connect(mozilla::nsGonkCameraControl* aTarget, uint32_t aCameraId);
  virtual void Close();

  virtual void OnRateLimitPreview(bool aLimit);

#ifdef MOZ_WIDGET_GONK
  
  virtual void OnNewFrame() override;

  
  virtual void notify(int32_t aMsgType, int32_t ext1, int32_t ext2);
  virtual void postData(int32_t aMsgType, const sp<IMemory>& aDataPtr, camera_frame_metadata_t* metadata);
  virtual void postDataTimestamp(nsecs_t aTimestamp, int32_t aMsgType, const sp<IMemory>& aDataPtr);
#endif

  













  enum {
    RAW_SENSOR_ORIENTATION,
    OFFSET_SENSOR_ORIENTATION
  };
  virtual int      GetSensorOrientation(uint32_t aType = RAW_SENSOR_ORIENTATION);

  virtual bool     IsEmulated();

  






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
  virtual nsresult PullParameters(mozilla::GonkCameraParameters& aParams);
#ifdef MOZ_WIDGET_GONK
  virtual int      PushParameters(const CameraParameters& aParams);
  virtual void     PullParameters(CameraParameters& aParams);
  virtual int      SetListener(const sp<GonkCameraListener>& aListener);
  virtual void     ReleaseRecordingFrame(const sp<IMemory>& aFrame);
#endif
  virtual int      StartRecording();
  virtual int      StopRecording();
  virtual int      StoreMetaDataInBuffers(bool aEnabled);

protected:
  uint32_t                      mCameraId;
  bool                          mClosing;
  uint32_t                      mNumFrames;
  sp<Camera>                    mCamera;
  mozilla::nsGonkCameraControl* mTarget;
#ifdef MOZ_WIDGET_GONK
  sp<GonkNativeWindow>          mNativeWindow;
  sp<GonkCameraListener>        mListener;
#endif
  int                           mRawSensorOrientation;
  int                           mSensorOrientation;
  bool                          mEmulated;

private:
  GonkCameraHardware(const GonkCameraHardware&) = delete;
  GonkCameraHardware& operator=(const GonkCameraHardware&) = delete;
};

} 

#endif 
