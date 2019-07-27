



#ifndef DOM_CAMERA_DOMCAMERACONTROL_H
#define DOM_CAMERA_DOMCAMERACONTROL_H

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/CameraControlBinding.h"
#include "ICameraControl.h"
#include "CameraCommon.h"
#include "DOMMediaStream.h"
#include "AudioChannelAgent.h"
#include "nsProxyRelease.h"
#include "nsHashPropertyBag.h"
#include "DeviceStorage.h"
#include "DOMCameraControlListener.h"

class nsDOMDeviceStorage;
class nsPIDOMWindow;
class nsIDOMBlob;

namespace mozilla {

namespace dom {
  class CameraCapabilities;
  struct CameraPictureOptions;
  struct CameraStartRecordingOptions;
  struct CameraRegion;
  struct CameraSize;
  template<typename T> class Optional;
}
class ErrorResult;
class StartRecordingHelper;


class nsDOMCameraControl MOZ_FINAL : public DOMMediaStream
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMCameraControl, DOMMediaStream)
  NS_DECL_ISUPPORTS_INHERITED

  
  
  
  
  
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  nsDOMCameraControl(uint32_t aCameraId,
                     const dom::CameraConfiguration& aInitialConfig,
                     dom::GetCameraCallback* aOnSuccess,
                     dom::CameraErrorCallback* aOnError,
                     nsPIDOMWindow* aWindow);

  void Shutdown();

  nsPIDOMWindow* GetParentObject() const { return mWindow; }

  
  void GetEffect(nsString& aEffect, ErrorResult& aRv);
  void SetEffect(const nsAString& aEffect, ErrorResult& aRv);
  void GetWhiteBalanceMode(nsString& aMode, ErrorResult& aRv);
  void SetWhiteBalanceMode(const nsAString& aMode, ErrorResult& aRv);
  void GetSceneMode(nsString& aMode, ErrorResult& aRv);
  void SetSceneMode(const nsAString& aMode, ErrorResult& aRv);
  void GetFlashMode(nsString& aMode, ErrorResult& aRv);
  void SetFlashMode(const nsAString& aMode, ErrorResult& aRv);
  void GetFocusMode(nsString& aMode, ErrorResult& aRv);
  void SetFocusMode(const nsAString& aMode, ErrorResult& aRv);
  double GetZoom(ErrorResult& aRv);
  void SetZoom(double aZoom, ErrorResult& aRv);
  double GetFocalLength(ErrorResult& aRv);
  double GetFocusDistanceNear(ErrorResult& aRv);
  double GetFocusDistanceOptimum(ErrorResult& aRv);
  double GetFocusDistanceFar(ErrorResult& aRv);
  void SetExposureCompensation(double aCompensation, ErrorResult& aRv);
  double GetExposureCompensation(ErrorResult& aRv);
  int32_t SensorAngle();
  already_AddRefed<dom::CameraCapabilities> Capabilities();
  void GetIsoMode(nsString& aMode, ErrorResult& aRv);
  void SetIsoMode(const nsAString& aMode, ErrorResult& aRv);
  double GetPictureQuality(ErrorResult& aRv);
  void SetPictureQuality(double aQuality, ErrorResult& aRv);

  
  dom::CameraShutterCallback* GetOnShutter();
  void SetOnShutter(dom::CameraShutterCallback* aCb);
  dom::CameraClosedCallback* GetOnClosed();
  void SetOnClosed(dom::CameraClosedCallback* aCb);
  dom::CameraRecorderStateChange* GetOnRecorderStateChange();
  void SetOnRecorderStateChange(dom::CameraRecorderStateChange* aCb);
  dom::CameraPreviewStateChange* GetOnPreviewStateChange();
  void SetOnPreviewStateChange(dom::CameraPreviewStateChange* aCb);
  dom::CameraAutoFocusMovingCallback* GetOnAutoFocusMoving();
  void SetOnAutoFocusMoving(dom::CameraAutoFocusMovingCallback* aCb);
  dom::CameraFaceDetectionCallback* GetOnFacesDetected();
  void SetOnFacesDetected(dom::CameraFaceDetectionCallback* aCb);

  
  void SetConfiguration(const dom::CameraConfiguration& aConfiguration,
                        const dom::Optional<dom::OwningNonNull<dom::CameraSetConfigurationCallback> >& aOnSuccess,
                        const dom::Optional<dom::OwningNonNull<dom::CameraErrorCallback> >& aOnError,
                        ErrorResult& aRv);
  void GetMeteringAreas(nsTArray<dom::CameraRegion>& aAreas, ErrorResult& aRv);
  void SetMeteringAreas(const dom::Optional<dom::Sequence<dom::CameraRegion> >& aAreas, ErrorResult& aRv);
  void GetFocusAreas(nsTArray<dom::CameraRegion>& aAreas, ErrorResult& aRv);
  void SetFocusAreas(const dom::Optional<dom::Sequence<dom::CameraRegion> >& aAreas, ErrorResult& aRv);
  void GetPictureSize(dom::CameraSize& aSize, ErrorResult& aRv);
  void SetPictureSize(const dom::CameraSize& aSize, ErrorResult& aRv);
  void GetThumbnailSize(dom::CameraSize& aSize, ErrorResult& aRv);
  void SetThumbnailSize(const dom::CameraSize& aSize, ErrorResult& aRv);
  void AutoFocus(dom::CameraAutoFocusCallback& aOnSuccess,
                 const dom::Optional<dom::OwningNonNull<dom::CameraErrorCallback> >& aOnError,
                 ErrorResult& aRv);
  void StartFaceDetection(ErrorResult& aRv);
  void StopFaceDetection(ErrorResult& aRv);
  void TakePicture(const dom::CameraPictureOptions& aOptions,
                   dom::CameraTakePictureCallback& aOnSuccess,
                   const dom::Optional<dom::OwningNonNull<dom::CameraErrorCallback> >& aOnError,
                   ErrorResult& aRv);
  void StartRecording(const dom::CameraStartRecordingOptions& aOptions,
                      nsDOMDeviceStorage& storageArea,
                      const nsAString& filename,
                      dom::CameraStartRecordingCallback& aOnSuccess,
                      const dom::Optional<dom::OwningNonNull<dom::CameraErrorCallback> >& aOnError,
                      ErrorResult& aRv);
  void StopRecording(ErrorResult& aRv);
  void ResumePreview(ErrorResult& aRv);
  void ReleaseHardware(const dom::Optional<dom::OwningNonNull<dom::CameraReleaseCallback> >& aOnSuccess,
                       const dom::Optional<dom::OwningNonNull<dom::CameraErrorCallback> >& aOnError,
                       ErrorResult& aRv);
  void ResumeContinuousFocus(ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

protected:
  virtual ~nsDOMCameraControl();

  class DOMCameraConfiguration MOZ_FINAL : public dom::CameraConfiguration
  {
  public:
    NS_INLINE_DECL_REFCOUNTING(DOMCameraConfiguration)

    DOMCameraConfiguration();
    explicit DOMCameraConfiguration(const dom::CameraConfiguration& aConfiguration);

    
    uint32_t mMaxFocusAreas;
    uint32_t mMaxMeteringAreas;

  private:
    
    ~DOMCameraConfiguration();
  };

  friend class DOMCameraControlListener;
  friend class mozilla::StartRecordingHelper;

  void OnCreatedFileDescriptor(bool aSucceeded);

  void OnAutoFocusComplete(bool aAutoFocusSucceeded);
  void OnAutoFocusMoving(bool aIsMoving);
  void OnTakePictureComplete(nsIDOMBlob* aPicture);
  void OnFacesDetected(const nsTArray<ICameraControl::Face>& aFaces);

  void OnHardwareStateChange(DOMCameraControlListener::HardwareState aState);
  void OnPreviewStateChange(DOMCameraControlListener::PreviewState aState);
  void OnRecorderStateChange(CameraControlListener::RecorderState aState, int32_t aStatus, int32_t aTrackNum);
  void OnConfigurationChange(DOMCameraConfiguration* aConfiguration);
  void OnShutter();
  void OnUserError(CameraControlListener::UserContext aContext, nsresult aError);

  bool IsWindowStillActive();

  nsresult NotifyRecordingStatusChange(const nsString& aMsg);

  nsRefPtr<ICameraControl> mCameraControl; 

  
  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;

  nsresult Set(uint32_t aKey, const dom::Optional<dom::Sequence<dom::CameraRegion> >& aValue, uint32_t aLimit);
  nsresult Get(uint32_t aKey, nsTArray<dom::CameraRegion>& aValue);

  nsRefPtr<DOMCameraConfiguration>              mCurrentConfiguration;
  nsRefPtr<dom::CameraCapabilities>             mCapabilities;

  
  nsRefPtr<dom::GetCameraCallback>              mGetCameraOnSuccessCb;
  nsRefPtr<dom::CameraErrorCallback>            mGetCameraOnErrorCb;
  nsRefPtr<dom::CameraAutoFocusCallback>        mAutoFocusOnSuccessCb;
  nsRefPtr<dom::CameraErrorCallback>            mAutoFocusOnErrorCb;
  nsRefPtr<dom::CameraTakePictureCallback>      mTakePictureOnSuccessCb;
  nsRefPtr<dom::CameraErrorCallback>            mTakePictureOnErrorCb;
  nsRefPtr<dom::CameraStartRecordingCallback>   mStartRecordingOnSuccessCb;
  nsRefPtr<dom::CameraErrorCallback>            mStartRecordingOnErrorCb;
  nsRefPtr<dom::CameraReleaseCallback>          mReleaseOnSuccessCb;
  nsRefPtr<dom::CameraErrorCallback>            mReleaseOnErrorCb;
  nsRefPtr<dom::CameraSetConfigurationCallback> mSetConfigurationOnSuccessCb;
  nsRefPtr<dom::CameraErrorCallback>            mSetConfigurationOnErrorCb;

  
  nsRefPtr<dom::CameraShutterCallback>          mOnShutterCb;
  nsRefPtr<dom::CameraClosedCallback>           mOnClosedCb;
  nsRefPtr<dom::CameraRecorderStateChange>      mOnRecorderStateChangeCb;
  nsRefPtr<dom::CameraPreviewStateChange>       mOnPreviewStateChangeCb;
  nsRefPtr<dom::CameraAutoFocusMovingCallback>  mOnAutoFocusMovingCb;
  nsRefPtr<dom::CameraFaceDetectionCallback>    mOnFacesDetectedCb;

  
  
  
  DOMCameraControlListener* mListener;

  
  nsRefPtr<CameraPreviewMediaStream> mInput;

  
  nsCOMPtr<nsPIDOMWindow>   mWindow;

  dom::CameraStartRecordingOptions mOptions;
  nsRefPtr<DeviceStorageFileDescriptor> mDSFileDescriptor;

private:
  nsDOMCameraControl(const nsDOMCameraControl&) MOZ_DELETE;
  nsDOMCameraControl& operator=(const nsDOMCameraControl&) MOZ_DELETE;
};

} 

#endif 
