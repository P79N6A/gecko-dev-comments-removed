



#ifndef DOM_CAMERA_DOMCAMERACONTROL_H
#define DOM_CAMERA_DOMCAMERACONTROL_H

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/CameraControlBinding.h"
#include "mozilla/dom/Promise.h"
#include "ICameraControl.h"
#include "CameraCommon.h"
#include "DOMMediaStream.h"
#include "AudioChannelAgent.h"
#include "nsProxyRelease.h"
#include "nsHashPropertyBag.h"
#include "DeviceStorage.h"
#include "DOMCameraControlListener.h"
#include "nsWeakReference.h"
#ifdef MOZ_WIDGET_GONK
#include "nsITimer.h"
#endif

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

#define NS_DOM_CAMERA_CONTROL_CID \
{ 0x3700c096, 0xf920, 0x438d, \
  { 0x8b, 0x3f, 0x15, 0xb3, 0xc9, 0x96, 0x23, 0x62 } }


class nsDOMCameraControl final : public DOMMediaStream
                               , public nsSupportsWeakReference
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DOM_CAMERA_CONTROL_CID)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMCameraControl, DOMMediaStream)
  NS_DECL_ISUPPORTS_INHERITED

  
  
  
  
  
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  nsDOMCameraControl(uint32_t aCameraId,
                     const dom::CameraConfiguration& aInitialConfig,
                     dom::Promise* aPromise,
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
  void GetMeteringMode(nsString& aMode, ErrorResult& aRv);
  void SetMeteringMode(const nsAString& aMode, ErrorResult& aRv);

  
  already_AddRefed<dom::Promise> SetConfiguration(const dom::CameraConfiguration& aConfiguration,
                                                  ErrorResult& aRv);
  void GetMeteringAreas(nsTArray<dom::CameraRegion>& aAreas, ErrorResult& aRv);
  void SetMeteringAreas(const dom::Optional<dom::Sequence<dom::CameraRegion> >& aAreas, ErrorResult& aRv);
  void GetFocusAreas(nsTArray<dom::CameraRegion>& aAreas, ErrorResult& aRv);
  void SetFocusAreas(const dom::Optional<dom::Sequence<dom::CameraRegion> >& aAreas, ErrorResult& aRv);
  void GetPictureSize(dom::CameraSize& aSize, ErrorResult& aRv);
  void SetPictureSize(const dom::CameraSize& aSize, ErrorResult& aRv);
  void GetThumbnailSize(dom::CameraSize& aSize, ErrorResult& aRv);
  void SetThumbnailSize(const dom::CameraSize& aSize, ErrorResult& aRv);
  already_AddRefed<dom::Promise> AutoFocus(ErrorResult& aRv);
  void StartFaceDetection(ErrorResult& aRv);
  void StopFaceDetection(ErrorResult& aRv);
  already_AddRefed<dom::Promise> TakePicture(const dom::CameraPictureOptions& aOptions,
                                             ErrorResult& aRv);
  already_AddRefed<dom::Promise> StartRecording(const dom::CameraStartRecordingOptions& aOptions,
                                                nsDOMDeviceStorage& storageArea,
                                                const nsAString& filename,
                                                ErrorResult& aRv);
  void StopRecording(ErrorResult& aRv);
  void ResumePreview(ErrorResult& aRv);
  already_AddRefed<dom::Promise> ReleaseHardware(ErrorResult& aRv);
  void ResumeContinuousFocus(ErrorResult& aRv);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  operator nsISupports*() { return static_cast<DOMMediaStream*>(this); }

#ifdef MOZ_WIDGET_GONK
  static void PreinitCameraHardware();
  static void DiscardCachedCameraInstance(nsITimer* aTimer, void* aClosure);
#endif

  IMPL_EVENT_HANDLER(facesdetected)
  IMPL_EVENT_HANDLER(shutter)
  IMPL_EVENT_HANDLER(close)
  IMPL_EVENT_HANDLER(recorderstatechange)
  IMPL_EVENT_HANDLER(previewstatechange)
  IMPL_EVENT_HANDLER(focus)
  IMPL_EVENT_HANDLER(picture)
  IMPL_EVENT_HANDLER(configurationchange)

protected:
  virtual ~nsDOMCameraControl();

  class DOMCameraConfiguration final : public dom::CameraConfiguration
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

  void OnGetCameraComplete();
  void OnHardwareStateChange(DOMCameraControlListener::HardwareState aState, nsresult aReason);
  void OnPreviewStateChange(DOMCameraControlListener::PreviewState aState);
  void OnRecorderStateChange(CameraControlListener::RecorderState aState, int32_t aStatus, int32_t aTrackNum);
  void OnConfigurationChange(DOMCameraConfiguration* aConfiguration);
  void OnShutter();
  void OnUserError(CameraControlListener::UserContext aContext, nsresult aError);

  bool IsWindowStillActive();
  nsresult SelectPreviewSize(const dom::CameraSize& aRequestedPreviewSize, ICameraControl::Size& aSelectedPreviewSize);

  void ReleaseAudioChannelAgent();
  nsresult NotifyRecordingStatusChange(const nsString& aMsg);

  already_AddRefed<dom::Promise> CreatePromise(ErrorResult& aRv);
  void AbortPromise(nsRefPtr<dom::Promise>& aPromise);
  virtual void EventListenerAdded(nsIAtom* aType) override;
  void DispatchPreviewStateEvent(DOMCameraControlListener::PreviewState aState);
  void DispatchStateEvent(const nsString& aType, const nsString& aState);

  nsRefPtr<ICameraControl> mCameraControl; 

  
  nsCOMPtr<nsIAudioChannelAgent> mAudioChannelAgent;

  nsresult Set(uint32_t aKey, const dom::Optional<dom::Sequence<dom::CameraRegion> >& aValue, uint32_t aLimit);
  nsresult Get(uint32_t aKey, nsTArray<dom::CameraRegion>& aValue);

  nsRefPtr<DOMCameraConfiguration>              mCurrentConfiguration;
  nsRefPtr<dom::CameraCapabilities>             mCapabilities;

  
  nsRefPtr<dom::Promise>                        mGetCameraPromise;
  nsRefPtr<dom::Promise>                        mAutoFocusPromise;
  nsRefPtr<dom::Promise>                        mTakePicturePromise;
  nsRefPtr<dom::Promise>                        mStartRecordingPromise;
  nsRefPtr<dom::Promise>                        mReleasePromise;
  nsRefPtr<dom::Promise>                        mSetConfigurationPromise;

  
  
  
  DOMCameraControlListener* mListener;

  
  nsRefPtr<CameraPreviewMediaStream> mInput;

  
  nsCOMPtr<nsPIDOMWindow>   mWindow;

  dom::CameraStartRecordingOptions mOptions;
  nsRefPtr<DeviceStorageFileDescriptor> mDSFileDescriptor;
  DOMCameraControlListener::PreviewState mPreviewState;
  bool mRecording;
  bool mSetInitialConfig;

#ifdef MOZ_WIDGET_GONK
  
  static StaticRefPtr<ICameraControl> sCachedCameraControl;
  static nsresult sCachedCameraControlStartResult;
  static nsCOMPtr<nsITimer> sDiscardCachedCameraControlTimer;
#endif

private:
  nsDOMCameraControl(const nsDOMCameraControl&) = delete;
  nsDOMCameraControl& operator=(const nsDOMCameraControl&) = delete;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDOMCameraControl, NS_DOM_CAMERA_CONTROL_CID)

} 

#endif 
