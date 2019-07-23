





































#include "gfxPlatformMac.h"

#include "gfxImageSurface.h"
#include "gfxQuartzSurface.h"
#include "gfxQuartzImageSurface.h"

#include "gfxMacPlatformFontList.h"
#include "gfxUserFontSet.h"

#ifdef MOZ_CORETEXT
#include "gfxCoreTextFonts.h"
#endif

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPrefLocalizedString.h"
#include "nsServiceManagerUtils.h"
#include "nsCRT.h"
#include "nsTArray.h"
#include "nsUnicodeRange.h"

#include "qcms.h"

gfxPlatformMac::gfxPlatformMac()
{
    mOSXVersion = 0;
    mFontAntiAliasingThreshold = ReadAntiAliasingThreshold();

#ifndef __LP64__
    
    
    mUseCoreText = PR_FALSE;

#ifdef MOZ_CORETEXT
    if (&CTLineCreateWithAttributedString != NULL) {
        mUseCoreText = PR_TRUE;
        nsCOMPtr<nsIPrefBranch> prefbranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefbranch) {
            PRBool enabled;
            nsresult rv = prefbranch->GetBoolPref("gfx.force_atsui_text", &enabled);
            if (NS_SUCCEEDED(rv) && enabled)
                mUseCoreText = PR_FALSE;
        }
    }
#ifdef DEBUG_jonathan
    printf("Using %s for font & glyph shaping support\n",
           mUseCoreText ? "CoreText" : "ATSUI");
#endif
#endif 

#endif 
}

gfxPlatformMac::~gfxPlatformMac()
{
#ifdef MOZ_CORETEXT
#ifndef __LP64__
    if (mUseCoreText)
#endif
        gfxCoreTextFont::Shutdown();
#endif
}

gfxPlatformFontList*
gfxPlatformMac::CreatePlatformFontList()
{
    return new gfxMacPlatformFontList();
}

already_AddRefed<gfxASurface>
gfxPlatformMac::CreateOffscreenSurface(const gfxIntSize& size,
                                       gfxASurface::gfxImageFormat imageFormat)
{
    gfxASurface *newSurface = nsnull;

    newSurface = new gfxQuartzSurface(size, imageFormat);

    NS_IF_ADDREF(newSurface);
    return newSurface;
}

already_AddRefed<gfxASurface>
gfxPlatformMac::OptimizeImage(gfxImageSurface *aSurface,
                              gfxASurface::gfxImageFormat format)
{
    const gfxIntSize& surfaceSize = aSurface->GetSize();
    nsRefPtr<gfxImageSurface> isurf = aSurface;

    if (format != aSurface->Format()) {
        isurf = new gfxImageSurface (surfaceSize, format);
        if (!isurf->CopyFrom (aSurface)) {
            
            NS_ADDREF(aSurface);
            return aSurface;
        }
    }

    nsRefPtr<gfxASurface> ret = new gfxQuartzImageSurface(isurf);
    return ret.forget();
}

nsresult
gfxPlatformMac::ResolveFontName(const nsAString& aFontName,
                                FontResolverCallback aCallback,
                                void *aClosure, PRBool& aAborted)
{
    nsAutoString resolvedName;
    if (!gfxPlatformFontList::PlatformFontList()->
             ResolveFontName(aFontName, resolvedName)) {
        aAborted = PR_FALSE;
        return NS_OK;
    }
    aAborted = !(*aCallback)(resolvedName, aClosure);
    return NS_OK;
}

nsresult
gfxPlatformMac::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    gfxPlatformFontList::PlatformFontList()->GetStandardFamilyName(aFontName, aFamilyName);
    return NS_OK;
}

gfxFontGroup *
gfxPlatformMac::CreateFontGroup(const nsAString &aFamilies,
                                const gfxFontStyle *aStyle,
                                gfxUserFontSet *aUserFontSet)
{
    return new gfxFontGroup(aFamilies, aStyle, aUserFontSet);
}


gfxFontEntry* 
gfxPlatformMac::LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                const nsAString& aFontName)
{
    return gfxPlatformFontList::PlatformFontList()->LookupLocalFont(aProxyEntry, 
                                                                    aFontName);
}

gfxFontEntry* 
gfxPlatformMac::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                 const PRUint8 *aFontData, PRUint32 aLength)
{
    
    
    
    return gfxPlatformFontList::PlatformFontList()->MakePlatformFont(aProxyEntry,
                                                                     aFontData,
                                                                     aLength);
}

PRBool
gfxPlatformMac::IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_WOFF     |
                        gfxUserFontSet::FLAG_FORMAT_OPENTYPE | 
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE | 
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE_AAT)) {
        return PR_TRUE;
    }

    
    if (aFormatFlags != 0) {
        return PR_FALSE;
    }

    
    return PR_TRUE;
}


nsresult
gfxPlatformMac::GetFontList(nsIAtom *aLangGroup,
                            const nsACString& aGenericFamily,
                            nsTArray<nsString>& aListOfFonts)
{
    gfxPlatformFontList::PlatformFontList()->GetFontList(aLangGroup, aGenericFamily, aListOfFonts);
    return NS_OK;
}

nsresult
gfxPlatformMac::UpdateFontList()
{
    gfxPlatformFontList::PlatformFontList()->UpdateFontList();
    return NS_OK;
}

PRInt32 
gfxPlatformMac::OSXVersion()
{
    if (!mOSXVersion) {
        
        OSErr err = ::Gestalt(gestaltSystemVersion, reinterpret_cast<SInt32*>(&mOSXVersion));
        if (err != noErr) {
            
            NS_ERROR("Couldn't determine OS X version, assuming 10.4");
            mOSXVersion = MAC_OS_X_VERSION_10_4_HEX;
        }
    }
    return mOSXVersion;
}

PRUint32
gfxPlatformMac::ReadAntiAliasingThreshold()
{
    PRUint32 threshold = 0;  
    
    
    PRBool useAntiAliasingThreshold = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        PRBool enabled;
        nsresult rv =
            prefs->GetBoolPref("gfx.use_text_smoothing_setting", &enabled);
        if (NS_SUCCEEDED(rv)) {
            useAntiAliasingThreshold = enabled;
        }
    }
    
    
    if (!useAntiAliasingThreshold)
        return threshold;
        
    
    CFNumberRef prefValue = (CFNumberRef)CFPreferencesCopyAppValue(CFSTR("AppleAntiAliasingThreshold"), kCFPreferencesCurrentApplication);

    if (prefValue) {
        if (!CFNumberGetValue(prefValue, kCFNumberIntType, &threshold)) {
            threshold = 0;
        }
        CFRelease(prefValue);
    }

    return threshold;
}

qcms_profile *
gfxPlatformMac::GetPlatformCMSOutputProfile()
{
    qcms_profile *profile = nsnull;
    CMProfileRef cmProfile;
    CMProfileLocation *location;
    UInt32 locationSize;

    












    CGDirectDisplayID displayID = CGMainDisplayID();

    CMError err = CMGetProfileByAVID(static_cast<CMDisplayIDType>(displayID), &cmProfile);
    if (err != noErr)
        return nsnull;

    
    err = NCMGetProfileLocation(cmProfile, NULL, &locationSize);
    if (err != noErr)
        return nsnull;

    
    location = static_cast<CMProfileLocation*>(malloc(locationSize));
    if (!location)
        goto fail_close;

    err = NCMGetProfileLocation(cmProfile, location, &locationSize);
    if (err != noErr)
        goto fail_location;

    switch (location->locType) {
#ifndef __LP64__
    case cmFileBasedProfile: {
        FSRef fsRef;
        if (!FSpMakeFSRef(&location->u.fileLoc.spec, &fsRef)) {
            char path[512];
            if (!FSRefMakePath(&fsRef, reinterpret_cast<UInt8*>(path), sizeof(path))) {
                profile = qcms_profile_from_path(path);
#ifdef DEBUG_tor
                if (profile)
                    fprintf(stderr,
                            "ICM profile read from %s fileLoc successfully\n", path);
#endif
            }
        }
        break;
    }
#endif
    case cmPathBasedProfile:
        profile = qcms_profile_from_path(location->u.pathLoc.path);
#ifdef DEBUG_tor
        if (profile)
            fprintf(stderr,
                    "ICM profile read from %s pathLoc successfully\n",
                    device.u.pathLoc.path);
#endif
        break;
    default:
#ifdef DEBUG_tor
        fprintf(stderr, "Unhandled ColorSync profile location\n");
#endif
        break;
    }

fail_location:
    free(location);
fail_close:
    CMCloseProfile(cmProfile);
    return profile;
}

void
gfxPlatformMac::SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString)
{
    TextBreakLocatorRef locator;
    OSStatus status = UCCreateTextBreakLocator(NULL, 0, kUCTextBreakClusterMask,
                                               &locator);
    if (status != noErr)
        return;
    UniCharArrayOffset breakOffset = 0;
    UCTextBreakOptions options = kUCTextBreakLeadingEdgeMask;
    PRUint32 length = aTextRun->GetLength();
    while (breakOffset < length) {
        UniCharArrayOffset next;
        status = UCFindTextBreak(locator, kUCTextBreakClusterMask, options,
                                 aString, length, breakOffset, &next);
        if (status != noErr)
            break;
        options |= kUCTextBreakIterateMask;
        PRUint32 i;
        for (i = breakOffset + 1; i < next; ++i) {
            gfxTextRun::CompressedGlyph g;
            
            
            
            aTextRun->SetGlyphs(i, g.SetComplex(PR_FALSE, PR_TRUE, 0), nsnull);
        }
        breakOffset = next;
    }
    UCDisposeTextBreakLocator(&locator);
}

