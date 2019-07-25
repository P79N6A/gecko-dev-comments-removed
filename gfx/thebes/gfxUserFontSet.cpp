





































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "gfxUserFontSet.h"
#include "gfxPlatform.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "prlong.h"

#include "woff.h"

#include "opentype-sanitiser.h"
#include "ots-memory-stream.h"

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo *gUserFontsLog = PR_NewLogModule("userfonts");
#endif 

#define LOG(args) PR_LOG(gUserFontsLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUserFontsLog, PR_LOG_DEBUG)

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
      mLoadingState(NOT_LOADING)
{
    mIsProxy = PR_TRUE;
    mSrcList = aFontFaceSrcList;
    mSrcIndex = 0;
    mWeight = aWeight;
    mStretch = aStretch;
    mItalic = (aItalicStyle & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) != 0;
    mFeatureSettings.AppendElements(aFeatureSettings);
    mLanguageOverride = aLanguageOverride;
    mIsUserFont = PR_TRUE;
}

gfxProxyFontEntry::~gfxProxyFontEntry()
{
}

gfxFont*
gfxProxyFontEntry::CreateFontInstance(const gfxFontStyle *aFontStyle, PRBool aNeedsBold)
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

void
gfxUserFontSet::AddFontFace(const nsAString& aFamilyName,
                            const nsTArray<gfxFontFaceSrc>& aFontFaceSrcList,
                            PRUint32 aWeight,
                            PRUint32 aStretch,
                            PRUint32 aItalicStyle,
                            const nsString& aFeatureSettings,
                            const nsString& aLanguageOverride,
                            gfxSparseBitSet *aUnicodeRanges)
{
    nsAutoString key(aFamilyName);
    ToLowerCase(key);

    PRBool found;

    if (aWeight == 0)
        aWeight = FONT_WEIGHT_NORMAL;

    

    gfxMixedFontFamily *family = mFontFamilies.GetWeak(key, &found);
    if (!family) {
        family = new gfxMixedFontFamily(aFamilyName);
        mFontFamilies.Put(key, family);
    }

    
    if (family) {
        nsTArray<gfxFontFeature> featureSettings;
        gfxFontStyle::ParseFontFeatureSettings(aFeatureSettings,
                                               featureSettings);
        PRUint32 languageOverride =
            gfxFontStyle::ParseFontLanguageOverride(aLanguageOverride);
        gfxProxyFontEntry *proxyEntry = 
            new gfxProxyFontEntry(aFontFaceSrcList, family, aWeight, aStretch, 
                                  aItalicStyle,
                                  featureSettings,
                                  languageOverride,
                                  aUnicodeRanges);
        family->AddFontEntry(proxyEntry);
#ifdef PR_LOGGING
        if (LOG_ENABLED()) {
            LOG(("userfonts (%p) added (%s) with style: %s weight: %d stretch: %d", 
                 this, NS_ConvertUTF16toUTF8(aFamilyName).get(), 
                 (aItalicStyle & FONT_STYLE_ITALIC ? "italic" : 
                     (aItalicStyle & FONT_STYLE_OBLIQUE ? "oblique" : "normal")), 
                 aWeight, aStretch));
        }
#endif
    }
}

gfxFontEntry*
gfxUserFontSet::FindFontEntry(const nsAString& aName, 
                              const gfxFontStyle& aFontStyle, 
                              PRBool& aFoundFamily,
                              PRBool& aNeedsBold,
                              PRBool& aWaitForUserFont)
{
    aWaitForUserFont = PR_FALSE;
    gfxMixedFontFamily *family = GetFamily(aName);

    
    if (!family) {
        aFoundFamily = PR_FALSE;
        return nsnull;
    }

    aFoundFamily = PR_TRUE;
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
    if (ots::Process(&output, aData, aLength,
        gfxPlatform::GetPlatform()->PreserveOTLTablesWhenSanitizing())) {
        aSaneLength = output.Tell();
        return static_cast<PRUint8*>(output.forget());
    } else {
        aSaneLength = 0;
        return nsnull;
    }
}










static void
CacheLayoutTablesFromSFNT(const PRUint8* aFontData, PRUint32 aLength,
                          gfxFontEntry* aFontEntry)
{
    const SFNTHeader *sfntHeader = reinterpret_cast<const SFNTHeader*>(aFontData);
    PRUint16 numTables = sfntHeader->numTables;
    
    
    const TableDirEntry *dirEntry = 
        reinterpret_cast<const TableDirEntry*>(aFontData + sizeof(SFNTHeader));

    while (numTables-- > 0) {
        switch (dirEntry->tag) {
        case TRUETYPE_TAG('G','D','E','F'):
        case TRUETYPE_TAG('G','P','O','S'):
        case TRUETYPE_TAG('G','S','U','B'): {
                FallibleTArray<PRUint8> buffer;
                if (!buffer.AppendElements(aFontData + dirEntry->offset,
                                           dirEntry->length)) {
                    NS_WARNING("failed to cache font table - out of memory?");
                    break;
                }
                aFontEntry->PreloadFontTable(dirEntry->tag, buffer);
            }
            break;

        default:
            if (dirEntry->tag > TRUETYPE_TAG('G','S','U','B')) {
                
                
                numTables = 0;
            }
            break;
        }
        ++dirEntry;
    }
}




static void
PreloadTableFromWOFF(const PRUint8* aFontData, PRUint32 aLength,
                     PRUint32 aTableTag, gfxFontEntry* aFontEntry)
{
    PRUint32 status = eWOFF_ok;
    PRUint32 len = woffGetTableSize(aFontData, aLength, aTableTag, &status);
    if (WOFF_SUCCESS(status) && len > 0) {
        FallibleTArray<PRUint8> buffer;
        if (!buffer.AppendElements(len)) {
            NS_WARNING("failed to cache font table - out of memory?");
            return;
        }
        woffGetTableToBuffer(aFontData, aLength, aTableTag,
                             buffer.Elements(), buffer.Length(),
                             &len, &status);
        if (WOFF_FAILURE(status)) {
            NS_WARNING("failed to cache font table - WOFF decoding error?");
            return;
        }
        aFontEntry->PreloadFontTable(aTableTag, buffer);
    }
}

static void
CacheLayoutTablesFromWOFF(const PRUint8* aFontData, PRUint32 aLength,
                          gfxFontEntry* aFontEntry)
{
    PreloadTableFromWOFF(aFontData, aLength, TRUETYPE_TAG('G','D','E','F'),
                         aFontEntry);
    PreloadTableFromWOFF(aFontData, aLength, TRUETYPE_TAG('G','P','O','S'),
                         aFontEntry);
    PreloadTableFromWOFF(aFontData, aLength, TRUETYPE_TAG('G','S','U','B'),
                         aFontEntry);
}




PRBool 
gfxUserFontSet::OnLoadComplete(gfxFontEntry *aFontToLoad,
                               const PRUint8 *aFontData, PRUint32 aLength, 
                               nsresult aDownloadStatus)
{
    NS_ASSERTION(aFontToLoad->mIsProxy,
                 "trying to load font data for wrong font entry type");

    if (!aFontToLoad->mIsProxy) {
        NS_Free((void*)aFontData);
        return PR_FALSE;
    }

    gfxProxyFontEntry *pe = static_cast<gfxProxyFontEntry*> (aFontToLoad);

    
    if (NS_SUCCEEDED(aDownloadStatus)) {
        gfxFontEntry *fe = nsnull;

        
        

        if (gfxPlatform::GetPlatform()->SanitizeDownloadedFonts()) {
            gfxUserFontType fontType =
                gfxFontUtils::DetermineFontDataType(aFontData, aLength);

            
            
            PRUint32 saneLen;
            const PRUint8* saneData =
                SanitizeOpenTypeData(aFontData, aLength, saneLen,
                                     fontType == GFX_USERFONT_WOFF);
#ifdef DEBUG
            if (!saneData) {
                char buf[1000];
                sprintf(buf, "downloaded font rejected for \"%s\"",
                        NS_ConvertUTF16toUTF8(pe->FamilyName()).get());
                NS_WARNING(buf);
            }
#endif
            if (saneData) {
                
                
                fe = gfxPlatform::GetPlatform()->MakePlatformFont(pe,
                                                                  saneData,
                                                                  saneLen);
                if (fe) {
                    
                    
                    
                    
                    switch (fontType) {
                    case GFX_USERFONT_OPENTYPE:
                        CacheLayoutTablesFromSFNT(aFontData, aLength, fe);
                        break;

                    case GFX_USERFONT_WOFF:
                        CacheLayoutTablesFromWOFF(aFontData, aLength, fe);
                        break;

                    default:
                        break;
                    }
                } else {
                    NS_WARNING("failed to make platform font from download");
                }
            }
        } else {
            
            
            
            aFontData = PrepareOpenTypeData(aFontData, &aLength);

            if (aFontData) {
                if (gfxFontUtils::ValidateSFNTHeaders(aFontData, aLength)) {
                    
                    
                    fe = gfxPlatform::GetPlatform()->MakePlatformFont(pe,
                                                                      aFontData,
                                                                      aLength);
                    aFontData = nsnull; 
                } else {
                    
                    
                    NS_WARNING("failed to make platform font from download");
                }
            }
        }

        if (aFontData) {
            NS_Free((void*)aFontData);
            aFontData = nsnull;
        }

        if (fe) {
            
            
            fe->mFeatureSettings.AppendElements(pe->mFeatureSettings);
            fe->mLanguageOverride = pe->mLanguageOverride;

            static_cast<gfxMixedFontFamily*>(pe->mFamily)->ReplaceFontEntry(pe, fe);
            IncrementGeneration();
#ifdef PR_LOGGING
            if (LOG_ENABLED()) {
                nsCAutoString fontURI;
                pe->mSrcList[pe->mSrcIndex].mURI->GetSpec(fontURI);
                LOG(("userfonts (%p) [src %d] loaded uri: (%s) for (%s) gen: %8.8x\n",
                     this, pe->mSrcIndex, fontURI.get(),
                     NS_ConvertUTF16toUTF8(pe->mFamily->Name()).get(),
                     PRUint32(mGeneration)));
            }
#endif
            return PR_TRUE;
        } else {
#ifdef PR_LOGGING
            if (LOG_ENABLED()) {
                nsCAutoString fontURI;
                pe->mSrcList[pe->mSrcIndex].mURI->GetSpec(fontURI);
                LOG(("userfonts (%p) [src %d] failed uri: (%s) for (%s) error making platform font\n",
                     this, pe->mSrcIndex, fontURI.get(),
                     NS_ConvertUTF16toUTF8(pe->mFamily->Name()).get()));
            }
#endif
        }
    } else {
        
#ifdef PR_LOGGING
        if (LOG_ENABLED()) {
            nsCAutoString fontURI;
            pe->mSrcList[pe->mSrcIndex].mURI->GetSpec(fontURI);
            LOG(("userfonts (%p) [src %d] failed uri: (%s) for (%s) error %8.8x downloading font data\n",
                 this, pe->mSrcIndex, fontURI.get(),
                 NS_ConvertUTF16toUTF8(pe->mFamily->Name()).get(),
                 aDownloadStatus));
        }
#endif
    }

    if (aFontData) {
        NS_Free((void*)aFontData);
    }

    
    LoadStatus status;

    status = LoadNext(pe);

    
    
    
    IncrementGeneration();
    return PR_TRUE;
}


gfxUserFontSet::LoadStatus
gfxUserFontSet::LoadNext(gfxProxyFontEntry *aProxyEntry)
{
    PRUint32 numSrc = aProxyEntry->mSrcList.Length();

    NS_ASSERTION(aProxyEntry->mSrcIndex < numSrc, "already at the end of the src list for user font");

    if (aProxyEntry->mLoadingState == gfxProxyFontEntry::NOT_LOADING) {
        aProxyEntry->mLoadingState = gfxProxyFontEntry::LOADING_STARTED;
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
                static_cast<gfxMixedFontFamily*>(aProxyEntry->mFamily)->ReplaceFontEntry(aProxyEntry, fe);
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
                nsresult rv = StartLoad(aProxyEntry, &currSrc);
                PRBool loadOK = NS_SUCCEEDED(rv);
                
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
#ifdef PR_LOGGING
                    if (LOG_ENABLED()) {
                        nsCAutoString fontURI;
                        currSrc.mURI->GetSpec(fontURI);
                        LOG(("userfonts (%p) [src %d] failed uri: (%s) for (%s) download failed\n", 
                             this, aProxyEntry->mSrcIndex, fontURI.get(), 
                             NS_ConvertUTF16toUTF8(aProxyEntry->mFamily->Name()).get()));
                    }
#endif
                }
            } else {
#ifdef PR_LOGGING
                if (LOG_ENABLED()) {
                    nsCAutoString fontURI;
                    currSrc.mURI->GetSpec(fontURI);
                    LOG(("userfonts (%p) [src %d] failed uri: (%s) for (%s) format not supported\n", 
                         this, aProxyEntry->mSrcIndex, fontURI.get(), 
                         NS_ConvertUTF16toUTF8(aProxyEntry->mFamily->Name()).get()));
                }
#endif
            }
        }

        aProxyEntry->mSrcIndex++;
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


gfxMixedFontFamily*
gfxUserFontSet::GetFamily(const nsAString& aFamilyName) const
{
    nsAutoString key(aFamilyName);
    ToLowerCase(key);

    return mFontFamilies.GetWeak(key);
}


void 
gfxUserFontSet::RemoveFamily(const nsAString& aFamilyName)
{
    nsAutoString key(aFamilyName);
    ToLowerCase(key);

    mFontFamilies.Remove(key);
}
