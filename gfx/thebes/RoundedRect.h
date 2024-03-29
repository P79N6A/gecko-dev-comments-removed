




#include "gfxRect.h"
#include "mozilla/gfx/PathHelpers.h"

namespace mozilla {






struct RoundedRect {
    typedef mozilla::gfx::RectCornerRadii RectCornerRadii;

    RoundedRect(gfxRect &aRect, RectCornerRadii &aCorners) : rect(aRect), corners(aCorners) { }
    void Deflate(gfxFloat aTopWidth, gfxFloat aBottomWidth, gfxFloat aLeftWidth, gfxFloat aRightWidth) {
        
        rect.x += aLeftWidth;
        rect.y += aTopWidth;
        rect.width = std::max(0., rect.width - aLeftWidth - aRightWidth);
        rect.height = std::max(0., rect.height - aTopWidth - aBottomWidth);

        corners.radii[NS_CORNER_TOP_LEFT].width  = std::max(0., corners.radii[NS_CORNER_TOP_LEFT].width - aLeftWidth);
        corners.radii[NS_CORNER_TOP_LEFT].height = std::max(0., corners.radii[NS_CORNER_TOP_LEFT].height - aTopWidth);

        corners.radii[NS_CORNER_TOP_RIGHT].width  = std::max(0., corners.radii[NS_CORNER_TOP_RIGHT].width - aRightWidth);
        corners.radii[NS_CORNER_TOP_RIGHT].height = std::max(0., corners.radii[NS_CORNER_TOP_RIGHT].height - aTopWidth);

        corners.radii[NS_CORNER_BOTTOM_LEFT].width  = std::max(0., corners.radii[NS_CORNER_BOTTOM_LEFT].width - aLeftWidth);
        corners.radii[NS_CORNER_BOTTOM_LEFT].height = std::max(0., corners.radii[NS_CORNER_BOTTOM_LEFT].height - aBottomWidth);

        corners.radii[NS_CORNER_BOTTOM_RIGHT].width  = std::max(0., corners.radii[NS_CORNER_BOTTOM_RIGHT].width - aRightWidth);
        corners.radii[NS_CORNER_BOTTOM_RIGHT].height = std::max(0., corners.radii[NS_CORNER_BOTTOM_RIGHT].height - aBottomWidth);
    }
    gfxRect rect;
    RectCornerRadii corners;
};

} 
