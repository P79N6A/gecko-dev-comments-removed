





































#ifndef GFX_GTKDFBFONTS_H
#define GFX_GTKDFBFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxContext.h"
#include "gfxFontUtils.h"

typedef struct FT_FaceRec_* FT_Face;






class FontEntry;
class FontFamily
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(FontFamily)

    FontFamily(const nsAString& aName) :
        mName(aName) { }

    FontEntry *FindFontEntry(const gfxFontStyle& aFontStyle);

public:
    nsTArray<nsRefPtr<FontEntry> > mFaces;
    nsString mName;
};

class FontEntry
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(FontEntry)

    FontEntry(const nsString& aFaceName) : 
        mFontFace(nsnull), mFaceName(aFaceName), mFTFontIndex(0), mUnicodeFont(PR_FALSE), mSymbolFont(PR_FALSE)
    { }

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

    PRPackedBool mUnicodeFont : 1;
    PRPackedBool mSymbolFont  : 1;
    PRPackedBool mTrueType    : 1;
    PRPackedBool mIsType1     : 1;
    PRPackedBool mItalic      : 1;
    PRUint16 mWeight;

    gfxSparseBitSet mCharacterMap;
};



class gfxFT2Font : public gfxFont {
public: 
    gfxFT2Font(FontEntry *aFontEntry,
               const gfxFontStyle *aFontStyle);
    virtual ~gfxFT2Font ();

    virtual const gfxFont::Metrics& GetMetrics();

    cairo_font_face_t *CairoFontFace();
    cairo_scaled_font_t *CairoScaledFont();

    virtual PRBool SetupCairoFont(gfxContext *aContext);
    virtual nsString GetUniqueName();
    virtual PRUint32 GetSpaceGlyph();

    FontEntry *GetFontEntry() { return mFontEntry; }
private:
    cairo_scaled_font_t *mScaledFont;

    PRBool mHasSpaceGlyph;
    PRUint32 mSpaceGlyph;
    PRBool mHasMetrics;
    Metrics mMetrics;
    gfxFloat mAdjustedSize;

    nsRefPtr<FontEntry> mFontEntry;
};

class THEBES_API gfxFT2FontGroup : public gfxFontGroup {
public: 
    gfxFT2FontGroup (const nsAString& families,
                    const gfxFontStyle *aStyle);
    virtual ~gfxFT2FontGroup ();

    inline gfxFT2Font *GetFontAt (PRInt32 i) {
        return static_cast <gfxFT2Font *>(static_cast <gfxFont *>(mFonts[i]));
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
    void AddRange(gfxTextRun *aTextRun, gfxFT2Font *font, const PRUnichar *str, PRUint32 len);

    static PRBool FontCallback (const nsAString & fontName, 
                                const nsACString & genericName, 
                                void *closure);
    PRBool mEnableKerning;

    gfxFT2Font *FindFontForChar(PRUint32 ch, PRUint32 prevCh, PRUint32 nextCh, gfxFT2Font *aFont);
    PRUint32 ComputeRanges();

    struct TextRange {
        TextRange(PRUint32 aStart,  PRUint32 aEnd) : start(aStart), end(aEnd) { }
        PRUint32 Length() const { return end - start; }
        nsRefPtr<gfxFT2Font> font;
        PRUint32 start, end;
    };

    nsTArray<TextRange> mRanges;
    nsString mString;
};

#endif 

