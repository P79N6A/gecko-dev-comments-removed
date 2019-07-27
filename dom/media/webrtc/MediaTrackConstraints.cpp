




#include "MediaTrackConstraints.h"

#include <limits>

namespace mozilla {

template<class ValueType>
template<class ConstrainRange>
void
NormalizedConstraintSet::Range<ValueType>::SetFrom(const ConstrainRange& aOther)
{
  if (aOther.mIdeal.WasPassed()) {
    mIdeal.Construct(aOther.mIdeal.Value());
  }
  if (aOther.mExact.WasPassed()) {
    mMin = aOther.mExact.Value();
    mMax = aOther.mExact.Value();
  } else {
    if (aOther.mMin.WasPassed()) {
      mMin = aOther.mMin.Value();
    }
    if (aOther.mMax.WasPassed()) {
      mMax = aOther.mMax.Value();
    }
  }
}

NormalizedConstraintSet::LongRange::LongRange(
    const dom::OwningLongOrConstrainLongRange& aOther, bool advanced)
: Range<int32_t>(1 + INT32_MIN, INT32_MAX) 
{
  if (aOther.IsLong()) {
    if (advanced) {
      mMin = mMax = aOther.GetAsLong();
    } else {
      mIdeal.Construct(aOther.GetAsLong());
    }
  } else {
    SetFrom(aOther.GetAsConstrainLongRange());
  }
}

NormalizedConstraintSet::DoubleRange::DoubleRange(
    const dom::OwningDoubleOrConstrainDoubleRange& aOther, bool advanced)
: Range<double>(-std::numeric_limits<double>::infinity(),
                std::numeric_limits<double>::infinity())
{
  if (aOther.IsDouble()) {
    if (advanced) {
      mMin = mMax = aOther.GetAsDouble();
    } else {
      mIdeal.Construct(aOther.GetAsDouble());
    }
  } else {
    SetFrom(aOther.GetAsConstrainDoubleRange());
  }
}

FlattenedConstraints::FlattenedConstraints(const dom::MediaTrackConstraints& aOther)
: NormalizedConstraintSet(aOther, false)
{
  if (aOther.mAdvanced.WasPassed()) {
    const auto& advanced = aOther.mAdvanced.Value();
    for (size_t i = 0; i < advanced.Length(); i++) {
      NormalizedConstraintSet set(advanced[i], true);
      
      
      if (mWidth.Intersects(set.mWidth) &&
          mHeight.Intersects(set.mHeight) &&
          mFrameRate.Intersects(set.mFrameRate)) {
        mWidth.Intersect(set.mWidth);
        mHeight.Intersect(set.mHeight);
        mFrameRate.Intersect(set.mFrameRate);
      }
    }
  }
}











uint32_t
MediaConstraintsHelper::GetMinimumFitnessDistance(
    const dom::MediaTrackConstraintSet &aConstraints,
    bool aAdvanced,
    const nsString& aDeviceId)
{
  uint64_t distance =
    uint64_t(FitnessDistance(aDeviceId, aConstraints.mDeviceId, aAdvanced));

  
  

  return uint32_t(std::min(distance, uint64_t(UINT32_MAX)));
}

template<class ValueType, class ConstrainRange>
 uint32_t
MediaConstraintsHelper::FitnessDistance(ValueType aN,
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
MediaConstraintsHelper::FitnessDistance(int32_t aN,
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
MediaConstraintsHelper::FitnessDistance(double aN,
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
MediaConstraintsHelper::FitnessDistance(nsString aN,
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
MediaConstraintsHelper::FitnessDistance(nsString aN,
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

}
