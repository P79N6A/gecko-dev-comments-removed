




#ifndef GFX_USER_FONT_SET_H
#define GFX_USER_FONT_SET_H

#include "gfxFont.h"
#include "nsRefPtrHashtable.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsIScriptError.h"
#include "nsURIHashKey.h"

class nsFontFaceLoader;





struct gfxFontFaceSrc {
    bool                   mIsLocal;       

    
    bool                   mUseOriginPrincipal;

    
    
    
    uint32_t               mFormatFlags;

    nsString               mLocalName;     
    nsCOMPtr<nsIURI>       mURI;           
    nsCOMPtr<nsIURI>       mReferrer;      
    nsCOMPtr<nsIPrincipal> mOriginPrincipal; 
};

inline bool
operator==(const gfxFontFaceSrc& a, const gfxFontFaceSrc& b)
{
    bool equals;
    return (a.mIsLocal && b.mIsLocal &&
            a.mLocalName == b.mLocalName) ||
           (!a.mIsLocal && !b.mIsLocal &&
            a.mUseOriginPrincipal == b.mUseOriginPrincipal &&
            a.mFormatFlags == b.mFormatFlags &&
            NS_SUCCEEDED(a.mURI->Equals(b.mURI, &equals)) && equals &&
            NS_SUCCEEDED(a.mReferrer->Equals(b.mReferrer, &equals)) && equals &&
            a.mOriginPrincipal->Equals(b.mOriginPrincipal));
}







class gfxUserFontData {
public:
    gfxUserFontData()
        : mSrcIndex(0), mFormat(0), mMetaOrigLen(0),
          mCRC32(0), mLength(0), mPrivate(false)
    { }
    virtual ~gfxUserFontData() { }

    nsTArray<uint8_t> mMetadata;  
    nsCOMPtr<nsIURI>  mURI;       
    nsCOMPtr<nsIPrincipal> mPrincipal; 
    nsString          mLocalName; 
    nsString          mRealName;  
    uint32_t          mSrcIndex;  
    uint32_t          mFormat;    
    uint32_t          mMetaOrigLen; 
    uint32_t          mCRC32;     
    uint32_t          mLength;    
    bool              mPrivate;   
};




class gfxMixedFontFamily : public gfxFontFamily {
public:
    friend class gfxUserFontSet;

    explicit gfxMixedFontFamily(const nsAString& aName)
        : gfxFontFamily(aName) { }

    virtual ~gfxMixedFontFamily() { }

    
    
    
    void AddFontEntry(gfxFontEntry *aFontEntry) {
        
        
        
        mAvailableFonts.AppendElement(aFontEntry);
        uint32_t i = mAvailableFonts.Length() - 1;
        while (i > 0) {
            if (mAvailableFonts[--i] == aFontEntry) {
                mAvailableFonts.RemoveElementAt(i);
                break;
            }
        }
        if (aFontEntry->mFamilyName.IsEmpty()) {
            aFontEntry->mFamilyName = Name();
        } else {
#ifdef DEBUG
            nsString thisName = Name();
            nsString entryName = aFontEntry->mFamilyName;
            ToLowerCase(thisName);
            ToLowerCase(entryName);
            MOZ_ASSERT(thisName.Equals(entryName));
#endif
        }
        ResetCharacterMap();
    }

    
    void ReplaceFontEntry(gfxFontEntry *aProxyFontEntry,
                          gfxFontEntry *aRealFontEntry) {
        uint32_t numFonts = mAvailableFonts.Length();
        uint32_t i;
        for (i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            if (fe == aProxyFontEntry) {
                
                
                mAvailableFonts[i] = aRealFontEntry;
                if (aRealFontEntry->mFamilyName.IsEmpty()) {
                    aRealFontEntry->mFamilyName = Name();
                } else {
#ifdef DEBUG
                  nsString thisName = Name();
                  nsString entryName = aRealFontEntry->mFamilyName;
                  ToLowerCase(thisName);
                  ToLowerCase(entryName);
                  MOZ_ASSERT(thisName.Equals(entryName));
#endif
                }
                break;
            }
        }
        NS_ASSERTION(i < numFonts, "font entry not found in family!");
        ResetCharacterMap();
    }

    
    void DetachFontEntries() {
        mAvailableFonts.Clear();
    }
};

class gfxProxyFontEntry;

class gfxUserFontSet {
    friend class gfxProxyFontEntry;

public:

    NS_INLINE_DECL_REFCOUNTING(gfxUserFontSet)

    gfxUserFontSet();

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


    
    
    
    
    
    
    already_AddRefed<gfxProxyFontEntry> CreateFontFace(
                              const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                              uint32_t aWeight,
                              int32_t aStretch,
                              uint32_t aItalicStyle,
                              const nsTArray<gfxFontFeature>& aFeatureSettings,
                              uint32_t aLanguageOverride,
                              gfxSparseBitSet* aUnicodeRanges);

    
    
    already_AddRefed<gfxProxyFontEntry> FindOrCreateFontFace(
                               const nsAString& aFamilyName,
                               const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                               uint32_t aWeight,
                               int32_t aStretch,
                               uint32_t aItalicStyle,
                               const nsTArray<gfxFontFeature>& aFeatureSettings,
                               uint32_t aLanguageOverride,
                               gfxSparseBitSet* aUnicodeRanges);

    
    void AddFontFace(const nsAString& aFamilyName, gfxFontEntry* aFontEntry);

    
    bool HasFamily(const nsAString& aFamilyName) const
    {
        return LookupFamily(aFamilyName) != nullptr;
    }

    
    
    gfxMixedFontFamily* LookupFamily(const nsAString& aName) const;

    
    
    gfxFontEntry *FindFontEntry(gfxFontFamily *aFamily,
                                const gfxFontStyle& aFontStyle,
                                bool& aNeedsBold,
                                bool& aWaitForUserFont);

    
    
    
    
    gfxFontFamily *FindFamilyFor(gfxFontEntry *aFontEntry) const;

    
    
    
    virtual nsresult CheckFontLoad(const gfxFontFaceSrc *aFontFaceSrc,
                                   nsIPrincipal **aPrincipal,
                                   bool *aBypassCache) = 0;

    
    
    virtual nsresult StartLoad(gfxMixedFontFamily *aFamily,
                               gfxProxyFontEntry *aProxy,
                               const gfxFontFaceSrc *aFontFaceSrc) = 0;

    
    
    
    
    
    
    bool OnLoadComplete(gfxMixedFontFamily *aFamily,
                        gfxProxyFontEntry *aProxy,
                        const uint8_t *aFontData, uint32_t aLength,
                        nsresult aDownloadStatus);

    
    
    
    virtual void ReplaceFontEntry(gfxMixedFontFamily *aFamily,
                                  gfxProxyFontEntry *aProxy,
                                  gfxFontEntry *aFontEntry) = 0;

    
    
    uint64_t GetGeneration() { return mGeneration; }

    
    void IncrementGeneration();

    
    void RebuildLocalRules();

    class UserFontCache {
    public:
        
        
        typedef enum {
            kDiscardable,
            kPersistent
        } EntryPersistence;

        
        
        
        
        
        
        static void CacheFont(gfxFontEntry *aFontEntry,
                              EntryPersistence aPersistence = kDiscardable);

        
        
        static void ForgetFont(gfxFontEntry *aFontEntry);

        
        
        
        
        
        static gfxFontEntry* GetFont(nsIURI            *aSrcURI,
                                     nsIPrincipal      *aPrincipal,
                                     gfxProxyFontEntry *aProxy,
                                     bool               aPrivate);

        
        static void Shutdown();

#ifdef DEBUG_USERFONT_CACHE
        
        static void Dump();
#endif

    private:
        
        
        class Flusher : public nsIObserver
        {
            virtual ~Flusher() {}
        public:
            NS_DECL_ISUPPORTS
            NS_DECL_NSIOBSERVER
            Flusher() {}
        };

        
        
        
        
        
        
        struct Key {
            nsCOMPtr<nsIURI>        mURI;
            nsCOMPtr<nsIPrincipal>  mPrincipal; 
            gfxFontEntry           *mFontEntry;
            uint32_t                mCRC32;
            uint32_t                mLength;
            bool                    mPrivate;
            EntryPersistence        mPersistence;

            Key(nsIURI* aURI, nsIPrincipal* aPrincipal,
                gfxFontEntry* aFontEntry, bool aPrivate,
                EntryPersistence aPersistence = kDiscardable)
                : mURI(aURI),
                  mPrincipal(aPrincipal),
                  mFontEntry(aFontEntry),
                  mCRC32(0),
                  mLength(0),
                  mPrivate(aPrivate),
                  mPersistence(aPersistence)
            { }

            Key(uint32_t aCRC32, uint32_t aLength,
                gfxFontEntry* aFontEntry, bool aPrivate,
                EntryPersistence aPersistence = kDiscardable)
                : mURI(nullptr),
                  mPrincipal(nullptr),
                  mFontEntry(aFontEntry),
                  mCRC32(aCRC32),
                  mLength(aLength),
                  mPrivate(aPrivate),
                  mPersistence(aPersistence)
            { }
        };

        class Entry : public PLDHashEntryHdr {
        public:
            typedef const Key& KeyType;
            typedef const Key* KeyTypePointer;

            explicit Entry(KeyTypePointer aKey)
                : mURI(aKey->mURI),
                  mPrincipal(aKey->mPrincipal),
                  mCRC32(aKey->mCRC32),
                  mLength(aKey->mLength),
                  mFontEntry(aKey->mFontEntry),
                  mPrivate(aKey->mPrivate),
                  mPersistence(aKey->mPersistence)
            { }

            Entry(const Entry& aOther)
                : mURI(aOther.mURI),
                  mPrincipal(aOther.mPrincipal),
                  mCRC32(aOther.mCRC32),
                  mLength(aOther.mLength),
                  mFontEntry(aOther.mFontEntry),
                  mPrivate(aOther.mPrivate),
                  mPersistence(aOther.mPersistence)
            { }

            ~Entry() { }

            bool KeyEquals(const KeyTypePointer aKey) const;

            static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

            static PLDHashNumber HashKey(const KeyTypePointer aKey) {
                if (aKey->mLength) {
                    return aKey->mCRC32;
                }
                uint32_t principalHash = 0;
                if (aKey->mPrincipal) {
                    aKey->mPrincipal->GetHashValue(&principalHash);
                }
                return mozilla::HashGeneric(principalHash + int(aKey->mPrivate),
                                            nsURIHashKey::HashKey(aKey->mURI),
                                            HashFeatures(aKey->mFontEntry->mFeatureSettings),
                                            mozilla::HashString(aKey->mFontEntry->mFamilyName),
                                            ((uint32_t)aKey->mFontEntry->mItalic |
                                             (aKey->mFontEntry->mWeight << 1) |
                                             (aKey->mFontEntry->mStretch << 10) ) ^
                                             aKey->mFontEntry->mLanguageOverride);
            }

            enum { ALLOW_MEMMOVE = false };

            gfxFontEntry* GetFontEntry() const { return mFontEntry; }

            static PLDHashOperator
            RemoveUnlessPersistent(Entry* aEntry, void* aUserData);
            static PLDHashOperator
            RemoveIfPrivate(Entry* aEntry, void* aUserData);
            static PLDHashOperator
            RemoveIfMatches(Entry* aEntry, void* aUserData);
            static PLDHashOperator
            DisconnectSVG(Entry* aEntry, void* aUserData);

#ifdef DEBUG_USERFONT_CACHE
            static PLDHashOperator DumpEntry(Entry* aEntry, void* aUserData);
#endif

        private:
            static uint32_t
            HashFeatures(const nsTArray<gfxFontFeature>& aFeatures) {
                return mozilla::HashBytes(aFeatures.Elements(),
                                          aFeatures.Length() * sizeof(gfxFontFeature));
            }

            nsCOMPtr<nsIURI>       mURI;
            nsCOMPtr<nsIPrincipal> mPrincipal; 

            uint32_t               mCRC32;
            uint32_t               mLength;

            
            
            
            gfxFontEntry          *mFontEntry;

            
            bool                   mPrivate;

            
            EntryPersistence       mPersistence;
        };

        static nsTHashtable<Entry> *sUserFonts;
    };

protected:
    
    virtual ~gfxUserFontSet();

    
    virtual bool GetPrivateBrowsing() = 0;

    
    virtual nsresult SyncLoadFontData(gfxProxyFontEntry *aFontToLoad,
                                      const gfxFontFaceSrc *aFontFaceSrc,
                                      uint8_t* &aBuffer,
                                      uint32_t &aBufferLength) = 0;

    
    virtual nsresult LogMessage(gfxMixedFontFamily *aFamily,
                                gfxProxyFontEntry *aProxy,
                                const char *aMessage,
                                uint32_t aFlags = nsIScriptError::errorFlag,
                                nsresult aStatus = NS_OK) = 0;

    
    virtual void DoRebuildUserFontSet() = 0;

    
    gfxProxyFontEntry* FindExistingProxyEntry(
                                   gfxMixedFontFamily* aFamily,
                                   const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                                   uint32_t aWeight,
                                   int32_t aStretch,
                                   uint32_t aItalicStyle,
                                   const nsTArray<gfxFontFeature>& aFeatureSettings,
                                   uint32_t aLanguageOverride,
                                   gfxSparseBitSet* aUnicodeRanges);

    
    
    gfxMixedFontFamily* GetFamily(const nsAString& aFamilyName);

    
    nsRefPtrHashtable<nsStringHashKey, gfxMixedFontFamily> mFontFamilies;

    uint64_t        mGeneration;

    
    bool mLocalRulesUsed;

    static PRLogModuleInfo* GetUserFontsLog();
};



class gfxProxyFontEntry : public gfxFontEntry {
    friend class gfxUserFontSet;
    friend class nsUserFontSet;
    friend class nsFontFaceLoader;

public:
    enum LoadStatus {
        STATUS_LOADING = 0,
        STATUS_LOADED,
        STATUS_FORMAT_NOT_SUPPORTED,
        STATUS_ERROR,
        STATUS_END_OF_LIST
    };

    gfxProxyFontEntry(gfxUserFontSet *aFontSet,
                      const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                      uint32_t aWeight,
                      int32_t aStretch,
                      uint32_t aItalicStyle,
                      const nsTArray<gfxFontFeature>& aFeatureSettings,
                      uint32_t aLanguageOverride,
                      gfxSparseBitSet *aUnicodeRanges);

    virtual ~gfxProxyFontEntry();

    
    bool Matches(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                 uint32_t aWeight,
                 int32_t aStretch,
                 uint32_t aItalicStyle,
                 const nsTArray<gfxFontFeature>& aFeatureSettings,
                 uint32_t aLanguageOverride,
                 gfxSparseBitSet *aUnicodeRanges);

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold);

protected:
    const uint8_t* SanitizeOpenTypeData(gfxMixedFontFamily *aFamily,
                                        const uint8_t* aData,
                                        uint32_t aLength,
                                        uint32_t& aSaneLength,
                                        bool aIsCompressed);

    
    
    
    LoadStatus LoadNext(gfxMixedFontFamily *aFamily,
                        bool& aLocalRulesUsed);

    
    
    
    
    gfxFontEntry* LoadFont(gfxMixedFontFamily *aFamily,
                           const uint8_t *aFontData, uint32_t &aLength);

    
    void StoreUserFontData(gfxFontEntry*      aFontEntry,
                           bool               aPrivate,
                           const nsAString&   aOriginalName,
                           FallibleTArray<uint8_t>* aMetadata,
                           uint32_t           aMetaOrigLen);

    static bool OTSMessage(void *aUserData, const char *format, ...);

    
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
    gfxUserFontSet          *mFontSet; 
    nsCOMPtr<nsIPrincipal>   mPrincipal;
};


#endif
