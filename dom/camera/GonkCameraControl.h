















#ifndef DOM_CAMERA_GONKCAMERACONTROL_H
#define DOM_CAMERA_GONKCAMERACONTROL_H

#include "base/basictypes.h"
#include <media/MediaProfiles.h>
#include "mozilla/ReentrantMonitor.h"
#include "DeviceStorage.h"
#include "CameraControlImpl.h"
#include "CameraCommon.h"
#include "GonkRecorder.h"
#include "GonkCameraHwMgr.h"
#include "GonkCameraParameters.h"

namespace android {
  class GonkCameraHardware;
  class MediaProfiles;
  class GonkRecorder;
}

namespace mozilla {

namespace layers {
  class TextureClient;
  class ImageContainer;
}

class GonkRecorderProfile;
class GonkRecorderProfileManager;

class nsGonkCameraControl : public CameraControlImpl
{
public:
  nsGonkCameraControl(uint32_t aCameraId);

  void OnAutoFocusComplete(bool aSuccess);
  void OnFacesDetected(camera_frame_metadata_t* aMetaData);
  void OnTakePictureComplete(uint8_t* aData, uint32_t aLength);
  void OnTakePictureError();
  void OnRateLimitPreview(bool aLimit);
  void OnNewPreviewFrame(layers::TextureClient* aBuffer);
  void OnRecorderEvent(int msg, int ext1, int ext2);
  void OnSystemError(CameraControlListener::SystemContext aWhere, nsresult aError);

  
  virtual nsresult Set(uint32_t aKey, const nsAString& aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, nsAString& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, double aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, double& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, int32_t aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, int32_t& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, int64_t aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, int64_t& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, bool aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, bool& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, const Size& aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, Size& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, const nsTArray<Region>& aRegions) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, nsTArray<Region>& aRegions) MOZ_OVERRIDE;

  virtual nsresult SetLocation(const Position& aLocation) MOZ_OVERRIDE;

  virtual nsresult Get(uint32_t aKey, nsTArray<Size>& aSizes) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, nsTArray<nsString>& aValues) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, nsTArray<double>& aValues) MOZ_OVERRIDE;

  nsresult PushParameters();
  nsresult PullParameters();

protected:
  ~nsGonkCameraControl();

  using CameraControlImpl::OnRateLimitPreview;
  using CameraControlImpl::OnNewPreviewFrame;
  using CameraControlImpl::OnAutoFocusComplete;
  using CameraControlImpl::OnFacesDetected;
  using CameraControlImpl::OnTakePictureComplete;
  using CameraControlImpl::OnConfigurationChange;
  using CameraControlImpl::OnUserError;

  virtual void BeginBatchParameterSet() MOZ_OVERRIDE;
  virtual void EndBatchParameterSet() MOZ_OVERRIDE;

  nsresult Initialize();

  nsresult SetConfigurationInternal(const Configuration& aConfig);
  nsresult SetPictureConfiguration(const Configuration& aConfig);
  nsresult SetVideoConfiguration(const Configuration& aConfig);

  template<class T> nsresult SetAndPush(uint32_t aKey, const T& aValue);

  
  virtual nsresult StartImpl(const Configuration* aInitialConfig = nullptr) MOZ_OVERRIDE;
  virtual nsresult SetConfigurationImpl(const Configuration& aConfig) MOZ_OVERRIDE;
  virtual nsresult StopImpl() MOZ_OVERRIDE;
  virtual nsresult StartPreviewImpl() MOZ_OVERRIDE;
  virtual nsresult StopPreviewImpl() MOZ_OVERRIDE;
  virtual nsresult AutoFocusImpl() MOZ_OVERRIDE;
  virtual nsresult StartFaceDetectionImpl() MOZ_OVERRIDE;
  virtual nsresult StopFaceDetectionImpl() MOZ_OVERRIDE;
  virtual nsresult TakePictureImpl() MOZ_OVERRIDE;
  virtual nsresult StartRecordingImpl(DeviceStorageFileDescriptor* aFileDescriptor,
                                      const StartRecordingOptions* aOptions = nullptr) MOZ_OVERRIDE;
  virtual nsresult StopRecordingImpl() MOZ_OVERRIDE;
  virtual nsresult ResumeContinuousFocusImpl() MOZ_OVERRIDE;
  virtual nsresult PushParametersImpl() MOZ_OVERRIDE;
  virtual nsresult PullParametersImpl() MOZ_OVERRIDE;
  virtual already_AddRefed<RecorderProfileManager> GetRecorderProfileManagerImpl() MOZ_OVERRIDE;
  already_AddRefed<GonkRecorderProfileManager> GetGonkRecorderProfileManager();

  nsresult SetupRecording(int aFd, int aRotation, uint64_t aMaxFileSizeBytes,
                          uint64_t aMaxVideoLengthMs);
  nsresult SetupRecordingFlash(bool aAutoEnableLowLightTorch);
  nsresult SetPreviewSize(const Size& aSize);
  nsresult SetVideoSize(const Size& aSize);
  nsresult PausePreview();
  nsresult GetSupportedSize(const Size& aSize, const nsTArray<Size>& supportedSizes, Size& best);

  friend class SetPictureSize;
  friend class SetThumbnailSize;
  nsresult SetPictureSize(const Size& aSize);
  nsresult SetPictureSizeImpl(const Size& aSize);
  nsresult SetThumbnailSize(const Size& aSize);
  nsresult UpdateThumbnailSize();
  nsresult SetThumbnailSizeImpl(const Size& aSize);

  int32_t RationalizeRotation(int32_t aRotation);

  android::sp<android::GonkCameraHardware> mCameraHw;

  Size                      mLastPictureSize;
  Size                      mLastThumbnailSize;
  Size                      mLastRecorderSize;
  uint32_t                  mPreviewFps;
  bool                      mResumePreviewAfterTakingPicture;
  bool                      mFlashSupported;
  bool                      mLuminanceSupported;
  bool                      mAutoFlashModeOverridden;
  bool                      mSeparateVideoAndPreviewSizesSupported;
  Atomic<uint32_t>          mDeferConfigUpdate;
  GonkCameraParameters      mParams;

  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;

  android::MediaProfiles*   mMediaProfiles;
  nsRefPtr<android::GonkRecorder> mRecorder;
  
  
  
  ReentrantMonitor          mRecorderMonitor;

  
  nsRefPtr<GonkRecorderProfileManager> mProfileManager;
  nsRefPtr<GonkRecorderProfile> mRecorderProfile;

  nsRefPtr<DeviceStorageFile> mVideoFile;
  nsString                  mFileFormat;

  
  ReentrantMonitor          mReentrantMonitor;

private:
  nsGonkCameraControl(const nsGonkCameraControl&) MOZ_DELETE;
  nsGonkCameraControl& operator=(const nsGonkCameraControl&) MOZ_DELETE;
};


void OnRateLimitPreview(nsGonkCameraControl* gc, bool aLimit);
void OnTakePictureComplete(nsGonkCameraControl* gc, uint8_t* aData, uint32_t aLength);
void OnTakePictureError(nsGonkCameraControl* gc);
void OnAutoFocusComplete(nsGonkCameraControl* gc, bool aSuccess);
void OnAutoFocusMoving(nsGonkCameraControl* gc, bool aIsMoving);
void OnFacesDetected(nsGonkCameraControl* gc, camera_frame_metadata_t* aMetaData);
void OnNewPreviewFrame(nsGonkCameraControl* gc, layers::TextureClient* aBuffer);
void OnShutter(nsGonkCameraControl* gc);
void OnClosed(nsGonkCameraControl* gc);
void OnSystemError(nsGonkCameraControl* gc,
                   CameraControlListener::SystemContext aWhere,
                   int32_t aArg1, int32_t aArg2);

} 

#endif 
