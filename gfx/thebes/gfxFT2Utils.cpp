




#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"
#include "mozilla/Likely.h"
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H
#include <algorithm>

#ifdef HAVE_FONTCONFIG_FCFREETYPE_H
#include <fontconfig/fcfreetype.h>
#endif

#include "prlink.h"


static inline FT_Long
ScaleRoundDesignUnits(FT_Short aDesignMetric, FT_Fixed aScale)
{
    FT_Long fixed26dot6 = FT_MulFix(aDesignMetric, aScale);
    return ROUND_26_6_TO_INT(fixed26dot6);
}









static void
SnapLineToPixels(gfxFloat& aOffset, gfxFloat& aSize)
{
    gfxFloat snappedSize = std::max(floor(aSize + 0.5), 1.0);
    
    gfxFloat offset = aOffset - 0.5 * (aSize - snappedSize);
    
    aOffset = floor(offset + 0.5);
    aSize = snappedSize;
}

void
gfxFT2LockedFace::GetMetrics(gfxFont::Metrics* aMetrics,
                             uint32_t* aSpaceGlyph)
{
    NS_PRECONDITION(aMetrics != nullptr, "aMetrics must not be NULL");
    NS_PRECONDITION(aSpaceGlyph != nullptr, "aSpaceGlyph must not be NULL");

    if (MOZ_UNLIKELY(!mFace)) {
        
        
        const gfxFloat emHeight = mGfxFont->GetStyle()->size;
        aMetrics->emHeight = emHeight;
        aMetrics->maxAscent = aMetrics->emAscent = 0.8 * emHeight;
        aMetrics->maxDescent = aMetrics->emDescent = 0.2 * emHeight;
        aMetrics->maxHeight = emHeight;
        aMetrics->internalLeading = 0.0;
        aMetrics->externalLeading = 0.2 * emHeight;
        const gfxFloat spaceWidth = 0.5 * emHeight;
        aMetrics->spaceWidth = spaceWidth;
        aMetrics->maxAdvance = spaceWidth;
        aMetrics->aveCharWidth = spaceWidth;
        aMetrics->zeroOrAveCharWidth = spaceWidth;
        const gfxFloat xHeight = 0.5 * emHeight;
        aMetrics->xHeight = xHeight;
        const gfxFloat underlineSize = emHeight / 14.0;
        aMetrics->underlineSize = underlineSize;
        aMetrics->underlineOffset = -underlineSize;
        aMetrics->strikeoutOffset = 0.25 * emHeight;
        aMetrics->strikeoutSize = underlineSize;

        *aSpaceGlyph = 0;
        return;
    }

    const FT_Size_Metrics& ftMetrics = mFace->size->metrics;

    gfxFloat emHeight;
    
    
    gfxFloat yScale = 0.0;
    if (FT_IS_SCALABLE(mFace)) {
        
        
        
        
        
        
        
        yScale = FLOAT_FROM_26_6(FLOAT_FROM_16_16(ftMetrics.y_scale));
        emHeight = mFace->units_per_EM * yScale;
    } else { 
        emHeight = ftMetrics.y_ppem;
        
        
        
        
        const TT_Header* head =
            static_cast<TT_Header*>(FT_Get_Sfnt_Table(mFace, ft_sfnt_head));
        if (head) {
            gfxFloat emUnit = head->Units_Per_EM;
            yScale = emHeight / emUnit;
        }
    }

    TT_OS2 *os2 =
        static_cast<TT_OS2*>(FT_Get_Sfnt_Table(mFace, ft_sfnt_os2));

    aMetrics->maxAscent = FLOAT_FROM_26_6(ftMetrics.ascender);
    aMetrics->maxDescent = -FLOAT_FROM_26_6(ftMetrics.descender);
    aMetrics->maxAdvance = FLOAT_FROM_26_6(ftMetrics.max_advance);

    gfxFloat lineHeight;
    if (os2 && os2->sTypoAscender && yScale > 0.0) {
        aMetrics->emAscent = os2->sTypoAscender * yScale;
        aMetrics->emDescent = -os2->sTypoDescender * yScale;
        FT_Short typoHeight =
            os2->sTypoAscender - os2->sTypoDescender + os2->sTypoLineGap;
        lineHeight = typoHeight * yScale;

        
        
        
        const uint16_t kUseTypoMetricsMask = 1 << 7;
        FT_ULong length = 0;
        if ((os2->fsSelection & kUseTypoMetricsMask) ||
            0 == FT_Load_Sfnt_Table(mFace, FT_MAKE_TAG('M','A','T','H'),
                                    0, nullptr, &length)) {
            aMetrics->maxAscent = NS_round(aMetrics->emAscent);
            aMetrics->maxDescent = NS_round(aMetrics->emDescent);
        } else {
            
            
            
            
            aMetrics->maxAscent =
                std::max(aMetrics->maxAscent, NS_round(aMetrics->emAscent));
            aMetrics->maxDescent =
                std::max(aMetrics->maxDescent, NS_round(aMetrics->emDescent));
        }
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
        if (os2 && os2->sxHeight && yScale > 0.0) {
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
            std::max(aMetrics->aveCharWidth, avgCharWidth);
    }
    aMetrics->aveCharWidth =
        std::max(aMetrics->aveCharWidth, aMetrics->zeroOrAveCharWidth);
    if (aMetrics->aveCharWidth == 0.0) {
        aMetrics->aveCharWidth = aMetrics->spaceWidth;
    }
    if (aMetrics->zeroOrAveCharWidth == 0.0) {
        aMetrics->zeroOrAveCharWidth = aMetrics->aveCharWidth;
    }
    
    aMetrics->maxAdvance =
        std::max(aMetrics->maxAdvance, aMetrics->aveCharWidth);

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (mFace->underline_position && mFace->underline_thickness && yScale > 0.0) {
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

    if (os2 && os2->yStrikeoutSize && os2->yStrikeoutPosition && yScale > 0.0) {
        aMetrics->strikeoutSize = os2->yStrikeoutSize * yScale;
        aMetrics->strikeoutOffset = os2->yStrikeoutPosition * yScale;
    } else { 
        aMetrics->strikeoutSize = aMetrics->underlineSize;
        
        aMetrics->strikeoutOffset = emHeight * 409.0 / 2048.0
            + 0.5 * aMetrics->strikeoutSize;
    }
    SnapLineToPixels(aMetrics->strikeoutOffset, aMetrics->strikeoutSize);

    aMetrics->maxHeight = aMetrics->maxAscent + aMetrics->maxDescent;

    
    
    
    
    
    
    aMetrics->emHeight = floor(emHeight + 0.5);

    
    
    aMetrics->internalLeading =
        floor(aMetrics->maxHeight - aMetrics->emHeight + 0.5);

    
    
    lineHeight = floor(std::max(lineHeight, aMetrics->maxHeight) + 0.5);
    aMetrics->externalLeading =
        lineHeight - aMetrics->internalLeading - aMetrics->emHeight;

    
    gfxFloat sum = aMetrics->emAscent + aMetrics->emDescent;
    aMetrics->emAscent = sum > 0.0 ?
        aMetrics->emAscent * aMetrics->emHeight / sum : 0.0;
    aMetrics->emDescent = aMetrics->emHeight - aMetrics->emAscent;
}

uint32_t
gfxFT2LockedFace::GetGlyph(uint32_t aCharCode)
{
    if (MOZ_UNLIKELY(!mFace))
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

typedef FT_UInt (*GetCharVariantFunction)(FT_Face  face,
                                          FT_ULong charcode,
                                          FT_ULong variantSelector);

uint32_t
gfxFT2LockedFace::GetUVSGlyph(uint32_t aCharCode, uint32_t aVariantSelector)
{
    NS_PRECONDITION(aVariantSelector, "aVariantSelector should not be NULL");

    if (MOZ_UNLIKELY(!mFace))
        return 0;

    
    static CharVariantFunction sGetCharVariantPtr = FindCharVariantFunction();
    if (!sGetCharVariantPtr)
        return 0;

#ifdef HAVE_FONTCONFIG_FCFREETYPE_H
    
    
    if (!mFace->charmap || mFace->charmap->encoding != FT_ENCODING_UNICODE) {
        FT_Select_Charmap(mFace, FT_ENCODING_UNICODE);
    }
#endif

    return (*sGetCharVariantPtr)(mFace, aCharCode, aVariantSelector);
}

uint32_t
gfxFT2LockedFace::GetCharExtents(char aChar, cairo_text_extents_t* aExtents)
{
    NS_PRECONDITION(aExtents != nullptr, "aExtents must not be NULL");

    if (!mFace)
        return 0;

    FT_UInt gid = mGfxFont->GetGlyph(aChar);
    if (gid) {
        mGfxFont->GetGlyphExtents(gid, aExtents);
    }

    return gid;
}

gfxFT2LockedFace::CharVariantFunction
gfxFT2LockedFace::FindCharVariantFunction()
{
    
    PRLibrary *lib = nullptr;
    CharVariantFunction function =
        reinterpret_cast<CharVariantFunction>
        (PR_FindFunctionSymbolAndLibrary("FT_Face_GetCharVariantIndex", &lib));
    if (!lib) {
        return nullptr;
    }

    FT_Int major;
    FT_Int minor;
    FT_Int patch;
    FT_Library_Version(mFace->glyph->library, &major, &minor, &patch);

    
    
    
    if (major == 2 && minor == 4 && patch < 4 &&
        PR_FindFunctionSymbol(lib, "FT_Alloc")) {
        function = nullptr;
    }

    
    
    PR_UnloadLibrary(lib);

    return function;
}
