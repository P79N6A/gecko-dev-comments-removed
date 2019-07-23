






































#ifndef __nsFontMetricsUtils_h
#define __nsFontMetricsUtils_h

extern PRUint32 NS_FontMetricsGetHints    ();
extern nsresult NS_FontMetricsFamilyExists(nsIDeviceContext *aDevice,
                                           const nsString &aName);
#ifdef MOZ_ENABLE_XFT
#ifdef MOZ_ENABLE_COREXFONTS
extern PRBool NS_IsXftEnabled();
#else
inline PRBool NS_IsXftEnabled() { return PR_TRUE; }
#endif
#endif

#ifdef MOZ_ENABLE_PANGO
#if defined(MOZ_ENABLE_XFT) || defined(MOZ_ENABLE_COREXFONTS)
extern PRBool NS_IsPangoEnabled();
#else
inline PRBool NS_IsPangoEnabled() { return PR_TRUE; }
#endif
#endif

#endif 
