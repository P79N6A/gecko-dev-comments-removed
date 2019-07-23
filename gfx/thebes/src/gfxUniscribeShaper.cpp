










































#include "prtypes.h"
#include "gfxTypes.h"

#include "gfxContext.h"
#include "gfxUniscribeShaper.h"
#include "gfxWindowsPlatform.h"
#include "gfxAtoms.h"

#include "gfxFontTest.h"

#include "cairo.h"
#include "cairo-win32.h"

#include <windows.h>

#include "nsTArray.h"

#include "prlog.h"
#include "prinit.h"
static PRLogModuleInfo *gFontLog = PR_NewLogModule("winfonts");







#define ESTIMATE_MAX_GLYPHS(L) (((3 * (L)) >> 1) + 16)

class UniscribeItem
{
public:
    UniscribeItem(gfxContext *aContext, HDC aDC,
                  gfxUniscribeShaper *aShaper,
                  const PRUnichar *aString, PRUint32 aLength,
                  SCRIPT_ITEM *aItem) :
        mContext(aContext), mDC(aDC),
        mShaper(aShaper),
        mItemString(aString), mItemLength(aLength), 
        mAlternativeString(nsnull), mScriptItem(aItem),
        mScript(aItem->a.eScript),
        mNumGlyphs(0), mMaxGlyphs(ESTIMATE_MAX_GLYPHS(aLength)),
        mFontSelected(PR_FALSE)
    {
        NS_ASSERTION(mMaxGlyphs < 65535, "UniscribeItem is too big, ScriptShape() will fail!");
    }

    ~UniscribeItem() {
        free(mAlternativeString);
    }

    PRBool AllocateBuffers() {
        return (mGlyphs.SetLength(mMaxGlyphs) &&
                mClusters.SetLength(mItemLength + 1) &&
                mAttr.SetLength(mMaxGlyphs));
    }

    




    HRESULT Shape() {
        HRESULT rv;
        HDC shapeDC = nsnull;

        const PRUnichar *str = mAlternativeString ? mAlternativeString : mItemString;

        mScriptItem->a.fLogicalOrder = PR_TRUE; 
        SCRIPT_ANALYSIS sa = mScriptItem->a;

        while (PR_TRUE) {

            rv = ScriptShape(shapeDC, mShaper->ScriptCache(),
                             str, mItemLength,
                             mMaxGlyphs, &sa,
                             mGlyphs.Elements(), mClusters.Elements(),
                             mAttr.Elements(), &mNumGlyphs);

            if (rv == E_OUTOFMEMORY) {
                mMaxGlyphs *= 2;
                if (!mGlyphs.SetLength(mMaxGlyphs) ||
                    !mAttr.SetLength(mMaxGlyphs)) {
                    return E_OUTOFMEMORY;
                }
                continue;
            }

            
            
            
            
            
            

            if (sa.fNoGlyphIndex) {
                return GDI_ERROR;
            }

            if (rv == E_PENDING) {
                if (shapeDC == mDC) {
                    
                    return E_PENDING;
                }

                SelectFont();

                shapeDC = mDC;
                continue;
            }

            
            
            
            
            
            
            
            
            if (rv == USP_E_SCRIPT_NOT_IN_FONT) {
                sa.eScript = SCRIPT_UNDEFINED;
                NS_WARNING("Uniscribe says font does not support script needed");
                continue;
            }

            return rv;
        }
    }

    PRBool ShapingEnabled() {
        return (mScriptItem->a.eScript != SCRIPT_UNDEFINED);
    }
    void DisableShaping() {
        mScriptItem->a.eScript = SCRIPT_UNDEFINED;
        
        
        
        
        GenerateAlternativeString();
    }
    void EnableShaping() {
        mScriptItem->a.eScript = mScript;
        if (mAlternativeString) {
            free(mAlternativeString);
            mAlternativeString = nsnull;
        }
    }

    PRBool IsGlyphMissing(SCRIPT_FONTPROPERTIES *aSFP, PRUint32 aGlyphIndex) {
        return (mGlyphs[aGlyphIndex] == aSFP->wgDefault);
    }


    HRESULT Place() {
        HRESULT rv;
        HDC placeDC = nsnull;

        if (!mOffsets.SetLength(mNumGlyphs) ||
            !mAdvances.SetLength(mNumGlyphs)) {
            return E_OUTOFMEMORY;
        }

        SCRIPT_ANALYSIS sa = mScriptItem->a;

        while (PR_TRUE) {
            rv = ScriptPlace(placeDC, mShaper->ScriptCache(),
                             mGlyphs.Elements(), mNumGlyphs,
                             mAttr.Elements(), &sa,
                             mAdvances.Elements(), mOffsets.Elements(), NULL);

            if (rv == E_PENDING) {
                SelectFont();
                placeDC = mDC;
                continue;
            }

            if (rv == USP_E_SCRIPT_NOT_IN_FONT) {
                sa.eScript = SCRIPT_UNDEFINED;
                continue;
            }

            break;
        }

        return rv;
    }

    void ScriptFontProperties(SCRIPT_FONTPROPERTIES *sfp) {
        HRESULT rv;

        memset(sfp, 0, sizeof(SCRIPT_FONTPROPERTIES));
        sfp->cBytes = sizeof(SCRIPT_FONTPROPERTIES);
        rv = ScriptGetFontProperties(NULL, mShaper->ScriptCache(),
                                     sfp);
        if (rv == E_PENDING) {
            SelectFont();
            rv = ScriptGetFontProperties(mDC, mShaper->ScriptCache(),
                                         sfp);
        }
    }

    void SaveGlyphs(gfxTextRun *aRun, PRUint32 aRunStart) {
        PRUint32 offsetInRun = aRunStart + mScriptItem->iCharPos;

        
        SCRIPT_FONTPROPERTIES sfp;
        ScriptFontProperties(&sfp);

        PRUint32 offset = 0;
        nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;
        gfxTextRun::CompressedGlyph g;
        const PRUint32 appUnitsPerDevUnit = aRun->GetAppUnitsPerDevUnit();
        while (offset < mItemLength) {
            PRUint32 runOffset = offsetInRun + offset;
            if (offset > 0 && mClusters[offset] == mClusters[offset - 1]) {
                g.SetComplex(aRun->IsClusterStart(runOffset), PR_FALSE, 0);
                aRun->SetGlyphs(runOffset, g, nsnull);
            } else {
                
                PRUint32 k = mClusters[offset];
                PRUint32 glyphCount = mNumGlyphs - k;
                PRUint32 nextClusterOffset;
                PRBool missing = IsGlyphMissing(&sfp, k);
                for (nextClusterOffset = offset + 1; nextClusterOffset < mItemLength; ++nextClusterOffset) {
                    if (mClusters[nextClusterOffset] > k) {
                        glyphCount = mClusters[nextClusterOffset] - k;
                        break;
                    }
                }
                PRUint32 j;
                for (j = 1; j < glyphCount; ++j) {
                    if (IsGlyphMissing(&sfp, k + j)) {
                        missing = PR_TRUE;
                    }
                }
                PRInt32 advance = mAdvances[k]*appUnitsPerDevUnit;
                WORD glyph = mGlyphs[k];
                NS_ASSERTION(!gfxFontGroup::IsInvalidChar(mItemString[offset]),
                             "invalid character detected");
                if (missing) {
                    if (NS_IS_HIGH_SURROGATE(mItemString[offset]) &&
                        offset + 1 < mItemLength &&
                        NS_IS_LOW_SURROGATE(mItemString[offset + 1])) {
                        aRun->SetMissingGlyph(runOffset,
                                              SURROGATE_TO_UCS4(mItemString[offset],
                                                                mItemString[offset + 1]));
                    } else {
                        aRun->SetMissingGlyph(runOffset, mItemString[offset]);
                    }
                } else if (glyphCount == 1 && advance >= 0 &&
                    mOffsets[k].dv == 0 && mOffsets[k].du == 0 &&
                    gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
                    gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyph)) {
                    aRun->SetSimpleGlyph(runOffset, g.SetSimpleGlyph(advance, glyph));
                } else {
                    if (detailedGlyphs.Length() < glyphCount) {
                        if (!detailedGlyphs.AppendElements(glyphCount - detailedGlyphs.Length()))
                            return;
                    }
                    PRUint32 i;
                    for (i = 0; i < glyphCount; ++i) {
                        gfxTextRun::DetailedGlyph *details = &detailedGlyphs[i];
                        details->mGlyphID = mGlyphs[k + i];
                        details->mAdvance = mAdvances[k + i]*appUnitsPerDevUnit;
                        details->mXOffset = float(mOffsets[k + i].du)*appUnitsPerDevUnit*aRun->GetDirection();
                        details->mYOffset = - float(mOffsets[k + i].dv)*appUnitsPerDevUnit;
                    }
                    aRun->SetGlyphs(runOffset,
                        g.SetComplex(PR_TRUE, PR_TRUE, glyphCount), detailedGlyphs.Elements());
                }
            }
            ++offset;
        }
    }

    void SelectFont() {
        if (mFontSelected)
            return;

        cairo_t *cr = mContext->GetCairo();

        cairo_set_font_face(cr, mShaper->GetFont()->CairoFontFace());
        cairo_set_font_size(cr, mShaper->GetFont()->GetAdjustedSize());
        cairo_scaled_font_t *scaledFont = mShaper->GetFont()->CairoScaledFont();
        cairo_win32_scaled_font_select_font(scaledFont, mDC);

        mFontSelected = PR_TRUE;
    }

private:

    void GenerateAlternativeString() {
        if (mAlternativeString)
            free(mAlternativeString);
        mAlternativeString = (PRUnichar *)malloc(mItemLength * sizeof(PRUnichar));
        if (!mAlternativeString)
            return;
        memcpy((void *)mAlternativeString, (const void *)mItemString,
               mItemLength * sizeof(PRUnichar));
        for (PRUint32 i = 0; i < mItemLength; i++) {
            if (NS_IS_HIGH_SURROGATE(mItemString[i]) || NS_IS_LOW_SURROGATE(mItemString[i]))
                mAlternativeString[i] = PRUnichar(0xFFFD);
        }
    }

private:
    nsRefPtr<gfxContext> mContext;
    HDC mDC;
    gfxUniscribeShaper *mShaper;

    SCRIPT_ITEM *mScriptItem;
    WORD mScript;

public:
    
    const PRUnichar *mItemString;
    const PRUint32 mItemLength;

private:
    PRUnichar *mAlternativeString;

#define AVERAGE_ITEM_LENGTH 40

    nsAutoTArray<WORD, PRUint32(ESTIMATE_MAX_GLYPHS(AVERAGE_ITEM_LENGTH))> mGlyphs;
    nsAutoTArray<WORD, AVERAGE_ITEM_LENGTH + 1> mClusters;
    nsAutoTArray<SCRIPT_VISATTR, PRUint32(ESTIMATE_MAX_GLYPHS(AVERAGE_ITEM_LENGTH))> mAttr;
 
    nsAutoTArray<GOFFSET, 2 * AVERAGE_ITEM_LENGTH> mOffsets;
    nsAutoTArray<int, 2 * AVERAGE_ITEM_LENGTH> mAdvances;

#undef AVERAGE_ITEM_LENGTH

    int mMaxGlyphs;
    int mNumGlyphs;

    PRPackedBool mFontSelected;
};

#define MAX_ITEM_LENGTH 32768

static PRUint32 FindNextItemStart(int aOffset, int aLimit,
                                  nsTArray<SCRIPT_LOGATTR> &aLogAttr,
                                  const PRUnichar *aString)
{
    if (aOffset + MAX_ITEM_LENGTH >= aLimit) {
        
        
        return aLimit;
    }

    
    
    PRUint32 off;
    int boundary = -1;
    for (off = MAX_ITEM_LENGTH; off > 1; --off) {
      if (aLogAttr[off].fCharStop) {
          if (off > boundary) {
              boundary = off;
          }
          if (aString[aOffset+off] == ' ' || aString[aOffset+off - 1] == ' ')
            return aOffset+off;
      }
    }

    
    if (boundary > 0) {
      return aOffset+boundary;
    }

    
    
    
    return aOffset + MAX_ITEM_LENGTH;
}

class Uniscribe
{
public:
    Uniscribe(const PRUnichar *aString, PRUint32 aLength, PRBool aIsRTL) :
        mString(aString), mLength(aLength), mIsRTL(aIsRTL)
    {
    }
    ~Uniscribe() {
    }

    void Init() {
        memset(&mControl, 0, sizeof(SCRIPT_CONTROL));
        memset(&mState, 0, sizeof(SCRIPT_STATE));
        
        
        mState.uBidiLevel = mIsRTL;
        mState.fOverrideDirection = PR_TRUE;
    }

private:

    
    
    nsresult CopyItemSplitOversize(int aIndex, nsTArray<SCRIPT_ITEM> &aDest) {
        aDest.AppendElement(mItems[aIndex]);
        const int itemLength = mItems[aIndex+1].iCharPos - mItems[aIndex].iCharPos;
        if (ESTIMATE_MAX_GLYPHS(itemLength) > 65535) {
            
            

            
            nsTArray<SCRIPT_LOGATTR> logAttr;
            if (!logAttr.SetLength(itemLength))
                return NS_ERROR_FAILURE;
            HRESULT rv = ScriptBreak(mString+mItems[aIndex].iCharPos, itemLength,
                                     &mItems[aIndex].a, logAttr.Elements());
            if (FAILED(rv))
                return NS_ERROR_FAILURE;

            const int nextItemStart = mItems[aIndex+1].iCharPos;
            int start = FindNextItemStart(mItems[aIndex].iCharPos,
                                          nextItemStart, logAttr, mString);

            while (start < nextItemStart) {
                SCRIPT_ITEM item = mItems[aIndex];
                item.iCharPos = start;
                aDest.AppendElement(item);
                start = FindNextItemStart(start, nextItemStart, logAttr, mString);
            }
        } 
        return NS_OK;
    }

public:

    int Itemize() {
        HRESULT rv;

        int maxItems = 5;

        Init();

        
        
        if (!mItems.SetLength(maxItems + 1)) {
            return 0;
        }
        while ((rv = ScriptItemize(mString, mLength, maxItems, &mControl, &mState,
                                   mItems.Elements(), &mNumItems)) == E_OUTOFMEMORY) {
            maxItems *= 2;
            if (!mItems.SetLength(maxItems + 1)) {
                return 0;
            }
            Init();
        }

        if (ESTIMATE_MAX_GLYPHS(mLength) > 65535) {
            
            
            
            
            nsTArray<SCRIPT_ITEM> items;
            for (int i=0; i<mNumItems; i++) {
                nsresult nrs = CopyItemSplitOversize(i, items);
                NS_ASSERTION(NS_SUCCEEDED(nrs), "CopyItemSplitOversize() failed");
            }
            items.AppendElement(mItems[mNumItems]); 

            mItems = items;
            mNumItems = items.Length() - 1; 
        }
        return mNumItems;
    }

    PRUint32 ItemsLength() {
        return mNumItems;
    }

    SCRIPT_ITEM *ScriptItem(PRUint32 i) {
        NS_ASSERTION(i <= (PRUint32)mNumItems, "Trying to get out of bounds item");
        return &mItems[i];
    }

private:
    const PRUnichar *mString;
    const PRUint32 mLength;
    const PRBool mIsRTL;

    SCRIPT_CONTROL mControl;
    SCRIPT_STATE   mState;
    nsTArray<SCRIPT_ITEM> mItems;
    int mNumItems;
};

gfxUniscribeShaper::gfxUniscribeShaper(gfxGDIFont *aFont)
    : gfxFontShaper(aFont)
    , mScriptCache(NULL)
{
}

gfxUniscribeShaper::~gfxUniscribeShaper()
{
}

PRBool
gfxUniscribeShaper::InitTextRun(gfxContext *aContext,
                                gfxTextRun *aTextRun,
                                const PRUnichar *aString,
                                PRUint32 aRunStart,
                                PRUint32 aRunLength)
{
    DCFromContext aDC(aContext);
 
    const PRBool isRTL = aTextRun->IsRightToLeft();

    PRBool result = PR_TRUE;
    HRESULT rv;

    gfxGDIFont *font = static_cast<gfxGDIFont*>(mFont);
    AutoSelectFont fs(aDC, font->GetHFONT());

    Uniscribe us(aString + aRunStart, aRunLength, isRTL);

    
    int numItems = us.Itemize();

    SaveDC(aDC);
    for (int i = 0; i < numItems; ++i) {
        UniscribeItem item(aContext, aDC, this,
                           aString + aRunStart + us.ScriptItem(i)->iCharPos,
                           us.ScriptItem(i+1)->iCharPos - us.ScriptItem(i)->iCharPos,
                           us.ScriptItem(i));
        if (!item.AllocateBuffers()) {
            result = PR_FALSE;
            break;
        }

        if (!item.ShapingEnabled()) {
            item.EnableShaping();
        }

        rv = item.Shape();
        if (FAILED(rv)) {
            PR_LOG(gFontLog, PR_LOG_DEBUG, ("shaping failed"));
            
            
            
            item.DisableShaping();
            rv = item.Shape();
        }
#ifdef DEBUG
        if (FAILED(rv)) {
            NS_WARNING("Uniscribe failed to shape with font");
        }
#endif

        if (SUCCEEDED(rv)) {
            rv = item.Place();
#ifdef DEBUG
            if (FAILED(rv)) {
                
                NS_WARNING("Uniscribe failed to place with font");
            }
#endif
        }

        if (FAILED(rv)) {
            aTextRun->ResetGlyphRuns();
            
            
            
            result = PR_FALSE;
            break;
        }

        item.SaveGlyphs(aTextRun, aRunStart);
    }

    RestoreDC(aDC, -1);

    return result;
}
