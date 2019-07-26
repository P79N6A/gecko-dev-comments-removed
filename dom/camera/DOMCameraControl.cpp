



#include "base/basictypes.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "jsapi.h"
#include "nsThread.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "nsIObserverService.h"
#include "nsIDOMDeviceStorage.h"
#include "nsXULAppAPI.h"
#include "DOMCameraManager.h"
#include "DOMCameraCapabilities.h"
#include "DOMCameraControl.h"
#include "CameraCommon.h"
#include "mozilla/dom/CameraManagerBinding.h"
#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

DOMCI_DATA(CameraControl, nsICameraControl)

NS_IMPL_CYCLE_COLLECTION_1(nsDOMCameraControl,
                           mDOMCapabilities)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMCameraControl)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsICameraControl)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CameraControl)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCameraControl)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCameraControl)

nsDOMCameraControl::~nsDOMCameraControl()
{
  DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
}


NS_IMETHODIMP
nsDOMCameraControl::GetCapabilities(nsICameraCapabilities** aCapabilities)
{
  if (!mDOMCapabilities) {
    mDOMCapabilities = new DOMCameraCapabilities(mCameraControl);
  }

  nsCOMPtr<nsICameraCapabilities> capabilities = mDOMCapabilities;
  capabilities.forget(aCapabilities);
  return NS_OK;
}


NS_IMETHODIMP
nsDOMCameraControl::GetEffect(nsAString& aEffect)
{
  return mCameraControl->Get(CAMERA_PARAM_EFFECT, aEffect);
}
NS_IMETHODIMP
nsDOMCameraControl::SetEffect(const nsAString& aEffect)
{
  return mCameraControl->Set(CAMERA_PARAM_EFFECT, aEffect);
}


NS_IMETHODIMP
nsDOMCameraControl::GetWhiteBalanceMode(nsAString& aWhiteBalanceMode)
{
  return mCameraControl->Get(CAMERA_PARAM_WHITEBALANCE, aWhiteBalanceMode);
}
NS_IMETHODIMP
nsDOMCameraControl::SetWhiteBalanceMode(const nsAString& aWhiteBalanceMode)
{
  return mCameraControl->Set(CAMERA_PARAM_WHITEBALANCE, aWhiteBalanceMode);
}


NS_IMETHODIMP
nsDOMCameraControl::GetSceneMode(nsAString& aSceneMode)
{
  return mCameraControl->Get(CAMERA_PARAM_SCENEMODE, aSceneMode);
}
NS_IMETHODIMP
nsDOMCameraControl::SetSceneMode(const nsAString& aSceneMode)
{
  return mCameraControl->Set(CAMERA_PARAM_SCENEMODE, aSceneMode);
}


NS_IMETHODIMP
nsDOMCameraControl::GetFlashMode(nsAString& aFlashMode)
{
  return mCameraControl->Get(CAMERA_PARAM_FLASHMODE, aFlashMode);
}
NS_IMETHODIMP
nsDOMCameraControl::SetFlashMode(const nsAString& aFlashMode)
{
  return mCameraControl->Set(CAMERA_PARAM_FLASHMODE, aFlashMode);
}


NS_IMETHODIMP
nsDOMCameraControl::GetFocusMode(nsAString& aFocusMode)
{
  return mCameraControl->Get(CAMERA_PARAM_FOCUSMODE, aFocusMode);
}
NS_IMETHODIMP
nsDOMCameraControl::SetFocusMode(const nsAString& aFocusMode)
{
  return mCameraControl->Set(CAMERA_PARAM_FOCUSMODE, aFocusMode);
}


NS_IMETHODIMP
nsDOMCameraControl::GetZoom(double* aZoom)
{
  return mCameraControl->Get(CAMERA_PARAM_ZOOM, aZoom);
}
NS_IMETHODIMP
nsDOMCameraControl::SetZoom(double aZoom)
{
  return mCameraControl->Set(CAMERA_PARAM_ZOOM, aZoom);
}


NS_IMETHODIMP
nsDOMCameraControl::GetMeteringAreas(JSContext* cx, JS::Value* aMeteringAreas)
{
  return mCameraControl->Get(cx, CAMERA_PARAM_METERINGAREAS, aMeteringAreas);
}
NS_IMETHODIMP
nsDOMCameraControl::SetMeteringAreas(JSContext* cx, const JS::Value& aMeteringAreas)
{
  return mCameraControl->SetMeteringAreas(cx, aMeteringAreas);
}


NS_IMETHODIMP
nsDOMCameraControl::GetFocusAreas(JSContext* cx, JS::Value* aFocusAreas)
{
  return mCameraControl->Get(cx, CAMERA_PARAM_FOCUSAREAS, aFocusAreas);
}
NS_IMETHODIMP
nsDOMCameraControl::SetFocusAreas(JSContext* cx, const JS::Value& aFocusAreas)
{
  return mCameraControl->SetFocusAreas(cx, aFocusAreas);
}


NS_IMETHODIMP
nsDOMCameraControl::GetFocalLength(double* aFocalLength)
{
  return mCameraControl->Get(CAMERA_PARAM_FOCALLENGTH, aFocalLength);
}


NS_IMETHODIMP
nsDOMCameraControl::GetFocusDistanceNear(double* aFocusDistanceNear)
{
  return mCameraControl->Get(CAMERA_PARAM_FOCUSDISTANCENEAR, aFocusDistanceNear);
}


NS_IMETHODIMP
nsDOMCameraControl::GetFocusDistanceOptimum(double* aFocusDistanceOptimum)
{
  return mCameraControl->Get(CAMERA_PARAM_FOCUSDISTANCEOPTIMUM, aFocusDistanceOptimum);
}


NS_IMETHODIMP
nsDOMCameraControl::GetFocusDistanceFar(double* aFocusDistanceFar)
{
  return mCameraControl->Get(CAMERA_PARAM_FOCUSDISTANCEFAR, aFocusDistanceFar);
}


NS_IMETHODIMP
nsDOMCameraControl::SetExposureCompensation(const JS::Value& aCompensation, JSContext* cx)
{
  if (aCompensation.isNullOrUndefined()) {
    
    return mCameraControl->Set(CAMERA_PARAM_EXPOSURECOMPENSATION, NAN);
  }

  double compensation;
  if (!JS_ValueToNumber(cx, aCompensation, &compensation)) {
    return NS_ERROR_INVALID_ARG;
  }

  return mCameraControl->Set(CAMERA_PARAM_EXPOSURECOMPENSATION, compensation);
}


NS_IMETHODIMP
nsDOMCameraControl::GetExposureCompensation(double* aExposureCompensation)
{
  return mCameraControl->Get(CAMERA_PARAM_EXPOSURECOMPENSATION, aExposureCompensation);
}


NS_IMETHODIMP
nsDOMCameraControl::GetOnShutter(nsICameraShutterCallback** aOnShutter)
{
  return mCameraControl->Get(aOnShutter);
}
NS_IMETHODIMP
nsDOMCameraControl::SetOnShutter(nsICameraShutterCallback* aOnShutter)
{
  return mCameraControl->Set(aOnShutter);
}


NS_IMETHODIMP
nsDOMCameraControl::GetOnClosed(nsICameraClosedCallback** aOnClosed)
{
  return mCameraControl->Get(aOnClosed);
}
NS_IMETHODIMP
nsDOMCameraControl::SetOnClosed(nsICameraClosedCallback* aOnClosed)
{
  return mCameraControl->Set(aOnClosed);
}


NS_IMETHODIMP
nsDOMCameraControl::GetOnRecorderStateChange(nsICameraRecorderStateChange** aOnRecorderStateChange)
{
  return mCameraControl->Get(aOnRecorderStateChange);
}
NS_IMETHODIMP
nsDOMCameraControl::SetOnRecorderStateChange(nsICameraRecorderStateChange* aOnRecorderStateChange)
{
  return mCameraControl->Set(aOnRecorderStateChange);
}


NS_IMETHODIMP
nsDOMCameraControl::StartRecording(const JS::Value& aOptions, nsIDOMDeviceStorage* storageArea, const nsAString& filename, nsICameraStartRecordingCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(storageArea, NS_ERROR_INVALID_ARG);

  mozilla::idl::CameraStartRecordingOptions options;

  
  options.rotation = 0;
  options.maxFileSizeBytes = 0;
  options.maxVideoLengthMs = 0;
  nsresult rv = options.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs) {
    NS_WARNING("Could not get the Observer service for CameraControl::StartRecording.");
    return NS_ERROR_FAILURE;
  }

  obs->NotifyObservers(nullptr,
                       "recording-device-events",
                       NS_LITERAL_STRING("starting").get());
  
  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    unused << ContentChild::GetSingleton()->SendRecordingDeviceEvents(NS_LITERAL_STRING("starting"));
  }

  #ifdef MOZ_B2G
  if (!mAudioChannelAgent) {
    mAudioChannelAgent = do_CreateInstance("@mozilla.org/audiochannelagent;1");
    if (mAudioChannelAgent) {
      
      mAudioChannelAgent->Init(AUDIO_CHANNEL_CONTENT, nullptr);
      
      bool canPlay;
      mAudioChannelAgent->StartPlaying(&canPlay);
    }
  }
  #endif

  nsCOMPtr<nsIFile> folder;
  rv = storageArea->GetRootDirectoryForFile(filename, getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv, rv);
  return mCameraControl->StartRecording(&options, folder, filename, onSuccess, onError);
}


NS_IMETHODIMP
nsDOMCameraControl::StopRecording()
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs) {
    NS_WARNING("Could not get the Observer service for CameraControl::StopRecording.");
    return NS_ERROR_FAILURE;
  }

  obs->NotifyObservers(nullptr,
                       "recording-device-events",
                       NS_LITERAL_STRING("shutdown").get());
  
  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    unused << ContentChild::GetSingleton()->SendRecordingDeviceEvents(NS_LITERAL_STRING("shutdown"));
  }

  #ifdef MOZ_B2G
  if (mAudioChannelAgent) {
    mAudioChannelAgent->StopPlaying();
    mAudioChannelAgent = nullptr;
  }
  #endif

  return mCameraControl->StopRecording();
}


NS_IMETHODIMP
nsDOMCameraControl::GetPreviewStream(const JS::Value& aOptions, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  mozilla::idl::CameraSize size;
  nsresult rv = size.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  return mCameraControl->GetPreviewStream(size, onSuccess, onError);
}


NS_IMETHODIMP
nsDOMCameraControl::ResumePreview()
{
  return mCameraControl->StartPreview(nullptr);
}


NS_IMETHODIMP
nsDOMCameraControl::AutoFocus(nsICameraAutoFocusCallback* onSuccess, nsICameraErrorCallback* onError)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);
  return mCameraControl->AutoFocus(onSuccess, onError);
}


NS_IMETHODIMP
nsDOMCameraControl::TakePicture(const JS::Value& aOptions, nsICameraTakePictureCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  RootedDictionary<CameraPictureOptions> options(cx);
  mozilla::idl::CameraSize           size;
  mozilla::idl::CameraPosition       pos;

  JS::Rooted<JS::Value> optionVal(cx, aOptions);
  if (!options.Init(cx, optionVal)) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = size.Init(cx, &options.mPictureSize);
  NS_ENSURE_SUCCESS(rv, rv);

  



  pos.latitude = NAN;
  pos.longitude = NAN;
  pos.altitude = NAN;
  pos.timestamp = NAN;
  rv = pos.Init(cx, &options.mPosition);
  NS_ENSURE_SUCCESS(rv, rv);

  return mCameraControl->TakePicture(size, options.mRotation, options.mFileFormat, pos, options.mDateTime, onSuccess, onError);
}


NS_IMETHODIMP
nsDOMCameraControl::GetPreviewStreamVideoMode(const JS::Value& aOptions, nsICameraPreviewStreamCallback* onSuccess, nsICameraErrorCallback* onError, JSContext* cx)
{
  NS_ENSURE_TRUE(onSuccess, NS_ERROR_INVALID_ARG);

  mozilla::idl::CameraRecorderOptions options;
  nsresult rv = options.Init(cx, &aOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  return mCameraControl->GetPreviewStreamVideoMode(&options, onSuccess, onError);
}

NS_IMETHODIMP
nsDOMCameraControl::ReleaseHardware(nsICameraReleaseCallback* onSuccess, nsICameraErrorCallback* onError)
{
  return mCameraControl->ReleaseHardware(onSuccess, onError);
}

class GetCameraResult : public nsRunnable
{
public:
  GetCameraResult(nsDOMCameraControl* aDOMCameraControl,
    nsresult aResult,
    const nsMainThreadPtrHandle<nsICameraGetCameraCallback>& onSuccess,
    const nsMainThreadPtrHandle<nsICameraErrorCallback>& onError,
    uint64_t aWindowId)
    : mDOMCameraControl(aDOMCameraControl)
    , mResult(aResult)
    , mOnSuccessCb(onSuccess)
    , mOnErrorCb(onError)
    , mWindowId(aWindowId)
  { }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (nsDOMCameraManager::IsWindowStillActive(mWindowId)) {
      DOM_CAMERA_LOGT("%s : this=%p -- BEFORE CALLBACK\n", __func__, this);
      if (NS_FAILED(mResult)) {
        if (mOnErrorCb.get()) {
          mOnErrorCb->HandleEvent(NS_LITERAL_STRING("FAILURE"));
        }
      } else {
        if (mOnSuccessCb.get()) {
          mOnSuccessCb->HandleEvent(mDOMCameraControl);
        }
      }
      DOM_CAMERA_LOGT("%s : this=%p -- AFTER CALLBACK\n", __func__, this);
    }

    




    NS_RELEASE(mDOMCameraControl);
    return NS_OK;
  }

protected:
  



  nsDOMCameraControl* mDOMCameraControl;
  nsresult mResult;
  nsMainThreadPtrHandle<nsICameraGetCameraCallback> mOnSuccessCb;
  nsMainThreadPtrHandle<nsICameraErrorCallback> mOnErrorCb;
  uint64_t mWindowId;
};

nsresult
nsDOMCameraControl::Result(nsresult aResult,
                           const nsMainThreadPtrHandle<nsICameraGetCameraCallback>& onSuccess,
                           const nsMainThreadPtrHandle<nsICameraErrorCallback>& onError,
                           uint64_t aWindowId)
{
  nsCOMPtr<GetCameraResult> getCameraResult = new GetCameraResult(this, aResult, onSuccess, onError, aWindowId);
  return NS_DispatchToMainThread(getCameraResult);
}

void
nsDOMCameraControl::Shutdown()
{
  DOM_CAMERA_LOGI("%s:%d\n", __func__, __LINE__);
  mCameraControl->Shutdown();
}

nsRefPtr<ICameraControl>
nsDOMCameraControl::GetNativeCameraControl()
{
  return mCameraControl;
}
