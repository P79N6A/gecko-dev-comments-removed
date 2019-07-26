















#ifndef DOM_CAMERA_GONKCAMERACONTROL_H
#define DOM_CAMERA_GONKCAMERACONTROL_H

#include "base/basictypes.h"
#include "prrwlock.h"
#include <media/MediaProfiles.h>
#include "nsIDOMCameraManager.h"
#include "DOMCameraControl.h"
#include "CameraControlImpl.h"
#include "CameraCommon.h"
#include "GonkRecorder.h"

using namespace android;

namespace mozilla {

namespace layers {
class GraphicBufferLocked;
}

class GonkRecorderProfile;
class GonkRecorderProfileManager;

class nsGonkCameraControl : public CameraControlImpl
{
public:
  nsGonkCameraControl(uint32_t aCameraId, nsIThread* aCameraThread, nsDOMCameraControl* aDOMCameraControl, nsICameraGetCameraCallback* onSuccess, nsICameraErrorCallback* onError, uint64_t aWindowId);
  nsresult Init();

  const char* GetParameter(const char* aKey);
  const char* GetParameterConstChar(uint32_t aKey);
  double GetParameterDouble(uint32_t aKey);
  void GetParameter(uint32_t aKey, nsTArray<dom::CameraRegion>& aRegions);
  void GetParameter(uint32_t aKey, nsTArray<CameraSize>& aSizes);
  void SetParameter(const char* aKey, const char* aValue);
  void SetParameter(uint32_t aKey, const char* aValue);
  void SetParameter(uint32_t aKey, double aValue);
  void SetParameter(uint32_t aKey, const nsTArray<dom::CameraRegion>& aRegions);
  void SetParameter(uint32_t aKey, int aValue);
  nsresult GetVideoSizes(nsTArray<CameraSize>& aVideoSizes);
  nsresult PushParameters();

  nsresult SetupRecording(int aFd, int64_t aMaxFileSizeBytes = -1, int64_t aMaxVideoLengthMs = -1);
  nsresult SetupVideoMode(const nsAString& aProfile);

  void AutoFocusComplete(bool aSuccess);
  void TakePictureComplete(uint8_t* aData, uint32_t aLength);
  void HandleRecorderEvent(int msg, int ext1, int ext2);

protected:
  ~nsGonkCameraControl();

  nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream);
  nsresult StartPreviewImpl(StartPreviewTask* aStartPreview);
  nsresult StopPreviewImpl(StopPreviewTask* aStopPreview);
  nsresult StopPreviewInternal(bool aForced = false);
  nsresult AutoFocusImpl(AutoFocusTask* aAutoFocus);
  nsresult TakePictureImpl(TakePictureTask* aTakePicture);
  nsresult StartRecordingImpl(StartRecordingTask* aStartRecording);
  nsresult StopRecordingImpl(StopRecordingTask* aStopRecording);
  nsresult PushParametersImpl();
  nsresult PullParametersImpl();
  nsresult GetPreviewStreamVideoModeImpl(GetPreviewStreamVideoModeTask* aGetPreviewStreamVideoMode);
  already_AddRefed<RecorderProfileManager> GetRecorderProfileManagerImpl();
  already_AddRefed<GonkRecorderProfileManager> GetGonkRecorderProfileManager();

  void SetPreviewSize(uint32_t aWidth, uint32_t aHeight);
  void SetupThumbnail(uint32_t aPictureWidth, uint32_t aPictureHeight, uint32_t aPercentQuality);

  uint32_t                  mHwHandle;
  double                    mExposureCompensationMin;
  double                    mExposureCompensationStep;
  bool                      mDeferConfigUpdate;
  PRRWLock*                 mRwLock;
  android::CameraParameters mParams;
  uint32_t                  mWidth;
  uint32_t                  mHeight;
  uint32_t                  mLastPictureWidth;
  uint32_t                  mLastPictureHeight;

  enum {
    PREVIEW_FORMAT_UNKNOWN,
    PREVIEW_FORMAT_YUV420P,
    PREVIEW_FORMAT_YUV420SP
  };
  uint32_t                  mFormat;

  uint32_t                  mFps;
  uint32_t                  mDiscardedFrameCount;

  android::MediaProfiles*   mMediaProfiles;
  android::GonkRecorder*    mRecorder;

  uint32_t                  mVideoRotation;
  nsString                  mVideoFile;

  
  nsRefPtr<GonkRecorderProfileManager> mProfileManager;
  nsRefPtr<GonkRecorderProfile> mRecorderProfile;

private:
  nsGonkCameraControl(const nsGonkCameraControl&) MOZ_DELETE;
  nsGonkCameraControl& operator=(const nsGonkCameraControl&) MOZ_DELETE;
};


void ReceiveImage(nsGonkCameraControl* gc, uint8_t* aData, uint32_t aLength);
void AutoFocusComplete(nsGonkCameraControl* gc, bool aSuccess);
void ReceiveFrame(nsGonkCameraControl* gc, layers::GraphicBufferLocked* aBuffer);
void OnShutter(nsGonkCameraControl* gc);
void OnClosed(nsGonkCameraControl* gc);

} 

#endif 
