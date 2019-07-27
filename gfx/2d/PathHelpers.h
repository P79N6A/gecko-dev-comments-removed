




#ifndef MOZILLA_GFX_PATHHELPERS_H_
#define MOZILLA_GFX_PATHHELPERS_H_

#include "2D.h"
#include "mozilla/Constants.h"

namespace mozilla {
namespace gfx {

template <typename T>
void ArcToBezier(T* aSink, const Point &aOrigin, const Size &aRadius,
                 float aStartAngle, float aEndAngle, bool aAntiClockwise)
{
  Point startPoint(aOrigin.x + cosf(aStartAngle) * aRadius.width,
                   aOrigin.y + sinf(aStartAngle) * aRadius.height);

  aSink->LineTo(startPoint);

  
  
  if (!aAntiClockwise && (aEndAngle < aStartAngle)) {
    Float correction = Float(ceil((aStartAngle - aEndAngle) / (2.0f * M_PI)));
    aEndAngle += float(correction * 2.0f * M_PI);
  } else if (aAntiClockwise && (aStartAngle < aEndAngle)) {
    Float correction = (Float)ceil((aEndAngle - aStartAngle) / (2.0f * M_PI));
    aStartAngle += float(correction * 2.0f * M_PI);
  }

  
  if (!aAntiClockwise && (aEndAngle - aStartAngle > 2 * M_PI)) {
    aEndAngle = float(aStartAngle + 2.0f * M_PI);
  } else if (aAntiClockwise && (aStartAngle - aEndAngle > 2.0f * M_PI)) {
    aEndAngle = float(aStartAngle - 2.0f * M_PI);
  }

  
  Float arcSweepLeft = fabs(aEndAngle - aStartAngle);

  Float sweepDirection = aAntiClockwise ? -1.0f : 1.0f;

  Float currentStartAngle = aStartAngle;

  while (arcSweepLeft > 0) {
    
    
    Float currentEndAngle;

    if (arcSweepLeft > M_PI / 2.0f) {
      currentEndAngle = Float(currentStartAngle + M_PI / 2.0f * sweepDirection);
    } else {
      currentEndAngle = currentStartAngle + arcSweepLeft * sweepDirection;
    }

    Point currentStartPoint(aOrigin.x + cosf(currentStartAngle) * aRadius.width,
                            aOrigin.y + sinf(currentStartAngle) * aRadius.height);
    Point currentEndPoint(aOrigin.x + cosf(currentEndAngle) * aRadius.width,
                          aOrigin.y + sinf(currentEndAngle) * aRadius.height);

    
    
    
    Float kappaFactor = (4.0f / 3.0f) * tan((currentEndAngle - currentStartAngle) / 4.0f);
    Float kappaX = kappaFactor * aRadius.width;
    Float kappaY = kappaFactor * aRadius.height;

    Point tangentStart(-sin(currentStartAngle), cos(currentStartAngle));
    Point cp1 = currentStartPoint;
    cp1 += Point(tangentStart.x * kappaX, tangentStart.y * kappaY);

    Point revTangentEnd(sin(currentEndAngle), -cos(currentEndAngle));
    Point cp2 = currentEndPoint;
    cp2 += Point(revTangentEnd.x * kappaX, revTangentEnd.y * kappaY);

    aSink->BezierTo(cp1, cp2, currentEndPoint);

    arcSweepLeft -= Float(M_PI / 2.0f);
    currentStartAngle = currentEndAngle;
  }
}




template <typename T>
void EllipseToBezier(T* aSink, const Point &aOrigin, const Size &aRadius)
{
  Point startPoint(aOrigin.x + aRadius.width,
                   aOrigin.y);

  aSink->LineTo(startPoint);

  
  
  
  Float kappaFactor = (4.0f / 3.0f) * tan((M_PI/2.0f) / 4.0f);
  Float kappaX = kappaFactor * aRadius.width;
  Float kappaY = kappaFactor * aRadius.height;
  Float cosStartAngle = 1;
  Float sinStartAngle = 0;
  for (int i = 0; i < 4; i++) {
    
    
    Point currentStartPoint(aOrigin.x + cosStartAngle * aRadius.width,
                            aOrigin.y + sinStartAngle * aRadius.height);
    Point currentEndPoint(aOrigin.x + -sinStartAngle * aRadius.width,
                          aOrigin.y + cosStartAngle * aRadius.height);

    Point tangentStart(-sinStartAngle, cosStartAngle);
    Point cp1 = currentStartPoint;
    cp1 += Point(tangentStart.x * kappaX, tangentStart.y * kappaY);

    Point revTangentEnd(cosStartAngle, sinStartAngle);
    Point cp2 = currentEndPoint;
    cp2 += Point(revTangentEnd.x * kappaX, revTangentEnd.y * kappaY);

    aSink->BezierTo(cp1, cp2, currentEndPoint);

    
    
    Float tmp = cosStartAngle;
    cosStartAngle = -sinStartAngle;
    sinStartAngle = tmp;
  }
}













GFX2D_API void AppendRoundedRectToPath(PathBuilder* aPathBuilder,
                                       const Rect& aRect,
                                       const Size(& aCornerRadii)[4],
                                       bool aDrawClockwise = true);








GFX2D_API void AppendEllipseToPath(PathBuilder* aPathBuilder,
                                   const Point& aCenter,
                                   const Size& aDimensions);

static inline bool
UserToDevicePixelSnapped(Rect& aRect, const Matrix& aTransform)
{
  Point p1 = aTransform * aRect.TopLeft();
  Point p2 = aTransform * aRect.TopRight();
  Point p3 = aTransform * aRect.BottomRight();

  
  
  
  
  
  
  if (p2 == Point(p1.x, p3.y) || p2 == Point(p3.x, p1.y)) {
      p1.Round();
      p3.Round();

      aRect.MoveTo(Point(std::min(p1.x, p3.x), std::min(p1.y, p3.y)));
      aRect.SizeTo(Size(std::max(p1.x, p3.x) - aRect.X(),
                        std::max(p1.y, p3.y) - aRect.Y()));
      return true;
  }

  return false;
}

} 
} 

#endif 
