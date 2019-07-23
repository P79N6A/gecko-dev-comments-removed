



































#ifndef nsFontConfigUtils_h__
#define nsFontConfigUtils_h__

#include "nspr.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsFont.h"
#include "prlog.h"

#include <fontconfig/fontconfig.h>

struct MozGtkLangGroup {
    const char    *mozLangGroup;
    const FcChar8 *Lang;
};

extern int     NS_CalculateSlant   (PRUint8  aStyle);
extern int     NS_CalculateWeight  (PRUint16 aWeight);
extern void    NS_AddLangGroup     (FcPattern *aPattern, nsIAtom *aLangGroup);
extern void    NS_AddFFRE          (FcPattern *aPattern, nsCString *aFamily,
                                    PRBool aWeak);
extern int     NS_FFRECountHyphens (nsACString &aFFREName);
extern void    NS_AddGenericFontFromPref (const nsCString *aGenericFont,
                                          nsIAtom *aLangGroup,
                                          FcPattern *aPattern, 
                                          const PRLogModuleInfo *aLogModule);
extern const   MozGtkLangGroup* NS_FindFCLangGroup (nsACString &aLangGroup);
#endif

