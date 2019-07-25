




































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
#include "nsIScriptError.h"

class nsIURI;
class gfxMixedFontFamily;



struct gfxFontFaceSrc {
    bool                   mIsLocal;       

    
    bool                   mUseOriginPrincipal;

    
    
    
    PRUint32               mFormatFlags;

    nsString               mLocalName;     
    nsCOMPtr<nsIURI>       mURI;           
    nsCOMPtr<nsIURI>       mReferrer;      
    nsCOMPtr<nsISupports>  mOriginPrincipal; 
    
};







class gfxUserFontData {
public:
    gfxUserFontData()
        : mSrcIndex(0), mFormat(0), mMetaOrigLen(0)
    { }
    virtual ~gfxUserFontData() { }

    nsTArray<PRUint8> mMetadata;  
    nsCOMPtr<nsIURI>  mURI;       
    nsString          mLocalName; 
    nsString          mRealName;  
    PRUint32          mSrcIndex;  
    PRUint32          mFormat;    
    PRUint32          mMetaOrigLen; 
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
                aOldFontEntry->SetFamily(nsnull);
                
                
                mAvailableFonts[i] = aNewFontEntry;
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

    
    bool AllLoaded() 
    {
        PRUint32 numFonts = mAvailableFonts.Length();
        for (PRUint32 i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe->mIsProxy)
                return false;
        }
        return true;
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

    
    bool HasFamily(const nsAString& aFamilyName) const
    {
        return GetFamily(aFamilyName) != nsnull;
    }

    
    gfxFontEntry *FindFontEntry(const nsAString& aName,
                                const gfxFontStyle& aFontStyle,
                                bool& aFoundFamily,
                                bool& aNeedsBold,
                                bool& aWaitForUserFont);
                                
    
    
    virtual nsresult StartLoad(gfxProxyFontEntry *aProxy, 
                               const gfxFontFaceSrc *aFontFaceSrc) = 0;

    
    
    
    
    
    
    bool OnLoadComplete(gfxProxyFontEntry *aProxy,
                          const PRUint8 *aFontData, PRUint32 aLength,
                          nsresult aDownloadStatus);

    
    
    
    virtual void ReplaceFontEntry(gfxProxyFontEntry *aProxy,
                                  gfxFontEntry *aFontEntry) = 0;

    
    
    PRUint64 GetGeneration() { return mGeneration; }

    
    void IncrementGeneration();

protected:
    
    
    LoadStatus LoadNext(gfxProxyFontEntry *aProxyEntry);

    gfxMixedFontFamily *GetFamily(const nsAString& aName) const;

    
    virtual nsresult LogMessage(gfxProxyFontEntry *aProxy,
                                const char *aMessage,
                                PRUint32 aFlags = nsIScriptError::errorFlag,
                                nsresult aStatus = 0) = 0;

    
    nsRefPtrHashtable<nsStringHashKey, gfxMixedFontFamily> mFontFamilies;

    PRUint64        mGeneration;

    static PRLogModuleInfo *sUserFontsLog;
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

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold);

    
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
