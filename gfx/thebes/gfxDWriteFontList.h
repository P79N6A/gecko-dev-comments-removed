




































#ifndef GFX_DWRITEFONTLIST_H
#define GFX_DWRITEFONTLIST_H

#include "gfxDWriteCommon.h"

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "cairo-win32.h"

#include "gfxPlatformFontList.h"







class gfxDWriteFontEntry;




class gfxDWriteFontFamily : public gfxFontFamily
{
public:
    






    gfxDWriteFontFamily(const nsAString& aName, 
                               IDWriteFontFamily *aFamily)
      : gfxFontFamily(aName), mDWFamily(aFamily) {}
    virtual ~gfxDWriteFontFamily();
    
    virtual void FindStyleVariations();

    virtual void LocalizedName(nsAString& aLocalizedName); 
protected:
    
    nsRefPtr<IDWriteFontFamily> mDWFamily;
};




class gfxDWriteFontEntry : public gfxFontEntry
{
public:
    





    gfxDWriteFontEntry(const nsAString& aFaceName,
                              IDWriteFont *aFont) 
      : gfxFontEntry(aFaceName), mFont(aFont), mFontFile(nsnull)
    {
        mItalic = (aFont->GetStyle() == DWRITE_FONT_STYLE_ITALIC ||
                   aFont->GetStyle() == DWRITE_FONT_STYLE_OBLIQUE);
        mStretch = FontStretchFromDWriteStretch(aFont->GetStretch());
        PRUint16 weight = PR_ROUNDUP(aFont->GetWeight() - 50, 100);

        weight = NS_MAX<PRUint16>(100, weight);
        weight = NS_MIN<PRUint16>(900, weight);
        mWeight = weight;

        mIsCJK = UNINITIALIZED_VALUE;
    }

    










    gfxDWriteFontEntry(const nsAString& aFaceName,
                              IDWriteFont *aFont,
                              PRUint16 aWeight,
                              PRInt16 aStretch,
                              PRBool aItalic)
      : gfxFontEntry(aFaceName), mFont(aFont), mFontFile(nsnull)
    {
        mWeight = aWeight;
        mStretch = aStretch;
        mItalic = aItalic;
        mIsUserFont = PR_TRUE;
        mIsLocalUserFont = PR_TRUE;
        mIsCJK = UNINITIALIZED_VALUE;
    }

    








    gfxDWriteFontEntry(const nsAString& aFaceName,
                              IDWriteFontFile *aFontFile,
                              PRUint16 aWeight,
                              PRInt16 aStretch,
                              PRBool aItalic)
      : gfxFontEntry(aFaceName), mFont(nsnull), mFontFile(aFontFile)
    {
        mWeight = aWeight;
        mStretch = aStretch;
        mItalic = aItalic;
        mIsUserFont = PR_TRUE;
        mIsCJK = UNINITIALIZED_VALUE;
    }

    virtual ~gfxDWriteFontEntry();

    virtual PRBool IsSymbolFont();

    virtual nsresult GetFontTable(PRUint32 aTableTag,
                                  FallibleTArray<PRUint8>& aBuffer);

    nsresult ReadCMAP();

    PRBool IsCJKFont();

protected:
    friend class gfxDWriteFont;
    friend class gfxDWriteFontList;

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle,
                                        PRBool aNeedsBold);
    
    nsresult CreateFontFace(
        IDWriteFontFace **aFontFace,
        DWRITE_FONT_SIMULATIONS aSimulations = DWRITE_FONT_SIMULATIONS_NONE);

    static PRBool InitLogFont(IDWriteFont *aFont, LOGFONTW *aLogFont);

    



    nsRefPtr<IDWriteFont> mFont;
    nsRefPtr<IDWriteFontFile> mFontFile;
    DWRITE_FONT_FACE_TYPE mFaceType;

    PRBool mIsCJK;
};


class gfxDWriteFontList : public gfxPlatformFontList {
public:
    gfxDWriteFontList();

    static gfxDWriteFontList* PlatformFontList() {
        return static_cast<gfxDWriteFontList*>(sPlatformFontList);
    }

    
    virtual nsresult InitFontList();

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle,
                                         PRBool& aNeedsBold);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);
    
    virtual PRBool ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName);

    PRBool GetStandardFamilyName(const nsAString& aFontName,
                                 nsAString& aFamilyName);

    IDWriteGdiInterop *GetGDIInterop() { return mGDIInterop; }
    PRBool UseGDIFontTableAccess() { return mGDIFontTableAccess; }

    virtual gfxFontFamily* FindFamily(const nsAString& aFamily);

    virtual void GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray);

private:
    friend class gfxDWriteFontFamily;

    nsresult GetFontSubstitutes();

    void GetDirectWriteSubstitutes();

    



    nsTArray<nsString> mNonExistingFonts;

    typedef nsDataHashtable<nsStringHashKey, nsRefPtr<gfxFontFamily> > FontTable;

    



    FontTable mFontSubstitutes;

    PRBool mInitialized;
    virtual nsresult DelayedInitFontList();

    
    PRBool mGDIFontTableAccess;
    nsRefPtr<IDWriteGdiInterop> mGDIInterop;
};


#endif 
