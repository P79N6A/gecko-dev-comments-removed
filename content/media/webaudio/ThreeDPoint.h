





#ifndef ThreeDPoint_h_
#define ThreeDPoint_h_

namespace mozilla {

namespace dom {

struct ThreeDPoint {
  ThreeDPoint()
    : x(0.)
    , y(0.)
    , z(0.)
  {
  }
  ThreeDPoint(double aX, double aY, double aZ)
    : x(aX)
    , y(aY)
    , z(aZ)
  {
  }

  double x, y, z;
};

}
}

#endif

