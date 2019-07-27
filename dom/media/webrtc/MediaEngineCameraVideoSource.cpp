



#include "MediaEngineCameraVideoSource.h"

namespace mozilla {

using namespace mozilla::gfx;
using dom::ConstrainLongRange;
using dom::ConstrainDoubleRange;
using dom::MediaTrackConstraintSet;

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#define LOGFRAME(msg) PR_LOG(GetMediaManagerLog(), 6, msg)
#else
#define LOG(msg)
#define LOGFRAME(msg)
#endif

 bool
MediaEngineCameraVideoSource::IsWithin(int32_t n, const ConstrainLongRange& aRange) {
  return aRange.mMin <= n && n <= aRange.mMax;
}

 bool
MediaEngineCameraVideoSource::IsWithin(double n, const ConstrainDoubleRange& aRange) {
  return aRange.mMin <= n && n <= aRange.mMax;
}

 int32_t
MediaEngineCameraVideoSource::Clamp(int32_t n, const ConstrainLongRange& aRange) {
  return std::max(aRange.mMin, std::min(n, aRange.mMax));
}

 bool
MediaEngineCameraVideoSource::AreIntersecting(const ConstrainLongRange& aA, const ConstrainLongRange& aB) {
  return aA.mMax >= aB.mMin && aA.mMin <= aB.mMax;
}

 bool
MediaEngineCameraVideoSource::Intersect(ConstrainLongRange& aA, const ConstrainLongRange& aB) {
  MOZ_ASSERT(AreIntersecting(aA, aB));
  aA.mMin = std::max(aA.mMin, aB.mMin);
  aA.mMax = std::min(aA.mMax, aB.mMax);
  return true;
}


bool MediaEngineCameraVideoSource::AppendToTrack(SourceMediaStream* aSource,
                                                 layers::Image* aImage,
                                                 TrackID aID,
                                                 StreamTime delta)
{
  MOZ_ASSERT(aSource);

  VideoSegment segment;
  nsRefPtr<layers::Image> image = aImage;
  IntSize size(image ? mWidth : 0, image ? mHeight : 0);
  segment.AppendFrame(image.forget(), delta, size);

  
  
  
  
  return aSource->AppendToTrack(aID, &(segment));
}


void
MediaEngineCameraVideoSource::GuessCapability(
    const VideoTrackConstraintsN& aConstraints,
    const MediaEnginePrefs& aPrefs)
{
  LOG(("GuessCapability: prefs: %dx%d @%d-%dfps",
       aPrefs.mWidth, aPrefs.mHeight, aPrefs.mFPS, aPrefs.mMinFPS));

  

  ConstrainLongRange cWidth(aConstraints.mRequired.mWidth);
  ConstrainLongRange cHeight(aConstraints.mRequired.mHeight);

  if (aConstraints.mAdvanced.WasPassed()) {
    const auto& advanced = aConstraints.mAdvanced.Value();
    for (uint32_t i = 0; i < advanced.Length(); i++) {
      if (AreIntersecting(cWidth, advanced[i].mWidth) &&
          AreIntersecting(cHeight, advanced[i].mHeight)) {
        Intersect(cWidth, advanced[i].mWidth);
        Intersect(cHeight, advanced[i].mHeight);
      }
    }
  }
  
  
  
  
  

  bool macHD = ((!aPrefs.mWidth || !aPrefs.mHeight) &&
                mDeviceName.EqualsASCII("FaceTime HD Camera (Built-in)") &&
                (aPrefs.GetWidth() < cWidth.mMin ||
                 aPrefs.GetHeight() < cHeight.mMin) &&
                !(aPrefs.GetWidth(true) > cWidth.mMax ||
                  aPrefs.GetHeight(true) > cHeight.mMax));
  int prefWidth = aPrefs.GetWidth(macHD);
  int prefHeight = aPrefs.GetHeight(macHD);

  

  if (IsWithin(prefWidth, cWidth) == IsWithin(prefHeight, cHeight)) {
    
    
    
    mCapability.width = Clamp(prefWidth, cWidth);
    mCapability.height = Clamp(prefHeight, cHeight);
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (IsWithin(prefWidth, cWidth)) {
      mCapability.height = Clamp(prefHeight, cHeight);
      mCapability.width = Clamp((mCapability.height * prefWidth) /
                                prefHeight, cWidth);
    } else {
      mCapability.width = Clamp(prefWidth, cWidth);
      mCapability.height = Clamp((mCapability.width * prefHeight) /
                                 prefWidth, cHeight);
    }
  }
  mCapability.maxFPS = MediaEngine::DEFAULT_VIDEO_FPS;
  LOG(("chose cap %dx%d @%dfps",
       mCapability.width, mCapability.height, mCapability.maxFPS));
}

void
MediaEngineCameraVideoSource::GetName(nsAString& aName)
{
  aName = mDeviceName;
}

void
MediaEngineCameraVideoSource::GetUUID(nsAString& aUUID)
{
  aUUID = mUniqueId;
}

void
MediaEngineCameraVideoSource::SetDirectListeners(bool aHasDirectListeners)
{
  LOG((__FUNCTION__));
  mHasDirectListeners = aHasDirectListeners;
}

} 
