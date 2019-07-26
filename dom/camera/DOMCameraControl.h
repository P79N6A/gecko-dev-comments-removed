



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
  class CameraPictureOptions;
  class CameraStartRecordingOptions;
  template<typename T> class Optional;
}
class ErrorResult;
class StartRecordingHelper;


class nsDOMCameraControl MOZ_FINAL : public DOMMediaStream
{
public:
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMCameraControl, DOMMediaStream)
  NS_DECL_ISUPPORTS_INHERITED

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
  JS::Value GetMeteringAreas(JSContext* aCx, ErrorResult& aRv);
  void SetMeteringAreas(JSContext* aCx, JS::Handle<JS::Value> aAreas, ErrorResult& aRv);
  JS::Value GetFocusAreas(JSContext* aCx, ErrorResult& aRv);
  void SetFocusAreas(JSContext* aCx, JS::Handle<JS::Value> aAreas, ErrorResult& aRv);
  JS::Value GetPictureSize(JSContext* aCx, ErrorResult& aRv);
  void SetPictureSize(JSContext* aCx, JS::Handle<JS::Value> aSize, ErrorResult& aRv);
  JS::Value GetThumbnailSize(JSContext* aCx, ErrorResult& aRv);
  void SetThumbnailSize(JSContext* aCx, JS::Handle<JS::Value> aSize, ErrorResult& aRv);
  double GetFocalLength(ErrorResult& aRv);
  double GetFocusDistanceNear(ErrorResult& aRv);
  double GetFocusDistanceOptimum(ErrorResult& aRv);
  double GetFocusDistanceFar(ErrorResult& aRv);
  void SetExposureCompensation(const dom::Optional<double>& aCompensation, ErrorResult& aRv);
  double GetExposureCompensation(ErrorResult& aRv);
  int32_t SensorAngle();
  already_AddRefed<dom::CameraCapabilities> Capabilities();

  
  already_AddRefed<dom::CameraShutterCallback> GetOnShutter();
  void SetOnShutter(dom::CameraShutterCallback* aCb);
  already_AddRefed<dom::CameraClosedCallback> GetOnClosed();
  void SetOnClosed(dom::CameraClosedCallback* aCb);
  already_AddRefed<dom::CameraRecorderStateChange> GetOnRecorderStateChange();
  void SetOnRecorderStateChange(dom::CameraRecorderStateChange* aCb);
  already_AddRefed<dom::CameraPreviewStateChange> GetOnPreviewStateChange();
  void SetOnPreviewStateChange(dom::CameraPreviewStateChange* aCb);

  
  void SetConfiguration(const dom::CameraConfiguration& aConfiguration,
                        const dom::Optional<dom::OwningNonNull<dom::CameraSetConfigurationCallback> >& aOnSuccess,
                        const dom::Optional<dom::OwningNonNull<dom::CameraErrorCallback> >& aOnError,
                        ErrorResult& aRv);
  void AutoFocus(dom::CameraAutoFocusCallback& aOnSuccess,
                 const dom::Optional<dom::OwningNonNull<dom::CameraErrorCallback> >& aOnError,
                 ErrorResult& aRv);
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

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

protected:
  virtual ~nsDOMCameraControl();

  class DOMCameraConfiguration : public dom::CameraConfiguration
  {
  public:
    NS_INLINE_DECL_REFCOUNTING(DOMCameraConfiguration)

    DOMCameraConfiguration();
    DOMCameraConfiguration(const dom::CameraConfiguration& aConfiguration);

    
    uint32_t mMaxFocusAreas;
    uint32_t mMaxMeteringAreas;

  protected:
    ~DOMCameraConfiguration();
  };

  friend class DOMCameraControlListener;
  friend class mozilla::StartRecordingHelper;

  void OnCreatedFileDescriptor(bool aSucceeded);

  void OnAutoFocusComplete(bool aAutoFocusSucceeded);
  void OnTakePictureComplete(nsIDOMBlob* aPicture);

  void OnHardwareStateChange(DOMCameraControlListener::HardwareState aState);
  void OnPreviewStateChange(DOMCameraControlListener::PreviewState aState);
  void OnRecorderStateChange(CameraControlListener::RecorderState aState, int32_t aStatus, int32_t aTrackNum);
  void OnConfigurationChange(DOMCameraConfiguration* aConfiguration);
  void OnShutter();
  void OnError(CameraControlListener::CameraErrorContext aContext, const nsAString& mError);

  bool IsWindowStillActive();

  nsresult NotifyRecordingStatusChange(const nsString& aMsg);

  nsRefPtr<ICameraControl> mCameraControl; 

  
  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;

  nsresult Set(JSContext* aCx, uint32_t aKey, const JS::Value& aValue, uint32_t aLimit);
  nsresult Get(JSContext* aCx, uint32_t aKey, JS::Value* aValue);

  nsRefPtr<DOMCameraConfiguration>              mCurrentConfiguration;
  nsRefPtr<dom::CameraCapabilities>             mCapabilities;

  
  nsCOMPtr<dom::GetCameraCallback>              mGetCameraOnSuccessCb;
  nsCOMPtr<dom::CameraErrorCallback>            mGetCameraOnErrorCb;
  nsCOMPtr<dom::CameraAutoFocusCallback>        mAutoFocusOnSuccessCb;
  nsCOMPtr<dom::CameraErrorCallback>            mAutoFocusOnErrorCb;
  nsCOMPtr<dom::CameraTakePictureCallback>      mTakePictureOnSuccessCb;
  nsCOMPtr<dom::CameraErrorCallback>            mTakePictureOnErrorCb;
  nsCOMPtr<dom::CameraStartRecordingCallback>   mStartRecordingOnSuccessCb;
  nsCOMPtr<dom::CameraErrorCallback>            mStartRecordingOnErrorCb;
  nsCOMPtr<dom::CameraReleaseCallback>          mReleaseOnSuccessCb;
  nsCOMPtr<dom::CameraErrorCallback>            mReleaseOnErrorCb;
  nsCOMPtr<dom::CameraSetConfigurationCallback> mSetConfigurationOnSuccessCb;
  nsCOMPtr<dom::CameraErrorCallback>            mSetConfigurationOnErrorCb;

  
  nsCOMPtr<dom::CameraShutterCallback>          mOnShutterCb;
  nsCOMPtr<dom::CameraClosedCallback>           mOnClosedCb;
  nsCOMPtr<dom::CameraRecorderStateChange>      mOnRecorderStateChangeCb;
  nsCOMPtr<dom::CameraPreviewStateChange>       mOnPreviewStateChangeCb;

  
  
  
  DOMCameraControlListener* mListener;

  
  CameraPreviewMediaStream* mInput;

  
  nsCOMPtr<nsPIDOMWindow>   mWindow;

  dom::CameraStartRecordingOptions mOptions;
  nsRefPtr<DeviceStorageFileDescriptor> mDSFileDescriptor;

private:
  nsDOMCameraControl(const nsDOMCameraControl&) MOZ_DELETE;
  nsDOMCameraControl& operator=(const nsDOMCameraControl&) MOZ_DELETE;
};

} 

#endif 
