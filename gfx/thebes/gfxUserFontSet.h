




#ifndef GFX_USER_FONT_SET_H
#define GFX_USER_FONT_SET_H

#include "gfxFont.h"
#include "gfxFontFamilyList.h"
#include "nsRefPtrHashtable.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsIScriptError.h"
#include "nsURIHashKey.h"
#include "mozilla/net/ReferrerPolicy.h"

class nsFontFaceLoader;



class gfxFontFaceBufferSource
{
  NS_INLINE_DECL_REFCOUNTING(gfxFontFaceBufferSource)
public:
  virtual void TakeBuffer(uint8_t*& aBuffer, uint32_t& aLength) = 0;

protected:
  virtual ~gfxFontFaceBufferSource() {}
};



struct gfxFontFaceSrc {

    enum SourceType {
        eSourceType_Local,
        eSourceType_URL,
        eSourceType_Buffer
    };

    SourceType             mSourceType;

    
    bool                   mUseOriginPrincipal;

    
    
    
    uint32_t               mFormatFlags;

    nsString               mLocalName;     
    nsCOMPtr<nsIURI>       mURI;           
    nsCOMPtr<nsIURI>       mReferrer;      
    mozilla::net::ReferrerPolicy mReferrerPolicy;
    nsCOMPtr<nsIPrincipal> mOriginPrincipal; 

    nsRefPtr<gfxFontFaceBufferSource> mBuffer;
};

inline bool
operator==(const gfxFontFaceSrc& a, const gfxFontFaceSrc& b)
{
    if (a.mSourceType != b.mSourceType) {
        return false;
    }
    switch (a.mSourceType) {
        case gfxFontFaceSrc::eSourceType_Local:
            return a.mLocalName == b.mLocalName;
        case gfxFontFaceSrc::eSourceType_URL: {
            bool equals;
            return a.mUseOriginPrincipal == b.mUseOriginPrincipal &&
                   a.mFormatFlags == b.mFormatFlags &&
                   NS_SUCCEEDED(a.mURI->Equals(b.mURI, &equals)) && equals &&
                   NS_SUCCEEDED(a.mReferrer->Equals(b.mReferrer, &equals)) &&
                     equals &&
                   a.mReferrerPolicy == b.mReferrerPolicy &&
                   a.mOriginPrincipal->Equals(b.mOriginPrincipal);
        }
        case gfxFontFaceSrc::eSourceType_Buffer:
            return a.mBuffer == b.mBuffer;
    }
    NS_WARNING("unexpected mSourceType");
    return false;
}







class gfxUserFontData {
public:
    gfxUserFontData()
        : mSrcIndex(0), mFormat(0), mMetaOrigLen(0),
          mCRC32(0), mLength(0), mCompression(kUnknownCompression),
          mPrivate(false), mIsBuffer(false)
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
    uint8_t           mCompression; 
    bool              mPrivate;   
    bool              mIsBuffer;  

    enum {
        kUnknownCompression = 0,
        kZlibCompression = 1,
        kBrotliCompression = 2
    };
};




class gfxUserFontFamily : public gfxFontFamily {
public:
    friend class gfxUserFontSet;

    explicit gfxUserFontFamily(const nsAString& aName)
        : gfxFontFamily(aName) { }

    virtual ~gfxUserFontFamily() { }

    
    void AddFontEntry(gfxFontEntry* aFontEntry) {
        
        nsRefPtr<gfxFontEntry> fe = aFontEntry;
        
        mAvailableFonts.RemoveElement(aFontEntry);
        mAvailableFonts.AppendElement(aFontEntry);

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

    
    void DetachFontEntries() {
        mAvailableFonts.Clear();
    }
};

class gfxUserFontEntry;
class gfxOTSContext;

class gfxUserFontSet {
    friend class gfxUserFontEntry;
    friend class gfxOTSContext;

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
        FLAG_FORMAT_WOFF2          = 1 << 7,

        
        FLAG_FORMATS_COMMON        = FLAG_FORMAT_OPENTYPE |
                                     FLAG_FORMAT_TRUETYPE |
                                     FLAG_FORMAT_WOFF     |
                                     FLAG_FORMAT_WOFF2,

        
        FLAG_FORMAT_NOT_USED       = ~((1 << 8)-1)
    };


    
    
    
    
    
    
    virtual already_AddRefed<gfxUserFontEntry> CreateUserFontEntry(
                              const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                              uint32_t aWeight,
                              int32_t aStretch,
                              uint32_t aItalicStyle,
                              const nsTArray<gfxFontFeature>& aFeatureSettings,
                              uint32_t aLanguageOverride,
                              gfxSparseBitSet* aUnicodeRanges) = 0;

    
    
    already_AddRefed<gfxUserFontEntry> FindOrCreateUserFontEntry(
                               const nsAString& aFamilyName,
                               const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                               uint32_t aWeight,
                               int32_t aStretch,
                               uint32_t aItalicStyle,
                               const nsTArray<gfxFontFeature>& aFeatureSettings,
                               uint32_t aLanguageOverride,
                               gfxSparseBitSet* aUnicodeRanges);

    
    void AddUserFontEntry(const nsAString& aFamilyName,
                          gfxUserFontEntry* aUserFontEntry);

    
    bool HasFamily(const nsAString& aFamilyName) const
    {
        return LookupFamily(aFamilyName) != nullptr;
    }

    
    
    gfxUserFontFamily* LookupFamily(const nsAString& aName) const;

    
    bool ContainsUserFontSetFonts(const mozilla::FontFamilyList& aFontList) const;

    
    
    
    gfxUserFontEntry* FindUserFontEntryAndLoad(gfxFontFamily* aFamily,
                                               const gfxFontStyle& aFontStyle,
                                               bool& aNeedsBold,
                                               bool& aWaitForUserFont);

    
    
    
    virtual nsresult CheckFontLoad(const gfxFontFaceSrc* aFontFaceSrc,
                                   nsIPrincipal** aPrincipal,
                                   bool* aBypassCache) = 0;

    
    
    virtual nsresult StartLoad(gfxUserFontEntry* aUserFontEntry,
                               const gfxFontFaceSrc* aFontFaceSrc) = 0;

    
    
    uint64_t GetGeneration() { return mGeneration; }

    
    void IncrementGeneration(bool aIsRebuild = false);

    
    
    
    uint64_t GetRebuildGeneration() { return mRebuildGeneration; }

    
    void RebuildLocalRules();

    class UserFontCache {
    public:
        
        
        typedef enum {
            kDiscardable,
            kPersistent
        } EntryPersistence;

        
        
        
        
        
        
        static void CacheFont(gfxFontEntry* aFontEntry,
                              EntryPersistence aPersistence = kDiscardable);

        
        
        static void ForgetFont(gfxFontEntry* aFontEntry);

        
        
        
        
        
        static gfxFontEntry* GetFont(nsIURI* aSrcURI,
                                     nsIPrincipal* aPrincipal,
                                     gfxUserFontEntry* aUserFontEntry,
                                     bool              aPrivate);

        
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
            
            
            gfxFontEntry* MOZ_NON_OWNING_REF mFontEntry;
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

            
            
            
            gfxFontEntry* MOZ_NON_OWNING_REF mFontEntry;

            
            bool                   mPrivate;

            
            EntryPersistence       mPersistence;
        };

        static nsTHashtable<Entry>* sUserFonts;
    };

    void SetLocalRulesUsed() {
        mLocalRulesUsed = true;
    }

    static PRLogModuleInfo* GetUserFontsLog();

protected:
    
    virtual ~gfxUserFontSet();

    
    virtual bool GetPrivateBrowsing() = 0;

    
    virtual nsresult SyncLoadFontData(gfxUserFontEntry* aFontToLoad,
                                      const gfxFontFaceSrc* aFontFaceSrc,
                                      uint8_t* &aBuffer,
                                      uint32_t &aBufferLength) = 0;

    
    virtual nsresult LogMessage(gfxUserFontEntry* aUserFontEntry,
                                const char* aMessage,
                                uint32_t aFlags = nsIScriptError::errorFlag,
                                nsresult aStatus = NS_OK) = 0;

    
    virtual void DoRebuildUserFontSet() = 0;

    
    gfxUserFontEntry* FindExistingUserFontEntry(
                                   gfxUserFontFamily* aFamily,
                                   const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                                   uint32_t aWeight,
                                   int32_t aStretch,
                                   uint32_t aItalicStyle,
                                   const nsTArray<gfxFontFeature>& aFeatureSettings,
                                   uint32_t aLanguageOverride,
                                   gfxSparseBitSet* aUnicodeRanges);

    
    
    gfxUserFontFamily* GetFamily(const nsAString& aFamilyName);

    
    nsRefPtrHashtable<nsStringHashKey, gfxUserFontFamily> mFontFamilies;

    uint64_t        mGeneration;        
    uint64_t        mRebuildGeneration; 

    
    bool mLocalRulesUsed;
};



class gfxUserFontEntry : public gfxFontEntry {
    friend class gfxUserFontSet;
    friend class nsUserFontSet;
    friend class nsFontFaceLoader;
    friend class gfxOTSContext;

public:
    enum UserFontLoadState {
        STATUS_NOT_LOADED = 0,
        STATUS_LOADING,
        STATUS_LOADED,
        STATUS_FAILED
    };

    gfxUserFontEntry(gfxUserFontSet* aFontSet,
                     const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                     uint32_t aWeight,
                     int32_t aStretch,
                     uint32_t aItalicStyle,
                     const nsTArray<gfxFontFeature>& aFeatureSettings,
                     uint32_t aLanguageOverride,
                     gfxSparseBitSet* aUnicodeRanges);

    virtual ~gfxUserFontEntry();

    
    bool Matches(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                 uint32_t aWeight,
                 int32_t aStretch,
                 uint32_t aItalicStyle,
                 const nsTArray<gfxFontFeature>& aFeatureSettings,
                 uint32_t aLanguageOverride,
                 gfxSparseBitSet* aUnicodeRanges);

    virtual gfxFont* CreateFontInstance(const gfxFontStyle* aFontStyle,
                                        bool aNeedsBold);

    gfxFontEntry* GetPlatformFontEntry() const { return mPlatformFontEntry; }

    
    UserFontLoadState LoadState() const { return mUserFontLoadState; }

    
    bool WaitForUserFont() const {
        return mUserFontLoadState == STATUS_LOADING &&
               mFontDataLoadingState < LOADING_SLOWLY;
    }

    
    
    bool CharacterInUnicodeRange(uint32_t ch) const {
        if (mCharacterMap) {
            return mCharacterMap->test(ch);
        }
        return true;
    }

    gfxCharacterMap* GetUnicodeRangeMap() const {
        return mCharacterMap.get();
    }

    
    
    void Load();

    
    
    void SetLoader(nsFontFaceLoader* aLoader) { mLoader = aLoader; }
    nsFontFaceLoader* GetLoader() { return mLoader; }
    nsIPrincipal* GetPrincipal() { return mPrincipal; }
    uint32_t GetSrcIndex() { return mSrcIndex; }
    void GetFamilyNameAndURIForLogging(nsACString& aFamilyName,
                                       nsACString& aURI);

protected:
    const uint8_t* SanitizeOpenTypeData(const uint8_t* aData,
                                        uint32_t aLength,
                                        uint32_t& aSaneLength,
                                        gfxUserFontType aFontType);

    
    void LoadNextSrc();

    
    virtual void SetLoadState(UserFontLoadState aLoadState);

    
    
    
    
    
    
    bool FontDataDownloadComplete(const uint8_t* aFontData, uint32_t aLength,
                                  nsresult aDownloadStatus);

    
    
    
    
    bool LoadPlatformFont(const uint8_t* aFontData, uint32_t& aLength);

    
    void StoreUserFontData(gfxFontEntry*      aFontEntry,
                           bool               aPrivate,
                           const nsAString&   aOriginalName,
                           FallibleTArray<uint8_t>* aMetadata,
                           uint32_t           aMetaOrigLen,
                           uint8_t            aCompression);

    
    UserFontLoadState        mUserFontLoadState;

    
    
    
    enum FontDataLoadingState {
        NOT_LOADING = 0,     
        LOADING_STARTED,     
        LOADING_ALMOST_DONE, 
                             
        LOADING_SLOWLY,      
                             
        LOADING_FAILED       
    };
    FontDataLoadingState     mFontDataLoadingState;

    bool                     mUnsupportedFormat;

    nsRefPtr<gfxFontEntry>   mPlatformFontEntry;
    nsTArray<gfxFontFaceSrc> mSrcList;
    uint32_t                 mSrcIndex; 
    nsFontFaceLoader*        mLoader; 
    gfxUserFontSet*          mFontSet; 
    nsCOMPtr<nsIPrincipal>   mPrincipal;
};


#endif
