




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
class nsFontFaceLoader;



struct gfxFontFaceSrc {
    bool                   mIsLocal;       

    
    bool                   mUseOriginPrincipal;

    
    
    
    uint32_t               mFormatFlags;

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

    nsTArray<uint8_t> mMetadata;  
    nsCOMPtr<nsIURI>  mURI;       
    nsString          mLocalName; 
    nsString          mRealName;  
    uint32_t          mSrcIndex;  
    uint32_t          mFormat;    
    uint32_t          mMetaOrigLen; 
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
        ResetCharacterMap();
    }

    void ReplaceFontEntry(gfxFontEntry *aOldFontEntry,
                          gfxFontEntry *aNewFontEntry) {
        uint32_t numFonts = mAvailableFonts.Length();
        for (uint32_t i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe == aOldFontEntry) {
                aOldFontEntry->SetFamily(nullptr);
                
                
                mAvailableFonts[i] = aNewFontEntry;
                aNewFontEntry->SetFamily(this);
                break;
            }
        }
        ResetCharacterMap();
    }

    void RemoveFontEntry(gfxFontEntry *aFontEntry) {
        uint32_t numFonts = mAvailableFonts.Length();
        for (uint32_t i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe == aFontEntry) {
                aFontEntry->SetFamily(nullptr);
                mAvailableFonts.RemoveElementAt(i);
                break;
            }
        }
        ResetCharacterMap();
    }

    
    
    
    void DetachFontEntries() {
        uint32_t i = mAvailableFonts.Length();
        while (i--) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe) {
                fe->SetFamily(nullptr);
            }
        }
        mAvailableFonts.Clear();
    }

    
    bool AllLoaded() 
    {
        uint32_t numFonts = mAvailableFonts.Length();
        for (uint32_t i = 0; i < numFonts; i++) {
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
                              uint32_t aWeight,
                              uint32_t aStretch,
                              uint32_t aItalicStyle,
                              const nsTArray<gfxFontFeature>& aFeatureSettings,
                              const nsString& aLanguageOverride,
                              gfxSparseBitSet *aUnicodeRanges = nullptr);

    
    void AddFontFace(const nsAString& aFamilyName, gfxFontEntry* aFontEntry);

    
    bool HasFamily(const nsAString& aFamilyName) const
    {
        return GetFamily(aFamilyName) != nullptr;
    }

    
    gfxFontEntry *FindFontEntry(const nsAString& aName,
                                const gfxFontStyle& aFontStyle,
                                bool& aFoundFamily,
                                bool& aNeedsBold,
                                bool& aWaitForUserFont);
                                
    
    
    virtual nsresult StartLoad(gfxProxyFontEntry *aProxy, 
                               const gfxFontFaceSrc *aFontFaceSrc) = 0;

    
    
    
    
    
    
    bool OnLoadComplete(gfxProxyFontEntry *aProxy,
                          const uint8_t *aFontData, uint32_t aLength,
                          nsresult aDownloadStatus);

    
    
    
    virtual void ReplaceFontEntry(gfxProxyFontEntry *aProxy,
                                  gfxFontEntry *aFontEntry) = 0;

    
    
    uint64_t GetGeneration() { return mGeneration; }

    
    void IncrementGeneration();

protected:
    
    
    LoadStatus LoadNext(gfxProxyFontEntry *aProxyEntry);

    
    
    
    
    gfxFontEntry* LoadFont(gfxProxyFontEntry *aProxy,
                           const uint8_t *aFontData, uint32_t &aLength);

    
    virtual nsresult SyncLoadFontData(gfxProxyFontEntry *aFontToLoad,
                                      const gfxFontFaceSrc *aFontFaceSrc,
                                      uint8_t* &aBuffer,
                                      uint32_t &aBufferLength)
    {
        
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    gfxMixedFontFamily *GetFamily(const nsAString& aName) const;

    
    virtual nsresult LogMessage(gfxProxyFontEntry *aProxy,
                                const char *aMessage,
                                uint32_t aFlags = nsIScriptError::errorFlag,
                                nsresult aStatus = NS_OK) = 0;

    const uint8_t* SanitizeOpenTypeData(gfxProxyFontEntry *aProxy,
                                        const uint8_t* aData,
                                        uint32_t aLength,
                                        uint32_t& aSaneLength,
                                        bool aIsCompressed);

#ifdef MOZ_OTS_REPORT_ERRORS
    static bool OTSMessage(void *aUserData, const char *format, ...);
#endif

    
    nsRefPtrHashtable<nsStringHashKey, gfxMixedFontFamily> mFontFamilies;

    uint64_t        mGeneration;

    static PRLogModuleInfo *sUserFontsLog;

private:
    static void CopyWOFFMetadata(const uint8_t* aFontData,
                                 uint32_t aLength,
                                 nsTArray<uint8_t>* aMetadata,
                                 uint32_t* aMetaOrigLen);
};



class gfxProxyFontEntry : public gfxFontEntry {
    friend class gfxUserFontSet;

public:
    gfxProxyFontEntry(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                      gfxMixedFontFamily *aFamily,
                      uint32_t aWeight,
                      uint32_t aStretch,
                      uint32_t aItalicStyle,
                      const nsTArray<gfxFontFeature>& aFeatureSettings,
                      uint32_t aLanguageOverride,
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
    bool                     mUnsupportedFormat;

    nsTArray<gfxFontFaceSrc> mSrcList;
    uint32_t                 mSrcIndex; 
    nsFontFaceLoader        *mLoader; 
};


#endif 
