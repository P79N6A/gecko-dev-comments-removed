




#ifndef mozilla_image_src_Orientation_h
#define mozilla_image_src_Orientation_h

#include <stdint.h>

namespace mozilla {
namespace image {

enum class Angle : uint8_t {
  D0,
  D90,
  D180,
  D270
};

enum class Flip : uint8_t {
  Unflipped,
  Horizontal
};






struct Orientation
{
  explicit Orientation(Angle aRotation = Angle::D0,
                       Flip mFlip = Flip::Unflipped)
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
