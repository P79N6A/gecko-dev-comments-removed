




































#include "gfxRect.h"

#include <math.h>

gfxRect
gfxRect::Intersect(const gfxRect& aRect) const
{
  gfxRect result(0,0,0,0);

  gfxFloat x = PR_MAX(aRect.X(), X());
  gfxFloat xmost = PR_MIN(aRect.XMost(), XMost());
  if (x >= xmost)
    return result;

  gfxFloat y = PR_MAX(aRect.Y(), Y());
  gfxFloat ymost = PR_MIN(aRect.YMost(), YMost());
  if (y >= ymost)
    return result;

  result = gfxRect(x, y, xmost - x, ymost - y);
  return result;
}

gfxRect
gfxRect::Union(const gfxRect& aRect) const
{
  if (IsEmpty())
    return aRect;
  if (aRect.IsEmpty())
    return *this;

  gfxFloat x = PR_MIN(aRect.X(), X());
  gfxFloat xmost = PR_MAX(aRect.XMost(), XMost());
  gfxFloat y = PR_MIN(aRect.Y(), Y());
  gfxFloat ymost = PR_MAX(aRect.YMost(), YMost());
  return gfxRect(x, y, xmost - x, ymost - y);
}

void
gfxRect::Round()
{
    gfxFloat x0 = floor(X() + 0.5);
    gfxFloat y0 = floor(Y() + 0.5);
    gfxFloat x1 = floor(XMost() + 0.5);
    gfxFloat y1 = floor(YMost() + 0.5);

    pos.x = x0;
    pos.y = y0;

    size.width = x1 - x0;
    size.height = y1 - y0;
}
