



#include "MediaEngineCameraVideoSource.h"

#include <limits>

namespace mozilla {

using namespace mozilla::gfx;
using dom::OwningLongOrConstrainLongRange;
using dom::ConstrainLongRange;
using dom::OwningDoubleOrConstrainDoubleRange;
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



size_t
MediaEngineCameraVideoSource::NumCapabilities()
{
  return mHardcodedCapabilities.Length();
}

void
MediaEngineCameraVideoSource::GetCapability(size_t aIndex,
                                            webrtc::CaptureCapability& aOut)
{
  MOZ_ASSERT(aIndex < mHardcodedCapabilities.Length());
  aOut = mHardcodedCapabilities[aIndex];
}






template<class ValueType, class ConstrainRange>
 uint32_t
MediaEngineCameraVideoSource::FitnessDistance(ValueType n,
                                              const ConstrainRange& aRange)
{
  if ((aRange.mExact.WasPassed() && aRange.mExact.Value() != n) ||
      (aRange.mMin.WasPassed() && aRange.mMin.Value() > n) ||
      (aRange.mMax.WasPassed() && aRange.mMax.Value() < n)) {
    return UINT32_MAX;
  }
  if (!aRange.mIdeal.WasPassed() || n == aRange.mIdeal.Value()) {
    return 0;
  }
  return uint32_t(ValueType((std::abs(n - aRange.mIdeal.Value()) * 1000) /
                            std::max(std::abs(n), std::abs(aRange.mIdeal.Value()))));
}



template<>
 uint32_t
MediaEngineCameraVideoSource::FitnessDistance(int32_t n,
    const OwningLongOrConstrainLongRange& aConstraint)
{
  if (aConstraint.IsLong()) {
    ConstrainLongRange range;
    range.mIdeal.Construct(aConstraint.GetAsLong());
    return FitnessDistance(n, range);
  } else {
    return FitnessDistance(n, aConstraint.GetAsConstrainLongRange());
  }
}

template<>
 uint32_t
MediaEngineCameraVideoSource::FitnessDistance(double n,
    const OwningDoubleOrConstrainDoubleRange& aConstraint)
{
  if (aConstraint.IsDouble()) {
    ConstrainDoubleRange range;
    range.mIdeal.Construct(aConstraint.GetAsDouble());
    return FitnessDistance(n, range);
  } else {
    return FitnessDistance(n, aConstraint.GetAsConstrainDoubleRange());
  }
}

 uint32_t
MediaEngineCameraVideoSource::GetFitnessDistance(const webrtc::CaptureCapability& aCandidate,
                                                 const MediaTrackConstraintSet &aConstraints)
{
  
  

  uint64_t distance =
    uint64_t(aCandidate.width? FitnessDistance(int32_t(aCandidate.width),
                                               aConstraints.mWidth) : 0) +
    uint64_t(aCandidate.height? FitnessDistance(int32_t(aCandidate.height),
                                                aConstraints.mHeight) : 0) +
    uint64_t(aCandidate.maxFPS? FitnessDistance(double(aCandidate.maxFPS),
                                                aConstraints.mFrameRate) : 0);
  return uint32_t(std::min(distance, uint64_t(UINT32_MAX)));
}



 void
MediaEngineCameraVideoSource::TrimLessFitCandidates(CapabilitySet& set) {
  uint32_t best = UINT32_MAX;
  for (auto& candidate : set) {
    if (best > candidate.mDistance) {
      best = candidate.mDistance;
    }
  }
  for (size_t i = 0; i < set.Length();) {
    if (set[i].mDistance > best) {
      set.RemoveElementAt(i);
    } else {
      ++i;
    }
  }
  MOZ_ASSERT(set.Length());
}







uint32_t
MediaEngineCameraVideoSource::GetBestFitnessDistance(
    const nsTArray<const MediaTrackConstraintSet*>& aConstraintSets)
{
  size_t num = NumCapabilities();

  CapabilitySet candidateSet;
  for (size_t i = 0; i < num; i++) {
    candidateSet.AppendElement(i);
  }

  bool first = true;
  for (const MediaTrackConstraintSet* cs : aConstraintSets) {
    for (size_t i = 0; i < candidateSet.Length();  ) {
      auto& candidate = candidateSet[i];
      webrtc::CaptureCapability cap;
      GetCapability(candidate.mIndex, cap);
      uint32_t distance = GetFitnessDistance(cap, *cs);
      if (distance == UINT32_MAX) {
        candidateSet.RemoveElementAt(i);
      } else {
        ++i;
        if (first) {
          candidate.mDistance = distance;
        }
      }
    }
    first = false;
  }
  if (!candidateSet.Length()) {
    return UINT32_MAX;
  }
  TrimLessFitCandidates(candidateSet);
  return candidateSet[0].mDistance;
}

void
MediaEngineCameraVideoSource::LogConstraints(
    const MediaTrackConstraintSet& aConstraints, bool aAdvanced)
{
#ifdef PR_LOGGING
  NormalizedConstraintSet c(aConstraints, aAdvanced);
  LOG(((c.mWidth.mIdeal.WasPassed()?
        "Constraints: width: { min: %d, max: %d, ideal: %d }" :
        "Constraints: width: { min: %d, max: %d }"),
       c.mWidth.mMin, c.mWidth.mMax,
       c.mWidth.mIdeal.WasPassed()? c.mWidth.mIdeal.Value() : 0));
  LOG(((c.mHeight.mIdeal.WasPassed()?
        "             height: { min: %d, max: %d, ideal: %d }" :
        "             height: { min: %d, max: %d }"),
       c.mHeight.mMin, c.mHeight.mMax,
       c.mHeight.mIdeal.WasPassed()? c.mHeight.mIdeal.Value() : 0));
  LOG(((c.mFrameRate.mIdeal.WasPassed()?
        "             frameRate: { min: %f, max: %f, ideal: %f }" :
        "             frameRate: { min: %f, max: %f }"),
       c.mFrameRate.mMin, c.mFrameRate.mMax,
       c.mFrameRate.mIdeal.WasPassed()? c.mFrameRate.mIdeal.Value() : 0));
#endif
}

bool
MediaEngineCameraVideoSource::ChooseCapability(
    const dom::MediaTrackConstraints &aConstraints,
    const MediaEnginePrefs &aPrefs)
{
#ifdef PR_LOGGING
  LOG(("ChooseCapability: prefs: %dx%d @%d-%dfps",
       aPrefs.GetWidth(), aPrefs.GetHeight(),
       aPrefs.mFPS, aPrefs.mMinFPS));
  LogConstraints(aConstraints, false);
  if (aConstraints.mAdvanced.WasPassed()) {
    LOG(("Advanced array[%u]:", aConstraints.mAdvanced.Value().Length()));
    for (auto& advanced : aConstraints.mAdvanced.Value()) {
      LogConstraints(advanced, true);
    }
  }
#endif

  size_t num = NumCapabilities();

  CapabilitySet candidateSet;
  for (size_t i = 0; i < num; i++) {
    candidateSet.AppendElement(i);
  }

  

  for (size_t i = 0; i < candidateSet.Length();) {
    auto& candidate = candidateSet[i];
    webrtc::CaptureCapability cap;
    GetCapability(candidate.mIndex, cap);
    candidate.mDistance = GetFitnessDistance(cap, aConstraints);
    if (candidate.mDistance == UINT32_MAX) {
      candidateSet.RemoveElementAt(i);
    } else {
      ++i;
    }
  }

  

  if (aConstraints.mAdvanced.WasPassed()) {
    for (const MediaTrackConstraintSet &cs : aConstraints.mAdvanced.Value()) {
      CapabilitySet rejects;
      for (size_t i = 0; i < candidateSet.Length();) {
        auto& candidate = candidateSet[i];
        webrtc::CaptureCapability cap;
        GetCapability(candidate.mIndex, cap);
        if (GetFitnessDistance(cap, cs) == UINT32_MAX) {
          rejects.AppendElement(candidate);
          candidateSet.RemoveElementAt(i);
        } else {
          ++i;
        }
      }
      if (!candidateSet.Length()) {
        candidateSet.MoveElementsFrom(rejects);
      }
    }
  }
  if (!candidateSet.Length()) {
    LOG(("failed to find capability match from %d choices",num));
    return false;
  }

  

  TrimLessFitCandidates(candidateSet);

  
  
  {
    MediaTrackConstraintSet prefs;
    prefs.mWidth.SetAsLong() = aPrefs.GetWidth();
    prefs.mHeight.SetAsLong() = aPrefs.GetHeight();
    prefs.mFrameRate.SetAsDouble() = aPrefs.mFPS;

    for (auto& candidate : candidateSet) {
      webrtc::CaptureCapability cap;
      GetCapability(candidate.mIndex, cap);
      candidate.mDistance = GetFitnessDistance(cap, prefs);
    }
    TrimLessFitCandidates(candidateSet);
  }

  
  
  

  bool found = false;
  for (auto& candidate : candidateSet) {
    webrtc::CaptureCapability cap;
    GetCapability(candidate.mIndex, cap);
    if (cap.rawType == webrtc::RawVideoType::kVideoI420 ||
        cap.rawType == webrtc::RawVideoType::kVideoYUY2 ||
        cap.rawType == webrtc::RawVideoType::kVideoYV12) {
      mCapability = cap;
      found = true;
      break;
    }
  }
  if (!found) {
    GetCapability(candidateSet[0].mIndex, mCapability);
  }

  LOG(("chose cap %dx%d @%dfps codec %d raw %d",
       mCapability.width, mCapability.height, mCapability.maxFPS,
       mCapability.codecType, mCapability.rawType));
  return true;
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
