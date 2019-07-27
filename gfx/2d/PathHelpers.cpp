




#define _USE_MATH_DEFINES
#include <cmath>

#include "PathHelpers.h"

namespace mozilla {
namespace gfx {

UserDataKey sDisablePixelSnapping;

void
AppendRectToPath(PathBuilder* aPathBuilder,
                 const Rect& aRect,
                 bool aDrawClockwise)
{
  if (aDrawClockwise) {
    aPathBuilder->MoveTo(aRect.TopLeft());
    aPathBuilder->LineTo(aRect.TopRight());
    aPathBuilder->LineTo(aRect.BottomRight());
    aPathBuilder->LineTo(aRect.BottomLeft());
  } else {
    aPathBuilder->MoveTo(aRect.TopRight());
    aPathBuilder->LineTo(aRect.TopLeft());
    aPathBuilder->LineTo(aRect.BottomLeft());
    aPathBuilder->LineTo(aRect.BottomRight());
  }
  aPathBuilder->Close();
}

void
AppendRoundedRectToPath(PathBuilder* aPathBuilder,
                        const Rect& aRect,
                        const RectCornerRadii& aRadii,
                        bool aDrawClockwise)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  const Float alpha = Float(0.55191497064665766025);

  typedef struct { Float a, b; } twoFloats;

  twoFloats cwCornerMults[4] = { { -1,  0 },    
                                 {  0, -1 },
                                 { +1,  0 },
                                 {  0, +1 } };
  twoFloats ccwCornerMults[4] = { { +1,  0 },   
                                  {  0, -1 },
                                  { -1,  0 },
                                  {  0, +1 } };

  twoFloats *cornerMults = aDrawClockwise ? cwCornerMults : ccwCornerMults;

  Point cornerCoords[] = { aRect.TopLeft(), aRect.TopRight(),
                           aRect.BottomRight(), aRect.BottomLeft() };

  Point pc, p0, p1, p2, p3;

  if (aDrawClockwise) {
    aPathBuilder->MoveTo(Point(aRect.X() + aRadii[RectCorner::TopLeft].width,
                               aRect.Y()));
  } else {
    aPathBuilder->MoveTo(Point(aRect.X() + aRect.Width() - aRadii[RectCorner::TopRight].width,
                               aRect.Y()));
  }

  for (int i = 0; i < 4; ++i) {
    
    int c = aDrawClockwise ? ((i+1) % 4) : ((4-i) % 4);

    
    
    
    int i2 = (i+2) % 4;
    int i3 = (i+3) % 4;

    pc = cornerCoords[c];

    if (aRadii[c].width > 0.0 && aRadii[c].height > 0.0) {
      p0.x = pc.x + cornerMults[i].a * aRadii[c].width;
      p0.y = pc.y + cornerMults[i].b * aRadii[c].height;

      p3.x = pc.x + cornerMults[i3].a * aRadii[c].width;
      p3.y = pc.y + cornerMults[i3].b * aRadii[c].height;

      p1.x = p0.x + alpha * cornerMults[i2].a * aRadii[c].width;
      p1.y = p0.y + alpha * cornerMults[i2].b * aRadii[c].height;

      p2.x = p3.x - alpha * cornerMults[i3].a * aRadii[c].width;
      p2.y = p3.y - alpha * cornerMults[i3].b * aRadii[c].height;

      aPathBuilder->LineTo(p0);
      aPathBuilder->BezierTo(p1, p2, p3);
    } else {
      aPathBuilder->LineTo(pc);
    }
  }

  aPathBuilder->Close();
}

void
AppendEllipseToPath(PathBuilder* aPathBuilder,
                    const Point& aCenter,
                    const Size& aDimensions)
{
  Size halfDim = aDimensions / 2.f;
  Rect rect(aCenter - Point(halfDim.width, halfDim.height), aDimensions);
  RectCornerRadii radii(halfDim.width, halfDim.height);

  AppendRoundedRectToPath(aPathBuilder, rect, radii);
}

bool
SnapLineToDevicePixelsForStroking(Point& aP1, Point& aP2,
                                  const DrawTarget& aDrawTarget)
{
  Matrix mat = aDrawTarget.GetTransform();
  if (mat.HasNonTranslation()) {
    return false;
  }
  if (aP1.x != aP2.x && aP1.y != aP2.y) {
    return false; 
  }
  Point p1 = aP1 + mat.GetTranslation(); 
  Point p2 = aP2 + mat.GetTranslation();
  p1.Round();
  p2.Round();
  p1 -= mat.GetTranslation(); 
  p2 -= mat.GetTranslation();
  if (aP1.x == aP2.x) {
    
    aP1 = p1 + Point(0.5, 0);
    aP2 = p2 + Point(0.5, 0);
  } else {
    
    aP1 = p1 + Point(0, 0.5);
    aP2 = p2 + Point(0, 0.5);
  }
  return true;
}

void
StrokeSnappedEdgesOfRect(const Rect& aRect, DrawTarget& aDrawTarget,
                        const ColorPattern& aColor,
                        const StrokeOptions& aStrokeOptions)
{
  if (aRect.IsEmpty()) {
    return;
  }

  Point p1 = aRect.TopLeft();
  Point p2 = aRect.BottomLeft();
  SnapLineToDevicePixelsForStroking(p1, p2, aDrawTarget);
  aDrawTarget.StrokeLine(p1, p2, aColor, aStrokeOptions);

  p1 = aRect.BottomLeft();
  p2 = aRect.BottomRight();
  SnapLineToDevicePixelsForStroking(p1, p2, aDrawTarget);
  aDrawTarget.StrokeLine(p1, p2, aColor, aStrokeOptions);

  p1 = aRect.TopLeft();
  p2 = aRect.TopRight();
  SnapLineToDevicePixelsForStroking(p1, p2, aDrawTarget);
  aDrawTarget.StrokeLine(p1, p2, aColor, aStrokeOptions);

  p1 = aRect.TopRight();
  p2 = aRect.BottomRight();
  SnapLineToDevicePixelsForStroking(p1, p2, aDrawTarget);
  aDrawTarget.StrokeLine(p1, p2, aColor, aStrokeOptions);
}


Margin
MaxStrokeExtents(const StrokeOptions& aStrokeOptions,
                 const Matrix& aTransform)
{
  double styleExpansionFactor = 0.5f;

  if (aStrokeOptions.mLineCap == CapStyle::SQUARE) {
    styleExpansionFactor = M_SQRT1_2;
  }

  if (aStrokeOptions.mLineJoin == JoinStyle::MITER &&
      styleExpansionFactor < M_SQRT2 * aStrokeOptions.mMiterLimit) {
    styleExpansionFactor = M_SQRT2 * aStrokeOptions.mMiterLimit;
  }

  styleExpansionFactor *= aStrokeOptions.mLineWidth;

  double dx = styleExpansionFactor * hypot(aTransform._11, aTransform._21);
  double dy = styleExpansionFactor * hypot(aTransform._22, aTransform._12);
  return Margin(dy, dx, dy, dx);
}

} 
} 

