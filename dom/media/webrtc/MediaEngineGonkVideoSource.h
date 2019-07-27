



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

  virtual nsresult Allocate(const dom::MediaTrackConstraints &aConstraints,
                            const MediaEnginePrefs &aPrefs) override;
  virtual nsresult Deallocate() override;
  virtual nsresult Start(SourceMediaStream* aStream, TrackID aID) override;
  virtual nsresult Stop(SourceMediaStream* aSource, TrackID aID) override;
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream* aSource,
                          TrackID aId,
                          StreamTime aDesiredTime) override;

  void OnHardwareStateChange(HardwareState aState, nsresult aReason) override;
  void GetRotation();
  bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight) override;
  void OnUserError(UserContext aContext, nsresult aError) override;
  void OnTakePictureComplete(const uint8_t* aData, uint32_t aLength, const nsAString& aMimeType) override;

  void AllocImpl();
  void DeallocImpl();
  void StartImpl(webrtc::CaptureCapability aCapability);
  void StopImpl();
  uint32_t ConvertPixelFormatToFOURCC(int aFormat);
  void RotateImage(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void Notify(const mozilla::hal::ScreenConfiguration& aConfiguration);

  nsresult TakePhoto(PhotoCallback* aCallback) override;

  
  
  nsresult UpdatePhotoOrientation();

  
  
  
  
  nsresult OnNewMediaBufferFrame(android::MediaBuffer* aBuffer);

protected:
  ~MediaEngineGonkVideoSource()
  {
    Shutdown();
  }
  
  void Init();
  void Shutdown();
  size_t NumCapabilities() override;
  
  
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
