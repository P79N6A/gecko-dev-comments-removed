




































#ifndef GFX_USER_FONT_SET_H
#define GFX_USER_FONT_SET_H

#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFontUtils.h"
#include "nsRefPtrHashtable.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIFile.h"

class nsIURI;
class gfxMixedFontFamily;



struct gfxFontFaceSrc {
    PRPackedBool           mIsLocal;       
    nsString               mLocalName;     
    nsCOMPtr<nsIURI>       mURI;           
    nsCOMPtr<nsIURI>       mReferrer;      

    
    
    
    PRUint32               mFormatFlags;
};



class gfxFontLoaderContext {
public:
  gfxFontLoaderContext() { }
  virtual ~gfxFontLoaderContext() { }
};



class gfxUserFontData {
public:
    gfxUserFontData() { }
    virtual ~gfxUserFontData() { }
};




class gfxMixedFontFamily : public gfxFontFamily {

public:
    gfxMixedFontFamily(const nsAString& aName)
        : gfxFontFamily(aName)
    { }

    virtual ~gfxMixedFontFamily() { }

    void AddFontEntry(gfxFontEntry *aFontEntry) {
        nsRefPtr<gfxFontEntry> fe = aFontEntry;
        mAvailableFonts.AppendElement(fe);
    }

    void ReplaceFontEntry(gfxFontEntry *aOldFontEntry, gfxFontEntry *aNewFontEntry) 
    {
        PRUint32 numFonts = mAvailableFonts.Length();
        for (PRUint32 i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe == aOldFontEntry) {
                mAvailableFonts[i] = aNewFontEntry;
                return;
            }
        }
    }

    void RemoveFontEntry(gfxFontEntry *aFontEntry) 
    {
        PRUint32 numFonts = mAvailableFonts.Length();
        for (PRUint32 i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe == aFontEntry) {
                mAvailableFonts.RemoveElementAt(i);
                return;
            }
        }
    }

    
    PRBool AllLoaded() 
    {
        PRUint32 numFonts = mAvailableFonts.Length();
        for (PRUint32 i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe->mIsProxy)
                return PR_FALSE;
        }
        return PR_TRUE;
    }

    nsTArray<nsRefPtr<gfxFontEntry> >  mAvailableFonts;

protected:
    PRBool FindWeightsForStyle(gfxFontEntry* aFontsForWeights[], 
                               const gfxFontStyle& aFontStyle);

};

class gfxProxyFontEntry;

class THEBES_API gfxUserFontSet {

public:
    class LoaderContext;
    typedef nsresult (*LoaderCallback) (gfxFontEntry *aFontToLoad, 
                                        nsIURI *aSrcURL,
                                        nsIURI *aReferrerURI,
                                        LoaderContext *aContextData);

    class LoaderContext {
    public:
        LoaderContext(LoaderCallback aLoader)
            : mUserFontSet(nsnull), mLoaderProc(aLoader) { }
        virtual ~LoaderContext() { }

        gfxUserFontSet* mUserFontSet;
        LoaderCallback  mLoaderProc;
    };

    THEBES_INLINE_DECL_REFCOUNTING(gfxUserFontSet)

    gfxUserFontSet(LoaderContext *aContext);
    virtual ~gfxUserFontSet();

    enum {
        
        FLAG_FORMAT_OPENTYPE       = 1,
        FLAG_FORMAT_TRUETYPE       = 2,
        FLAG_FORMAT_TRUETYPE_AAT   = 4,
        FLAG_FORMAT_EOT            = 8,
        FLAG_FORMAT_SVG            = 16
    };

    enum LoadStatus {
        STATUS_LOADING = 0,
        STATUS_LOADED,
        STATUS_FORMAT_NOT_SUPPORTED,
        STATUS_ERROR,
        STATUS_END_OF_LIST
    };


    
    
    
    
    void AddFontFace(const nsAString& aFamilyName, 
                     const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList, 
                     PRUint32 aWeight = 0, 
                     PRUint32 aStretch = 0, 
                     PRUint32 aItalicStyle = 0, 
                     gfxSparseBitSet *aUnicodeRanges = nsnull);

    
    gfxFontEntry *FindFontEntry(const nsAString& aName, 
                                const gfxFontStyle& aFontStyle, PRBool& aNeedsBold);

    
    
    
    
    PRBool OnLoadComplete(gfxFontEntry *aFontToLoad, 
                          const PRUint8 *aFontData, PRUint32 aLength,
                          nsresult aDownloadStatus);

    
    
    PRUint64 GetGeneration() { return mGeneration; }

protected:
    
    
    LoadStatus LoadNext(gfxProxyFontEntry *aProxyEntry);

    
    void IncrementGeneration();

    
    void RemoveFamily(const nsAString& aFamilyName);

    
    nsRefPtrHashtable<nsStringHashKey, gfxMixedFontFamily> mFontFamilies;

    PRUint64        mGeneration;

    
    nsAutoPtr<LoaderContext> mLoaderContext;
};



class gfxProxyFontEntry : public gfxFontEntry {

public:
    gfxProxyFontEntry(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList, 
                      PRUint32 aWeight, 
                      PRUint32 aStretch, 
                      PRUint32 aItalicStyle, 
                      gfxSparseBitSet *aUnicodeRanges);

    virtual ~gfxProxyFontEntry();

    PRPackedBool                           mIsLoading;
    nsTArray<gfxFontFaceSrc>               mSrcList;
    PRUint32                               mSrcIndex; 
    gfxMixedFontFamily*                    mFamily;
};


#endif 
