





































#ifndef GFX_FT2FONTS_H
#define GFX_FT2FONTS_H

#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFT2FontBase.h"
#include "gfxContext.h"
#include "gfxFontUtils.h"
#include "gfxUserFontSet.h"

typedef struct FT_FaceRec_* FT_Face;

class FileAndIndex {
public:
    FileAndIndex(nsCString aFilename, PRUint32 aIndex) :
        filename(aFilename), index(aIndex) {}
    FileAndIndex(FileAndIndex* fai) :
        filename(fai->filename), index(fai->index) {}
    nsCString filename;
    PRUint32 index;
};






class FontEntry;
class FontFamily : public gfxFontFamily
{
public:
    FontFamily(const nsAString& aName) :
        gfxFontFamily(aName) { }

    FontEntry *FindFontEntry(const gfxFontStyle& aFontStyle);
    virtual void FindStyleVariations();
    void AddFontFileAndIndex(nsCString aFilename, PRUint32 aIndex);

private:
    
    
    nsTArray<FileAndIndex> mFilenames;
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

    ~FontEntry();

    const nsString& GetName() const {
        return Name();
    }

    static FontEntry* 
    CreateFontEntry(const gfxProxyFontEntry &aProxyEntry,
                    const PRUint8 *aFontData, PRUint32 aLength);

    static FontEntry* 
    CreateFontEntryFromFace(FT_Face aFace, const PRUint8 *aFontData = nsnull);
        
        

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, PRBool aNeedsBold);

    cairo_font_face_t *CairoFontFace();
    nsresult ReadCMAP();

    nsresult GetFontTable(PRUint32 aTableTag, FallibleTArray<PRUint8>& aBuffer);

    FT_Face mFTFace;
    cairo_font_face_t *mFontFace;

    nsCString mFilename;
    PRUint8 mFTFontIndex;
};


class gfxFT2Font : public gfxFT2FontBase {
public: 
    gfxFT2Font(cairo_scaled_font_t *aCairoFont,
               FontEntry *aFontEntry,
               const gfxFontStyle *aFontStyle);
    virtual ~gfxFT2Font ();

    cairo_font_face_t *CairoFontFace();

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

protected:
    virtual PRBool InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript,
                               PRBool aPreferPlatformShaping = PR_FALSE);

    void FillGlyphDataForChar(PRUint32 ch, CachedGlyphData *gd);

    void AddRange(gfxTextRun *aTextRun, const PRUnichar *str, PRUint32 offset, PRUint32 len);

    typedef nsBaseHashtableET<nsUint32HashKey, CachedGlyphData> CharGlyphMapEntryType;
    typedef nsTHashtable<CharGlyphMapEntryType> CharGlyphMap;
    CharGlyphMap mCharGlyphCache;
};

class THEBES_API gfxFT2FontGroup : public gfxFontGroup {
public: 
    gfxFT2FontGroup (const nsAString& families,
                    const gfxFontStyle *aStyle,
                    gfxUserFontSet *aUserFontSet);
    virtual ~gfxFT2FontGroup ();

protected: 

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);


protected: 

    static PRBool FontCallback (const nsAString & fontName, 
                                const nsACString & genericName, 
                                void *closure);
    PRBool mEnableKerning;

    void GetPrefFonts(nsIAtom *aLangGroup,
                      nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList);
    void GetCJKPrefFonts(nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList);
    void FamilyListToArrayList(const nsString& aFamilies,
                               nsIAtom *aLangGroup,
                               nsTArray<nsRefPtr<gfxFontEntry> > *aFontEntryList);
    already_AddRefed<gfxFT2Font> WhichFontSupportsChar(const nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList,
                                                       PRUint32 aCh);
    already_AddRefed<gfxFont> WhichPrefFontSupportsChar(PRUint32 aCh);
    already_AddRefed<gfxFont> WhichSystemFontSupportsChar(PRUint32 aCh);

    nsTArray<gfxTextRange> mRanges;
    nsString mString;
};

#endif 

