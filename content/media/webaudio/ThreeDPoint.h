





#ifndef ThreeDPoint_h_
#define ThreeDPoint_h_

#include <cmath>

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

  double Magnitude() const
  {
    return sqrt(x * x + y * y + z * z);
  }

  void Normalize()
  {
    double invDistance = 1 / Magnitude();
    x *= invDistance;
    y *= invDistance;
    z *= invDistance;
  }

  ThreeDPoint CrossProduct(const ThreeDPoint& rhs) const
  {
    return ThreeDPoint(y * rhs.z - z * rhs.y,
                       z * rhs.x - x * rhs.z,
                       x * rhs.y - y * rhs.x);
  }

  double DotProduct(const ThreeDPoint& rhs)
  {
    return x * rhs.x + y * rhs.y + z * rhs.z;
  }

  double Distance(const ThreeDPoint& rhs)
  {
    return sqrt(hypot(rhs.x, x) + hypot(rhs.y, y) + hypot(rhs.z, z));
  }

  bool IsZero() const
  {
    return x == 0 && y == 0 && z == 0;
  }
  double x, y, z;
};

ThreeDPoint operator-(const ThreeDPoint& lhs, const ThreeDPoint& rhs);
ThreeDPoint operator*(const ThreeDPoint& lhs, const ThreeDPoint& rhs);
ThreeDPoint operator*(const ThreeDPoint& lhs, const double rhs);
bool operator==(const ThreeDPoint& lhs, const ThreeDPoint& rhs);

}
}

#endif

