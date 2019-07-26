



#ifndef DOM_CAMERA_DOMCAMERACONTROL_H
#define DOM_CAMERA_DOMCAMERACONTROL_H

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "DictionaryHelpers.h"
#include "ICameraControl.h"
#include "DOMCameraPreview.h"
#include "nsIDOMCameraManager.h"
#include "CameraCommon.h"
#include "AudioChannelAgent.h"
#include "nsProxyRelease.h"

class nsDOMDeviceStorage;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
class CameraPictureOptions;
template<typename T> class Optional;
}
class ErrorResult;


class nsDOMCameraControl MOZ_FINAL : public nsISupports,
                                     public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsDOMCameraControl)

  nsDOMCameraControl(uint32_t aCameraId, nsIThread* aCameraThread,
                     nsICameraGetCameraCallback* onSuccess,
                     nsICameraErrorCallback* onError, nsPIDOMWindow* aWindow);
  nsresult Result(nsresult aResult,
                  const nsMainThreadPtrHandle<nsICameraGetCameraCallback>& onSuccess,
                  const nsMainThreadPtrHandle<nsICameraErrorCallback>& onError,
                  uint64_t aWindowId);
  nsRefPtr<ICameraControl> GetNativeCameraControl();

  void Shutdown();

  nsPIDOMWindow* GetParentObject() const { return mWindow; }

  
  nsICameraCapabilities* Capabilities();
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
  double GetFocalLength(ErrorResult& aRv);
  double GetFocusDistanceNear(ErrorResult& aRv);
  double GetFocusDistanceOptimum(ErrorResult& aRv);
  double GetFocusDistanceFar(ErrorResult& aRv);
  void SetExposureCompensation(const dom::Optional<double>& aCompensation, ErrorResult& aRv);
  double GetExposureCompensation(ErrorResult& aRv);
  already_AddRefed<nsICameraShutterCallback> GetOnShutter(ErrorResult& aRv);
  void SetOnShutter(nsICameraShutterCallback* aCb, ErrorResult& aRv);
  already_AddRefed<nsICameraClosedCallback> GetOnClosed(ErrorResult& aRv);
  void SetOnClosed(nsICameraClosedCallback* aCb, ErrorResult& aRv);
  already_AddRefed<nsICameraRecorderStateChange> GetOnRecorderStateChange(ErrorResult& aRv);
  void SetOnRecorderStateChange(nsICameraRecorderStateChange* aCb, ErrorResult& aRv);
  void AutoFocus(nsICameraAutoFocusCallback* aOnSuccess, const dom::Optional<nsICameraErrorCallback*>& aOnErro, ErrorResult& aRvr);
  void TakePicture(JSContext* aCx, const dom::CameraPictureOptions& aOptions,
                   nsICameraTakePictureCallback* onSuccess,
                   const dom::Optional<nsICameraErrorCallback* >& onError,
                   ErrorResult& aRv);
  already_AddRefed<nsICameraPreviewStateChange> GetOnPreviewStateChange() const;
  void SetOnPreviewStateChange(nsICameraPreviewStateChange* aOnStateChange);
  void GetPreviewStreamVideoMode(JSContext* cx, JS::Handle<JS::Value> aOptions, nsICameraPreviewStreamCallback* onSuccess, const dom::Optional<nsICameraErrorCallback* >& onError, ErrorResult& aRv);
  void StartRecording(JSContext* cx, JS::Handle<JS::Value> aOptions, nsDOMDeviceStorage& storageArea, const nsAString& filename, nsICameraStartRecordingCallback* onSuccess, const dom::Optional<nsICameraErrorCallback* >& onError, ErrorResult& aRv);
  void StopRecording(ErrorResult& aRv);
  void GetPreviewStream(JSContext* cx, JS::Handle<JS::Value> aOptions, nsICameraPreviewStreamCallback* onSuccess, const dom::Optional<nsICameraErrorCallback* >& onError, ErrorResult& aRv);
  void ResumePreview(ErrorResult& aRv);
  void ReleaseHardware(const dom::Optional<nsICameraReleaseCallback* >& onSuccess, const dom::Optional<nsICameraErrorCallback* >& onError, ErrorResult& aRv);

protected:
  virtual ~nsDOMCameraControl();

private:
  nsDOMCameraControl(const nsDOMCameraControl&) MOZ_DELETE;
  nsDOMCameraControl& operator=(const nsDOMCameraControl&) MOZ_DELETE;

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

protected:
  
  nsRefPtr<ICameraControl>        mCameraControl; 
  nsCOMPtr<nsICameraCapabilities> mDOMCapabilities;
  
  nsCOMPtr<nsIAudioChannelAgent>  mAudioChannelAgent;
  nsCOMPtr<nsPIDOMWindow> mWindow;
};

} 

#endif 
