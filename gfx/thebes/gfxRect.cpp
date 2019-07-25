




































#include "gfxRect.h"

#include "nsMathUtils.h"

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

PRBool
gfxRect::Contains(const gfxRect& aRect) const
{
  return aRect.X() >= X() && aRect.XMost() <= XMost() &&
         aRect.Y() >= Y() && aRect.YMost() <= YMost();
}

PRBool
gfxRect::Contains(const gfxPoint& aPoint) const
{
  return aPoint.x >= X() && aPoint.x <= XMost() &&
         aPoint.y >= Y() && aPoint.y <= YMost();
}

void
gfxRect::Round()
{
    
    gfxFloat x0 = NS_floor(X() + 0.5);
    gfxFloat y0 = NS_floor(Y() + 0.5);
    gfxFloat x1 = NS_floor(XMost() + 0.5);
    gfxFloat y1 = NS_floor(YMost() + 0.5);

    pos.x = x0;
    pos.y = y0;

    size.width = x1 - x0;
    size.height = y1 - y0;
}

void
gfxRect::RoundIn()
{
    gfxFloat x0 = NS_ceil(X());
    gfxFloat y0 = NS_ceil(Y());
    gfxFloat x1 = NS_floor(XMost());
    gfxFloat y1 = NS_floor(YMost());

    pos.x = x0;
    pos.y = y0;

    size.width = x1 - x0;
    size.height = y1 - y0;
}

void
gfxRect::RoundOut()
{
    gfxFloat x0 = NS_floor(X());
    gfxFloat y0 = NS_floor(Y());
    gfxFloat x1 = NS_ceil(XMost());
    gfxFloat y1 = NS_ceil(YMost());

    pos.x = x0;
    pos.y = y0;

    size.width = x1 - x0;
    size.height = y1 - y0;
}








#define CAIRO_COORD_MAX (16777215.0)
#define CAIRO_COORD_MIN (-16777216.0)

void
gfxRect::Condition()
{
    
    
    if (pos.x > CAIRO_COORD_MAX) {
        pos.x = CAIRO_COORD_MAX;
        size.width = 0.0;
    } 

    if (pos.y > CAIRO_COORD_MAX) {
        pos.y = CAIRO_COORD_MAX;
        size.height = 0.0;
    }

    if (pos.x < CAIRO_COORD_MIN) {
        size.width += pos.x - CAIRO_COORD_MIN;
        if (size.width < 0.0)
            size.width = 0.0;
        pos.x = CAIRO_COORD_MIN;
    }

    if (pos.y < CAIRO_COORD_MIN) {
        size.height += pos.y - CAIRO_COORD_MIN;
        if (size.height < 0.0)
            size.height = 0.0;
        pos.y = CAIRO_COORD_MIN;
    }

    if (pos.x + size.width > CAIRO_COORD_MAX) {
        size.width = CAIRO_COORD_MAX - pos.x;
    }

    if (pos.y + size.height > CAIRO_COORD_MAX) {
        size.height = CAIRO_COORD_MAX - pos.y;
    }
}
