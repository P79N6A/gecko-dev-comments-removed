



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


  virtual void GetName(nsAString& aName) override;
  virtual void GetUUID(nsAString& aUUID) override;
  virtual void SetDirectListeners(bool aHasListeners) override;
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise,
                          int32_t aPlayoutDelay) override
  {
    return NS_OK;
  };

  virtual bool IsFake() override
  {
    return false;
  }

  virtual const dom::MediaSourceEnum GetMediaSource() override {
      return dom::MediaSourceEnum::Camera;
  }

  virtual nsresult TakePhoto(PhotoCallback* aCallback) override
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  uint32_t GetBestFitnessDistance(
      const nsTArray<const dom::MediaTrackConstraintSet*>& aConstraintSets) override;

protected:
  struct CapabilityCandidate {
    explicit CapabilityCandidate(uint8_t index, uint32_t distance = 0)
    : mIndex(index), mDistance(distance) {}

    size_t mIndex;
    uint32_t mDistance;
  };
  typedef nsTArray<CapabilityCandidate> CapabilitySet;

  ~MediaEngineCameraVideoSource() {}

  
  virtual bool AppendToTrack(SourceMediaStream* aSource,
                             layers::Image* aImage,
                             TrackID aID,
                             StreamTime delta);
  template<class ValueType, class ConstrainRange>
  static uint32_t FitnessDistance(ValueType n, const ConstrainRange& aRange);
  static uint32_t GetFitnessDistance(const webrtc::CaptureCapability& aCandidate,
                                     const dom::MediaTrackConstraintSet &aConstraints);
  static void TrimLessFitCandidates(CapabilitySet& set);
  static void LogConstraints(const dom::MediaTrackConstraintSet& aConstraints,
                             bool aAdvanced);
  virtual size_t NumCapabilities();
  virtual void GetCapability(size_t aIndex, webrtc::CaptureCapability& aOut);
  bool ChooseCapability(const dom::MediaTrackConstraints &aConstraints,
                        const MediaEnginePrefs &aPrefs);

  

  
  
  
  
  

  
  Monitor mMonitor; 
  nsTArray<nsRefPtr<SourceMediaStream>> mSources; 
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
