







































#ifndef __LP64__ 

#include "prtypes.h"
#include "prmem.h"
#include "nsString.h"
#include "nsBidiUtils.h"

#include "gfxTypes.h"

#include "nsPromiseFlatString.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxPlatformMac.h"
#include "gfxAtsuiFonts.h"

#include "gfxFontTest.h"
#include "gfxFontUtils.h"

#include "cairo-quartz.h"

#include "gfxQuartzSurface.h"
#include "gfxMacPlatformFontList.h"
#include "gfxUserFontSet.h"

#include "nsUnicodeRange.h"




#ifdef DUMP_TEXT_RUNS
static PRLogModuleInfo *gAtsuiTextRunLog = PR_NewLogModule("atsuiTextRun");
#endif

#define ROUND(x) (floor((x) + 0.5))


#ifdef FloatToFixed
#undef FloatToFixed
#define FloatToFixed(a)     ((Fixed)((float)(a) * fixed1))
#endif


#if 0
OSStatus ATSUGetStyleGroup(ATSUStyle style, void **styleGroup);
OSStatus ATSUDisposeStyleGroup(void *styleGroup);
OSStatus ATSUConvertCharToGlyphs(void *styleGroup,
                                 PRunichar *buffer
                                 unsigned int bufferLength,
                                 void *glyphVector);
OSStatus ATSInitializeGlyphVector(int size, void *glyphVectorPtr);
OSStatus ATSClearGlyphVector(void *glyphVectorPtr);
#endif

gfxAtsuiFont::gfxAtsuiFont(MacOSFontEntry *aFontEntry,
                           const gfxFontStyle *fontStyle, PRBool aNeedsBold)
    : gfxFont(aFontEntry, fontStyle),
      mFontStyle(fontStyle), mATSUStyle(nsnull),
      mHasMirroring(PR_FALSE), mHasMirroringLookedUp(PR_FALSE),
      mFontFace(nsnull), mScaledFont(nsnull), mAdjustedSize(0.0f)
{
    ATSFontRef fontRef = aFontEntry->GetFontRef();
    ATSUFontID fontID = FMGetFontFromATSFontRef(fontRef);

    
    PRInt8 baseWeight, weightDistance;
    mFontStyle->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    PRUint16 targetWeight = (baseWeight * 100) + (weightDistance * 100);

    
    
    
    
    if (!aFontEntry->IsBold()
        && ((weightDistance == 0 && targetWeight >= 600) || (weightDistance > 0 && aNeedsBold))) 
    {
        mSyntheticBoldOffset = 1;  
    }

    InitMetrics(fontID, fontRef);
    if (!mIsValid) {
        return;
    }

    mFontFace = cairo_quartz_font_face_create_for_atsu_font_id(fontID);

    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

    
    PRBool needsOblique = (!aFontEntry->IsItalic() && (mFontStyle->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)));

    if (needsOblique) {
        double skewfactor = (needsOblique ? Fix2X(kATSItalicQDSkew) : 0);

        cairo_matrix_t style;
        cairo_matrix_init(&style,
                          1,                
                          0,                
                          -1 * skewfactor,   
                          1,                
                          0,                
                          0);               
        cairo_matrix_multiply(&sizeMatrix, &sizeMatrix, &style);
    }

    cairo_font_options_t *fontOptions = cairo_font_options_create();

    
    if (mAdjustedSize <= (float) gfxPlatformMac::GetPlatform()->GetAntiAliasingThreshold()) {
        cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE); 
        
    }

    mScaledFont = cairo_scaled_font_create(mFontFace, &sizeMatrix, &ctm, fontOptions);
    cairo_font_options_destroy(fontOptions);

    cairo_status_t cairoerr = cairo_scaled_font_status(mScaledFont);
    if (cairoerr != CAIRO_STATUS_SUCCESS) {
        mIsValid = PR_FALSE;

#ifdef DEBUG        
        char warnBuf[1024];
        sprintf(warnBuf, "Failed to create scaled font: %s status: %d", NS_ConvertUTF16toUTF8(GetName()).get(), cairoerr);
        NS_WARNING(warnBuf);
#endif
    }
}


ATSFontRef gfxAtsuiFont::GetATSFontRef()
{
    return static_cast<MacOSFontEntry*>(GetFontEntry())->GetFontRef();
}

static void
DisableUncommonLigaturesAndLineBoundarySwashes(ATSUStyle aStyle)
{
    static const ATSUFontFeatureType types[] = {
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kSmartSwashType,
        kSmartSwashType
    };
    static const ATSUFontFeatureType selectors[NS_ARRAY_LENGTH(types)] = {
        kRareLigaturesOffSelector,
        kLogosOffSelector,
        kRebusPicturesOffSelector,
        kDiphthongLigaturesOffSelector,
        kSquaredLigaturesOffSelector,
        kAbbrevSquaredLigaturesOffSelector,
        kLineInitialSwashesOffSelector,
        kLineFinalSwashesOffSelector
    };
    ATSUSetFontFeatures(aStyle, NS_ARRAY_LENGTH(types), types, selectors);
}

static void
DisableCommonLigatures(ATSUStyle aStyle)
{
    static const ATSUFontFeatureType types[] = {
        kLigaturesType
    };
    static const ATSUFontFeatureType selectors[NS_ARRAY_LENGTH(types)] = {
        kCommonLigaturesOffSelector
    };
    ATSUSetFontFeatures(aStyle, NS_ARRAY_LENGTH(types), types, selectors);
}

static double
RoundToNearestMultiple(double aValue, double aFraction)
{
  return floor(aValue/aFraction + 0.5)*aFraction;
}

void
gfxAtsuiFont::InitMetrics(ATSUFontID aFontID, ATSFontRef aFontRef)
{
    

    gfxFloat size =
        PR_MAX(((mAdjustedSize != 0.0f) ? mAdjustedSize : GetStyle()->size), 1.0f);

    

    if (mATSUStyle)
      ATSUDisposeStyle(mATSUStyle);

    ATSUFontID fid = aFontID;
    
    Fixed fSize = FloatToFixed(size);
    
    CGAffineTransform transform = CGAffineTransformMakeScale(1, -1);

    static const ATSUAttributeTag styleTags[] = {
        kATSUFontTag,
        kATSUSizeTag,
        kATSUFontMatrixTag
    };
    const ATSUAttributeValuePtr styleArgs[NS_ARRAY_LENGTH(styleTags)] = {
        &fid,
        &fSize,
        &transform
    };
    static const ByteCount styleArgSizes[NS_ARRAY_LENGTH(styleTags)] = {
        sizeof(ATSUFontID),
        sizeof(Fixed),
        sizeof(CGAffineTransform)
    };

    ATSUCreateStyle(&mATSUStyle);
    ATSUSetAttributes(mATSUStyle,
                      NS_ARRAY_LENGTH(styleTags),
                      styleTags,
                      styleArgSizes,
                      styleArgs);
    
    
    
    
    
    
    
    DisableUncommonLigaturesAndLineBoundarySwashes(mATSUStyle);

    

    ATSFontMetrics atsMetrics;
    OSStatus err;
    
    err = ATSFontGetHorizontalMetrics(aFontRef, kATSOptionFlagsDefault,
                                &atsMetrics);
                                
    if (err != noErr) {
        mIsValid = PR_FALSE;
        
#ifdef DEBUG        
        char warnBuf[1024];
        sprintf(warnBuf, "Bad font metrics for: %s err: %8.8x", NS_ConvertUTF16toUTF8(GetName()).get(), PRUint32(err));
        NS_WARNING(warnBuf);
#endif
        return;
    }

    if (atsMetrics.xHeight)
        mMetrics.xHeight = atsMetrics.xHeight * size;
    else
        mMetrics.xHeight = GetCharHeight('x');

    if (mAdjustedSize == 0.0f) {
        if (mMetrics.xHeight != 0.0f && GetStyle()->sizeAdjust != 0.0f) {
            gfxFloat aspect = mMetrics.xHeight / size;
            mAdjustedSize = GetStyle()->GetAdjustedSize(aspect);
            InitMetrics(aFontID, aFontRef);
            return;
        }
        mAdjustedSize = size;
    }

    mMetrics.emHeight = size;

    mMetrics.maxAscent =
      NS_ceil(RoundToNearestMultiple(atsMetrics.ascent*size, 1/1024.0));
    mMetrics.maxDescent =
      NS_ceil(-RoundToNearestMultiple(atsMetrics.descent*size, 1/1024.0));

    mMetrics.maxHeight = mMetrics.maxAscent + mMetrics.maxDescent;

    if (mMetrics.maxHeight - mMetrics.emHeight > 0)
        mMetrics.internalLeading = mMetrics.maxHeight - mMetrics.emHeight; 
    else
        mMetrics.internalLeading = 0.0;
    mMetrics.externalLeading = atsMetrics.leading * size;

    mMetrics.emAscent = mMetrics.maxAscent * mMetrics.emHeight / mMetrics.maxHeight;
    mMetrics.emDescent = mMetrics.emHeight - mMetrics.emAscent;

    mMetrics.maxAdvance = atsMetrics.maxAdvanceWidth * size + mSyntheticBoldOffset;

    float xWidth = GetCharWidth('x');
    if (atsMetrics.avgAdvanceWidth != 0.0)
        mMetrics.aveCharWidth =
            PR_MIN(atsMetrics.avgAdvanceWidth * size, xWidth);
    else
        mMetrics.aveCharWidth = xWidth;

    mMetrics.aveCharWidth += mSyntheticBoldOffset;

    if (GetFontEntry()->IsFixedPitch()) {
        
        
        
        mMetrics.maxAdvance = mMetrics.aveCharWidth;
    }

    mMetrics.underlineOffset = atsMetrics.underlinePosition * size;
    mMetrics.underlineSize = atsMetrics.underlineThickness * size;

    mMetrics.subscriptOffset = mMetrics.xHeight;
    mMetrics.superscriptOffset = mMetrics.xHeight;

    mMetrics.strikeoutOffset = mMetrics.xHeight / 2.0;
    mMetrics.strikeoutSize = mMetrics.underlineSize;

    PRUint32 glyphID;
    mMetrics.spaceWidth = GetCharWidth(' ', &glyphID);
    mSpaceGlyph = glyphID;

    mMetrics.zeroOrAveCharWidth = GetCharWidth('0', &glyphID);
    if (glyphID == 0) 
        mMetrics.zeroOrAveCharWidth = mMetrics.aveCharWidth;

    SanitizeMetrics(&mMetrics, GetFontEntry()->mIsBadUnderlineFont);

#if 0
    fprintf (stderr, "Font: %p (%s) size: %f\n", this,
             NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size);
    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics.maxAscent, mMetrics.maxDescent, mMetrics.maxAdvance);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.internalLeading, mMetrics.externalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f suOff: %f suSize: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize,
                              mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

PRBool
gfxAtsuiFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}

nsString
gfxAtsuiFont::GetUniqueName()
{
    return GetName();
}

float
gfxAtsuiFont::GetCharWidth(PRUnichar c, PRUint32 *aGlyphID)
{
    
    
    
    ATSUTextLayout layout;

    UniCharCount one = 1;
    ATSUCreateTextLayoutWithTextPtr(&c, 0, 1, 1, 1, &one, &mATSUStyle, &layout);

    ATSTrapezoid trap;
    ItemCount numBounds;
    ATSUGetGlyphBounds(layout, FloatToFixed(0.0), FloatToFixed(0.0),
                       0, 1, kATSUseFractionalOrigins, 1, &trap, &numBounds);

    float f =
        FixedToFloat(PR_MAX(trap.upperRight.x, trap.lowerRight.x)) -
        FixedToFloat(PR_MIN(trap.upperLeft.x, trap.lowerLeft.x));

    if (aGlyphID) {
        ATSUGlyphInfoArray glyphInfo;
        ByteCount bytes = sizeof(glyphInfo);
        ATSUGetGlyphInfo(layout, 0, 1, &bytes, &glyphInfo);
        *aGlyphID = glyphInfo.glyphs[0].glyphID;
    }

    ATSUDisposeTextLayout(layout);

    return f;
}

float
gfxAtsuiFont::GetCharHeight(PRUnichar c)
{
    
    
    
    ATSUTextLayout layout;

    UniCharCount one = 1;
    ATSUCreateTextLayoutWithTextPtr(&c, 0, 1, 1, 1, &one, &mATSUStyle, &layout);

    Rect rect;
    ATSUMeasureTextImage(layout, 0, 1, 0, 0, &rect);

    ATSUDisposeTextLayout(layout);

    return rect.bottom - rect.top;
}

gfxAtsuiFont::~gfxAtsuiFont()
{
    if (mScaledFont)
        cairo_scaled_font_destroy(mScaledFont);
    if (mFontFace)
        cairo_font_face_destroy(mFontFace);

    if (mATSUStyle)
        ATSUDisposeStyle(mATSUStyle);
}

const gfxFont::Metrics&
gfxAtsuiFont::GetMetrics()
{
    return mMetrics;
}

void
gfxAtsuiFont::SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID,
        PRBool aNeedTight, gfxGlyphExtents *aExtents)
{
    ATSGlyphScreenMetrics metrics;
    GlyphID glyph = aGlyphID;
    OSStatus err = ATSUGlyphGetScreenMetrics(mATSUStyle, 1, &glyph, 0, false, false,
                                             &metrics);
    if (err != noErr)
        return;
    PRUint32 appUnitsPerDevUnit = aExtents->GetAppUnitsPerDevUnit();

    if (!aNeedTight && metrics.topLeft.x >= 0 &&
        -metrics.topLeft.y + metrics.height <= mMetrics.maxAscent &&
        metrics.topLeft.y <= mMetrics.maxDescent) {
        PRUint32 appUnitsWidth =
            PRUint32(NS_ceil((metrics.topLeft.x + metrics.width)*appUnitsPerDevUnit));
        if (appUnitsWidth < gfxGlyphExtents::INVALID_WIDTH) {
            aExtents->SetContainedGlyphWidthAppUnits(aGlyphID, PRUint16(appUnitsWidth));
            return;
        }
    }

    double d2a = appUnitsPerDevUnit;
    gfxRect bounds(metrics.topLeft.x*d2a, (metrics.topLeft.y - metrics.height)*d2a,
                   metrics.width*d2a, metrics.height*d2a);
    aExtents->SetTightGlyphExtents(aGlyphID, bounds);
}

PRBool 
gfxAtsuiFont::HasMirroringInfo()
{
    if (!mHasMirroringLookedUp) {
        OSStatus status;
        ByteCount size;
        
        
        status = ATSFontGetTable(GetATSFontRef(), TRUETYPE_TAG('p','r','o','p'), 0, 0, 0, &size);
        mHasMirroring = (status == noErr);
        mHasMirroringLookedUp = PR_TRUE;
    }

    return mHasMirroring;
}




class AutoLayoutDataArrayPtr {
public:
    AutoLayoutDataArrayPtr(ATSULineRef aLineRef,
                           ATSUDirectDataSelector aSelector)
        : mLineRef(aLineRef), mSelector(aSelector)
    {
        OSStatus status =
            ATSUDirectGetLayoutDataArrayPtrFromLineRef(aLineRef,
                aSelector, PR_FALSE, &mArray, &mItemCount);
        if (status != noErr) {
            mArray = NULL;
            mItemCount = 0;
        }
    }
    ~AutoLayoutDataArrayPtr() {
        if (mArray) {
            ATSUDirectReleaseLayoutDataArrayPtr(mLineRef, mSelector, &mArray);
        }
    }

    void     *mArray;
    ItemCount mItemCount;

private:
    ATSULineRef            mLineRef;
    ATSUDirectDataSelector mSelector;
};





#define ATSUI_OVERRUNNING_GLYPH_FLAG 0x100000




static PRBool
PostLayoutCallback(ATSULineRef aLine, gfxTextRun *aTextRun,
                   const PRUnichar *aString, PRInt32 aStartOffset,
                   PRInt32 aRunStart, PRInt32 aRunLength)
{
    AutoLayoutDataArrayPtr baselineDeltasArray(aLine, kATSUDirectDataBaselineDeltaFixedArray);
    Fixed *baselineDeltas = static_cast<Fixed *>(baselineDeltasArray.mArray);
    AutoLayoutDataArrayPtr glyphRecordsArray(aLine, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent);

    PRInt32 numGlyphs = glyphRecordsArray.mItemCount;
    if (numGlyphs == 0 || !glyphRecordsArray.mArray) {
        NS_WARNING("Failed to retrieve key glyph data");
        return PR_FALSE;
    }
    ATSLayoutRecord *glyphRecords = static_cast<ATSLayoutRecord *>(glyphRecordsArray.mArray);
    NS_ASSERTION(!baselineDeltas || baselineDeltasArray.mItemCount == (PRUint32)numGlyphs,
                 "Mismatched glyph counts");
    NS_ASSERTION(glyphRecords[numGlyphs - 1].flags & kATSGlyphInfoTerminatorGlyph,
                 "Last glyph should be a terminator glyph");
    --numGlyphs;
    if (numGlyphs == 0)
        return PR_FALSE;

    PRBool isLTR = !aTextRun->IsRightToLeft();

    PRUint32 allFlags = 0;
    PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    nsAutoTArray<gfxTextRun::DetailedGlyph,8> detailedGlyphs;
    gfxTextRun::CompressedGlyph g;

    Fixed runWidth = glyphRecords[numGlyphs].realPos - glyphRecords[0].realPos;

    static const PRInt32 NO_GLYPH = -1;
    nsAutoTArray<PRInt32,128> charToGlyphArray;
    if (!charToGlyphArray.SetLength(aRunLength))
        return NS_ERROR_OUT_OF_MEMORY;

    PRInt32 *charToGlyph = charToGlyphArray.Elements();
    for (PRInt32 offset = 0; offset < aRunLength; ++offset) {
        charToGlyph[offset] = NO_GLYPH;
    }
    for (PRInt32 g = 0; g < numGlyphs; ++g) {
        
        
        PRInt32 loc = glyphRecords[g].originalOffset/2;
        if (loc >= 0 && loc < aRunLength) {
            charToGlyph[loc] = g;
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    PRInt32 glyphStart = 0; 
    PRInt32 charStart = isLTR ? 0 : aRunLength-1; 
    PRInt32 direction = isLTR ? 1 : -1; 

    while (glyphStart < numGlyphs) { 

        PRBool inOrder = PR_TRUE;
        PRInt32 charEnd = (PRInt32)glyphRecords[glyphStart].originalOffset/2;
        PRInt32 glyphEnd = glyphStart;
        PRInt32 charLimit = isLTR ? aRunLength : -1;
        do {
            
            
            
            
            
            charEnd += direction;
            while (charEnd != charLimit && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd += direction;
            }
            
            
            if (!isLTR && charToGlyph[charEnd+1] == NO_GLYPH &&
                NS_IS_LOW_SURROGATE(aString[charEnd+1])) {
                charEnd += 1;
            }

            
            for (PRInt32 i = charStart; i != charEnd; i += direction) {
                if (charToGlyph[i] != NO_GLYPH) {
                    glyphEnd = PR_MAX(glyphEnd, charToGlyph[i] + 1); 
                }
            }

            
            PRBool extendedCharRange = PR_FALSE;
            while (glyphEnd < numGlyphs && glyphRecords[glyphEnd].glyphID == 0xffff) {
                if (isLTR) {
                    if ((PRInt32)glyphRecords[glyphEnd].originalOffset/2 >= charEnd) {
                        
                        
                        charEnd = (PRInt32)glyphRecords[glyphEnd].originalOffset/2;
                        extendedCharRange = PR_TRUE;
                    }
                } else {
                    if ((PRInt32)glyphRecords[glyphEnd].originalOffset/2 <= charEnd) {
                        charEnd = (PRInt32)glyphRecords[glyphEnd].originalOffset/2;
                        extendedCharRange = PR_TRUE;
                    }
                }
                ++glyphEnd;
            }
            if (extendedCharRange) {
                
                
                continue;
            }

            if (glyphEnd == glyphStart + 1) {
                
                break;
            }

            if (glyphEnd == glyphStart) {
                
                continue;
            }

            
            
            
            PRBool allGlyphsAreWithinCluster = PR_TRUE;
            PRInt32 prevGlyphCharIndex = charStart;
            for (PRInt32 i = glyphStart; i < glyphEnd; ++i) {
                PRInt32 glyphCharIndex = (PRInt32)glyphRecords[i].originalOffset/2;
                if (isLTR) {
                    if (glyphCharIndex < charStart || glyphCharIndex >= charEnd) {
                        allGlyphsAreWithinCluster = PR_FALSE;
                        break;
                    }
                    if (glyphCharIndex < prevGlyphCharIndex) {
                        inOrder = PR_FALSE;
                    }
                    prevGlyphCharIndex = glyphCharIndex;
                } else {
                    if (glyphCharIndex > charStart || glyphCharIndex <= charEnd) {
                        allGlyphsAreWithinCluster = PR_FALSE;
                        break;
                    }
                    if (glyphCharIndex > prevGlyphCharIndex) {
                        inOrder = PR_FALSE;
                    }
                    prevGlyphCharIndex = glyphCharIndex;
                }
            }
            if (allGlyphsAreWithinCluster) {
                break;
            }
        } while (charEnd != charLimit);

        NS_ASSERTION(glyphStart < glyphEnd, "character/glyph clump contains no glyphs!");
        NS_ASSERTION(charStart != charEnd, "character/glyph contains no characters!");

        
        
        
        PRInt32 baseCharIndex, endCharIndex;
        if (isLTR) {
            baseCharIndex = charStart + aRunStart;
            endCharIndex = charEnd + aRunStart;
        } else {
            baseCharIndex = charEnd + aRunStart + 1;
            endCharIndex = charStart + aRunStart + 1;
        }

        
        if (baseCharIndex >= aRunStart + aRunLength || endCharIndex <= aRunStart) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }

        
        
        baseCharIndex = PR_MAX(baseCharIndex, aRunStart);
        endCharIndex = PR_MIN(endCharIndex, aRunStart + aRunLength);

        
        
        double toNextGlyph;
        if (glyphStart < numGlyphs-1) {
            toNextGlyph = FixedToFloat(glyphRecords[glyphStart+1].realPos -
                                       glyphRecords[glyphStart].realPos);
        } else {
            toNextGlyph = FixedToFloat(glyphRecords[0].realPos + runWidth -
                                       glyphRecords[glyphStart].realPos);
        }
        PRInt32 advance = PRInt32(toNextGlyph * appUnitsPerDevUnit);

        
        PRInt32 glyphsInClump = glyphEnd - glyphStart;
        if (glyphsInClump == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyphRecords[glyphStart].glyphID) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            aTextRun->IsClusterStart(baseCharIndex) &&
            (!baselineDeltas || baselineDeltas[glyphStart] == 0))
        {
            aTextRun->SetSimpleGlyph(baseCharIndex,
                                     g.SetSimpleGlyph(advance, glyphRecords[glyphStart].glyphID));
        } else {
            
            
            
            while (1) {
                gfxTextRun::DetailedGlyph *details = detailedGlyphs.AppendElement();
                details->mGlyphID = glyphRecords[glyphStart].glyphID;
                details->mXOffset = 0;
                details->mYOffset = !baselineDeltas ? 0.0f
                    : - FixedToFloat(baselineDeltas[glyphStart]) * appUnitsPerDevUnit;
                details->mAdvance = advance;
                if (++glyphStart >= glyphEnd) {
                    break;
                }
                if (glyphStart < numGlyphs-1) {
                    toNextGlyph = FixedToFloat(glyphRecords[glyphStart+1].realPos -
                                               glyphRecords[glyphStart].realPos);
                } else {
                    toNextGlyph = FixedToFloat(glyphRecords[0].realPos + runWidth -
                                               glyphRecords[glyphStart].realPos);
                }
                advance = PRInt32(toNextGlyph * appUnitsPerDevUnit);
            }

            gfxTextRun::CompressedGlyph g;
            g.SetComplex(aTextRun->IsClusterStart(baseCharIndex),
                         PR_TRUE, detailedGlyphs.Length());
            aTextRun->SetGlyphs(baseCharIndex, g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        while (++baseCharIndex != endCharIndex) {
            g.SetComplex(inOrder && aTextRun->IsClusterStart(baseCharIndex),
                         PR_FALSE, 0);
            aTextRun->SetGlyphs(baseCharIndex, g, nsnull);
        }

        glyphStart = glyphEnd;
        charStart = charEnd;
    }

    return (allFlags & ATSUI_OVERRUNNING_GLYPH_FLAG) != 0;
}

struct PostLayoutCallbackClosure {
    gfxTextRun                  *mTextRun;
    const PRUnichar             *mString;
    PRUint32                     mStartOffset;
    PRUint32                     mOffsetInTextRun;
    PRUint32                     mLengthInTextRun;
    
    PRPackedBool                 mOverrunningGlyphs;
};


static PostLayoutCallbackClosure *gCallbackClosure = nsnull;

static OSStatus
PostLayoutOperationCallback(ATSULayoutOperationSelector iCurrentOperation, 
                            ATSULineRef iLineRef, 
                            UInt32 iRefCon, 
                            void *iOperationCallbackParameterPtr, 
                            ATSULayoutOperationCallbackStatus *oCallbackStatus)
{
    gCallbackClosure->mOverrunningGlyphs =
        PostLayoutCallback(iLineRef, gCallbackClosure->mTextRun,
                           gCallbackClosure->mString,
                           gCallbackClosure->mStartOffset,
                           gCallbackClosure->mOffsetInTextRun,
                           gCallbackClosure->mLengthInTextRun);
    *oCallbackStatus = kATSULayoutOperationCallbackStatusContinue;
    return noErr;
}

void
gfxAtsuiFont::InitTextRun(gfxTextRun *aTextRun,
                          const PRUnichar *aString,
                          PRUint32 aRunStart,
                          PRUint32 aRunLength)
{
    
    
    
    

    PRBool isRTL = aTextRun->IsRightToLeft();

    
    
    PRBool bidiWrap = isRTL;
    if (!bidiWrap && (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) == 0) {
        PRUint32 i;
        for (i = aRunStart; i < aRunStart + aRunLength; ++i) {
            if (gfxFontUtils::PotentialRTLChar(aString[i])) {
                bidiWrap = PR_TRUE;
                break;
            }
        }
    }

    
    
    const UniChar beginLTR[]    = { 0x202d };
    const UniChar beginRTL[]    = { 0x202e };
    const UniChar endBidiWrap[] = { 0x202c };

    const UniChar *textPtr;
    PRUint32 startOffset;
    nsAutoArrayPtr<UniChar> textBuffer;
    if (bidiWrap) {
        startOffset = isRTL ?
            sizeof(beginRTL) / sizeof(beginRTL[0]) : sizeof(beginLTR) / sizeof(beginLTR[0]);
        UniCharCount len = startOffset + aRunLength + sizeof(endBidiWrap) / sizeof(endBidiWrap[0]);
        textBuffer = new UniChar[len];
        ::memcpy(textBuffer.get(), isRTL ? beginRTL : beginLTR, startOffset * sizeof(UniChar));
        if (isRTL && !HasMirroringInfo()) {
            
            
            
            PRUint32 i;
            for (i = 0; i < aRunLength; ++i) {
                textBuffer[startOffset + i] = SymmSwap(aString[aRunStart + i]);
            }
        } else {
            ::memcpy(textBuffer.get() + startOffset, aString + aRunStart, aRunLength * sizeof(UniChar));
        }
        ::memcpy(textBuffer.get() + startOffset + aRunLength, endBidiWrap, sizeof(endBidiWrap));
        textPtr = textBuffer.get();
    } else {
        startOffset = 0;
        textPtr = aString + aRunStart;
    }

    OSStatus status;
    ATSUStyle atsuStyle = GetATSUStyle();

    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) {
        status = ::ATSUCreateAndCopyStyle(atsuStyle, &atsuStyle);
        if (status == noErr) {
            DisableCommonLigatures(atsuStyle);
        } else {
            
            atsuStyle = GetATSUStyle();
        }
    }

    
    
    
    
    
    
    nsTArray<PRUint32> hackForLiGothic;

    MacOSFontEntry *fe = static_cast<MacOSFontEntry*>(GetFontEntry());
    if (fe->UseLiGothicAtsuiHack()) {
        PRUnichar *text = const_cast<PRUnichar*>(textPtr);
        for (PRUint32 i = startOffset; i < startOffset + aRunLength; ++i) {
            if (text[i] == kLiGothicBadCharUnicode) {
                hackForLiGothic.AppendElement(i);
                text[i] = kLiGothicBadCharUnicode + 1;
            }
        }
    }

    ATSUTextLayout layout;
    
    
    
    
    UniCharCount runLength = aRunLength;
    status = ::ATSUCreateTextLayoutWithTextPtr(textPtr,
                                               startOffset,
                                               aRunLength,
                                               startOffset + aRunLength,
                                               1,
                                               &runLength,
                                               &atsuStyle,
                                               &layout);
    

    PostLayoutCallbackClosure closure;
    closure.mTextRun = aTextRun;
    closure.mString = textPtr;
    closure.mStartOffset = startOffset;
    closure.mOffsetInTextRun = aRunStart;
    closure.mLengthInTextRun = aRunLength;
    NS_ASSERTION(!gCallbackClosure, "Reentering InitTextRun? Expect disaster!");
    gCallbackClosure = &closure;

    ATSULayoutOperationOverrideSpecifier override;
    override.operationSelector = kATSULayoutOperationPostLayoutAdjustment;
    override.overrideUPP = PostLayoutOperationCallback;

    
    ATSLineLayoutOptions lineLayoutOptions = kATSLineKeepSpacesOutOfMargin | kATSLineHasNoHangers;
    Boolean lineDirection = isRTL ? kATSURightToLeftBaseDirection : kATSULeftToRightBaseDirection;

    static ATSUAttributeTag layoutTags[] = {
        kATSULineLayoutOptionsTag,
        kATSULayoutOperationOverrideTag,
        kATSULineDirectionTag
    };
    static ByteCount layoutArgSizes[] = {
        sizeof(ATSLineLayoutOptions),
        sizeof(ATSULayoutOperationOverrideSpecifier),
        sizeof(Boolean)
    };

    ATSUAttributeValuePtr layoutArgs[] = {
        &lineLayoutOptions,
        &override,
        &lineDirection
    };
    ::ATSUSetLayoutControls(layout,
                            NS_ARRAY_LENGTH(layoutTags),
                            layoutTags,
                            layoutArgSizes,
                            layoutArgs);

    
    
    ATSTrapezoid trap;
    ItemCount trapCount;
    ::ATSUGetGlyphBounds(layout, 0, 0, startOffset, aRunLength,
                         kATSUseFractionalOrigins, 1, &trap, &trapCount); 

    ::ATSUDisposeTextLayout(layout);

    aTextRun->AdjustAdvancesForSyntheticBold(aRunStart, aRunLength);

    for (PRUint32 i = 0; i < hackForLiGothic.Length(); ++i) {
        gfxTextRun::CompressedGlyph glyph =
            aTextRun->GetCharacterGlyphs()[aRunStart + hackForLiGothic[i]];
        if (glyph.IsSimpleGlyph()) {
            aTextRun->SetSimpleGlyph(aRunStart + hackForLiGothic[i],
                                 glyph.SetSimpleGlyph(glyph.GetSimpleAdvance(),
                                                      kLiGothicBadCharGlyph));
        }
    }

    if (atsuStyle != GetATSUStyle()) {
        ::ATSUDisposeStyle(atsuStyle);
    }
    gCallbackClosure = nsnull;
}

#endif 
