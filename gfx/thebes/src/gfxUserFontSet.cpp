




































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "gfxUserFontSet.h"
#include "gfxPlatform.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "prlong.h"

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
             gfxSparseBitSet *aUnicodeRanges)
    : gfxFontEntry(NS_LITERAL_STRING("Proxy"), aFamily), mIsLoading(PR_FALSE)
{
    mIsProxy = PR_TRUE;
    mSrcList = aFontFaceSrcList;
    mSrcIndex = 0;
    mWeight = aWeight;
    mStretch = aStretch;
    mItalic = (aItalicStyle & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) != 0;
    mIsUserFont = PR_TRUE;
}

gfxProxyFontEntry::~gfxProxyFontEntry()
{

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
        gfxProxyFontEntry *proxyEntry = 
            new gfxProxyFontEntry(aFontFaceSrcList, family, aWeight, aStretch, 
                                  aItalicStyle, aUnicodeRanges);
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
                              PRBool& aNeedsBold)
{
    gfxMixedFontFamily *family = GetFamily(aName);

    
    if (!family)
        return nsnull;

    gfxFontEntry* fe = family->FindFontForStyle(aFontStyle, aNeedsBold);

    
    if (!fe->mIsProxy)
        return fe;

    gfxProxyFontEntry *proxyEntry = static_cast<gfxProxyFontEntry*> (fe);

    
    if (proxyEntry->mIsLoading)
        return nsnull;

    
    LoadStatus status;

    status = LoadNext(proxyEntry);

    
    
    if (status == STATUS_LOADED)
        return family->FindFontForStyle(aFontStyle, aNeedsBold);

    
    return nsnull;
}


PRBool 
gfxUserFontSet::OnLoadComplete(gfxFontEntry *aFontToLoad,
                               nsISupports *aLoader,
                               const PRUint8 *aFontData, PRUint32 aLength, 
                               nsresult aDownloadStatus)
{
    NS_ASSERTION(aFontToLoad->mIsProxy, "trying to load font data for wrong font entry type");

    if (!aFontToLoad->mIsProxy)
        return PR_FALSE;

    gfxProxyFontEntry *pe = static_cast<gfxProxyFontEntry*> (aFontToLoad);

    
    if (NS_SUCCEEDED(aDownloadStatus)) {
        gfxFontEntry *fe = 
            gfxPlatform::GetPlatform()->MakePlatformFont(pe, aLoader,
                                                         aFontData, aLength);
        if (fe) {
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

    
    LoadStatus status;

    status = LoadNext(pe);
    if (status == STATUS_LOADED) {
        
        
        IncrementGeneration();
        return PR_TRUE;
    }

    return PR_FALSE;
}


gfxUserFontSet::LoadStatus
gfxUserFontSet::LoadNext(gfxProxyFontEntry *aProxyEntry)
{
    PRUint32 numSrc = aProxyEntry->mSrcList.Length();

    NS_ASSERTION(aProxyEntry->mSrcIndex < numSrc, "already at the end of the src list for user font");

    if (aProxyEntry->mIsLoading) {
        aProxyEntry->mSrcIndex++;
    } else {
        aProxyEntry->mIsLoading = PR_TRUE;
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

    gfxMixedFontFamily *family = static_cast<gfxMixedFontFamily*>(aProxyEntry->mFamily);

    family->RemoveFontEntry(aProxyEntry);

    
    if (family->mAvailableFonts.Length() == 0) {
        LOG(("userfonts (%p) failed all faces, remove family (%s)\n", 
             this, NS_ConvertUTF16toUTF8(family->Name()).get()));            
        RemoveFamily(family->Name());
    }

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
