



































#include "nsServiceManagerUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsFontConfigUtils.h"

const MozGtkLangGroup MozGtkLangGroups[] = {
    { "x-western",      (const FcChar8 *)"en" },
    { "x-central-euro", (const FcChar8 *)"pl" },
    { "x-cyrillic",     (const FcChar8 *)"ru" },
    { "x-baltic",       (const FcChar8 *)"lv" },
    { "x-devanagari",   (const FcChar8 *)"hi" },
    { "x-tamil",        (const FcChar8 *)"ta" },
    { "x-armn",         (const FcChar8 *)"hy" },
    { "x-beng",         (const FcChar8 *)"bn" },
    { "x-cans",         (const FcChar8 *)"iu" },
    { "x-ethi",         (const FcChar8 *)"am" },
    { "x-geor",         (const FcChar8 *)"ka" },
    { "x-gujr",         (const FcChar8 *)"gu" },
    { "x-guru",         (const FcChar8 *)"pa" },
    { "x-khmr",         (const FcChar8 *)"km" },
    { "x-mlym",         (const FcChar8 *)"ml" },
    { "x-unicode",                       0    },
    { "x-user-def",                      0    },
};

#define NUM_GTK_LANG_GROUPS (sizeof (MozGtkLangGroups) / \
                             sizeof (MozGtkLangGroups[0]))

static 
void FFREToFamily(nsACString &aFFREName, nsACString &oFamily);

const MozGtkLangGroup*
NS_FindFCLangGroup (nsACString &aLangGroup)
{
    for (unsigned int i=0; i < NUM_GTK_LANG_GROUPS; ++i) {
        if (aLangGroup.Equals(MozGtkLangGroups[i].mozLangGroup,
                              nsCaseInsensitiveCStringComparator())) {
            return &MozGtkLangGroups[i];
        }
    }

    return nsnull;
}

int
NS_CalculateSlant(PRUint8 aStyle)
{
    int fcSlant;

    switch(aStyle) {
    case NS_FONT_STYLE_ITALIC:
        fcSlant = FC_SLANT_ITALIC;
        break;
    case NS_FONT_STYLE_OBLIQUE:
        fcSlant = FC_SLANT_OBLIQUE;
        break;
    default:
        fcSlant = FC_SLANT_ROMAN;
        break;
    }

    return fcSlant;
}

int
NS_CalculateWeight (PRUint16 aWeight)
{
    







    PRInt32 baseWeight = (aWeight + 50) / 100;
    PRInt32 offset = aWeight - baseWeight * 100;

    
    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    
    static int fcWeightLookup[10] = {
        0, 0, 0, 0, 1, 1, 2, 3, 3, 4,
    };

    PRInt32 fcWeight = fcWeightLookup[baseWeight];

    



    fcWeight += offset;
    if (fcWeight < 0)
        fcWeight = 0;
    if (fcWeight > 4)
        fcWeight = 4;

    
    static int fcWeights[5] = {
        FC_WEIGHT_LIGHT,      
        FC_WEIGHT_MEDIUM,     
        FC_WEIGHT_DEMIBOLD,   
        FC_WEIGHT_BOLD,       
        FC_WEIGHT_BLACK,      
    };

    return fcWeights[fcWeight];
}

void
NS_AddLangGroup(FcPattern *aPattern, nsIAtom *aLangGroup)
{
    
    nsCAutoString cname;
    aLangGroup->ToUTF8String(cname);

    
    
    const struct MozGtkLangGroup *langGroup;
    langGroup = NS_FindFCLangGroup(cname);

    
    
    
    
    
    if (!langGroup)
        FcPatternAddString(aPattern, FC_LANG, (FcChar8 *)cname.get());
    else if (langGroup->Lang)
        FcPatternAddString(aPattern, FC_LANG, (FcChar8 *)langGroup->Lang);
}

void
NS_AddFFRE(FcPattern *aPattern, nsCString *aFamily, PRBool aWeak)
{
    nsCAutoString family;
    FFREToFamily(*aFamily, family);

    FcValue v;
    v.type = FcTypeString;
    
    v.u.s = (FcChar8 *)family.get();

    if (aWeak)
        FcPatternAddWeak(aPattern, FC_FAMILY, v, FcTrue);
    else
        FcPatternAdd(aPattern, FC_FAMILY, v, FcTrue);
}


void
FFREToFamily(nsACString &aFFREName, nsACString &oFamily)
{
  if (NS_FFRECountHyphens(aFFREName) == 3) {
      PRInt32 familyHyphen = aFFREName.FindChar('-') + 1;
      PRInt32 registryHyphen = aFFREName.FindChar('-',familyHyphen);
      oFamily.Append(Substring(aFFREName, familyHyphen,
                               registryHyphen-familyHyphen));
  }
  else {
      oFamily.Append(aFFREName);
  }
}

int
NS_FFRECountHyphens (nsACString &aFFREName)
{
    int h = 0;
    PRInt32 hyphen = 0;
    while ((hyphen = aFFREName.FindChar('-', hyphen)) >= 0) {
        ++h;
        ++hyphen;
    }
    return h;
}

inline static void 
AddFFREandLog(FcPattern *aPattern, nsCString aFamily,
              const PRLogModuleInfo *aLogModule)
{
    
    
    if (NS_FFRECountHyphens(aFamily) >= 3)
        return;

    if (aLogModule && PR_LOG_TEST(aLogModule, PR_LOG_DEBUG)) {
        printf("\tadding generic font from preferences: %s\n",
               aFamily.get());
    }

    NS_AddFFRE(aPattern, &aFamily, PR_FALSE);
}



void 
NS_AddGenericFontFromPref(const nsCString *aGenericFont,
                          nsIAtom *aLangGroup, FcPattern *aPattern, 
                          const PRLogModuleInfo *aLogModule)
{
    nsCOMPtr<nsIPrefService> prefService;
    prefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefService)
        return;
    nsCOMPtr<nsIPrefBranch> pref;
    if (NS_FAILED(prefService->GetBranch("font.", getter_AddRefs(pref))))
        return;

    nsCAutoString genericDotLangGroup(aGenericFont->get());
    genericDotLangGroup.Append('.');
    nsAutoString langGroup;
    aLangGroup->ToString(langGroup);
    LossyAppendUTF16toASCII(langGroup, genericDotLangGroup);

    nsCAutoString name("name.");
    name.Append(genericDotLangGroup);

    
    
    
    nsresult rv;
    nsXPIDLCString value;
    rv = pref->GetCharPref(name.get(), getter_Copies(value));

    if (NS_SUCCEEDED(rv)) {
        AddFFREandLog(aPattern, value, aLogModule);
    }

    nsCAutoString nameList("name-list.");
    nameList.Append(genericDotLangGroup);
    rv = pref->GetCharPref(nameList.get(), getter_Copies(value));

    if (NS_SUCCEEDED(rv)) {
        PRInt32 prevCommaPos = -1;
        PRInt32 commaPos; 
        nsCAutoString family;

        while ((commaPos = value.FindChar(',', prevCommaPos + 1)) > 0) {
            family = Substring(value, prevCommaPos + 1, 
                               commaPos - prevCommaPos - 1);
            prevCommaPos = commaPos;
            AddFFREandLog(aPattern, family, aLogModule);
        }

        family = Substring(value, prevCommaPos + 1);
        AddFFREandLog(aPattern, family, aLogModule);
    }
}
