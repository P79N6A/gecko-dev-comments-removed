




































#include "PathSkia.h"
#include <math.h>
#include "DrawTargetSkia.h"
#include "Logging.h"
#include "HelpersSkia.h"

namespace mozilla {
namespace gfx {

PathBuilderSkia::PathBuilderSkia(const Matrix& aTransform, const SkPath& aPath, FillRule aFillRule)
  : mPath(aPath)
{
  SkMatrix matrix;
  GfxMatrixToSkiaMatrix(aTransform, matrix);
  mPath.transform(matrix);
  SetFillRule(aFillRule);
}

PathBuilderSkia::PathBuilderSkia(FillRule aFillRule)
{
  SetFillRule(aFillRule);
}

void
PathBuilderSkia::SetFillRule(FillRule aFillRule)
{
  mFillRule = aFillRule;
  if (mFillRule == FILL_WINDING) {
    mPath.setFillType(SkPath::kWinding_FillType);
  } else {
    mPath.setFillType(SkPath::kEvenOdd_FillType);
  }
}

void
PathBuilderSkia::MoveTo(const Point &aPoint)
{
  mPath.moveTo(SkFloatToScalar(aPoint.x), SkFloatToScalar(aPoint.y));
}

void
PathBuilderSkia::LineTo(const Point &aPoint)
{
  if (!mPath.countPoints()) {
    MoveTo(aPoint);
  } else {
    mPath.lineTo(SkFloatToScalar(aPoint.x), SkFloatToScalar(aPoint.y));
  }
}

void
PathBuilderSkia::BezierTo(const Point &aCP1,
                          const Point &aCP2,
                          const Point &aCP3)
{
  if (!mPath.countPoints()) {
    MoveTo(aCP1);
  }
  mPath.cubicTo(SkFloatToScalar(aCP1.x), SkFloatToScalar(aCP1.y),
                SkFloatToScalar(aCP2.x), SkFloatToScalar(aCP2.y),
                SkFloatToScalar(aCP3.x), SkFloatToScalar(aCP3.y));
}

void
PathBuilderSkia::QuadraticBezierTo(const Point &aCP1,
                                   const Point &aCP2)
{
  if (!mPath.countPoints()) {
    MoveTo(aCP1);
  }
  mPath.quadTo(SkFloatToScalar(aCP1.x), SkFloatToScalar(aCP1.y),
               SkFloatToScalar(aCP2.x), SkFloatToScalar(aCP2.y));
}

void
PathBuilderSkia::Close()
{
  mPath.close();
}

void
PathBuilderSkia::Arc(const Point &aOrigin, float aRadius, float aStartAngle,
                     float aEndAngle, bool aAntiClockwise)
{ 
  
  Point startPoint(aOrigin.x + cos(aStartAngle) * aRadius, 
                   aOrigin.y + sin(aStartAngle) * aRadius); 
  
  LineTo(startPoint); 

  
  
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
    
    BezierTo(cp1, cp2, currentEndPoint);
    
    arcSweepLeft -= M_PI / 2.0f;
    currentStartAngle = currentEndAngle;
  }
}

Point
PathBuilderSkia::CurrentPoint() const
{
  int pointCount = mPath.countPoints();
  if (!pointCount) {
    return Point(0, 0);
  }
  SkPoint point = mPath.getPoint(pointCount - 1);
  return Point(SkScalarToFloat(point.fX), SkScalarToFloat(point.fY));
}

TemporaryRef<Path>
PathBuilderSkia::Finish()
{
  RefPtr<PathSkia> path = new PathSkia(mPath, mFillRule);
  return path;
}

TemporaryRef<PathBuilder>
PathSkia::CopyToBuilder(FillRule aFillRule) const
{
  return TransformedCopyToBuilder(Matrix(), aFillRule);
}

TemporaryRef<PathBuilder>
PathSkia::TransformedCopyToBuilder(const Matrix &aTransform, FillRule aFillRule) const
{
  RefPtr<PathBuilderSkia> builder = new PathBuilderSkia(aTransform, mPath, aFillRule);
  return builder;
}

bool
PathSkia::ContainsPoint(const Point &aPoint, const Matrix &aTransform) const
{
  Matrix inverse = aTransform;
  inverse.Invert();
  Point transformed = inverse * aPoint;

  Rect bounds = GetBounds(aTransform);

  if (aPoint.x < bounds.x || aPoint.y < bounds.y ||
      aPoint.x > bounds.XMost() || aPoint.y > bounds.YMost()) {
    return false;
  }

  SkRegion pointRect;
  pointRect.setRect(SkFloatToScalar(transformed.x - 1), SkFloatToScalar(transformed.y - 1), 
                    SkFloatToScalar(transformed.x + 1), SkFloatToScalar(transformed.y + 1));

  SkRegion pathRegion;
  
  return pathRegion.setPath(mPath, pointRect);
}

static Rect SkRectToRect(const SkRect& aBounds)
{
  return Rect(SkScalarToFloat(aBounds.fLeft),
              SkScalarToFloat(aBounds.fTop),
              SkScalarToFloat(aBounds.fRight - aBounds.fLeft),
              SkScalarToFloat(aBounds.fBottom - aBounds.fTop));
}

Rect
PathSkia::GetBounds(const Matrix &aTransform) const
{
  Rect bounds = SkRectToRect(mPath.getBounds());
  return aTransform.TransformBounds(bounds);
}

Rect
PathSkia::GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                           const Matrix &aTransform) const
{
  NS_ASSERTION(false, "GetStrokedBounds not supported yet!");
  return Rect(0, 0, 0, 0);
}

}
}
