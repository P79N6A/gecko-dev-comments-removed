




































#include "gfxFontMissingGlyphs.h"

#define CHAR_BITS(b00, b01, b02, b10, b11, b12, b20, b21, b22, b30, b31, b32, b40, b41, b42) \
  ((b00 << 0) | (b01 << 1) | (b02 << 2) | (b10 << 3) | (b11 << 4) | (b12 << 5) | \
   (b20 << 6) | (b21 << 7) | (b22 << 8) | (b30 << 9) | (b31 << 10) | (b32 << 11) | \
   (b40 << 12) | (b41 << 13) | (b42 << 14))

static const PRUint16 glyphMicroFont[16] = {
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
  CHAR_BITS(1, 1, 1,
            1, 0, 1,
            1, 1, 1,
            1, 0, 1,
            1, 1, 1),
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




static const int MIN_DESIRED_WIDTH =
  BOX_HORIZONTAL_INSET + BOX_BORDER_WIDTH + HEX_CHAR_GAP +
  MINIFONT_WIDTH + HEX_CHAR_GAP + MINIFONT_WIDTH +
  HEX_CHAR_GAP + BOX_BORDER_WIDTH + BOX_HORIZONTAL_INSET;








static void
DrawHexChar(gfxContext *aContext, const gfxPoint& aPt, PRUint32 aDigit)
{
    aContext->NewPath();
    PRUint32 glyphBits = glyphMicroFont[aDigit];
    int x, y;
    for (y = 0; y < MINIFONT_HEIGHT; ++y) {
        for (x = 0; x < MINIFONT_WIDTH; ++x) {
            if (glyphBits & 1) {
                aContext->Rectangle(gfxRect(x, y, 1, 1) + aPt, PR_TRUE);
            }
            glyphBits >>= 1;
        }
    }
    aContext->Fill();
}

void
gfxFontMissingGlyphs::DrawMissingGlyph(gfxContext *aContext, const gfxRect& aRect,
                                       PRUnichar aChar)
{
    aContext->Save();

    gfxRGBA currentColor;
    if (!aContext->GetColor(currentColor)) {
        
        
        currentColor = gfxRGBA(0,0,0,1);
    }

    
    
    
    gfxFloat halfBorderWidth = BOX_BORDER_WIDTH/2.0;
    gfxFloat borderLeft = aRect.X() + BOX_HORIZONTAL_INSET + halfBorderWidth;
    gfxFloat borderRight = aRect.XMost() - BOX_HORIZONTAL_INSET - halfBorderWidth;
    gfxRect borderStrokeRect(borderLeft, aRect.Y() + halfBorderWidth,
                             borderRight - borderLeft, aRect.Height() - 2*halfBorderWidth);
    if (!borderStrokeRect.IsEmpty()) {
        aContext->SetLineWidth(BOX_BORDER_WIDTH);
        aContext->SetDash(gfxContext::gfxLineSolid);
        aContext->SetLineCap(gfxContext::LINE_CAP_SQUARE);
        aContext->SetLineJoin(gfxContext::LINE_JOIN_MITER);
        gfxRGBA color = currentColor;
        color.a *= BOX_BORDER_OPACITY;
        aContext->SetColor(color);
        aContext->NewPath();
        aContext->Rectangle(borderStrokeRect);
        aContext->Stroke();
    }

    if (aRect.Width() >= 2*MINIFONT_WIDTH + HEX_CHAR_GAP &&
        aRect.Height() >= 2*MINIFONT_HEIGHT + HEX_CHAR_GAP) {
        aContext->SetColor(currentColor);
        gfxPoint center(aRect.X() + aRect.Width()/2,
                        aRect.Y() + aRect.Height()/2);
        gfxFloat halfGap = HEX_CHAR_GAP/2.0;
        gfxFloat left = -(MINIFONT_WIDTH + halfGap);
        gfxFloat top = -(MINIFONT_HEIGHT + halfGap);
        DrawHexChar(aContext,
                    center + gfxPoint(left, top), (aChar >> 12) & 0xF);
        DrawHexChar(aContext,
                    center + gfxPoint(halfGap, top), (aChar >> 8) & 0xF);
        DrawHexChar(aContext,
                    center + gfxPoint(left, halfGap), (aChar >> 4) & 0xF);
        DrawHexChar(aContext,
                    center + gfxPoint(halfGap, halfGap), aChar & 0xF);
    }

    aContext->Restore();
}

gfxFloat
gfxFontMissingGlyphs::GetDesiredMinWidth()
{
    return MIN_DESIRED_WIDTH;
}
