




#ifndef MOZILLA_GFX_PATHHELPERS_H_
#define MOZILLA_GFX_PATHHELPERS_H_

#include "2D.h"
#include "mozilla/Constants.h"
#include "UserData.h"

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











GFX2D_API void AppendRectToPath(PathBuilder* aPathBuilder,
                                const Rect& aRect,
                                bool aDrawClockwise = true);

inline TemporaryRef<Path> MakePathForRect(const DrawTarget& aDrawTarget,
                                          const Rect& aRect,
                                          bool aDrawClockwise = true)
{
  RefPtr<PathBuilder> builder = aDrawTarget.CreatePathBuilder();
  AppendRectToPath(builder, aRect, aDrawClockwise);
  return builder->Finish();
}

struct RectCornerRadii {
  Size radii[RectCorner::Count];

  RectCornerRadii() {}

  explicit RectCornerRadii(Float radius) {
    for (int i = 0; i < RectCorner::Count; i++) {
      radii[i].SizeTo(radius, radius);
    }
  }

  explicit RectCornerRadii(Float radiusX, Float radiusY) {
    for (int i = 0; i < RectCorner::Count; i++) {
      radii[i].SizeTo(radiusX, radiusY);
    }
  }

  RectCornerRadii(Float tl, Float tr, Float br, Float bl) {
    radii[RectCorner::TopLeft].SizeTo(tl, tl);
    radii[RectCorner::TopRight].SizeTo(tr, tr);
    radii[RectCorner::BottomRight].SizeTo(br, br);
    radii[RectCorner::BottomLeft].SizeTo(bl, bl);
  }

  RectCornerRadii(const Size& tl, const Size& tr,
                  const Size& br, const Size& bl) {
    radii[RectCorner::TopLeft] = tl;
    radii[RectCorner::TopRight] = tr;
    radii[RectCorner::BottomRight] = br;
    radii[RectCorner::BottomLeft] = bl;
  }

  const Size& operator[](size_t aCorner) const {
    return radii[aCorner];
  }

  Size& operator[](size_t aCorner) {
    return radii[aCorner];
  }

  void Scale(Float aXScale, Float aYScale) {
    for (int i = 0; i < RectCorner::Count; i++) {
      radii[i].Scale(aXScale, aYScale);
    }
  }

  const Size TopLeft() const { return radii[RectCorner::TopLeft]; }
  Size& TopLeft() { return radii[RectCorner::TopLeft]; }

  const Size TopRight() const { return radii[RectCorner::TopRight]; }
  Size& TopRight() { return radii[RectCorner::TopRight]; }

  const Size BottomRight() const { return radii[RectCorner::BottomRight]; }
  Size& BottomRight() { return radii[RectCorner::BottomRight]; }

  const Size BottomLeft() const { return radii[RectCorner::BottomLeft]; }
  Size& BottomLeft() { return radii[RectCorner::BottomLeft]; }
};













GFX2D_API void AppendRoundedRectToPath(PathBuilder* aPathBuilder,
                                       const Rect& aRect,
                                       const RectCornerRadii& aRadii,
                                       bool aDrawClockwise = true);

inline TemporaryRef<Path> MakePathForRoundedRect(const DrawTarget& aDrawTarget,
                                                 const Rect& aRect,
                                                 const RectCornerRadii& aRadii,
                                                 bool aDrawClockwise = true)
{
  RefPtr<PathBuilder> builder = aDrawTarget.CreatePathBuilder();
  AppendRoundedRectToPath(builder, aRect, aRadii, aDrawClockwise);
  return builder->Finish();
}








GFX2D_API void AppendEllipseToPath(PathBuilder* aPathBuilder,
                                   const Point& aCenter,
                                   const Size& aDimensions);

inline TemporaryRef<Path> MakePathForEllipse(const DrawTarget& aDrawTarget,
                                             const Point& aCenter,
                                             const Size& aDimensions)
{
  RefPtr<PathBuilder> builder = aDrawTarget.CreatePathBuilder();
  AppendEllipseToPath(builder, aCenter, aDimensions);
  return builder->Finish();
}











GFX2D_API bool SnapLineToDevicePixelsForStroking(Point& aP1, Point& aP2,
                                                 const DrawTarget& aDrawTarget);








GFX2D_API void StrokeSnappedEdgesOfRect(const Rect& aRect,
                                        DrawTarget& aDrawTarget,
                                        const ColorPattern& aColor,
                                        const StrokeOptions& aStrokeOptions);








GFX2D_API Margin MaxStrokeExtents(const StrokeOptions& aStrokeOptions,
                                  const Matrix& aTransform);

extern UserDataKey sDisablePixelSnapping;














inline bool UserToDevicePixelSnapped(Rect& aRect, const DrawTarget& aDrawTarget,
                                     bool aAllowScaleOr90DegreeRotate = false)
{
  if (aDrawTarget.GetUserData(&sDisablePixelSnapping)) {
    return false;
  }

  Matrix mat = aDrawTarget.GetTransform();

  const Float epsilon = 0.0000001f;
#define WITHIN_E(a,b) (fabs((a)-(b)) < epsilon)
  if (!aAllowScaleOr90DegreeRotate &&
      (!WITHIN_E(mat._11, 1.f) || !WITHIN_E(mat._22, 1.f) ||
       !WITHIN_E(mat._12, 0.f) || !WITHIN_E(mat._21, 0.f))) {
    
    return false;
  }
#undef WITHIN_E

  Point p1 = mat * aRect.TopLeft();
  Point p2 = mat * aRect.TopRight();
  Point p3 = mat * aRect.BottomRight();

  
  
  
  
  
  
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





inline void MaybeSnapToDevicePixels(Rect& aRect, const DrawTarget& aDrawTarget,
                                    bool aIgnoreScale = false)
{
  if (UserToDevicePixelSnapped(aRect, aDrawTarget, aIgnoreScale)) {
    
    
    Matrix mat = aDrawTarget.GetTransform();
    mat.Invert();
    aRect = mat.TransformBounds(aRect);
  }
}

} 
} 

#endif 
