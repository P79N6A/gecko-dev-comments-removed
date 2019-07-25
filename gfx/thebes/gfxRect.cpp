




































#include "gfxRect.h"

#include "nsMathUtils.h"

static bool
WithinEpsilonOfInteger(gfxFloat aX, gfxFloat aEpsilon)
{
    return fabs(NS_round(aX) - aX) <= fabs(aEpsilon);
}

bool
gfxRect::WithinEpsilonOfIntegerPixels(gfxFloat aEpsilon) const
{
    NS_ASSERTION(-0.5 < aEpsilon && aEpsilon < 0.5, "Nonsense epsilon value");
    return (WithinEpsilonOfInteger(x, aEpsilon) &&
            WithinEpsilonOfInteger(y, aEpsilon) &&
            WithinEpsilonOfInteger(width, aEpsilon) &&
            WithinEpsilonOfInteger(height, aEpsilon));
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
