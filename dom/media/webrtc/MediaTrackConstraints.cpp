




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

}
