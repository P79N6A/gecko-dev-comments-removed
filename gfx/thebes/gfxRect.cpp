




































#include "gfxRect.h"

#include "nsMathUtils.h"

static PRBool
WithinEpsilonOfInteger(gfxFloat aX, gfxFloat aEpsilon)
{
    return fabs(NS_round(aX) - aX) <= fabs(aEpsilon);
}

PRBool
gfxRect::WithinEpsilonOfIntegerPixels(gfxFloat aEpsilon) const
{
    NS_ASSERTION(-0.5 < aEpsilon && aEpsilon < 0.5, "Nonsense epsilon value");
    return (WithinEpsilonOfInteger(x, aEpsilon) &&
            WithinEpsilonOfInteger(y, aEpsilon) &&
            WithinEpsilonOfInteger(width, aEpsilon) &&
            WithinEpsilonOfInteger(height, aEpsilon));
}

void
gfxRect::Round()
{
    
    gfxFloat x0 = NS_floor(X() + 0.5);
    gfxFloat y0 = NS_floor(Y() + 0.5);
    gfxFloat x1 = NS_floor(XMost() + 0.5);
    gfxFloat y1 = NS_floor(YMost() + 0.5);

    x = x0;
    y = y0;

    width = x1 - x0;
    height = y1 - y0;
}

void
gfxRect::RoundIn()
{
    gfxFloat x0 = NS_ceil(X());
    gfxFloat y0 = NS_ceil(Y());
    gfxFloat x1 = NS_floor(XMost());
    gfxFloat y1 = NS_floor(YMost());

    x = x0;
    y = y0;

    width = x1 - x0;
    height = y1 - y0;
}

void
gfxRect::RoundOut()
{
    gfxFloat x0 = NS_floor(X());
    gfxFloat y0 = NS_floor(Y());
    gfxFloat x1 = NS_ceil(XMost());
    gfxFloat y1 = NS_ceil(YMost());

    x = x0;
    y = y0;

    width = x1 - x0;
    height = y1 - y0;
}








#define CAIRO_COORD_MAX (16777215.0)
#define CAIRO_COORD_MIN (-16777216.0)

void
gfxRect::Condition()
{
    
    
    if (x > CAIRO_COORD_MAX) {
        x = CAIRO_COORD_MAX;
        width = 0.0;
    } 

    if (y > CAIRO_COORD_MAX) {
        y = CAIRO_COORD_MAX;
        height = 0.0;
    }

    if (x < CAIRO_COORD_MIN) {
        width += x - CAIRO_COORD_MIN;
        if (width < 0.0)
            width = 0.0;
        x = CAIRO_COORD_MIN;
    }

    if (y < CAIRO_COORD_MIN) {
        height += y - CAIRO_COORD_MIN;
        if (height < 0.0)
            height = 0.0;
        y = CAIRO_COORD_MIN;
    }

    if (x + width > CAIRO_COORD_MAX) {
        width = CAIRO_COORD_MAX - x;
    }

    if (y + height > CAIRO_COORD_MAX) {
        height = CAIRO_COORD_MAX - y;
    }
}
