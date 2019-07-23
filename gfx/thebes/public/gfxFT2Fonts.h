





































#ifndef GFX_GTKDFBFONTS_H
#define GFX_GTKDFBFONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxContext.h"
#include "gfxFontUtils.h"
#include "gfxUserFontSet.h"

typedef struct FT_FaceRec_* FT_Face;






class FontEntry;
class FontFamily : public gfxFontFamily
{
public:
    FontFamily(const nsAString& aName) :
        gfxFontFamily(aName) { }

    FontEntry *FindFontEntry(const gfxFontStyle& aFontStyle);

protected:
    virtual PRBool FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                       PRBool anItalic, PRInt16 aStretch);
};

class FontEntry : public gfxFontEntry
{
public:
    FontEntry(const nsAString& aFaceName) :
        gfxFontEntry(aFaceName)
    {
        mFTFace = nsnull;
        mFontFace = nsnull;
        mFTFontIndex = 0;
    }

    FontEntry(const FontEntry& aFontEntry);
    ~FontEntry();

    const nsString& GetName() const {
        return mFaceName;
    }

    static FontEntry* 
    CreateFontEntry(const gfxProxyFontEntry &aProxyEntry, nsISupports *aLoader,
                    const PRUint8 *aFontData, PRUint32 aLength);
    
    static FontEntry* 
    CreateFontEntryFromFace(FT_Face aFace);
    
    cairo_font_face_t *CairoFontFace();
    nsresult ReadCMAP();

    FT_Face mFTFace;
    cairo_font_face_t *mFontFace;

    nsString mFaceName;
    nsCString mFilename;
    PRUint8 mFTFontIndex;
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

    FontEntry *GetFontEntry();

    static already_AddRefed<gfxFT2Font>
    GetOrMakeFont(const nsAString& aName, const gfxFontStyle *aStyle);
    static already_AddRefed<gfxFT2Font>
    GetOrMakeFont(FontEntry *aFontEntry, const gfxFontStyle *aStyle);

    struct CachedGlyphData {
        CachedGlyphData()
            : glyphIndex(0xffffffffU) { }

        CachedGlyphData(PRUint32 gid)
            : glyphIndex(gid) { }

        PRUint32 glyphIndex;
        PRInt32 lsbDelta;
        PRInt32 rsbDelta;
        PRInt32 xAdvance;
    };

    const CachedGlyphData* GetGlyphDataForChar(PRUint32 ch) {
        CharGlyphMapEntryType *entry = mCharGlyphCache.PutEntry(ch);

        if (!entry)
            return nsnull;

        if (entry->mData.glyphIndex == 0xffffffffU) {
            
            FillGlyphDataForChar(ch, &entry->mData);
        }

        return &entry->mData;
    }

    class FaceLock {
    public:
        FaceLock(gfxFT2Font *font);
        ~FaceLock();

        FT_Face Face() { return mFace; }

    protected:
        cairo_scaled_font_t *mScaledFont;
        FT_Face mFace;
    };

protected:
    cairo_scaled_font_t *mScaledFont;

    PRBool mHasSpaceGlyph;
    PRUint32 mSpaceGlyph;
    PRBool mHasMetrics;
    Metrics mMetrics;
    gfxFloat mAdjustedSize;

    void FillGlyphDataForChar(PRUint32 ch, CachedGlyphData *gd);

    typedef nsBaseHashtableET<nsUint32HashKey, CachedGlyphData> CharGlyphMapEntryType;
    typedef nsTHashtable<CharGlyphMapEntryType> CharGlyphMap;
    CharGlyphMap mCharGlyphCache;
};

class THEBES_API gfxFT2FontGroup : public gfxFontGroup {
public: 
    gfxFT2FontGroup (const nsAString& families,
                    const gfxFontStyle *aStyle);
    virtual ~gfxFT2FontGroup ();

    inline gfxFT2Font *GetFontAt (PRInt32 i) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");
        NS_ASSERTION(mFonts.Length() > PRUint32(i), 
                     "Requesting a font index that doesn't exist");

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
    void AddRange(gfxTextRun *aTextRun, gfxFT2Font *font, const PRUnichar *str, PRUint32 offset, PRUint32 len);

    static PRBool FontCallback (const nsAString & fontName, 
                                const nsACString & genericName, 
                                void *closure);
    PRBool mEnableKerning;

    void GetPrefFonts(const char *aLangGroup,
                      nsTArray<nsRefPtr<FontEntry> >& aFontEntryList);
    void GetCJKPrefFonts(nsTArray<nsRefPtr<FontEntry> >& aFontEntryList);
    void FamilyListToArrayList(const nsString& aFamilies,
                               const nsCString& aLangGroup,
                               nsTArray<nsRefPtr<FontEntry> > *aFontEntryList);
    already_AddRefed<gfxFT2Font> WhichFontSupportsChar(const nsTArray<nsRefPtr<FontEntry> >& aFontEntryList,
                                                       PRUint32 aCh);
    already_AddRefed<gfxFont> WhichPrefFontSupportsChar(PRUint32 aCh);
    already_AddRefed<gfxFont> WhichSystemFontSupportsChar(PRUint32 aCh);

    nsTArray<gfxTextRange> mRanges;
    nsString mString;
};

#endif 

