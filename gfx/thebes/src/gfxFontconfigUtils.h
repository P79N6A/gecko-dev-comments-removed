




































#ifndef GFX_FONTCONFIG_UTILS_H
#define GFX_FONTCONFIG_UTILS_H

#include "gfxPlatform.h"

#include "nsTArray.h"
#include "nsDataHashtable.h"

class gfxFontNameList : public nsTArray<nsString>
{
public:
    THEBES_INLINE_DECL_REFCOUNTING(gfxFontList)
    PRBool Exists(nsAString& aName);
};

class gfxFontconfigUtils {
public:
    gfxFontconfigUtils();

    static gfxFontconfigUtils* GetFontconfigUtils() {
        if (!sUtils)
            sUtils = new gfxFontconfigUtils();
        return sUtils;
    }

    static void Shutdown() {
        delete sUtils;
        sUtils = nsnull;
    }

    nsresult GetFontList(const nsACString& aLangGroup,
                         const nsACString& aGenericFamily,
                         nsStringArray& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             gfxPlatform::FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

protected:
    static gfxFontconfigUtils* sUtils;

    PRInt32 IsExistingFont(const nsACString& aFontName);
    nsresult GetResolvedFonts(const nsACString& aName,
                              gfxFontNameList* aResult);

    nsresult GetFontListInternal(nsCStringArray& aListOfFonts,
                                 const nsACString *aLangGroup = nsnull);
    nsresult UpdateFontListInternal(PRBool aForce = PR_FALSE);

    nsCStringArray mFonts;
    nsCStringArray mNonExistingFonts;
    nsCStringArray mAliasForSingleFont;
    nsCStringArray mAliasForMultiFonts;

    nsDataHashtable<nsCStringHashKey, nsRefPtr<gfxFontNameList> > mAliasTable;
};

#endif 
