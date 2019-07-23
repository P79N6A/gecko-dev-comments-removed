









































#ifdef MOZ_ENABLE_XFT
#include "nsFontMetricsXft.h"
#include "nsIPref.h"
#include "nsServiceManagerUtils.h"
#include "prenv.h"
#endif 

#ifdef MOZ_ENABLE_COREXFONTS
#include "nsFontMetricsGTK.h"
#endif

#ifdef MOZ_ENABLE_PANGO
#include "nsFontMetricsPango.h"
#include "prenv.h"
#endif

#include "nsFontMetricsUtils.h"

PRUint32
NS_FontMetricsGetHints(void)
{
#ifdef MOZ_ENABLE_PANGO
    if (NS_IsPangoEnabled()) {
        return nsFontMetricsPango::GetHints();
    }
#endif
#ifdef MOZ_ENABLE_XFT
    if (NS_IsXftEnabled()) {
        return nsFontMetricsXft::GetHints();
    }
#endif

#ifdef MOZ_ENABLE_COREXFONTS
    return nsFontMetricsGTK::GetHints();
#endif
}

nsresult
NS_FontMetricsFamilyExists(nsIDeviceContext *aDevice, const nsString &aName)
{
#ifdef MOZ_ENABLE_PANGO
    if (NS_IsPangoEnabled()) {
        return nsFontMetricsPango::FamilyExists(aDevice, aName);
    }
#endif
#ifdef MOZ_ENABLE_XFT
    
    if (NS_IsXftEnabled()) {
        return nsFontMetricsXft::FamilyExists(aDevice, aName);
    }
#endif

#ifdef MOZ_ENABLE_COREXFONTS
    return nsFontMetricsGTK::FamilyExists(aDevice, aName);
#endif
}

#if defined(MOZ_ENABLE_XFT) && defined(MOZ_ENABLE_COREXFONTS)

PRBool
NS_IsXftEnabled(void)
{
    static PRBool been_here = PR_FALSE;
    static PRBool cachedXftSetting = PR_TRUE;

    if (!been_here) {
        been_here = PR_TRUE;
        nsCOMPtr<nsIPref> prefService;
        prefService = do_GetService(NS_PREF_CONTRACTID);
        if (!prefService)
            return PR_TRUE;

        nsresult rv;

        rv = prefService->GetBoolPref("fonts.xft.enabled", &cachedXftSetting);

        
        
        
        if (NS_FAILED(rv)) {
            char *val = PR_GetEnv("GDK_USE_XFT");

            if (val && val[0] == '0') {
                cachedXftSetting = PR_FALSE;
                goto end;
            }

            cachedXftSetting = PR_TRUE;
        }
    }

 end:

    return cachedXftSetting;
}

#endif

#if defined(MOZ_ENABLE_PANGO) && (defined(MOZ_ENABLE_XFT) || defined(MOZ_ENABLE_COREXFONTS))

PRBool
NS_IsPangoEnabled(void)
{
    char *val = PR_GetEnv("MOZ_DISABLE_PANGO");
    if (val)
        return FALSE;

    return TRUE;
}

#endif
