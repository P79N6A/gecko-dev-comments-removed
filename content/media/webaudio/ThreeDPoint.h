





#ifndef ThreeDPoint_h_
#define ThreeDPoint_h_

namespace mozilla {

namespace dom {

struct ThreeDPoint {
  ThreeDPoint()
    : x(0.f)
    , y(0.f)
    , z(0.f)
  {
  }
  ThreeDPoint(float aX, float aY, float aZ)
    : x(aX)
    , y(aY)
    , z(aZ)
  {
  }

  float x, y, z;
};

}
}

#endif

