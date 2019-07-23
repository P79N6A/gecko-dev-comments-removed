





































#ifndef GFX_QTFONTS_H
#define GFX_QTFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxContext.h"
#include "gfxFontUtils.h"

typedef struct FT_FaceRec_* FT_Face;






class FontEntry;
class FontFamily : public gfxFontFamily
{
public:
    FontFamily(const nsAString& aName) :
        gfxFontFamily(aName) { }

    FontEntry *FindFontEntry(const gfxFontStyle& aFontStyle);

public:
    nsTArray<nsRefPtr<FontEntry> > mFaces;
};

class FontEntry : public gfxFontEntry
{
public:
    FontEntry(const nsAString& aFaceName) :
        gfxFontEntry(aFaceName)
    {
        mFontFace = nsnull;
        mFTFontIndex = 0;
        mUnicodeFont = PR_FALSE;
        mSymbolFont = PR_FALSE;
    }

    FontEntry(const FontEntry& aFontEntry);
    ~FontEntry();

    const nsString& GetName() const {
        return mFaceName;
    }

    cairo_font_face_t *CairoFontFace();

    cairo_font_face_t *mFontFace;

    nsString mFaceName;
    nsCString mFilename;
    PRUint8 mFTFontIndex;

    PRPackedBool mTrueType    : 1;
    PRPackedBool mIsType1     : 1;
};



class gfxQtFont : public gfxFont {
public: 
    gfxQtFont(FontEntry *aFontEntry,
               const gfxFontStyle *aFontStyle);
    virtual ~gfxQtFont ();

    virtual const gfxFont::Metrics& GetMetrics();

    cairo_font_face_t *CairoFontFace();
    cairo_scaled_font_t *CairoScaledFont();

    virtual PRBool SetupCairoFont(gfxContext *aContext);
    virtual nsString GetUniqueName();
    virtual PRUint32 GetSpaceGlyph();

    FontEntry *GetFontEntry();
private:
    cairo_scaled_font_t *mScaledFont;

    PRBool mHasSpaceGlyph;
    PRUint32 mSpaceGlyph;
    PRBool mHasMetrics;
    Metrics mMetrics;
    gfxFloat mAdjustedSize;

};

class THEBES_API gfxQtFontGroup : public gfxFontGroup {
public: 
    gfxQtFontGroup (const nsAString& families,
                    const gfxFontStyle *aStyle);
    virtual ~gfxQtFontGroup ();

    inline gfxQtFont *GetFontAt (PRInt32 i) {
        return static_cast <gfxQtFont *>(static_cast <gfxFont *>(mFonts[i]));
    }

protected: 
    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, 
                                    PRUint32 aLength,
                                    const Parameters *aParams, 
                                    PRUint32 aFlags);

    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, 
                                    PRUint32 aLength,
                                    const Parameters *aParams, 
                                    PRUint32 aFlags);

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);


protected: 
    void InitTextRun(gfxTextRun *aTextRun);

    void CreateGlyphRunsFT(gfxTextRun *aTextRun);
    void AddRange(gfxTextRun *aTextRun, gfxQtFont *font, const PRUnichar *str, PRUint32 len);

    static PRBool FontCallback (const nsAString & fontName, 
                                const nsACString & genericName, 
                                void *closure);
    PRBool mEnableKerning;

    gfxQtFont *FindFontForChar(PRUint32 ch, PRUint32 prevCh, PRUint32 nextCh, gfxQtFont *aFont);
    PRUint32 ComputeRanges();

    struct TextRange {
        TextRange(PRUint32 aStart,  PRUint32 aEnd) : start(aStart), end(aEnd) { }
        PRUint32 Length() const { return end - start; }
        nsRefPtr<gfxQtFont> font;
        PRUint32 start, end;
    };

    nsTArray<TextRange> mRanges;
    nsString mString;
};

#endif 

