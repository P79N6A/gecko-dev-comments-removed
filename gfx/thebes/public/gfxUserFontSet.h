




































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
};

class gfxProxyFontEntry;

class THEBES_API gfxUserFontSet {

public:

    THEBES_INLINE_DECL_REFCOUNTING(gfxUserFontSet)

    gfxUserFontSet();
    virtual ~gfxUserFontSet();

    enum {
        
        
        FLAG_FORMAT_UNKNOWN        = 1,
        FLAG_FORMAT_OPENTYPE       = 1 << 1,
        FLAG_FORMAT_TRUETYPE       = 1 << 2,
        FLAG_FORMAT_TRUETYPE_AAT   = 1 << 3,
        FLAG_FORMAT_EOT            = 1 << 4,
        FLAG_FORMAT_SVG            = 1 << 5,
        
        
        FLAG_FORMAT_NOT_USED       = ~((1 << 6)-1)
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

    
    PRBool HasFamily(const nsAString& aFamilyName) const
    {
        return GetFamily(aFamilyName) != nsnull;
    }

    
    gfxFontEntry *FindFontEntry(const nsAString& aName, 
                                const gfxFontStyle& aFontStyle, 
                                PRBool& aNeedsBold);
                                
    
    
    virtual nsresult StartLoad(gfxFontEntry *aFontToLoad, 
                               const gfxFontFaceSrc *aFontFaceSrc) = 0;

    
    
    
    
    PRBool OnLoadComplete(gfxFontEntry *aFontToLoad, nsISupports *aLoader,
                          const PRUint8 *aFontData, PRUint32 aLength,
                          nsresult aDownloadStatus);

    
    
    PRUint64 GetGeneration() { return mGeneration; }

protected:
    
    
    LoadStatus LoadNext(gfxProxyFontEntry *aProxyEntry);

    
    void IncrementGeneration();

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
                      gfxSparseBitSet *aUnicodeRanges);

    virtual ~gfxProxyFontEntry();

    PRPackedBool                           mIsLoading;
    nsTArray<gfxFontFaceSrc>               mSrcList;
    PRUint32                               mSrcIndex; 
};


#endif 
