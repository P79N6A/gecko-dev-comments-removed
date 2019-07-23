









































#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H

#ifdef HAVE_FONTCONFIG_FCFREETYPE_H
#include <fontconfig/fcfreetype.h>
#endif


static inline FT_Long
ScaleRoundDesignUnits(FT_Short aDesignMetric, FT_Fixed aScale)
{
    FT_Long fixed26dot6 = FT_MulFix(aDesignMetric, aScale);
    return ROUND_26_6_TO_INT(fixed26dot6);
}









static void
SnapLineToPixels(gfxFloat& aOffset, gfxFloat& aSize)
{
    gfxFloat snappedSize = PR_MAX(NS_floor(aSize + 0.5), 1.0);
    
    gfxFloat offset = aOffset - 0.5 * (aSize - snappedSize);
    
    aOffset = NS_floor(offset + 0.5);
    aSize = snappedSize;
}

void
gfxFT2LockedFace::GetMetrics(gfxFont::Metrics* aMetrics,
                             PRUint32* aSpaceGlyph)
{
    NS_PRECONDITION(aMetrics != NULL, "aMetrics must not be NULL");
    NS_PRECONDITION(aSpaceGlyph != NULL, "aSpaceGlyph must not be NULL");

    if (NS_UNLIKELY(!mFace)) {
        
        
        aMetrics->emHeight = mGfxFont->GetStyle()->size;
        aMetrics->emAscent = 0.8 * aMetrics->emHeight;
        aMetrics->emDescent = 0.2 * aMetrics->emHeight;
        aMetrics->maxAscent = aMetrics->emAscent;
        aMetrics->maxDescent = aMetrics->maxDescent;
        aMetrics->maxHeight = aMetrics->emHeight;
        aMetrics->internalLeading = 0.0;
        aMetrics->externalLeading = 0.2 * aMetrics->emHeight;
        aSpaceGlyph = 0;
        aMetrics->spaceWidth = 0.5 * aMetrics->emHeight;
        aMetrics->maxAdvance = aMetrics->spaceWidth;
        aMetrics->aveCharWidth = aMetrics->spaceWidth;
        aMetrics->zeroOrAveCharWidth = aMetrics->spaceWidth;
        aMetrics->xHeight = 0.5 * aMetrics->emHeight;
        aMetrics->underlineSize = aMetrics->emHeight / 14.0;
        aMetrics->underlineOffset = -aMetrics->underlineSize;
        aMetrics->strikeoutOffset = 0.25 * aMetrics->emHeight;
        aMetrics->strikeoutSize = aMetrics->underlineSize;
        aMetrics->superscriptOffset = aMetrics->xHeight;
        aMetrics->subscriptOffset = aMetrics->xHeight;

        return;
    }

    const FT_Size_Metrics& ftMetrics = mFace->size->metrics;

    gfxFloat emHeight;
    
    gfxFloat yScale;
    if (FT_IS_SCALABLE(mFace)) {
        
        
        
        
        
        
        
        yScale = FLOAT_FROM_26_6(FLOAT_FROM_16_16(ftMetrics.y_scale));
        emHeight = mFace->units_per_EM * yScale;
    } else { 
        
        
        gfxFloat emUnit = mFace->units_per_EM;
        emHeight = ftMetrics.y_ppem;
        yScale = emHeight / emUnit;
    }

    TT_OS2 *os2 =
        static_cast<TT_OS2*>(FT_Get_Sfnt_Table(mFace, ft_sfnt_os2));

    aMetrics->maxAscent = FLOAT_FROM_26_6(ftMetrics.ascender);
    aMetrics->maxDescent = -FLOAT_FROM_26_6(ftMetrics.descender);
    aMetrics->maxAdvance = FLOAT_FROM_26_6(ftMetrics.max_advance);

    gfxFloat lineHeight;
    if (os2 && os2->sTypoAscender) {
        aMetrics->emAscent = os2->sTypoAscender * yScale;
        aMetrics->emDescent = -os2->sTypoDescender * yScale;
        FT_Short typoHeight =
            os2->sTypoAscender - os2->sTypoDescender + os2->sTypoLineGap;
        lineHeight = typoHeight * yScale;

        
        
        if (aMetrics->emAscent > aMetrics->maxAscent)
            aMetrics->maxAscent = aMetrics->emAscent;
        if (aMetrics->emDescent > aMetrics->maxDescent)
            aMetrics->maxDescent = aMetrics->emDescent;
    } else {
        aMetrics->emAscent = aMetrics->maxAscent;
        aMetrics->emDescent = aMetrics->maxDescent;
        lineHeight = FLOAT_FROM_26_6(ftMetrics.height);
    }

    cairo_text_extents_t extents;
    *aSpaceGlyph = GetCharExtents(' ', &extents);
    if (*aSpaceGlyph) {
        aMetrics->spaceWidth = extents.x_advance;
    } else {
        aMetrics->spaceWidth = aMetrics->maxAdvance; 
    }

    aMetrics->zeroOrAveCharWidth = 0.0;
    if (GetCharExtents('0', &extents)) {
        aMetrics->zeroOrAveCharWidth = extents.x_advance;
    }

    
    
    
    
    if (GetCharExtents('x', &extents) && extents.y_bearing < 0.0) {
        aMetrics->xHeight = -extents.y_bearing;
        aMetrics->aveCharWidth = extents.x_advance;
    } else {
        if (os2 && os2->sxHeight) {
            aMetrics->xHeight = os2->sxHeight * yScale;
        } else {
            
            
            
            aMetrics->xHeight = 0.5 * emHeight;
        }
        aMetrics->aveCharWidth = 0.0; 
    }
    
    
    if (os2 && os2->xAvgCharWidth) {
        
        
        gfxFloat avgCharWidth =
            ScaleRoundDesignUnits(os2->xAvgCharWidth, ftMetrics.x_scale);
        aMetrics->aveCharWidth =
            PR_MAX(aMetrics->aveCharWidth, avgCharWidth);
    }
    aMetrics->aveCharWidth =
        PR_MAX(aMetrics->aveCharWidth, aMetrics->zeroOrAveCharWidth);
    if (aMetrics->aveCharWidth == 0.0) {
        aMetrics->aveCharWidth = aMetrics->spaceWidth;
    }
    if (aMetrics->zeroOrAveCharWidth == 0.0) {
        aMetrics->zeroOrAveCharWidth = aMetrics->aveCharWidth;
    }
    
    aMetrics->maxAdvance =
        PR_MAX(aMetrics->maxAdvance, aMetrics->aveCharWidth);

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (mFace->underline_position && mFace->underline_thickness) {
        aMetrics->underlineSize = mFace->underline_thickness * yScale;
        TT_Postscript *post = static_cast<TT_Postscript*>
            (FT_Get_Sfnt_Table(mFace, ft_sfnt_post));
        if (post && post->underlinePosition) {
            aMetrics->underlineOffset = post->underlinePosition * yScale;
        } else {
            aMetrics->underlineOffset = mFace->underline_position * yScale
                + 0.5 * aMetrics->underlineSize;
        }
    } else { 
        
        aMetrics->underlineSize = emHeight / 14.0;
        aMetrics->underlineOffset = -aMetrics->underlineSize;
    }

    if (os2 && os2->yStrikeoutSize && os2->yStrikeoutPosition) {
        aMetrics->strikeoutSize = os2->yStrikeoutSize * yScale;
        aMetrics->strikeoutOffset = os2->yStrikeoutPosition * yScale;
    } else { 
        aMetrics->strikeoutSize = aMetrics->underlineSize;
        
        aMetrics->strikeoutOffset = emHeight * 409.0 / 2048.0
            + 0.5 * aMetrics->strikeoutSize;
    }
    SnapLineToPixels(aMetrics->strikeoutOffset, aMetrics->strikeoutSize);

    if (os2 && os2->ySuperscriptYOffset) {
        gfxFloat val = ScaleRoundDesignUnits(os2->ySuperscriptYOffset,
                                             ftMetrics.y_scale);
        aMetrics->superscriptOffset = PR_MAX(1.0, val);
    } else {
        aMetrics->superscriptOffset = aMetrics->xHeight;
    }
    
    if (os2 && os2->ySubscriptYOffset) {
        gfxFloat val = ScaleRoundDesignUnits(os2->ySubscriptYOffset,
                                             ftMetrics.y_scale);
        
        val = fabs(val);
        aMetrics->subscriptOffset = PR_MAX(1.0, val);
    } else {
        aMetrics->subscriptOffset = aMetrics->xHeight;
    }

    aMetrics->maxHeight = aMetrics->maxAscent + aMetrics->maxDescent;

    
    
    
    
    
    
    aMetrics->emHeight = NS_floor(emHeight + 0.5);

    
    
    aMetrics->internalLeading =
        NS_floor(aMetrics->maxHeight - aMetrics->emHeight + 0.5);

    
    
    lineHeight = NS_floor(PR_MAX(lineHeight, aMetrics->maxHeight) + 0.5);
    aMetrics->externalLeading =
        lineHeight - aMetrics->internalLeading - aMetrics->emHeight;

    
    gfxFloat sum = aMetrics->emAscent + aMetrics->emDescent;
    aMetrics->emAscent = sum > 0.0 ?
        aMetrics->emAscent * aMetrics->emHeight / sum : 0.0;
    aMetrics->emDescent = aMetrics->emHeight - aMetrics->emAscent;
}

PRUint32
gfxFT2LockedFace::GetGlyph(PRUint32 aCharCode)
{
    if (NS_UNLIKELY(!mFace))
        return 0;

#ifdef HAVE_FONTCONFIG_FCFREETYPE_H
    
    
    
    
    
    
    
    if (!mFace->charmap || mFace->charmap->encoding != FT_ENCODING_UNICODE) {
        FT_Select_Charmap(mFace, FT_ENCODING_UNICODE);
    }

    return FcFreeTypeCharIndex(mFace, aCharCode);
#else
    return FT_Get_Char_Index(mFace, aCharCode);
#endif
}

PRUint32
gfxFT2LockedFace::GetCharExtents(char aChar, cairo_text_extents_t* aExtents)
{
    NS_PRECONDITION(aExtents != NULL, "aExtents must not be NULL");

    if (!mFace)
        return 0;

    FT_UInt gid = mGfxFont->GetGlyph(aChar);
    if (gid) {
        mGfxFont->GetGlyphExtents(gid, aExtents);
    }

    return gid;
}
