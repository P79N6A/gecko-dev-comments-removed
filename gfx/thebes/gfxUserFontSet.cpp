




#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "gfxUserFontSet.h"
#include "gfxPlatform.h"
#include "nsUnicharUtils.h"
#include "nsNetUtil.h"
#include "nsICacheService.h"
#include "nsIProtocolHandler.h"
#include "nsIPrincipal.h"
#include "gfxFontConstants.h"
#include "mozilla/Services.h"
#include "mozilla/gfx/2D.h"

#include "opentype-sanitiser.h"
#include "ots-memory-stream.h"

using namespace mozilla;

#ifdef PR_LOGGING
PRLogModuleInfo *
gfxUserFontSet::GetUserFontsLog()
{
    static PRLogModuleInfo *sLog;
    if (!sLog)
        sLog = PR_NewLogModule("userfonts");
    return sLog;
}
#endif 

#define LOG(args) PR_LOG(GetUserFontsLog(), PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(GetUserFontsLog(), PR_LOG_DEBUG)

static uint64_t sFontSetGeneration = 0;



gfxProxyFontEntry::gfxProxyFontEntry(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
             uint32_t aWeight,
             int32_t aStretch,
             uint32_t aItalicStyle,
             const nsTArray<gfxFontFeature>& aFeatureSettings,
             uint32_t aLanguageOverride,
             gfxSparseBitSet *aUnicodeRanges)
    : gfxFontEntry(NS_LITERAL_STRING("Proxy")),
      mLoadingState(NOT_LOADING),
      mUnsupportedFormat(false),
      mLoader(nullptr)
{
    mIsProxy = true;
    mSrcList = aFontFaceSrcList;
    mSrcIndex = 0;
    mWeight = aWeight;
    mStretch = aStretch;
    
    
    mItalic = (aItalicStyle & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)) != 0;
    mFeatureSettings.AppendElements(aFeatureSettings);
    mLanguageOverride = aLanguageOverride;
    mIsUserFont = true;
}

gfxProxyFontEntry::~gfxProxyFontEntry()
{
}

bool
gfxProxyFontEntry::Matches(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                           uint32_t aWeight,
                           int32_t aStretch,
                           uint32_t aItalicStyle,
                           const nsTArray<gfxFontFeature>& aFeatureSettings,
                           uint32_t aLanguageOverride,
                           gfxSparseBitSet *aUnicodeRanges)
{
    
    bool isItalic =
        (aItalicStyle & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)) != 0;

    return mWeight == aWeight &&
           mStretch == aStretch &&
           mItalic == isItalic &&
           mFeatureSettings == aFeatureSettings &&
           mLanguageOverride == aLanguageOverride &&
           mSrcList == aFontFaceSrcList;
           
           
}

gfxFont*
gfxProxyFontEntry::CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold)
{
    
    return nullptr;
}

gfxUserFontSet::gfxUserFontSet()
    : mFontFamilies(5)
{
    IncrementGeneration();
}

gfxUserFontSet::~gfxUserFontSet()
{
}

gfxFontEntry*
gfxUserFontSet::AddFontFace(const nsAString& aFamilyName,
                            const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                            uint32_t aWeight,
                            int32_t aStretch,
                            uint32_t aItalicStyle,
                            const nsTArray<gfxFontFeature>& aFeatureSettings,
                            const nsString& aLanguageOverride,
                            gfxSparseBitSet *aUnicodeRanges)
{
    nsAutoString key(aFamilyName);
    ToLowerCase(key);

    bool found;

    if (aWeight == 0)
        aWeight = NS_FONT_WEIGHT_NORMAL;

    

    gfxMixedFontFamily *family = mFontFamilies.GetWeak(key, &found);
    if (!family) {
        family = new gfxMixedFontFamily(aFamilyName);
        mFontFamilies.Put(key, family);
    }

    uint32_t languageOverride =
        gfxFontStyle::ParseFontLanguageOverride(aLanguageOverride);

    
    
    
    
    
    
    nsTArray<nsRefPtr<gfxFontEntry> >& fontList = family->GetFontList();
    for (uint32_t i = 0, count = fontList.Length(); i < count; i++) {
        if (!fontList[i]->mIsProxy) {
            continue;
        }

        gfxProxyFontEntry *existingProxyEntry =
            static_cast<gfxProxyFontEntry*>(fontList[i].get());
        if (!existingProxyEntry->Matches(aFontFaceSrcList,
                                         aWeight, aStretch, aItalicStyle,
                                         aFeatureSettings, languageOverride,
                                         aUnicodeRanges)) {
            continue;
        }

        
        
        
        family->AddFontEntry(existingProxyEntry);
        return existingProxyEntry;
    }

    
    gfxProxyFontEntry *proxyEntry =
        new gfxProxyFontEntry(aFontFaceSrcList, aWeight, aStretch,
                              aItalicStyle,
                              aFeatureSettings,
                              languageOverride,
                              aUnicodeRanges);
    family->AddFontEntry(proxyEntry);
#ifdef PR_LOGGING
    if (LOG_ENABLED()) {
        LOG(("userfonts (%p) added (%s) with style: %s weight: %d stretch: %d",
             this, NS_ConvertUTF16toUTF8(aFamilyName).get(),
             (aItalicStyle & NS_FONT_STYLE_ITALIC ? "italic" :
                 (aItalicStyle & NS_FONT_STYLE_OBLIQUE ? "oblique" : "normal")),
             aWeight, aStretch));
    }
#endif

    return proxyEntry;
}

void
gfxUserFontSet::AddFontFace(const nsAString& aFamilyName,
                            gfxFontEntry     *aFontEntry)
{
    nsAutoString key(aFamilyName);
    ToLowerCase(key);

    bool found;

    gfxMixedFontFamily *family = mFontFamilies.GetWeak(key, &found);
    if (!family) {
        family = new gfxMixedFontFamily(aFamilyName);
        mFontFamilies.Put(key, family);
    }

    family->AddFontEntry(aFontEntry);
}

gfxFontEntry*
gfxUserFontSet::FindFontEntry(gfxFontFamily *aFamily,
                              const gfxFontStyle& aFontStyle,
                              bool& aNeedsBold,
                              bool& aWaitForUserFont)
{
    aWaitForUserFont = false;
    gfxMixedFontFamily *family = static_cast<gfxMixedFontFamily*>(aFamily);

    gfxFontEntry* fe = family->FindFontForStyle(aFontStyle, aNeedsBold);

    
    if (!fe->mIsProxy) {
        return fe;
    }

    gfxProxyFontEntry *proxyEntry = static_cast<gfxProxyFontEntry*> (fe);

    
    if (proxyEntry->mLoadingState > gfxProxyFontEntry::NOT_LOADING) {
        aWaitForUserFont =
            (proxyEntry->mLoadingState < gfxProxyFontEntry::LOADING_SLOWLY);
        return nullptr;
    }

    
    LoadStatus status;

    
    
    status = LoadNext(family, proxyEntry);

    
    
    if (status == STATUS_LOADED) {
        return family->FindFontForStyle(aFontStyle, aNeedsBold);
    }

    
    
    aWaitForUserFont = (status != STATUS_END_OF_LIST) &&
        (proxyEntry->mLoadingState < gfxProxyFontEntry::LOADING_SLOWLY);

    
    return nullptr;
}




class ExpandingMemoryStream : public ots::OTSStream {
public:
    ExpandingMemoryStream(size_t initial, size_t limit)
        : mLength(initial), mLimit(limit), mOff(0) {
        mPtr = NS_Alloc(mLength);
    }

    ~ExpandingMemoryStream() {
        NS_Free(mPtr);
    }

    
    
    
    void* forget() {
        void* p = mPtr;
        mPtr = nullptr;
        return p;
    }

    bool WriteRaw(const void *data, size_t length) {
        if ((mOff + length > mLength) ||
            (mLength > std::numeric_limits<size_t>::max() - mOff)) {
            if (mLength == mLimit) {
                return false;
            }
            size_t newLength = (mLength + 1) * 2;
            if (newLength < mLength) {
                return false;
            }
            if (newLength > mLimit) {
                newLength = mLimit;
            }
            mPtr = NS_Realloc(mPtr, newLength);
            mLength = newLength;
            return WriteRaw(data, length);
        }
        std::memcpy(static_cast<char*>(mPtr) + mOff, data, length);
        mOff += length;
        return true;
    }

    bool Seek(off_t position) {
        if (position < 0) {
            return false;
        }
        if (static_cast<size_t>(position) > mLength) {
            return false;
        }
        mOff = position;
        return true;
    }

    off_t Tell() const {
        return mOff;
    }

private:
    void*        mPtr;
    size_t       mLength;
    const size_t mLimit;
    off_t        mOff;
};

static ots::TableAction
OTSTableAction(uint32_t aTag, void *aUserData)
{
    
    if (aTag == TRUETYPE_TAG('S', 'i', 'l', 'f') ||
        aTag == TRUETYPE_TAG('S', 'i', 'l', 'l') ||
        aTag == TRUETYPE_TAG('G', 'l', 'o', 'c') ||
        aTag == TRUETYPE_TAG('G', 'l', 'a', 't') ||
        aTag == TRUETYPE_TAG('F', 'e', 'a', 't') ||
        aTag == TRUETYPE_TAG('S', 'V', 'G', ' ')) {
        return ots::TABLE_ACTION_PASSTHRU;
    }
    return ots::TABLE_ACTION_DEFAULT;
}

struct OTSCallbackUserData {
    gfxUserFontSet     *mFontSet;
    gfxMixedFontFamily *mFamily;
    gfxProxyFontEntry  *mProxy;
};

 bool
gfxUserFontSet::OTSMessage(void *aUserData, const char *format, ...)
{
    va_list va;
    va_start(va, format);

    
    
    char buf[512];
    (void)vsnprintf(buf, sizeof(buf), format, va);

    va_end(va);

    OTSCallbackUserData *d = static_cast<OTSCallbackUserData*>(aUserData);
    d->mFontSet->LogMessage(d->mFamily, d->mProxy, buf);

    return false;
}



const uint8_t*
gfxUserFontSet::SanitizeOpenTypeData(gfxMixedFontFamily *aFamily,
                                     gfxProxyFontEntry *aProxy,
                                     const uint8_t* aData, uint32_t aLength,
                                     uint32_t& aSaneLength, bool aIsCompressed)
{
    
    ExpandingMemoryStream output(aIsCompressed ? aLength * 2 : aLength,
                                 1024 * 1024 * 256);

    OTSCallbackUserData userData;
    userData.mFontSet = this;
    userData.mFamily = aFamily;
    userData.mProxy = aProxy;

    ots::SetTableActionCallback(&OTSTableAction, nullptr);
    ots::SetMessageCallback(&gfxUserFontSet::OTSMessage, &userData);

    if (ots::Process(&output, aData, aLength)) {
        aSaneLength = output.Tell();
        return static_cast<uint8_t*>(output.forget());
    } else {
        aSaneLength = 0;
        return nullptr;
    }
}

static void
StoreUserFontData(gfxFontEntry* aFontEntry, gfxProxyFontEntry* aProxy,
                  bool aPrivate, const nsAString& aOriginalName,
                  FallibleTArray<uint8_t>* aMetadata, uint32_t aMetaOrigLen)
{
    if (!aFontEntry->mUserFontData) {
        aFontEntry->mUserFontData = new gfxUserFontData;
    }
    gfxUserFontData* userFontData = aFontEntry->mUserFontData;
    userFontData->mSrcIndex = aProxy->mSrcIndex;
    const gfxFontFaceSrc& src = aProxy->mSrcList[aProxy->mSrcIndex];
    if (src.mIsLocal) {
        userFontData->mLocalName = src.mLocalName;
    } else {
        userFontData->mURI = src.mURI;
        userFontData->mPrincipal = aProxy->mPrincipal;
    }
    userFontData->mPrivate = aPrivate;
    userFontData->mFormat = src.mFormatFlags;
    userFontData->mRealName = aOriginalName;
    if (aMetadata) {
        userFontData->mMetadata.SwapElements(*aMetadata);
        userFontData->mMetaOrigLen = aMetaOrigLen;
    }
}

struct WOFFHeader {
    AutoSwap_PRUint32 signature;
    AutoSwap_PRUint32 flavor;
    AutoSwap_PRUint32 length;
    AutoSwap_PRUint16 numTables;
    AutoSwap_PRUint16 reserved;
    AutoSwap_PRUint32 totalSfntSize;
    AutoSwap_PRUint16 majorVersion;
    AutoSwap_PRUint16 minorVersion;
    AutoSwap_PRUint32 metaOffset;
    AutoSwap_PRUint32 metaCompLen;
    AutoSwap_PRUint32 metaOrigLen;
    AutoSwap_PRUint32 privOffset;
    AutoSwap_PRUint32 privLen;
};

void
gfxUserFontSet::CopyWOFFMetadata(const uint8_t* aFontData,
                                 uint32_t aLength,
                                 FallibleTArray<uint8_t>* aMetadata,
                                 uint32_t* aMetaOrigLen)
{
    
    
    
    
    
    
    if (aLength < sizeof(WOFFHeader)) {
        return;
    }
    const WOFFHeader* woff = reinterpret_cast<const WOFFHeader*>(aFontData);
    uint32_t metaOffset = woff->metaOffset;
    uint32_t metaCompLen = woff->metaCompLen;
    if (!metaOffset || !metaCompLen || !woff->metaOrigLen) {
        return;
    }
    if (metaOffset >= aLength || metaCompLen > aLength - metaOffset) {
        return;
    }
    if (!aMetadata->SetLength(woff->metaCompLen)) {
        return;
    }
    memcpy(aMetadata->Elements(), aFontData + metaOffset, metaCompLen);
    *aMetaOrigLen = woff->metaOrigLen;
}




bool
gfxUserFontSet::OnLoadComplete(gfxMixedFontFamily *aFamily,
                               gfxProxyFontEntry *aProxy,
                               const uint8_t *aFontData, uint32_t aLength,
                               nsresult aDownloadStatus)
{
    
    
    aProxy->mLoader = nullptr;

    
    if (NS_SUCCEEDED(aDownloadStatus)) {
        gfxFontEntry *fe = LoadFont(aFamily, aProxy, aFontData, aLength);
        aFontData = nullptr;

        if (fe) {
            IncrementGeneration();
            return true;
        }

    } else {
        
        LogMessage(aFamily, aProxy,
                   "download failed", nsIScriptError::errorFlag,
                   aDownloadStatus);
    }

    if (aFontData) {
        NS_Free((void*)aFontData);
    }

    
    (void)LoadNext(aFamily, aProxy);

    
    
    
    
    IncrementGeneration();
    return true;
}


gfxUserFontSet::LoadStatus
gfxUserFontSet::LoadNext(gfxMixedFontFamily *aFamily,
                         gfxProxyFontEntry *aProxyEntry)
{
    uint32_t numSrc = aProxyEntry->mSrcList.Length();

    NS_ASSERTION(aProxyEntry->mSrcIndex < numSrc,
                 "already at the end of the src list for user font");

    if (aProxyEntry->mLoadingState == gfxProxyFontEntry::NOT_LOADING) {
        aProxyEntry->mLoadingState = gfxProxyFontEntry::LOADING_STARTED;
        aProxyEntry->mUnsupportedFormat = false;
    } else {
        
        
        
        aProxyEntry->mSrcIndex++;
    }

    
    
    while (aProxyEntry->mSrcIndex < numSrc) {
        const gfxFontFaceSrc& currSrc = aProxyEntry->mSrcList[aProxyEntry->mSrcIndex];

        

        if (currSrc.mIsLocal) {
            gfxFontEntry *fe =
                gfxPlatform::GetPlatform()->LookupLocalFont(aProxyEntry,
                                                            currSrc.mLocalName);
            if (fe) {
                LOG(("userfonts (%p) [src %d] loaded local: (%s) for (%s) gen: %8.8x\n",
                     this, aProxyEntry->mSrcIndex,
                     NS_ConvertUTF16toUTF8(currSrc.mLocalName).get(),
                     NS_ConvertUTF16toUTF8(aFamily->Name()).get(),
                     uint32_t(mGeneration)));
                fe->mFeatureSettings.AppendElements(aProxyEntry->mFeatureSettings);
                fe->mLanguageOverride = aProxyEntry->mLanguageOverride;
                
                
                
                StoreUserFontData(fe, aProxyEntry, false, nsString(), nullptr, 0);
                ReplaceFontEntry(aFamily, aProxyEntry, fe);
                return STATUS_LOADED;
            } else {
                LOG(("userfonts (%p) [src %d] failed local: (%s) for (%s)\n",
                     this, aProxyEntry->mSrcIndex,
                     NS_ConvertUTF16toUTF8(currSrc.mLocalName).get(),
                     NS_ConvertUTF16toUTF8(aFamily->Name()).get()));
            }
        }

        
        else {
            if (gfxPlatform::GetPlatform()->IsFontFormatSupported(currSrc.mURI,
                    currSrc.mFormatFlags)) {

                nsIPrincipal *principal = nullptr;
                bool bypassCache;
                nsresult rv = CheckFontLoad(&currSrc, &principal, &bypassCache);

                if (NS_SUCCEEDED(rv) && principal != nullptr) {
                    if (!bypassCache) {
                        
                        gfxFontEntry *fe =
                            UserFontCache::GetFont(currSrc.mURI, principal,
                                                   aProxyEntry,
                                                   GetPrivateBrowsing());
                        if (fe) {
                            ReplaceFontEntry(aFamily, aProxyEntry, fe);
                            return STATUS_LOADED;
                        }
                    }

                    
                    
                    
                    aProxyEntry->mPrincipal = principal;

                    bool loadDoesntSpin = false;
                    rv = NS_URIChainHasFlags(currSrc.mURI,
                           nsIProtocolHandler::URI_SYNC_LOAD_IS_OK,
                           &loadDoesntSpin);
                    if (NS_SUCCEEDED(rv) && loadDoesntSpin) {
                        uint8_t *buffer = nullptr;
                        uint32_t bufferLength = 0;

                        
                        rv = SyncLoadFontData(aProxyEntry, &currSrc,
                                              buffer, bufferLength);
                        if (NS_SUCCEEDED(rv) && LoadFont(aFamily, aProxyEntry,
                                                         buffer, bufferLength)) {
                            return STATUS_LOADED;
                        } else {
                            LogMessage(aFamily, aProxyEntry,
                                       "font load failed",
                                       nsIScriptError::errorFlag, rv);
                        }
                    } else {
                        
                        rv = StartLoad(aFamily, aProxyEntry, &currSrc);
                        if (NS_SUCCEEDED(rv)) {
#ifdef PR_LOGGING
                            if (LOG_ENABLED()) {
                                nsAutoCString fontURI;
                                currSrc.mURI->GetSpec(fontURI);
                                LOG(("userfonts (%p) [src %d] loading uri: (%s) for (%s)\n",
                                     this, aProxyEntry->mSrcIndex, fontURI.get(),
                                     NS_ConvertUTF16toUTF8(aFamily->Name()).get()));
                            }
#endif
                            return STATUS_LOADING;
                        } else {
                            LogMessage(aFamily, aProxyEntry,
                                       "download failed",
                                       nsIScriptError::errorFlag, rv);
                        }
                    }
                } else {
                    LogMessage(aFamily, aProxyEntry, "download not allowed",
                               nsIScriptError::errorFlag, rv);
                }
            } else {
                
                
                aProxyEntry->mUnsupportedFormat = true;
            }
        }

        aProxyEntry->mSrcIndex++;
    }

    if (aProxyEntry->mUnsupportedFormat) {
        LogMessage(aFamily, aProxyEntry, "no supported format found",
                   nsIScriptError::warningFlag);
    }

    
    LOG(("userfonts (%p) failed all src for (%s)\n",
        this, NS_ConvertUTF16toUTF8(aFamily->Name()).get()));
    aProxyEntry->mLoadingState = gfxProxyFontEntry::LOADING_FAILED;

    return STATUS_END_OF_LIST;
}

void
gfxUserFontSet::IncrementGeneration()
{
    
    ++sFontSetGeneration;
    if (sFontSetGeneration == 0)
       ++sFontSetGeneration;
    mGeneration = sFontSetGeneration;
}


gfxFontEntry*
gfxUserFontSet::LoadFont(gfxMixedFontFamily *aFamily,
                         gfxProxyFontEntry *aProxy,
                         const uint8_t *aFontData, uint32_t &aLength)
{
    gfxFontEntry *fe = nullptr;

    gfxUserFontType fontType =
        gfxFontUtils::DetermineFontDataType(aFontData, aLength);

    
    

    
    
    
    nsAutoString originalFullName;

    
    
    uint32_t saneLen;
    const uint8_t* saneData =
        SanitizeOpenTypeData(aFamily, aProxy, aFontData, aLength, saneLen,
                             fontType == GFX_USERFONT_WOFF);
    if (!saneData) {
        LogMessage(aFamily, aProxy, "rejected by sanitizer");
    }
    if (saneData) {
        
        
        
        
        gfxFontUtils::GetFullNameFromSFNT(saneData, saneLen,
                                          originalFullName);
        
        
        fe = gfxPlatform::GetPlatform()->MakePlatformFont(aProxy,
                                                          saneData,
                                                          saneLen);
        if (!fe) {
            LogMessage(aFamily, aProxy, "not usable by platform");
        }
    }

    if (fe) {
        
        
        
        FallibleTArray<uint8_t> metadata;
        uint32_t metaOrigLen = 0;
        if (fontType == GFX_USERFONT_WOFF) {
            CopyWOFFMetadata(aFontData, aLength, &metadata, &metaOrigLen);
        }

        
        
        fe->mFeatureSettings.AppendElements(aProxy->mFeatureSettings);
        fe->mLanguageOverride = aProxy->mLanguageOverride;
        StoreUserFontData(fe, aProxy, GetPrivateBrowsing(),
                          originalFullName, &metadata, metaOrigLen);
#ifdef PR_LOGGING
        if (LOG_ENABLED()) {
            nsAutoCString fontURI;
            aProxy->mSrcList[aProxy->mSrcIndex].mURI->GetSpec(fontURI);
            LOG(("userfonts (%p) [src %d] loaded uri: (%s) for (%s) gen: %8.8x\n",
                 this, aProxy->mSrcIndex, fontURI.get(),
                 NS_ConvertUTF16toUTF8(aFamily->Name()).get(),
                 uint32_t(mGeneration)));
        }
#endif
        ReplaceFontEntry(aFamily, aProxy, fe);
        UserFontCache::CacheFont(fe);
    } else {
#ifdef PR_LOGGING
        if (LOG_ENABLED()) {
            nsAutoCString fontURI;
            aProxy->mSrcList[aProxy->mSrcIndex].mURI->GetSpec(fontURI);
            LOG(("userfonts (%p) [src %d] failed uri: (%s) for (%s)"
                 " error making platform font\n",
                 this, aProxy->mSrcIndex, fontURI.get(),
                 NS_ConvertUTF16toUTF8(aFamily->Name()).get()));
        }
#endif
    }

    
    
    NS_Free((void*)aFontData);

    return fe;
}

gfxFontFamily*
gfxUserFontSet::GetFamily(const nsAString& aFamilyName) const
{
    nsAutoString key(aFamilyName);
    ToLowerCase(key);

    return mFontFamilies.GetWeak(key);
}

struct FindFamilyCallbackData {
    gfxFontEntry  *mFontEntry;
    gfxFontFamily *mFamily;
};

static PLDHashOperator
FindFamilyCallback(const nsAString&    aName,
                   gfxMixedFontFamily* aFamily,
                   void*               aUserArg)
{
    FindFamilyCallbackData *d = static_cast<FindFamilyCallbackData*>(aUserArg);
    if (aFamily->ContainsFace(d->mFontEntry)) {
        d->mFamily = aFamily;
        return PL_DHASH_STOP;
    }

    return PL_DHASH_NEXT;
}

gfxFontFamily*
gfxUserFontSet::FindFamilyFor(gfxFontEntry* aFontEntry) const
{
    FindFamilyCallbackData d = { aFontEntry, nullptr };
    mFontFamilies.EnumerateRead(FindFamilyCallback, &d);
    return d.mFamily;
}















nsTHashtable<gfxUserFontSet::UserFontCache::Entry>*
    gfxUserFontSet::UserFontCache::sUserFonts = nullptr;

NS_IMPL_ISUPPORTS1(gfxUserFontSet::UserFontCache::Flusher, nsIObserver)

PLDHashOperator
gfxUserFontSet::UserFontCache::Entry::RemoveIfPrivate(Entry* aEntry,
                                                      void* aUserData)
{
    return aEntry->mPrivate ? PL_DHASH_REMOVE : PL_DHASH_NEXT;
}

PLDHashOperator
gfxUserFontSet::UserFontCache::Entry::RemoveIfMatches(Entry* aEntry,
                                                      void* aUserData)
{
    return aEntry->GetFontEntry() == static_cast<gfxFontEntry*>(aUserData) ?
        PL_DHASH_REMOVE : PL_DHASH_NEXT;
}

PLDHashOperator
gfxUserFontSet::UserFontCache::Entry::DisconnectSVG(Entry* aEntry,
                                                    void* aUserData)
{
    aEntry->GetFontEntry()->DisconnectSVG();
    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
gfxUserFontSet::UserFontCache::Flusher::Observe(nsISupports* aSubject,
                                                const char* aTopic,
                                                const char16_t* aData)
{
    if (!sUserFonts) {
        return NS_OK;
    }

    if (!strcmp(aTopic, NS_CACHESERVICE_EMPTYCACHE_TOPIC_ID)) {
        sUserFonts->Clear();
    } else if (!strcmp(aTopic, "last-pb-context-exited")) {
        sUserFonts->EnumerateEntries(Entry::RemoveIfPrivate, nullptr);
    } else if (!strcmp(aTopic, "xpcom-shutdown")) {
        sUserFonts->EnumerateEntries(Entry::DisconnectSVG, nullptr);
    } else {
        NS_NOTREACHED("unexpected topic");
    }

    return NS_OK;
}

bool
gfxUserFontSet::UserFontCache::Entry::KeyEquals(const KeyTypePointer aKey) const
{
    bool equal;
    if (NS_FAILED(mURI->Equals(aKey->mURI, &equal)) || !equal) {
        return false;
    }

    if (NS_FAILED(mPrincipal->Equals(aKey->mPrincipal, &equal)) || !equal) {
        return false;
    }

    if (mPrivate != aKey->mPrivate) {
        return false;
    }

    const gfxFontEntry *fe = aKey->mFontEntry;
    if (mFontEntry->mItalic           != fe->mItalic          ||
        mFontEntry->mWeight           != fe->mWeight          ||
        mFontEntry->mStretch          != fe->mStretch         ||
        mFontEntry->mFeatureSettings  != fe->mFeatureSettings ||
        mFontEntry->mLanguageOverride != fe->mLanguageOverride ||
        mFontEntry->mFamilyName       != fe->mFamilyName) {
        return false;
    }

    return true;
}

void
gfxUserFontSet::UserFontCache::CacheFont(gfxFontEntry *aFontEntry)
{
    NS_ASSERTION(aFontEntry->mFamilyName.Length() != 0,
                 "caching a font associated with no family yet");
    if (!sUserFonts) {
        sUserFonts = new nsTHashtable<Entry>;

        nsCOMPtr<nsIObserverService> obs =
            mozilla::services::GetObserverService();
        if (obs) {
            Flusher *flusher = new Flusher;
            obs->AddObserver(flusher, NS_CACHESERVICE_EMPTYCACHE_TOPIC_ID,
                             false);
            obs->AddObserver(flusher, "last-pb-context-exited", false);
            obs->AddObserver(flusher, "xpcom-shutdown", false);
        }
    }

    gfxUserFontData *data = aFontEntry->mUserFontData;
    sUserFonts->PutEntry(Key(data->mURI, data->mPrincipal, aFontEntry,
                             data->mPrivate));

#ifdef DEBUG_USERFONT_CACHE
    printf("userfontcache added fontentry: %p\n", aFontEntry);
    Dump();
#endif
}

void
gfxUserFontSet::UserFontCache::ForgetFont(gfxFontEntry *aFontEntry)
{
    if (!sUserFonts) {
        
        
        return;
    }

    
    
    
    sUserFonts->EnumerateEntries(
        gfxUserFontSet::UserFontCache::Entry::RemoveIfMatches, aFontEntry);

#ifdef DEBUG_USERFONT_CACHE
    printf("userfontcache removed fontentry: %p\n", aFontEntry);
    Dump();
#endif
}

gfxFontEntry*
gfxUserFontSet::UserFontCache::GetFont(nsIURI            *aSrcURI,
                                       nsIPrincipal      *aPrincipal,
                                       gfxProxyFontEntry *aProxy,
                                       bool               aPrivate)
{
    if (!sUserFonts) {
        return nullptr;
    }

    Entry* entry = sUserFonts->GetEntry(Key(aSrcURI, aPrincipal, aProxy,
                                            aPrivate));
    if (entry) {
        return entry->GetFontEntry();
    }

    return nullptr;
}

void
gfxUserFontSet::UserFontCache::Shutdown()
{
    if (sUserFonts) {
        delete sUserFonts;
        sUserFonts = nullptr;
    }
}

#ifdef DEBUG_USERFONT_CACHE

PLDHashOperator
gfxUserFontSet::UserFontCache::Entry::DumpEntry(Entry* aEntry, void* aUserData)
{
    nsresult rv;

    nsAutoCString principalURISpec;

    nsCOMPtr<nsIURI> principalURI;
    rv = aEntry->mPrincipal->GetURI(getter_AddRefs(principalURI));
    if (NS_SUCCEEDED(rv)) {
        principalURI->GetSpec(principalURISpec);
    }

    bool setDomain = false;
    nsCOMPtr<nsIURI> domainURI;

    aEntry->mPrincipal->GetDomain(getter_AddRefs(domainURI));
    if (domainURI) {
        setDomain = true;
    }

    NS_ASSERTION(aEntry->mURI, "null URI in userfont cache entry");

    printf("userfontcache fontEntry: %p fonturihash: %8.8x family: %s domainset: %s principal: [%s]\n",
            aEntry->mFontEntry,
            nsURIHashKey::HashKey(aEntry->mURI),
            NS_ConvertUTF16toUTF8(aEntry->mFontEntry->FamilyName()).get(),
            (setDomain ? "true" : "false"),
            principalURISpec.get()
           );
    return PL_DHASH_NEXT;
}

void
gfxUserFontSet::UserFontCache::Dump()
{
    if (!sUserFonts) {
        return;
    }

    printf("userfontcache dump count: %d ========\n", sUserFonts->Count());
    sUserFonts->EnumerateEntries(Entry::DumpEntry, nullptr);
    printf("userfontcache dump ==================\n");
}

#endif
