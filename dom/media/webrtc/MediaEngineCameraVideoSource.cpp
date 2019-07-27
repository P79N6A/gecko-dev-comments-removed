



#include "MediaEngineCameraVideoSource.h"

#include <limits>

namespace mozilla {

using namespace mozilla::gfx;
using namespace mozilla::dom;

extern PRLogModuleInfo* GetMediaManagerLog();
#define LOG(msg) MOZ_LOG(GetMediaManagerLog(), mozilla::LogLevel::Debug, msg)
#define LOGFRAME(msg) MOZ_LOG(GetMediaManagerLog(), mozilla::LogLevel::Verbose, msg)


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
MediaEngineCameraVideoSource::FitnessDistance(ValueType aN,
                                              const ConstrainRange& aRange)
{
  if ((aRange.mExact.WasPassed() && aRange.mExact.Value() != aN) ||
      (aRange.mMin.WasPassed() && aRange.mMin.Value() > aN) ||
      (aRange.mMax.WasPassed() && aRange.mMax.Value() < aN)) {
    return UINT32_MAX;
  }
  if (!aRange.mIdeal.WasPassed() || aN == aRange.mIdeal.Value()) {
    return 0;
  }
  return uint32_t(ValueType((std::abs(aN - aRange.mIdeal.Value()) * 1000) /
                            std::max(std::abs(aN), std::abs(aRange.mIdeal.Value()))));
}



 uint32_t
MediaEngineCameraVideoSource::FitnessDistance(int32_t aN,
    const OwningLongOrConstrainLongRange& aConstraint, bool aAdvanced)
{
  if (aConstraint.IsLong()) {
    ConstrainLongRange range;
    (aAdvanced ? range.mExact : range.mIdeal).Construct(aConstraint.GetAsLong());
    return FitnessDistance(aN, range);
  } else {
    return FitnessDistance(aN, aConstraint.GetAsConstrainLongRange());
  }
}

 uint32_t
MediaEngineCameraVideoSource::FitnessDistance(double aN,
    const OwningDoubleOrConstrainDoubleRange& aConstraint,
    bool aAdvanced)
{
  if (aConstraint.IsDouble()) {
    ConstrainDoubleRange range;
    (aAdvanced ? range.mExact : range.mIdeal).Construct(aConstraint.GetAsDouble());
    return FitnessDistance(aN, range);
  } else {
    return FitnessDistance(aN, aConstraint.GetAsConstrainDoubleRange());
  }
}



 uint32_t
MediaEngineCameraVideoSource::FitnessDistance(nsString aN,
                             const ConstrainDOMStringParameters& aParams)
{
  struct Func
  {
    static bool
    Contains(const OwningStringOrStringSequence& aStrings, nsString aN)
    {
      return aStrings.IsString() ? aStrings.GetAsString() == aN
                                 : aStrings.GetAsStringSequence().Contains(aN);
    }
  };

  if (aParams.mExact.WasPassed() && !Func::Contains(aParams.mExact.Value(), aN)) {
    return UINT32_MAX;
  }
  if (aParams.mIdeal.WasPassed() && !Func::Contains(aParams.mIdeal.Value(), aN)) {
    return 1000;
  }
  return 0;
}

 uint32_t
MediaEngineCameraVideoSource::FitnessDistance(nsString aN,
    const OwningStringOrStringSequenceOrConstrainDOMStringParameters& aConstraint,
    bool aAdvanced)
{
  if (aConstraint.IsString()) {
    ConstrainDOMStringParameters params;
    if (aAdvanced) {
      params.mExact.Construct();
      params.mExact.Value().SetAsString() = aConstraint.GetAsString();
    } else {
      params.mIdeal.Construct();
      params.mIdeal.Value().SetAsString() = aConstraint.GetAsString();
    }
    return FitnessDistance(aN, params);
  } else if (aConstraint.IsStringSequence()) {
    ConstrainDOMStringParameters params;
    if (aAdvanced) {
      params.mExact.Construct();
      params.mExact.Value().SetAsStringSequence() = aConstraint.GetAsStringSequence();
    } else {
      params.mIdeal.Construct();
      params.mIdeal.Value().SetAsStringSequence() = aConstraint.GetAsStringSequence();
    }
    return FitnessDistance(aN, params);
  } else {
    return FitnessDistance(aN, aConstraint.GetAsConstrainDOMStringParameters());
  }
}

uint32_t
MediaEngineCameraVideoSource::GetFitnessDistance(const webrtc::CaptureCapability& aCandidate,
                                                 const MediaTrackConstraintSet &aConstraints,
                                                 bool aAdvanced)
{
  
  

  uint64_t distance =
    uint64_t(FitnessDistance(mFacingMode, aConstraints.mFacingMode, aAdvanced)) +
    uint64_t(aCandidate.width? FitnessDistance(int32_t(aCandidate.width),
                                               aConstraints.mWidth,
                                               aAdvanced) : 0) +
    uint64_t(aCandidate.height? FitnessDistance(int32_t(aCandidate.height),
                                                aConstraints.mHeight,
                                                aAdvanced) : 0) +
    uint64_t(aCandidate.maxFPS? FitnessDistance(double(aCandidate.maxFPS),
                                                aConstraints.mFrameRate,
                                                aAdvanced) : 0);
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
      uint32_t distance = GetFitnessDistance(cap, *cs, !first);
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
}

bool
MediaEngineCameraVideoSource::ChooseCapability(
    const MediaTrackConstraints &aConstraints,
    const MediaEnginePrefs &aPrefs)
{
  if (MOZ_LOG_TEST(GetMediaManagerLog(), LogLevel::Debug)) {
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
  }

  size_t num = NumCapabilities();

  CapabilitySet candidateSet;
  for (size_t i = 0; i < num; i++) {
    candidateSet.AppendElement(i);
  }

  

  for (size_t i = 0; i < candidateSet.Length();) {
    auto& candidate = candidateSet[i];
    webrtc::CaptureCapability cap;
    GetCapability(candidate.mIndex, cap);
    candidate.mDistance = GetFitnessDistance(cap, aConstraints, false);
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
        if (GetFitnessDistance(cap, cs, true) == UINT32_MAX) {
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
      candidate.mDistance = GetFitnessDistance(cap, prefs, false);
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
MediaEngineCameraVideoSource::SetName(nsString aName)
{
  mDeviceName = aName;
  bool hasFacingMode = false;
  VideoFacingModeEnum facingMode = VideoFacingModeEnum::User;

  
#if defined(MOZ_B2G_CAMERA) && defined(MOZ_WIDGET_GONK)
  if (aName.EqualsLiteral("back")) {
    hasFacingMode = true;
    facingMode = VideoFacingModeEnum::Environment;
  } else if (aName.EqualsLiteral("front")) {
    hasFacingMode = true;
    facingMode = VideoFacingModeEnum::User;
  }
#endif 
#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
  
  
  
  

  if (aName.Find(NS_LITERAL_STRING("Facing back")) != kNotFound) {
    hasFacingMode = true;
    facingMode = VideoFacingModeEnum::Environment;
  } else if (aName.Find(NS_LITERAL_STRING("Facing front")) != kNotFound) {
    hasFacingMode = true;
    facingMode = VideoFacingModeEnum::User;
  }
#endif 
#ifdef XP_MACOSX
  
  if (aName.Find(NS_LITERAL_STRING("Face")) != -1) {
    hasFacingMode = true;
    facingMode = VideoFacingModeEnum::User;
  }
#endif
  if (hasFacingMode) {
    mFacingMode.Assign(NS_ConvertUTF8toUTF16(
        VideoFacingModeEnumValues::strings[uint32_t(facingMode)].value));
  } else {
    mFacingMode.Truncate();
  }
}

void
MediaEngineCameraVideoSource::GetName(nsAString& aName)
{
  aName = mDeviceName;
}

void
MediaEngineCameraVideoSource::SetUUID(const char* aUUID)
{
  mUniqueId.Assign(aUUID);
}

void
MediaEngineCameraVideoSource::GetUUID(nsACString& aUUID)
{
  aUUID = mUniqueId;
}

const nsCString&
MediaEngineCameraVideoSource::GetUUID()
{
  return mUniqueId;
}


void
MediaEngineCameraVideoSource::SetDirectListeners(bool aHasDirectListeners)
{
  LOG((__FUNCTION__));
  mHasDirectListeners = aHasDirectListeners;
}

} 
