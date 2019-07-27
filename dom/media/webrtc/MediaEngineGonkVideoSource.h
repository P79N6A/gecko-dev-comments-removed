



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
#include "mozilla/layers/TextureClientRecycleAllocator.h"
#include "GonkCameraSource.h"

namespace android {
class MOZ_EXPORT MediaBuffer;
}

namespace mozilla {



















class MediaEngineGonkVideoSource : public MediaEngineCameraVideoSource
                                 , public mozilla::hal::ScreenConfigurationObserver
                                 , public CameraControlListener
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  MediaEngineGonkVideoSource(int aIndex)
    : MediaEngineCameraVideoSource(aIndex, "GonkCamera.Monitor")
    , mCallbackMonitor("GonkCamera.CallbackMonitor")
    , mCameraControl(nullptr)
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
                          StreamTime aDesiredTime) MOZ_OVERRIDE;

  void OnHardwareStateChange(HardwareState aState, nsresult aReason) MOZ_OVERRIDE;
  void GetRotation();
  bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight) MOZ_OVERRIDE;
  void OnUserError(UserContext aContext, nsresult aError) MOZ_OVERRIDE;
  void OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType) MOZ_OVERRIDE;

  void AllocImpl();
  void DeallocImpl();
  void StartImpl(webrtc::CaptureCapability aCapability);
  void StopImpl();
  uint32_t ConvertPixelFormatToFOURCC(int aFormat);
  void RotateImage(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void Notify(const mozilla::hal::ScreenConfiguration& aConfiguration);

  nsresult TakePhoto(PhotoCallback* aCallback) MOZ_OVERRIDE;

  
  
  nsresult UpdatePhotoOrientation();

  
  
  
  
  nsresult OnNewMediaBufferFrame(android::MediaBuffer* aBuffer);

protected:
  ~MediaEngineGonkVideoSource()
  {
    Shutdown();
  }
  
  void Init();
  void Shutdown();
  size_t NumCapabilities() MOZ_OVERRIDE;
  
  
  nsresult InitDirectMediaBuffer();

  mozilla::ReentrantMonitor mCallbackMonitor; 
  
  nsRefPtr<ICameraControl> mCameraControl;
  nsCOMPtr<nsIDOMFile> mLastCapture;

  android::sp<android::GonkCameraSource> mCameraSource;

  
  nsTArray<nsRefPtr<PhotoCallback>> mPhotoCallbacks;
  int mRotation;
  int mCameraAngle; 
  bool mBackCamera;
  bool mOrientationChanged; 

  RefPtr<layers::TextureClientRecycleAllocator> mTextureClientAllocator;
};

} 

#endif 
