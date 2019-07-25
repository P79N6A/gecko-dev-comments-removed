





































#ifndef nsProxyRelease_h__
#define nsProxyRelease_h__

#include "nsIEventTarget.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

#ifdef XPCOM_GLUE_AVOID_NSPR
#error NS_ProxyRelease implementation depends on NSPR.
#endif






template <class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease
    (nsIEventTarget *target, nsCOMPtr<T> &doomed, bool alwaysProxy=false)
{
   T* raw = nsnull;
   doomed.swap(raw);
   return NS_ProxyRelease(target, raw, alwaysProxy);
}






template <class T>
inline NS_HIDDEN_(nsresult)
NS_ProxyRelease
    (nsIEventTarget *target, nsRefPtr<T> &doomed, bool alwaysProxy=false)
{
   T* raw = nsnull;
   doomed.swap(raw);
   return NS_ProxyRelease(target, raw, alwaysProxy);
}














NS_COM_GLUE nsresult
NS_ProxyRelease
    (nsIEventTarget *target, nsISupports *doomed, bool alwaysProxy=false);

#endif
