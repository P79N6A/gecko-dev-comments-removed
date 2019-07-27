



#ifndef MediaEngineCameraVideoSource_h
#define MediaEngineCameraVideoSource_h

#include "MediaEngine.h"
#include "MediaTrackConstraints.h"

#include "nsDirectoryServiceDefs.h"


#undef FF
#include "webrtc/video_engine/include/vie_capture.h"

namespace mozilla {

class MediaEngineCameraVideoSource : public MediaEngineVideoSource
{
public:
  explicit MediaEngineCameraVideoSource(int aIndex,
                                        const char* aMonitorName = "Camera.Monitor")
    : MediaEngineVideoSource(kReleased)
    , mMonitor(aMonitorName)
    , mWidth(0)
    , mHeight(0)
    , mInitDone(false)
    , mHasDirectListeners(false)
    , mCaptureIndex(aIndex)
    , mTrackID(0)
    , mFps(-1)
  {}


  virtual void GetName(nsAString& aName) MOZ_OVERRIDE;
  virtual void GetUUID(nsAString& aUUID) MOZ_OVERRIDE;
  virtual void SetDirectListeners(bool aHasListeners) MOZ_OVERRIDE;
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise,
                          int32_t aPlayoutDelay) MOZ_OVERRIDE
  {
    return NS_OK;
  };

  virtual bool IsFake() MOZ_OVERRIDE
  {
    return false;
  }

  virtual const dom::MediaSourceEnum GetMediaSource() MOZ_OVERRIDE {
      return dom::MediaSourceEnum::Camera;
  }

  virtual nsresult TakePhoto(PhotoCallback* aCallback) MOZ_OVERRIDE
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  bool SatisfiesConstraintSets(
      const nsTArray<const dom::MediaTrackConstraintSet*>& aConstraintSets) MOZ_OVERRIDE;

protected:
  typedef nsTArray<size_t> CapabilitySet;

  ~MediaEngineCameraVideoSource() {}

  
  virtual bool AppendToTrack(SourceMediaStream* aSource,
                             layers::Image* aImage,
                             TrackID aID,
                             StreamTime delta);

  static bool IsWithin(int32_t n, const dom::ConstrainLongRange& aRange);
  static bool IsWithin(double n, const dom::ConstrainDoubleRange& aRange);
  static int32_t Clamp(int32_t n, const dom::ConstrainLongRange& aRange);
  static bool AreIntersecting(const dom::ConstrainLongRange& aA,
                              const dom::ConstrainLongRange& aB);
  static bool Intersect(dom::ConstrainLongRange& aA, const dom::ConstrainLongRange& aB);
  static bool SatisfiesConstraintSet(const dom::MediaTrackConstraintSet& aConstraints,
                                     const webrtc::CaptureCapability& aCandidate);
  virtual size_t NumCapabilities();
  virtual void GetCapability(size_t aIndex, webrtc::CaptureCapability& aOut);
  void ChooseCapability(const VideoTrackConstraintsN &aConstraints,
                        const MediaEnginePrefs &aPrefs);

  

  
  
  
  
  

  
  Monitor mMonitor; 
  nsTArray<SourceMediaStream*> mSources; 
  nsRefPtr<layers::Image> mImage;
  nsRefPtr<layers::ImageContainer> mImageContainer;
  int mWidth, mHeight; 
  


  bool mInitDone;
  bool mHasDirectListeners;
  int mCaptureIndex;
  TrackID mTrackID;
  int mFps; 

  webrtc::CaptureCapability mCapability; 

  nsTArray<webrtc::CaptureCapability> mHardcodedCapabilities; 
  nsString mDeviceName;
  nsString mUniqueId;
};


} 
#endif 
