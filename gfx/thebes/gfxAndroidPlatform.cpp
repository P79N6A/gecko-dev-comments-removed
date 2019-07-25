





































#include <android/log.h>

#include <sys/types.h>
#include <dirent.h>

#include "gfxAndroidPlatform.h"

#include "cairo.h"
#include "cairo-ft.h"

#include "gfxImageSurface.h"

#include "nsUnicharUtils.h"

#include "nsMathUtils.h"
#include "nsTArray.h"

#include "qcms.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#include "gfxFT2Fonts.h"
#include "gfxPlatformFontList.h"
#include "gfxFT2FontList.h"

static FT_Library gPlatformFTLibrary = NULL;

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gecko" , ## args)

gfxAndroidPlatform::gfxAndroidPlatform()
{
    FT_Init_FreeType(&gPlatformFTLibrary);

    mFonts.Init(200);
    mFontAliases.Init(20);
    mFontSubstitutes.Init(50);
    mPrefFonts.Init(10);

    UpdateFontList();
}

gfxAndroidPlatform::~gfxAndroidPlatform()
{
    cairo_debug_reset_static_data();

    FT_Done_FreeType(gPlatformFTLibrary);
    gPlatformFTLibrary = NULL;
}

already_AddRefed<gfxASurface>
gfxAndroidPlatform::CreateOffscreenSurface(const gfxIntSize& size,
                                      gfxASurface::gfxContentType contentType)
{
    nsRefPtr<gfxASurface> newSurface;
    if (contentType == gfxImageSurface::CONTENT_COLOR)
        newSurface = new gfxImageSurface (size, gfxASurface::ImageFormatRGB16_565);
    else
        newSurface = new gfxImageSurface (size, gfxASurface::FormatFromContent(contentType));

    return newSurface.forget();
}

struct FontListData {
    FontListData(nsIAtom *aLangGroup, const nsACString& aGenericFamily, nsTArray<nsString>& aListOfFonts) :
        mLangGroup(aLangGroup), mGenericFamily(aGenericFamily), mStringArray(aListOfFonts) {}
    nsIAtom *mLangGroup;
    const nsACString& mGenericFamily;
    nsTArray<nsString>& mStringArray;
};

static PLDHashOperator
FontListHashEnumFunc(nsStringHashKey::KeyType aKey,
                     nsRefPtr<FontFamily>& aFontFamily,
                     void* userArg)
{
    FontListData *data = (FontListData*)userArg;

    
    
    
    gfxFontStyle style;
    style.language = data->mLangGroup;
    nsRefPtr<FontEntry> aFontEntry = aFontFamily->FindFontEntry(style);
    NS_ASSERTION(aFontEntry, "couldn't find any font entry in family");
    if (!aFontEntry)
        return PL_DHASH_NEXT;


    data->mStringArray.AppendElement(aFontFamily->Name());

    return PL_DHASH_NEXT;
}

nsresult
gfxAndroidPlatform::GetFontList(nsIAtom *aLangGroup,
                                const nsACString& aGenericFamily,
                                nsTArray<nsString>& aListOfFonts)
{
    FontListData data(aLangGroup, aGenericFamily, aListOfFonts);

    mFonts.Enumerate(FontListHashEnumFunc, &data);

    aListOfFonts.Sort();
    aListOfFonts.Compact();

    return NS_OK;
}


void
gfxAndroidPlatform::AppendFacesFromFontFile(const char *fileName)
{
    FT_Face dummy;
    if (FT_Err_Ok == FT_New_Face(GetFTLibrary(), fileName, -1, &dummy)) {
        for (FT_Long i = 0; i < dummy->num_faces; i++) {
            FT_Face face;
            if (FT_Err_Ok != FT_New_Face(GetFTLibrary(), fileName, 
                                         i, &face))
                continue;

            FontEntry* fe = FontEntry::CreateFontEntryFromFace(face);
            if (fe) {
                LOG("font family: %s", face->family_name);

                NS_ConvertUTF8toUTF16 name(face->family_name);
                ToLowerCase(name);

                nsRefPtr<FontFamily> ff;
                if (!mFonts.Get(name, &ff)) {
                    ff = new FontFamily(name);
                    mFonts.Put(name, ff);
                }

                ff->AddFontEntry(fe);
                ff->SetHasStyles(PR_TRUE);
            }
        }
        FT_Done_Face(dummy);
    }
}

nsresult
gfxAndroidPlatform::UpdateFontList()
{
    gfxFontCache *fc = gfxFontCache::GetCache();
    if (fc)
        fc->AgeAllGenerations();
    mFonts.Clear();
    mFontAliases.Clear();
    mFontSubstitutes.Clear();
    mPrefFonts.Clear();
    mCodepointsWithNoFonts.reset();

    

    DIR *d = opendir("/system/fonts");
    struct dirent *ent = NULL;
    while(d && (ent = readdir(d)) != NULL) {
        int namelen = strlen(ent->d_name);
        if (namelen > 4 &&
            strcasecmp(ent->d_name + namelen - 4, ".ttf") == 0)
        {
            nsCString s("/system/fonts");
            s.Append("/");
            s.Append(nsDependentCString(ent->d_name));

            LOG("Font: %s", nsPromiseFlatCString(s).get());

            AppendFacesFromFontFile(nsPromiseFlatCString(s).get());
        }
    }

    
    

    
    mCodepointsWithNoFonts.SetRange(0,0x1f);     
    mCodepointsWithNoFonts.SetRange(0x7f,0x9f);  

    return NS_OK;
}

nsresult
gfxAndroidPlatform::ResolveFontName(const nsAString& aFontName,
                                    FontResolverCallback aCallback,
                                    void *aClosure,
                                    PRBool& aAborted)
{
    if (aFontName.IsEmpty())
        return NS_ERROR_FAILURE;

    nsAutoString resolvedName;
    gfxPlatformFontList* platformFontList = gfxPlatformFontList::PlatformFontList();
    if (platformFontList) {
        if (!platformFontList->ResolveFontName(aFontName, resolvedName)) {
            aAborted = PR_FALSE;
            return NS_OK;
        }
    }

    nsAutoString keyName(aFontName);
    ToLowerCase(keyName);

    nsRefPtr<FontFamily> ff;
    if (mFonts.Get(keyName, &ff) ||
        mFontSubstitutes.Get(keyName, &ff) ||
        mFontAliases.Get(keyName, &ff))
    {
        aAborted = !(*aCallback)(ff->Name(), aClosure);
    } else {
        aAborted = PR_FALSE;
    }

    return NS_OK;
}

static PRBool SimpleResolverCallback(const nsAString& aName, void* aClosure)
{
    nsString *result = static_cast<nsString*>(aClosure);
    result->Assign(aName);
    return PR_FALSE;
}

nsresult
gfxAndroidPlatform::GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName)
{
    aFamilyName.Truncate();
    PRBool aborted;
    return ResolveFontName(aFontName, SimpleResolverCallback, &aFamilyName, aborted);
}

gfxPlatformFontList*
gfxAndroidPlatform::CreatePlatformFontList()
{
    return new gfxFT2FontList();
}

PRBool
gfxAndroidPlatform::IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags)
{
    
    NS_ASSERTION(!(aFormatFlags & gfxUserFontSet::FLAG_FORMAT_NOT_USED),
                 "strange font format hint set");

    
    if (aFormatFlags & (gfxUserFontSet::FLAG_FORMAT_OPENTYPE | 
                        gfxUserFontSet::FLAG_FORMAT_TRUETYPE)) {
        return PR_TRUE;
    }

    
    if (aFormatFlags != 0) {
        return PR_FALSE;
    }

    
    return PR_TRUE;
}

gfxFontGroup *
gfxAndroidPlatform::CreateFontGroup(const nsAString &aFamilies,
                               const gfxFontStyle *aStyle,
                               gfxUserFontSet* aUserFontSet)
{
    return new gfxFT2FontGroup(aFamilies, aStyle, aUserFontSet);
}

FT_Library
gfxAndroidPlatform::GetFTLibrary()
{
    return gPlatformFTLibrary;
}

FontFamily *
gfxAndroidPlatform::FindFontFamily(const nsAString& aName)
{
    nsAutoString name(aName);
    ToLowerCase(name);

    nsRefPtr<FontFamily> ff;
    if (!mFonts.Get(name, &ff) &&
        !mFontSubstitutes.Get(name, &ff) &&
        !mFontAliases.Get(name, &ff)) {
        return nsnull;
    }
    return ff.get();
}

FontEntry *
gfxAndroidPlatform::FindFontEntry(const nsAString& aName, const gfxFontStyle& aFontStyle)
{
    nsRefPtr<FontFamily> ff = FindFontFamily(aName);
    if (!ff)
        return nsnull;

    return ff->FindFontEntry(aFontStyle);
}

static PLDHashOperator
FindFontForCharProc(nsStringHashKey::KeyType aKey,
                    nsRefPtr<FontFamily>& aFontFamily,
                    void* aUserArg)
{
    FontSearch *data = (FontSearch*)aUserArg;
    aFontFamily->FindFontForChar(data);
    return PL_DHASH_NEXT;
}

already_AddRefed<gfxFont>
gfxAndroidPlatform::FindFontForChar(PRUint32 aCh, gfxFont *aFont)
{
    
    if (mCodepointsWithNoFonts.test(aCh)) {
        return nsnull;
    }

    FontSearch data(aCh, aFont);

    
    mFonts.Enumerate(FindFontForCharProc, &data);

    if (data.mBestMatch) {
        nsRefPtr<gfxFT2Font> font =
            gfxFT2Font::GetOrMakeFont(static_cast<FontEntry*>(data.mBestMatch.get()), 
                                      aFont->GetStyle()); 
        gfxFont* ret = font.forget().get();
        return already_AddRefed<gfxFont>(ret);
    }

    
    mCodepointsWithNoFonts.set(aCh);

    return nsnull;
}

gfxFontEntry* 
gfxAndroidPlatform::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                     const PRUint8 *aFontData, PRUint32 aLength)
{
    return gfxPlatformFontList::PlatformFontList()->MakePlatformFont(aProxyEntry,
                                                                     aFontData,
                                                                     aLength);
}

PRBool
gfxAndroidPlatform::GetPrefFontEntries(const nsCString& aKey, nsTArray<nsRefPtr<gfxFontEntry> > *aFontEntryList)
{
    return mPrefFonts.Get(aKey, aFontEntryList);
}

void
gfxAndroidPlatform::SetPrefFontEntries(const nsCString& aKey, nsTArray<nsRefPtr<gfxFontEntry> >& aFontEntryList)
{
    mPrefFonts.Put(aKey, aFontEntryList);
}
