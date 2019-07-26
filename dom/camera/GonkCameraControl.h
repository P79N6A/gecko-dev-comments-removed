















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
  void OnTakePictureComplete(uint8_t* aData, uint32_t aLength);
  void OnTakePictureError();
  void OnNewPreviewFrame(layers::TextureClient* aBuffer);
  void OnRecorderEvent(int msg, int ext1, int ext2);
  void OnError(CameraControlListener::CameraErrorContext aWhere,
               CameraControlListener::CameraError aError);

  virtual nsresult Set(uint32_t aKey, const nsAString& aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, nsAString& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, double aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, double& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, int32_t aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, int32_t& aValue) MOZ_OVERRIDE;
  virtual nsresult Set(uint32_t aKey, int64_t aValue) MOZ_OVERRIDE;
  virtual nsresult Get(uint32_t aKey, int64_t& aValue) MOZ_OVERRIDE;
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

  using CameraControlImpl::OnNewPreviewFrame;
  using CameraControlImpl::OnAutoFocusComplete;
  using CameraControlImpl::OnTakePictureComplete;
  using CameraControlImpl::OnConfigurationChange;
  using CameraControlImpl::OnError;

  virtual void BeginBatchParameterSet() MOZ_OVERRIDE;
  virtual void EndBatchParameterSet() MOZ_OVERRIDE;

  virtual nsresult StartImpl(const Configuration* aInitialConfig = nullptr) MOZ_OVERRIDE;
  virtual nsresult StopImpl() MOZ_OVERRIDE;
  nsresult Initialize();

  virtual nsresult SetConfigurationImpl(const Configuration& aConfig) MOZ_OVERRIDE;
  nsresult SetConfigurationInternal(const Configuration& aConfig);
  nsresult SetPictureConfiguration(const Configuration& aConfig);
  nsresult SetVideoConfiguration(const Configuration& aConfig);

  template<class T> nsresult SetAndPush(uint32_t aKey, const T& aValue);

  virtual nsresult StartPreviewImpl() MOZ_OVERRIDE;
  virtual nsresult StopPreviewImpl() MOZ_OVERRIDE;
  virtual nsresult AutoFocusImpl(bool aCancelExistingCall) MOZ_OVERRIDE;
  virtual nsresult TakePictureImpl() MOZ_OVERRIDE;
  virtual nsresult StartRecordingImpl(DeviceStorageFileDescriptor* aFileDescriptor,
                                      const StartRecordingOptions* aOptions = nullptr) MOZ_OVERRIDE;
  virtual nsresult StopRecordingImpl() MOZ_OVERRIDE;
  virtual nsresult PushParametersImpl() MOZ_OVERRIDE;
  virtual nsresult PullParametersImpl() MOZ_OVERRIDE;
  virtual already_AddRefed<RecorderProfileManager> GetRecorderProfileManagerImpl() MOZ_OVERRIDE;
  already_AddRefed<GonkRecorderProfileManager> GetGonkRecorderProfileManager();

  nsresult SetupRecording(int aFd, int aRotation, int64_t aMaxFileSizeBytes, int64_t aMaxVideoLengthMs);
  nsresult SetupVideoMode(const nsAString& aProfile);
  nsresult SetPreviewSize(const Size& aSize);
  nsresult PausePreview();

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

  Atomic<uint32_t>          mDeferConfigUpdate;
  GonkCameraParameters      mParams;

  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;

  android::MediaProfiles*   mMediaProfiles;
  nsRefPtr<android::GonkRecorder> mRecorder;

  
  nsRefPtr<GonkRecorderProfileManager> mProfileManager;
  nsRefPtr<GonkRecorderProfile> mRecorderProfile;

  nsRefPtr<DeviceStorageFile> mVideoFile;
  nsString                  mFileFormat;

  
  ReentrantMonitor          mReentrantMonitor;

private:
  nsGonkCameraControl(const nsGonkCameraControl&) MOZ_DELETE;
  nsGonkCameraControl& operator=(const nsGonkCameraControl&) MOZ_DELETE;
};


void OnTakePictureComplete(nsGonkCameraControl* gc, uint8_t* aData, uint32_t aLength);
void OnTakePictureError(nsGonkCameraControl* gc);
void OnAutoFocusComplete(nsGonkCameraControl* gc, bool aSuccess);
void OnNewPreviewFrame(nsGonkCameraControl* gc, layers::TextureClient* aBuffer);
void OnShutter(nsGonkCameraControl* gc);
void OnClosed(nsGonkCameraControl* gc);
void OnError(nsGonkCameraControl* gc, CameraControlListener::CameraError aError,
             int32_t aArg1, int32_t aArg2);

} 

#endif 
