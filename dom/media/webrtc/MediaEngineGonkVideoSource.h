



#ifndef MediaEngineGonkVideoSource_h_
#define MediaEngineGonkVideoSource_h_

#ifndef MOZ_B2G_CAMERA
#error MediaEngineGonkVideoSource is only available when MOZ_B2G_CAMERA is defined.
#endif

#include "CameraControlListener.h"
#include "MediaEngineCameraVideoSource.h"

#include "mozilla/Hal.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/dom/File.h"

namespace mozilla {



















class MediaEngineGonkVideoSource : public MediaEngineCameraVideoSource
                                 , public mozilla::hal::ScreenConfigurationObserver
                                 , public CameraControlListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  MediaEngineGonkVideoSource(int aIndex)
    : MediaEngineCameraVideoSource(aIndex, "GonkCamera.Monitor")
    , mCameraControl(nullptr)
    , mCallbackMonitor("GonkCamera.CallbackMonitor")
    , mRotation(0)
    , mBackCamera(false)
    , mOrientationChanged(true) 
    {
      Init();
    }

  virtual nsresult Allocate(const VideoTrackConstraintsN &aConstraints,
                            const MediaEnginePrefs &aPrefs) MOZ_OVERRIDE;
  virtual nsresult Deallocate() MOZ_OVERRIDE;
  virtual nsresult Start(SourceMediaStream* aStream, TrackID aID) MOZ_OVERRIDE;
  virtual nsresult Stop(SourceMediaStream* aSource, TrackID aID) MOZ_OVERRIDE;
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream* aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          StreamTime &aLastEndTime) MOZ_OVERRIDE;
  virtual bool SatisfiesConstraintSets(
      const nsTArray<const dom::MediaTrackConstraintSet*>& aConstraintSets)
  {
    return true;
  }

  void OnHardwareStateChange(HardwareState aState);
  void GetRotation();
  bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void OnUserError(UserContext aContext, nsresult aError);
  void OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType);

  void AllocImpl();
  void DeallocImpl();
  void StartImpl(webrtc::CaptureCapability aCapability);
  void StopImpl();
  uint32_t ConvertPixelFormatToFOURCC(int aFormat);
  void RotateImage(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void Notify(const mozilla::hal::ScreenConfiguration& aConfiguration);

  nsresult TakePhoto(PhotoCallback* aCallback) MOZ_OVERRIDE;

  
  
  nsresult UpdatePhotoOrientation();

protected:
  ~MediaEngineGonkVideoSource()
  {
    Shutdown();
  }
  
  void Init();
  void Shutdown();
  void ChooseCapability(const VideoTrackConstraintsN& aConstraints,
                        const MediaEnginePrefs& aPrefs);

  mozilla::ReentrantMonitor mCallbackMonitor; 
  
  nsRefPtr<ICameraControl> mCameraControl;
  nsCOMPtr<nsIDOMFile> mLastCapture;

  
  nsTArray<nsRefPtr<PhotoCallback>> mPhotoCallbacks;
  int mRotation;
  int mCameraAngle; 
  bool mBackCamera;
  bool mOrientationChanged; 
};

} 

#endif 
