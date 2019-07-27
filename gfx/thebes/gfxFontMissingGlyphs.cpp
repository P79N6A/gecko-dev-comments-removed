




#include "gfxFontMissingGlyphs.h"
#include "nsDeviceContext.h"
#include "gfxContext.h"
#include "gfxColor.h"

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




static const gfxFloat BOX_BORDER_OPACITY = 0.5;







#ifndef MOZ_GFX_OPTIMIZE_MOBILE
static void
DrawHexChar(gfxContext *aContext, const gfxPoint& aPt, uint32_t aDigit)
{
    aContext->NewPath();
    uint32_t glyphBits = glyphMicroFont[aDigit];
    int x, y;
    for (y = 0; y < MINIFONT_HEIGHT; ++y) {
        for (x = 0; x < MINIFONT_WIDTH; ++x) {
            if (glyphBits & 1) {
                aContext->Rectangle(gfxRect(x, y, 1, 1) + aPt, true);
            }
            glyphBits >>= 1;
        }
    }
    aContext->Fill();
}
#endif 

void
gfxFontMissingGlyphs::DrawMissingGlyph(gfxContext    *aContext,
                                       const gfxRect& aRect,
                                       uint32_t       aChar,
                                       uint32_t       aAppUnitsPerDevPixel)
{
    aContext->Save();

    gfxRGBA currentColor;
    if (!aContext->GetDeviceColor(currentColor)) {
        
        
        currentColor = gfxRGBA(0,0,0,1);
    }

    
    
    
    gfxFloat halfBorderWidth = BOX_BORDER_WIDTH / 2.0;
    gfxFloat borderLeft = aRect.X() + BOX_HORIZONTAL_INSET + halfBorderWidth;
    gfxFloat borderRight = aRect.XMost() - BOX_HORIZONTAL_INSET - halfBorderWidth;
    gfxRect borderStrokeRect(borderLeft, aRect.Y() + halfBorderWidth,
                             borderRight - borderLeft,
                             aRect.Height() - 2.0 * halfBorderWidth);
    if (!borderStrokeRect.IsEmpty()) {
        aContext->SetLineWidth(BOX_BORDER_WIDTH);
        aContext->SetDash(gfxContext::gfxLineSolid);
        aContext->SetLineCap(gfxContext::LINE_CAP_SQUARE);
        aContext->SetLineJoin(gfxContext::LINE_JOIN_MITER);
        gfxRGBA color = currentColor;
        color.a *= BOX_BORDER_OPACITY;
        aContext->SetDeviceColor(color);
        aContext->NewPath();
        aContext->Rectangle(borderStrokeRect);

#ifdef MOZ_GFX_OPTIMIZE_MOBILE
        aContext->Fill();
#else
        aContext->Stroke();
#endif
    }

#ifndef MOZ_GFX_OPTIMIZE_MOBILE
    gfxPoint center(aRect.X() + aRect.Width() / 2,
                    aRect.Y() + aRect.Height() / 2);
    gfxFloat halfGap = HEX_CHAR_GAP / 2.0;
    gfxFloat top = -(MINIFONT_HEIGHT + halfGap);
    aContext->SetDeviceColor(currentColor);
    
    
    int32_t scale =
        std::max<int32_t>(1, nsDeviceContext::AppUnitsPerCSSPixel() /
                             aAppUnitsPerDevPixel);
    aContext->SetMatrix(
      aContext->CurrentMatrix().Translate(center).Scale(scale, scale));
    if (aChar < 0x10000) {
        if (aRect.Width() >= 2 * (MINIFONT_WIDTH + HEX_CHAR_GAP) &&
            aRect.Height() >= 2 * MINIFONT_HEIGHT + HEX_CHAR_GAP) {
            
            gfxFloat left = -(MINIFONT_WIDTH + halfGap);
            DrawHexChar(aContext,
                        gfxPoint(left, top), (aChar >> 12) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(halfGap, top), (aChar >> 8) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(left, halfGap), (aChar >> 4) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(halfGap, halfGap), aChar & 0xF);
        }
    } else {
        if (aRect.Width() >= 3 * (MINIFONT_WIDTH + HEX_CHAR_GAP) &&
            aRect.Height() >= 2 * MINIFONT_HEIGHT + HEX_CHAR_GAP) {
            
            gfxFloat first = -(MINIFONT_WIDTH * 1.5 + HEX_CHAR_GAP);
            gfxFloat second = -(MINIFONT_WIDTH / 2.0);
            gfxFloat third = (MINIFONT_WIDTH / 2.0 + HEX_CHAR_GAP);
            DrawHexChar(aContext,
                        gfxPoint(first, top), (aChar >> 20) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(second, top), (aChar >> 16) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(third, top), (aChar >> 12) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(first, halfGap), (aChar >> 8) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(second, halfGap), (aChar >> 4) & 0xF);
            DrawHexChar(aContext,
                        gfxPoint(third, halfGap), aChar & 0xF);
        }
    }
#endif

    aContext->Restore();
}

gfxFloat
gfxFontMissingGlyphs::GetDesiredMinWidth(uint32_t aChar,
                                         uint32_t aAppUnitsPerDevPixel)
{




    gfxFloat width = BOX_HORIZONTAL_INSET + BOX_BORDER_WIDTH + HEX_CHAR_GAP +
        MINIFONT_WIDTH + HEX_CHAR_GAP + MINIFONT_WIDTH +
         ((aChar < 0x10000) ? 0 : HEX_CHAR_GAP + MINIFONT_WIDTH) +
        HEX_CHAR_GAP + BOX_BORDER_WIDTH + BOX_HORIZONTAL_INSET;
    
    
    width *= gfxFloat(nsDeviceContext::AppUnitsPerCSSPixel()) / aAppUnitsPerDevPixel;
    return width;
}
