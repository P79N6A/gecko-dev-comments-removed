





































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "gfxUserFontSet.h"
#include "gfxPlatform.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "prlong.h"
#include "nsNetUtil.h"
#include "nsIProtocolHandler.h"

#include "woff.h"

#include "opentype-sanitiser.h"
#include "ots-memory-stream.h"

using namespace mozilla;

#ifdef PR_LOGGING
PRLogModuleInfo *gfxUserFontSet::sUserFontsLog = PR_NewLogModule("userfonts");
#endif 

#define LOG(args) PR_LOG(sUserFontsLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(sUserFontsLog, PR_LOG_DEBUG)

static PRUint64 sFontSetGeneration = LL_INIT(0, 0);



gfxProxyFontEntry::gfxProxyFontEntry(const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
             gfxMixedFontFamily *aFamily,
             PRUint32 aWeight,
             PRUint32 aStretch,
             PRUint32 aItalicStyle,
             const nsTArray<gfxFontFeature>& aFeatureSettings,
             PRUint32 aLanguageOverride,
             gfxSparseBitSet *aUnicodeRanges)
    : gfxFontEntry(NS_LITERAL_STRING("Proxy"), aFamily),
      mLoadingState(NOT_LOADING),
      mUnsupportedFormat(false),
      mLoader(nsnull)
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

gfxFont*
gfxProxyFontEntry::CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold)
{
    
    return nsnull;
}

gfxUserFontSet::gfxUserFontSet()
{
    mFontFamilies.Init(5);
    IncrementGeneration();
}

gfxUserFontSet::~gfxUserFontSet()
{
}

gfxFontEntry*
gfxUserFontSet::AddFontFace(const nsAString& aFamilyName,
                            const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                            PRUint32 aWeight,
                            PRUint32 aStretch,
                            PRUint32 aItalicStyle,
                            const nsTArray<gfxFontFeature>& aFeatureSettings,
                            const nsString& aLanguageOverride,
                            gfxSparseBitSet *aUnicodeRanges)
{
    gfxProxyFontEntry *proxyEntry = nsnull;

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

    
    PRUint32 languageOverride =
        gfxFontStyle::ParseFontLanguageOverride(aLanguageOverride);
    proxyEntry =
        new gfxProxyFontEntry(aFontFaceSrcList, family, aWeight, aStretch,
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
gfxUserFontSet::FindFontEntry(const nsAString& aName, 
                              const gfxFontStyle& aFontStyle, 
                              bool& aFoundFamily,
                              bool& aNeedsBold,
                              bool& aWaitForUserFont)
{
    aWaitForUserFont = false;
    gfxMixedFontFamily *family = GetFamily(aName);

    
    if (!family) {
        aFoundFamily = false;
        return nsnull;
    }

    aFoundFamily = true;
    gfxFontEntry* fe = family->FindFontForStyle(aFontStyle, aNeedsBold);

    
    if (!fe->mIsProxy) {
        return fe;
    }

    gfxProxyFontEntry *proxyEntry = static_cast<gfxProxyFontEntry*> (fe);

    
    if (proxyEntry->mLoadingState > gfxProxyFontEntry::NOT_LOADING) {
        aWaitForUserFont =
            (proxyEntry->mLoadingState < gfxProxyFontEntry::LOADING_SLOWLY);
        return nsnull;
    }

    
    LoadStatus status;

    
    
    status = LoadNext(proxyEntry);

    
    
    if (status == STATUS_LOADED) {
        return family->FindFontForStyle(aFontStyle, aNeedsBold);
    }

    
    
    aWaitForUserFont = (status != STATUS_END_OF_LIST) &&
        (proxyEntry->mLoadingState < gfxProxyFontEntry::LOADING_SLOWLY);

    
    return nsnull;
}







static const PRUint8*
PrepareOpenTypeData(const PRUint8* aData, PRUint32* aLength)
{
    switch(gfxFontUtils::DetermineFontDataType(aData, *aLength)) {
    
    case GFX_USERFONT_OPENTYPE:
        
        return aData;
        
    case GFX_USERFONT_WOFF: {
        PRUint32 status = eWOFF_ok;
        PRUint32 bufferSize = woffGetDecodedSize(aData, *aLength, &status);
        if (WOFF_FAILURE(status)) {
            break;
        }
        PRUint8* decodedData = static_cast<PRUint8*>(NS_Alloc(bufferSize));
        if (!decodedData) {
            break;
        }
        woffDecodeToBuffer(aData, *aLength,
                           decodedData, bufferSize,
                           aLength, &status);
        
        NS_Free((void*)aData);
        aData = decodedData;
        if (WOFF_FAILURE(status)) {
            
            break;
        }
        
        return aData;
    }

    

    default:
        NS_WARNING("unknown font format");
        break;
    }

    
    NS_Free((void*)aData);

    return nsnull;
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
        mPtr = nsnull;
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



static const PRUint8*
SanitizeOpenTypeData(const PRUint8* aData, PRUint32 aLength,
                     PRUint32& aSaneLength, bool aIsCompressed)
{
    
    ExpandingMemoryStream output(aIsCompressed ? aLength * 2 : aLength,
                                 1024 * 1024 * 256);
#ifdef MOZ_GRAPHITE
#define PRESERVE_GRAPHITE true
#else
#define PRESERVE_GRAPHITE false
#endif
    if (ots::Process(&output, aData, aLength, PRESERVE_GRAPHITE)) {
        aSaneLength = output.Tell();
        return static_cast<PRUint8*>(output.forget());
    } else {
        aSaneLength = 0;
        return nsnull;
    }
}

static void
StoreUserFontData(gfxFontEntry* aFontEntry, gfxProxyFontEntry* aProxy,
                  const nsAString& aOriginalName,
                  nsTArray<PRUint8>* aMetadata, PRUint32 aMetaOrigLen)
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
    }
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
gfxUserFontSet::CopyWOFFMetadata(const PRUint8* aFontData,
                                 PRUint32 aLength,
                                 nsTArray<PRUint8>* aMetadata,
                                 PRUint32* aMetaOrigLen)
{
    
    
    
    
    
    
    if (aLength < sizeof(WOFFHeader)) {
        return;
    }
    const WOFFHeader* woff = reinterpret_cast<const WOFFHeader*>(aFontData);
    PRUint32 metaOffset = woff->metaOffset;
    PRUint32 metaCompLen = woff->metaCompLen;
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
gfxUserFontSet::OnLoadComplete(gfxProxyFontEntry *aProxy,
                               const PRUint8 *aFontData, PRUint32 aLength,
                               nsresult aDownloadStatus)
{
    
    
    aProxy->mLoader = nsnull;

    
    if (NS_SUCCEEDED(aDownloadStatus)) {
        gfxFontEntry *fe = LoadFont(aProxy, aFontData, aLength);
        aFontData = nsnull;

        if (fe) {
            IncrementGeneration();
            return true;
        }

    } else {
        
        LogMessage(aProxy, "download failed", nsIScriptError::errorFlag,
                   aDownloadStatus);
    }

    if (aFontData) {
        NS_Free((void*)aFontData);
    }

    
    (void)LoadNext(aProxy);

    
    
    
    
    IncrementGeneration();
    return true;
}


gfxUserFontSet::LoadStatus
gfxUserFontSet::LoadNext(gfxProxyFontEntry *aProxyEntry)
{
    PRUint32 numSrc = aProxyEntry->mSrcList.Length();

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
                     NS_ConvertUTF16toUTF8(aProxyEntry->mFamily->Name()).get(),
                     PRUint32(mGeneration)));
                fe->mFeatureSettings.AppendElements(aProxyEntry->mFeatureSettings);
                fe->mLanguageOverride = aProxyEntry->mLanguageOverride;
                StoreUserFontData(fe, aProxyEntry, nsString(), nsnull, 0);
                ReplaceFontEntry(aProxyEntry, fe);
                return STATUS_LOADED;
            } else {
                LOG(("userfonts (%p) [src %d] failed local: (%s) for (%s)\n",
                     this, aProxyEntry->mSrcIndex,
                     NS_ConvertUTF16toUTF8(currSrc.mLocalName).get(),
                     NS_ConvertUTF16toUTF8(aProxyEntry->mFamily->Name()).get()));
            }
        }

        
        else {
            if (gfxPlatform::GetPlatform()->IsFontFormatSupported(currSrc.mURI,
                    currSrc.mFormatFlags)) {

                nsresult rv;
                bool loadDoesntSpin = false;
                rv = NS_URIChainHasFlags(currSrc.mURI,
                       nsIProtocolHandler::URI_SYNC_LOAD_IS_OK,
                       &loadDoesntSpin);

                if (NS_SUCCEEDED(rv) && loadDoesntSpin) {
                    PRUint8 *buffer = nsnull;
                    PRUint32 bufferLength = 0;

                    
                    rv = SyncLoadFontData(aProxyEntry, &currSrc, buffer,
                                          bufferLength);

                    if (NS_SUCCEEDED(rv) &&
                        LoadFont(aProxyEntry, buffer, bufferLength)) {
                        return STATUS_LOADED;
                    } else {
                        LogMessage(aProxyEntry, "font load failed",
                                   nsIScriptError::errorFlag, rv);
                    }

                } else {
                    
                    rv = StartLoad(aProxyEntry, &currSrc);
                    bool loadOK = NS_SUCCEEDED(rv);

                    if (loadOK) {
#ifdef PR_LOGGING
                        if (LOG_ENABLED()) {
                            nsCAutoString fontURI;
                            currSrc.mURI->GetSpec(fontURI);
                            LOG(("userfonts (%p) [src %d] loading uri: (%s) for (%s)\n",
                                 this, aProxyEntry->mSrcIndex, fontURI.get(),
                                 NS_ConvertUTF16toUTF8(aProxyEntry->mFamily->Name()).get()));
                        }
#endif
                        return STATUS_LOADING;
                    } else {
                        LogMessage(aProxyEntry, "download failed",
                                   nsIScriptError::errorFlag, rv);
                    }
                }
            } else {
                
                
                aProxyEntry->mUnsupportedFormat = true;
            }
        }

        aProxyEntry->mSrcIndex++;
    }

    if (aProxyEntry->mUnsupportedFormat) {
        LogMessage(aProxyEntry, "no supported format found",
                   nsIScriptError::warningFlag);
    }

    
    LOG(("userfonts (%p) failed all src for (%s)\n",
        this, NS_ConvertUTF16toUTF8(aProxyEntry->mFamily->Name()).get()));
    aProxyEntry->mLoadingState = gfxProxyFontEntry::LOADING_FAILED;

    return STATUS_END_OF_LIST;
}

void
gfxUserFontSet::IncrementGeneration()
{
    
    LL_ADD(sFontSetGeneration, sFontSetGeneration, 1);
    if (LL_IS_ZERO(sFontSetGeneration))
        LL_ADD(sFontSetGeneration, sFontSetGeneration, 1);
    mGeneration = sFontSetGeneration;
}


gfxFontEntry*
gfxUserFontSet::LoadFont(gfxProxyFontEntry *aProxy,
                         const PRUint8 *aFontData, PRUint32 &aLength)
{
    
    
    
    if (!aProxy->Family()) {
        NS_Free(aFontData);
        return nsnull;
    }

    gfxFontEntry *fe = nsnull;

    gfxUserFontType fontType =
        gfxFontUtils::DetermineFontDataType(aFontData, aLength);

    
    
    
    
    
    
    nsTArray<PRUint8> metadata;
    PRUint32 metaOrigLen = 0;
    if (fontType == GFX_USERFONT_WOFF) {
        CopyWOFFMetadata(aFontData, aLength, &metadata, &metaOrigLen);
    }

    
    

    
    
    
    nsAutoString originalFullName;

    if (gfxPlatform::GetPlatform()->SanitizeDownloadedFonts()) {
       
        
        PRUint32 saneLen;
        const PRUint8* saneData =
            SanitizeOpenTypeData(aFontData, aLength, saneLen,
                                 fontType == GFX_USERFONT_WOFF);
        if (!saneData) {
            LogMessage(aProxy, "rejected by sanitizer");
        }
        if (saneData) {
            
            
            
            
            gfxFontUtils::GetFullNameFromSFNT(saneData, saneLen,
                                              originalFullName);
            
            
            fe = gfxPlatform::GetPlatform()->MakePlatformFont(aProxy,
                                                              saneData,
                                                              saneLen);
            if (!fe) {
                LogMessage(aProxy, "not usable by platform");
            }
        }
    } else {
        
        
        
        aFontData = PrepareOpenTypeData(aFontData, &aLength);

        if (aFontData) {
            if (gfxFontUtils::ValidateSFNTHeaders(aFontData, aLength)) {
                
                
                gfxFontUtils::GetFullNameFromSFNT(aFontData, aLength,
                                                  originalFullName);
                
                
                fe = gfxPlatform::GetPlatform()->MakePlatformFont(aProxy,
                                                                  aFontData,
                                                                  aLength);
                if (!fe) {
                    LogMessage(aProxy, "not usable by platform");
                }
                aFontData = nsnull; 
            } else {
                
                
                LogMessage(aProxy, "SFNT header or tables invalid");
            }
        }
    }

    if (aFontData) {
        NS_Free((void*)aFontData);
        aFontData = nsnull;
    }

    if (fe) {
        
        
        fe->mFeatureSettings.AppendElements(aProxy->mFeatureSettings);
        fe->mLanguageOverride = aProxy->mLanguageOverride;
        StoreUserFontData(fe, aProxy, originalFullName,
                          &metadata, metaOrigLen);
#ifdef PR_LOGGING
        
        
        if (LOG_ENABLED()) {
            nsCAutoString fontURI;
            aProxy->mSrcList[aProxy->mSrcIndex].mURI->GetSpec(fontURI);
            LOG(("userfonts (%p) [src %d] loaded uri: (%s) for (%s) gen: %8.8x\n",
                 this, aProxy->mSrcIndex, fontURI.get(),
                 NS_ConvertUTF16toUTF8(aProxy->mFamily->Name()).get(),
                 PRUint32(mGeneration)));
        }
#endif
        ReplaceFontEntry(aProxy, fe);
    } else {
#ifdef PR_LOGGING
        if (LOG_ENABLED()) {
            nsCAutoString fontURI;
            aProxy->mSrcList[aProxy->mSrcIndex].mURI->GetSpec(fontURI);
            LOG(("userfonts (%p) [src %d] failed uri: (%s) for (%s)"
                 " error making platform font\n",
                 this, aProxy->mSrcIndex, fontURI.get(),
                 NS_ConvertUTF16toUTF8(aProxy->mFamily->Name()).get()));
        }
#endif
    }

    return fe;
}

gfxMixedFontFamily*
gfxUserFontSet::GetFamily(const nsAString& aFamilyName) const
{
    nsAutoString key(aFamilyName);
    ToLowerCase(key);

    return mFontFamilies.GetWeak(key);
}

