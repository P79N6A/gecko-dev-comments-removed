




































#ifndef MOZILLA_GFX_PATHHELPERS_H_
#define MOZILLA_GFX_PATHHELPERS_H_

#include "2D.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace mozilla {
namespace gfx {

template <typename T>
void ArcToBezier(T* aSink, const Point &aOrigin, float aRadius, float aStartAngle,
                 float aEndAngle, bool aAntiClockwise)
{
  Point startPoint(aOrigin.x + cos(aStartAngle) * aRadius, 
                   aOrigin.y + sin(aStartAngle) * aRadius); 
  
  aSink->LineTo(startPoint); 

  
  
  if (!aAntiClockwise && (aEndAngle < aStartAngle)) {
    Float correction = ceil((aStartAngle - aEndAngle) / (2.0f * M_PI));
    aEndAngle += correction * 2.0f * M_PI;
  } else if (aAntiClockwise && (aStartAngle < aEndAngle)) {
    Float correction = ceil((aEndAngle - aStartAngle) / (2.0f * M_PI));
    aStartAngle += correction * 2.0f * M_PI;
  }
  
  
  if (!aAntiClockwise && (aEndAngle - aStartAngle > 2 * M_PI)) {
    aEndAngle = aStartAngle + 2.0f * M_PI;
  } else if (aAntiClockwise && (aStartAngle - aEndAngle > 2.0f * M_PI)) {
    aEndAngle = aStartAngle - 2.0f * M_PI;
  }

  
  Float arcSweepLeft = fabs(aEndAngle - aStartAngle);
  
  Float sweepDirection = aAntiClockwise ? -1.0f : 1.0f;
  
  Float currentStartAngle = aStartAngle;
  
  while (arcSweepLeft > 0) {
    
    
    Float currentEndAngle;
    
    if (arcSweepLeft > M_PI / 2.0f) {
      currentEndAngle = currentStartAngle + M_PI / 2.0f * sweepDirection;                                                               
    } else {
      currentEndAngle = currentStartAngle + arcSweepLeft * sweepDirection;
    }
    
    Point currentStartPoint(aOrigin.x + cos(currentStartAngle) * aRadius,
                            aOrigin.y + sin(currentStartAngle) * aRadius);
    Point currentEndPoint(aOrigin.x + cos(currentEndAngle) * aRadius,
                          aOrigin.y + sin(currentEndAngle) * aRadius);
    
    
    
    
    Float kappa = (4.0f / 3.0f) * tan((currentEndAngle - currentStartAngle) / 4.0f) * aRadius;
    
    Point tangentStart(-sin(currentStartAngle), cos(currentStartAngle));
    Point cp1 = currentStartPoint;
    cp1 += tangentStart * kappa;
    
    Point revTangentEnd(sin(currentEndAngle), -cos(currentEndAngle));
    Point cp2 = currentEndPoint;
    cp2 += revTangentEnd * kappa;
    
    aSink->BezierTo(cp1, cp2, currentEndPoint);
    
    arcSweepLeft -= M_PI / 2.0f;
    currentStartAngle = currentEndAngle;
  }
}

}
}

#endif 
