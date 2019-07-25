




































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
#include "nsISupportsImpl.h"

class nsIURI;
class gfxMixedFontFamily;



struct gfxFontFaceSrc {
    PRPackedBool           mIsLocal;       

    
    PRPackedBool           mUseOriginPrincipal;

    
    
    
    PRUint32               mFormatFlags;

    nsString               mLocalName;     
    nsCOMPtr<nsIURI>       mURI;           
    nsCOMPtr<nsIURI>       mReferrer;      
    nsCOMPtr<nsISupports>  mOriginPrincipal; 
    
};



class gfxUserFontData {
public:
    gfxUserFontData() { }
    virtual ~gfxUserFontData() { }
};




class gfxMixedFontFamily : public gfxFontFamily {
public:
    friend class gfxUserFontSet;

    gfxMixedFontFamily(const nsAString& aName)
        : gfxFontFamily(aName) { }

    virtual ~gfxMixedFontFamily() { }

    void AddFontEntry(gfxFontEntry *aFontEntry) {
        nsRefPtr<gfxFontEntry> fe = aFontEntry;
        mAvailableFonts.AppendElement(fe);
        aFontEntry->SetFamily(this);
    }

    void ReplaceFontEntry(gfxFontEntry *aOldFontEntry, gfxFontEntry *aNewFontEntry) 
    {
        PRUint32 numFonts = mAvailableFonts.Length();
        for (PRUint32 i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe == aOldFontEntry) {
                mAvailableFonts[i] = aNewFontEntry;
                aOldFontEntry->SetFamily(nsnull);
                aNewFontEntry->SetFamily(this);
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
                aFontEntry->SetFamily(nsnull);
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
};

class gfxProxyFontEntry;

class THEBES_API gfxUserFontSet {

public:

    NS_INLINE_DECL_REFCOUNTING(gfxUserFontSet)

    gfxUserFontSet();
    virtual ~gfxUserFontSet();

    enum {
        
        
        FLAG_FORMAT_UNKNOWN        = 1,
        FLAG_FORMAT_OPENTYPE       = 1 << 1,
        FLAG_FORMAT_TRUETYPE       = 1 << 2,
        FLAG_FORMAT_TRUETYPE_AAT   = 1 << 3,
        FLAG_FORMAT_EOT            = 1 << 4,
        FLAG_FORMAT_SVG            = 1 << 5,
        FLAG_FORMAT_WOFF           = 1 << 6,

        
        FLAG_FORMAT_NOT_USED       = ~((1 << 7)-1)
    };

    enum LoadStatus {
        STATUS_LOADING = 0,
        STATUS_LOADED,
        STATUS_FORMAT_NOT_SUPPORTED,
        STATUS_ERROR,
        STATUS_END_OF_LIST
    };


    
    
    
    
    gfxFontEntry *AddFontFace(const nsAString& aFamilyName,
                              const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                              PRUint32 aWeight,
                              PRUint32 aStretch,
                              PRUint32 aItalicStyle,
                              const nsString& aFeatureSettings,
                              const nsString& aLanguageOverride,
                              gfxSparseBitSet *aUnicodeRanges = nsnull);

    
    void AddFontFace(const nsAString& aFamilyName, gfxFontEntry* aFontEntry);

    
    PRBool HasFamily(const nsAString& aFamilyName) const
    {
        return GetFamily(aFamilyName) != nsnull;
    }

    
    gfxFontEntry *FindFontEntry(const nsAString& aName,
                                const gfxFontStyle& aFontStyle,
                                PRBool& aFoundFamily,
                                PRBool& aNeedsBold,
                                PRBool& aWaitForUserFont);
                                
    
    
    virtual nsresult StartLoad(gfxFontEntry *aFontToLoad, 
                               const gfxFontFaceSrc *aFontFaceSrc) = 0;

    
    
    
    
    
    
    PRBool OnLoadComplete(gfxFontEntry *aFontToLoad,
                          const PRUint8 *aFontData, PRUint32 aLength,
                          nsresult aDownloadStatus);

    
    
    
    virtual void ReplaceFontEntry(gfxProxyFontEntry *aProxy,
                                  gfxFontEntry *aFontEntry) = 0;

    
    
    PRUint64 GetGeneration() { return mGeneration; }

    
    void IncrementGeneration();

protected:
    
    
    LoadStatus LoadNext(gfxProxyFontEntry *aProxyEntry);

    gfxMixedFontFamily *GetFamily(const nsAString& aName) const;

    
    void RemoveFamily(const nsAString& aFamilyName);

    
    nsRefPtrHashtable<nsStringHashKey, gfxMixedFontFamily> mFontFamilies;

    PRUint64        mGeneration;
};



class gfxProxyFontEntry : public gfxFontEntry {
    friend class gfxUserFontSet;

public:
    gfxProxyFontEntry(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                      gfxMixedFontFamily *aFamily,
                      PRUint32 aWeight,
                      PRUint32 aStretch,
                      PRUint32 aItalicStyle,
                      const nsTArray<gfxFontFeature>& aFeatureSettings,
                      PRUint32 aLanguageOverride,
                      gfxSparseBitSet *aUnicodeRanges);

    virtual ~gfxProxyFontEntry();

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, PRBool aNeedsBold);

    
    enum LoadingState {
        NOT_LOADING = 0,     
        LOADING_STARTED,     
        LOADING_ALMOST_DONE, 
                             
        LOADING_SLOWLY,      
                             
        LOADING_FAILED       
    };
    LoadingState             mLoadingState;

    nsTArray<gfxFontFaceSrc> mSrcList;
    PRUint32                 mSrcIndex; 
};


#endif 
