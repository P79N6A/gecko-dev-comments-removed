




#include "PathHelpers.h"

namespace mozilla {
namespace gfx {

void
AppendRoundedRectToPath(PathBuilder* aPathBuilder,
                        const Rect& aRect,
                        
                        const Size(& aCornerRadii)[4],
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

  
  const int kTopLeft = 0, kTopRight = 1;

  if (aDrawClockwise) {
    aPathBuilder->MoveTo(Point(aRect.X() + aCornerRadii[kTopLeft].width,
                               aRect.Y()));
  } else {
    aPathBuilder->MoveTo(Point(aRect.X() + aRect.Width() - aCornerRadii[kTopRight].width,
                               aRect.Y()));
  }

  for (int i = 0; i < 4; ++i) {
    
    int c = aDrawClockwise ? ((i+1) % 4) : ((4-i) % 4);

    
    
    
    int i2 = (i+2) % 4;
    int i3 = (i+3) % 4;

    pc = cornerCoords[c];

    if (aCornerRadii[c].width > 0.0 && aCornerRadii[c].height > 0.0) {
      p0.x = pc.x + cornerMults[i].a * aCornerRadii[c].width;
      p0.y = pc.y + cornerMults[i].b * aCornerRadii[c].height;

      p3.x = pc.x + cornerMults[i3].a * aCornerRadii[c].width;
      p3.y = pc.y + cornerMults[i3].b * aCornerRadii[c].height;

      p1.x = p0.x + alpha * cornerMults[i2].a * aCornerRadii[c].width;
      p1.y = p0.y + alpha * cornerMults[i2].b * aCornerRadii[c].height;

      p2.x = p3.x - alpha * cornerMults[i3].a * aCornerRadii[c].width;
      p2.y = p3.y - alpha * cornerMults[i3].b * aCornerRadii[c].height;

      aPathBuilder->LineTo(p0);
      aPathBuilder->BezierTo(p1, p2, p3);
    } else {
      aPathBuilder->LineTo(pc);
    }
  }

  aPathBuilder->Close();
}

} 
} 

