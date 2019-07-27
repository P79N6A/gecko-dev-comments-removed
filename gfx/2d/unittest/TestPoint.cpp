




#include "TestPoint.h"

#include "Point.h"

using namespace mozilla::gfx;

TestPoint::TestPoint()
{
  REGISTER_TEST(TestPoint, Addition);
  REGISTER_TEST(TestPoint, Subtraction);
}

void
TestPoint::Addition()
{
  Point a, b;
  a.x = 2;
  a.y = 2;
  b.x = 5;
  b.y = -5;

  a += b;

  VERIFY(a.x == 7.f);
  VERIFY(a.y == -3.f);
}

void
TestPoint::Subtraction()
{
  Point a, b;
  a.x = 2;
  a.y = 2;
  b.x = 5;
  b.y = -5;

  a -= b;

  VERIFY(a.x == -3.f);
  VERIFY(a.y == 7.f);
}
