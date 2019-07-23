






































#ifndef GFX_WINDOWSFONTS_H
#define GFX_WINDOWSFONTS_H

#include "prtypes.h"
#include "gfxTypes.h"
#include "gfxColor.h"
#include "gfxFont.h"
#include "gfxMatrix.h"
#include "gfxFontUtils.h"
#include "gfxUserFontSet.h"

#include "nsDataHashtable.h"

#include <usp10.h>
#include <cairo-win32.h>



#include <bitset>

class GDIFontEntry;







class gfxWindowsFont : public gfxFont {
public:
    gfxWindowsFont(gfxFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
                   cairo_antialias_t anAntialiasOption = CAIRO_ANTIALIAS_DEFAULT);
    virtual ~gfxWindowsFont();

    virtual const gfxFont::Metrics& GetMetrics();

    HFONT GetHFONT() { return mFont; }
    cairo_font_face_t *CairoFontFace();
    cairo_scaled_font_t *CairoScaledFont();
    SCRIPT_CACHE *ScriptCache() { return &mScriptCache; }
    gfxFloat GetAdjustedSize() { MakeHFONT(); return mAdjustedSize; }

    virtual nsString GetUniqueName();

    virtual void Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
                      gfxContext *aContext, PRBool aDrawToPath, gfxPoint *aBaselineOrigin,
                      Spacing *aSpacing);

    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               PRUint32 aStart, PRUint32 aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);

    virtual PRUint32 GetSpaceGlyph() {
        GetMetrics(); 
        return mSpaceGlyph;
    };

    PRBool IsValid() { GetMetrics(); return mIsValid; }
    GDIFontEntry *GetFontEntry();

    static already_AddRefed<gfxWindowsFont>
    GetOrMakeFont(gfxFontEntry *aFontEntry, const gfxFontStyle *aStyle,
                  PRBool aNeedsBold = PR_FALSE);

protected:
    HFONT MakeHFONT();
    void FillLogFont(gfxFloat aSize);

    HFONT    mFont;
    gfxFloat mAdjustedSize;
    PRUint32 mSpaceGlyph;

private:
    void ComputeMetrics();

    SCRIPT_CACHE mScriptCache;

    cairo_font_face_t *mFontFace;
    cairo_scaled_font_t *mScaledFont;

    gfxFont::Metrics *mMetrics;

    LOGFONTW mLogFont;

    cairo_antialias_t mAntialiasOption;

    virtual PRBool SetupCairoFont(gfxContext *aContext);
};







class THEBES_API gfxWindowsFontGroup : public gfxFontGroup {

public:
    gfxWindowsFontGroup(const nsAString& aFamilies, const gfxFontStyle* aStyle, gfxUserFontSet *aUserFontSet);
    virtual ~gfxWindowsFontGroup();

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const Parameters* aParams, PRUint32 aFlags);

    const nsACString& GetGenericFamily() const {
        return mGenericFamily;
    }

    void GroupFamilyListToArrayList(nsTArray<nsRefPtr<gfxFontEntry> > *list,
                                    nsTArray<PRPackedBool> *aNeedsBold);
    void FamilyListToArrayList(const nsString& aFamilies,
                               const nsCString& aLangGroup,
                               nsTArray<nsRefPtr<gfxFontEntry> > *list);

    virtual void UpdateFontList();
    virtual gfxFloat GetUnderlineOffset();

    gfxWindowsFont* GetFontAt(PRInt32 aFontIndex) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");

        return static_cast<gfxWindowsFont*>(static_cast<gfxFont*>(mFonts[aFontIndex]));
    }

protected:
    void InitFontList();
    void InitTextRunGDI(gfxContext *aContext, gfxTextRun *aRun, const char *aString, PRUint32 aLength);
    void InitTextRunGDI(gfxContext *aContext, gfxTextRun *aRun, const PRUnichar *aString, PRUint32 aLength);

    void InitTextRunUniscribe(gfxContext *aContext, gfxTextRun *aRun, const PRUnichar *aString, PRUint32 aLength);

    already_AddRefed<gfxFont> WhichPrefFontSupportsChar(PRUint32 aCh);
    already_AddRefed<gfxFont> WhichSystemFontSupportsChar(PRUint32 aCh);

    already_AddRefed<gfxWindowsFont> WhichFontSupportsChar(const nsTArray<nsRefPtr<gfxFontEntry> >& fonts, PRUint32 ch);
    void GetPrefFonts(const char *aLangGroup, nsTArray<nsRefPtr<gfxFontEntry> >& array);
    void GetCJKPrefFonts(nsTArray<nsRefPtr<gfxFontEntry> >& array);

    static PRBool FindWindowsFont(const nsAString& aName,
                                  const nsACString& aGenericName,
                                  void *closure);

    PRBool HasFont(gfxFontEntry *aFontEntry);

private:

    nsCString mGenericFamily;
    nsTArray<PRPackedBool> mFontNeedsBold;

    const char *mItemLangGroup;  

};

#endif 
