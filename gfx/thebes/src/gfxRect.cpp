




































#include "gfxRect.h"

gfxRect gfxRect::Intersect(const gfxRect& aRect) const
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

gfxRect gfxRect::Union(const gfxRect& aRect) const
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
