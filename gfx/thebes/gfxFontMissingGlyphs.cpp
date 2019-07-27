




#include "gfxFontMissingGlyphs.h"

#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Helpers.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/RefPtr.h"
#include "nsDeviceContext.h"
#include "nsLayoutUtils.h"

using namespace mozilla;
using namespace mozilla::gfx;

#define CHAR_BITS(b00, b01, b02, b10, b11, b12, b20, b21, b22, b30, b31, b32, b40, b41, b42) \
  ((b00 << 0) | (b01 << 1) | (b02 << 2) | (b10 << 3) | (b11 << 4) | (b12 << 5) | \
   (b20 << 6) | (b21 << 7) | (b22 << 8) | (b30 << 9) | (b31 << 10) | (b32 << 11) | \
   (b40 << 12) | (b41 << 13) | (b42 << 14))

static const uint16_t glyphMicroFont[16] = {
  CHAR_BITS(0, 1, 0,
            1, 0, 1,
            1, 0, 1,
            1, 0, 1,
            0, 1, 0),
  CHAR_BITS(0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0,
            0, 1, 0),
  CHAR_BITS(1, 1, 1,
            0, 0, 1,
            1, 1, 1,
            1, 0, 0,
            1, 1, 1),
  CHAR_BITS(1, 1, 1,
            0, 0, 1,
            1, 1, 1,
            0, 0, 1,
            1, 1, 1),
  CHAR_BITS(1, 0, 1,
            1, 0, 1,
            1, 1, 1,
            0, 0, 1,
            0, 0, 1),
  CHAR_BITS(1, 1, 1,
            1, 0, 0,
            1, 1, 1,
            0, 0, 1,
            1, 1, 1),
  CHAR_BITS(1, 1, 1,
            1, 0, 0,
            1, 1, 1,
            1, 0, 1,
            1, 1, 1),
  CHAR_BITS(1, 1, 1,
            0, 0, 1,
            0, 0, 1,
            0, 0, 1,
            0, 0, 1),
  CHAR_BITS(0, 1, 0,
            1, 0, 1,
            0, 1, 0,
            1, 0, 1,
            0, 1, 0),
  CHAR_BITS(1, 1, 1,
            1, 0, 1,
            1, 1, 1,
            0, 0, 1,
            0, 0, 1),
  CHAR_BITS(1, 1, 1,
            1, 0, 1,
            1, 1, 1,
            1, 0, 1,
            1, 0, 1),
  CHAR_BITS(1, 1, 0,
            1, 0, 1,
            1, 1, 0,
            1, 0, 1,
            1, 1, 0),
  CHAR_BITS(0, 1, 1,
            1, 0, 0,
            1, 0, 0,
            1, 0, 0,
            0, 1, 1),
  CHAR_BITS(1, 1, 0,
            1, 0, 1,
            1, 0, 1,
            1, 0, 1,
            1, 1, 0),
  CHAR_BITS(1, 1, 1,
            1, 0, 0,
            1, 1, 1,
            1, 0, 0,
            1, 1, 1),
  CHAR_BITS(1, 1, 1,
            1, 0, 0,
            1, 1, 1,
            1, 0, 0,
            1, 0, 0)
};
























static const int MINIFONT_WIDTH = 3;

static const int MINIFONT_HEIGHT = 5;




static const int HEX_CHAR_GAP = 1;





static const int BOX_HORIZONTAL_INSET = 1;

static const int BOX_BORDER_WIDTH = 1;




static const Float BOX_BORDER_OPACITY = 0.5;







#ifndef MOZ_GFX_OPTIMIZE_MOBILE
static void
DrawHexChar(uint32_t aDigit, const Point& aPt, DrawTarget& aDrawTarget,
            const Pattern &aPattern)
{
    
    
    
    RefPtr<PathBuilder> builder = aDrawTarget.CreatePathBuilder();
    uint32_t glyphBits = glyphMicroFont[aDigit];
    for (int y = 0; y < MINIFONT_HEIGHT; ++y) {
        for (int x = 0; x < MINIFONT_WIDTH; ++x) {
            if (glyphBits & 1) {
                Rect r(aPt.x + x, aPt.y + y, 1, 1);
                MaybeSnapToDevicePixels(r, aDrawTarget, true);
                builder->MoveTo(r.TopLeft());
                builder->LineTo(r.TopRight());
                builder->LineTo(r.BottomRight());
                builder->LineTo(r.BottomLeft());
                builder->Close();
            }
            glyphBits >>= 1;
        }
    }
    RefPtr<Path> path = builder->Finish();
    aDrawTarget.Fill(path, aPattern);
}
#endif 

void
gfxFontMissingGlyphs::DrawMissingGlyph(uint32_t aChar,
                                       const Rect& aRect,
                                       DrawTarget& aDrawTarget,
                                       const Pattern& aPattern,
                                       uint32_t aAppUnitsPerDevPixel)
{
    
    
    ColorPattern color = aPattern.GetType() == PatternType::COLOR ?
        static_cast<const ColorPattern&>(aPattern) :
        ColorPattern(ToDeviceColor(Color(0.f, 0.f, 0.f, 1.f)));

    
    
    
    Float halfBorderWidth = BOX_BORDER_WIDTH / 2.0;
    Float borderLeft = aRect.X() + BOX_HORIZONTAL_INSET + halfBorderWidth;
    Float borderRight = aRect.XMost() - BOX_HORIZONTAL_INSET - halfBorderWidth;
    Rect borderStrokeRect(borderLeft, aRect.Y() + halfBorderWidth,
                          borderRight - borderLeft,
                          aRect.Height() - 2.0 * halfBorderWidth);
    if (!borderStrokeRect.IsEmpty()) {
        ColorPattern adjustedColor = color;
        color.mColor.a *= BOX_BORDER_OPACITY;
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
        aDrawTarget.FillRect(borderStrokeRect, adjustedColor);
#else
        StrokeOptions strokeOptions(BOX_BORDER_WIDTH);
        aDrawTarget.StrokeRect(borderStrokeRect, adjustedColor, strokeOptions);
#endif
    }

#ifndef MOZ_GFX_OPTIMIZE_MOBILE
    Point center = aRect.Center();
    Float halfGap = HEX_CHAR_GAP / 2.f;
    Float top = -(MINIFONT_HEIGHT + halfGap);
    
    
    int32_t devPixelsPerCSSPx =
        std::max<int32_t>(1, nsDeviceContext::AppUnitsPerCSSPixel() /
                             aAppUnitsPerDevPixel);
    AutoRestoreTransform autoRestoreTransform(&aDrawTarget);
    aDrawTarget.SetTransform(
      aDrawTarget.GetTransform().PreTranslate(center).
                                 PreScale(devPixelsPerCSSPx,
                                          devPixelsPerCSSPx));
    if (aChar < 0x10000) {
        if (aRect.Width() >= 2 * (MINIFONT_WIDTH + HEX_CHAR_GAP) &&
            aRect.Height() >= 2 * MINIFONT_HEIGHT + HEX_CHAR_GAP) {
            
            Float left = -(MINIFONT_WIDTH + halfGap);
            DrawHexChar((aChar >> 12) & 0xF,
                        Point(left, top), aDrawTarget, color);
            DrawHexChar((aChar >> 8) & 0xF,
                        Point(halfGap, top), aDrawTarget, color);
            DrawHexChar((aChar >> 4) & 0xF,
                        Point(left, halfGap), aDrawTarget, color);
            DrawHexChar(aChar & 0xF,
                        Point(halfGap, halfGap), aDrawTarget, color);
        }
    } else {
        if (aRect.Width() >= 3 * (MINIFONT_WIDTH + HEX_CHAR_GAP) &&
            aRect.Height() >= 2 * MINIFONT_HEIGHT + HEX_CHAR_GAP) {
            
            Float first = -(MINIFONT_WIDTH * 1.5 + HEX_CHAR_GAP);
            Float second = -(MINIFONT_WIDTH / 2.0);
            Float third = (MINIFONT_WIDTH / 2.0 + HEX_CHAR_GAP);
            DrawHexChar((aChar >> 20) & 0xF,
                        Point(first, top), aDrawTarget, color);
            DrawHexChar((aChar >> 16) & 0xF,
                        Point(second, top), aDrawTarget, color);
            DrawHexChar((aChar >> 12) & 0xF,
                        Point(third, top), aDrawTarget, color);
            DrawHexChar((aChar >> 8) & 0xF,
                        Point(first, halfGap), aDrawTarget, color);
            DrawHexChar((aChar >> 4) & 0xF,
                        Point(second, halfGap), aDrawTarget, color);
            DrawHexChar(aChar & 0xF,
                        Point(third, halfGap), aDrawTarget, color);
        }
    }
#endif
}

Float
gfxFontMissingGlyphs::GetDesiredMinWidth(uint32_t aChar,
                                         uint32_t aAppUnitsPerDevPixel)
{




    Float width = BOX_HORIZONTAL_INSET + BOX_BORDER_WIDTH + HEX_CHAR_GAP +
        MINIFONT_WIDTH + HEX_CHAR_GAP + MINIFONT_WIDTH +
         ((aChar < 0x10000) ? 0 : HEX_CHAR_GAP + MINIFONT_WIDTH) +
        HEX_CHAR_GAP + BOX_BORDER_WIDTH + BOX_HORIZONTAL_INSET;
    
    
    width *= Float(nsDeviceContext::AppUnitsPerCSSPixel()) / aAppUnitsPerDevPixel;
    return width;
}
