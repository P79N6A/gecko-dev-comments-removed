




#ifndef MOZILLA_IMAGELIB_ORIENTATION_H_
#define MOZILLA_IMAGELIB_ORIENTATION_H_

#include <stdint.h>
#include "mozilla/TypedEnum.h"

namespace mozilla {
namespace image {

MOZ_BEGIN_ENUM_CLASS(Angle, uint8_t)
  D0,
  D90,
  D180,
  D270
MOZ_END_ENUM_CLASS(Angle)

MOZ_BEGIN_ENUM_CLASS(Flip, uint8_t)
  Unflipped,
  Horizontal
MOZ_END_ENUM_CLASS(Flip)






struct Orientation
{
  explicit Orientation(Angle aRotation = Angle::D0, Flip mFlip = Flip::Unflipped)
    : rotation(aRotation)
    , flip(mFlip)
  { }

  bool IsIdentity() const {
    return (rotation == Angle::D0) && (flip == Flip::Unflipped);
  }

  bool SwapsWidthAndHeight() const {
    return (rotation == Angle::D90) || (rotation == Angle::D270);
  }

  bool operator==(const Orientation& aOther) const {
    return (rotation == aOther.rotation) && (flip == aOther.flip);
  }

  bool operator!=(const Orientation& aOther) const {
    return !(*this == aOther);
  }

  Angle rotation;
  Flip  flip;
};

}
}

#endif
