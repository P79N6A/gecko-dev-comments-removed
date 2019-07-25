





































#include "gfxPlatformMac.h"

#include "gfxImageSurface.h"
#include "gfxQuartzSurface.h"
#include "gfxQuartzImageSurface.h"

#include "gfxMacPlatformFontList.h"
#include "gfxMacFont.h"
#include "gfxCoreTextShaper.h"
#include "gfxUserFontSet.h"

#include "nsCRT.h"
#include "nsTArray.h"
#include "nsUnicodeRange.h"

#include "mozilla/Preferences.h"

#include "qcms.h"

#include <dlfcn.h>

using namespace mozilla;


enum {
   kAutoActivationDisabled = 1
};
typedef uint32_t AutoActivationSetting;



static void 
DisableFontActivation()
{
    
    CFBundleRef mainBundle = ::CFBundleGetMainBundle();
    CFStringRef mainBundleID = NULL;

    if (mainBundle) {
        mainBundleID = ::CFBundleGetIdentifier(mainBundle);
    }

    
    void (*CTFontManagerSetAutoActivationSettingPtr)
            (CFStringRef, AutoActivationSetting);
    CTFontManagerSetAutoActivationSettingPtr =
        (void (*)(CFStringRef, AutoActivationSetting))
        dlsym(RTLD_DEFAULT, "CTFontManagerSetAutoActivationSetting");

    
    if (CTFontManagerSetAutoActivationSettingPtr) {
        CTFontManagerSetAutoActivationSettingPtr(mainBundleID,
                                                 kAutoActivationDisabled);
    }

    if (mainBundleID) {
        ::CFRelease(mainBundleID);
    }
    if (mainBundle) {
        ::CFRelease(mainBundle);
    }
}

gfxPlatformMac::gfxPlatformMac()
{
    mOSXVersion = 0;
    OSXVersion();
    if (mOSXVersion >= MAC_OS_X_VERSION_10_6_HEX) {
        DisableFontActivation();
    }
    mFontAntiAliasingThreshold = ReadAntiAliasingThreshold();
}

gfxPlatformMac::~gfxPlatformMac()
{
    gfxCoreTextShaper::Shutdown();
}

gfxPlatformFontList*
gfxPlatformMac::CreatePlatformFontList()
{
    gfxPlatformFontList* list = new gfxMacPlatformFontList();
    if (NS_SUCCEEDED(list->InitFontList())) {
        return list;
    }
    gfxPlatformFontList::Shutdown();
    return nsnull;
}

already_AddRefed<gfxASurface>
gfxPlatformMac::CreateOffscreenSurface(const gfxIntSize& size,
                                       gfxASurface::gfxContentType contentType)
{
    gfxASurface *newSurface = nsnull;

    newSurface = new gfxQuartzSurface(size, gfxASurface::FormatFromContent(contentType));

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
    
    
    PRBool useAntiAliasingThreshold = Preferences::GetBool("gfx.use_text_smoothing_setting", PR_FALSE);

    
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
